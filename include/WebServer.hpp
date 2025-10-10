/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:33:13 by lshein            #+#    #+#             */
/*   Updated: 2025/10/10 00:45:56 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Server.hpp"
#include <fstream>
#include <sstream>

typedef struct its
{
    std::string::iterator it1;
    std::string::iterator it2;
} t_its;

class WebServer
{
    private:
        std::vector<Server> _servers;
    public:
        WebServer();
        ~WebServer();
        // WebServer(const WebServer &src);
        // WebServer &operator=(const WebServer &other);
        void setServer(std::string configFile);
};
t_its getIts(std::string &content, std::string::iterator start, const std::string &target1, const std::string &target2);
void setAttributes(const std::vector<std::string> &line, Server &server);
void setLocationAttributes(const std::vector<std::string> &line, t_location &location, std::string &key);
void getServerBlock(t_its it, std::vector<Server> &servers);
void getLocationBlock(t_its it, Server &server);

#endif
