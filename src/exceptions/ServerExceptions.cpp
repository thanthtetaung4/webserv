/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerExceptions.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/11 01:56:39 by taung             #+#    #+#             */
/*   Updated: 2025/11/08 20:46:59 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/ServerException.hpp"

const char* InvalidSocket::what() const throw() {
	return "Invalid Socket";
}

const char* UnableToOpenSocket::what() const throw() {
	return "Unable To OpenSocket";
}

const char* UnableToBind::what() const throw() {
	return "Unable To Bind";
}

const char* UnableToListen::what() const throw() {
	return "Unable To Listen";
}

const char* UnableToCreateResponse::what() const throw() {
	return "Unable To Create Response";
}

const char* UnableToCreateRequest::what() const throw() {
	return "Unable To Create Request";
}
