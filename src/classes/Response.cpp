/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:39:28 by hthant            #+#    #+#             */
/*   Updated: 2025/12/14 20:01:37 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Response.hpp"

Response::Response(Request &req, Server &server)
{
	std::cout << "=========== Res Start ===========" << std::endl;
	size_t size;
	std::string res;
	std::string path = req.getFinalPath();
	this->_httpVersion = req.getHttpVersion();
	std::stringstream ss(server.getMaxByte());
	ss >> size;
	std::cout << size << std::endl;
	size_t i;
	if (req.getIt() != server.getLocation().end())
	{
		t_location loc = req.getIt()->second;

		if (isDirectory(req.getFinalPath()))
		{
			std::cout << "=========== is file ===========" << std::endl;
			if (!loc._proxy_pass.empty())
			{
				std::cout << "=========== ppass ===========" << std::endl;
				// handle proxypass
				std::string rawRes = handleReverseProxy(req);
				std::cout << rawRes << std::endl;
			}
			else if (!loc._index.empty())
			{
				for (i = 0; i < loc._index.size(); i++)
				{
					if (isRegularFile(req.getFinalPath() + (req.getFinalPath().at(req.getFinalPath().length() - 1) == '/' ? "" : "/")  + loc._index[i]))
					{
						path = req.getFinalPath() + "/" + loc._index[i];
						break;
					}
				}
				if (i == loc._index.size() && loc._autoIndex == "on") {
					std::cout << "=========== auto index ===========" << std::endl;
					handleAutoIndex(req.getPath(), req.getFinalPath());
				}
				else
				{
					if (loc._isCgi && path.substr(path.size() - loc._cgiExt.size()) == loc._cgiExt)
					{
						req.setFinalPath(path);
						std::cout << "=========== cgi ===========" << std::endl;
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
		else if (isRegularFile(req.getFinalPath()) && loc._isCgi) {
			std::cout << "=========== is reg file ===========" << std::endl;
			std::cout << "=========== cgi ===========" << std::endl;
			handleCGI(req, server);
		}
		else {
			std::cout << "=========== is not file, not dir ===========" << std::endl;
			if (!loc._proxy_pass.empty()) {
				std::cout << "=========== ppass ===========" << std::endl;
				std::string rawRes = handleReverseProxy(req);
				std::cout << rawRes << std::endl;
			} else if (!loc._uploadStore.empty()) {
				std::cout << "upload store" << std::endl;
				if (std::find(loc._limit_except.begin(), loc._limit_except.end(), req.getMethodType()) != loc._limit_except.end()) {
					handleStore(loc, req);
				} else {
					std::cout << "408 Method not allowed" << std::endl;
				}
			} else {
				std::cout << "=========== static ===========" << std::endl;
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
	else
	{
		std::cout << "=========== loc is end =========" << std::endl;
		std::cout << "=========== static ===========" << std::endl;
		std::cout << "Requested Path: " << path << std::endl;
		if (!checkHttpError(req, size, path, server))
			serveFile(path);
		if (this->_body.empty())
		{
			std::cout << "Body is empty" << std::endl;
		}
	}
	std::cout << "=========== Res END ===========" << std::endl;
}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		this->_httpVersion = other._httpVersion;
		this->_statusCode = other._statusCode;
		this->_statusTxt = other._statusTxt;
		this->_headers = other._headers;
		this->_body = other._body;
	}
	return *this;
}
void Response::processFileRequest(Request &req, const std::string &path, size_t maxSize, Server &server)
{
	std::cout << "Requested Path: " << path << std::endl;
	if (!checkHttpError(req, maxSize, path, server))
		serveFile(path);
	if (this->_body.empty())
		std::cout << "Body is empty" << std::endl;
}

void Response::processDirectoryRequest(Request &req, const t_location &loc, size_t maxSize, Server &server)
{
	// Handle index files
	if (!loc._index.empty())
	{
		std::string indexPath;
		for (size_t i = 0; i < loc._index.size(); i++)
		{
			indexPath = buildIndexPath(req.getFinalPath(), loc._index[i]);
			if (isRegularFile(indexPath))
			{
				// Found index file - check if CGI
				if (loc._isCgi && indexPath.size() >= loc._cgiExt.size() &&
					indexPath.substr(indexPath.size() - loc._cgiExt.size()) == loc._cgiExt)
				{
					req.setFinalPath(indexPath);
					handleCGI(req, server);
				}
				else
				{
					processFileRequest(req, indexPath, maxSize, server);
				}
				return;
			}
		}
		// No index file found - try autoindex
		if (loc._autoIndex == "on")
			handleAutoIndex(req.getPath(), req.getFinalPath());
	}
	else if (loc._autoIndex == "on")
	{
		// No index configured, but autoindex enabled
		handleAutoIndex(req.getPath(), req.getFinalPath());
	}
}



std::string Response::buildIndexPath(const std::string &basePath, const std::string &indexFile) const
{
	if (basePath.empty())
		return indexFile;
	if (basePath[basePath.length() - 1] == '/')
		return basePath + indexFile;
	return basePath + "/" + indexFile;
}

void Response::handleRedirect(const std::string &redirUrlPath)
{
	this->_statusCode = 302;
	this->_statusTxt = "Found";
	this->_headers["Location"] = redirUrlPath;
	this->_headers["Content-Type"] = "text/html";
	this->_headers["Content-Length"] = "0";
}

bool Response::isRedirectStatus(int statusCode) const
{
	return (statusCode >= 301 && statusCode <= 308) &&
	       (statusCode == 301 || statusCode == 302 || statusCode == 303 ||
	        statusCode == 307 || statusCode == 308);
}

void Response::setRedirectResponse(int statusCode, const std::string &statusTxt, const std::string &location)
{
	this->_statusCode = statusCode;
	this->_statusTxt = statusTxt;
	this->_headers.clear();
	this->_headers["Location"] = location;
	this->_headers["Content-Type"] = "text/html";
	this->_headers["Content-Length"] = "0";
	this->_body = "";
}

std::string Response::handleReverseProxy(const Request &req)
{
	// 1. Parse proxy_pass directive
	t_proxyPass pp = parseProxyPass(req.getIt()->second._proxy_pass);
	std::cout << "Proxying to " << pp.host << ":" << pp.port << pp.path << std::endl;

	// 2. Create upstream client socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		throw std::runtime_error("Failed to create proxy socket");

	// 3. Build remote address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = htons(std::atoi(pp.port.c_str()));
	server_addr.sin_addr.s_addr = inet_addr(pp.host.c_str());

	if (server_addr.sin_addr.s_addr == INADDR_NONE) {
		close(sockfd);
		throw std::runtime_error("Invalid proxy address");
	}

	// 4. Connect to upstream
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect");
		close(sockfd);
		throw std::runtime_error("Unable to connect to proxy server");
	}

	std::cout << "Connected to upstream" << std::endl;

	// 5. Build request to upstream
	std::string proxyRequest;
	proxyRequest += req.getMethodType() + " " + pp.path + " " + req.getHttpVersion() + "\r\n";

	// Copy client headers
	for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin();
			it != req.getHeaders().end(); ++it)
	{
		proxyRequest += it->first + ": " + it->second + "\r\n";
	}

	proxyRequest += "\r\n"; // end headers
	proxyRequest += req.getBody();

	std::cout << "Sending to upstream:\n" << proxyRequest << std::endl;

	// 6. Send request to upstream
	ssize_t sent = send(sockfd, proxyRequest.c_str(), proxyRequest.size(), 0);
	if (sent < 0) {
		close(sockfd);
		throw std::runtime_error("Unable to send to proxy");
	}

	// 7. Receive full response
	std::string fullResponse;
	char buffer[4096];

	while (true) {
		ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0);
		if (n < 0) {
			close(sockfd);
			throw std::runtime_error("Error receiving from proxy");
		}
		if (n == 0)
			break; // EOF

		fullResponse.append(buffer, n);

		// If Content-Length is known and we got all, we can break early
		// (optional optimization later)
	}

	close(sockfd);

	std::cout << "Received from upstream (" << fullResponse.size() << " bytes)" << std::endl;

	return fullResponse;
}

void Response::handleReturn(const std::vector<std::string> &returnDirective)
{
	// Return directive format: [status_code] or [status_code, url/text/file_path]
	if (returnDirective.empty())
		return;

	int statusCode = atoi(returnDirective[0].c_str());
	std::string statusTxt;
	std::string body;

	// Get status text and default body from error map
	std::map<int, std::pair<std::string, std::string> > errorMap = getErrorMap();
	std::map<int, std::pair<std::string, std::string> >::const_iterator it = errorMap.find(statusCode);
	if (it != errorMap.end())
	{
		statusTxt = it->second.first;
		body = it->second.second;
	}
	else
	{
		statusTxt = "Redirect";
		body = "<h1>" + returnDirective[0] + " Redirect</h1>";
	}

	// No second parameter - use default body
	if (returnDirective.size() <= 1 || returnDirective[1].empty())
	{
		setResponseState(statusCode, statusTxt, body, "text/html");
		return;
	}

	std::string secondParam = returnDirective[1];

	// External URL (http:// or https://) - always redirect
	if (secondParam.find("http://") == 0 || secondParam.find("https://") == 0)
	{
		setRedirectResponse(statusCode, statusTxt, secondParam);
		return;
	}

	// Path starting with / - behavior depends on status code
	if (secondParam[0] == '/')
	{
		if (isRedirectStatus(statusCode))
		{
			// 3xx redirect codes - treat as internal redirect
			setRedirectResponse(statusCode, statusTxt, secondParam);
		}
		else
		{
			// Non-redirect status - try to load custom error page from file
			std::ifstream file(secondParam.c_str());
			if (file.is_open())
			{
				std::stringstream buffer;
				buffer << file.rdbuf();
				body = buffer.str();
				file.close();
			}
			// Use loaded content or fall back to default body
			setResponseState(statusCode, statusTxt, body, "text/html");
		}
		return;
	}

	// Plain text - use as custom response body
	body = "<h1>" + secondParam + "</h1>";
	setResponseState(statusCode, statusTxt, body, "text/html");
}

void Response::handleAutoIndex(const std::string &urlPath, const std::string &fullPath)
{
	// Ensure URL ends with slash
	if (urlPath.empty() || urlPath[urlPath.size() - 1] != '/')
	{
		handleRedirect(urlPath + "/");
		return;
	}

	// Check directory accessibility
	if (access(fullPath.c_str(), R_OK) == -1)
	{
		std::pair<std::string, std::string> error = getErrorFromMap(403);
		setResponseState(403, error.first, error.second, "text/html");
		return;
	}

	// Try opening directory
	DIR *dir = opendir(fullPath.c_str());
	if (!dir)
	{
		std::pair<std::string, std::string> error = getErrorFromMap(404);
		setResponseState(404, error.first, error.second, "text/html");
		return;
	}

	// Build directory listing HTML
	std::string body = buildDirectoryListingHTML(urlPath, fullPath);
	closedir(dir);

	// Set successful response
	setResponseState(200, "OK", body, "text/html");
}

std::string Response::buildDirectoryListingHTML(const std::string &urlPath, const std::string &fullPath)
{
	std::string html;
	html += "<html><head><title>Index of " + urlPath + "</title></head><body>";
	html += "<h1>Index of " + urlPath + "</h1><hr><pre>";

	DIR *dir = opendir(fullPath.c_str());
	if (!dir)
		return html + "</pre><hr></body></html>";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;

		// Skip . and ..
		if (name == "." || name == "..")
			continue;

		// Get file info
		std::string itemFullPath = fullPath + "/" + name;
		struct stat st;
		if (stat(itemFullPath.c_str(), &st) == -1)
			continue;

		// Build link
		html += "<a href=\"" + urlPath + name;
		if (S_ISDIR(st.st_mode))
			html += "/";
		html += "\">" + name + "</a>\n";
	}

	closedir(dir);
	html += "</pre><hr></body></html>";
	return html;
}

void	Response::doPost(std::string uploadPath, const Request &req) {
	// std::cout << "DO POST\n" << uploadPath << "\n" << req << std::endl;
	std::vector<std::string> fileNames;
	std::vector<std::string> fileContents;

	parseFile(req.getBody(), req.getContentType(), fileNames, fileContents);
	std::vector<std::string>::const_iterator itFileContents;
	std::vector<std::string>::const_iterator itFileNames;

	for (itFileContents = fileContents.begin(), itFileNames = fileNames.begin(); (itFileContents != fileContents.end()) && itFileNames != fileNames.end(); itFileContents++, itFileNames++) {
		// std::cout << "===================================== FILE =====================================" << std::endl;

		// std::cout << "File name: " << *itFileNames << std::endl;
		// std::cout << "File content: \n" << *itFileContents << std::endl;

		const std::string &fileName    = *itFileNames;
		const std::string &fileContent = *itFileContents;

		// Build full path (e.g. /upload/image.png)
		std::string fullPath = uploadPath + "/" + fileName;
		// std::cout << "File upload path: " << fullPath << std::endl;

		// Open file (binary mode)
		std::ofstream out(fullPath.c_str(), std::ios::binary);
		if (!out)
		{
			std::cerr << "Upload error: failed to open " << fullPath << std::endl;
			this->_statusCode = 500;
			this->_body = "<!DOCTYPE html>\n"
							"<html>\n"
							"<head><title>KO</title></head>\n"
							"<body>\n"
							"<h1>Response from C++</h1>\n"
							"<p>Upload of file" + fileName + " failed</p>\n"
							"</body>\n"
							"</html>";
			return;
		}

		// Write raw content
		out.write(fileContent.data(), fileContent.size());
		out.close();

		std::cout << "Uploaded: " << fullPath
				<< " (" << fileContent.size() << " bytes)"
				<< std::endl;
		// std::cout << "===================================== FILE END =====================================" << std::endl;
	}
	this->_statusCode = 200;
	this->_body = "<!DOCTYPE html>\n"
					"<html>\n"
					"<head><title>OK</title></head>\n"
					"<body>\n"
					"<h1>Response from C++</h1>\n"
					"<p>Everything works!</p>\n"
					"</body>\n"
					"</html>";
}

#include <sys/stat.h>
#include <unistd.h>

void	Response::doDelete(std::string uploadPath, const Request &req) {
	// std::cout << "DO DELETE\n" << uploadPath << "\n" << req << std::endl;

	std::string filePath;

	// Parse from URL path (e.g. "DELETE /upload/test.txt")
	// parseFile writes the relative path into filePath
	parseFile(req.getPath(), req.getIt()->second._root, filePath);

	// std::cout << "=====================================" << std::endl;
	// std::cout << "fileName: " << filePath << std::endl;

	// Construct the full path
	std::string fullPath = uploadPath + filePath.substr(filePath.find_last_of('/'));

	// Check existence
	struct stat st;
	if (stat(fullPath.c_str(), &st) != 0) {
		// file doesn't exist
		std::cerr << "File does not exist: " << fullPath << std::endl;
		// You can set 404 here
		this->_statusCode = 404;
		this->_body = "File not found.\n";
		return;
	}

	// Delete the file
	if (unlink(fullPath.c_str()) != 0) {
		// unlink failed → no permission or locked
		std::cerr << "Failed to delete: " << fullPath << std::endl;
		// set 500 or 403 depending on your logic
		this->_statusCode = 500;
		this->_body = "Failed to delete the file.\n";
		return;
	}

	std::cout << "Deleted: " << fullPath << std::endl;

	// Success
	this->_statusCode = 200;
	this->_body = "File deleted.\n";
}

void Response::handleStore(t_location loc, const Request& req) {
	if (req.getMethodType() == "POST") {
		doPost(loc._uploadStore, req);
	} else if (req.getMethodType() == "DELETE") {
		doDelete(loc._uploadStore, req);
	}
}

void Response::handleCGI(const Request &req, const Server &server)
{
	// Execute CGI and parse output
	Cgi cgi(req, server);
	std::string output = cgi.execute();
	CgiResult parsed = cgi.parseCgiHeaders(output);

	// Determine content type
	std::string contentType = "text/html"; // Default fallback
	if (parsed.headers.count("Content-Type"))
		contentType = parsed.headers["Content-Type"];

	// Parse status code from CGI output
	if (parsed.headers.count("Status"))
		parseCgiStatus(parsed.headers["Status"]);
	else
	{
		this->_statusCode = 200;
		this->_statusTxt = "OK";
	}

	// Set response state
	setResponseState(this->_statusCode, this->_statusTxt, parsed.body, contentType);
}

void Response::parseCgiStatus(const std::string &statusHeader)
{
	// Parse "Status: 404 Not Found" format
	size_t space = statusHeader.find(' ');
	if (space != std::string::npos)
	{
		this->_statusCode = atoi(statusHeader.substr(0, space).c_str());
		this->_statusTxt = statusHeader.substr(space + 1);
	}
	else
	{
		// Invalid format, default to 200
		this->_statusCode = 200;
		this->_statusTxt = "OK";
	}
}

void Response::setResponseState(int statusCode, const std::string &statusTxt, const std::string &body, const std::string &contentType)
{
	this->_statusCode = statusCode;
	this->_statusTxt = statusTxt;
	this->_body = body;
	this->_headers.clear();
	this->_headers["Content-Type"] = contentType;
	this->_headers["Content-Length"] = intToString(body.size());
	this->_headers["Connection"] = "close";
}

bool safePath(const std::string &path)
{
	return path.find("..") == std::string::npos;
}

std::string getMimeType(const std::string &path)
{
	size_t deliPos = path.rfind('.');

	// No extension or extension is at the end
	if (deliPos == std::string::npos || deliPos >= path.length() - 1)
		return "text/plain";

	std::string type = path.substr(deliPos + 1);

	if (type == "html")
		return "text/html";
	if (type == "css")
		return "text/css";
	if (type == "js")
		return "application/javascript";
	if (type == "png")
		return "image/png";
	if (type == "jpg" || type == "jpeg")
		return "image/jpeg";
	if (type == "ico")
		return "image/x-icon";
	if (type == "json")
		return "application/json";

	return "text/plain";
}

std::string readFile(const std::string &path)
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}

bool Response::generateError(int errorCode, std::string const errorMsg, std::string const bodyMsg, Server &server)
{
	this->_statusCode = errorCode;
	std::map<std::string, std::string> errorPages = server.getErrorPage();
	std::map<std::string, std::string>::iterator it = errorPages.find(intToString(this->_statusCode));
	std::string body;

	if (it != errorPages.end())
	{
		std::cout << "Custom error page found for " << this->_statusCode << ": " << it->second << std::endl;
		body = readFile(it->second);
		std::cout << "Custom page path: " << it->second << std::endl;
		if (body.empty())
		{
			std::cout << "Failed to read custom error page: " << it->second << std::endl;
		}
	}

	// Use simple default error page if no custom page loaded
	if (body.empty())
	{
		std::cout << "Using default error page for " << this->_statusCode << std::endl;
		body = bodyMsg;
	}

	setResponseState(errorCode, errorMsg, body, "text/html");
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

std::map<int, std::pair<std::string, std::string> > Response::getErrorMap()
{
	std::map<int, std::pair<std::string, std::string> > errorMap;
	errorMap[400] = std::make_pair("Bad Request", "<h1>400 Bad Request</h1>");
	errorMap[401] = std::make_pair("Unauthorized", "<h1>401 Unauthorized</h1>");
	errorMap[403] = std::make_pair("Forbidden", "<h1>403 Forbidden</h1>");
	errorMap[404] = std::make_pair("Not Found", "<h1>404 Not Found</h1>");
	errorMap[405] = std::make_pair("Method Not Allowed", "<h1>405 Method Not Allowed</h1>");
	errorMap[411] = std::make_pair("Length Required", "<h1>411 Length Required</h1>");
	errorMap[413] = std::make_pair("Content Too Large", "<h1>413 Content Too Large</h1>");
	errorMap[414] = std::make_pair("URI Too Long", "<h1>414 URI Too Long</h1>");
	errorMap[415] = std::make_pair("Unsupported Media Type", "<h1>415 Unsupported Media Type</h1>");
	errorMap[418] = std::make_pair("I'm a teapot", "<h1>418 I'm a teapot</h1>");
	errorMap[500] = std::make_pair("Internal Server Error", "<h1>500 Internal Server Error</h1>");
	errorMap[502] = std::make_pair("Bad Gateway", "<h1>502 Bad Gateway</h1>");
	errorMap[505] = std::make_pair("HTTP Version Not Supported", "<h1>505 HTTP Version Not Supported</h1>");
	return errorMap;
}

std::pair<std::string, std::string> Response::getErrorFromMap(int errorCode)
{
	std::map<int, std::pair<std::string, std::string> > errorMap = getErrorMap();
	std::map<int, std::pair<std::string, std::string> >::const_iterator it = errorMap.find(errorCode);
	if (it != errorMap.end())
		return it->second;
	// Default fallback
	return std::make_pair("Error", "<h1>Error</h1>");
}

bool Response::checkHttpError(const Request &req, size_t size, std::string path, Server &server)
{
	std::map<int, std::pair<std::string, std::string> > errorMap = getErrorMap();
	int errorCode = 0;

	if (req.getMethodType().empty())
		errorCode = 400;
	else if (req.getPath().find("/private") == 0 && !req.hasHeader("Authorization"))
		errorCode = 401;
	else if (req.getPath().empty() || path.empty())
		errorCode = 500;
	else if (req.getHttpVersion() != "HTTP/1.0" && req.getHttpVersion() != "HTTP/1.1")
		errorCode = 505;
	else if (req.getPath() == "/teapot")
		errorCode = 418;
	else if (req.getPath().size() > 2048)
		errorCode = 414;
	else if (req.getMethodType() != "GET" && req.getMethodType() != "POST" && req.getMethodType() != "DELETE")
		errorCode = 405;
	else if (req.getMethodType() == "POST" && !req.hasHeader("Content-Length"))
		errorCode = 411;
	else if (req.getBody().size() > size)
		errorCode = 413;
	else
	{
		std::ifstream file(path.c_str());
		if (!file.is_open())
			errorCode = 404;
		else if (access(path.c_str(), R_OK) < 0 || !safePath(path))
			errorCode = 403;
		else if (req.getMethodType() == "POST")
		{
			std::string type = getMimeType(path);
			if (!isSupportedType(type))
				errorCode = 415;
		}
	}
	if (errorCode != 0)
	{
		std::map<int, std::pair<std::string, std::string> >::const_iterator it = errorMap.find(errorCode);
		if (it != errorMap.end())
			return generateError(errorCode, it->second.first, it->second.second, server);
	}

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
	std::string body = os.str();
	std::string contentType = getMimeType(filePath);
	setResponseState(200, "OK", body, contentType);
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
