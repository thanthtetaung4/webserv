/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 14:08:59 by taung             #+#    #+#             */
/*   Updated: 2025/12/13 23:21:35 by taung            ###   ########.fr       */
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
		RES_RDY,
		DONE
	};

class Client {
	private:
		int		fd;             // client socket fd
		Server&	server;
		std::string	inBuffer;       // raw request data (headers + body)
		std::string	outBuffer;      // serialized HTTP response

		Request*	request;        // parsed request
		Response*	response;       // generated response

		ClientState	state;

		size_t	headerEndPos; // header end position
		size_t	contentLength;

		// Reverse proxy support
		int		upstreamFd;     // fd of upstream server (if proxy_pass)
		bool	isProxyClient;  // true if this request uses proxy_pass

	public:
		Client();
		Client(int fd, const Server& server);
		Client&	operator=(const Client& other);
		~Client();

		// Utils
		bool	buildRes();
		bool	buildReq();
		void	appendRecvBuffer(std::string buff);
		void	addToOutBuffer(std::string buff);
		bool	isRequestComplete(void);
		bool	isProxyRequest();
		void	setSendBuffer(std::string rawString);
		bool	foundHeader(void) const;

		//	Accessors
		//	Getters
		int	getFd(void) const;
		const Request*		getRequest(void) const;
		const Response*	getResponse(void) const;
		ClientState	getState(void) const;
		const std::string&	getInBuffer(void) const;
		size_t	getHeaderEndPos(void) const;
		size_t	getContentLength(void) const;

		//	Setters
		void	setInBuffer(std::string rawStr);
		void	setState(ClientState state);
		void	setHeaderEndPos(size_t pos);
		void	setContentLength(size_t cl);
};
std::ostream &operator<<(std::ostream &os, const Client &client);

#endif
