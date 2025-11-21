/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 06:28:13 by lshein            #+#    #+#             */
/*   Updated: 2025/11/17 13:51:18 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Cgi.hpp"
#include "../../include/Response.hpp"

Cgi::Cgi(const std::string &path, const std::string &interpreter, const std::map<std::string, std::string> &env, const std::string &body) : _path(path), _interpreter(interpreter), _env(env), _body(body) {}

Cgi::Cgi(const Request &request, const Server &server)
{
	std::string contentType = "";
	std::map<std::string, std::string> headers = request.getHeaders();
	std::map<std::string, std::string>::const_iterator it = headers.find("Accept");
	if (it != headers.end())
	{
		std::string value = it->second;
		std::size_t pos = value.find(',');
		contentType = it->second.substr(0, pos);
	}
	std::string filename = "";
	std::map<std::string, t_location> paths = server.getLocation();
	std::map<std::string, t_location>::const_iterator it1 = paths.find(request.getUrlPath());
	if (it1 != paths.end())
		filename = it1->second._root.empty() ? (server.getRoot() + request.getUrlPath()) : (it1->second._root + request.getUrlPath());
	std::map<std::string, std::string> env;
	env["REQUEST_METHOD"] = request.getMethodType();
	env["SCRIPT_FILENAME"] = filename;
	env["CONTENT_LENGTH"] = intToString(request.getBody().size());
	env["CONTENT_TYPE"] = contentType;
	env["SERVER_NAME"] = server.getServerName().empty() ? "" : server.getServerName();
	env["SERVER_PORT"] = server.getPort();
	env["SERVER_PROTOCOL"] = request.getHttpVersion();
	env["SERVER_SOFTWARE"] = "webserv/1.0";
	std::cout << "env: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it2 = env.begin(); it2 != env.end(); ++it2)
	{
		std::cout << "[" << it2->first << "] = " << it2->second << std::endl;
	}
	std::string fullpath = filename + "/test.py";
	Cgi cgi(fullpath, it1->second._cgiPass, env, request.getBody());
}

Cgi::~Cgi() {}

void Cgi::execute()
{
}

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
