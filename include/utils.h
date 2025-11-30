/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 19:48:22 by taung             #+#    #+#             */
/*   Updated: 2025/12/01 04:49:18 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <map>
#include "proxyPass.h"
#include "Location.h"
#include <sys/stat.h>
#include <string>

template <typename K, typename V>
bool search_map(const std::map<K, V> &m, const K &key)
{
	typename std::map<K, V>::const_iterator it = m.find(key);
	if (it != m.end())
		return true;
	else
		return false;
}

template <typename K, typename V>
typename std::map<K, V>::const_iterator search_map_iterator(const std::map<K, V> &m, const K &key)
{
	typename std::map<K, V>::const_iterator it = m.find(key);
	return it;
}

t_location *searchMapLongestMatch(const std::map<std::string,
												 t_location> &map,
								  std::string key);

std::map<std::string, t_location>::const_iterator searchLongestMatch(const std::map<std::string,
																					t_location> &map,
																	 std::string key);
bool isRegularFile(const std::string &path);
bool isDirectory(const std::string &path);
t_proxyPass parseProxyPass(const std::string &proxyPassStr);
std::string intToString(size_t n);

bool	parseFile(std::string body, std::string contentType, std::string &fileName, std::string &fileContent);
bool	parseFile(std::string body, std::string contentType, std::string &fileName);

#endif
