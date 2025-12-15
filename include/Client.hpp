/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 14:08:59 by taung             #+#    #+#             */
/*   Updated: 2025/12/15 15:09:15 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

class Server;
class Request;
class Response;

#include <string>
#include <map>
#include <cstddef>
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

enum ClientState {
		READ_REQ,
		REQ_RDY,
		WAIT_UPSTREAM,
		UPSTREAM_RDY,
		RES_RDY
	};

class Client {
	private:
		int		fd;             // client socket fd
		Server&	server;
		int		upstreamFd;
		std::string	inBuffer;       // raw request data (headers + body)
		std::string	upstreamBuffer;      // buffer for upstream

		Request*	request;        // parsed request
		Response*	response;       // generated response

		ClientState	state;

		size_t	headerEndPos; // header end position
		size_t	contentLength;

	public:
		Client();
		Client(int fd, const Server& server);
		Client&	operator=(const Client& other);
		~Client();

		// Utils
		bool	buildRes();
		bool	buildReq();
		void	appendRecvBuffer(std::string buff);
		void	addToUpstreamBuffer(std::string buff);
		bool	isRequestComplete(void);
		bool	isProxyRequest();
		void	setSendBuffer(std::string rawString);
		bool	foundHeader(void) const;
		bool	isProxyPass(void) const;

		//	Accessors
		//	Getters
		int	getFd(void) const;
		const Request*		getRequest(void) const;
		const Response*	getResponse(void) const;
		ClientState	getState(void) const;
		const std::string&	getInBuffer(void) const;
		size_t	getHeaderEndPos(void) const;
		size_t	getContentLength(void) const;
		const std::string&	getUpstreamBuffer(void) const;
		int	getUpstreamFd(void) const;

		//	Setters
		void	setInBuffer(std::string rawStr);
		void	setState(ClientState state);
		void	setHeaderEndPos(size_t pos);
		void	setContentLength(size_t cl);
		void	setUpstreamFd(int fd);
};
std::ostream &operator<<(std::ostream &os, const Client &client);

#endif
