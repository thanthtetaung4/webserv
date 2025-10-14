/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 00:55:47 by hthant            #+#    #+#             */
/*   Updated: 2025/10/10 11:30:06 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "Request.hpp"
# include <filesystem>

class Response{
	public:
		std::string _httpVersion;
		int _statusCode;
		std::string _statusTxt;
		std::map<std::string , std::string> _headers;
		std::string _body;

		Response handleResponse(const Request& req, std::string _maxBytes);
		std::string toStr() const;
};

std::ostream& operator<<(std::ostream& os, const Response& res);
# endif

