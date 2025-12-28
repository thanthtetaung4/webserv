/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validator.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 06:45:57 by lshein            #+#    #+#             */
/*   Updated: 2025/11/30 12:18:07 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Validator.hpp"

bool ConfigValidator::validateListen(const std::string &value)
{
	// size_t colon = value.find(':');
	// if (colon == std::string::npos)
	// 	return false;

	// std::string ip = value.substr(0, colon);
	// std::string portStr = value.substr(colon + 1);

	// if (!isValidIp(ip) || !isNumber(portStr))
	// 	return false;

	long port = std::strtol(value.c_str(), NULL, 10);
	if (port < 1 || port > 65535)
		return false;

	return true;
}

bool ConfigValidator::isValidIp(const std::string &ip)
{
	int dots = 0;
	std::string part;
	for (size_t i = 0; i <= ip.size(); ++i)
	{
		if (i == ip.size() || ip[i] == '.')
		{
			if (part.empty())
				return false;
			int num = std::atoi(part.c_str());
			if (num < 0 || num > 255)
				return false;
			part.clear();
			dots++;
		}
		else if (ip[i] >= '0' && ip[i] <= '9')
		{
			part += ip[i];
		}
		else
			return false;
	}
	return dots == 4;
}

bool ConfigValidator::validateServerName(const std::string &value)
{
	if (value.empty())
		return false;
	for (size_t i = 0; i < value.size(); ++i)
	{
		char c = value[i];
		if (!(std::isalnum(c) || c == '.' || c == '*' || c == '-' || c == '_' || c == ' '))
			return false;
	}
	return true;
}

bool ConfigValidator::validateErrorPage(int code, const std::string &path)
{
	if (code < 300 || code > 599)
		return false;
	return isAbsolutePath(path);
}

bool ConfigValidator::validateSize(const std::string &value)
{
	if (value.empty())
		return false;

	for (size_t i = 0; i < value.size(); ++i)
	{
		char c = value[i];
		if (!isdigit(c) && !(i == value.size() - 1 &&
							 (c == 'k' || c == 'K' || c == 'm' || c == 'M' || c == 'g' || c == 'G')))
			return false;
	}
	return true;
}

bool ConfigValidator::validateRoot(const std::string &root)
{
	if (root.empty() || !isAbsolutePath(root))
		return false;
	return true;
}

bool isValidIndexToken(const std::string &s)
{
	if (s.empty())
		return false;
	const std::string forbidden = " \t\n\r;";
	for (size_t i = 0; i < s.length(); ++i)
	{
		if (forbidden.find(s[i]) != std::string::npos)
			return false;
	}
	if (s.find('/') != std::string::npos)
		return false;
	return true;
}

bool ConfigValidator::validateIndex(const std::string &value)
{
	if (value.empty())
		return false;
	if (!isValidIndexToken(value))
		return false;
	return true;
}

bool ConfigValidator::validateLocation(const location &loc, const std::string &key)
{
	bool ok = true;

	if (key.empty() || key[0] != '/')
	{
		std::cerr << "  [x] Invalid path: " << key << std::endl;
		ok = false;
	}

	if (!loc._root.empty() && !isAbsolutePath(loc._root))
	{
		std::cerr << "  [x] Root must be absolute: " << loc._root << std::endl;
		ok = false;
	}

	if (!loc._index.empty())
	{
		for (size_t i = 0; i < loc._index.size(); i++)
		{
			if (!validateIndex(loc._index[i]))
			{
				std::cerr << "  [x] Invalid index value: " << loc._index[i] << std::endl;
				ok = false;
			}
		}
	}
	if (!loc._autoIndex.empty() && (loc._autoIndex != "on" && loc._autoIndex != "off"))
	{
		std::cerr << "  [x] Invalid autoindex value: " << loc._autoIndex << std::endl;
		ok = false;
	}

	if (!loc._proxy_pass.empty() && !validateUrl(loc._proxy_pass))
	{
		std::cerr << "  [x] Invalid proxy_pass URL: " << loc._proxy_pass << std::endl;
		ok = false;
	}

	if (!loc._uploadStore.empty() && !isAbsolutePath(loc._uploadStore))
	{
		std::cerr << "  [x] upload_store must be absolute: " << loc._uploadStore << std::endl;
		ok = false;
	}

	if (!loc._cgiPass.empty() && !isAbsolutePath(loc._cgiPass))
	{
		std::cerr << "  [x] cgi_pass must be absolute: " << loc._cgiPass << std::endl;
		ok = false;
	}

	if (!loc._cgiExt.empty() && loc._cgiExt[0] != '.')
	{
		std::cerr << "  [x] cgi_ext must start with '.'" << std::endl;
		ok = false;
	}

	if (!loc._return.empty() && !validateReturn(atol(loc._return[0].c_str()), loc._return[1]))
		ok = false;

	if (!validateMethods(loc._limit_except))
		ok = false;

	return ok;
}

bool ConfigValidator::isAbsolutePath(const std::string &path)
{
	return !path.empty() && path[0] == '/';
}

bool ConfigValidator::isNumber(const std::string &s)
{
	if (s.empty())
		return false;
	for (size_t i = 0; i < s.size(); ++i)
		if (!isdigit(s[i]))
			return false;
	return true;
}

bool ConfigValidator::validateMethods(const std::vector<std::string> &methods)
{
	static const char *valid[] = {
		"GET", "POST", "DELETE"};
	for (size_t i = 0; i < methods.size(); ++i)
	{
		bool found = false;
		for (size_t j = 0; j < sizeof(valid) / sizeof(valid[0]); ++j)
		{
			if (methods[i] == valid[j])
				found = true;
		}
		if (!found)
		{
			std::cerr << "  [x] Invalid HTTP method: " << methods[i] << std::endl;
			return false;
		}
	}
	return true;
}

bool ConfigValidator::validateUrl(const std::string &url)
{
	if (url.size() < 8)
		return false;
	if (url.compare(0, 7, "http://") != 0 &&
		url.compare(0, 8, "https://") != 0)
		return false;
	return true;
}

bool ConfigValidator::validateCgiExt(const std::string &ext)
{
	if (ext.empty() || ext[0] != '.')
		return false;
	for (size_t i = 1; i < ext.size(); ++i)
	{
		char c = ext[i];
		if (!std::isalnum(c))
			return false;
	}
	return true;
}

bool ConfigValidator::validateReturn(int code, const std::string &path)
{
	if (code < 300 || code > 599)
		return false;
	return (isAbsolutePath(path) || validateUrl(path));
}
