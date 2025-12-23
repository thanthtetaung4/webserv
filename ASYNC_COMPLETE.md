# ✅ CGI Async-Only Mode Refactoring - COMPLETE

## Summary

Successfully refactored your webserver to use **only asynchronous/non-blocking CGI handling**. This simplifies the codebase and enables timeout implementation.

## What Was Changed

### 1. **Response.cpp Constructor** (Line 14-25)
- Added initialization of all member variables with default values
- Response constructor now returns early for CGI files instead of executing them
- Prevents blocking on CGI execution

**Before:**
```cpp
Response::Response(Request &req, Server &server)
{
    // ...
    if (isRegularFile(finalPath) && loc._isCgi)
    {
        handleCGI(req, server);  // BLOCKS HERE!
        return;
    }
}
```

**After:**
```cpp
Response::Response(Request &req, Server &server)
{
    // Initialize all members first
    this->_httpVersion = "HTTP/1.1";
    this->_statusCode = 200;
    this->_statusTxt = "OK";
    this->_body = "";
    this->_headers.clear();

    // ...
    if (isRegularFile(finalPath) && loc._isCgi)
    {
        // Don't execute - let WebServer handle it asynchronously
        return;
    }
}
```

### 2. **WebServer::updateClient()** (Lines 354-435)
- Enhanced CGI file detection (checks extension)
- Creates Cgi object and calls `executeAsync()` (non-blocking fork)
- Adds CGI pipe to epoll for monitoring
- Only builds Response for non-CGI requests
- CGI Response is built later in `finalizeCgiResponse()`

**Key changes:**
```cpp
// Detect if file is a CGI file by extension
bool isCgiFile = false;
if (loc._isCgi && !loc._cgiExt.empty() &&
    finalPath.size() >= loc._cgiExt.size() &&
    finalPath.substr(finalPath.size() - loc._cgiExt.size()) == loc._cgiExt)
{
    isCgiFile = true;
}

// Execute CGI asynchronously
if (!loc._cgiPass.empty() && isCgiFile && ...) {
    Cgi* cgi = new Cgi(*req, client.getServer());
    cgi->executeAsync();  // Non-blocking fork

    client.setCgi(cgi);
    client.setState(WAIT_CGI);

    // Monitor CGI output pipe
    epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, cgiFd, ...);
    _cgiClients[cgiFd] = &client;
    return;
}

// For non-CGI: build Response immediately
if (!client.buildRes()) {
    return;
}
```

### 3. **WebServer::finalizeCgiResponse()** (Lines 770-845)
- Properly parses CGI Status header (handles "200 OK" and "404 Not Found" formats)
- Sets status code and text correctly
- Includes all CGI headers in response
- Ensures Content-Length is set
- Removes CGI fd from epoll after parsing

**Key additions:**
```cpp
// Parse status code from CGI output
if (result.headers.count("Status") > 0) {
    std::string statusLine = result.headers["Status"];
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

### 4. **WebServer::handleWrite()** (Lines 562-585)
- Added check to prevent rebuilding Response if it already exists
- This was critical - Response was being created twice!

**Before:**
```cpp
if (!client->buildRes())  // Always rebuilds!
    throw "Fatal Err: Response cannot be create";
```

**After:**
```cpp
if (!client->getResponse()) {  // Only build if missing
    if (!client->buildRes())
        throw "Fatal Err: Response cannot be create";
}
```

### 5. **Cgi::parseCgiHeaders()** (Lines 225-275)
- Now handles both CRLF (`\r\n\r\n`) and LF (`\n\n`) delimiters
- Python scripts output LF, so this fix was necessary

**Key change:**
```cpp
// Find the end of the CGI header block
size_t headerEnd = output.find("\r\n\r\n");
size_t bodyStart = 0;
std::string delimiter = "\r\n";

if (headerEnd == std::string::npos)
{
    // Try Unix line endings (\n\n)
    headerEnd = output.find("\n\n");
    if (headerEnd == std::string::npos) {
        // No delimiter found
        result.body = output;
        return result;
    }
    bodyStart = headerEnd + 2;  // "\n\n" is 2 bytes
    delimiter = "\n";
}
else {
    bodyStart = headerEnd + 4;  // "\r\n\r\n" is 4 bytes
}
```

## New Execution Flow

```
REQUEST PHASE:
  Client connects
  Raw HTTP request received
  epoll: EPOLLIN on client socket
  handleRead() reads request
  Request complete

UPDATE PHASE (WebServer::updateClient):
  buildReq() - parse HTTP
  Check if CGI file needed

  ┌─ If CGI ─┐
  │          ├─ Cgi::executeAsync() (fork, returns immediately)
  │          ├─ client.setState(WAIT_CGI)
  │          ├─ Add CGI fd to epoll(EPOLLIN)
  │          └─ Return
  │
  └─ If NOT CGI
             ├─ client.buildRes() (create Response now)
             ├─ client.setState(RES_RDY)
             └─ Return

CGI MONITORING PHASE (epoll loop):
  CGI produces output
  epoll: EPOLLIN on CGI fd
  handleCgiRead() calls Cgi::readOutput()
  When CGI complete: finalizeCgiResponse()
    ├─ Get CGI output
    ├─ Parse headers
    ├─ client.buildRes() (create empty Response)
    ├─ Populate Response with CGI data
    ├─ Remove CGI fd from epoll
    ├─ Delete Cgi object
    ├─ client.setState(RES_RDY)
    └─ Add client fd to epoll(EPOLLOUT)

RESPONSE SENDING PHASE:
  epoll: EPOLLOUT on client socket
  handleWrite() sends response via socket
  Connection closed or keep-alive
```

## Benefits

✅ **Non-blocking**: Server never waits for CGI
✅ **Scalable**: Multiple CGI scripts execute in parallel
✅ **Timeout-ready**: Easy to add timeout logic now
✅ **Cleaner**: Single code path for all requests
✅ **Predictable**: Clear state transitions
✅ **Tested**: CGI responses working correctly

## Test Results

```
REQUEST: GET /cgi-bin/hello.py HTTP/1.1

RESPONSE RECEIVED:
HTTP/1.1 200 OK
Content-Length: 24
Content-Type: text/html

<h1>Hello from CGI</h1>

✅ Status: 200 OK
✅ Headers: Correctly set (Content-Type, Content-Length)
✅ Body: <h1>Hello from CGI</h1>
✅ Full response sent: 88 bytes
```

## How to Add Timeout Now

Add this to your epoll loop in `WebServer::run()`:

```cpp
// Track CGI start times
static std::map<int, time_t> cgiStartTimes;

// In CGI fork:
cgiStartTimes[cgiFd] = time(NULL);

// In CGI monitoring:
if (cgiClient) {
    time_t elapsed = time(NULL) - cgiStartTimes[cgiFd];

    if (elapsed > CGI_TIMEOUT_SECONDS) {
        // Kill process and finalize
        kill(_cgiClients[fd]->getCgi()->getPid(), SIGKILL);
        finalizeCgiResponse(*cgiClient);
        cgiStartTimes.erase(fd);
    }
}
```

## Files Modified

- `src/classes/Response.cpp` - Constructor initialization & CGI skip
- `src/classes/WebServer.cpp` - updateClient, finalizeCgiResponse, handleWrite
- `src/classes/Cgi.cpp` - parseCgiHeaders (dual delimiter support)

## Status

✅ **COMPLETE AND TESTED**

All CGI requests now use async-only mode. The server:
- Properly parses CGI output
- Sets correct HTTP status and headers
- Includes CGI response body
- Is ready for timeout implementation
