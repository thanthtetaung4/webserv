/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/11/20 19:30:53 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../../include/WebServer.hpp"
#include "../../include/Request.hpp"
#include "../../include/Response.hpp"
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../../include/Validator.hpp"
#include "../../include/Cgi.hpp"

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

bool WebServer::isProxyPass(std::string urlPath, Server server)
{
	std::map<std::string, t_location>::const_iterator it = search_map_iterator(server.getLocation(), urlPath);

	if (it != server.getLocation().end())
	{
		return (!(it->second._proxy_pass.empty()));
	}
	return (false);
}

bool WebServer::isCGI(std::string urlPath, Server server)
{
	std::map<std::string, t_location>::const_iterator it = search_map_iterator(server.getLocation(), urlPath);

	if (it != server.getLocation().end())
	{
		return ((it->second._isCgi));
	}
	return (false);
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
const std::string WebServer::handleReverseProxy(const Request &req, const Server &server)
{
	std::cout << "Handling reverse proxy..." << std::endl;
	std::cout << search_map_iterator(server.getLocation(), req.getUrlPath())->second._proxy_pass << std::endl;

	t_proxyPass pp = parseProxyPass(search_map_iterator(server.getLocation(), req.getUrlPath())->second._proxy_pass);

	std::cout << "Proxying to " << pp.host << ":" << pp.port << pp.path << std::endl;

	// Create socket with the correct port
	Socket proxySocket(std::atol(pp.port.c_str()));
	proxySocket.openSock(); // Only create the socket, don't bind/listen

	std::cout << "Proxy socket: " << proxySocket.getServerFd() << std::endl;

	// Setup address structure for the proxy server
	struct sockaddr_in server_addr;
	// Initialize to zero without memset
	for (size_t i = 0; i < sizeof(server_addr); i++)
	{
		((char *)&server_addr)[i] = 0;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(std::atol(pp.port.c_str()));

	// Convert IP address from string to binary using inet_addr
	server_addr.sin_addr.s_addr = inet_addr(pp.host.c_str());
	if (server_addr.sin_addr.s_addr == INADDR_NONE)
	{
		throw std::runtime_error("Invalid proxy address");
	}

	// Connect to proxy server
	if (connect(proxySocket.getServerFd(), (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		throw std::runtime_error("Unable to connect to proxy server");
	}

	std::cout << "Connected successfully!" << std::endl;

	// Build the proxy request
	std::string proxyRequest = req.getMethodType() + " " + pp.path + " " + req.getHttpVersion() + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin();
		 it != req.getHeaders().end(); ++it)
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

std::map<std::string, t_location>::const_iterator getBestLocationMatch(const std::map<std::string, t_location> &locations,
																	   const std::string &url)
{
	std::map<std::string, t_location>::const_iterator best = locations.end();
	size_t bestLen = 0;

	for (std::map<std::string, t_location>::const_iterator it = locations.begin();
		 it != locations.end(); ++it)
	{
		if (url.find(it->first) == 0 && it->first.size() > bestLen)
		{
			best = it;
			bestLen = it->first.size();
		}
	}
	return best;
}

const std::string WebServer::handleRedirect(std::string redirUrlPath)
{
	std::string res;

	res = "HTTP/1.1 302 Found\r\n";
	res += "Location: " + redirUrlPath + "\r\n";
	res += "Content-Type: text/html\r\n";
	res += "Content-Length: 0\r\n";
	res += "\r\n";

	return res;
}

const std::string WebServer::handleAutoIndex(const Request &req, const Server &server)
{
	std::string fullPath;

	if (req.getUrlPath()[req.getUrlPath().length() - 1] != '/')
	{
		return this->handleRedirect(req.getUrlPath() + "/");
	}

	// 1. Resolve full path from location or server root
	t_location *loc = searchMapLongestMatch(server.getLocation(), req.getUrlPath());

	if (loc)
	{
		if (!loc->_root.empty())
			fullPath = loc->_root + req.getUrlPath();
		else if (!server.getRoot().empty())
			fullPath = server.getRoot() + req.getUrlPath();
	}

	std::cout << "[AUTOINDEX] fullPath: " << fullPath << std::endl;

	/*
		here check the requested path is a file or a folder
		if folder => show auto index page
		if file => serve the file
	*/

	// if (open(fullPath.c_str(), O_RDONLY) >= 0) {
	// 	/*
	// 		build response from the path and return
	// 	*/
	// }
	// else {

	// Check access
	if (access(fullPath.c_str(), R_OK) == -1)
	{
		return "HTTP/1.1 403 Forbidden\r\n\r\n";
	}

	// Try opening directory
	DIR *dir = opendir(fullPath.c_str());
	if (!dir)
	{
		return "HTTP/1.1 404 Not Found\r\n\r\n";
	}

	// 2. Build HTML body
	std::string body;
	body += "<html><head><title>Index of " + req.getUrlPath() + "</title></head><body>";
	body += "<h1>Index of " + req.getUrlPath() + "</h1><hr><pre>";

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;

		if (name == "." || name == "..")
			continue;

		std::string itemFullPath = fullPath + "/" + name;

		struct stat st;
		if (stat(itemFullPath.c_str(), &st) == -1)
			continue;

		body += "<a href=\"" + req.getUrlPath();

		// Ensure trailing slash for directory
		if (*(req.getUrlPath().end()) != '/')
			body += "/";

		body += name;

		if (S_ISDIR(st.st_mode))
			body += "/";

		body += "\">" + name + "</a>\n";
	}

	body += "</pre><hr></body></html>";

	closedir(dir);

	// 3. Build Raw HTTP Response
	std::string response;
	response += "HTTP/1.1 200 OK\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + intToString(body.size()) + "\r\n";
	response += "\r\n"; // end of header
	response += body;	// body

	return response;
	// }
	// return "";
}

int WebServer::serve(void)
{
	int epoll_fd = epoll_create(1);
	if (epoll_fd == -1)
	{
		perror("epoll_create");
		return 1;
	}
	// Register all server sockets with epoll
	for (size_t i = 0; i < _sockets.size(); ++i)
	{

		int fd = _sockets[i].getServerFd();
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLRDHUP; 
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
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 5000);
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
			int flags = fcntl(client_fd, F_GETFL, 0);
			if (flags == -1) {
				perror("fcntl F_GETFL");
				close(client_fd);
				continue;
			}
			if(fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1){
				perror("fcntl");
				close(client_fd);
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
			std::cout << "================================= REQUEST PLAIN =====================" << std::endl;
			std::cout << buffer << std::endl;
			std::cout << "================================= REQUEST PLAIN END =====================" << std::endl;

			Request req(buffer);
			std::cout << "================================= SEVER TEST =====================" << std::endl;
			std::cout << _servers[idx] << std::endl;
			std::cout << "================================= SERVER TEST END =====================" << std::endl;

			// int i = req.validateAgainstConfig(_servers[idx]);
			// if(i != 200) {
			// 		Response res(i);
			// }

			std::cout << this->isProxyPass(req.getUrlPath(), _servers[idx]) << std::endl;
			if (this->isProxyPass(req.getUrlPath(), _servers[idx]))
			{
				std::cout << "POST method detected" << std::endl;
				std::string rawRes = this->handleReverseProxy(req, _servers[idx]);
				// just send the plain text to the client no need to change it back to Response obj
				std::cout << "================================= SERVER TEST START =====================" << std::endl;
				std::cout << rawRes << std::endl;
				std::cout << "================================= SERVER TEST END =====================" << std::endl;
				ssize_t sent = send(client_fd, rawRes.c_str(), rawRes.size(), 0);
				if (sent < 0)
				{
					perror("send");
				}
				close(client_fd);
				continue;
			} else if (req.isAutoIndex(_servers[idx])) {
				std::cout << "auto index" << std::endl;
				std::string	rawRes = handleAutoIndex(req, _servers[idx]);
				ssize_t sent = send(client_fd, rawRes.c_str(), rawRes.size(), 0);
				if (sent < 0)
				{
					perror("send");
				}
				close(client_fd);
				continue;
			}
			if (isCGI(req.getUrlPath(), _servers[idx]))
			{
				Cgi cgi(req, _servers[idx]);
			}
			std::cout << req << std::endl;
			std::cout << "================================= RESPONSE =====================" << std::endl;

			std::cout << "creating res" << std::endl;
			Response res(req, _servers[idx]);
			std::cout << "printing res" << std::endl;
			std::cout << res << std::endl;
			std::cout << "res printed" << std::endl;
			std::cout << "================================= RESPONSE END =====================" << std::endl;
			std::string httpResponse = res.toStr();

			std::cout << "================================= FINAL RESPONSE =====================" << std::endl;
			std::cout << "http res: " << httpResponse << std::endl;
			std::cout << "================================= FINAL RESPONSE END =====================" << std::endl;

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
