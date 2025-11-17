/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 04:47:58 by lshein            #+#    #+#             */
/*   Updated: 2025/11/17 06:12:23 by lshein           ###   ########.fr       */
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
    std::string _root;
    std::vector<std::string> _index;
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
    void setRoot(const std::string &root);
    void setIndex(const std::string &index);
    std::string getPort() const;
    std::string getServerName() const;
    std::string getMaxByte() const;
    std::string getRoot() const;
    std::vector<std::string> getIndex() const;
    const std::map<std::string, std::string> &getErrorPage() const;
    const std::map<std::string, t_location> &getLocation() const;
    const std::map<std::string, std::string> &getReturn() const;
};
std::ostream &operator<<(std::ostream &os, const Server &s);
#endif
