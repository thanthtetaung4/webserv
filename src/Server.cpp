/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/08 07:59:51 by lshein            #+#    #+#             */
/*   Updated: 2025/10/08 12:11:06 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../include/Server.hpp"

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

std::ostream &operator<<(std::ostream &os, const Server &s)
{
    os << "*********Server*********\n";
    os << "Port: " << s.getPort() << std::endl;
    os << "ServerName: " << s.getServerName() << std::endl;
    os << "MaxByte: " << s.getMaxByte() << std::endl;
	const std::map<std::string, std::string> &errors = s.getErrorPage();
	for (std::map<std::string, std::string>::const_iterator it = errors.begin(); it != errors.end(); ++it) 
	{
		std::cout << "Error page: [" << it->first << "] = " << it->second << std::endl;
	}
	os << "************************" << std::endl;
	return os;
}
