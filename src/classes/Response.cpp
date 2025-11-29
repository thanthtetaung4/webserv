/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:39:28 by hthant            #+#    #+#             */
/*   Updated: 2025/11/29 11:20:05 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Response.hpp"

// std::string intToString(size_t n) {
// 	std::ostringstream ss;
// 	ss << n;
// 	return ss.str();
// }

Response::Response(Request &req, Server &server)
{
	size_t size;
	std::string res;
	std::string path = req.getFinalPath();
	this->_httpVersion = req.getHttpVersion();
	std::stringstream ss(server.getMaxByte());
	ss >> size;
	size_t i;
	t_location loc = req.getIt()->second;

	if (isDirectory(req.getFinalPath()))
	{
		if (!loc._proxy_pass.empty())
		{
			// handle proxypass
		}
		else if (!loc._index.empty())
		{
			for (i = 0; i < loc._index.size(); i++)
			{
				if (isRegularFile(req.getFinalPath() + "/" + loc._index[i]))
				{
					path = req.getFinalPath() + "/" + loc._index[i];
					break;
				}
			}
			if (i == loc._index.size() && loc._autoIndex == "on")
				handleAutoIndex(req.getPath(), req.getFinalPath());
			else
			{
				if (loc._isCgi && path.substr(path.size() - loc._cgiExt.size()) == loc._cgiExt)
				{
					req.setFinalPath(path);
					handleCGI(req, server);
				}
				else
				{
					std::cout << "Requested Path: " << path << std::endl;
					if (!checkHttpError(req, size, path, server))
						serveFile(path);
					if (this->_body.empty())
					{
						std::cout << "Body is empty" << std::endl;
					}
				}
			}
		}
		else if ((loc._index.empty()) && loc._autoIndex == "on")
			handleAutoIndex(req.getPath(), req.getFinalPath());
	}
	else if (isRegularFile(req.getFinalPath()) && loc._isCgi)
		handleCGI(req, server);
	else
	{
		std::cout << "Requested Path: " << path << std::endl;
		if (!checkHttpError(req, size, path, server))
			serveFile(path);
		if (this->_body.empty())
		{
			std::cout << "Body is empty" << std::endl;
		}
	}
}

void Response::handleRedirect(const std::string &redirUrlPath)
{
	this->_statusCode = 302;
	this->_statusTxt = "Found";
	this->_headers["Location"] = redirUrlPath;
	this->_headers["Content-Type"] = "text/html";
	this->_headers["Content-Length"] = "0";
}

void Response::handleAutoIndex(const std::string &urlPath, const std::string &fullPath)
{
	if (urlPath.at(urlPath.size() - 1) != '/')
		return handleRedirect(urlPath + "/");
	// Check access
	if (access(fullPath.c_str(), R_OK) == -1)
	{
		this->_statusCode = 403;
		this->_statusTxt = "Forbidden";
		this->_body = "";
		return;
	}

	// Try opening directory
	DIR *dir = opendir(fullPath.c_str());
	if (!dir)
	{
		this->_statusCode = 404;
		this->_statusTxt = "Not Found";
		this->_body = "";
		return;
	}

	// Build HTML body
	std::string body;
	body += "<html><head><title>Index of " + urlPath + "</title></head><body>";
	body += "<h1>Index of " + urlPath + "</h1><hr><pre>";

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;

		if (name == "." || name == "..")
			continue;

		std::string itemFullPath = fullPath + "/" + name;

		struct stat st;
		if (stat(itemFullPath.c_str(), &st) == -1)
			continue;

		body += "<a href=\"" + urlPath;
		body += name;

		if (S_ISDIR(st.st_mode))
			body += "/";

		body += "\">" + name + "</a>\n";
	}

	body += "</pre><hr></body></html>";
	closedir(dir);

	// Set response
	this->_statusCode = 200;
	this->_statusTxt = "OK";
	this->_body = body;
	this->_headers["Content-Type"] = "text/html";
	this->_headers["Content-Length"] = intToString(body.size());
	this->_headers["Connection"] = "close";
}

void Response::handleCGI(const Request &req, const Server &server)
{
	Cgi cgi(req, server);
	std::string output = cgi.execute();
	CgiResult parsed = cgi.parseCgiHeaders(output);
	std::string contentType;
	if (parsed.headers.count("Content-Type"))
		contentType = parsed.headers["Content-Type"];
	else
		contentType = "text/html"; // fallback (required by spec)

	// 4. Body from CGI output
	this->_body = parsed.body;

	// 5. HTTP status code (use CGI header or default to 200)
	if (parsed.headers.count("Status"))
	{
		// Example: "Status: 404 Not Found"
		std::string status = parsed.headers["Status"];
		size_t space = status.find(' ');
		if (space != std::string::npos)
		{
			this->_statusCode = atoi(status.substr(0, space).c_str());
			this->_statusTxt = status.substr(space + 1);
		}
		else
		{
			this->_statusCode = 200;
			this->_statusTxt = "OK";
		}
	}
	else
	{
		this->_statusCode = 200;
		this->_statusTxt = "OK";
	}

	// 6. Build HTTP headers for the final response
	this->_headers.clear();
	this->_headers["Content-Type"] = contentType;
	this->_headers["Content-Length"] = intToString(this->_body.size());
	this->_headers["Connection"] = "close";
}

std::string Response::handleReverseProxy(const Request &req)
{
	t_proxyPass pp = parseProxyPass(req.getIt()->second._proxy_pass);

	std::cout << "Proxying to " << pp.host << ":" << pp.port << pp.path << std::endl;

	// Create socket with the correct port
	Socket proxySocket(std::atol(pp.port.c_str()));
	proxySocket.openSock(); // Only create the socket, don't bind/listen

	std::cout << "Proxy socket: " << proxySocket.getServerFd() << std::endl;

	// Setup address structure for the proxy server
	struct sockaddr_in server_addr;
	// Initialize to zero without memset
	for (size_t i = 0; i < sizeof(server_addr); i++)
	{
		((char *)&server_addr)[i] = 0;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(std::atol(pp.port.c_str()));

	// Convert IP address from string to binary using inet_addr
	server_addr.sin_addr.s_addr = inet_addr(pp.host.c_str());
	if (server_addr.sin_addr.s_addr == INADDR_NONE)
	{
		throw std::runtime_error("Invalid proxy address");
	}

	// Connect to proxy server
	if (connect(proxySocket.getServerFd(), (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		throw std::runtime_error("Unable to connect to proxy server");
	}

	std::cout << "Connected successfully!" << std::endl;

	// Build the proxy request
	std::string proxyRequest = req.getMethodType() + " " + pp.path + " " + req.getHttpVersion() + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin();
		 it != req.getHeaders().end(); ++it)
	{
		proxyRequest += it->first + ": " + it->second + "\r\n";
	}
	proxyRequest += "\r\n" + req.getBody();

	std::cout << "Proxy Request:\n"
			  << proxyRequest << std::endl;

	// Send the request to proxy server
	ssize_t sent = send(proxySocket.getServerFd(), proxyRequest.c_str(), proxyRequest.size(), 0);
	if (sent < 0)
	{
		throw std::runtime_error("Unable to send request to proxy server");
	}

	std::cout << "Request sent successfully (" << sent << " bytes)" << std::endl;

	// Receive the response from proxy server
	char buffer[4096];
	// Initialize buffer to zero without memset
	for (size_t i = 0; i < sizeof(buffer); i++)
	{
		buffer[i] = 0;
	}

	ssize_t bytes_received = recv(proxySocket.getServerFd(), buffer, sizeof(buffer) - 1, 0);

	if (bytes_received < 0)
	{
		throw std::runtime_error("Error receiving response from proxy server");
	}

	if (bytes_received == 0)
	{
		return ("");
	}

	buffer[bytes_received] = '\0'; // Null-terminate the received data

	std::cout << "Received " << bytes_received << " bytes from proxy" << std::endl;

	return std::string(buffer);
}

bool safePath(std::string const &path)
{
	if (path.find("..") == std::string::npos)
		return true;
	return false;
}

std::string getMimeType(const std::string &path)
{
	size_t deliPos = path.rfind('.');

	std::string type = path.substr(deliPos + 1);

	if (type == "html")
		return "text/html";
	else if (type == "css")
		return "text/css";
	else if (type == "js")
		return "application/javascript";
	else if (type == "png")
		return "image/png";
	else if (type == "jpg" || type == "jpeg")
		return "image/jpeg";
	else if (type == "ico")
		return "image/x-icon";
	else if (deliPos == std::string::npos || deliPos == path.length() - 1)
	{
		return "text/plain";
	}
	else
		return "text/plain";
}

std::string readFile(std::string &path)
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool Response::generateError(int errorCode, std::string const errorMsg, std::string const bodyMsg, Server &server)
{
	this->_statusCode = errorCode;
	std::map<std::string, std::string> errorPages = server.getErrorPage();
	std::map<std::string, std::string>::iterator it = errorPages.find(intToString(this->_statusCode));
	if (it != errorPages.end())
	{
		std::cout << "Custom error page found for " << this->_statusCode << ": " << it->second << std::endl;
		this->_body = readFile(it->second);
		std::cout << "Cuastom page path: " << it->second << std::endl;
		if (this->_body.empty())
		{
			std::cout << "Failed to read custom error page: " << it->second << std::endl;
			this->_body = "<h1>" + bodyMsg + "</h1>";
		}
	}
	else
	{
		std::cout << "No custom error page found for " << this->_statusCode << ", using default message." << std::endl;
		this->_body = "<h1>" + bodyMsg + "</h1>";
	}
	this->_statusTxt = errorMsg;
	this->_headers["Content-Type"] = "text/html";
	this->_headers["Content-Length"] = intToString(this->_body.size());
	this->_headers["Connection"] = "close";
	return true;
}

std::string Response::toStr() const
{
	std::ostringstream response;
	response << "HTTP/1.1 " << _statusCode << " " << _statusTxt << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << "\r\n";
	}
	response << "\r\n";
	response << _body;
	return response.str();
}

bool Request::hasHeader(const std::string &key) const
{
	return _headers.find(key) != _headers.end();
}

bool isSupportedType(const std::string &type)
{
	return (type == "text/html" ||
			type == "text/plain" ||
			type == "text/css" ||
			type == "image/png" ||
			type == "image/jpeg" ||
			type == "image/jpg" ||
			type == "image/x-icon" ||
			type == "application/json");
}

bool Response::checkHttpError(const Request &req, size_t size, std::string path, Server &server)
{
	if (req.getMethodType().empty())
		return (generateError(400, "Bad Request", "400 Bad Request", server));

	if (req.getPath().find("/private") == 0 && !req.hasHeader("Authorization"))
		return (generateError(401, "Unauthorized", "401 Unauthorized", server));

	std::ifstream file(path.c_str());
	if (!file.is_open())
		return (generateError(404, "Not Found", "404 Not Found", server));

	if (access(path.c_str(), R_OK) < 0 || !safePath(path))
		return (generateError(403, "Forbidden", "403 Forbidden", server));

	if (req.getMethodType() != "GET" && req.getMethodType() != "POST" && req.getMethodType() != "DELETE")
		return (generateError(405, "Method Not Allowed", "405 Method Not Allowed", server));

	if (req.getMethodType() == "POST" && !req.hasHeader("Content-Length"))
		return (generateError(411, "Required Length", "411 Required Length", server));

	if (req.getBody().size() > size)
		return (generateError(413, "Content Too Large", "413 Content Too Large", server));

	if (req.getPath().size() > 2048)
		return (generateError(414, "URL Too Long", "414 URL Too Long", server));

	if (req.getMethodType() == "POST")
	{
		std::string type = getMimeType(path);
		if (!isSupportedType(type))
			return (generateError(415, "Unsupported Media Type", "415 Unsupported Media Type", server));
	}

	if (req.getPath() == "/teapot")
		return (generateError(418, "I'm a teapot", "418 I'm a teapot", server));

	if (req.getPath().empty() || path.empty())
		return (generateError(500, "Internal Server Error", "500 Internal Server Error", server));

	if (req.getHttpVersion() != "HTTP/1.0" && req.getHttpVersion() != "HTTP/1.1")
		return (generateError(505, "HTTP Version not Supported", "505 HTTP Version Not Supported", server));

	return false;
}

Response::Response(unsigned int errorCode)
{
	this->_statusCode = errorCode;
}

void Response::serveFile(const std::string &filePath)
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	std::ostringstream os;
	os << file.rdbuf();
	this->_body = os.str();
	this->_statusCode = 200;
	this->_statusTxt = "OK";
	this->_headers["Content-Type"] = getMimeType(filePath);
	this->_headers["Content-Length"] = intToString(this->_body.size());
	this->_headers["Connection"] = "close";
}

std::string Response::getHttpVersion() const
{
	return this->_httpVersion;
}

int Response::getStatusCode() const
{
	return this->_statusCode;
}

std::string Response::getStatusTxt() const
{
	return this->_statusTxt;
}

const std::map<std::string, std::string> &Response::getHeaders() const
{
	return this->_headers;
}

std::string Response::getBody() const
{
	return this->_body;
}

std::ostream &operator<<(std::ostream &os, const Response &res)
{
	os << "Http version: " << res.getHttpVersion() << std::endl;
	os << "Error Code: " << res.getStatusCode() << std::endl;
	os << "Error Msg: " << res.getStatusTxt() << std::endl;

	os << "Headers:" << std::endl;
	if (res.getHeaders().empty())
	{
		os << "  (none)" << std::endl;
	}
	else
	{
		std::map<std::string, std::string>::const_iterator it;
		for (it = res.getHeaders().begin(); it != res.getHeaders().end(); it++)
		{
			os << "  [" << it->first << "] = " << it->second << std::endl;
		}
		std::cout << "----------------HEADER END-----------------" << std::endl;
	}
	os << "Body: " << res.getBody() << std::endl;
	return os;
}
