/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 14:06:54 by taung             #+#    #+#             */
/*   Updated: 2025/11/09 21:07:50 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef SOCKET_HPP
# define SOCKET_HPP

# include <arpa/inet.h>
# include <iostream>

class Socket {
	private:
		int	serverFd;
		sockaddr_in	addr;
		socklen_t	addr_len;
	public:
		Socket();
		Socket(unsigned int port);
		~Socket();
		void				setServerFd(int fd);
		int					getServerFd(void) const;
		void				openSock(void);
		void				bindSock(void);
		void				listenSock(void);
		void				prepSock(void);
		sockaddr_in*		getAddrPtr(void);
		socklen_t*			getAddrLen(void);
};
std::ostream&	operator<<(std::ostream& os, const Socket& sock);

# endif
