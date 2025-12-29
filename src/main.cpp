/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:41:50 by lshein            #+#    #+#             */
/*   Updated: 2025/12/20 02:36:34 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Request.hpp"
#include "../include/Response.hpp"
#include "../include/WebServer.hpp"
#include "../include/utils.h"
#include <csignal>

int main(int argc, char **argv)
{
        std::string configFile;

         if (argc == 1) {
                configFile = "./config/default.conf";
        } else if (argc == 2) {
                configFile = argv[1];
        } else {
                std::cerr << "Usage: ./webserv [config file]" << std::endl;
                return 1;
        }
        // Register signal handlers
        signal(SIGINT, handleSignal);
        signal(SIGTERM, handleSignal);
        
        try {
                WebServer ws;
                ws.setServer(configFile);   // read config
                ws.getServers();

                std::cout << "============ SERVERS =============" << std::endl;
                std::vector<Server> servers = ws.getServers();
                for (size_t i = 0; i < servers.size(); i++) {
                        std::cout << servers[i] << std::endl;
                }
                std::cout << "============ SERVERS END =============" << std::endl;
                ws.setUpSock();          // bind/listen
                return ws.run();         // event-driven loop
        }
        catch(const std::exception& e) {
                std::cerr << "Fatal Error: " << e.what() << std::endl;
                return 1;
        }
}
