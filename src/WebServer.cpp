/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/10/11 04:51:32 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../include/WebServer.hpp"

WebServer::WebServer(){}

WebServer::~WebServer(){}

// WebServer::WebServer(const WebServer &src) {}

// WebServer &WebServer::operator=(const WebServer &other) {}

std::string	WebServer::getIndex(std::string path) {
	std::cout << path << std::endl;
	std::ifstream	file(path.c_str());

	if (file.good()) {
		std::stringstream buffer;
		buffer << file.rdbuf();

		std::cout << buffer.str() << std::endl;

		return buffer.str();
	} else {
		std::cout << "file not opened" << std::endl;
	}
	return "";
}

void	printRange(std::string::iterator it1, std::string::iterator it2)
{
	if (it1 == it2) {
		std::cout << "(empty range)" << std::endl;
		return;
	}

	for (std::string::iterator it = it1; it != it2; ++it) {
		std::cout << *it;
	}
	std::cout << std::endl;
}

t_its getIts(std::string &content, std::string::iterator start, const std::string &target1, const std::string &target2)
{
	t_its it;

	it.it1 = std::search(start, content.end(), target1.begin(), target1.end());
	if (it.it1 == content.end())
		throw "Invalid config file";
	else
		it.it2 = std::search(it.it1 + 1, content.end(), target2.begin(), target2.end());
	return it;
}

void getServerBlock(t_its it, std::vector<Server> &servers)
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
	servers.push_back(server);
	std::cout << server;
}

void getLocationBlock(t_its it, Server &server)
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
	server.setLocation(key, location);
}

void setAttributes(const std::vector<std::string>& line, Server& server)
{
	if (line.empty()) return;
	if (line[0] == "listen" && line.size() == 2)
		server.setPort(line[1]);
	else if (line[0] == "server_name" && line.size() == 2)
		server.setServerName(line[1]);
	else if (line[0] == "error_page" && line.size() >= 3)
		server.setErrorPage(line[1], line[2]);
	else if (line[0] == "client_max_body_size" && line.size() == 2)
		server.setMaxBytes(line[1]);
}

void setLocationAttributes(const std::vector<std::string> &line, t_location &location, std::string &key)
{
	if (line.empty())
		return;
	if (line[0] == "location")
		key = line[1];
	else if (line[0] == "root")
		location._root = line[1];
	else if (line[0] == "index")
		location._index = line[1];
	else if (line[0] == "limit_except")
	{
		for (size_t i = 1; i < line.size(); i++)
			location._limit_except.push_back(line[i]);
	}
	else if (line[0] == "return")
		location._return[line[1]] = line[2];
	else if (line[0] == "autoindex")
		location._autoIndex = line[1];
	else if (line[0] == "cgiPass")
		location._cgiPass = line[1];
	else if (line[0] == "cgiExt")
		location._cgiExt = line[1];
	else if (line[0] == "uploadStore")
		location._uploadStore = line[1];
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
			getServerBlock(it, _servers);
			if (it.it2 != content.end())
				it = getIts(content, it.it2, target, target);
			else
				break;
		}
		config.close();
	}
}

void	WebServer::addServer(Server server) {
	this->_servers.push_back(server);
	this->_sockets.push_back(Socket(std::atol(server.getPort().c_str())));
}

void	WebServer::setUpSock(void) {
	std::vector<Socket>::iterator it;

	for (it = this->_sockets.begin(); it != this->_sockets.end(); ++it) {
		(*it).prepSock();
		std::cout << (*it) << std::endl;
	}
}


int	WebServer::serve(void) {
		int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		perror("epoll_create1");
		return 1;
	}

	// Register all server sockets with epoll
	for (size_t i = 0; i < _sockets.size(); ++i) {
		int fd = _sockets[i].getServerFd();
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.u32 = i; // Store index to map back to Server
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
			perror("epoll_ctl: listen_sock");
			close(epoll_fd);
			return 1;
		}
		std::cout << "Listening on port: http://localhost:" << _servers[i].getPort() << std::endl;
	}

	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			break;
		}
		for (int n = 0; n < nfds; ++n) {
			size_t idx = events[n].data.u32;
			int listen_fd = _sockets[idx].getServerFd();
			sockaddr_in client_addr;
			socklen_t client_len = sizeof(client_addr);
			int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
			if (client_fd < 0) {
				perror("accept");
				continue;
			}
			            // --- RECV REQUEST ---
			char buffer[4096];
			ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
			if (bytes_received < 0) {
				perror("recv");
				close(client_fd);
				continue;
			}
			buffer[bytes_received] = '\0';
			std::cout << "Request received on port " << _servers[idx].getPort() << ":\n";
			std::cout << buffer << std::endl;

			// --- SEND RESPONSE ---
			std::string	body = this->getIndex("www/index.html");

			std::cout << "==================================" << std::endl;
			std::cout << body << std::endl;
			std::cout << "==================================" << std::endl;

			std::string	httpResponse =
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/html\r\n"
				// "Content-Length: 13\r\n"
				"Connection: close\r\n"
				"\r\n"
				+ body;

			std::cout << "http res: " << httpResponse << std::endl;
			ssize_t sent = send(client_fd, httpResponse.c_str(), httpResponse.size(), 0);
			if (sent < 0) {
				perror("send");
			}
			close(client_fd);
		}
	}
	close(epoll_fd);
	return 0;
}
