/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/03 12:33:35 by lshein           ###   ########.fr       */
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

// Proxy connection state for asynchronous proxy handling
struct ProxyConn {
	int client_fd;
	int upstream_fd;
	std::string request_buffer;  // Data to send to upstream
	std::string response_buffer; // Data received from upstream
	bool connecting;             // True during non-blocking connect
	size_t server_idx;
	
	ProxyConn(int cfd, int ufd, const std::string &req, bool conn, size_t idx)
		: client_fd(cfd), upstream_fd(ufd), request_buffer(req), 
		  response_buffer(""), connecting(conn), server_idx(idx) {}
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


// Helper function: Set socket to non-blocking mode
static int make_nonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Helper function: Cleanup proxy connection
static void cleanup_proxy(ProxyConn *pc, std::map<int, ProxyConn*> &proxies_by_upstream,
                          std::map<int, ProxyConn*> &proxies_by_client, int epoll_fd)
{
	if (pc->upstream_fd >= 0)
	{
		close(pc->upstream_fd);
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pc->upstream_fd, NULL);
	}
	proxies_by_upstream.erase(pc->upstream_fd);
	proxies_by_client.erase(pc->client_fd);
	delete pc;
}

// Helper function: Cleanup client connection
static void cleanup_client(int client_fd, std::map<int, client*> &clients, int epoll_fd)
{
	std::map<int, client*>::iterator it = clients.find(client_fd);
	if (it != clients.end())
	{
		close(client_fd);
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
		delete it->second;
		clients.erase(it);
	}
}

// Setup all server listening sockets
bool WebServer::setupServerSockets(int epoll_fd, std::set<int> &server_fds, 
                                    std::map<int, size_t> &fd_to_server_idx)
{
	for (size_t i = 0; i < _sockets.size(); ++i)
	{
		int fd = _sockets[i].getServerFd();
		if (make_nonblock(fd) == -1)
		{
			perror("make_nonblock: server socket");
			return false;
		}

		epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			return false;
		}
		
		server_fds.insert(fd);
		fd_to_server_idx[fd] = i;
		std::cout << "Listening on port: http://localhost:" << _servers[i].getPort() << std::endl;
	}
	return true;
}

// Handle new client connections
void WebServer::handleNewConnection(int event_fd, size_t server_idx,
                                    std::map<int, client*> &clients, int epoll_fd)
{
	while (true)
	{
		sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(event_fd, (struct sockaddr*)&client_addr, &client_len);
		
		if (client_fd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break; // No more connections
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
}

// Cleanup timed out clients
void WebServer::cleanupTimedOutClients(std::map<int, client*> &clients, int epoll_fd)
{
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
		else 
			++it;
	}
}

int WebServer::serve(void)
{
    // Initialize epoll instance
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        perror("epoll_create");
        return 1;
    }

    // Connection tracking structures
    std::map<int, client*> clients;
    std::map<int, ProxyConn*> proxies_by_upstream; // upstream_fd -> ProxyConn*
    std::map<int, ProxyConn*> proxies_by_client;   // client_fd -> ProxyConn*
    std::set<int> server_fds;
    std::map<int, size_t> fd_to_server_idx;

    // Setup server listening sockets
    if (!setupServerSockets(epoll_fd, server_fds, fd_to_server_idx))
    {
        close(epoll_fd);
        return 1;
    }

    // Main event loop
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

        // Process all ready events
        for (int n = 0; n < nfds; ++n)
        {
            int event_fd = events[n].data.fd;

            // Check if this is a proxy upstream socket
            std::map<int, ProxyConn*>::iterator pit = proxies_by_upstream.find(event_fd);
            if (pit != proxies_by_upstream.end())
            {
                handleProxyEvent(events[n], pit->second, clients,
                                proxies_by_upstream, proxies_by_client, epoll_fd);
                continue;
            }

            // Handle new client connections on listening sockets
            if (server_fds.find(event_fd) != server_fds.end())
            {
                handleNewConnection(event_fd, fd_to_server_idx[event_fd], 
                                   clients, epoll_fd);
                continue;
            }

            // Handle existing client socket events
            handleClientEvent(events[n], clients, proxies_by_upstream,
                             proxies_by_client, epoll_fd);
        }

        // Cleanup timed out clients
        cleanupTimedOutClients(clients, epoll_fd);
    }

    close(epoll_fd);
    return 0;
}

void WebServer::handleClientEvent(const struct epoll_event &event,
                                  std::map<int, client*> &clients,
                                  std::map<int, ProxyConn*> &proxies_by_upstream,
                                  std::map<int, ProxyConn*> &proxies_by_client,
                                  int epoll_fd)
{
    int event_fd = event.data.fd;
    std::map<int, client*>::iterator it = clients.find(event_fd);
    
    if (it == clients.end())
    {
        // Not a tracked client, remove
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
        return;
    }
    
    client* c = it->second;
    c->update_activity();
    size_t server_idx = c->server_idx;

    // Handle errors/hangups
    if (event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        std::cout << "Client disconnected: " << event_fd << std::endl;
        cleanup_client(event_fd, clients, epoll_fd);
        return;
    }

    // Handle incoming data
    if (event.events & EPOLLIN)
    {
        // Read loop for edge-triggered mode
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
                // Client closed connection
                cleanup_client(event_fd, clients, epoll_fd);
                return;
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break; // No more data available
                perror("recv");
                cleanup_client(event_fd, clients, epoll_fd);
                return;
            }
        }

        // Process request if we have complete data and no pending write
        if (!c->read_buff.empty() && c->write_buff.empty())
        {
            try {
                Request req(c->read_buff.c_str(), _servers[server_idx]);
                
                // Check if this is a proxy_pass request
                std::map<std::string, t_location>::const_iterator locit = req.getIt();
                if (locit != _servers[server_idx].getLocation().end() && 
                    !locit->second._proxy_pass.empty())
                {
                    // Setup async proxy connection
                    if (setupProxyConnection(req, c, event_fd, server_idx,
                                            proxies_by_upstream, proxies_by_client, epoll_fd))
                    {
                        return; // Proxy setup successful, wait for upstream
                    }
                    // If proxy setup failed, error response already set in write_buff
                }
                else
                {
                    // Regular request - handle with Response class
                    Response res(req, _servers[server_idx]);
                    c->write_buff = res.toStr();
                    c->read_buff.clear();
                }

                epoll_event mod_ev;
                mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                mod_ev.data.fd = event_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &mod_ev);
            }
            catch (const std::exception &e) {
                std::cerr << "Request/Response error: " << e.what() << std::endl;
                cleanup_client(event_fd, clients, epoll_fd);
                return;
            }
        }
    }

    // Handle outgoing data
    if ((event.events & EPOLLOUT) && c->has_write_data())
    {
        while (!c->write_buff.empty())
        {
            ssize_t bytes_sent = send(event_fd, c->write_buff.c_str(), c->write_buff.size(), MSG_NOSIGNAL);
            if (bytes_sent > 0)
            {
                c->write_buff.erase(0, bytes_sent);
            }
            else if (bytes_sent < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                perror("send");
                cleanup_client(event_fd, clients, epoll_fd);
                return;
            }
        }

        // If all data sent, close connection
        if (!c->has_write_data())
        {
            std::cout << "Response sent, closing: fd=" << event_fd << std::endl;
            cleanup_client(event_fd, clients, epoll_fd);
        }
    }
}

// Handle proxy upstream socket events
void WebServer::handleProxyEvent(const struct epoll_event &event, ProxyConn *pc,
                                 std::map<int, client*> &clients,
                                 std::map<int, ProxyConn*> &proxies_by_upstream,
                                 std::map<int, ProxyConn*> &proxies_by_client,
                                 int epoll_fd)
{
    // Handle errors/hangups
    if (event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
        // Forward any buffered response to client before cleanup
        if (!pc->response_buffer.empty())
        {
            std::map<int, client*>::iterator cit = clients.find(pc->client_fd);
            if (cit != clients.end())
            {
                cit->second->write_buff.append(pc->response_buffer);
                epoll_event mod_ev;
                mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                mod_ev.data.fd = pc->client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, pc->client_fd, &mod_ev);
            }
        }
        cleanup_proxy(pc, proxies_by_upstream, proxies_by_client, epoll_fd);
        return;
    }

    // Handle writable (connect completion or send data)
    if (event.events & EPOLLOUT)
    {
        if (pc->connecting)
        {
            // Check if connect succeeded
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(pc->upstream_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
            {
                std::cerr << "Proxy connect failed: " << strerror(err) << std::endl;
                cleanup_proxy(pc, proxies_by_upstream, proxies_by_client, epoll_fd);
                return;
            }
            pc->connecting = false;
        }

        // Send request data to upstream
        while (!pc->request_buffer.empty())
        {
            ssize_t sent = send(pc->upstream_fd, pc->request_buffer.c_str(), 
                               pc->request_buffer.size(), 0);
            if (sent > 0)
            {
                pc->request_buffer.erase(0, sent);
            }
            else if (sent < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                perror("send to upstream");
                cleanup_proxy(pc, proxies_by_upstream, proxies_by_client, epoll_fd);
                return;
            }
        }
    }

    // Handle readable (receive response from upstream)
    if (event.events & EPOLLIN)
    {
        while (true)
        {
            char buffer[8192];
            ssize_t bytes = recv(pc->upstream_fd, buffer, sizeof(buffer), 0);
            if (bytes > 0)
            {
                // Forward to client immediately
                std::map<int, client*>::iterator cit = clients.find(pc->client_fd);
                if (cit != clients.end())
                {
                    cit->second->write_buff.append(buffer, bytes);
                    epoll_event mod_ev;
                    mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                    mod_ev.data.fd = pc->client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, pc->client_fd, &mod_ev);
                }
            }
            else if (bytes == 0)
            {
                // Upstream closed - cleanup
                cleanup_proxy(pc, proxies_by_upstream, proxies_by_client, epoll_fd);
                break;
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                perror("recv from upstream");
                cleanup_proxy(pc, proxies_by_upstream, proxies_by_client, epoll_fd);
                break;
            }
        }
    }
}

// Setup non-blocking proxy connection
bool WebServer::setupProxyConnection(const Request &req, client *c, int client_fd,
                                     size_t server_idx,
                                     std::map<int, ProxyConn*> &proxies_by_upstream,
                                     std::map<int, ProxyConn*> &proxies_by_client,
                                     int epoll_fd)
{
    t_proxyPass pp = parseProxyPass(req.getIt()->second._proxy_pass);

    // Build proxy request
    std::string proxyRequest = req.getMethodType() + " " + pp.path + " " + 
                               req.getHttpVersion() + "\r\n";
    proxyRequest += "Host: " + pp.host + "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin();
         it != req.getHeaders().end(); ++it)
    {
        if (it->first != "Host")
            proxyRequest += it->first + ": " + it->second + "\r\n";
    }
    proxyRequest += "Connection: close\r\n\r\n" + req.getBody();

    // Create non-blocking upstream socket
    int upstream_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (upstream_fd < 0)
    {
        std::cerr << "Failed to create upstream socket" << std::endl;
        c->write_buff = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
        c->read_buff.clear();
        return false;
    }

    if (make_nonblock(upstream_fd) == -1)
    {
        close(upstream_fd);
        c->write_buff = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
        c->read_buff.clear();
        return false;
    }

    // Setup upstream address
    struct sockaddr_in upstream_addr;
    std::memset(&upstream_addr, 0, sizeof(upstream_addr));
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_port = htons(static_cast<uint16_t>(std::atol(pp.port.c_str())));

    if (inet_pton(AF_INET, pp.host.c_str(), &upstream_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid proxy address: " << pp.host << std::endl;
        close(upstream_fd);
        c->write_buff = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
        c->read_buff.clear();
        return false;
    }

    // Non-blocking connect
    int conn_result = connect(upstream_fd, (struct sockaddr *)&upstream_addr, 
                             sizeof(upstream_addr));
    bool connecting = false;
    if (conn_result < 0)
    {
        if (errno == EINPROGRESS)
        {
            connecting = true;
        }
        else
        {
            std::cerr << "Connect failed: " << strerror(errno) << std::endl;
            close(upstream_fd);
            c->write_buff = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
            c->read_buff.clear();
            return false;
        }
    }

    // Register upstream socket with epoll
    epoll_event upev;
    upev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
    upev.data.fd = upstream_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, upstream_fd, &upev) == -1)
    {
        perror("epoll_ctl: add upstream");
        close(upstream_fd);
        c->write_buff = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
        c->read_buff.clear();
        return false;
    }

    // Create proxy connection object
    ProxyConn *pc = new ProxyConn(client_fd, upstream_fd, proxyRequest, connecting, server_idx);
    proxies_by_upstream[upstream_fd] = pc;
    proxies_by_client[client_fd] = pc;

    // Clear client read buffer (request consumed)
    c->read_buff.clear();
    
    std::cout << "Proxy connection setup for client " << client_fd << " -> upstream " << upstream_fd << std::endl;
    return true;
}
