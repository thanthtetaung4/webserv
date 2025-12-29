/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/30 03:26:16 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/utils.h"
#include "../../include/WebServer.hpp"
#include "../../include/Request.hpp"
#include "../../include/Response.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../../include/Validator.hpp"
#include "../../include/Cgi.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include "../../include/Client.hpp"

WebServer::WebServer() {}

WebServer::~WebServer() {}

void WebServer::setServer(std::string configFile)
{
	std::ifstream config(configFile.c_str());
	if (!config)
		throw std::runtime_error("Unable to open file!!");

	std::stringstream ss;
	ss << config.rdbuf();
	std::string content = ss.str();
	config.close();

	size_t pos = 0;
	while (pos < content.length())
	{
		size_t serverStart = content.find("server", pos);
		if (serverStart == std::string::npos)
			break;
		size_t openBracePos = content.find('{', serverStart);
		if (openBracePos == std::string::npos)
			throw std::runtime_error("Missing opening brace after 'server' keyword");
		int braceCount = 0;
		size_t closeBracePos = openBracePos;
		bool foundMatch = false;
		for (size_t i = openBracePos; i < content.length(); ++i)
		{
			if (content[i] == '{')
				braceCount++;
			else if (content[i] == '}')
			{
				braceCount--;
				if (braceCount == 0)
				{
					closeBracePos = i;
					foundMatch = true;
					break;
				}
			}
		}
		if (!foundMatch)
			throw std::runtime_error("Mismatched braces in server block!");
		t_iterators it;
		it.it1 = content.begin() + openBracePos + 1;
		it.it2 = content.begin() + closeBracePos;
		Server server;
		server.fetchSeverInfo(it);
		addServer(server);
		pos = closeBracePos + 1;
	}
}

void WebServer::addServer(Server server)
{
	this->_servers.push_back(server);
	this->_sockets.push_back(Socket(std::atol(server.getPort().c_str())));
}

void WebServer::setUpSock(void)
{
	std::vector<Socket>::iterator it;

	for (it = this->_sockets.begin(); it != this->_sockets.end(); ++it)
	{
		(*it).prepSock();
		// std::cout << (*it) << std::endl;
	}
}

std::vector<Server> WebServer::getServers() const
{
	return _servers;
}

std::vector<Socket>& WebServer::getSockets()
{
	return _sockets;
}

int parseContentLength(const std::string &headers)
{
	std::string key = "Content-Length:";
	size_t pos = headers.find(key);
	if (pos == std::string::npos)
		return 0;

	pos += key.length();

	// Skip spaces
	while (pos < headers.size() && (headers[pos] == ' ' || headers[pos] == '\t'))
		pos++;

	// Read digits
	int value = 0;
	while (pos < headers.size() && isdigit(headers[pos]))
	{
		value = value * 10 + (headers[pos] - '0');
		pos++;
	}

	return value;
}

static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error(std::string("fcntl(F_GETFL)"));
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error(std::string("fcntl(F_SETFL)"));
}

bool WebServer::isUpStream(int fd) const {
	std::vector<int>::const_iterator it;

	for (it = _upstreamFds.begin(); it != _upstreamFds.end(); it++) {
		if (fd == *it)
			return (true);
	}
	return (false);
}

bool WebServer::isListenFd(int fd) const {
	for (size_t i = 0; i < _sockets.size(); ++i) {
		if (_sockets[i].getServerFd() == fd)
			return true;
	}
	return false;
}

Client*	WebServer::searchClients(int fd) {
	std::map<int, Client*>::iterator i;

	for (i = _clients.begin() ; i != _clients.end(); i++) {
		if (i->first == fd)
			return (i->second);
	}

	return (NULL);
}

Client*	WebServer::searchClientsUpstream(int fd) {
	std::map<int, Client*>::iterator i;

	for (i = _upstreamClient.begin() ; i != _upstreamClient.end(); i++) {
		if (i->first == fd)
			return (i->second);
	}
	return (NULL);
}

Client*	WebServer::searchClientsByCgi(int cgiFd) {
	std::map<int, Client*>::iterator i;

	for (i = _cgiClients.begin() ; i != _cgiClients.end(); i++) {
		if (i->first == cgiFd)
			return (i->second);
	}
	return (NULL);
}

void WebServer::handleAccept(int listenfd) {
	// Accept in a loop to drain any pending connections.
	// With non-blocking sockets (and especially with epoll edge-triggered mode)
	// there may be multiple connections queued; accept until EAGAIN/EWOULDBLOCK.
	while (true) {
		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		// std::cout << "listen fd: " << listenfd << std::endl;
		int client_fd = accept(listenfd, (sockaddr*)&addr, &len);

		if (client_fd == -1) {
			// When no more pending connections are available on a non-blocking
			// socket, accept returns -1 and errno is EAGAIN or EWOULDBLOCK.
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// Normal: we've accepted all pending connections
				break;
			}

			// Real error
			std::perror("accept");
			break;
		}

		// Make client socket non-blocking (could also use accept4 with SOCK_NONBLOCK)
		setNonBlocking(client_fd);

		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events = EPOLLIN;
		ev.data.fd = client_fd;
		if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
			std::perror("epoll_ctl ADD client_fd");
			close(client_fd);
			continue;
		}

		_clients[client_fd] = new Client(client_fd, _servers[searchSocketIndex(_sockets, listenfd)]);
		std::cout << "Accepted new client: " << client_fd << std::endl;
	}
}

int	WebServer::searchVecIndex(std::vector<int> vec, int key) {
	for (size_t i = 0; i < vec.size(); i++) {
		if (vec[i] == key)
			return (i);
	}
	return (-1);
}

int	WebServer::searchSocketIndex(std::vector<Socket> vec, int key) {
	for (size_t i = 0; i < vec.size(); i++) {
		// std::cout << vec[i].getServerFd() << std::endl;
		if (vec[i].getServerFd() == key)
			return (i);
	}
	return (-1);
}

void WebServer::readFromClient(Client& client) {
	// std::cout << "reading from client" << std::endl;
	char buffer[4096];

	if (client.isTimedOut()) {
		// std::cout << "Client timed out during read" << std::endl;
		client.setState(REQ_RDY);
		// closeClient(client.getFd());
		return;
	}

	// Read ONLY what's available right now (non-blocking)
	ssize_t bytes_received = recv(client.getFd(), buffer, sizeof(buffer), MSG_DONTWAIT);

	// Handle recv errors
	if (bytes_received < 0) {
		// Real error
		std::cerr << "recv failed" << std::endl;
		closeClient(client.getFd());
		return;
	}

	// Client closed connection
	if (bytes_received == 0) {
		std::cerr << "Client closed connection" << std::endl;
		closeClient(client.getFd());
		return;
	}

	// Append what we just received
	client.appendRecvBuffer(std::string(buffer, bytes_received));
	// std::cout << "Received " << bytes_received << " bytes" << std::endl;

	std::string requestStr = client.getInBuffer();

	// Check if we have complete headers yet
	if (!client.foundHeader()) {
		// std::cout << "header not found, finding" << std::endl;
		size_t pos = requestStr.find("\r\n\r\n");
		if (pos == std::string::npos) {
			// Headers incomplete, wait for next epoll event
			return;
		}

		size_t bodyStart = pos + 4;
		client.setHeaderEndPos(bodyStart);

		std::string headers = requestStr.substr(0, bodyStart);

		int contentLength = parseContentLength(headers);
		client.setContentLength(contentLength);

		// Check if body is already complete (no body or body already received)
		if (requestStr.size() >= bodyStart + contentLength) {
			// std::cout << "Request complete (headers + body)" << std::endl;
			updateClient(client);
			return;
		}

		// Body incomplete, wait for next epoll event
		// std::cout << "Body incomplete: have " << (requestStr.size() - bodyStart)
		// 			<< ", need " << contentLength << std::endl;
		return;
	}

	// Headers already found in previous call, check if body is now complete
	size_t bodyStart = client.getHeaderEndPos();
	int contentLength = client.getContentLength();

	// std::cout << "Checking body: have " << (requestStr.size() - bodyStart)
	// 			<< " bytes, need " << contentLength << " bytes" << std::endl;

	if (requestStr.size() >= bodyStart + contentLength) {
		// std::cout << "Request body complete" << std::endl;
		updateClient(client);
		return;
	}

	// Still waiting for more body data
	// std::cout << "Still waiting for more body data" << std::endl;
}

void	WebServer::updateClient(Client& client) {

	if (!client.isTimedOut()) {
		// Creating the Request Instance
		if (!client.buildReq())
			throw "Fatal Err: Response Cannot be created";
	}

	// Check if this request needs CGI processing
	const Request* req = client.getRequest();
	if (req && req->getIt() != client.getServer().getLocation().end()) {
		const t_location& loc = req->getIt()->second;
		const std::string& finalPath = req->getFinalPath();
		bool isCgiFile = false;

		// Check if the file is a CGI file (with matching extension)
		if (loc._isCgi && !loc._cgiExt.empty() &&
		    finalPath.size() >= loc._cgiExt.size() &&
		    finalPath.substr(finalPath.size() - loc._cgiExt.size()) == loc._cgiExt)
		{
			isCgiFile = true;
		}

		// Check if this location requires CGI and the file is a CGI file
		if (!loc._cgiPass.empty() && isCgiFile && (req->getMethodType() == "GET" || req->getMethodType() == "POST")) {
			// std::cout << "Starting CGI execution for: " << finalPath << std::endl;

			try {
				// Create and execute CGI asynchronously
				Cgi* cgi = new Cgi(*req, client.getServer());
				cgi->executeAsync();

				client.setCgi(cgi);
				client.setState(WAIT_CGI);

				// Add CGI output fd to epoll
				int cgiFd = cgi->getOutputFd();
				if (cgiFd != -1) {
					struct epoll_event ev;
					std::memset(&ev, 0, sizeof(ev));
					ev.events = EPOLLIN;
					ev.data.fd = cgiFd;

					if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, cgiFd, &ev) == -1) {
						std::cerr << "Failed to add CGI fd to epoll" << std::endl;
						delete cgi;
						client.setCgi(NULL);
						client.setState(RES_RDY);
					} else {
						_cgiClients[cgiFd] = &client;
					}
				}
			} catch (std::exception &e) {
				std::cerr << "CGI execution failed: " << e.what() << std::endl;
				client.setState(RES_RDY);
			}
			return;
		}
	}

	// No CGI needed, proceed to response generation
	// Build the Response for non-CGI requests
	if (!client.buildRes()) {
		std::cerr << "Failed to build response" << std::endl;
		client.setState(RES_RDY);
		return;
	}

	// Setting epoll event to EPOLLOUT
	struct epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.events  = EPOLLOUT; // ready to write
	ev.data.fd = client.getFd();

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client.getFd(), &ev) == -1) {
		throw std::runtime_error(std::string("epoll_ctl MOD listen_fd failed"));
	}
	client.setState(RES_RDY);
	// std::cout << "updating done" << std::endl;
}

int	setupUpstreamSock(Client& client) {
	// std::cout << "Setting up sockets" << std::endl;
	t_proxyPass pp = parseProxyPass((*client.getRequest()).getIt()->second._proxy_pass);
	// std::cout << "Proxying to " << pp.host << ":" << pp.port << pp.path << std::endl;

	// 2. Create upstream client socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cerr << "Failed to create proxy socket" << std::endl;
		return (-1);
	}

	// 3. Build remote address
	struct sockaddr_in server_addr;
	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(std::atoi(pp.port.c_str()));

	// Resolve hostname to IPv4 address
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_INET;      // IPv4 only
	hints.ai_socktype = SOCK_STREAM;  // TCP

	struct addrinfo* ai = NULL;
	int rc = getaddrinfo(pp.host.c_str(), NULL, &hints, &ai);
	if (rc != 0 || ai == NULL) {
		std::cerr << "DNS resolve failed: " << gai_strerror(rc) << std::endl;
		close(sockfd);
		return (-1);
	}
	server_addr.sin_addr = ((struct sockaddr_in*)ai->ai_addr)->sin_addr;
	freeaddrinfo(ai);

	// 4. Connect to upstream
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		std::cerr << "Upstream connection failed" << std::endl;
		close(sockfd);
		return (-1);
	}

	// std::cout << "Connected to upstream" << std::endl;
	return (sockfd);
}

void	WebServer::handleRead(int fd) {
	Client* client = searchClients(fd);
	if (!client) throw "client not found";

	// std::cout << "client has state" << client->getState() << std::endl;

	if (client->getState() == READ_REQ) {
		readFromClient(*client);
		// std::cout << "client state: " << client->getState() << std::endl;
	}

	if (client->getState() == REQ_RDY) {
		updateClient(*client);
	}

	if (client->isProxyPass()) {
		int	ppassFd = setupUpstreamSock(*client);
		if (ppassFd == -1) {
			std::cerr << "ppass fd cannot be oppened" << std::endl;
		}

		client->setUpstreamFd(ppassFd);
		client->setState(WAIT_UPSTREAM);

		_upstreamFds.push_back(ppassFd);
		_upstreamClient[ppassFd] = client;

		setNonBlocking(ppassFd);

		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events  = EPOLLOUT; // ready to write
		ev.data.fd = ppassFd;


		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, ppassFd, &ev) == -1) {
			throw std::runtime_error(std::string("epoll_ctl ADD ppassFD failed"));
		}
	}
}

void WebServer::handleWrite(int fd) {
	// std::cout << "handle write called" << std::endl;
	Client* client = searchClients(fd);
	// std::cout << *client << std::endl;
	if (client->getState() == RES_SENDING) {
		if (!client->getUpstreamBuffer().empty()) {
			// std::cout << "raw res from upstream: " << client->getUpstreamBuffer() << std::endl;
			// std::cout << "WAIT_UPSTREAM" << std::endl;
			// std::cout << "writing to client with res from upstream:" << client->getUpstreamBuffer() << std::endl;

			std::string httpResponse = client->getUpstreamBuffer();

			ssize_t sent = send(fd, httpResponse.c_str(), httpResponse.size(), MSG_DONTWAIT);
			if (sent < 0)
			{
				std::cerr << "send error:" << std::endl;
			}
			closeClient(fd);
		} else {

			std::string	httpResponse = client->getOutBuffer().c_str();

			if (!client->isTimedOut()) {
				ssize_t sent = send(fd, httpResponse.c_str(), httpResponse.size(), MSG_NOSIGNAL);
				if (sent < 0)
				{
					std::cerr << "send error:" << std::endl;
					closeClient(fd);
				} else if (sent > 0) {
					// std::cout << "Sent " << sent << " bytes to client" << "httpResponse size: " << httpResponse.size() << std::endl;
					client->setOutBuffer(std::string(httpResponse.begin() + sent, httpResponse.end()));
					// std::cout << "Remaining outBuffer size: " << client->getOutBuffer().size() << std::endl;
					client->updateLastActiveTime();
				}
				if (client->getOutBuffer().empty()) {
					closeClient(fd);
				}
			} else {
				// std::cout << "Client timed out during send" << std::endl;
				closeClient(fd);
			}
		}
	} else if (client->getState() == RES_RDY) {
		// std::cout << "RES RDY" << std::endl;
		if (client->isTimedOut()) {
			client->setOutBuffer("HTTP/1.1 408 Request Timeout\r \
									Content-Type: text/html\r \
									Content-Length: 69\r \
									Connection: close\r \
									\r \
									<html><body><h1>408 Request Timeout</h1></body></html>");
			client->setState(RES_SENDING);
		} else {
			// Creating Response Instance only if it doesn't exist
			if (!client->getResponse()) {
				// std::cout << *client->getRequest() << std::endl;
				if (!client->buildRes())
					throw "Fatal Err: Response cannot be create";
			}
			// std::cout << "========================" << std::endl;
			// std::cout << *client->getResponse() << std::endl;
			// std::cout << "========================" << std::endl;
			client->setOutBuffer(client->getResponse()->toStr());
			client->setState(RES_SENDING);
		}
	}
	// std::cout << "handle write done" << std::endl;
}

void WebServer::closeClient(int fd) {
	std::map<int, Client*>::iterator it = this->_clients.find(fd);
	if (it == this->_clients.end())
		return;

	// 1) Remove from epoll (so it stops monitoring this fd)
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		std::cerr << "epoll_ctl DEL failed" << std::endl;
	}

	// 2) Close the socket
	if (close(fd) == -1) {
		std::cerr << "close failed" << std::endl;
	}

	// 3) Remove from clients map
	delete it->second;
	this->_clients.erase(it);

	// std::cout <<"==============================================" << std::endl;
	// std::cout << "Client map size after removal: " << this->_clients.size() << std::endl;
	// std::cout << "==============================================" << std::endl;

	std::cout << "Client " << fd << " closed and removed from epoll" << std::endl;
}

void WebServer::handleUpstreamWrite(Client& c, int fd) {
	// std::cout << "handling upstream write for fd: " << fd << " with client fd: " << c.getFd() << std::endl;
	// std::cout << c << std::endl;
	Request req = (*c.getRequest());
	t_proxyPass pp = parseProxyPass(req.getIt()->second._proxy_pass);

	std::string proxyRequest;
	proxyRequest += req.getMethodType() + " " + pp.path + " " + req.getHttpVersion() + "\r\n";

	// Copy client headers
	for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin();
			it != req.getHeaders().end(); ++it)
	{
		proxyRequest += it->first + ": " + it->second + "\r\n";
	}

	proxyRequest += "\r\n"; // end headers
	proxyRequest += req.getBody();

	// std::cout << "Sending to client with fd " << fd << ":\n" << proxyRequest << std::endl;

	// 6. Send request to upstream
	ssize_t sent = send(fd, proxyRequest.c_str(), proxyRequest.size(), 0);
	if (sent < 0) {
		close(fd);
		throw std::runtime_error("Unable to send to proxy");
	}
	// std::cout << "req sent to upstream" << std::endl;

	epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = c.getUpstreamFd();
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, c.getUpstreamFd(), &ev);

	// std::cout << "client state and epoll event updated" << std::endl;
}

bool isUpstreamResponseComplete(Client& client)
{
	const std::string& resp = client.getUpstreamBuffer();

	size_t headerEnd = resp.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return false;

	int contentLength = parseContentLength(resp.substr(0, headerEnd + 4));
	if (contentLength < 0)
		return false;

	return resp.size() >= headerEnd + 4 + contentLength;
}

void WebServer::closeUpstream(int upstreamFd) {
	std::cout << "closing the upstream" << upstreamFd << std::endl;
}

void WebServer::finalizeUpstreamResponse(Client& client)
{
	// std::cout << "Upstream response complete" << std::endl;

	// Modify epoll to watch for EPOLLOUT on client socket
	epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = client.getFd();
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client.getFd(), &ev);
	client.setState(RES_RDY);

	// handleWrite(client.getFd());

	// Stop watching upstream socket
	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client.getUpstreamFd(), NULL);
	close(client.getUpstreamFd());
}

void WebServer::handleUpstreamRead(Client& c, int fd) {
	// std::cout << "Reading res from upstream" << std::endl;
	char buffer[4096];

	ssize_t bytes = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);

	if (bytes < 0) {
		// Real error
		std::cerr << "recv upstream failed" << std::endl;
		closeUpstream(fd);
		return;
	}

	if (bytes == 0) {
		// Upstream closed connection
		// std::cout << "Upstream closed connection" << std::endl;
		closeUpstream(fd);
		finalizeUpstreamResponse(c);
		return;
	}

	// Append to upstream buffer
	// std::cout << "buffer from upstream read: " << buffer << std::endl;
	c.addToUpstreamBuffer(buffer);

	// std::cout << "Upstream received " << bytes << " bytes" << std::endl;

	// Optional: check if response is complete
	if (isUpstreamResponseComplete(c)) {
		finalizeUpstreamResponse(c);
		// std::cout << "reading res from upstream done" << std::endl;
	}
}

void WebServer::handleUpstreamEvent(int fd, uint32_t events) {
	// std::cout << "handleUpstreamEvent" << std::endl;
	Client *client = searchClientsUpstream(fd);
	client->setUpstreamFd(fd);
	// Handle EPOLLIN / EPOLLOUT / ERR for upstream socket
	// Similar to handleRead/handleWrite but for upstream
	if (events & EPOLLIN) {
		// std::cout << "UPSTREAM EPOLLIN" << std::endl;
		handleUpstreamRead(*client, fd);
	}

	if (events & EPOLLOUT) {
		// std::cout << "UPSTREAM EPOLLOUT" << std::endl;
		handleUpstreamWrite(*client, fd);
	}
	if (events & (EPOLLHUP | EPOLLERR)) {
		// Handle upstream error/hangup
	}
}

void WebServer::handleCgiRead(int cgiFd)
{
	// std::cout << "Reading from CGI fd: " << cgiFd << std::endl;
	Client* client = searchClientsByCgi(cgiFd);
	if (!client) {
		std::cerr << "Client not found for CGI fd" << std::endl;
		return;
	}

	Cgi* cgi = client->getCgi();
	if (!cgi) {
		std::cerr << "CGI not found for client" << std::endl;
		return;
	}

	// Read available output from CGI (non-blocking)
	if (cgi->readOutput()) {
		// CGI is complete
		finalizeCgiResponse(*client);
	}
	// If not complete, we'll read more on next epoll event
}

void WebServer::finalizeCgiResponse(Client& client)
{
	// std::cout << "CGI response complete, finalizing" << std::endl;

	Cgi* cgi = client.getCgi();
	if (!cgi) return;

	std::string cgiOutput = cgi->getOutput();
	CgiResult result = cgi->parseCgiHeaders(cgiOutput);

	// Build response with CGI output
	if (!client.getResponse()) {
		try {
			client.buildRes();
		} catch (std::exception &e) {
			std::cerr << "Failed to build response: " << e.what() << std::endl;
			return;
		}
	}

	// Update response with CGI data
	Response* res = const_cast<Response*>(client.getResponse());
	if (res) {
		// Check if CGI timed out (empty output = timeout)
		if (cgiOutput.empty())
		{
			// std::cout << "CGI timeout - sending 504 response" << std::endl;
			res->setStatusCode(504);
			res->setStatusTxt("Gateway Timeout");
			std::string timeoutBody = "<html><body><h1>504 Gateway Timeout</h1><p>CGI script execution timeout</p></body></html>";
			res->setBody(timeoutBody);
			res->setHeader("Content-Type", "text/html");
			res->setHeader("Content-Length", intToString(timeoutBody.size()));
		}
		else
		{
			// Normal CGI response processing
			// Set default status code
			int statusCode = 200;
			std::string statusTxt = "OK";

			// Parse status code from CGI Status header if present
			if (result.headers.count("Status") > 0) {
				std::string statusLine = result.headers["Status"];
				// Parse "200 OK" or "404 Not Found" format
				size_t spacePos = statusLine.find(' ');
				if (spacePos != std::string::npos) {
					statusCode = std::atoi(statusLine.substr(0, spacePos).c_str());
					statusTxt = statusLine.substr(spacePos + 1);
				} else {
					statusCode = std::atoi(statusLine.c_str());
				}
			}

			res->setStatusCode(statusCode);
			res->setStatusTxt(statusTxt);
			res->setBody(result.body);

			// Set all CGI headers in response
			for (std::map<std::string, std::string>::iterator it = result.headers.begin();
				 it != result.headers.end(); ++it) {
				// Skip Status header as we've already processed it
				if (it->first != "Status") {
					res->setHeader(it->first, it->second);
				}
			}

			// Ensure Content-Length is set if not present
			std::string body = result.body;
			if (res->getHeaders().count("Content-Length") == 0) {
				res->setHeader("Content-Length", intToString(body.size()));
			}
		}
	}

	// Remove CGI fd from epoll
	struct epoll_event ev;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, cgi->getOutputFd(), &ev) == -1) {
		std::cerr << "epoll_ctl DEL CGI failed" << std::endl;
	}

	// Remove from CGI clients map
	_cgiClients.erase(cgi->getOutputFd());
	delete cgi;
	client.setCgi(NULL);

	// Switch to response sending
	client.setState(RES_RDY);

	// Add client fd back to epoll for writing response
	std::memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLOUT;
	ev.data.fd = client.getFd();
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client.getFd(), &ev) == -1) {
		std::cerr << "epoll_ctl MOD client for response failed" << std::endl;
	}
}

int WebServer::run(void) {
	// 1) Create epoll instance
	_epoll_fd = epoll_create(1);
	if (_epoll_fd == -1)
		throw std::runtime_error(std::string("epoll_create1 failed: "));

	// 2) Register all listening sockets
	for (size_t i = 0; i < _sockets.size(); ++i) {
		int fd = _sockets[i].getServerFd();

		// Make listening sockets non-blocking
		setNonBlocking(fd);

		struct epoll_event ev;
		std::memset(&ev, 0, sizeof(ev));
		ev.events  = EPOLLIN;     // ready to accept()
		ev.data.fd = fd;

		if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
			throw std::runtime_error(std::string("epoll_ctl ADD listen_fd failed: "));
		}

		std::cout << "Listening on port: http://localhost:" << _servers[i].getPort() << std::endl;
	}

	// 3) Main event loop
	struct epoll_event events[MAX_EVENTS];

	int	count = 0;
	while (!g_shutdown) {
		int nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			if (!g_shutdown)
				throw std::runtime_error(std::string("epoll_wait failed: "));
		}

		for (int i = 0; i < nfds; ++i) {
			int      fd = events[i].data.fd;
			uint32_t ev = events[i].events;

			// 1) Listening sockets → incoming connections
			if (isListenFd(fd)) {
				handleAccept(fd);
				continue;
			}

			// 2) CGI output sockets
			Client* cgiClient = searchClientsByCgi(fd);
			if (cgiClient) {
				if (ev & EPOLLIN) {
					handleCgiRead(fd);
				}
				if (ev & (EPOLLHUP | EPOLLERR)) {
					finalizeCgiResponse(*cgiClient);
				}
				continue;
			}

			// 3) Upstream sockets (proxy connections)
			if (this->isUpStream(fd)) {
				// Let a single handler decode EPOLLIN / EPOLLOUT / ERR
				handleUpstreamEvent(fd, ev);
				continue;
			}

			// 4) Normal client sockets
			if (ev & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
				// error / hangup
				closeClient(fd);
				continue;
			}

			if (ev & EPOLLIN) {
				// std::cout << "handle read called" << std::endl;
				handleRead(fd);
			}

			if (ev & EPOLLOUT) {
				// std::cout << "EPOLLOUT" << std::endl;
				handleWrite(fd);
			}
		}

		count++;
	}

	freeAll(*this);
	// Usually unreachable in a daemon, but just in case:
	close(_epoll_fd);
	return 0;
}

std::map<int, Client*>& WebServer::getClients() {
	return _clients;
}

std::map<int, Client*>& WebServer::getUpstreamClients() {
	return _upstreamClient;
}
