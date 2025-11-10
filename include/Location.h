/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 05:09:26 by lshein            #+#    #+#             */
/*   Updated: 2025/11/10 00:15:15 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <iostream>
#include <vector>
#include <map>

typedef struct location
{
        std::string _root;
        std::vector<std::string> _index;
        std::vector<std::string> _limit_except;
        std::map<std::string, std::string> _return;
        std::string _autoIndex;
        std::string _cgiPass;
        std::string _cgiExt;
        std::string _uploadStore;
        std::string _proxyPass;
} t_location;


#endif
