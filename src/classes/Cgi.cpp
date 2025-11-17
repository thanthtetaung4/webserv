/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 06:28:13 by lshein            #+#    #+#             */
/*   Updated: 2025/11/13 08:22:48 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"

Cgi::Cgi(const std::string &path, const std::string &interpreter, const std::map<std::string, std::string> &env, const std::string &body) : _path(path), _interpreter(interpreter), _env(env), _body(body) {}

Cgi::~Cgi() {}

char **createEnvArray(const std::map<std::string, std::string> &env)
{
	char **envArray = new char *[env.size() + 1];
	size_t i = 0;
	for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it)
	{
		std::string entry = it->first + "=" + it->second;
		envArray[i] = new char[entry.size() + 1];
		std::copy(entry.begin(), entry.end(), envArray[i]);
		envArray[i][entry.size()] = '\0';
		++i;
	}
	envArray[i] = NULL;
	return envArray;
}

void freeEnvArray(char **env)
{
	for (size_t i = 0; env[i]; ++i)
	{
		delete env[i];
	}
	delete[] env;
}

std::map<std::string, std::string> &createEnvMap(const Request &request)
{
	std::map<std::string, std::string> env;
	
	return env;
}
