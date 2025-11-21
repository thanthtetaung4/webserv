/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   buildPath.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/16 18:06:48 by taung             #+#    #+#             */
/*   Updated: 2025/11/20 19:31:38 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include"../../include/locale.h"
# include<iostream>
# include "../../include/Server.hpp"

std::string	buildPath(const Server& server, std::string urlPath) {
	std::string	path = "";
	if (server.getRoot().empty())
		return (path);
	path.append(server.getRoot());
	path.append("/");
	path.append(urlPath);
	return (path);
}
