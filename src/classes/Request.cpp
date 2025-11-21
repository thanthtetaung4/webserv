/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:04:38 by hthant            #+#    #+#             */
/*   Updated: 2025/11/20 19:36:02 by taung            ###   ########.fr       */
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

static std::string trim(const std::string &s)
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

std::string Request::getUrlPath() const
{
	return this->_urlPath;
}

std::string Request::getHttpVersion() const
{
	return this->_httpVersion;
}

std::string Request::getBody() const
{
	return this->_body;
}

const std::map<std::string, std::string> &Request::getHeaders() const
{
	return this->_headers;
}

/*
	check whether the header values matche the server config
	@ param void
	@ return true if all header values are valid
	@ return false if any header value is invalid
*/
bool Request::checkHeaderValue(void) const
{

	return true;
}

Request::Request(void)
{
	throw UnableToCreateRequest();
}


int Request::validateAgainstConfig(Server &server) {
// 	size_t maxBody;
// 	std::stringstream ss(server.getMaxByte());
// 	ss >> maxBody;

// 	std::map<std::string, t_location> locations = server.getLocation();

// 	t_location* matched = NULL;
// 	size_t bestLen = 0;
// 	for (std::map<std::string, t_location>::iterator it = locations.begin(); it != locations.end(); ++it) {
// 		if (this->_urlPath.find(it->first) == 0 && it->first.length() > bestLen) {
// 			matched = &it->second;
// 			bestLen = it->first.length();
// 		}
// 	}
// 	if (!matched) {
// 		std::cout << "404 returned" << std::endl;
// 		return 404;
// 	}

// 	if(this->getMethodType() != "GET" && this->getMethodType() != "POST" && this->getMethodType() != "DELETE")
// 		return 405;

// 	if (this->getBody().size() > maxBody)
// 		return 413;

// 	std::string path = matched->_root + this->_urlPath.substr(bestLen);
// 	std::ifstream file(path.c_str());
// 	if(!file.is_open())
// 		return 404;

// 	if(access(path.c_str(), R_OK) < 0 || access(path.c_str(), W_OK)< 0 || access(path.c_str(), X_OK) < 0 )
// 		return 403;

// 	return 200;
// }
	size_t maxBody;
	std::stringstream ss(server.getMaxByte());
	ss >> maxBody;

	std::map<std::string, t_location> locations = server.getLocation();

	t_location *matched = NULL;
	size_t bestLen = 0;

	// Longest-prefix-match (location matching)
	for (std::map<std::string, t_location>::iterator it = locations.begin();
		it != locations.end(); ++it)
	{
		if (this->_urlPath.find(it->first) == 0 && it->first.length() > bestLen) {
			matched  = &it->second;
			bestLen  = it->first.length();
		}
	}

	if (!matched)
		return 404;

	// Method validation
	if(this->getMethodType() != "GET" &&
	this->getMethodType() != "POST" &&
	this->getMethodType() != "DELETE")
	{
		return 405;
	}

	// Body size
	if (this->getBody().size() > maxBody)
		return 413;

	// Build full file system path
	std::string relPath = this->_urlPath.substr(bestLen);
	std::string fullPath = matched->_root + relPath;

	struct stat st;
	if (stat(fullPath.c_str(), &st) == -1) {
		return 404; // Not found
	}

	// Permission checks
	if (access(fullPath.c_str(), R_OK) == -1)
		return 403;

	return 200;
}

bool	checkIndices(std::vector<std::string> indices, std::string locationRoot, std::string serverRoot) {
	std::vector<std::string>::const_iterator it = indices.begin();
	(void)locationRoot;
	(void)serverRoot;
	std::cout << "checking indices" << std::endl;
	it == indices.end() ? std::cout << "fuck" : std::cout << "unfuck";
	std::cout << std::endl;
	while (it != indices.end()) {
		std::string	index = *it;
		if (!locationRoot.empty()) {
			locationRoot.append(index);
			index = locationRoot;
		} else {
			if (!serverRoot.empty()) {
				serverRoot.append(index);
				index = serverRoot;
			} else
				return (false);
		}
		if(access(index.c_str(), F_OK) == -1)
			return (false);
		it++;
	}
	return (true);
}

bool	Request::isAutoIndex(Server& server) const {
	// t_location*	loc = searchMapLongestMatch(server.getLocation(), this->_urlPath);

	// std::cout << "checking auto index: " << loc->_autoIndex << std::endl;

	// if (loc->_index.empty() ||
	// 	!checkIndices(loc->_index, loc->_root, server.getServerRoot())) {
	// 	return true;
	// }

	// return false;

	std::map<std::string, t_location>::const_iterator it;
	std::map<std::string, t_location> locs = server.getLocation();

	it = locs.end();

	for (std::map<std::string, t_location>::const_iterator iter = locs.begin();
		iter != locs.end(); ++iter)
	{
		// if (this->_urlPath.compare(0, iter->first.size(), iter->first) == 0 &&
		// 	iter->first.size() > bestLen)
		// {
		// 	bestLen = iter->first.size();
		// 	it = iter;
		// }
		std::cout << "The path: " << iter->first << std::endl;
		if (this->_urlPath == iter->first || this->_urlPath == iter->first + "/") {
			if (iter->second._autoIndex == "on")
				it = iter;
		}
	}

	if (it != locs.end()) {
		std::cout << "checking auto index: " << it->first << std::endl;

		if (it->second._index.empty() ||
			!checkIndices(it->second._index, it->second._root, server.getRoot()))
		{
			return true;
		}

		return false;
	}

	return false;
}

Request::Request(const std::string &raw) {
	size_t hearderEnd = raw.find("\r\n\r\n");
	std::string hearderPart = raw.substr(0, hearderEnd);
	if (hearderEnd != std::string::npos)
		this->_body = raw.substr(hearderEnd + 4);

	std::stringstream lines(hearderPart);
	std::string line;
	if (std::getline(lines, line))
	{
		std::istringstream first(line);
		std::cout << "First line: " << line << std::endl;
		first >> this->_method >> this->_urlPath >> this->_httpVersion;
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
}

std::ostream &operator<<(std::ostream &os, const Request &req)
{
	os << "Method: " << req.getMethodType() << std::endl;
	os << "URL Path: " << req.getUrlPath() << std::endl;
	os << "HTTP Version: " << req.getHttpVersion() << std::endl;
	os << "Headers: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
	{
		os << "	[" << it->first << "]: " << it->second << std::endl;
	}
	return os;
}
