/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 00:42:43 by hthant            #+#    #+#             */
/*   Updated: 2025/12/08 15:48:20 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include "Server.hpp"
#include "Location.h"
#include "ServerException.hpp"
#include "utils.h"

class Request {
	private:
		std::string _method;
		std::string _path;
		std::string _finalPath;
		std::string _httpVersion;
		std::map<std::string, std::string> _headers;
		std::string _body;
		std::string _queryString;
		std::map<std::string, t_location>::const_iterator _it;

	public:
		Request(const std::string &raw, Server &server);
		bool hasHeader(const std::string &key) const;
		void setFinalPath(const std::string &path);
		std::string getMethodType() const;
		std::string getPath() const;
		std::string getFinalPath() const;
		std::string getHttpVersion() const;
		std::string getBody() const;
		std::string getContentType() const;
		std::string getQueryString() const;
		std::map<std::string, t_location>::const_iterator getIt() const;
		const std::map<std::string, std::string> &getHeaders() const;
};
std::string trim(const std::string &s);
std::ostream &operator<<(std::ostream &os, const Request &req);
#endif

// check path if not error
// if mathod check
// if pass then go to response
