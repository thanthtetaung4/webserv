/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:41:50 by lshein            #+#    #+#             */
/*   Updated: 2025/10/11 14:51:08 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "./../include/WebServer.hpp"
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
}
