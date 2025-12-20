/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 06:20:30 by lshein            #+#    #+#             */
/*   Updated: 2025/12/20 22:54:03 by taung            ###   ########.fr       */
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
#include <errno.h>
#include "Request.hpp"

struct CgiResult
{
	std::map<std::string, std::string> headers;
	std::string body;
};

class Cgi
{
private:
	std::string _path;
	std::string _interpreter;
	std::map<std::string, std::string> _env;
	std::string _body;
	pid_t _pid;
	int _outPipe[2];
	int _inPipe[2];
	std::string _output;
	bool _isComplete;

public:
	Cgi(const std::string &path, const std::string &interpreter, const std::map<std::string, std::string> &env, const std::string &body);
	Cgi(const Request &request, const Server &server);
	~Cgi();

	// Non-blocking methods
	void executeAsync();
	bool readOutput();
	bool isComplete() const;
	std::string getOutput() const;
	int getOutputFd() const;

	// Legacy blocking method (optional)
	std::string execute();
	CgiResult parseCgiHeaders(const std::string &output);
};

char **createEnvArray(const std::map<std::string, std::string> &env);
void freeEnvArray(char **env);

#endif
