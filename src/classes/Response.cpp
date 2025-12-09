/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:39:28 by hthant            #+#    #+#             */
/*   Updated: 2025/12/09 13:13:19 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Response.hpp"

Response::Response(Request &req, Server &server)
{
	// Parse max body size
	size_t maxSize = 0;
	std::stringstream ss(server.getMaxByte());
	ss >> maxSize;

	this->_httpVersion = req.getHttpVersion();
	
	// Handle server-level return directive (highest priority)
	if (!server.getReturn().empty())
	{
		handleReturn(server.getReturn());
		return;
	}

	// Get location configuration
	std::map<std::string, t_location>::const_iterator locIt = req.getIt();
	
	// Location found - process with location rules
	if (locIt != server.getLocation().end())
	{
		const t_location &loc = locIt->second;

		// Handle location-level return directive (second priority)
		if (!loc._return.empty())
		{
			handleReturn(loc._return);
			return;
		}

		const std::string &finalPath = req.getFinalPath();
		const std::string &method = req.getMethodType();

		// Check limit_except restrictions (method whitelist)
		if (!loc._limit_except.empty())
		{
			bool methodAllowed = std::find(loc._limit_except.begin(), 
			                               loc._limit_except.end(), 
			                               method) != loc._limit_except.end();
			
			if (!methodAllowed)
			{
				generateError(405, "Method Not Allowed", "405 Method Not Allowed", server);
				return;
			}
		}

		// Handle upload/store operations (POST/DELETE to upload directories)
		if (!loc._uploadStore.empty() && (method == "POST" || method == "DELETE"))
		{
			std::cout << "Upload/Store detected for method: " << method << std::endl;
			handleStore(loc, req);
			return;
		}

		// Handle directory requests (only for GET)
		if (isDirectory(finalPath))
		{
			processDirectoryRequest(req, loc, maxSize, server);
			return;
		}

		// Handle CGI requests (file must exist and be regular file)
		if (isRegularFile(finalPath) && loc._isCgi)
		{
			handleCGI(req, server);
			return;
		}

		// Handle regular file requests
		processFileRequest(req, finalPath, maxSize, server);
	}
	else
	{
		// No location match - use default file processing
		processFileRequest(req, req.getFinalPath(), maxSize, server);
	}
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
	const std::string &dirPath = req.getFinalPath();
	
	// Try to find and serve index files if configured
	if (!loc._index.empty())
	{
		for (size_t i = 0; i < loc._index.size(); i++)
		{
			std::string indexPath = buildIndexPath(dirPath, loc._index[i]);
			
			// Check if index file exists and is a regular file
			if (isRegularFile(indexPath))
			{
				// Check if it's a CGI file
				if (loc._isCgi && !loc._cgiExt.empty() &&
				    indexPath.size() >= loc._cgiExt.size() &&
				    indexPath.substr(indexPath.size() - loc._cgiExt.size()) == loc._cgiExt)
				{
					// Handle as CGI
					req.setFinalPath(indexPath);
					handleCGI(req, server);
				}
				else
				{
					// Handle as regular file
					processFileRequest(req, indexPath, maxSize, server);
				}
				return;
			}
		}
		
		// No index file found - check if autoindex is enabled
		if (loc._autoIndex == "on")
		{
			handleAutoIndex(req.getPath(), dirPath);
		}
		else
		{
			// No index file and autoindex disabled
			generateError(403, "Forbidden", "403 Forbidden", server);
		}
		return;
	}
	
	// No index configured - check autoindex
	if (loc._autoIndex == "on")
	{
		handleAutoIndex(req.getPath(), dirPath);
	}
	else
	{
		// Directory browsing not allowed
		generateError(403, "Forbidden", "403 Forbidden", server);
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
	std::cout << "handling store" << std::endl;
	if (req.getMethodType() == "POST") {
		doPost(loc._uploadStore, req);
	} else if (req.getMethodType() == "DELETE") {
		doDelete(loc._uploadStore, req);
	}
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

bool Response::checkHttpError(const Request &req, size_t maxSize, std::string path, Server &server)
{
	std::map<int, std::pair<std::string, std::string> > errorMap = getErrorMap();
	int errorCode = 0;
	const std::string &method = req.getMethodType();
	const std::string &requestPath = req.getPath();
	const std::string &httpVersion = req.getHttpVersion();

	// Validate HTTP basics
	if (method.empty())
	{
		errorCode = 400; // Bad Request - no method
	}
	else if (httpVersion != "HTTP/1.0" && httpVersion != "HTTP/1.1")
	{
		errorCode = 505; // HTTP Version Not Supported
	}
	else if (requestPath.empty() || path.empty())
	{
		errorCode = 500; // Internal Server Error - path resolution failed
	}
	else if (requestPath.size() > 2048)
	{
		errorCode = 414; // URI Too Long
	}
	// Validate HTTP method
	else if (method != "GET" && method != "POST" && method != "DELETE")
	{
		errorCode = 405; // Method Not Allowed
	}
	// POST-specific validations
	else if (method == "POST")
	{
		if (!req.hasHeader("Content-Length"))
			errorCode = 411; // Length Required
		else if (req.getBody().size() > maxSize)
			errorCode = 413; // Content Too Large
	}
	// Check body size for all methods with body
	else if (!req.getBody().empty() && req.getBody().size() > maxSize)
	{
		errorCode = 413; // Content Too Large
	}
	// Special endpoints
	else if (requestPath == "/teapot")
	{
		errorCode = 418; // I'm a teapot (Easter egg)
	}
	// Authorization check for private paths
	else if (requestPath.find("/private") == 0 && !req.hasHeader("Authorization"))
	{
		errorCode = 401; // Unauthorized
	}
	// Path safety and file access checks
	else if (!safePath(path))
	{
		errorCode = 403; // Forbidden - unsafe path (e.g., contains ..)
	}
	else
	{
		// Check file existence and readability
		std::ifstream file(path.c_str());
		if (!file.is_open())
		{
			errorCode = 404; // Not Found
		}
		else
		{
			file.close();
			
			// Check read permissions
			if (access(path.c_str(), R_OK) < 0)
			{
				errorCode = 403; // Forbidden - no read permission
			}
			// POST-specific content type validation
			else if (method == "POST")
			{
				std::string contentType = getMimeType(path);
				if (!isSupportedType(contentType))
				{
					errorCode = 415; // Unsupported Media Type
				}
			}
		}
	}

	// Generate error response if error detected
	if (errorCode != 0)
	{
		std::map<int, std::pair<std::string, std::string> >::const_iterator it = errorMap.find(errorCode);
		if (it != errorMap.end())
			return generateError(errorCode, it->second.first, it->second.second, server);
		
		// Fallback for unmapped error codes
		return generateError(errorCode, "Error", "<h1>Error " + intToString(errorCode) + "</h1>", server);
	}

	return false; // No error
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
