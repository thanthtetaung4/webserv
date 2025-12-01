/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 06:28:13 by lshein            #+#    #+#             */
/*   Updated: 2025/11/26 13:49:04 by lshein           ###   ########.fr       */
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
	std::map<std::string, std::string> env;
	env["REQUEST_METHOD"] = request.getMethodType();
	env["SCRIPT_FILENAME"] = request.getFinalPath();
	env["CONTENT_LENGTH"] = intToString(request.getBody().size());
	env["CONTENT_TYPE"] = contentType;
	env["SERVER_NAME"] = server.getServerName().empty() ? "" : server.getServerName();
	env["SERVER_PORT"] = server.getPort();
	env["SERVER_PROTOCOL"] = request.getHttpVersion();
	env["SERVER_SOFTWARE"] = "webserv/1.0";
	// std::cout << "env: " << std::endl;
	// for (std::map<std::string, std::string>::const_iterator it1 = env.begin(); it1 != env.end(); ++it1)
	// {
	// 	std::cout << "[" << it1->first << "] = " << it1->second << std::endl;
	// }
	_path = request.getFinalPath();
	_interpreter = request.getIt()->second._cgiPass;
	_env = env;
	_body = request.getBody();
}

Cgi::~Cgi() {}

std::string Cgi::execute()
{
	int inPipe[2];
	int outPipe[2];
	if (pipe(inPipe) < 0 || pipe(outPipe) < 0)
		return "Status: 500 Internal Server Error\r\n\r\nPipe creation failed";

	pid_t pid = fork();
	if (pid < 0)
		return "Status: 500 Internal Server Error\r\n\r\nFork failed";

	if (pid == 0)
	{
		// CHILD
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		close(inPipe[1]);
		close(outPipe[0]);

		char **envArray = createEnvArray(_env);

		char *argv[3];
		argv[0] = const_cast<char *>(_interpreter.c_str());
		argv[1] = const_cast<char *>(_path.c_str());
		argv[2] = NULL;

		execve(argv[0], argv, envArray);

		perror("execve");
		freeEnvArray(envArray);
		exit(1);
	}

	// PARENT
	close(inPipe[0]);
	close(outPipe[1]);

	if (!_body.empty())
		write(inPipe[1], _body.c_str(), _body.size());
	close(inPipe[1]);

	std::string output;
	char buffer[1024];
	ssize_t bytesRead;
	while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0)
		output.append(buffer, bytesRead);

	close(outPipe[0]);
	waitpid(pid, NULL, 0);

	return output;
}

CgiResult Cgi::parseCgiHeaders(const std::string &output)
{
	CgiResult result;

	// Find the end of the CGI header block
	size_t headerEnd = output.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
	{
		// No header delimiter found → treat everything as body
		result.body = output;
		return result;
	}

	std::string headerBlock = output.substr(0, headerEnd);
	result.body = output.substr(headerEnd + 4); // skip "\r\n\r\n"

	size_t start = 0;
	while (start < headerBlock.size())
	{
		size_t end = headerBlock.find("\r\n", start);
		if (end == std::string::npos)
			end = headerBlock.size();

		std::string line = headerBlock.substr(start, end - start);

		// Parse "Key: Value"
		size_t colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
				value.erase(0, 1);

			result.headers[key] = value;
		}
		start = end + 2; // skip "\r\n"
	}

	return result;
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
