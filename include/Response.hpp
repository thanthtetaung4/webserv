/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */ /*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 13:39:37 by lshein            #+#    #+#             */
/*   Updated: 2025/11/10 17:17:37 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include "Cgi.hpp"
#include <filesystem>
#include "Server.hpp"
#include "ServerException.hpp"
#include <cstdlib>
#include <fstream>
#include <ostream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <map>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include "Socket.hpp"

class Response
{
private:
	std::string _httpVersion;
	int _statusCode;
	std::string _statusTxt;
	std::map<std::string, std::string> _headers;
	std::string _body;
	void	doPost(std::string uploadPath, const Request &req);
	void	doDelete(std::string uploadPath, const Request &req);

public:
	Response(Request &req, Server &server);
	Response(unsigned int errorCode);
	void handleRedirect(const std::string &redirUrlPath);
	void handleStore(t_location loc, const Request& req);
	void handleAutoIndex(const std::string &urlPath, const std::string &fullPath);
	void handleCGI(const Request &req, const Server &server);
	std::string handleReverseProxy(const Request &req);
	bool generateError(int errorCode, std::string const errorMsg, std::string const bodyMsg, Server &server);
	bool checkHttpError(const Request &req, size_t size, std::string path, Server &server);
	void serveFile(const std::string &filePath);
	std::string toStr() const;

	// Accessors
	void setHttpVersion(const std::string &version);
	void setStatusCode(int code);
	void setStatusTxt(const std::string &text);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);
	std::string getHttpVersion() const;
	int getStatusCode() const;
	std::string getStatusTxt() const;
	const std::map<std::string, std::string> &getHeaders() const;
	std::string getBody() const;
};
std::string intToString(size_t n);

std::ostream &operator<<(std::ostream &os, const Response &res);
#endif
