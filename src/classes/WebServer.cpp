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


/*
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
			// std::cout << "=================REQUEST============================" << std::endl;

			ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
			if (bytes_received < 0)
			{
				perror("recv");
				close(client_fd);
				continue;
			}
			buffer[bytes_received] = '\0';
			std::cout << "Request received on port " << _servers[idx].getPort() << ":\n";
			// std::cout << "================================= REQUEST PLAIN =====================" << std::endl;
			// std::cout << buffer << std::endl;
			// std::cout << "================================= REQUEST PLAIN END =====================" << std::endl;

			Request req(buffer, _servers[idx]);
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
}*/
# define TIMEOUT 1000
# include "../../include/Client.hpp"

int make_nonblock(int fd){
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
		return  -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
/*
int WebServer::serve(){
	int epoll_fd =  epoll_create(1);
	if(epoll_fd == -1){
		perror("epoll_create");
		return 1;
	}
	std::map<int, client*> clients;
	for(size_t i = 0; i < _sockets.size(); ++i){
		int fd = _sockets[i].getServerFd();
		make_nonblock(fd);

		epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.u32 = i;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
			perror("epoll_ctl = listen_sock");
			close(epoll_fd);
			return 1;
		}
		std::cout << "Listening on port https://localhost:" << _servers[i].getPort() << std::endl;
	}
	epoll_event events[MAX_EVENTS];
	while (true) {
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
		if(nfds < 0){
			perror("epoll wait");
			break;
		}
		for(int n  = 0; n < nfds; ++n){
			int idx = events[n].data.u32;

			if(idx < (int)_sockets.size()){
				int listen_fd =	_sockets[idx].getServerFd();
				while (true) {
					int client_fd = accept(listen_fd, NULL, NULL);
					if(client_fd < 0){
						if(errno == EAGAIN || errno == EWOULDBLOCK)
							break;
						perror("accept");
						break;
					}
					make_nonblock(client_fd);

					epoll_event client_ev;
					client_ev.events = EPOLLIN;
					client_ev.data.fd = client_fd;
					if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1){
						perror("epoll_ctl : add client");
						close(client_fd);
						continue;
					}

					clients[client_fd] = new client(client_fd, idx);
					std::cout << "Client Connected : " << client_fd << std::endl;
				}
			}
			else {
				int client_fd = events[n].data.fd;
				std::map<int, client*>::iterator it = clients.find(client_fd);
				if(it == clients.end())
					continue;
				client* c =it->second;
				c->update_activity();
				size_t server_idx = c->server_idx;
				if(events[n].events & EPOLLIN){
					char buffer[40960];
					int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
					if( bytes < 0  )
					{
						if(errno == EAGAIN || errno == EWOULDBLOCK)
							break;
						perror("recv");
						close(client_fd);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
						delete c;
						clients.erase(it);
						std::cout << "Client disconnected: " << client_fd << std::endl;
						continue;
					}
					else if(bytes == 0){
						close(client_fd);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
						delete c;
						clients.erase(it);
						break;
					}
					buffer[bytes] = '\0';
					c->read_buff.append(buffer, bytes);
					
					Request req(c->read_buff.c_str());
					Response res(req, _servers[server_idx]);
					std::string httpResponse =  res.toStr();
					c->write_buff = httpResponse;
					epoll_event mod_ev;
					mod_ev.events = EPOLLIN | EPOLLOUT;
					mod_ev.data.fd =  client_fd;			
					epoll_ctl(epoll_fd, EPOLL_CTL_MOD,client_fd,&mod_ev);
				}
				if((events[n].events & EPOLLOUT) && c->has_write_data()) {
					int byte_send = send(client_fd, c->write_buff.c_str(), c->write_buff.size(), 0);
					if(byte_send > 0){
						c->write_buff.erase(0, byte_send);
					}
					if(!c->has_write_data()) {
						epoll_event mod_ev;
						mod_ev.events = EPOLLIN;
						mod_ev.data.fd =  client_fd;
						epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &mod_ev);
					}
				}
			}
		}
		std::map<int, client*>::iterator it;
		for(it = clients.begin(); it != clients.end();) {
			client *c = it->second;
			if(c->is_timeout(TIMEOUT)) {
				close(c->fd);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->fd, NULL);
				delete c;
				std::cout << "Client Timeout : " << c->fd << std::endl;
				std::map<int, client*>::iterator temp = it;
				++it;
				clients.erase(temp);
			}
			else {
				++it;
			}
		}
	}
	close(epoll_fd);
	return 0;
}
*/
#include <set>

int WebServer::serve()
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        return 1;
    }

    std::map<int, client*> clients;
    std::map<int, size_t> fd_to_server_idx;

    // Register all server sockets with epoll
    for (size_t i = 0; i < _sockets.size(); ++i)
    {
        int fd = _sockets[i].getServerFd();
        if (make_nonblock(fd) == -1)
        {
            perror("make_nonblock: server socket");
            close(epoll_fd);
            return 1;
        }

        fd_to_server_idx[fd] = i;
        
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLET; // Edge-triggered for better performance
        ev.data.fd = fd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
        {
            perror("epoll_ctl: listen_sock");
            close(epoll_fd);
            return 1;
        }
        std::cout << "Listening on port " << _servers[i].getPort() 
                  << " (fd=" << fd << ")" << std::endl;
    }

    epoll_event events[MAX_EVENTS];

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

        // Process all events
        for (int n = 0; n < nfds; ++n)
        {
            int event_fd = events[n].data.fd;

            // Check if it's a server socket
            std::map<int, size_t>::iterator server_it = fd_to_server_idx.find(event_fd);
            if (server_it != fd_to_server_idx.end())
            {
                // Accept all pending connections
                size_t server_idx = server_it->second;
                while (true)
                {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(event_fd, (struct sockaddr*)&client_addr, &client_len);
                    
                    if (client_fd < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // All connections accepted
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
                    client_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // Edge-triggered + hangup detection
                    client_ev.data.fd = client_fd;
                    
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1)
                    {
                        perror("epoll_ctl: add client");
                        close(client_fd);
                        continue;
                    }

                    clients[client_fd] = new client(client_fd, server_idx);
                    std::cout << "Client connected: fd=" << client_fd 
                              << " on port " << _servers[server_idx].getPort() << std::endl;
                }
                continue;
            }

            // Handle client socket
            std::map<int, client*>::iterator it = clients.find(event_fd);
            if (it == clients.end())
            {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                continue;
            }

            client* c = it->second;
            c->update_activity();
            size_t server_idx = c->server_idx;

            // Handle errors, hangups, or peer closed connection
            if (events[n].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                std::cout << "Client disconnected: fd=" << event_fd << " (error/hangup)" << std::endl;
                close(event_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                delete c;
                clients.erase(it);
                continue;
            }

            // Handle incoming data
            if (events[n].events & EPOLLIN)
            {
                // Read all available data (edge-triggered mode)
                while (true)
                {
                    char buffer[8192];
                    ssize_t bytes = recv(event_fd, buffer, sizeof(buffer) - 1, 0);

                    if (bytes > 0)
                    {
                        buffer[bytes] = '\0';
                        c->read_buff.append(buffer, bytes);
                    }
                    else if (bytes == 0)
                    {
                        // Connection closed by client
                        std::cout << "Client disconnected: fd=" << event_fd << " (closed)" << std::endl;
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        goto next_event;
                    }
                    else
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // All data read
                        
                        perror("recv");
                        std::cout << "Client disconnected: fd=" << event_fd << " (error)" << std::endl;
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        goto next_event;
                    }
                }

                // Check if we have a complete HTTP request
                if (!c->read_buff.empty() && c->write_buff.empty())
                {
                    try
                    {
                        Request req(c->read_buff, _servers[server_idx]);
                        Response res(req, _servers[server_idx]);
                        c->write_buff = res.toStr();
                        c->read_buff.clear();

                        // Modify event to include EPOLLOUT
                        epoll_event mod_ev;
                        mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                        mod_ev.data.fd = event_fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Request/Response error: " << e.what() << std::endl;
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        continue;
                    }
                }
            }

            // Handle outgoing data
            if ((events[n].events & EPOLLOUT) && c->has_write_data())
            {
                // Write all available data (edge-triggered mode)
                while (!c->write_buff.empty())
                {
                    ssize_t bytes_sent = send(event_fd, c->write_buff.c_str(), 
                                             c->write_buff.size(), MSG_NOSIGNAL);

                    if (bytes_sent > 0)
                    {
                        c->write_buff.erase(0, bytes_sent);
                    }
                    else if (bytes_sent < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // Socket buffer full, try again later
                        
                        perror("send");
                        std::cout << "Client disconnected: fd=" << event_fd << " (send error)" << std::endl;
                        close(event_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                        delete c;
                        clients.erase(it);
                        goto next_event;
                    }
                }

                // If all data sent, remove EPOLLOUT from events
                if (!c->has_write_data())
                {
                    epoll_event mod_ev;
                    mod_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    mod_ev.data.fd = event_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);

                    // Close connection after response sent (HTTP/1.0 style)
                    std::cout << "Response sent, closing: fd=" << event_fd << std::endl;
                    close(event_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                    delete c;
                    clients.erase(it);
                }
            }

            next_event:
            continue;
        }

        // Cleanup timed-out connections
        std::map<int, client*>::iterator it = clients.begin();
        while (it != clients.end())
        {
            client* c = it->second;
            if (c->is_timeout(TIMEOUT))
            {
                int fd = c->fd;
                std::cout << "Client timeout: fd=" << fd << std::endl;
                close(fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                delete c;
                std::map<int, client*>::iterator temp = it;
                ++it;
                clients.erase(temp);
            }
            else
            {
                ++it;
            }
        }
    }

    // Cleanup all remaining connections
    for (std::map<int, client*>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    close(epoll_fd);
    return 0;
}
