/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:42:58 by hthant            #+#    #+#             */
/*   Updated: 2025/12/13 23:35:45 by taung            ###   ########.fr       */
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
	this->outBuffer = "";

	this->request = NULL;
	this->response = NULL;

	this->upstreamFd = 0;
	this->isProxyClient = false;
}

Client&	Client::operator=(const Client& other) {
	if (this != &other) {
		this->fd = other.fd;
		this->server = other.server;
		this->inBuffer = other.inBuffer;
		this->outBuffer = other.outBuffer;
		this->request = other.request;
		this->response = other.response;
		this->upstreamFd = other.upstreamFd;
		this->isProxyClient = other.isProxyClient;
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

void	Client::appendRecvBuffer(std::string buff) {
	this->inBuffer += buff;
}

void	Client::addToOutBuffer(std::string buff) {
	this->outBuffer += buff;
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

void	Client::setHeaderEndPos(size_t pos) {
	this->headerEndPos = pos;
}

void	Client::setInBuffer(std::string rawStr) {
	this->inBuffer += rawStr;
}

void	Client::setState(ClientState state) {
	this->state = state;
}

std::ostream &operator<<(std::ostream &os, const Client &client) {
	os << "Client FD: " << client.getFd();
	os << "\nResquest: " << client.getRequest();
	os << "\nResponse: " << client.getResponse();
	return os;
}
