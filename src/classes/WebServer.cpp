/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/14 01:12:22 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../../include/WebServer.hpp"
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
	std::map<int, Client>::iterator i;

	for (i = _clients.begin() ; i != _clients.end(); i++) {
		if (i->first == fd)
			return &(i->second);
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
		_clients[client_fd] = Client(client_fd, _servers[searchSocketIndex(_sockets, listenfd)]);
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

// void WebServer::handleRead(int fd) {
// 	Client* client = searchClients(fd);
// 	if (!client) throw "client connection lost";

// 	char buffer[4096];
// 	// std::cout << "=================REQUEST============================" << std::endl;

// 	std::string requestStr;
// 	ssize_t bytes_received = 0;
// 	while (true) {
// 		ssize_t bytes_received = recv(client->getFd(), buffer, sizeof(buffer), 0);
// 		if (bytes_received <= 0) {
// 			std::cout << "No more data to read or error occurred." << std::endl;
// 			break;
// 		}
// 		std::cout << "================================= BYTE RECIEVED START =====================" << std::endl;
// 		std::cout << bytes_received << std::endl;
// 		std::cout << "================================= BYTE RECIEVED END =====================" << std::endl;

// 		requestStr.append(buffer, bytes_received);

// 		// stop when the headers are received + full body matches Content-Length
// 		if (requestStr.find("\r\n\r\n") != std::string::npos) {
// 			size_t bodyStart = requestStr.find("\r\n\r\n") + 4;
// 			std::string headers = requestStr.substr(0, bodyStart);
// 			size_t contentLength = parseContentLength(headers);

// 			if (requestStr.size() >= bodyStart + contentLength)
// 				break;
// 		}
// 	}
// 	if (bytes_received < 0) {
// 		perror("recv err:");
// 		close(client->getFd());
// 		return;
// 	}

// 	// Append to client buffer
// 	client->setInBuffer(std::string(requestStr));

// 	std::cout << "================================" << std::endl;
// 	std::cout << requestStr << std::endl;
// 	std::cout << "================================" << std::endl;

// 	// Creating the Request Intance
// 	if (!client->buildReq())
// 		throw "Fatal Err: Response Cannot be created";

// 	std::cout << *client->getRequest() << std::endl;

// 	// Setting epoll event to EPOLLOUT
// 	struct epoll_event ev;
// 	std::memset(&ev, 0, sizeof(ev));
// 	ev.events  = EPOLLOUT; // ready to write
// 	ev.data.fd = fd;

// 	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
// 		throw std::runtime_error(std::string("epoll_ctl ADD listen_fd failed: ") +
// 									std::strerror(errno));
// 	}
// }

// void	WebServer::readFromClient(Client& client) {
// 	std::cout << "reading from client" << std::endl;
// 	char buffer[4096];
// 	ssize_t bytes_received = 0;
// 	std::string requestStr;

// 	bytes_received = recv(client.getFd(), buffer, sizeof(buffer), 0);

// 	// Handle recv errors
// 	if (bytes_received < 0) {
// 		if (errno == EAGAIN || errno == EWOULDBLOCK) {
// 			// No data available right now, that's OK
// 			return;
// 		}
// 		// Real error
// 		perror("recv");
// 		closeClient(client.getFd());
// 		return;
// 	}

// 	// If clinet closes before sending full req discard the Client
// 	if (bytes_received == 0) {
// 		std::cout << "Client closed connection" << std::endl;
// 		closeClient(client.getFd());
// 		return;
// 	}

// 	client.appendRecvBuffer(buffer);

// 	requestStr = client.getInBuffer();

// 	if (!client.foundHeader()) {
// 		std::cout << "header not found, finding" << std::endl;
// 		size_t pos = requestStr.find("\r\n\r\n");
// 		if (pos == std::string::npos)
// 			return;

// 		size_t bodyStart = pos + 4;
// 		client.setHeaderEndPos(bodyStart);

// 		std::string headers = requestStr.substr(0, bodyStart);
// 		std::cout << "======================" << std::endl;
// 		std::cout << headers << std::endl;
// 		std::cout << "======================" << std::endl;
// 		client.setContentLength(parseContentLength(headers));
// 		std::cout << "client content len, " << client.getContentLength() << std::endl;
// 		if (client.getContentLength() == 0) {
// 			std::cout << "request has no body" << std::endl;
// 			client.setState(REQ_RDY);
// 		}

// 	}
// 	if (client.foundHeader()) {
// 		size_t bodyStart = client.getHeaderEndPos();
// 		if (requestStr.size() >= bodyStart + client.getContentLength()) {
// 			std::cout << "client state changed" << std::endl;
// 			client.setState(REQ_RDY);
// 		}
// 	}
// 	std::cout << "client content length: " << client.getContentLength() << " , request str: " << client.getInBuffer().size() << std::endl;
// 	std::cout << "reading done" << std::endl;
// }

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
		std::cout << "======================" << std::endl;
		std::cout << headers << std::endl;
		std::cout << "======================" << std::endl;

		int contentLength = parseContentLength(headers);
		client.setContentLength(contentLength);
		std::cout << "Content-Length: " << contentLength << std::endl;

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
	std::cout << "updating done" << std::endl;
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
}

void WebServer::handleWrite(int fd) {
	Client* client = searchClients(fd);

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

void WebServer::closeClient(int fd) {
	std::map<int, Client>::iterator it = this->_clients.find(fd);
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

void WebServer::handleUpstreamEvent(int fd, uint32_t events) {
	(void)fd;
	// Handle EPOLLIN / EPOLLOUT / ERR for upstream socket
	// Similar to handleRead/handleWrite but for upstream
	if (events & EPOLLIN) {
		// Handle upstream read
	}

	if (events & EPOLLOUT) {
		// Handle upstream write
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

	while (true) {
		int nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			if (errno == EINTR)
				continue; // interrupted by signal, just retry
			throw std::runtime_error(std::string("epoll_wait failed: ") + std::strerror(errno));
		}

		for (int i = 0; i < nfds; ++i) {
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
				std::cout << "handle write called" << std::endl;
				handleWrite(fd);
			}
		}
	}

	// Usually unreachable in a daemon, but just in case:
	close(_epoll_fd);
	return 0;
}
