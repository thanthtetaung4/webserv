/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 04:47:58 by lshein            #+#    #+#             */
/*   Updated: 2025/11/09 13:45:55 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>
#include <string>
#include "Location.h"

class Server
{
    private:
        std::string _port;
        std::string _serverName;
        std::string _maxBytes;
        std::map<std::string, std::string> _errorPage;
        std::map<std::string, t_location> _locations;
        std::map<std::string, std::string> _return;
    public:
        void setPort(const std::string &port);
        void setServerName(const std::string &serverName);
        void setMaxBytes(const std::string &byte);
        void setErrorPage(const std::string &key, const std::string &value);
        void setLocation(std::string key, const t_location &location);
        void setReturn(const std::string &key, const std::string &value);
        std::string getPort() const;
        std::string getServerName() const;
        std::string getMaxByte() const;
        const std::map<std::string, std::string> &getErrorPage() const;
        const std::map<std::string, t_location> &getLocation() const;
        const std::map<std::string, std::string> &getReturn() const;
};
std::ostream &operator<<(std::ostream &os, const Server &s);
#endif
