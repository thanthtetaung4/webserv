/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 04:47:58 by lshein            #+#    #+#             */
/*   Updated: 2025/11/16 19:52:38 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>
#include <string>
# include <vector>
#include "Location.h"

class Server
{
    private:
        std::string _port;
        std::string _serverName;
        std::string _maxBytes;
        std::string _serverRoot;
        std::vector<std::string> _serverIndices;
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
        void setServerRoot(std::string sr);
        void setServerIndex(const std::vector<std::string> &indices);
        std::string getPort() const;
        std::string getServerName() const;
        std::string getMaxByte() const;
        std::string getServerRoot() const;
        const std::vector<std::string> &getServerIndex(void) const;
        const std::map<std::string, std::string> &getErrorPage() const;
        const std::map<std::string, t_location> &getLocation() const;
        const std::map<std::string, std::string> &getReturn() const;
};
std::ostream &operator<<(std::ostream &os, const Server &s);
#endif
