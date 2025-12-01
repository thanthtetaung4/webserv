/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/01 18:00:20 by lshein           ###   ########.fr       */
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


int make_nonblock(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
		return  -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
int WebServer::serve(void)
{
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        perror("epoll_create");
        return 1;
    }
    std::map<int, client*> clients;
    std::set<int> server_fds;
    std::map<int, size_t> fd_to_server_idx;

    for (size_t i = 0; i < _sockets.size(); ++i)
    {
        int fd = _sockets[i].getServerFd();
        if (make_nonblock(fd) == -1)
        {
            perror("make_nonblock: server socket");
            close(epoll_fd);
            return 1;
        }

        epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
        {
            perror("epoll_ctl: listen_sock");
            close(epoll_fd);
            return 1;
        }
        server_fds.insert(fd);
        fd_to_server_idx[fd] = i;
        std::cout << "Listening on port: http://localhost:" << _servers[i].getPort() << std::endl;
    }

    struct epoll_event events[MAX_EVENTS];
    while (true)
    {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        if (nfds < 0)
        {
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;
        }

        for (int n = 0; n < nfds; ++n)
        {
            int event_fd = events[n].data.fd;

            // If event is on a listening socket, accept loop
            if (server_fds.find(event_fd) != server_fds.end())
            {
                size_t server_idx = fd_to_server_idx[event_fd];
                while (true)
                {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(event_fd, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // no more connections
                        perror("accept");
                        break;
                    }

                    if (make_nonblock(client_fd) == -1)
                    {
                        perror("make_nonblock: client socket");
                        close(client_fd);
                        continue;
                    }

                    epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    client_ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1)
                    {
                        perror("epoll_ctl: add client");
                        close(client_fd);
                        continue;
                    }

                    clients[client_fd] = new client(client_fd, server_idx);
                    std::cout << "Client Connected : " << client_fd << std::endl;
                }
                continue;
            }

            // Handle client socket
            std::map<int, client*>::iterator it = clients.find(event_fd);
            if (it == clients.end())
            {
                // Not a tracked client, remove
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                continue;
            }
            client* c = it->second;
            c->update_activity();
            size_t server_idx = c->server_idx;

            // Handle errors/hangups
            if (events[n].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                std::cout << "Client disconnected: " << event_fd << std::endl;
                close(event_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                delete c;
                clients.erase(it);
                continue;
            }

            if (events[n].events & EPOLLIN)
            {
                // Read loop for edge-triggered
                while (true)
                {
                    char buffer[8192];
                    ssize_t bytes = recv(event_fd, buffer, sizeof(buffer), 0);
                    if (bytes > 0)
                    {
                        c->read_buff.append(buffer, bytes);
                    }
                    else if (bytes == 0)
                    {
                        // client closed
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        break;
                    }
                    else
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // no more data
                        perror("recv");
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        break;
                    }
                }

                // If we have data and no pending write, build response
                if (!c->read_buff.empty() && c->write_buff.empty())
                {
                    try {
                        Request req(c->read_buff.c_str(), _servers[server_idx]);
                        Response res(req, _servers[server_idx]);
                        c->write_buff = res.toStr();
                        c->read_buff.clear();

                        epoll_event mod_ev;
                        mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                        mod_ev.data.fd = event_fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                    }
                    catch (const std::exception &e) {
                        std::cerr << "Request/Response error: " << e.what() << std::endl;
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        continue;
                    }
                }
            }

            if ((events[n].events & EPOLLOUT) && c->has_write_data())
            {
                while (!c->write_buff.empty())
                {
                    ssize_t bytes_sent = send(event_fd, c->write_buff.c_str(), c->write_buff.size(), MSG_NOSIGNAL);
                    if (bytes_sent > 0)
                        c->write_buff.erase(0, bytes_sent);
                    else if (bytes_sent < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        perror("send");
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        break;
                    }
                }

                if (!c->has_write_data())
                {
                    epoll_event mod_ev;
                    mod_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    mod_ev.data.fd = event_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);

                    std::cout << "Response sent, closing: fd=" << event_fd << std::endl;
                    close(event_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                    delete c;
                    clients.erase(it);
                }
            }
        }
        // Timeout cleanup
        for (std::map<int, client*>::iterator it = clients.begin(); it != clients.end(); )
        {
            client *c = it->second;
            if (c->is_timeout(TIMEOUT))
            {
                int fd = c->fd;
                close(fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                delete c;
                std::map<int, client*>::iterator temp = it;
                ++it;
                clients.erase(temp);
            }
            else ++it;
        }
    }
    close(epoll_fd);
    return 0;
}
