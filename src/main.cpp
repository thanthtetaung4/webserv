/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:41:50 by lshein            #+#    #+#             */
/*   Updated: 2025/10/07 11:38:16 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../include/WebServer.hpp"

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
        char buffer[1024] = {0};
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            std::cout << "--------------------------\n";
            std::cout << "Received request:\n" << buffer << "\n";
        }

	std::cout << "---------------Finish request ---------------"<<std::endl;

        // 7. Send response
        const char *response =
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<h1>Hello from mini C++ server!</h1>";

        if (write(client_fd, response, strlen(response)) < 0)
            perror("write failed");

        // 8. Close client connection
        close(client_fd);
    }

    // (Unreachable normally)
    close(server_fd);
    return 0;
}



int main(int argc, char **argv)
{
    if (argc == 2)
    {
        WebServer ws;
        ws.setServer(argv[1]);
    }
    else
	    std::cerr << "Usage: ./webserv [config file]" << std::endl;
    loop();
}
