/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 13:39:37 by lshein            #+#    #+#             */
/*   Updated: 2025/11/09 13:43:14 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


# ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "Request.hpp"
# include <filesystem>
# include "Server.hpp"
# include "ServerException.hpp"
# include <cstdlib>
# include <fstream>
# include <ostream>
# include <pthread.h>
# include <sstream>
# include <string>
# include <map>
# include <unistd.h>
# include <vector>

class Response{
	private:
		std::string _httpVersion;
		int _statusCode;
		std::string _statusTxt;
		std::map<std::string , std::string> _headers;
		std::string _body;
	public:
		Response(void);
		Response(const Request& res, Server& server);
		static Response handleResponse(const Request& req, Server& server);
		bool								generateError(int errorCode, std::string const errorMsg, std::string const bodyMsg, Server& server);
		bool								checkHttpError(const Request& req, size_t size, std::string path, Server& server);
		std::string							toStr() const;

		// Accessors
		void								setHttpVersion(const std::string& version);
		void								setStatusCode(int code);
		void								setStatusTxt(const std::string& text);
		void								setHeader(const std::string& key, const std::string& value);
		void								setBody(const std::string& body);
		std::string							getHttpVersion() const;
		int									getStatusCode() const;
		std::string							getStatusTxt() const;
		const std::map<std::string, std::string>	&getHeaders() const;
		std::string							getBody() const;

};

std::ostream& operator<<(std::ostream& os, const Response& res);
# endif

