/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:04:38 by hthant            #+#    #+#             */
/*   Updated: 2025/11/08 20:50:34 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/Request.hpp"
#include <ostream>
#include <sstream>
#include <string>

static std::string trim(const std::string &s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
}

/*
	check whether the header values matche the server config
	@ param void
	@ return true if all header values are valid
	@ return false if any header value is invalid
*/
bool Request::checkHeaderValue(void) const {

	return true;
}

Request::Request(void) {
	throw UnableToCreateRequest();
}

Request::Request(const std::string &raw) {
	size_t hearderEnd = raw.find("\r\n\r\n");
	std::string hearderPart = raw.substr(0, hearderEnd);
	if(hearderEnd != std::string::npos)
		this->_body = raw.substr(hearderEnd + 4);

	std::stringstream lines(hearderPart);
	std::string line;
	if(std::getline(lines, line)) {
		std::istringstream first(line);
		std::cout << "First line: " << line << std::endl;
		first >> this->_method >> this->_urlPath >> this->_httpVersion;
	}

	while(std::getline(lines, line)) {
		if(line.empty())
			break;
		size_t delimeterPostion = line.find(":");
		if(delimeterPostion != std::string::npos) {
			std::string key = line.substr(0, delimeterPostion);
			std::string value = trim(line.substr(delimeterPostion + 1));
			this->_headers[key] = value;
		}
	}
}

std::ostream& operator<<(std::ostream& os, const Request& req){
	os << "Method: " << req._method << std::endl;
	os << "URL Path: " << req._urlPath << std::endl;
	os << "HTTP Version: " << req._httpVersion << std::endl;
	return  os;
}
