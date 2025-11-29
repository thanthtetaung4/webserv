/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:39:28 by hthant            #+#    #+#             */
/*   Updated: 2025/11/24 18:33:28 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/Response.hpp"

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

Response::Response(const Response &res)
{
	if (this == &res)
		throw UnableToCreateResponse();
	this->_httpVersion = res._httpVersion;
	this->_statusCode = res._statusCode;
	this->_statusTxt = res._statusTxt;
	this->_headers = res._headers;
	this->_body = res._body;
}

bool	Response::generateError(int errorCode, std::string const errorMsg, std::string const bodyMsg, Server& server){
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

	if (req.getUrlPath().find("/private") == 0 && !req.hasHeader("Authorization"))
		return (generateError(401, "Unauthorized", "401 Unauthorized", server));

	std::ifstream file(path.c_str());
	if (!file.is_open())
		return (generateError(404, "Not Found", "404 Not Found", server));

	if(access(path.c_str(), R_OK) < 0 || !safePath(path))
		return (generateError(403,"Forbidden", "403 Forbidden", server));

	if (req.getMethodType() != "GET" && req.getMethodType() != "POST" && req.getMethodType() != "DELETE")
		return (generateError(405, "Method Not Allowed", "405 Method Not Allowed", server));

	if (req.getMethodType() == "POST" && !req.hasHeader("Content-Length"))
		return (generateError(411, "Required Length", "411 Required Length", server));

	if (req.getBody().size() > size)
		return (generateError(413, "Content Too Large", "413 Content Too Large", server));

	if (req.getUrlPath().size() > 2048)
		return (generateError(414, "URL Too Long", "414 URL Too Long", server));

	if (req.getMethodType() == "POST")
	{
		std::string type = getMimeType(path);
		if (!isSupportedType(type))
			return (generateError(415, "Unsupported Media Type", "415 Unsupported Media Type", server));
	}

	if (req.getUrlPath() == "/teapot")
		return (generateError(418, "I'm a teapot", "418 I'm a teapot", server));

	if (req.getUrlPath().empty() || path.empty())
		return (generateError(500, "Internal Server Error", "500 Internal Server Error", server));

	if (req.getHttpVersion() != "HTTP/1.0" && req.getHttpVersion() != "HTTP/1.1")
		return (generateError(505, "HTTP Version not Supported", "505 HTTP Version Not Supported", server));

	return false;
}

Response::Response(void)
{
	throw UnableToCreateResponse();
}

Response::Response(const std::string rawRes) {
	(void)rawRes;
}


Response::Response(unsigned int errorCode){
	this->_statusCode =  errorCode;
}

Response::Response(const Request &req, Server &server)
{
	this->_httpVersion = req.getHttpVersion();

	size_t size;
	std::stringstream ss(server.getMaxByte());
	ss >> size;
	//find req.urlPath in server locations

	std::map<std::string, t_location> locations = server.getLocation();
	t_location* loc = searchMapLongestMatch(locations, req.getUrlPath());

	std::string	path = "";
	// path building
	if (loc) {
		if (!loc->_root.empty())
			path = loc->_root + req.getUrlPath();
		else if (!server.getRoot().empty())
			path = server.getRoot() + req.getUrlPath();
	}

	//return error here
	std::cout << "The real path " << path << std::endl;

	// check config error here
	//
	std::cout << "-----------------------------SERVER TEST----------------------" << std::endl;
	std::cout << server << std::endl;
	std::cout << "--------------------------------------------------------------------------" << std::endl;
	//
	std::cout << "Requested Path: " << path << std::endl;
	if (!checkHttpError(req, size, path, server))
	{
		std::ifstream file(path.c_str(), std::ios::binary);
		std::ostringstream os;
		os << file.rdbuf();
		this->_body = os.str();
		this->_statusCode = 200;
		this->_statusTxt = "OK";
		this->_headers["Content-Type"] = getMimeType(path);
		this->_headers["Content-Length"] = intToString(this->_body.size());
		this->_headers["Connection"] = "close";
	}
	if (this->_body.empty())
	{
		std::cout << "Body is empty" << std::endl;
	}
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
