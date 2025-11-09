/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 19:48:22 by taung             #+#    #+#             */
/*   Updated: 2025/11/09 19:48:45 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef UTILS_H
# define UTILS_H

# include <iostream>
# include <map>

template <typename K, typename V>
bool	search_map(const std::map<K, V>& m, const K& key);

# endif
