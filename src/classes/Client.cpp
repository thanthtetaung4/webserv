/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:42:58 by hthant            #+#    #+#             */
/*   Updated: 2025/12/11 20:50:10 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Client.hpp"
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <iostream>
#include <sstream>

Client::Client(int fd, const Server& server) {
	this->fd = fd;
	this->server = server;

	this->inBuffer = "";
	this->outBuffer = "";

	this->request = NULL;
	this->response = NULL;

	this->headerEndPos = 0;
	this->contentLength = 0;
	this->hasContentLength = false;

	this->upstreamFd = 0;
	this->isProxyClient = false;
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

bool	Client::isRequestComplete(void) {
	return (this->inBuffer.find("\r\n\r\n") == this->headerEndPos);
}
