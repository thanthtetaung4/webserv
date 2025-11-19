/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 19:48:22 by taung             #+#    #+#             */
/*   Updated: 2025/11/19 20:55:37 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef UTILS_H
# define UTILS_H

# include <iostream>
# include <sstream>
# include <map>
# include "proxyPass.h"
# include "Location.h"

template <typename K, typename V>
bool	search_map(const std::map<K, V>& m, const K& key) {
	typename std::map<K, V>::const_iterator it = m.find(key);
	if (it != m.end())
		return true;
	else
		return false;
}

template <typename K, typename V>
typename std::map<K, V>::const_iterator search_map_iterator(const std::map<K, V>& m, const K& key)  {
	typename std::map<K, V>::const_iterator it = m.find(key);
	return it;
}

t_location*	searchMapLongestMatch(const std::map<std::string,
									t_location>& map, std::string key);

t_proxyPass	parseProxyPass(const std::string &proxyPassStr);
std::string	intToString(size_t n);

# endif
