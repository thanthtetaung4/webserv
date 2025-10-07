/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:33:13 by lshein            #+#    #+#             */
/*   Updated: 2025/10/07 11:40:10 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Server.hpp"
#include <fstream>
#include <sstream>

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
void parser(std::string config, int type);

#endif
