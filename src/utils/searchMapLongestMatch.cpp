/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   searchMapLongestMatch.cpp                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/18 18:16:46 by taung             #+#    #+#             */
/*   Updated: 2025/11/18 18:39:44 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/utils.h"

t_location*	searchMapLongestMatch(const std::map<std::string,
									t_location>& map, std::string key) {
	t_location* matched = NULL;
	size_t bestLen = 0;

	for (std::map<std::string, t_location>::const_iterator it = map.begin();
		it != map.end(); ++it)
	{
		if (key.find(it->first) == 0 && it->first.length() > bestLen) {
			matched  = (t_location*)&it->second;
			bestLen  = it->first.length();
		}
	}
	return (matched);
}
