/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 00:42:43 by hthant            #+#    #+#             */
/*   Updated: 2025/11/08 20:48:16 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef REQUEST_HPP
# define REQUEST_HPP

# include <iostream>
# include <map>
# include <ostream>
# include <sstream>
# include <fstream>
#include "Server.hpp"
# include "ServerException.hpp"

class Request{
	private:
		std::string _method;
		std::string _urlPath;
		std::string _httpVersion;
		std::map<std::string, std::string> _headers;
		std::string _body;

	public:
		Request(void);
		Request(const std::string &raw);
		bool hasHeader(const std::string &key) const;
		bool checkHeaderValue(void) const;
		std::string getMethodType() const;
		std::string getUrlPath() const;
		std::string getHttpVersion() const;
		std::string getBody() const;
		const std::map<std::string, std::string> &getHeaders() const;
		int validateAgainstConfig(Server &server);
		
};
std::ostream& operator<<(std::ostream& os, const Request& req);
# endif


//check path if not error
//if mathod check
//if pass then go to response
