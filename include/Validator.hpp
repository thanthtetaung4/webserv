/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validator.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 06:22:40 by lshein            #+#    #+#             */
/*   Updated: 2025/11/17 06:46:08 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include "Server.hpp"
#include "Location.h"

struct Validator
{
	static void requireSize(const std::vector<std::string> &line, size_t expected, const std::string &name, bool(func))
	{
		if (line.size() != expected || !func)
			throw std::runtime_error("Invalid '" + name + "' directive format");
	}
	static void requireMinSize(const std::vector<std::string> &line, size_t min, const std::string &name, bool(func))
	{
		if (line.size() < min || !func)
			throw std::runtime_error("Invalid '" + name + "' directive format");
	}
};

class ConfigValidator
{
public:
	static bool validateListen(const std::string &value);
	static bool validateServerName(const std::string &value);
	static bool validateErrorPage(int code, const std::string &path);
	static bool validateSize(const std::string &value);
	static bool validateLocation(const location &loc, const std::string &key);
	static bool isAbsolutePath(const std::string &path);
	static bool validateMethods(const std::vector<std::string> &methods);
	static bool validateUrl(const std::string &url);
	static bool isValidIp(const std::string &ip);
	static bool isNumber(const std::string &s);
	static bool validateCgiExt(const std::string &ext);
	static bool validateReturn(int code, const std::string &path);
	static bool validateRoot(const std::string &root);
	static bool validateIndex(const std::string &value);
};

#endif
