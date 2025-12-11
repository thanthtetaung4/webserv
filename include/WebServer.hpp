/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:33:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/09 01:46:54 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Server.hpp"
#include "Client.hpp"
#include "ServerException.hpp"
#include <fcntl.h>
#include "Socket.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <poll.h>
#include <sstream>
#include <string>
#include <dirent.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>
#include "Request.hpp"
#include "Response.hpp"
#include "proxyPass.h"
#include "Validator.hpp"
#include <sys/time.h>
#include <set>
#include <map>
#define MAX_EVENTS 10

class client;

class WebServer
{
private:
	std::vector<Server> _servers;
	std::vector<Socket> _sockets;
	std::map<int, Client*> _clients;
	std::map<int, int> _upstreamToClient;  // upstreamFd -> clientFd mapping
	int _epoll_fd;
	int findServerIndex(int listenfd);

public:
	WebServer();
	~WebServer();

	void setServer(std::string configFile);
	void addServer(Server server);
	void setUpSock(void);
	int serve(void);
	std::vector<Server> getServers() const;
	int run(void);

	void handleAccept(int listenfd);
	void handleRead(int fd);
	void handleWrite(int fd);
	void handleUpstreamRead(int fd);
	void handleUpstreamEvent(int fd, uint32_t events);
	void closeClient(int fd);
	void closeUpstream(int upstreamFd);

	bool isListenFd(int fd) const;
	int startUpstreamConnection(int clientFd, const std::string& host, const std::string& port);
};

#endif
