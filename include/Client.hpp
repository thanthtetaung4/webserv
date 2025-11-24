/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:04:28 by hthant            #+#    #+#             */
/*   Updated: 2025/11/25 00:49:34 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef CLIENT_HPP
# define CLIENT_HPP

#include <cstddef>
# include <iostream>
# include <ctime>
#include <iterator>

class client {
	public:
		int fd;
		std::string read_buff;
		std::string write_buff;
		time_t last_active;
		size_t server_idx;

		client(int fd, size_t idx);
		void update_activity();
		bool is_timeout(int timeout) const;
		bool has_write_data() const;

};

# endif
