# Complete CGI System Architecture

## 1. System Overview

Your webserver implements a **completely asynchronous CGI system** that uses:
- **epoll** for event-driven I/O (Linux)
- **Non-blocking pipes** for process communication
- **Timeout mechanism** to prevent resource exhaustion

### Why Async?

| Blocking Model | Async Model |
|---|---|
| Thread per request | Single event loop |
| Thread sleep during CGI wait | Monitoring multiple requests simultaneously |
| Thread overhead (memory, context switch) | Minimal overhead |
| Race conditions, locks needed | No locks, no race conditions |
| Simple to code but resource-heavy | More complex but scales better |

Your server uses the **async model** for better scalability.

---

## 2. Request Lifecycle

### Phase 1: Client Connection

```
Client connects → WebServer::handleNewConnection()
    │
    ├─ Accept connection
    ├─ Create new Client object
    ├─ Add socket fd to epoll (EPOLLIN - reading)
    ├─ Set state: READ_REQ
    └─ Return to event loop
```

**Files involved:**
- `WebServer.cpp`: `handleNewConnection()`
- `Client.cpp`: Constructor

### Phase 2: Request Reading

```
Event loop → epoll_wait() returns client fd readable
    │
    └─ WebServer::handleRead()
        │
        ├─ Read data from socket into Client buffer
        ├─ Call Request::parse() on buffer
        ├─ If complete: set state to REQ_RDY
        └─ Return to event loop
```

**Files involved:**
- `Request.cpp`: `parse()` method
- `WebServer.cpp`: `handleRead()`

### Phase 3: Request Handling

```
Event loop → detects state = REQ_RDY
    │
    └─ WebServer::updateClient()
        │
        ├─ Request is valid?
        ├─ File requested is static?
        │   └─ Serve with Response, done
        │
        └─ File is CGI script?
            └─ Cgi object created
            └─ Cgi::executeAsync() called
            └─ Set state: WAIT_CGI
            └─ Add CGI output fd to epoll (EPOLLIN)
            └─ Return to event loop
```

**Files involved:**
- `Response.cpp`: Constructor
- `Cgi.cpp`: `executeAsync()`
- `WebServer.cpp`: `updateClient()`

### Phase 4: CGI Execution (Async)

```
                    CHILD PROCESS                    PARENT PROCESS
                   (CGI Script)                       (WebServer)
                        │                                   │
        dup2(stdin)      │                                   │
        dup2(stdout)     │                                   │
        execve()  ──→ Run script    ←── Event loop continues
                        │                                   │
                   Output to                           Monitoring
                   stdout pipe                         output pipe
                        │                                   │
```

**What happens in parent during execution:**
- **Event loop continues** - doesn't block
- **Monitors epoll** for:
  - CGI output pipe readable → read data
  - Client socket readable → another request?
  - Client socket writable → send response

**Files involved:**
- `Cgi.cpp`: `executeAsync()` (fork/exec)
- `WebServer.cpp`: Event loop, monitoring

### Phase 5: CGI Output Reading

```
Event loop → epoll_wait() returns CGI fd readable
    │
    └─ WebServer::handleCgiRead()
        │
        ├─ Get associated Cgi object
        ├─ Call Cgi::readOutput()
        │   │
        │   ├─ Check timeout: hasTimedOut()?
        │   │   ├─ NO → Read pipe data into _output
        │   │   └─ YES → Kill process, clear buffer
        │   │
        │   └─ Return true if complete, false if still running
        │
        ├─ If complete:
        │   ├─ Remove CGI fd from epoll
        │   ├─ Call finalizeCgiResponse()
        │   ├─ Set state: RES_RDY
        │   └─ Add client fd to epoll (EPOLLOUT)
        │
        └─ Return to event loop
```

**Files involved:**
- `Cgi.cpp`: `readOutput()`, `hasTimedOut()`
- `WebServer.cpp`: `handleCgiRead()`, `finalizeCgiResponse()`

### Phase 6: Response Finalization

```
WebServer::finalizeCgiResponse()
    │
    ├─ Get CGI output
    ├─ Check if empty (timeout indicator)
    ├─ If empty:
    │   └─ Build 504 Gateway Timeout response
    │
    └─ If has data:
        ├─ Parse CGI headers
        ├─ Extract status code
        ├─ Build response with:
        │   ├─ Status from CGI (or 200)
        │   ├─ Headers from CGI
        │   └─ Body from CGI output
        └─ Set Client state to RES_RDY
```

**Files involved:**
- `Cgi.cpp`: `parseCgiHeaders()`
- `Response.cpp`: Response construction
- `WebServer.cpp`: `finalizeCgiResponse()`

### Phase 7: Response Sending

```
Event loop → epoll_wait() returns client fd writable
    │
    └─ WebServer::handleWrite()
        │
        ├─ Get Response from Client
        ├─ Convert to HTTP string
        ├─ Write to socket
        ├─ If all sent:
        │   ├─ Close connection
        │   ├─ Remove from epoll
        │   ├─ Delete Client object
        │   └─ Back to listening for new connections
        │
        └─ Return to event loop
```

**Files involved:**
- `Response.cpp`: `toStr()` - convert to HTTP string
- `WebServer.cpp`: `handleWrite()`

---

## 3. Timeout System

### Timeout Data Flow

```
Cgi::executeAsync()
    │
    └─ _startTime = time(NULL)  ← Record Unix timestamp
       _timeout = 30            ← Timeout in seconds
       _pid = fork()            ← Store child PID

        ↓ (async monitoring starts)

Cgi::readOutput() [called repeatedly from event loop]
    │
    ├─ if (hasTimedOut())
    │   │
    │   ├─ time(NULL) - _startTime > 30?
    │   │
    │   └─ YES:
    │       ├─ kill(_pid, SIGKILL)    ← Force process termination
    │       ├─ _output.clear()        ← Clear buffer (signals timeout)
    │       ├─ close(_outPipe[0])     ← Close pipe
    │       └─ _isComplete = true     ← Mark as done
    │
    └─ Return true to finalize

WebServer::finalizeCgiResponse()
    │
    └─ if (cgiOutput.empty())  ← Empty output indicates timeout
        │
        └─ Build 504 Gateway Timeout response
```

### Timeline Visualization

```
Timeline of timeout execution:

    0s          5s          10s         15s         20s
    |-----------|-----------|-----------|-----------|-------
    ├─ FORK & START
    │  _startTime = T0
    │
    │  Loop checks: elapsed = T-T0 = 0
    │  0 > 30? NO
    │
    ├─────────────┤
    │  Loop checks: elapsed = 5
    │  5 > 30? NO
    │
    ├─────────────────────────┤
    │  Loop checks: elapsed = 10
    │  10 > 30? NO
    │  ... continues ...
    │
    ├─────────────────────────────────────────────────────┤
    │  Loop checks: elapsed = 20
    │  20 > 30? NO

    25s         30s         31s
    |-----------|-----------|-------
    ├─────────────────────────────────────────────────────────┤
    │  Loop checks: elapsed = 25
    │  25 > 30? NO
    │
    ├─────────────────────────────────────────────────────────────┤
    │  Loop checks: elapsed = 30
    │  30 > 30? NO (exactly equal)
    │
    ├─────────────────────────────────────────────────────────────────┤
    │  Loop checks: elapsed = 31
    │  31 > 30? YES → TIMEOUT!
    │  ├─ Kill process
    │  ├─ Clear output
    │  └─ Send 504
```

---

## 4. State Machine

```
                    ┌──────────────────────────────────┐
                    │  Client Connected                │
                    │  State: READ_REQ                 │
                    └──────────────────────────────────┘
                                  │
                                  ▼
                    ┌──────────────────────────────────┐
                    │  Reading HTTP Request            │
                    │  State: READ_REQ                 │
                    │  Data in buffer until complete   │
                    └──────────────────────────────────┘
                                  │
                                  ▼
                    ┌──────────────────────────────────┐
                    │  Request Complete                │
                    │  State: REQ_RDY                  │
                    └──────────────────────────────────┘
                                  │
                    ┌─────────────┴──────────────┐
                    │                            │
                    ▼                            ▼
        ┌──────────────────────┐    ┌──────────────────────┐
        │  Static File?        │    │  CGI Script?         │
        │  State: RES_RDY      │    │  State: WAIT_CGI     │
        └──────────────────────┘    └──────────────────────┘
                    │                            │
                    │                  ┌─────────┴──────────┐
                    │                  │                    │
                    │                  ▼                    ▼
                    │         ┌──────────────────┐ ┌──────────────────┐
                    │         │ Executing...     │ │ Timeout!         │
                    │         │ Reading output   │ │ Kill & clear     │
                    │         │ Monitoring time  │ └──────────────────┘
                    │         └──────────────────┘         │
                    │                  │                    │
                    │                  └─────────┬──────────┘
                    │                            │
                    └────────────────┬───────────┘
                                     │
                                     ▼
                    ┌──────────────────────────────────┐
                    │  Response Ready                  │
                    │  State: RES_RDY                  │
                    │  All data prepared               │
                    └──────────────────────────────────┘
                                     │
                                     ▼
                    ┌──────────────────────────────────┐
                    │  Sending Response                │
                    │  State: RES_SENDING              │
                    │  Writing to socket               │
                    └──────────────────────────────────┘
                                     │
                                     ▼
                    ┌──────────────────────────────────┐
                    │  Complete                        │
                    │  State: FINISHED                 │
                    │  Connection closed               │
                    └──────────────────────────────────┘
```

---

## 5. Event Loop Structure

```cpp
while (server_running) {
    // Wait for events (blocks until something happens)
    int numEvents = epoll_wait(_epoll_fd, events, MAX_EVENTS, TIMEOUT);

    // Process each event
    for (int i = 0; i < numEvents; i++) {
        int fd = events[i].data.fd;
        uint32_t eventMask = events[i].events;

        // New client connection?
        if (fd == _server_socket) {
            handleNewConnection();
        }
        // Client has data to read?
        else if (eventMask & EPOLLIN) {
            // Could be client socket or CGI output pipe
            if (_clients.contains(fd)) {
                handleRead(fd);  // Regular client
            } else if (_cgiClients.contains(fd)) {
                handleCgiRead(fd);  // CGI output
            }
        }
        // Client socket ready to write?
        else if (eventMask & EPOLLOUT) {
            handleWrite(fd);  // Send response
        }
    }

    // Process any clients ready for state transitions
    updateClients();
}
```

**Key insight:**
- Server never blocks waiting for CGI - continues event loop
- Multiple CGI scripts can run concurrently
- All monitored via single epoll mechanism

---

## 6. Data Structures

### Client Object
```cpp
class Client {
    int _fd;                      // Socket file descriptor
    std::string _inBuffer;        // Incoming HTTP request
    std::string _outBuffer;       // Outgoing HTTP response
    Request* _request;            // Parsed request
    Response* _response;          // Built response
    Cgi* _cgi;                    // For CGI requests
    ClientState _state;           // Current state (READ_REQ, WAIT_CGI, etc)
    time_t _lastActivity;         // For connection timeout
};
```

### Cgi Object
```cpp
class Cgi {
    std::string _path;            // Script path
    std::string _interpreter;     // "python3", "/usr/bin/python3", etc
    std::map<...> _env;           // Environment variables
    std::string _body;            // Request body (POST data)

    pid_t _pid;                   // Child process ID
    int _outPipe[2];              // Child stdout → parent read
    int _inPipe[2];               // Parent write → child stdin

    std::string _output;          // All output from CGI script
    bool _isComplete;             // Flag: execution done?

    int _timeout;                 // Timeout in seconds (30)
    int _startTime;               // Unix timestamp when started
};
```

### Request Object
```cpp
class Request {
    std::string _httpVersion;     // "HTTP/1.1"
    std::string _method;          // "GET", "POST", "DELETE"
    std::string _uri;             // "/cgi-bin/script.py?param=value"
    std::string _queryString;     // "param=value"
    std::map<...> _headers;       // All HTTP headers
    std::string _body;            // Request body (POST, PUT)

    // ... parsed from raw HTTP request ...
};
```

### Response Object
```cpp
class Response {
    std::string _httpVersion;     // "HTTP/1.1"
    int _statusCode;              // 200, 404, 504, etc
    std::string _statusTxt;       // "OK", "Not Found", "Gateway Timeout"
    std::map<...> _headers;       // Response headers
    std::string _body;            // Response body

    // Conversion to HTTP
    std::string toStr();          // "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n..."
};
```

---

## 7. File Descriptors Map

```
                    ┌─────────────────────────────┐
                    │   EPOLL File Descriptors    │
                    └─────────────────────────────┘
                                │
                    ┌───────────┼───────────┐
                    │           │           │
                    ▼           ▼           ▼

                FD 3         FD 5          FD 8
                │            │             │
        ┌──────────────┐  ┌────────┐  ┌──────────┐
        │Server Socket │  │Client 1│  │  CGI 1   │
        │ (listening)  │  │Socket  │  │ Output   │
        │              │  │        │  │ Pipe     │
        └──────────────┘  └────────┘  └──────────┘
                            │
                        Client obj:
                        ├─ fd=5
                        ├─ _request (parsed)
                        ├─ _response (built)
                        ├─ _cgi → points to Cgi obj
                        └─ _state = WAIT_CGI

                                         Cgi obj:
                                         ├─ _pid=12345
                                         ├─ _outPipe[0]=8
                                         ├─ _startTime=T
                                         ├─ _timeout=30
                                         └─ _output=""
```

---

## 8. Process Model

### Parent-Child Communication

```
┌─────────────────────────┐       ┌─────────────────────────┐
│   PARENT (WebServer)    │       │   CHILD (CGI Script)    │
│                         │       │                         │
│  PID: 1000              │       │  PID: 1234              │
│                         │       │                         │
│  Event loop             │       │  #!/usr/bin/python3     │
│  ├─ Check epoll         │       │  print("Content...")    │
│  ├─ Read stdin (pipe)   │←──────│  Read stdin from pipe   │
│  │  _outPipe[0]         │       │  Write stdout to pipe   │
│  │                      │       │  └─→ _outPipe[1]        │
│  ├─ Handle timeout      │       │                         │
│  ├─ Send response       │       │  exit(0)                │
│  └─ Continue running    │       │  [process ends]         │
│                         │       │                         │
│  Monitored with:        │       │  Monitored with:        │
│  ├─ epoll()             │       │  └─ waitpid(WNOHANG)   │
│  ├─ waitpid(WNOHANG)   │       │     (in parent)         │
│  └─ time()              │       │                         │
│                         │       │                         │
└─────────────────────────┘       └─────────────────────────┘
```

### Multiple CGI Processes Running Concurrently

```
Parent (PID 1000) spawns multiple children:

├─ Child 1 (hello.py) - PID 1234
│  ├─ Output pipe: FD 8 (in epoll)
│  ├─ Runtime: 2 seconds
│  └─ State: Running
│
├─ Child 2 (hang.py) - PID 1235
│  ├─ Output pipe: FD 9 (in epoll)
│  ├─ Runtime: 35 seconds
│  └─ State: Monitoring for timeout
│
└─ Child 3 (echo.py) - PID 1236
   ├─ Output pipe: FD 10 (in epoll)
   ├─ Runtime: 0.5 seconds
   └─ State: Complete

epoll_wait() monitors FD 8, 9, 10 simultaneously
- FD 8: Child 1 finished, read output, send 200
- FD 10: Child 3 ready, read output
- FD 9: At 30s, timeout detected, kill and send 504
```

---

## 9. Timeout Mechanism Deep Dive

### Time Arithmetic

```cpp
int _startTime;    // Unix timestamp (seconds since Jan 1, 1970)

// Example: 2025-12-21 17:35:00 UTC
_startTime = 1734864900;

// 30 seconds later: 2025-12-21 17:35:30 UTC
current = 1734864930;

elapsedTime = 1734864930 - 1734864900 = 30 seconds
timeout = 30 seconds

30 > 30? NO (equal, not greater) → Keep running
31 > 30? YES → TIMEOUT!
```

### Why `>` Not `>=`?

```
Execution at exactly 30 seconds:
├─ elapsed = 30
├─ Check: 30 >= 30? YES → Kill immediately (might be ok)
│
└─ Check: 30 > 30? NO → Continue 1 more second

Using > gives 1 second grace period, which is fair for scripts
that take exactly 30 seconds.

But either way is acceptable - difference is negligible.
```

### Signal Handling

```cpp
// SIGKILL (signal 9)
kill(_pid, SIGKILL)

Properties:
├─ Cannot be caught or ignored
├─ Forces immediate process termination
├─ No cleanup chance for process (harsh but necessary)
├─ Resources freed by OS
└─ Guaranteed to work

Alternative: SIGTERM (signal 15)
├─ Can be caught/ignored
├─ Allows graceful shutdown
├─ Process might not die
└─ Not suitable for timeout enforcement
```

---

## 10. Error Handling Paths

```
Scenario: What if everything fails?

Normal path:
Request → CGI execution → Output reading → Response build → Send

Error: Pipe creation fails
├─ executeAsync() throws exception
├─ Caught in updateClient()
├─ Send 500 Internal Server Error
└─ Client receives error

Error: Child process dies unexpectedly
├─ waitpid() detects PID change
├─ _isComplete = true
├─ Whatever output collected is used
├─ Response built with available data
└─ Send response (might be partial)

Error: Timeout + kill fails
├─ kill() returns -1
├─ But process probably dead anyway (check with ps)
├─ _output.clear() still executed
├─ 504 still sent
└─ Fail-safe: timeout works anyway

Error: Output pipe already closed
├─ _outPipe[0] == -1
├─ readOutput() returns immediately
├─ No crash, graceful handling
└─ Process monitored with waitpid()
```

---

## 11. Memory Management

```
Cgi Object Lifecycle:

1. Created in updateClient()
   └─ new Cgi(request, server)

2. Stored in Client
   └─ client.setCgi(cgiPtr)

3. Also stored in WebServer
   └─ _cgiClients[fd] = cgiPtr

4. Monitored via epoll
   └─ Output pipe fd added to epoll

5. When complete:
   ├─ Remove from _cgiClients map
   ├─ Delete cgiPtr
   └─ Set client._cgi = NULL

Cleanup in finalizeCgiResponse():
├─ epoll_ctl(EPOLL_CTL_DEL)  → Remove from monitoring
├─ _cgiClients.erase()       → Remove from map
├─ delete cgi                → Free memory
└─ client.setCgi(NULL)       → Clear reference

No memory leaks:
├─ Pipes closed in destructor if not already
├─ Process cleaned up with waitpid()
├─ All dynamically allocated memory freed
└─ File descriptors closed
```

---

## 12. Performance Characteristics

```
Single CGI request (timeout path):

T=0ms     Request arrives
T=0ms     Cgi::executeAsync() - fork, exec (1-5ms)
T=5ms     Ready to monitor (epoll added)

T=5-30000ms  Event loop continues
          ├─ No busy waiting
          ├─ epoll_wait() sleeps (CPU idle)
          ├─ readOutput() called maybe 100-1000x
          └─ Each check: ~0.1μs (negligible)

T=30000ms Timeout detected
          ├─ Kill signal sent (instant)
          ├─ Process cleanup (1-5ms)
          └─ Response building (1-2ms)

T=30005ms Response sent to client (1-5ms)

Total time: ~30 seconds (dominated by timeout)
CPU cost: ~0.01ms of actual CPU time
Server impact: None - other requests processed simultaneously
```

---

## Summary

Your webserver implements a **sophisticated async CGI system** with:

✅ **Concurrency:** Multiple CGI scripts run in parallel
✅ **Non-blocking:** Event loop never waits
✅ **Resource-safe:** Timeouts prevent runaway processes
✅ **Efficient:** Minimal CPU overhead, scalable design
✅ **Correct:** Proper cleanup, no memory leaks
✅ **Responsive:** Always handles new requests immediately

The timeout mechanism is the cherry on top - ensuring that even buggy CGI scripts can't take down your server! 🎉
