/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/16 19:06:52 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
#include <errno.h>
#include "../../include/Client.hpp"

WebServer::WebServer() {}

WebServer::~WebServer() {}

// WebServer::WebServer(const WebServer &src) {}

// WebServer &WebServer::operator=(const WebServer &other) {}
void WebServer::setServer(std::string configFile)
{
	t_iterators it;
	std::string target = "server {";

	std::ifstream config(configFile.c_str());
	if (!config)
		throw "Unable to open file!!";
	else
	{
		std::stringstream ss;
		ss << config.rdbuf();
		std::string content = ss.str();
		it = Server::getIterators(content, content.begin(), target, target);
		while (it.it1 != content.end())
		{
			Server server;
			server.fetchSeverInfo(it);
			addServer(server);
			if (it.it2 != content.end())
				it = Server::getIterators(content, it.it2, target, target);
			else
				break;
		}
		config.close();
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
		std::cout << (*it) << std::endl;
	}
}

std::vector<Server> WebServer::getServers() const
{
	return _servers;
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
        throw std::runtime_error(std::string("fcntl(F_GETFL): ") + std::strerror(errno));
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error(std::string("fcntl(F_SETFL): ") + std::strerror(errno));
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

void WebServer::handleAccept(int listenfd) {
	while (true) {
		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		int client_fd = accept(listenfd, (sockaddr*)&addr, &len);

		if (client_fd < 0) {
			if (errno == EAGAIN) break;
			perror("accept");
			break;
		}

		setNonBlocking(client_fd);

		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = client_fd;
		epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
		_clients[client_fd] = new Client(client_fd, _servers[searchSocketIndex(_sockets, listenfd)]);
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
		std::cout << vec[i].getServerFd() << std::endl;
		if (vec[i].getServerFd() == key)
			return (i);
	}
	return (-1);
}

void WebServer::readFromClient(Client& client) {
	std::cout << "reading from client" << std::endl;
	char buffer[4096];

	// Read ONLY what's available right now (non-blocking)
	ssize_t bytes_received = recv(client.getFd(), buffer, sizeof(buffer), MSG_DONTWAIT);

	// Handle recv errors
	if (bytes_received < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// No data available right now, that's OK
			// epoll will trigger again when more data arrives
			return;
		}
		// Real error
		perror("recv");
		closeClient(client.getFd());
		return;
	}

	// Client closed connection
	if (bytes_received == 0) {
		std::cout << "Client closed connection" << std::endl;
		closeClient(client.getFd());
		return;
	}

	// Append what we just received
	client.appendRecvBuffer(std::string(buffer, bytes_received));
	std::cout << "Received " << bytes_received << " bytes" << std::endl;

	std::string requestStr = client.getInBuffer();

	// Check if we have complete headers yet
	if (!client.foundHeader()) {
		std::cout << "header not found, finding" << std::endl;
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
			std::cout << "Request complete (headers + body)" << std::endl;
			updateClient(client);
			return;
		}

		// Body incomplete, wait for next epoll event
		std::cout << "Body incomplete: have " << (requestStr.size() - bodyStart)
					<< ", need " << contentLength << std::endl;
		return;
	}

	// Headers already found in previous call, check if body is now complete
	size_t bodyStart = client.getHeaderEndPos();
	int contentLength = client.getContentLength();

	std::cout << "Checking body: have " << (requestStr.size() - bodyStart)
				<< " bytes, need " << contentLength << " bytes" << std::endl;

	if (requestStr.size() >= bodyStart + contentLength) {
		std::cout << "Request body complete" << std::endl;
		updateClient(client);
		return;
	}

	// Still waiting for more body data
	std::cout << "Still waiting for more body data" << std::endl;
}

void	WebServer::updateClient(Client& client) {
	std::cout << "updating client" << std::endl;
	// Creating the Request Intance
	if (!client.buildReq())
		throw "Fatal Err: Response Cannot be created";

	// std::cout << *client.getRequest() << std::endl;

	// Setting epoll event to EPOLLOUT
	struct epoll_event ev;
	std::memset(&ev, 0, sizeof(ev));
	ev.events  = EPOLLOUT; // ready to write
	ev.data.fd = client.getFd();

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, client.getFd(), &ev) == -1) {
		throw std::runtime_error(std::string("epoll_ctl ADD listen_fd failed: ") +
									std::strerror(errno));
	}
	client.setState(RES_RDY);
	std::cout << "updating done" << std::endl;
}

int	setupUpstreamSock(Client& client) {
	std::cout << "Setting up sockets" << std::endl;
	t_proxyPass pp = parseProxyPass((*client.getRequest()).getIt()->second._proxy_pass);
	std::cout << "Proxying to " << pp.host << ":" << pp.port << pp.path << std::endl;

	// 2. Create upstream client socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cout << "Failed to create proxy socket" << std::endl;
		return (-1);
	}

	// 3. Build remote address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = htons(std::atoi(pp.port.c_str()));
	server_addr.sin_addr.s_addr = inet_addr(pp.host.c_str());

	if (server_addr.sin_addr.s_addr == INADDR_NONE) {
		close(sockfd);
		std::cout << "socket err" << std::endl;
		return (-1);
	}

	// 4. Connect to upstream
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect");
		close(sockfd);
		return (-1);
	}

	std::cout << "Connected to upstream" << std::endl;
	return (sockfd);
}

void	WebServer::handleRead(int fd) {
	Client* client = searchClients(fd);
	if (!client) throw "client not found";

	std::cout << "client has state" << client->getState() << std::endl;

	if (client->getState() == READ_REQ) {
		readFromClient(*client);
		std::cout << "client state: " << client->getState() << std::endl;
	}

	if (client->getState() == REQ_RDY) {
		updateClient(*client);
	}

	if (client->isProxyPass()) {
		int	ppassFd = setupUpstreamSock(*client);
		if (ppassFd == -1) {
			std::cout << "ppass fd cannot be oppened" << std::endl;
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
			throw std::runtime_error(std::string("epoll_ctl ADD ppassFD failed: ") +
										std::strerror(errno));
		}
	}
}

void WebServer::handleWrite(int fd) {
	std::cout << "handle write called" << std::endl;
	Client* client = searchClients(fd);
	std::cout << *client << std::endl;
	std::cout << "raw res from upstream: " << client->getUpstreamBuffer() << std::endl;
	if (client->getState() == RES_RDY) {
		if (!client->getUpstreamBuffer().empty()) {
			std::cout << "WAIT_UPSTREAM" << std::endl;
			std::cout << "writing to client with res from upstream:" << client->getUpstreamBuffer() << std::endl;

			std::string httpResponse = client->getUpstreamBuffer();

			ssize_t sent = send(fd, httpResponse.c_str(), httpResponse.size(), 0);
			if (sent < 0)
			{
				perror("send error:");
			}
			closeClient(fd);
		} else {
			std::cout << "RES RDY" << std::endl;
			// Creating Response Instance
			if (!client->buildRes())
				throw "Fatal Err: Response cannot be create";

			std::cout << *client->getResponse() << std::endl;
			std::string	httpResponse = client->getResponse()->toStr();

			ssize_t sent = send(fd, httpResponse.c_str(), httpResponse.size(), 0);
			if (sent < 0)
			{
				perror("send error:");
			}
			closeClient(fd);
		}
	}
	std::cout << "handle write done" << std::endl;

	// if (client->getState() == WAIT_UPSTREAM) {
	// 	std::cout << "WAIT_UPSTREAM" << std::endl;
	// 	std::cout << "writing to client with res from upstream:" << client->getUpstreamBuffer() << std::endl;

	// 	std::string httpResponse = client->getUpstreamBuffer();

	// 	ssize_t sent = send(fd, httpResponse.c_str(), httpResponse.size(), 0);
	// 	if (sent < 0)
	// 	{
	// 		perror("send error:");
	// 	}
	// 	closeClient(fd);
	// }
}

void WebServer::closeClient(int fd) {
	std::map<int, Client*>::iterator it = this->_clients.find(fd);
	if (it == this->_clients.end())
		return;

	// 1) Remove from epoll (so it stops monitoring this fd)
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		perror("epoll_ctl DEL");
	}

	// 2) Close the socket
	if (close(fd) == -1) {
		perror("close");
	}

	// 3) Remove from clients map
	this->_clients.erase(it);

	std::cout << "Client " << fd << " closed and removed from epoll" << std::endl;
}

void WebServer::handleUpstreamWrite(Client& c, int fd) {
	std::cout << "handling upstream write for fd: " << fd << " with client fd: " << c.getFd() << std::endl;
	std::cout << c << std::endl;
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

	std::cout << "Sending to client with fd " << fd << ":\n" << proxyRequest << std::endl;

	// 6. Send request to upstream
	ssize_t sent = send(fd, proxyRequest.c_str(), proxyRequest.size(), 0);
	if (sent < 0) {
		close(fd);
		throw std::runtime_error("Unable to send to proxy");
	}
	std::cout << "req sent to upstream" << std::endl;

	epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = c.getUpstreamFd();
	epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, c.getUpstreamFd(), &ev);

	std::cout << "client state and epoll event updated" << std::endl;
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
	std::cout << "Upstream response complete" << std::endl;

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
	std::cout << "Reading res from upstream" << std::endl;
	char buffer[4096];

	ssize_t bytes = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);

	if (bytes < 0) {
	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		// No more data for now
		return;
	}

	// Real error
	perror("recv upstream");
	closeUpstream(fd);
	return;
	}

	if (bytes == 0) {
		// Upstream closed connection
		std::cout << "Upstream closed connection" << std::endl;
		closeUpstream(fd);
		finalizeUpstreamResponse(c);
		return;
	}

	// Append to upstream buffer
	std::cout << "buffer from upstream read: " << buffer << std::endl;
	c.addToUpstreamBuffer(buffer);

	std::cout << "Upstream received " << bytes << " bytes" << std::endl;

	// Optional: check if response is complete
	if (isUpstreamResponseComplete(c)) {
		finalizeUpstreamResponse(c);
		std::cout << "reading res from upstream done" << std::endl;
	}
}


void WebServer::handleUpstreamEvent(int fd, uint32_t events) {
	std::cout << "handleUpstreamEvent" << std::endl;
	Client *client = searchClientsUpstream(fd);
	client->setUpstreamFd(fd);
	// Handle EPOLLIN / EPOLLOUT / ERR for upstream socket
	// Similar to handleRead/handleWrite but for upstream
	if (events & EPOLLIN) {
		std::cout << "UPSTREAM EPOLLIN" << std::endl;
		handleUpstreamRead(*client, fd);
	}

	if (events & EPOLLOUT) {
		std::cout << "UPSTREAM EPOLLOUT" << std::endl;
		handleUpstreamWrite(*client, fd);
	}
	if (events & (EPOLLHUP | EPOLLERR)) {
		// Handle upstream error/hangup
	}
}


int WebServer::run(void) {
	// 1) Create epoll instance
	_epoll_fd = epoll_create(1);
	if (_epoll_fd == -1)
		throw std::runtime_error(std::string("epoll_create1 failed: ") + std::strerror(errno));

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
			throw std::runtime_error(std::string("epoll_ctl ADD listen_fd failed: ") +
										std::strerror(errno));
		}

		std::cout << "Listening on port: http://localhost:" << _servers[i].getPort() << std::endl;
	}

	// 3) Main event loop
	struct epoll_event events[MAX_EVENTS];

	int	count = 0;
	while (true) {
		std::cout << count << " times looping" << std::endl;
		int nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			if (errno == EINTR)
				continue; // interrupted by signal, just retry
			throw std::runtime_error(std::string("epoll_wait failed: ") + std::strerror(errno));
		}

		for (int i = 0; i < nfds; ++i) {
			std::cout << "epoll loop: " << i << std::endl;
			int      fd = events[i].data.fd;
			uint32_t ev = events[i].events;

			// 1) Listening sockets → incoming connections
			if (isListenFd(fd)) {
				handleAccept(fd);
				continue;
			}

			// 2) Upstream sockets (proxy connections)
			if (this->isUpStream(fd)) {
				// Let a single handler decode EPOLLIN / EPOLLOUT / ERR
				handleUpstreamEvent(fd, ev);
				continue;
			}

			// 3) Normal client sockets
			if (ev & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
				// error / hangup
				closeClient(fd);
				continue;
			}

			if (ev & EPOLLIN) {
				std::cout << "handle read called" << std::endl;
				handleRead(fd);
			}

			if (ev & EPOLLOUT) {
				std::cout << "EPOLLOUT" << std::endl;
				handleWrite(fd);
			}
		}
		count++;
	}

	// Usually unreachable in a daemon, but just in case:
	close(_epoll_fd);
	return 0;
}


