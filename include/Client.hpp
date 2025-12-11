/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/11 14:08:59 by taung             #+#    #+#             */
/*   Updated: 2025/12/11 20:49:32 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <map>
#include <cstddef> // size_t
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

class Request;
class Response;

// States a client connection can be in
enum ClientState {
	READING_HEADERS,   // reading until \r\n\r\n
	READING_BODY,      // reading request body (POST, PUT)
	READY_TO_PROCESS,  // full request has been received; construct Request/Response
	WAITING_UPSTREAM,  // reverse proxy: waiting for upstream server
	READY_TO_WRITE,    // response prepared, ready to write to socket
	WRITING_RESPONSE,  // epoll writes incomplete, continue writing
	DONE               // response sent, ready to close or reuse
};

class Client {

public:
	int		fd;             // client socket fd
	Server	server;
	std::string	inBuffer;       // raw request data (headers + body)
	std::string	outBuffer;      // serialized HTTP response

	Request*	request;        // parsed request
	Response*	response;       // generated response

	ClientState	state;          // current state

	size_t	headerEndPos;   // position of "\r\n\r\n"
	size_t	contentLength;  // body length from Content-Length
	bool	hasContentLength;

	// Reverse proxy support
	int		upstreamFd;     // fd of upstream server (if proxy_pass)
	bool	isProxyClient;  // true if this request uses proxy_pass

public:
	Client(int fd, const Server& server);
	~Client();

	bool	buildRes();
	bool	buildReq();
	void	appendRecvBuffer(std::string buff);
	void	addToOutBuffer(std::string buff);
	bool	isRequestComplete(void);
	ClientState	getState(void) const;
	void	setState(ClientState cs);
	bool	isProxyRequest();
	void	setSendBuffer(std::string rawString);

};

#endif
