/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 14:06:45 by taung             #+#    #+#             */
/*   Updated: 2025/12/20 22:04:44 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/Socket.hpp"
# include "../../include/ServerException.hpp"
#include <sys/socket.h>
#include <cstring>
#include <cerrno>
#include <unistd.h>

Socket::Socket() {
	// throw InvalidSocket();
	this->addr_len = sizeof(addr);
	// Zero the sockaddr_in to avoid any uninitialized fields
	std::memset(&this->addr, 0, sizeof(this->addr));
	this->addr.sin_family = AF_INET;
	this->addr.sin_addr.s_addr = INADDR_ANY;
}

Socket::Socket(unsigned int port) {
	this->addr_len = sizeof(addr);
	// Zero the sockaddr_in to avoid any uninitialized fields
	std::memset(&this->addr, 0, sizeof(this->addr));
	this->addr.sin_family = AF_INET;
	this->addr.sin_addr.s_addr = INADDR_ANY;
	this->addr.sin_port = htons(port);
}

Socket::~Socket() {
	// closeSock();
}

void	Socket::setServerFd(int fd) {
	this->serverFd = fd;
}

int		Socket::getServerFd(void) const {
	return (this->serverFd);
}

void					Socket::openSock(void) {
	this->serverFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (this->serverFd < 0)
		throw	UnableToOpenSocket();
	// Allow quick restart: reuse address (and port where supported)
	int opt = 1;
	if (setsockopt(this->serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		std::cerr << "warning: setsockopt SO_REUSEADDR failed" << std::endl;
	std::cout << "socket open OK" << std::endl;
}

void					Socket::bindSock(void) {
	if (bind(this->serverFd, (struct sockaddr*)&(this->addr), sizeof(this->addr)) < 0)
		throw	UnableToBind();
	std::cout << "socket bind OK" << std::endl;
}

void					Socket::listenSock(void) {
	if (listen(this->serverFd, 10) < 0)
		throw	UnableToListen();
	std::cout << "socket listen OK" << std::endl;
}

void			Socket::prepSock(void) {
	this->openSock();
	this->bindSock();
	this->listenSock();
}

sockaddr_in*	Socket::getAddrPtr(void) {
	return (&this->addr);
}

socklen_t*		Socket::getAddrLen(void) {
	return (&this->addr_len);
}

void			Socket::closeSock(void) {
	if (close(this->serverFd) < 0)
		std::cerr << "warning: close socket failed: " << std::endl;
	else
		std::cout << "socket closed OK" << std::endl;
}

std::ostream&	operator<<(std::ostream& os, const Socket& sock) {
	os << "socket: " << sock.getServerFd() << std::endl;
	return (os);
}
