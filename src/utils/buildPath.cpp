/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   buildPath.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/16 18:06:48 by taung             #+#    #+#             */
/*   Updated: 2025/11/16 20:12:06 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include"../../include/locale.h"
# include<iostream>
# include "../../include/Server.hpp"

std::string	buildPath(const Server& server, std::string urlPath) {
	std::string	path = "";
	if (server.getServerRoot().empty())
		return (path);
	path.append(server.getServerRoot());
	path.append("/");
	path.append(urlPath);
	return (path);
}
