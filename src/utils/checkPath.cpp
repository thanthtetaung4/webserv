/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   checkPath.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 17:22:43 by lshein            #+#    #+#             */
/*   Updated: 2025/11/24 17:22:44 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/utils.h"

bool isDirectory(const std::string &path)
{
	struct stat s;
	if (stat(path.c_str(), &s) == -1)
		return false; // or handle error
	return S_ISDIR(s.st_mode);
}

bool isRegularFile(const std::string &path)
{
	struct stat s;
	if (stat(path.c_str(), &s) == -1)
		return false;
	return S_ISREG(s.st_mode);
}
