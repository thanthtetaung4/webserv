/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:33:13 by lshein            #+#    #+#             */
/*   Updated: 2025/11/20 19:03:44 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

# include "Server.hpp"
# include "ServerException.hpp"
# include "Socket.hpp"
# include <cstdlib>
# include <cstring>
# include <fstream>
# include <poll.h>
# include <sstream>
# include <string>
# include <dirent.h>
# include <sys/epoll.h>
# include <sys/stat.h>
# include <unistd.h>
# include "Request.hpp"
# include "Response.hpp"
# include "utils.h"
# include "proxyPass.h"
# include "Validator.hpp"

# define MAX_EVENTS 10

typedef struct its
{
	std::string::iterator it1;
	std::string::iterator it2;
} t_its;

class WebServer
{
private:
	std::vector<Server> _servers;
	std::vector<Socket> _sockets;
	// std::vector<pollfd> _pollFds;
	std::string getIndex(std::string path);

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
	const std::string	handleAutoIndex(const Request& req, const Server &server);
	bool isProxyPass(std::string urlPath, Server server);
	bool isCGI(std::string urlPath, Server server);
	const std::string	handleRedirect(std::string redirUrlPath);
};

#endif
