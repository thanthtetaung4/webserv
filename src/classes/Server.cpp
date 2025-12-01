/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/08 07:59:51 by lshein            #+#    #+#             */
/*   Updated: 2025/11/30 12:14:50 by lshein           ###   ########.fr       */
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
	_return.push_back(key);
	_return.push_back(value);
}

void Server::setRoot(const std::string &root)
{
	_root = root;
}

void Server::setIndex(const std::string &index)
{
	_index.push_back(index);
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

std::string Server::getRoot() const
{
	return _root;
}

std::vector<std::string> Server::getIndex() const
{
	return _index;
}

const std::map<std::string, std::string> &Server::getErrorPage() const
{
	return _errorPage;
}

const std::map<std::string, t_location> &Server::getLocation() const
{
	return _locations;
}

const std::vector<std::string> &Server::getReturn() const
{
	return _return;
}

t_iterators Server::getIterators(std::string &content, std::string::iterator start, const std::string &target1, const std::string &target2)
{
	t_iterators it;

	it.it1 = std::search(start, content.end(), target1.begin(), target1.end());
	if (it.it1 == content.end())
		throw "Invalid config file";
	else
		it.it2 = std::search(it.it1 + 1, content.end(), target2.begin(), target2.end());
	return it;
}

void Server::fetchSeverInfo(t_iterators it)
{
	std::string content(it.it1, it.it2);
	std::stringstream serverString(content);
	std::string line;
	t_iterators itLoc;

	std::string::iterator pos = content.begin();
	while (std::getline(serverString, line))
	{
		if (line.find_first_not_of(" \t") == std::string::npos)
			continue;
		if (!line.empty())
		{
			if (line.at(line.size() - 1) != '{' && line.at(line.size() - 1) != '}')
			{
				if (line.at(line.size() - 1) != ';')
					throw std::runtime_error("Invalid directive format.\nMissing ';'");
			}
		}
		std::stringstream ss(line);
		std::string token;
		std::vector<std::string> lineVec;
		while (ss >> token)
		{
			if (!token.empty() && token == "location")
			{
				itLoc = getIterators(content, pos, token, "}");
				fetchLocationInfo(itLoc);
				pos = itLoc.it2;
				std::string remaining(pos, content.end());
				serverString.str(remaining);
				serverString.clear();
				continue;
			}
			if (!token.empty() && !(token.at(token.size() - 1) == ';'))
				lineVec.push_back(token);
			else if (!token.empty() && (token.at(token.size() - 1) == ';'))
			{
				token = token.substr(0, token.size() - 1);
				lineVec.push_back(token);
				setServerInfo(lineVec);
				lineVec.clear();
			}
		}
	}
	// addServer(server);
}

void Server::fetchLocationInfo(t_iterators it)
{
	std::string content(it.it1, it.it2);
	std::stringstream locationString(content);
	std::string line;
	t_location location;
	std::string key;

	while (std::getline(locationString, line))
	{
		std::stringstream ss(line);
		std::string token;
		std::vector<std::string> lineVec;

		while (ss >> token)
		{
			if (!token.empty() && !(token.at(token.size() - 1) == ';') && !(token == "{"))
				lineVec.push_back(token);
			else if (!token.empty() && (token.at(token.size() - 1) == ';'))
			{
				token = token.substr(0, token.size() - 1);
				lineVec.push_back(token);
				setLocationInfo(lineVec, location, key);
				lineVec.clear();
			}
			else if (!token.empty() && token == "{")
			{
				lineVec.push_back(token);
				setLocationInfo(lineVec, location, key);
				lineVec.clear();
			}
		}
	}
	if (location._limit_except.empty())
		location._limit_except.push_back("GET");
	if (!ConfigValidator::validateLocation(location, key))
		throw std::runtime_error("Invalid location directive format");
	if (!location._cgiExt.empty() && !location._cgiPass.empty())
		location._isCgi = true;
	else
		location._isCgi = false;
	setLocation(key, location);
}

std::map<std::string, Server::AttrHandler> &Server::getServerHandlers()
{
	static std::map<std::string, AttrHandler> handlers;
	if (handlers.empty())
	{
		handlers["listen"] = &Server::handlePort;
		handlers["server_name"] = &Server::handleServerName;
		handlers["client_max_body_size"] = &Server::handleMaxBodySize;
		handlers["error_page"] = &Server::handleErrorPage;
		handlers["root"] = &Server::handleRoot;
		handlers["index"] = &Server::handleIndex;
		handlers["return"] = &Server::handleReturn;
	}
	return handlers;
}

std::map<std::string, Server::LocAttrHandler> &Server::getLocationHandlers()
{
	static std::map<std::string, LocAttrHandler> handlers;
	if (handlers.empty())
	{
		handlers["location"] = &Server::handleLocation;
		handlers["root"] = &Server::handleLocRoot;
		handlers["index"] = &Server::handleLocIndex;
		handlers["limit_except"] = &Server::handleLimitExcept;
		handlers["return"] = &Server::handleLocReturn;
		handlers["autoindex"] = &Server::handleAutoindex;
		handlers["cgi_pass"] = &Server::handleCgiPass;
		handlers["cgi_ext"] = &Server::handleCgiExt;
		handlers["upload_store"] = &Server::handleUploadStore;
		handlers["proxy_pass"] = &Server::handleProxyPass;
	}
	return handlers;
}

void Server::setServerInfo(const std::vector<std::string> &line)
{
	if (line.empty())
		return;

	const std::string &directive = line[0];
	std::map<std::string, AttrHandler>::iterator it = getServerHandlers().find(directive);
	if (it != getServerHandlers().end())
	{
		AttrHandler handler = it->second;
		(this->*handler)(line);
	}
}

void Server::setLocationInfo(const std::vector<std::string> &line, t_location &location, std::string &key)
{
	if (line.empty())
		return;

	const std::string &directive = line[0];
	std::map<std::string, LocAttrHandler>::iterator it = getLocationHandlers().find(directive);
	if (it != getLocationHandlers().end())
	{
		LocAttrHandler handler = it->second;
		(this->*handler)(line, location, key);
	}
}

void Server::handlePort(const std::vector<std::string> &line)
{
	Validator::requireSize(line, 2, "port", ConfigValidator::validateListen(line[1]));
	_port = line[1];
}
void Server::handleServerName(const std::vector<std::string> &line)
{
	Validator::requireSize(line, 2, "server_name", ConfigValidator::validateServerName(line[1]));
	_serverName = line[1];
}
void Server::handleMaxBodySize(const std::vector<std::string> &line)
{
	Validator::requireSize(line, 2, "max_body_size", ConfigValidator::validateSize(line[1]));
	_maxBytes = line[1];
}
void Server::handleErrorPage(const std::vector<std::string> &line)
{
	Validator::requireMinSize(line, 3, "error_page", true);
	const std::string &errorPage = line.back();
	for (size_t i = 1; i < line.size() - 1; ++i)
	{
		Validator::requireMinSize(line, 3, "error_page", ConfigValidator::validateErrorPage(atoi(line[i].c_str()), errorPage));
		_errorPage[line[i]] = errorPage;
	}
}

void Server::handleRoot(const std::vector<std::string> &line)
{
	Validator::requireSize(line, 2, "root", ConfigValidator::validateRoot(line[1]));
	_root = line[1];
}

void Server::handleIndex(const std::vector<std::string> &line)
{
	Validator::requireMinSize(line, 2, "index", true);
	for (size_t i = 1; i < line.size(); ++i)
	{
		Validator::requireMinSize(line, 2, "index", ConfigValidator::validateIndex(line[i]));
		_index.push_back(line[i]);
	}
}

void Server::handleReturn(const std::vector<std::string> &line)
{
	Validator::requireSize(line, 3, "return", ConfigValidator::validateReturn(atoi(line[1].c_str()), line[2]));
	_return.push_back(line[1]);
	_return.push_back(line[2]);
}

void Server::handleLocation(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)loc;
	Validator::requireSize(line, 3, "location", true);
	key = line[1];
}

void Server::handleLocRoot(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireSize(line, 2, "root", ConfigValidator::validateRoot(line[1]));
	loc._root = line[1];
}

void Server::handleLocIndex(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireMinSize(line, 2, "index", true);
	for (size_t i = 1; i < line.size(); ++i)
	{
		Validator::requireMinSize(line, 2, "index", ConfigValidator::validateIndex(line[i]));
		loc._index.push_back(line[i]);
	}
}

void Server::handleLimitExcept(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireMinSize(line, 1, "limit_except", true);
	for (size_t i = 1; i < line.size(); ++i)
	{
		Validator::requireMinSize(line, 1, "limit_except", ConfigValidator::validateMethods(std::vector<std::string>(line.begin() + 1, line.end())));
		loc._limit_except.push_back(line[i]);
	}
}

void Server::handleLocReturn(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireSize(line, 3, "return", ConfigValidator::validateReturn(atoi(line[1].c_str()), line[2]));
	loc._return.push_back(line[1]);
	loc._return.push_back(line[2]);
}

void Server::handleAutoindex(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireSize(line, 2, "autoindex", (line[1] == "on" || line[1] == "off"));
	loc._autoIndex = line[1];
}

void Server::handleCgiPass(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireSize(line, 2, "cgi_pass", ConfigValidator::isAbsolutePath(line[1]));
	loc._cgiPass = line[1];
}

void Server::handleCgiExt(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireSize(line, 2, "cgi_ext", ConfigValidator::validateCgiExt(line[1]));
	loc._cgiExt = line[1];
}

void Server::handleUploadStore(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireSize(line, 2, "upload_store", ConfigValidator::isAbsolutePath(line[1]));
	loc._uploadStore = line[1];
}

void Server::handleProxyPass(const std::vector<std::string> &line, t_location &loc, std::string &key)
{
	(void)key;
	Validator::requireSize(line, 2, "proxy_pass", ConfigValidator::validateUrl(line[1]));
	loc._proxy_pass = line[1];
}

std::ostream &operator<<(std::ostream &os, const Server &s)
{
	os << "*********Server*********\n";
	os << "Port: " << (s.getPort().empty() ? "" : s.getPort()) << std::endl;
	os << "ServerName: " << (s.getServerName().empty() ? "" : s.getServerName()) << std::endl;
	os << "MaxByte: " << (s.getMaxByte().empty() ? "" : s.getMaxByte()) << std::endl;
	os << "Root: " << (s.getRoot().empty() ? "" : s.getRoot()) << std::endl;
	std::string indexStr;
	for (size_t i = 0; i < s.getIndex().size(); i++)
	{
		indexStr += s.getIndex()[i];
		if (i + 1 < s.getIndex().size())
			indexStr += ", ";
	}

	os << "Index: " << indexStr << std::endl;
	const std::map<std::string, std::string> &errors = s.getErrorPage();
	if (!errors.empty())
	{
		for (std::map<std::string, std::string>::const_iterator it = errors.begin(); it != errors.end(); ++it)
		{
			os << "Error page: [" << it->first << "] = " << it->second << std::endl;
		}
	}
	const std::map<std::string, t_location> &locations = s.getLocation();
	if (!locations.empty())
	{
		for (std::map<std::string, t_location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
		{
			os << "Location: [" << it->first << "](" << (it->second._isCgi ? "CGI block)" : "not CGI block)") << std::endl;
			os << "	root: " << it->second._root << std::endl;
			os << "	index: ";
			for (size_t i = 0; i < it->second._index.size(); i++)
				os << it->second._index[i] << " ";
			os << std::endl;
			os << "	limit except: ";
			for (unsigned int i = 0; i < it->second._limit_except.size(); i++)
				os << it->second._limit_except[i] << " ";
			os << std::endl;
			os << "	return: [" << it->second._root[0] << "] = " << it->second._root[1] << std::endl;
			os << "	autoIndex: " << it->second._autoIndex << std::endl;
			os << "	cgiPass: " << it->second._cgiPass << std::endl;
			os << "	cgiExt: " << it->second._cgiExt << std::endl;
			os << "	uploadStore: " << it->second._uploadStore << std::endl;
			os << "	proxyPass: " << it->second._proxy_pass << std::endl;
		}
	}
	os << "************************" << std::endl;
	return os;
}
