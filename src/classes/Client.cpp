/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hthant <hthant@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/25 00:42:58 by hthant            #+#    #+#             */
/*   Updated: 2025/11/25 00:51:21 by hthant           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/Client.hpp"
#include <cstddef>
#include <ctime>
#include <unistd.h>

client::client(int fd, size_t idx){
	this->server_idx = idx;
	this->fd = fd;
	this->read_buff = "";
	this->write_buff = "";
	this->last_active = time(NULL);
}
void client::update_activity(){
	last_active = time(NULL);
}

bool client::is_timeout(int timeout) const {
	return difftime(time(NULL), last_active) > timeout;
}

bool client::has_write_data() const {
	return !write_buff.empty();
}
