/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   freeAll.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/20 02:31:35 by taung             #+#    #+#             */
/*   Updated: 2025/12/20 13:58:02 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/Client.hpp"
#include "../../include/WebServer.hpp"
#include "../../include/utils.h"

void	freeAllClients(WebServer& ws) {
	std::map<int, Client*>::iterator it = ws.getClients().begin();
	std::map<int, Client*>::iterator ite = ws.getClients().end();

	for (; it != ite; ++it) {
		close(it->first);
		delete it->second;
	}
	ws.getClients().clear();
}

void	freeAllUpstreamClients(WebServer& ws) {
	std::map<int, Client*>::iterator it = ws.getUpstreamClients().begin();
	std::map<int, Client*>::iterator ite = ws.getUpstreamClients().end();

	for (; it != ite; ++it) {
		close(it->first);
		delete it->second;
	}
	ws.getUpstreamClients().clear();
}

void closeAllSockets(WebServer& ws) {
	std::vector<Socket>	sockets = ws.getSockets();
	for (size_t i = 0; i < sockets.size(); ++i) {
		sockets[i].closeSock();
	}
}

void	freeAll(WebServer& ws) {
	freeAllClients(ws);
	freeAllUpstreamClients(ws);
	closeAllSockets(ws);
}
