/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/13 06:28:13 by lshein            #+#    #+#             */
/*   Updated: 2025/12/30 03:50:44 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Cgi.hpp"
#include "../../include/Response.hpp"
#include <signal.h>
#include <ctime>

Cgi::Cgi(const std::string &path, const std::string &interpreter, const std::map<std::string, std::string> &env, const std::string &body)
	: _path(path), _interpreter(interpreter), _env(env), _body(body), _pid(-1), _isComplete(false), _timeout(30), _startTime(0)
{
	_outPipe[0] = -1;
	_outPipe[1] = -1;
	_inPipe[0] = -1;
	_inPipe[1] = -1;
}

Cgi::Cgi(const Request &request, const Server &server)
	: _pid(-1), _isComplete(false), _timeout(30), _startTime(0)
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
	env["QUERY_STRING"] = request.getQueryString();

	_path = request.getFinalPath();
	_interpreter = request.getIt()->second._cgiPass;
	_env = env;
	_body = request.getBody();

	_outPipe[0] = -1;
	_outPipe[1] = -1;
	_inPipe[0] = -1;
	_inPipe[1] = -1;
}

Cgi::~Cgi()
{
	if (_outPipe[0] != -1) close(_outPipe[0]);
	if (_outPipe[1] != -1) close(_outPipe[1]);
	if (_inPipe[0] != -1) close(_inPipe[0]);
	if (_inPipe[1] != -1) close(_inPipe[1]);
	if (_pid > 0) {
		kill(_pid, SIGTERM);
		waitpid(_pid, NULL, WNOHANG);
	}
}

void Cgi::executeAsync()
{
	if (pipe(_inPipe) < 0 || pipe(_outPipe) < 0)
		throw std::runtime_error("Pipe creation failed");

	// Record start time for timeout tracking
	_startTime = time(NULL);

	_pid = fork();
	if (_pid < 0)
		throw std::runtime_error("Fork failed");

	if (_pid == 0)
	{
		// CHILD PROCESS
		dup2(_inPipe[0], STDIN_FILENO);
		dup2(_outPipe[1], STDOUT_FILENO);
		close(_inPipe[0]);
		close(_inPipe[1]);
		close(_outPipe[0]);
		close(_outPipe[1]);

		char **envArray = createEnvArray(_env);

		char *argv[3];
		argv[0] = const_cast<char *>(_interpreter.c_str());
		argv[1] = const_cast<char *>(_path.c_str());
		argv[2] = NULL;

		execve(argv[0], argv, envArray);

		std::cerr << "child execve failed" << std::endl;
		freeEnvArray(envArray);
		std::exit(1);
	}

	// PARENT PROCESS
	close(_inPipe[0]);
	close(_outPipe[1]);

	// Write body to CGI stdin (non-blocking)
	if (!_body.empty())
	{
		ssize_t written = write(_inPipe[1], _body.c_str(), _body.size());
		if (written < 0)
			std::cerr << "Failed to write to CGI stdin" << std::endl;
	}
	close(_inPipe[1]);
	_inPipe[1] = -1;

	// Make output pipe non-blocking
	int flags = fcntl(_outPipe[0], F_GETFL, 0);
	fcntl(_outPipe[0], F_SETFL, flags | O_NONBLOCK);

	// std::cout << "CGI process started with PID: " << _pid << std::endl;
}

bool Cgi::readOutput()
{
	if (_outPipe[0] == -1 || _isComplete)
		return false;

	// Check for timeout
	if (hasTimedOut())
	{
		// std::cout << "CGI timeout exceeded (" << _timeout << "s)" << std::endl;
		// Kill the process
		if (_pid > 0)
			kill(_pid, SIGKILL);
		// Discard all data - don't read from pipe, clear any previous buffer
		_output.clear();
		close(_outPipe[0]);
		_outPipe[0] = -1;
		_isComplete = true;
		return true;
	}

	char buffer[1024];
	ssize_t bytesRead;

	// Read available data (non-blocking). This function is intended to be
	// driven by the server's epoll loop (or called periodically). We avoid
	// checking errno here so this code stays epoll-friendly — when epoll
	// signals readability, read() should succeed; if read() returns -1
	// (e.g. EAGAIN) we simply don't treat it as a fatal error here.
	while ((bytesRead = read(_outPipe[0], buffer, sizeof(buffer))) > 0)
		_output.append(buffer, bytesRead);

	// If read returned a negative value, don't mark as complete here.
	// Instead rely on the waitpid check below to detect child termination.
	// This avoids direct errno comparisons so the code works with epoll.

	// Check if process has finished
	int status;
	pid_t result = waitpid(_pid, &status, WNOHANG);
	if (result == _pid)
	{
		// Process finished, read any remaining data
		while ((bytesRead = read(_outPipe[0], buffer, sizeof(buffer))) > 0)
			_output.append(buffer, bytesRead);

		close(_outPipe[0]);
		_outPipe[0] = -1;
		_isComplete = true;
		// std::cout << "CGI process completed" << std::endl;
		return true;
	}

	return false;  // Still running
}

bool Cgi::isComplete() const
{
	return _isComplete;
}

std::string Cgi::getOutput() const
{
	return _output;
}

int Cgi::getOutputFd() const
{
	return _outPipe[0];
}

int Cgi::getTimeout() const
{
	return _timeout;
}

int Cgi::getStartTime() const
{
	return _startTime;
}

bool Cgi::hasTimedOut() const
{
	if (_startTime == 0)
		return false;

	int elapsedTime = time(NULL) - _startTime;
	return elapsedTime > _timeout;
}

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
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);

		char **envArray = createEnvArray(_env);

		char *argv[3];
		argv[0] = const_cast<char *>(_interpreter.c_str());
		argv[1] = const_cast<char *>(_path.c_str());
		argv[2] = NULL;

		execve(argv[0], argv, envArray);

		std::cerr << "execve child failed: " << std::endl;
		freeEnvArray(envArray);
		std::exit(1);
	}

	// PARENT
	close(inPipe[0]);
	close(outPipe[1]);

	if (!_body.empty())
		write(inPipe[1], _body.c_str(), _body.size());
	close(inPipe[1]);
	// std::cout << "CGI body sent " << _body << std::endl;
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

	// Find the end of the CGI header block (handle both \r\n\r\n and \n\n)
	size_t headerEnd = output.find("\r\n\r\n");
	size_t bodyStart = 0;
	std::string delimiter = "\r\n";

	if (headerEnd == std::string::npos)
	{
		// Try Unix line endings (\n\n)
		headerEnd = output.find("\n\n");
		if (headerEnd == std::string::npos)
		{
			// No header delimiter found → treat everything as body
			result.body = output;
			return result;
		}
		bodyStart = headerEnd + 2; // skip "\n\n"
		delimiter = "\n";
	}
	else
	{
		bodyStart = headerEnd + 4; // skip "\r\n\r\n"
	}

	std::string headerBlock = output.substr(0, headerEnd);
	result.body = output.substr(bodyStart);

	size_t start = 0;
	while (start < headerBlock.size())
	{
		size_t end = headerBlock.find(delimiter, start);
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
		start = end + delimiter.size(); // skip delimiter
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
