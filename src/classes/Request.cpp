/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:04:38 by hthant            #+#    #+#             */
/*   Updated: 2025/12/01 17:44:49 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Request.hpp"
#include <cstddef>
#include <map>
#include <ostream>
#include <sstream>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

Request::Request(const std::string &raw, Server &server)
{
	size_t hearderEnd = raw.find("\r\n\r\n");
	std::string hearderPart = raw.substr(0, hearderEnd);
	if (hearderEnd != std::string::npos)
		this->_body = raw.substr(hearderEnd + 4);

	std::stringstream lines(hearderPart);
	std::string line;
	if (std::getline(lines, line))
	{
		std::istringstream first(line);
		std::cout << "Request line: " << line << std::endl;
		first >> this->_method >> this->_path >> this->_httpVersion;
	}
	while (std::getline(lines, line))
	{
		if (line.empty())
			break;
		size_t delimeterPostion = line.find(":");
		if (delimeterPostion != std::string::npos)
		{
			std::string key = line.substr(0, delimeterPostion);
			std::string value = trim(line.substr(delimeterPostion + 1));
			this->_headers[key] = value;
		}
	}
	_it = searchLongestMatch(server.getLocation(), this->_path);
	this->_finalPath = "";
	if (_it != server.getLocation().end())
	{
		// if (this->_path == "/")
		// 	this->_finalPath = this->_path;
		if (!_it->second._root.empty())
			this->_finalPath = _it->second._root + (this->_path.substr(_it->first.length()).empty() ? "" : "/" + this->_path.substr(_it->first.length()));
		else
			this->_finalPath = server.getRoot() + ((server.getRoot().at(server.getRoot().length() - 1) == '/' || _path[0] == '/' ) ? "" : "/") + this->_path;
	}
	else
		this->_finalPath = server.getRoot() + ((server.getRoot().at(server.getRoot().length() - 1) == '/' || _path[0] == '/' ) ? "" : "/") + this->_path;
}

std::string trim(const std::string &s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
}

std::string Request::getMethodType() const
{
	return this->_method;
}

std::string Request::getPath() const
{
	return this->_path;
}

std::string Request::getHttpVersion() const
{
	return this->_httpVersion;
}

std::string Request::getBody() const
{
	return this->_body;
}

std::string Request::getFinalPath() const
{
	return this->_finalPath;
}

std::map<std::string, t_location>::const_iterator Request::getIt() const
{
	return this->_it;
}

const std::map<std::string, std::string> &Request::getHeaders() const
{
	return this->_headers;
}

void Request::setFinalPath(const std::string &path)
{
	_finalPath = path;
}

std::ostream &operator<<(std::ostream &os, const Request &req)
{
	os << "Method: " << req.getMethodType() << std::endl;
	os << "URL Path: " << req.getPath() << std::endl;
	os << "Final Path: " << req.getFinalPath() << std::endl;
	os << "HTTP Version: " << req.getHttpVersion() << std::endl;
	os << "Headers: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
	{
		os << "	[" << it->first << "]: " << it->second << std::endl;
	}
	return os;
}
