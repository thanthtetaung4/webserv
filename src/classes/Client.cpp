/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:42:58 by hthant            #+#    #+#             */
/*   Updated: 2025/12/18 17:57:47 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Client.hpp"
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>

Client::Client(void) : server(*(new Server())) {
	std::cout << "Client Default Constructor called" << std::endl;
}

Client::Client(int fd, const Server& server) : server(const_cast<Server&>(server)) {
	std::cout << "Client fd, server constructor called" << std::endl;
	this->fd = fd;

	this->inBuffer = "";
	this->upstreamBuffer = "";

	this->request = NULL;
	this->response = NULL;

	this->state = READ_REQ;
	this->contentLength = 0;
	this->headerEndPos = 0;
}

Client&	Client::operator=(const Client& other) {
	if (this != &other) {
		this->fd = other.fd;
		this->server = other.server;
		this->inBuffer = other.inBuffer;
		this->upstreamBuffer = other.upstreamBuffer;
		this->request = other.request;
		this->response = other.response;
		this->state = other.state;
		this->headerEndPos = other.headerEndPos;
		this->contentLength = other.contentLength;
		std::cout << "client copied as: " << *this << std::endl;
	}
	return *this;
}


Client::~Client() {}

bool	Client::buildReq() {
	try {
		request = new Request(this->inBuffer, this->server);
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool	Client::buildRes() {
	try {
		response = new Response(*this->request, this->server);
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool	Client::isProxyPass(void) const {
	if (this->request) {
		if (this->request->getIt() != this->server.getLocation().end()) {
			t_location loc = this->request->getIt()->second;
			if (!loc._proxy_pass.empty())
				return (true);
		}
	}
	return (false);
}

void	Client::appendRecvBuffer(std::string buff) {
	this->inBuffer += buff;
}

void	Client::addToUpstreamBuffer(std::string buff) {
	this->upstreamBuffer += buff;
}

int	Client::getFd(void) const {
	return (this->fd);
}

const Response*	Client::getResponse(void) const{
	if (this->response)
		return (this->response);
	return (NULL);
}

const Request*	Client::getRequest(void) const{
	if (this->request)
		return (this->request);
	return (NULL);
}

ClientState	Client::getState(void) const {
	return (this->state);
}

const std::string&	Client::getInBuffer(void) const {
	return (this->inBuffer);
}

size_t	Client::getContentLength(void) const {
	return (this->contentLength);
}

void	Client::setContentLength(size_t cl) {
	this->contentLength = cl;
}

bool	Client::foundHeader(void) const {
	return (this->headerEndPos != 0);
}

size_t	Client::getHeaderEndPos(void) const {
	return (this->headerEndPos);
}

const std::string&	Client::getUpstreamBuffer(void) const {
	return (this->upstreamBuffer);
}

int	Client::getUpstreamFd(void) const {
	return (this->upstreamFd);
}

void	Client::setUpstreamFd(int fd) {
	this->upstreamFd = fd;
}

void	Client::setHeaderEndPos(size_t pos) {
	this->headerEndPos = pos;
}

void	Client::setInBuffer(std::string rawStr) {
	this->inBuffer += rawStr;
}

void	Client::setState(ClientState state) {
	this->state = state;
}

bool	Client::removeFromOutBuffer(size_t pos) {
	if (pos > this->outBuffer.size())
		return (false);
	this->outBuffer.erase(0, pos);
	return (true);
}

std::string	Client::getOutBuffer(void) const {
	return (this->outBuffer);
}

void	Client::setOutBuffer(std::string rawStr) {
	this->outBuffer = rawStr;
}



std::ostream &operator<<(std::ostream &os, const Client &client) {
	os << "Client FD: " << client.getFd();
	os << "\nResquest: " << client.getRequest();
	os << "\nResponse: " << client.getResponse();
	os << "\nState: " << client.getState();
	os << std::endl;
	return os;
}
