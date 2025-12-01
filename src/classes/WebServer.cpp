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

// Simple proxy connection state tracked by the main event loop
struct ProxyConn {
    int client_fd;                // original client fd
    int upstream_fd;              // fd connected to upstream
    std::string up_req_buf;       // data to send to upstream
    std::string up_resp_buf;      // data received from upstream (to forward to client)
    bool connecting;              // true if connect() is in progress
    size_t server_idx;            // which server config this belongs to
    ProxyConn(int cfd, int ufd, const std::string &req, bool conn, size_t idx)
        : client_fd(cfd), upstream_fd(ufd), up_req_buf(req), up_resp_buf(""), connecting(conn), server_idx(idx) {}
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
    std::map<int, ProxyConn*> proxies_by_upstream; // upstream_fd -> ProxyConn*
    std::map<int, ProxyConn*> proxies_by_client;   // client_fd -> ProxyConn*
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

            // If event is on an upstream proxy socket, handle proxy state
            std::map<int, ProxyConn*>::iterator pit_up = proxies_by_upstream.find(event_fd);
            if (pit_up != proxies_by_upstream.end())
            {
                ProxyConn *pc = pit_up->second;

                // Error/hangup handling for upstream
                if (events[n].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                {
                    // upstream closed or errored — forward any remaining data and close both sides
                    if (!pc->up_resp_buf.empty())
                    {
                        std::map<int, client*>::iterator cit = clients.find(pc->client_fd);
                        if (cit != clients.end())
                        {
                            client *cclient = cit->second;
                            cclient->write_buff.append(pc->up_resp_buf);
                            // request EPOLLOUT for client
                            epoll_event mod_ev;
                            mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                            mod_ev.data.fd = pc->client_fd;
                            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, pc->client_fd, &mod_ev);
                        }
                    }
                    close(pc->upstream_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pc->upstream_fd, NULL);
                    proxies_by_upstream.erase(pit_up);
                    proxies_by_client.erase(pc->client_fd);
                    delete pc;
                    continue;
                }

                // Writable: either connect completed or ready to send request
                if (events[n].events & EPOLLOUT)
                {
                    if (pc->connecting)
                    {
                        // Check connect result
                        int err = 0;
                        socklen_t len = sizeof(err);
                        if (getsockopt(pc->upstream_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
                        {
                            std::cerr << "Proxy upstream connect failed: " << strerror(err) << std::endl;
                            // close and cleanup
                            close(pc->upstream_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pc->upstream_fd, NULL);
                            proxies_by_upstream.erase(pit_up);
                            proxies_by_client.erase(pc->client_fd);
                            delete pc;
                            continue;
                        }
                        pc->connecting = false;
                    }

                    // Send pending request data to upstream
                    while (!pc->up_req_buf.empty())
                    {
                        ssize_t sent = send(pc->upstream_fd, pc->up_req_buf.c_str(), pc->up_req_buf.size(), 0);
                        if (sent > 0)
                        {
                            pc->up_req_buf.erase(0, sent);
                        }
                        else if (sent < 0)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break;
                            perror("send to upstream");
                            // error — close upstream and notify client
                            close(pc->upstream_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pc->upstream_fd, NULL);
                            proxies_by_upstream.erase(pit_up);
                            proxies_by_client.erase(pc->client_fd);
                            delete pc;
                            break;
                        }
                    }
                }

                // Read from upstream
                if (events[n].events & EPOLLIN)
                {
                    while (true)
                    {
                        char buf[8192];
                        ssize_t r = recv(pc->upstream_fd, buf, sizeof(buf), 0);
                        if (r > 0)
                        {
                            // forward immediately to client write buffer
                            std::map<int, client*>::iterator cit = clients.find(pc->client_fd);
                            if (cit != clients.end())
                            {
                                client *cclient = cit->second;
                                cclient->write_buff.append(buf, r);
                                epoll_event mod_ev;
                                mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                                mod_ev.data.fd = pc->client_fd;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, pc->client_fd, &mod_ev);
                            }
                        }
                        else if (r == 0)
                        {
                            // upstream closed — close upstream and remove mapping
                            close(pc->upstream_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pc->upstream_fd, NULL);
                            proxies_by_upstream.erase(pit_up);
                            proxies_by_client.erase(pc->client_fd);
                            delete pc;
                            break;
                        }
                        else
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break;
                            perror("recv from upstream");
                            close(pc->upstream_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pc->upstream_fd, NULL);
                            proxies_by_upstream.erase(pit_up);
                            proxies_by_client.erase(pc->client_fd);
                            delete pc;
                            break;
                        }
                    }
                }
                continue;
            }

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

                // If we have data and no pending write, build response or start a proxy
                if (!c->read_buff.empty() && c->write_buff.empty())
                {
                    try {
                        Request req(c->read_buff.c_str(), _servers[server_idx]);
                        // If this location config has a proxy_pass, start non-blocking proxying
                        std::map<std::string, t_location>::const_iterator locit = req.getIt();
                        if (locit != _servers[server_idx].getLocation().end() && !locit->second._proxy_pass.empty())
                        {
                            // parse proxy target
                            t_proxyPass pp = parseProxyPass(locit->second._proxy_pass);

                            // Build proxy request
                            std::string proxyRequest = req.getMethodType() + " " + pp.path + " " + req.getHttpVersion() + "\r\n";
                            proxyRequest += "Host: " + pp.host + "\r\n";
                            for (std::map<std::string, std::string>::const_iterator ih = req.getHeaders().begin(); ih != req.getHeaders().end(); ++ih)
                            {
                                if (ih->first == "Host")
                                    continue;
                                proxyRequest += ih->first + ": " + ih->second + "\r\n";
                            }
                            proxyRequest += "Connection: close\r\n";
                            proxyRequest += "\r\n" + req.getBody();

                            // Create upstream socket non-blocking
                            int upstream_fd = socket(AF_INET, SOCK_STREAM, 0);
                            if (upstream_fd < 0)
                            {
                                std::cerr << "Failed to create upstream socket" << std::endl;
                                // send 502 to client
                                std::string body = "<h1>502 Bad Gateway - Socket creation failed</h1>";
                                std::ostringstream h;
                                h << "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
                                c->write_buff = h.str();
                                c->read_buff.clear();
                                epoll_event mod_ev;
                                mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                                mod_ev.data.fd = event_fd;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                                continue;
                            }

                            if (make_nonblock(upstream_fd) == -1)
                            {
                                perror("make_nonblock: upstream socket");
                                close(upstream_fd);
                                std::string body = "<h1>502 Bad Gateway - Socket error</h1>";
                                std::ostringstream h;
                                h << "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
                                c->write_buff = h.str();
                                c->read_buff.clear();
                                epoll_event mod_ev;
                                mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                                mod_ev.data.fd = event_fd;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                                continue;
                            }

                            struct sockaddr_in upstream_addr;
                            for (size_t z = 0; z < sizeof(upstream_addr); z++)
                                ((char *)&upstream_addr)[z] = 0;
                            upstream_addr.sin_family = AF_INET;
                            upstream_addr.sin_port = htons(std::atol(pp.port.c_str()));
                            if (inet_pton(AF_INET, pp.host.c_str(), &upstream_addr.sin_addr) <= 0)
                            {
                                std::cerr << "Invalid proxy address: " << pp.host << std::endl;
                                close(upstream_fd);
                                std::string body = "<h1>502 Bad Gateway - Invalid proxy address</h1>";
                                std::ostringstream h;
                                h << "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
                                c->write_buff = h.str();
                                c->read_buff.clear();
                                epoll_event mod_ev;
                                mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                                mod_ev.data.fd = event_fd;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                                continue;
                            }

                            int cres = connect(upstream_fd, (struct sockaddr *)&upstream_addr, sizeof(upstream_addr));
                            bool connecting = false;
                            if (cres < 0)
                            {
                                if (errno == EINPROGRESS)
                                    connecting = true;
                                else
                                {
                                    std::cerr << "Unable to connect to proxy server: " << strerror(errno) << std::endl;
                                    close(upstream_fd);
                                    std::string body = "<h1>502 Bad Gateway - Connection refused</h1>";
                                    std::ostringstream h;
                                    h << "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
                                    c->write_buff = h.str();
                                    c->read_buff.clear();
                                    epoll_event mod_ev;
                                    mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                                    mod_ev.data.fd = event_fd;
                                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                                    continue;
                                }
                            }

                            // Register upstream socket with epoll for asynchronous communication
                            epoll_event upev;
                            upev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                            upev.data.fd = upstream_fd;
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, upstream_fd, &upev) == -1)
                            {
                                perror("epoll_ctl: add upstream");
                                close(upstream_fd);
                                std::string body = "<h1>502 Bad Gateway - Epoll failed</h1>";
                                std::ostringstream h;
                                h << "HTTP/1.1 502 Bad Gateway\r\nContent-Type: text/html\r\nContent-Length: " << body.size() << "\r\nConnection: close\r\n\r\n" << body;
                                c->write_buff = h.str();
                                c->read_buff.clear();
                                epoll_event mod_ev;
                                mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                                mod_ev.data.fd = event_fd;
                                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                                continue;
                            }

                            // Create ProxyConn and map
                            ProxyConn *pc = new ProxyConn(event_fd, upstream_fd, proxyRequest, connecting, server_idx);
                            proxies_by_upstream[upstream_fd] = pc;
                            proxies_by_client[event_fd] = pc;

                            // Clear client read buffer (we consumed the request)
                            c->read_buff.clear();
                            // keep client registered for EPOLLIN; upstream will drive writes to client
                            continue;
                        }
                        else
                        {
                            Response res(req, _servers[server_idx]);
                            c->write_buff = res.toStr();
                            c->read_buff.clear();

                            epoll_event mod_ev;
                            mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                            mod_ev.data.fd = event_fd;
                            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
                        }
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
