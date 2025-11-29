/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   searchMapLongestMatchIt.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 18:10:46 by taung             #+#    #+#             */
/*   Updated: 2025/11/24 20:38:10 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/utils.h"

/*
	Search in the map and return the IT with the longest match
	@ param map, key
	@ return it of the longest match
	@ return end() if cannot find
*/
std::map<std::string, t_location>::const_iterator
searchMapLongestMatchIt(const std::map<std::string, t_location>& map, std::string key)
{
	std::map<std::string, t_location>::const_iterator matched = map.end();
	size_t bestLen = 0;

	for (std::map<std::string, t_location>::const_iterator it = map.begin();
			it != map.end(); ++it)
	{
		if (key.find(it->first) == 0 && it->first.length() > bestLen) {
			matched = it;
			bestLen = it->first.length();
		}
	}

	return matched;
}

