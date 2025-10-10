/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 00:42:43 by hthant            #+#    #+#             */
/*   Updated: 2025/10/10 00:59:40 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef REQUEST_HPP
# define REQUEST_HPP

# include <iostream>
# include <map>

class Request{
	public:
		std::string _method;
		std::string _urlPath;
		std::string _httpVersion;
		std::map<std::string, std::string> _headers;
		std::string _body;

		static Request Parse(const std::string &raw);
};
# endif
