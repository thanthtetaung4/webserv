/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:41:50 by lshein            #+#    #+#             */
/*   Updated: 2025/11/09 14:17:47 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServer.hpp"
#include "../include/Response.hpp"
#include "../include/Request.hpp"

int main(int argc, char **argv)
{
        if (argc == 2)
        {
                WebServer ws;
                try
                {
                        ws.setServer(argv[1]);
                        for (int i = 0; i < 2; i++)
                        {
                                std::cout << ws.getServers()[i] << std::endl;
                        }
                        ws.setUpSock();
                        ws.serve();
                }
                catch (std::exception &e)
                {
                        e.what();
                }
        }
        else
                std::cerr << "Usage: ./webserv [config file]" << std::endl;
}

// int main(int argc, char **argv)
// {
//         (void)argv;
//         if (argc == 2)
//         {
//                 WebServer ws;
//                 Server s1, s2;

//                 // Configure Server 1
//                 s1.setPort("8080");
//                 s1.setServerName("server_one");
//                 s1.setMaxBytes("1048576"); // 1MB
//                 s1.setErrorPage("404", "www/html/404.html");

//                 t_location loc1;
//                 loc1._root = "www/html";
//                 loc1._index.push_back("index.html");
//                 loc1._index.push_back("index.htm");
//                 loc1._limit_except.push_back("GET");
//                 loc1._autoIndex = "off";
//                 s1.setLocation("/", loc1);

//                 // Configure Server 2
//                 s2.setPort("9090");
//                 s2.setServerName("server_two");
//                 s2.setMaxBytes("2097152"); // 2MB
//                 s2.setErrorPage("500", "/errors/500.html");
//                 s2.setErrorPage("404", "/errors/404.html");

//                 t_location loc2;
//                 loc2._root = "/var/www/html";
//                 loc2._index.push_back("home.html");
//                 loc2._limit_except.push_back("GET");
//                 loc2._limit_except.push_back("POST");
//                 loc2._autoIndex = "on";
//                 loc2._proxy_pass = "http://127.0.0.1:8080";
//                 s2.setLocation("/", loc2);

//                 // Add servers to WebServer (assuming addServer takes a Server object)
//                 ws.addServer(s1);
//                 ws.addServer(s2);

//                 // For testing: print server configs
//                 std::cout << s1 << std::endl;
//                 std::cout << s2 << std::endl;
//                 // std::cout << s2 << std::endl;
//                 ws.setUpSock();
//                 ws.serve();
//         }
//         else
//                 std::cerr << "Usage: ./webserv [config file]" << std::endl;
// }

/*
        std::string path;
        std::map<std::string, t_location> locations = server.getLocation();
        std::map<std::string, t_location>::iterator it = locations.find(req._urlPath);
        if(it != locations.end())
                path = it->second._root;
        else
                path = "";*/
