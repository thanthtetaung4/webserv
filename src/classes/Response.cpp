/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:39:28 by hthant            #+#    #+#             */
/*   Updated: 2025/10/16 13:13:17 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/Response.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <locale>
#include <ostream>
#include <pthread.h>
#include <sstream>
#include <string>
# include <map>
# include <unistd.h>

static std::string intToString(size_t n); 

bool safePath(std::string const& path){
	if(path.find("..") == std::string::npos)
		return true;
	return false;
}

std::string getMimeType(const std::string& path) {
	size_t deliPos = path.rfind('.');

	if (deliPos == std::string::npos || deliPos == path.length() - 1) {
		return "text/plain";
	}
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
	else
		return "text/plain";
}

std::string readFile(std::string& path) {
	path.erase(0,1);
	std::ifstream file(path.c_str());
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool generateError(int errorCode, std::string const errorMsg, Response &res , std::string const bodyMsg, Server& server){
	res._statusCode = errorCode;
	std::map<std::string, std::string> errorPages = server.getErrorPage();
	std::map<std::string, std::string>::iterator it = errorPages.find(intToString(res._statusCode));
    	if (it != errorPages.end())
	{
        	res._statusTxt = errorMsg;
        	res._body = readFile(it->second); 
        	return true;
	}
	res._statusTxt = errorMsg;
	res._body = "<h1>" + bodyMsg + "</h1>";	
	return true;
}

std::string Response::toStr() const {
	std::ostringstream response;
	response << "HTTP/1.1 " << _statusCode << " " << _statusTxt << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
	}
	response << "\r\n";
	response << _body;
	return response.str();
}

static std::string intToString(size_t n) {
	std::ostringstream ss;
	ss << n;
	return ss.str();
}

std::ostream& operator<<(std::ostream& os , const Response& res)
{
	os << "Http version: " << res._httpVersion << std::endl;
	os << "Error Code: " << res._statusCode << std::endl;
	os << "Error Msg: " << res._statusTxt << std::endl;

	os << "Headers:" << std::endl;
	if (res._headers.empty()) {
		os << "  (none)" << std::endl;
	} else {
        	std::map<std::string, std::string>::const_iterator it;
        	for (it = res._headers.begin(); it != res._headers.end(); ++it) {
			os << "  [" << it->first << "] = " << it->second << std::endl;
		}
	}
	os << "Body: " << res._body << std::endl;
	return os;
}

bool Request::hasHeader(const std::string& key) const {
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
	    type == "image/x-icon"||
            type == "application/json");
}

static bool checkHttpError(const Request& req,Response& res, size_t size, std::string path, Server& server){
	if(req._method.empty())
		return (generateError(400, "Bad Request", res, "400 Bad Request", server));
	
	if(req._urlPath.find("/private")==0 && !req.hasHeader("Authorization"))
		return (generateError(401, "Unauthorized", res, "401 Unauthorized", server));

	std::ifstream file(path.c_str());
	if(!file.is_open())
		return (generateError(404,"Not Found", res, "404 Not Found", server));

	if(access(path.c_str(), R_OK) < 0 || access(path.c_str(), W_OK)< 0 || access(path.c_str(), X_OK) < 0 || !safePath(path))
		return (generateError(403,"Forbidden", res, "403 Forbidden", server)); 

	if(req._method != "GET" && req._method != "POST" && req._method != "DELETE")
		return (generateError(405, "Method Not Allowed", res, "405 Method Not Allowed", server));

	if(req._method == "POST" && !req.hasHeader("Content-Length"))
		return (generateError(411, "Required Length", res, "411 Required Length", server));

	if(req._body.size() > size)
		return (generateError(413, "Content Too Large" , res, "413 Content Too Large", server));

	if(req._urlPath.size() > 2048)
		return (generateError(414, "URL Too Long", res, "414 URL Too Long", server));

	if(req._method == "POST"){
		std::string type = getMimeType(path);
		if(!isSupportedType(type))
			return (generateError(415, "Unsupported Media Type", res, "415 Unsupported Media Type", server));
	}

	if(req._urlPath == "/teapot")
		return (generateError(418, "I'm a teapot", res, "418 I'm a teapot", server));

	if(req._urlPath.empty())
		return (generateError(500, "Internal Server Error",  res, "500 Internal Server Error", server));

	if( req._httpVersion != "HTTP/1.0" && req._httpVersion != "HTTP/1.1" )
		return (generateError(505, "HTTP Version not Supported", res, "505 HTTP Version Not Supported", server));

 return false;
}

Response Response::handleResponse(const Request &req, Server& server){
	Response res;
	res._httpVersion = req._httpVersion;
	
	size_t size;
	std::stringstream ss(server.getMaxByte());
	ss >> size;
	std::string path = "." + req._urlPath;
	if(path[path.size() - 1] == '/')
		path +=  "index.html";
	//check config error here
	//
	std::cout << "-----------------------------SERVER TEST----------------------" << std::endl;
	std::cout << server << std::endl;
	std::cout << "--------------------------------------------------------------------------" << std::endl;
	//
	if(checkHttpError(req, res, size, path, server)){
		 return res;
	}

	std::ifstream file(path.c_str(), std::ios::binary); 
	std::ostringstream os;	
	os << file.rdbuf();
	res._body = os.str();

	res._statusCode = 200;
	res._statusTxt = "OK";
	res._headers["Content-Type"] = getMimeType(path);
	res._headers["Content-Length"] = intToString(res._body.size());
	res._headers["Connection"] = "close";
	return res;
}



