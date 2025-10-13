#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

int loop() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // 2. Set address and bind
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080); // use 80 if you want, but need sudo

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    // 3. Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        return 1;
    }

    std::cout << "Server running on http://localhost:8080\n";

    // 4. Accept a connection
    client_fd = accept(server_fd, (struct sockaddr*)&addr, &addr_len);
    if (client_fd < 0) {
        perror("accept failed");
        return 1;
    }

    // 5. Read the client request
    char buffer[1024] = {0};
    read(client_fd, buffer, sizeof(buffer) - 1);
    std::cout << "Received request:\n" << buffer << "\n";

    // 6. Send a basic HTTP response
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<h1>Hello from mini C++ server!</h1>";

    write(client_fd, response, strlen(response));

    // 7. Close sockets
    close(client_fd);
    close(server_fd);
    return 0;
}
