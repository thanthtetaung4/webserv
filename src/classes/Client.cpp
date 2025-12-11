/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:42:58 by hthant            #+#    #+#             */
/*   Updated: 2025/12/09 17:51:47 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Client.hpp"
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>

Client::Client() : _fd(-1), _headerComplete(false),
	_contentLength(0), _bodyReceived(0), _responseReady(false),
	_server(NULL), _state(STATE_READING_REQUEST),
	_request(NULL), _response(NULL),
	_upstreamFd(-1), _upstreamConnecting(false) {}

Client::Client(int fd, Server* server) : _fd(fd), _headerComplete(false),
	_contentLength(0), _bodyReceived(0), _responseReady(false),
	_server(server), _state(STATE_READING_REQUEST),
	_request(NULL), _response(NULL),
	_upstreamFd(-1), _upstreamConnecting(false) {}

Client::~Client() {
	if (_request) delete _request;
	if (_response) delete _response;
	closeUpstream();
	if (_fd != -1)
		close(_fd);
}

int Client::getFd() const {
	return _fd;
}

int Client::getUpstreamFd() const {
	return _upstreamFd;
}

ClientState Client::getState() const {
	return _state;
}

void Client::setState(ClientState state) {
	_state = state;
}

Server* Client::getServer() const {
	return _server;
}

void Client::setUpstreamFd(int fd) {
	_upstreamFd = fd;
}

bool Client::headerComplete() const {
	return _headerComplete;
}

bool Client::isResponseReady() const {
	return _responseReady;
}

size_t Client::parseContentLength(const std::string& headers) {
	std::string key = "Content-Length:";
	size_t pos = headers.find(key);
	if (pos == std::string::npos)
		return 0;
	pos += key.size();
	while (pos < headers.size() && headers[pos] == ' ') pos++;

	size_t end = headers.find("\r\n", pos);
	if (end == std::string::npos)
		end = headers.size();

	return std::atoi(headers.substr(pos, end - pos).c_str());
}

void Client::parseHeader() {
	size_t pos = _recvBuffer.find("\r\n\r\n");
	if (pos != std::string::npos) {
		_headerComplete = true;
		std::string headers = _recvBuffer.substr(0, pos + 4);
		_contentLength = parseContentLength(headers);
	}
}

void Client::appendToSendBuffer(const std::string& data) {
	_sendBuffer.append(data);
}

std::string Client::getRawRequest() const {
	return _recvBuffer;
}

bool Client::handleRead() {
	char buf[4096];
	while (true) {
		ssize_t n = recv(_fd, buf, sizeof(buf), 0);
		if (n == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			return false;
		}
		if (n == 0) {
			return false;
		}
		_recvBuffer.append(buf, n);

		if (!_headerComplete)
			parseHeader();

		if (_headerComplete) {
			size_t bodyStart = _recvBuffer.find("\r\n\r\n") + 4;
			_bodyReceived = _recvBuffer.size() - bodyStart;
			if (_recvBuffer.size() >= bodyStart + _contentLength) {
				_responseReady = true;
				break;
			}
		}
	}
	return true;
}

void Client::buildResponse() {
	if (!_server || !_responseReady)
		return;

	std::cout << "======================= START RAW REQ =======================" << std::endl;
	std::cout << _recvBuffer << std::endl;
	std::cout << "======================= END RAW REQ =======================" << std::endl;

	try {
		_request = new Request(_recvBuffer, *_server);
		_response = new Response(*_request, *_server);

		// Check if this location needs reverse proxy
		std::map<std::string, location> locs = _server->getLocation();
		for (std::map<std::string, location>::iterator it = locs.begin();
			 it != locs.end(); ++it) {
			if (!it->second._proxy_pass.empty()) {
				// This is a reverse proxy request
				_state = STATE_PROXYING_UPSTREAM;
				return;
			}
		}

		// Normal response
		_sendBuffer = _response->toStr();
		_state = STATE_WRITING_RESPONSE;
	} catch (const std::exception& e) {
		std::cerr << "Error building response: " << e.what() << std::endl;
		_state = STATE_CLOSED;
	}
}

void Client::startProxyConnection(const std::string& host, const std::string& port) {
	_upstreamRequest = _recvBuffer;
	// Force Connection: close for upstream
	size_t connPos = _upstreamRequest.find("Connection:");
	if (connPos != std::string::npos) {
		size_t lineEnd = _upstreamRequest.find("\r\n", connPos);
		_upstreamRequest.replace(connPos, lineEnd - connPos, "Connection: close");
	} else {
		size_t hdrEnd = _upstreamRequest.find("\r\n\r\n");
		if (hdrEnd != std::string::npos)
			_upstreamRequest.insert(hdrEnd, "\r\nConnection: close");
	}

	// Store host/port for connection attempt (will be initiated by WebServer)
	(void)host;
	(void)port;
	// For now just mark that we need to connect
	_state = STATE_PROXYING_UPSTREAM;
	_upstreamConnecting = true;
}

void Client::handleProxyConnected() {
	_upstreamConnecting = false;
	_state = STATE_PROXYING_UPSTREAM;
}

bool Client::handleUpstreamRead() {
	if (_upstreamFd < 0)
		return false;

	char buf[4096];
	while (true) {
		ssize_t n = recv(_upstreamFd, buf, sizeof(buf), 0);
		if (n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			return false;
		}
		if (n == 0) {
			// Upstream closed - we got full response
			_state = STATE_PROXYING_RESPONSE;
			return true;
		}
		_upstreamBuffer.append(buf, n);
	}
	return true;
}

bool Client::handleUpstreamWrite() {
	// For now, handled by WebServer with non-blocking logic
	return true;
}

void Client::completeProxyResponse() {
	_sendBuffer = _upstreamBuffer;
	_state = STATE_WRITING_RESPONSE;
}

bool Client::handleWrite() {
	while (!_sendBuffer.empty()) {
		ssize_t n = send(_fd,
			_sendBuffer.data(),
			_sendBuffer.size(),
			0);

		if (n == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return true;
			return false;
		}
		_sendBuffer.erase(0, n);
	}
	_state = STATE_CLOSED;
	return false;
}

void Client::closeUpstream() {
	if (_upstreamFd >= 0) {
		close(_upstreamFd);
		_upstreamFd = -1;
	}
}

void Client::closeClient() {
	closeUpstream();
	if (_fd != -1) {
		close(_fd);
		_fd = -1;
	}
}
