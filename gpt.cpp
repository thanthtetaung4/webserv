
struct Conn
{
	std::string buffer;
	long start_time_ms;
};
#include <sys/time.h>
long now_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}
int WebServer::serve(void)
{
	std::map<int, size_t> clientToServer;
	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
		perror("epoll_create1");
		return 1;
	}

	std::map<int, Conn> connections; // client_fd → connection state
	const long TIMEOUT_MS = 5000;	 // 5 second timeout

	// REGISTER SERVER SOCKETS
	for (size_t i = 0; i < _sockets.size(); ++i)
	{
		int fd = _sockets[i].getServerFd();

		int flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);

		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = fd;

		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			close(epoll_fd);
			return 1;
		}

		std::cout << "Listening on http://localhost:"
				  << _servers[i].getPort() << std::endl;
	}

	struct epoll_event events[MAX_EVENTS];

	while (true)
	{
		// 1. Check timeout for all clients
		long current = now_ms();
		std::map<int, Conn>::iterator it;
		for (it = connections.begin(); it != connections.end();)
		{
			long elapsed = current - it->second.start_time_ms;
			if (elapsed > TIMEOUT_MS)
			{
				std::string timeoutRes =
					"HTTP/1.1 408 Request Timeout\r\n"
					"Content-Length: 0\r\n\r\n";

				send(it->first, timeoutRes.c_str(), timeoutRes.size(), 0);
				close(it->first);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, NULL);

				std::map<int, Conn>::iterator tmp = it;
				++it;
				connections.erase(tmp);
				clientToServer.erase(tmp->first);
			}
			else
			{
				++it;
			}
		}

		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 100);
		if (nfds == -1)
		{
			perror("epoll_wait");
			break;
		}

		for (int n = 0; n < nfds; ++n)
		{
			int fd = events[n].data.fd;

			// Check if fd is a server socket
			bool isServerSocket = false;
			size_t serverIndex = 0;
			for (size_t i = 0; i < _sockets.size(); ++i)
			{
				if (_sockets[i].getServerFd() == fd)
				{
					isServerSocket = true;
					serverIndex = i;
					break;
				}
			}

			if (isServerSocket)
			{
				// ACCEPT ALL PENDING CONNECTIONS
				while (true)
				{
					sockaddr_in client_addr;
					socklen_t client_len = sizeof(client_addr);
					int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
					if (client_fd < 0)
					{
						if (errno == EAGAIN || errno == EWOULDBLOCK)
							break;
						perror("accept");
						break;
					}

					// Non-blocking client
					int flags = fcntl(client_fd, F_GETFL, 0);
					fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

					struct epoll_event ev;
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = client_fd;
					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

					Conn c;
					c.buffer = "";
					c.start_time_ms = now_ms();
					connections[client_fd] = c;
					clientToServer[client_fd] = serverIndex;
				}
				continue;
			}

			// CLIENT SOCKET → READ DATA
			char buffer[4096];
			while (true)
			{
				ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
				if (bytes < 0)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						break;
					perror("recv");
					close(fd);
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
					connections.erase(fd);
					clientToServer.erase(fd);
					break;
				}
				else if (bytes == 0) // client disconnected
				{
					close(fd);
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
					connections.erase(fd);
					clientToServer.erase(fd);
					break;
				}
				else
				{
					connections[fd].buffer.append(buffer, bytes);
					connections[fd].start_time_ms = now_ms(); // reset timeout
				}
			}

			std::string &reqStr = connections[fd].buffer;
			if (reqStr.find("\r\n\r\n") == std::string::npos)
				continue; // not full request

			size_t idx = clientToServer[fd];

			// DEBUG PRINT
			std::cout << "=========== RAW REQUEST (port "
					  << _servers[idx].getPort() << ") ===========" << std::endl;
			std::cout << reqStr << std::endl;

			// Parse request
			Request req(reqStr.c_str());
			int validation_status = req.validateAgainstConfig(_servers[idx]);
			if (validation_status != 200)
			{
				Response res(validation_status);
				std::string httpResponse = res.toStr();
				send(fd, httpResponse.c_str(), httpResponse.size(), 0);
				close(fd);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				connections.erase(fd);
				clientToServer.erase(fd);
				continue;
			}

			// Proxy pass
			if (this->isProxyPass(req.getUrlPath(), _servers[idx]))
			{
				std::string rawRes = this->handleReverseProxy(req, _servers[idx]);
				send(fd, rawRes.c_str(), rawRes.size(), 0);
				close(fd);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				connections.erase(fd);
				clientToServer.erase(fd);
				continue;
			}

			// Autoindex
			if (req.isAutoIndex(_servers[idx]))
			{
				std::string rawRes = handleAutoIndex(req, _servers[idx]);
				send(fd, rawRes.c_str(), rawRes.size(), 0);
				close(fd);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				connections.erase(fd);
				clientToServer.erase(fd);
				continue;
			}

			// CGI
			if (isCGI(req.getUrlPath(), _servers[idx]))
			{
				Cgi cgi(req, _servers[idx]);
				close(fd);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
				connections.erase(fd);
				clientToServer.erase(fd);
				continue;
			}

			// Normal response
			Response res(req, _servers[idx]);
			std::string httpResponse = res.toStr();
			send(fd, httpResponse.c_str(), httpResponse.size(), 0);
			close(fd);
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
			connections.erase(fd);
			clientToServer.erase(fd);
		}
	}

	close(epoll_fd);
	return 0;
}

