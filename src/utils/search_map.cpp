/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   search_map.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 19:44:54 by taung             #+#    #+#             */
/*   Updated: 2025/11/09 19:48:57 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/utils.h"

/*
	Returns true if the key in found in the map, false otherwise
*/
template <typename K, typename V>
bool	search_map(const std::map<K, V>& m, const K& key) {
	typename std::map<K, V>::const_iterator it = m.find(key);
	if (it != m.end())
		return true;
	else
		return false;
}
