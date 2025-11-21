/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   intToString.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/19 20:54:24 by taung             #+#    #+#             */
/*   Updated: 2025/11/19 20:55:22 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/utils.h"

std::string intToString(size_t n) {
	std::ostringstream ss;
	ss << n;
	return ss.str();
}
