# CGI Async-Only Mode Refactoring

## Summary of Changes

You've successfully refactored the CGI handling system to use **only async/non-blocking mode**. This simplifies the code and allows for easy timeout implementation later.

## What Changed

### 1. Response Constructor (`src/classes/Response.cpp`)

**Before:**
- Blocked on CGI execution via `handleCGI(req, server)` call
- Server would freeze while waiting for CGI process to complete

**After:**
- Returns early when CGI request detected
- All member variables initialized with defaults
- CGI execution deferred to WebServer

```cpp
// Initialize all members first
this->_httpVersion = "HTTP/1.1";
this->_statusCode = 200;
this->_statusTxt = "OK";
this->_body = "";
this->_headers.clear();

// ... process request ...

// For CGI files, just return early - let WebServer handle it
if (isRegularFile(finalPath) && loc._isCgi)
{
    // Don't handle CGI here - let WebServer handle it asynchronously
    // This prevents blocking the server
    return;
}
```

### 2. WebServer::updateClient (`src/classes/WebServer.cpp`)

**Enhanced to:**
- Detect CGI files early (check extension)
- Execute CGI asynchronously
- Build Response only for non-CGI requests
- Map CGI file descriptors to clients for monitoring

```cpp
// Detect if file is a CGI file
bool isCgiFile = false;
if (loc._isCgi && !loc._cgiExt.empty() &&
    finalPath.size() >= loc._cgiExt.size() &&
    finalPath.substr(finalPath.size() - loc._cgiExt.size()) == loc._cgiExt)
{
    isCgiFile = true;
}

if (!loc._cgiPass.empty() && isCgiFile && (GET or POST)) {
    // Fork CGI process async
    Cgi* cgi = new Cgi(*req, client.getServer());
    cgi->executeAsync();

    client.setCgi(cgi);
    client.setState(WAIT_CGI);

    // Add CGI pipe to epoll for monitoring
    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, cgiFd, ...);
    _cgiClients[cgiFd] = &client;
}

// For non-CGI: build Response immediately
if (!client.buildRes()) {
    std::cerr << "Failed to build response" << std::endl;
    return;
}
```

### 3. WebServer::finalizeCgiResponse (`src/classes/WebServer.cpp`)

**Enhanced to:**
- Parse Status headers from CGI output
- Properly set status codes and text
- Handle Content-Length correctly

```cpp
// Parse status code from CGI
if (result.headers.count("Status") > 0) {
    std::string statusLine = result.headers["Status"];
    // Parse "200 OK" or "404 Not Found"
    size_t spacePos = statusLine.find(' ');
    if (spacePos != std::string::npos) {
        statusCode = std::atoi(statusLine.substr(0, spacePos).c_str());
        statusTxt = statusLine.substr(spacePos + 1);
    }
}

res->setStatusCode(statusCode);
res->setStatusTxt(statusTxt);
res->setBody(result.body);

// Set all CGI headers
for (auto header : result.headers) {
    if (header.first != "Status") {
        res->setHeader(header.first, header.second);
    }
}
```

## Execution Flow (New Async-Only)

```
CLIENT REQUEST
    ↓
WebServer::run() - epoll loop
    ↓
EPOLLIN on client socket
    ↓
WebServer::handleRead() - read raw HTTP
    ↓
Client has complete request
    ↓
WebServer::updateClient()
    ├─ buildReq() - parse HTTP
    ├─ Check if CGI file
    │
    ├─ YES (CGI File):
    │   ├─ Cgi::executeAsync() - fork, return immediately
    │   ├─ client.setState(WAIT_CGI)
    │   ├─ Add CGI fd to epoll(EPOLLIN)
    │   └─ Return - wait for CGI to produce output
    │
    └─ NO (Regular file):
        ├─ client.buildRes() - create Response
        ├─ client.setState(RES_RDY)
        ├─ Modify epoll(client fd, EPOLLOUT)
        └─ Return - ready to send response

(For CGI files, continue in epoll loop...)
    ↓
EPOLLIN on CGI pipe (CGI has output)
    ↓
WebServer::handleCgiRead()
    ├─ Client* = search _cgiClients[fd]
    ├─ Cgi::readOutput() - read non-blocking
    └─ If complete → finalizeCgiResponse()

WebServer::finalizeCgiResponse()
    ├─ Get CGI output
    ├─ Parse headers
    ├─ client.buildRes() - create empty Response
    ├─ Populate with CGI data
    ├─ Remove CGI fd from epoll
    ├─ Delete Cgi object
    ├─ client.setState(RES_RDY)
    ├─ Modify epoll(client fd, EPOLLOUT)
    └─ Return - wait for client socket ready to write

(For all requests now...)
    ↓
EPOLLOUT on client socket (ready to send)
    ↓
WebServer::handleWrite()
    ├─ Get Response
    ├─ Convert to raw HTTP via Response::toStr()
    ├─ Socket::send() to client
    └─ Close or keep-alive based on Connection header
```

## Benefits of Async-Only Mode

1. **Non-blocking**: Server never waits for CGI execution
2. **Scalable**: Multiple CGI scripts execute in parallel
3. **Timeout-friendly**: Easy to add timeout by checking elapsed time in epoll loop
4. **Cleaner**: Single code path for request handling
5. **Predictable**: State machine is clearer

## How to Add CGI Timeout

Now you can easily add CGI timeout logic:

```cpp
// In WebServer::run() epoll loop
if (cgiClient) {
    time_t currentTime = time(NULL);
    time_t elapsedTime = currentTime - cgiStartTime[cgiFd];

    if (elapsedTime > CGI_TIMEOUT_SECONDS) {
        // CGI timeout - kill process and finalize
        kill(_cgiClients[cgiFd]->getCgi()->getPid(), SIGKILL);
        finalizeCgiResponse(*cgiClient);
    }
}
```

## Files Modified

1. **src/classes/Response.cpp**
   - Initialize member variables in constructor
   - Return early for CGI files instead of calling handleCGI()
   - Also in directory processing (processDirectoryRequest)

2. **src/classes/WebServer.cpp** (updateClient)
   - Enhanced CGI detection with extension checking
   - Build Response for non-CGI requests immediately
   - Improved CGI state handling

3. **src/classes/WebServer.cpp** (finalizeCgiResponse)
   - Proper Status header parsing
   - Content-Length calculation
   - Better header handling

## Testing Checklist

- [ ] Regular static files serve correctly
- [ ] CGI scripts execute and return output
- [ ] CGI Status headers are parsed
- [ ] Content-Length is set correctly
- [ ] Directory listing works
- [ ] Error pages are served
- [ ] Multiple CGI requests handled in parallel
- [ ] Keep-alive connections work
- [ ] Timeout can be detected (check elapsed time)

## Next Steps

1. Test the new async-only system
2. Implement CGI timeout with elapsed time tracking
3. Add timeout configuration to your server config
4. Test timeout kills CGI process correctly
