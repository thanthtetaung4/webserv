/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:33:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/03 12:33:36 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

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
#include "Response.hpp"
#include "proxyPass.h"
#include "Validator.hpp"
#include <sys/time.h>
#include <set>
#include <map>

#define MAX_EVENTS 10

// Forward declarations
struct ProxyConn;
class client;

class WebServer
{
private:
	std::vector<Server> _servers;
	std::vector<Socket> _sockets;
	
	// Private helper methods for serve()
	bool setupServerSockets(int epoll_fd, std::set<int> &server_fds, 
	                        std::map<int, size_t> &fd_to_server_idx);
	void handleNewConnection(int event_fd, size_t server_idx,
	                        std::map<int, client*> &clients, int epoll_fd);
	void handleClientEvent(const struct epoll_event &event,
	                      std::map<int, client*> &clients,
	                      std::map<int, ProxyConn*> &proxies_by_upstream,
	                      std::map<int, ProxyConn*> &proxies_by_client,
	                      int epoll_fd);
	void handleProxyEvent(const struct epoll_event &event, ProxyConn *pc,
	                     std::map<int, client*> &clients,
	                     std::map<int, ProxyConn*> &proxies_by_upstream,
	                     std::map<int, ProxyConn*> &proxies_by_client,
	                     int epoll_fd);
	void cleanupTimedOutClients(std::map<int, client*> &clients, int epoll_fd);
	bool setupProxyConnection(const Request &req, client *c, int client_fd,
	                         size_t server_idx,
	                         std::map<int, ProxyConn*> &proxies_by_upstream,
	                         std::map<int, ProxyConn*> &proxies_by_client,
	                         int epoll_fd);

public:
	WebServer();
	~WebServer();
	// WebServer(const WebServer &src);
	// WebServer &operator=(const WebServer &other);

	void setServer(std::string configFile);
	void addServer(Server server);
	void setUpSock(void);
	int serve(void);
	std::vector<Server> getServers() const;
	const std::string handleReverseProxy(const Request &req, const Server &server);
	const std::string handleAutoIndex(const Request &req, const Server &server);
	bool isProxyPass(std::string urlPath, Server server);
	bool isCGI(std::string urlPath, Server server);
	const std::string handleRedirect(std::string redirUrlPath);
};

#endif
