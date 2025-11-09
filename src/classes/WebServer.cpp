/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/11/09 14:57:26 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../../include/WebServer.hpp"
#include "../../include/Request.hpp"
#include "../../include/Response.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

WebServer::WebServer() {}

WebServer::~WebServer() {}

// WebServer::WebServer(const WebServer &src) {}

// WebServer &WebServer::operator=(const WebServer &other) {}

std::string WebServer::getIndex(std::string path)
{
	std::cout << path << std::endl;
	std::ifstream file(path.c_str());

	if (file.good())
	{
		std::stringstream buffer;
		buffer << file.rdbuf();

		std::cout << buffer.str() << std::endl;

		return buffer.str();
	}
	else
	{
		std::cout << "file not opened" << std::endl;
	}
	return "";
}

void printRange(std::string::iterator it1, std::string::iterator it2)
{
	if (it1 == it2)
	{
		std::cout << "(empty range)" << std::endl;
		return;
	}

	for (std::string::iterator it = it1; it != it2; ++it)
	{
		std::cout << *it;
	}
	std::cout << std::endl;
}

t_its WebServer::getIts(std::string &content, std::string::iterator start, const std::string &target1, const std::string &target2)
{
	t_its it;

	it.it1 = std::search(start, content.end(), target1.begin(), target1.end());
	if (it.it1 == content.end())
		throw "Invalid config file";
	else
		it.it2 = std::search(it.it1 + 1, content.end(), target2.begin(), target2.end());
	return it;
}

void WebServer::getServerBlock(t_its it)
{
	std::string content(it.it1, it.it2);
	std::stringstream serverString(content);
	std::string line;
	Server server;
	t_its itLoc;

	std::string::iterator pos = content.begin();
	while (std::getline(serverString, line))
	{
		std::stringstream ss(line);
		std::string token;
		std::vector<std::string> lineVec;
		while (ss >> token)
		{
			if (!token.empty() && token == "location")
			{
				itLoc = getIts(content, pos, token, "}");
				getLocationBlock(itLoc, server);
				pos = itLoc.it2;
				std::string remaining(pos, content.end());
				serverString.str(remaining);
				serverString.clear();
				continue;
			}
			if (!token.empty() && !(token.at(token.size() - 1) == ';'))
				lineVec.push_back(token);
			else if (!token.empty() && (token.at(token.size() - 1) == ';'))
			{
				token = token.substr(0, token.size() - 1);
				lineVec.push_back(token);
				setAttributes(lineVec, server);
				lineVec.clear();
			}
		}
	}
	addServer(server);
	// std::cout << server;
}

void WebServer::getLocationBlock(t_its it, Server &server)
{
	std::string content(it.it1, it.it2);
	std::stringstream locationString(content);
	std::string line;
	t_location location;
	std::string key;

	while (std::getline(locationString, line))
	{
		std::stringstream ss(line);
		std::string token;
		std::vector<std::string> lineVec;

		while (ss >> token)
		{
			if (!token.empty() && !(token.at(token.size() - 1) == ';') && !(token == "{"))
				lineVec.push_back(token);
			else if (!token.empty() && (token.at(token.size() - 1) == ';'))
			{
				token = token.substr(0, token.size() - 1);
				lineVec.push_back(token);
				setLocationAttributes(lineVec, location, key);
				lineVec.clear();
			}
			else if (!token.empty() && token == "{")
			{
				lineVec.push_back(token);
				setLocationAttributes(lineVec, location, key);
				lineVec.clear();
			}
		}
	}
	if (location._limit_except.empty())
		location._limit_except.push_back("GET");
	server.setLocation(key, location);
}

void WebServer::setAttributes(const std::vector<std::string> &line, Server &server)
{
	if (line.empty())
		return;

	const std::string &key = line[0];
	if (key == "listen")
	{
		Validator::requireSize(line, 2, key);
		server.setPort(line[1]);
	}
	else if (key == "server_name")
	{
		Validator::requireSize(line, 2, key);
		server.setServerName(line[1]);
	}
	else if (key == "error_page")
	{
		Validator::requireMinSize(line, 3, key);
		const std::string &errorPage = line.back();
		for (size_t i = 1; i < line.size() - 1; ++i)
			server.setErrorPage(line[i], errorPage);
	}
	else if (key == "client_max_body_size")
	{
		Validator::requireSize(line, 2, key);
		server.setMaxBytes(line[1]);
	}
	else if (key == "return")
	{
		Validator::requireSize(line, 3, key);
		server.setReturn(line[1], line[2]);
	}
	else
		throw std::runtime_error("Unknown directive: '" + key + "'");
}

void WebServer::setLocationAttributes(const std::vector<std::string> &line, t_location &location, std::string &key)
{
	if (line.empty())
		return;

	const std::string &directive = line[0];

	if (directive == "location")
	{
		Validator::requireSize(line, 3, directive);
		key = line[1];
	}
	else if (directive == "root")
	{
		Validator::requireSize(line, 2, directive);
		location._root = line[1];
	}
	else if (directive == "index")
	{
		Validator::requireMinSize(line, 2, directive);
		for (size_t i = 1; i < line.size(); ++i)
			location._index.push_back(line[i]);
	}
	else if (directive == "limit_except")
	{
		Validator::requireMinSize(line, 1, directive);
		for (size_t i = 1; i < line.size(); ++i)
			location._limit_except.push_back(line[i]);
	}
	else if (directive == "return")
	{
		Validator::requireSize(line, 3, directive);
		location._return[line[1]] = line[2];
	}
	else if (directive == "autoindex")
	{
		Validator::requireSize(line, 2, directive);
		if (line[1] != "on" && line[1] != "off")
			throw std::runtime_error("Invalid 'autoindex' value — expected 'on' or 'off'");
		location._autoIndex = line[1];
	}
	else if (directive == "cgiPass")
	{
		Validator::requireSize(line, 2, directive);
		location._cgiPass = line[1];
	}
	else if (directive == "cgiExt")
	{
		Validator::requireSize(line, 2, directive);
		location._cgiExt = line[1];
	}
	else if (directive == "uploadStore")
	{
		Validator::requireSize(line, 2, directive);
		location._uploadStore = line[1];
	}
	else if (directive == "proxy_pass")
	{
		Validator::requireSize(line, 2, directive);
		location._proxy_pass = line[1];
	}
	else
		throw std::runtime_error("Unknown location directive: '" + directive + "'");
}

void WebServer::setServer(std::string configFile)
{
	t_its it;
	std::string target = "server {";

	std::ifstream config(configFile.c_str());
	if (!config)
		throw "Unable to open file!!";
	else
	{
		std::stringstream ss;
		ss << config.rdbuf();
		std::string content = ss.str();
		it = getIts(content, content.begin(), target, target);
		while (it.it1 != content.end())
		{
			getServerBlock(it);
			if (it.it2 != content.end())
				it = getIts(content, it.it2, target, target);
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
/*
	send the request to the proxy server and get the response
	create a new Response object with the response from server
	and return it
*/
const Response &WebServer::handleReverseProxy()
{
	Response *res = new Response();
	return *res;
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
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
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
			std::cout << "=================REQUEST============================" << std::endl;

			ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
			if (bytes_received < 0)
			{
				perror("recv");
				close(client_fd);
				continue;
			}
			buffer[bytes_received] = '\0';
			std::cout << "Request received on port " << _servers[idx].getPort() << ":\n";
			std::cout << "=======================================================" << std::endl;
			std::cout << buffer << std::endl;
			std::cout << "=====================================================" << std::endl;
			Request req(buffer);

			if (req._method == "POST")
			{
				std::cout << "POST method detected" << std::endl;
				Response res = this->handleReverseProxy();
			}

			std::cout << req << std::endl;
			std::cout << "=================================I do not know let see=====================" << std::endl;

			std::cout << "creating res" << std::endl;
			Response res(req, _servers[idx]);
			std::cout << "printing res" << std::endl;
			std::cout << res << std::endl;
			std::cout << "res printed" << std::endl;
			std::cout << "=================================I do not know let see=====================" << std::endl;
			std::string httpResponse = res.toStr();

			std::cout << "http res: " << httpResponse << std::endl;

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
