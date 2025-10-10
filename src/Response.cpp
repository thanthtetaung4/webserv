/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:39:28 by hthant            #+#    #+#             */
/*   Updated: 2025/10/10 11:30:16 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../include/Response.hpp"
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>

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


void generateError(int errorCode, std::string const errorMsg, Response &res , std::string const bodyMsg){
	res._statusCode = errorCode;
	res._statusTxt = errorMsg;
	res._body = "<h1>" + bodyMsg + "</h1>";	
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
        // Use an explicit iterator to loop through the map
        std::map<std::string, std::string>::const_iterator it;
        for (it = res._headers.begin(); it != res._headers.end(); ++it) {
            os << "  [" << it->first << "] = " << it->second << std::endl;
        }
    }

    os << "Body: " << res._body << std::endl;
    return os;
}
Response Response::handleResponse(const Request &req){
	Response res;
	res._httpVersion = req._httpVersion;
	if( req._httpVersion != "HTTP/1.0" || req._httpVersion != "HTTP/1.1" )
		generateError(505, "HTTP Version not Supported", res, "Error 505");

	std::string path = "." + req._urlPath;
	if(path[path.size() - 1] == '/')
		path +=  "index.html";

	if(!safePath(path)){
		res._statusCode = 403;
		res._statusTxt = "Forbidden";
		res._body = "<h1>403 Forbidden<h1>";
		res._headers["Content-Type"] = "text/html";
	}

	std::cout << "Path is " << path <<std::endl;
	std::ifstream file(path.c_str());
	if(file.good()){
		if(req._method == "GET"){
			std::ifstream file(path.c_str(), std::ios::binary); 
			if(file.is_open()){
				std::ostringstream os;	
				os << file.rdbuf();
				res._body = os.str();
				res._statusCode = 200;
				res._statusTxt = "OK";
				res._headers["Content-Type"] = getMimeType(path);
				std::cout << res << std::endl;
			}
			else {
				generateError(500, "Internal Server Error", res , "File Read Error");
			}
		}
	}
	else {
		std::cout << " Here\n";
		generateError(404, "Not Found" , res, "404 Not Found");
	}
	res._headers["Content-Length"] = intToString(res._body.size());
        res._headers["Connection"] = "close";
	return  res;
}
