/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:04:28 by hthant            #+#    #+#             */
/*   Updated: 2025/12/09 17:51:48 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <cstdlib>
#include "Request.hpp"
#include "Response.hpp"

enum ClientState {
	STATE_READING_REQUEST,
	STATE_BUILDING_RESPONSE,
	STATE_WRITING_RESPONSE,
	STATE_PROXYING_UPSTREAM,
	STATE_PROXYING_RESPONSE,
	STATE_CLOSED
};

class Client {
	private:
		int					_fd;
		std::string			_recvBuffer;
		std::string			_sendBuffer;
		bool				_headerComplete;
		size_t				_contentLength;
		size_t				_bodyReceived;
		bool				_responseReady;
		Server*				_server;
		ClientState			_state;
		Request*			_request;
		Response*			_response;

		// Reverse proxy state
		int					_upstreamFd;
		std::string			_upstreamBuffer;
		std::string			_upstreamRequest;
		bool				_upstreamConnecting;

	public:
		Client();
		Client(int fd, Server* server);
		~Client();

	int					getFd() const;
	int					getUpstreamFd() const;
	ClientState			getState() const;
	void				setState(ClientState state);
	Server*				getServer() const;
	void				setUpstreamFd(int fd);		bool				handleRead();
		bool				handleWrite();
		bool				handleUpstreamRead();
		bool				handleUpstreamWrite();

		bool				isResponseReady() const;
		void				buildResponse();
		void				startProxyConnection(const std::string& host, const std::string& port);
		void				handleProxyConnected();
		void				completeProxyResponse();

		bool				headerComplete() const;
		void				parseHeader();
		size_t				parseContentLength(const std::string& headers);

		void				appendToSendBuffer(const std::string& data);
		void				closeClient();
		void				closeUpstream();
		std::string			getRawRequest() const;
};

#endif

