/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/09 19:04:56 by taung            ###   ########.fr       */
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


int WebServer::serve(void)
{
	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
		perror("epoll_create1");
		return 1;
	}
	// Register all server sockets with epoll
	for (size_t i = 0; i < _sockets.size(); ++i)
	{

		int fd = _sockets[i].getServerFd();
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.u32 = i; // Store index to map back to Server
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			close(epoll_fd);
			return 1;
		}
		std::cout << "Listening on port: http://localhost:" << _servers[i].getPort() << std::endl;
	}

	struct epoll_event events[MAX_EVENTS];
	while (true)
	{
		// std::cout << "here" << std::endl;
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1);
		if (nfds == -1)
		{
			perror("epoll_wait");
			break;
		}
		for (int n = 0; n < nfds; ++n)
		{
			size_t idx = events[n].data.u32;
			int listen_fd = _sockets[idx].getServerFd();
			sockaddr_in client_addr;
			socklen_t client_len = sizeof(client_addr);
			int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
			if (client_fd < 0)
			{
				perror("accept");
				continue;
			}
			char buffer[4096];
			// std::cout << "=================REQUEST============================" << std::endl;

			// ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
			std::string requestStr;
			ssize_t bytes_received = 0;
			while (true) {
				ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
				if (bytes_received <= 0) break;
				std::cout << "================================= BYTE RECIEVED START =====================" << std::endl;
				std::cout << bytes_received << std::endl;
				std::cout << "================================= BYTE RECIEVED END =====================" << std::endl;
				requestStr.append(buffer, bytes_received);

				// stop when the headers are received + full body matches Content-Length
				if (requestStr.find("\r\n\r\n") != std::string::npos) {
					size_t bodyStart = requestStr.find("\r\n\r\n") + 4;
					std::string headers = requestStr.substr(0, bodyStart);
					size_t contentLength = parseContentLength(headers);

					if (requestStr.size() >= bodyStart + contentLength)
						break;
				}
			}
			if (bytes_received < 0)
			{
				perror("recv");
				close(client_fd);
				continue;
			}
			buffer[bytes_received] = '\0';
			std::cout << "Request received on port " << _servers[idx].getPort() << ":\n";
			std::cout << "================================= REQUEST PLAIN =====================" << std::endl;
			std::cout << requestStr << std::endl;
			std::cout << "================================= REQUEST PLAIN END =====================" << std::endl;

			Request req(requestStr, _servers[idx]);
			// std::cout << "================================= SEVER TEST =====================" << std::endl;
			// std::cout << _servers[idx] << std::endl;
			// std::cout << "================================= SERVER TEST END =====================" << std::endl;
			// std::cout << "================================= REQ OBJ =====================" << std::endl;
			// std::cout << req << std::endl;
			// std::cout << "================================= REQ OBJ =====================" << std::endl;
			// int i = req.validateAgainstConfig(_servers[idx]);
			// if(i != 200) {
			// 		Response res(i);
			// }

			// std::cout << this->isProxyPass(req.getPath(), _servers[idx]) << std::endl;
			// if (this->isProxyPass(req.getPath(), _servers[idx]))
			// {
			// 	std::cout << "POST method detected" << std::endl;
			// 	std::string rawRes = this->handleReverseProxy(req, _servers[idx]);
			// 	// just send the plain text to the client no need to change it back to Response obj
			// 	std::cout << "================================= SERVER TEST START =====================" << std::endl;
			// 	std::cout << rawRes << std::endl;
			// 	std::cout << "================================= SERVER TEST END =====================" << std::endl;
			// 	ssize_t sent = send(client_fd, rawRes.c_str(), rawRes.size(), 0);
			// 	if (sent < 0)
			// 	{
			// 		perror("send");
			// 	}
			// 	close(client_fd);
			// 	continue;
			// }
			// if (isCGI(req.getPath(), _servers[idx]))
			// {
			// 	Cgi cgi(req, _servers[idx]);
			// }
			// std::cout << req << std::endl;
			// std::cout << "================================= RESPONSE =====================" << std::endl;

			// std::cout << "creating res" << std::endl;
			Response res(req, _servers[idx]);
			// std::cout << "printing res" << std::endl;
			// std::cout << res << std::endl;
			// std::cout << "res printed" << std::endl;
			// std::cout << "================================= RESPONSE END =====================" << std::endl;
			std::string httpResponse = res.toStr();

			// std::cout << "================================= FINAL RESPONSE =====================" << std::endl;
			// std::cout << "http res: " << httpResponse << std::endl;
			// std::cout << "================================= FINAL RESPONSE END =====================" << std::endl;

			ssize_t sent = send(client_fd, httpResponse.c_str(), httpResponse.size(), 0);
			if (sent < 0)
			{
				perror("send");
			}
			close(client_fd);
		}
	}
	close(epoll_fd);

	return 0;
}


/*
	o
*/
