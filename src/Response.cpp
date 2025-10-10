/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 01:39:28 by hthant            #+#    #+#             */
/*   Updated: 2025/10/10 11:30:16 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../include/Response.hpp"

Response Response::handleResponse(const Request &req){
	Response res;
	res._httpVersion = req._httpVersion;
	if(req._method == "GET"){
		
	}

	return  res;
}
