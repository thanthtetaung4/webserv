/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 05:09:26 by lshein            #+#    #+#             */
/*   Updated: 2025/10/07 10:27:16 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <iostream>
#include <vector>
#include <map>

class Location
{
        std::string _root;
        std::string _index;
        std::vector<std::string> _limit_except; 
        std::map<std::string, std::string> _return;
        std::string _autoIndex;
        std::string _cgiPass;
        std::string _cgiExt;
        std::string _uploadStore;       
};

#endif
