/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/08 07:59:51 by lshein            #+#    #+#             */
/*   Updated: 2025/11/10 17:17:55 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../../include/Server.hpp"

void Server::setPort(const std::string &port)
{
	_port = port;
}

void Server::setServerName(const std::string &serverName)
{
	_serverName = serverName;
}

void Server::setErrorPage(const std::string &key, const std::string &value)
{
	_errorPage[key] = value;
}

void Server::setMaxBytes(const std::string &byte)
{
	_maxBytes = byte;
}

void Server::setLocation(std::string key, const t_location &location)
{
	_locations[key] = location;
}

void Server::setReturn(const std::string &key, const std::string &value)
{
	_return[key] = value;
}

std::string Server::getPort() const
{
	return _port;
}

std::string Server::getServerName() const
{
	return _serverName;
}

std::string Server::getMaxByte() const
{
	return _maxBytes;
}

const std::map<std::string, std::string> &Server::getErrorPage() const
{
	return _errorPage;
}

const std::map<std::string, t_location> &Server::getLocation() const
{
	return _locations;
}

const std::map<std::string, std::string> &Server::getReturn() const
{
	return _return;
}

std::ostream &operator<<(std::ostream &os, const Server &s)
{
    os << "*********Server*********\n";
    os << "Port: " << s.getPort() << std::endl;
    os << "ServerName: " << s.getServerName() << std::endl;
    os << "MaxByte: " << s.getMaxByte() << std::endl;
	const std::map<std::string, std::string> &errors = s.getErrorPage();
	for (std::map<std::string, std::string>::const_iterator it = errors.begin(); it != errors.end(); ++it)
	{
		os << "Error page: [" << it->first << "] = " << it->second << std::endl;
	}
	const std::map<std::string, t_location> &locations = s.getLocation();
	for (std::map<std::string, t_location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
	{
		os << "Location: [" << it->first << "]" << std::endl;

		os << "	root: " << it->second._root << std::endl;
		os << "	index: ";
		for (size_t i = 0; i < it->second._index.size(); i++)
			os << it->second._index[i] << " ";
		os << std::endl;
		os << "	limit except: ";
		for (unsigned int i = 0; i < it->second._limit_except.size(); i++)
			os << it->second._limit_except[i] << " ";
		os << std::endl;
		for (std::map<std::string, std::string>::const_iterator it1 = it->second._return.begin(); it1 != it->second._return.end(); ++it1)
		{
			os << "	return: [" << it1->first << "] = " << it1->second << std::endl;
		}
		os << "	autoIndex: " << it->second._autoIndex << std::endl;
		os << "	cgiPass: " << it->second._cgiPass << std::endl;
		os << "	cgiExt: " << it->second._cgiExt << std::endl;
		os << "	uploadStore: " << it->second._uploadStore << std::endl;
		os << "	proxyPass: " << it->second._proxyPass << std::endl;
	}
	os << "************************" << std::endl;
	return os;
}
