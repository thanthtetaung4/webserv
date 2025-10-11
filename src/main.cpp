/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:41:50 by lshein            #+#    #+#             */
/*   Updated: 2025/10/11 15:23:27 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../include/WebServer.hpp"
# include "../include/Response.hpp"
# include "../include/Request.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

int loop() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int opt = 1;

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // 2. Allow address reuse (avoid “Address already in use”)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }

    // 3. Bind socket to port 8080
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    // 4. Start listening for connections
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "************************\n";
    std::cout << "Server running on http://localhost:8080\n";
    std::cout << "Waiting for connections...\n";

    // 5. Main server loop
    while (true) {
        client_fd = accept(server_fd, (struct sockaddr*)&addr, &addr_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // 6. Read request
	char buffer[4096] = {0};
	ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
	if (bytes_read > 0) {
		std::string raw(buffer, bytes_read);
		Request req = Request::Parse(raw);
		Response res = Response::handleResponse(req);
		std::string out = res.toStr();
		std::cout << "---------------------------" <<std::endl;
		std::cout << out << std::endl;
		std::cout << "----------------------------" <<std::endl;
		write(client_fd, out.c_str(), out.size());
	}

	else// 8. Close client connection
        	close(client_fd);
    }

    // (Unreachable normally)
    std::cout << "------------------------End-------------------\n";
    close(server_fd);
    return 0;
}


# include "../include/Server.hpp"

int main(int argc, char **argv) {
    (void)argv;
    if (argc == 2)
    {
        WebServer ws;
        Server s1,s2;

                // Configure Server 1
        s1.setPort("8080");
        s1.setServerName("server_one");
        s1.setMaxBytes("1048576"); // 1MB
        s1.setErrorPage("404", "/errors/404.html");

        t_location loc1;
        loc1._root = "/var/www/html1";
        loc1._index = "index.html";
        loc1._limit_except.push_back("GET");
        loc1._autoIndex = "off";
        s1.setLocation("/", loc1);

        // Configure Server 2
        s2.setPort("9090");
        s2.setServerName("server_two");
        s2.setMaxBytes("2097152"); // 2MB
        s2.setErrorPage("500", "/errors/500.html");

        t_location loc2;
        loc2._root = "/var/www/html2";
        loc2._index = "home.html";
        loc2._limit_except.push_back("GET");
        loc2._limit_except.push_back("POST");
        loc2._autoIndex = "on";
        s2.setLocation("/app", loc2);

        // Add servers to WebServer (assuming addServer takes a Server object)
        ws.addServer(s1);
        ws.addServer(s2);

        // For testing: print server configs
        std::cout << s1 << std::endl;
        // std::cout << s2 << std::endl;
        ws.setUpSock();
        ws.serve();
    }
    else
	    std::cerr << "Usage: ./webserv [config file]" << std::endl;
    loop();
}
