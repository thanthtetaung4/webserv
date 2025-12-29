/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/13 16:58:05 by taung             #+#    #+#             */
/*   Updated: 2025/12/14 20:01:43 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

class Request;
class Server;

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
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Socket.hpp"
#include <cstdio>

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
	std::string	rawRes;

	// Private helper methods
	void processFileRequest(Request &req, const std::string &path, size_t maxSize, Server &server);
	void processDirectoryRequest(Request &req, const t_location &loc, size_t maxSize, Server &server);
	std::string buildIndexPath(const std::string &basePath, const std::string &indexFile) const;
	std::string buildDirectoryListingHTML(const std::string &urlPath, const std::string &fullPath);
	void parseCgiStatus(const std::string &statusHeader);
	void setResponseState(int statusCode, const std::string &statusTxt, const std::string &body, const std::string &contentType);
	void setRedirectResponse(int statusCode, const std::string &statusTxt, const std::string &location);
	bool isRedirectStatus(int statusCode) const;

public:
	Response(Request &req, Server &server);
	Response(unsigned int errorCode);
	Response& operator=(const Response& other);
	std::string handleReverseProxy(const Request &req);
	void handleRedirect(const std::string &redirUrlPath);
	void handleStore(t_location loc, const Request& req);
	void handleReturn(const std::vector<std::string> &returnDirective);
	void handleAutoIndex(const std::string &urlPath, const std::string &fullPath);
	void handleCGI(const Request &req, const Server &server);
	bool generateError(int errorCode, std::string const errorMsg, std::string const bodyMsg, Server &server);
	bool checkHttpError(const Request &req, size_t size, std::string path, Server &server);
	static std::map<int, std::pair<std::string, std::string> > getErrorMap();
	static std::pair<std::string, std::string> getErrorFromMap(int errorCode);
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
