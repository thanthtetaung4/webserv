/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 04:47:58 by lshein            #+#    #+#             */
/*   Updated: 2025/11/20 08:56:36 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>
#include <string>
#include "Location.h"
#include <sstream>
#include <vector>
#include <map>
#include <iostream>
#include "Validator.hpp"

typedef struct iterators
{
    std::string::iterator it1;
    std::string::iterator it2;
} t_iterators;
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
    typedef void (Server::*AttrHandler)(const std::vector<std::string> &line);
    typedef void (Server::*LocAttrHandler)(const std::vector<std::string> &line, t_location &location, std::string &key);
    static std::map<std::string, AttrHandler> &getServerHandlers();
    static std::map<std::string, LocAttrHandler> &getLocationHandlers();
    void handlePort(const std::vector<std::string> &line);
    void handleServerName(const std::vector<std::string> &line);
    void handleMaxBodySize(const std::vector<std::string> &line);
    void handleErrorPage(const std::vector<std::string> &line);
    void handleRoot(const std::vector<std::string> &line);
    void handleIndex(const std::vector<std::string> &line);
    void handleReturn(const std::vector<std::string> &line);
    void handleLocation(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleLocRoot(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleLocIndex(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleLimitExcept(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleLocReturn(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleAutoindex(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleCgiPass(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleCgiExt(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleUploadStore(const std::vector<std::string> &line, t_location &loc, std::string &key);
    void handleProxyPass(const std::vector<std::string> &line, t_location &loc, std::string &key);

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
    static t_iterators getIterators(std::string &content, std::string::iterator start, const std::string &target1, const std::string &target2);
    void fetchSeverInfo(t_iterators it);
    void fetchLocationInfo(t_iterators it);
    void setServerInfo(const std::vector<std::string> &line);
    void setLocationInfo(const std::vector<std::string> &line, t_location &location, std::string &key);
};
std::ostream &operator<<(std::ostream &os, const Server &s);

#endif
