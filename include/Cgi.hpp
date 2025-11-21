/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 06:20:30 by lshein            #+#    #+#             */
/*   Updated: 2025/11/17 13:50:44 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#include <iostream>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>
#include "Request.hpp"

class Cgi
{
private:
	std::string _path;
	std::string _interpreter;
	std::map<std::string, std::string> _env;
	std::string _body;

public:
	Cgi(const std::string &path, const std::string &interpreter, const std::map<std::string, std::string> &env, const std::string &body);
	Cgi(const Request &request, const Server &server);
	~Cgi();
	void execute();
};
char **createEnvArray(const std::map<std::string, std::string> &env);
void freeEnvArray(char **env);
// std::map<std::string, std::string> createEnvMap(const Request &request, const Server &server);
#endif
