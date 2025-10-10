/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:04:38 by hthant            #+#    #+#             */
/*   Updated: 2025/10/10 01:39:25 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../include/Request.hpp"
#include <sstream>
#include <string>

static std::string trim(const std::string &s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
}

Request Request::Parse(const std::string &raw){
	Request req;

	size_t hearderEnd = raw.find("/r/n/r/n");
	std::string hearderPart = raw.substr(0, hearderEnd);
	if(hearderEnd != std::string::npos)
		req._body = raw.substr(hearderEnd + 4);

	std::stringstream lines(hearderPart);
	std::string line;
	if(std::getline(lines, line)){
		std::istringstream first(line);
		first >> req._method >> req._urlPath >> req._httpVersion;
	}

	while(std::getline(lines, line)){
		if(line.empty())
			break;
		size_t delimeterPostion = line.find(":");
		if(delimeterPostion != std::string::npos){
			std::string key = line.substr(0, delimeterPostion);
			std::string value = trim(line.substr(delimeterPostion + 1));
			req._headers[key] = value;
		}
	}
	return req;
}
