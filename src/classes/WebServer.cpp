/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/07 21:38:48 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../../include/WebServer.hpp"
#include "../../include/Request.hpp"
#include "../../include/Response.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <set>
#include <fcntl.h>
#include <map>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../../include/Validator.hpp"
#include "../../include/Cgi.hpp"
# define TIMEOUT 5000
# include "../../include/Client.hpp"

// Proxy connection state for asynchronous proxy handling
struct ProxyConn {
	int client_fd;
	int upstream_fd;
	std::string request_buffer;  // Data to send to upstream
	std::string response_buffer; // Data received from upstream
	bool connecting;             // True during non-blocking connect
	size_t server_idx;

	ProxyConn(int cfd, int ufd, const std::string &req, bool conn, size_t idx)
		: client_fd(cfd), upstream_fd(ufd), request_buffer(req),
		  response_buffer(""), connecting(conn), server_idx(idx) {}
};

WebServer::WebServer() {}

WebServer::~WebServer() {}

// WebServer::WebServer(const WebServer &src) {}

// WebServer &WebServer::operator=(const WebServer &other) {}
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
		std::cout << (*it) << std::endl;
	}
}

std::vector<Server> WebServer::getServers() const
{
	return _servers;
}


// Helper function: Set socket to non-blocking mode
static int make_nonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Helper function: Cleanup proxy connection
static void cleanup_proxy(ProxyConn *pc, std::map<int, ProxyConn*> &proxies_by_upstream,
                          std::map<int, ProxyConn*> &proxies_by_client, int epoll_fd)
{
	if (pc->upstream_fd >= 0)
	{
		close(pc->upstream_fd);
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pc->upstream_fd, NULL);
	}
	proxies_by_upstream.erase(pc->upstream_fd);
	proxies_by_client.erase(pc->client_fd);
	delete pc;
}

// Helper function: Cleanup client connection
static void cleanup_client(int client_fd, std::map<int, client*> &clients, int epoll_fd)
{
	std::map<int, client*>::iterator it = clients.find(client_fd);
	if (it != clients.end())
	{
		proxyRequest += it->first + ": " + it->second + "\r\n";
	}
	proxyRequest += "\r\n" + req.getBody();

	std::cout << "Proxy Request:\n"
			  << proxyRequest << std::endl;

	// Send the request to proxy server
	ssize_t sent = send(proxySocket.getServerFd(), proxyRequest.c_str(), proxyRequest.size(), 0);
	if (sent < 0)
	{
		throw std::runtime_error("Unable to send request to proxy server");
	}

	std::cout << "Request sent successfully (" << sent << " bytes)" << std::endl;

	// Receive the response from proxy server
	char buffer[4096];
	// Initialize buffer to zero without memset
	for (size_t i = 0; i < sizeof(buffer); i++)
	{
		buffer[i] = 0;
	}

	ssize_t bytes_received = recv(proxySocket.getServerFd(), buffer, sizeof(buffer) - 1, 0);

	if (bytes_received < 0)
	{
		throw std::runtime_error("Error receiving response from proxy server");
	}

	if (bytes_received == 0)
	{
		return ("");
	}

	buffer[bytes_received] = '\0'; // Null-terminate the received data

	std::cout << "Received " << bytes_received << " bytes from proxy" << std::endl;

	return std::string(buffer);
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
		if (make_nonblock(fd) == -1)
		{
			perror("make_nonblock: server socket");
			return false;
		}

		epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			return false;
		}

		server_fds.insert(fd);
		fd_to_server_idx[fd] = i;
		std::cout << "Listening on port: http://localhost:" << _servers[i].getPort() << std::endl;
	}
	return true;
}

// Handle new client connections
void WebServer::handleNewConnection(int event_fd, size_t server_idx,
                                    std::map<int, client*> &clients, int epoll_fd)
{
	while (true)
	{
		sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(event_fd, (struct sockaddr*)&client_addr, &client_len);

		if (client_fd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break; // No more connections
			perror("accept");
			break;
		}

		if (make_nonblock(client_fd) == -1)
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
