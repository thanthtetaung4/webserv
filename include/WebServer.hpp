/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:33:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/30 04:12:39 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

// Forward declaration to break circular dependency
class Client;

#include "Server.hpp"
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

// enum UpStreamState {

// }

class WebServer
{
private:
	std::vector<Server> _servers;
	std::vector<Socket> _sockets;
	std::map<int, Client*> _clients;
	std::vector<int> _upstreamFds;
	std::map<int, Client*> _upstreamClient;
	std::map<int, Client*> _cgiClients;  // Map CGI output fd to client
	int _epoll_fd;

	int	searchVecIndex(std::vector<int> vec, int key);
	int	searchSocketIndex(std::vector<Socket> vec, int key);
	void	readFromClient(Client& client);
	void	updateClient(Client& client);

public:
	WebServer();
	~WebServer();

	void setServer(std::string configFile);
	void addServer(Server server);
	void setUpSock(void);
	std::vector<Server> getServers() const;
	int run(void);

	void handleAccept(int listenfd);
	void handleRead(int fd);
	void handleWrite(int fd);
	void closeClient(int fd);
	void handleUpstreamWrite(Client& c, int fd);
	void handleUpstreamRead(Client& c, int fd);
	void handleUpstreamEvent(int fd, uint32_t events);
	void closeUpstream(int upstreamFd);
	void finalizeUpstreamResponse(Client& client);
	void handleCgiRead(int cgiFd);
	void finalizeCgiResponse(Client& client);
	void checkCgiTimeouts();

	bool isUpStream(int fd) const;
	bool isListenFd(int fd) const;

	// Accessors
	std::map<int, Client*>&	getClients();
	std::map<int, Client*>&	getUpstreamClients();
	std::vector<Socket>&		getSockets();

	// Utils
	Client*	searchClients(int fd);
	Client*	searchClientsUpstream(int fd);
	Client*	searchClientsByCgi(int cgiFd);
	void	removeUpstreamFd(int fd);
};

// Include Client after WebServer definition to avoid circular dependency
#include "Client.hpp"

#endif
