/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:42:58 by hthant            #+#    #+#             */
/*   Updated: 2025/12/12 06:31:13 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Client.hpp"
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>

Client::Client(int fd, const Server& server) : server(const_cast<Server&>(server)) {
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
	}
	return *this;
}


Client::~Client() {}

bool	Client::buildReq() {
	try {
		request = new Request(this->inBuffer, this->server);
	} catch (std::exception e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	return true;
}

bool	Client::buildRes() {
	try {
		response = new Response(*this->request, this->server);
	} catch (std::exception e) {
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

const Response&	Client::getResponse(void) {
	return (*this->response);
}

void	Client::setInBuffer(std::string rawStr) {
	this->inBuffer += rawStr;
}
