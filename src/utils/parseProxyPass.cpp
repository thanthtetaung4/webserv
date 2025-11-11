/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parseProxyPass.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 14:36:13 by taung             #+#    #+#             */
/*   Updated: 2025/11/10 19:14:41 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/proxyPass.h"

t_proxyPass	parseProxyPass(const std::string &proxyPassStr) {
	t_proxyPass	pp;
	size_t		pos;
	size_t		start;

	// Example proxyPassStr: "http://
	pos = proxyPassStr.find("://");
	if (pos == std::string::npos)
		throw std::runtime_error("Invalid proxyPass format: missing '://'");
	start = pos + 3;
	pos = proxyPassStr.find(':', start);
	if (pos == std::string::npos)
		throw std::runtime_error("Invalid proxyPass format: missing port");
	pp.host = proxyPassStr.substr(start, pos - start);
	start = pos + 1;
	pos = proxyPassStr.find('/', start);
	if (pos == std::string::npos)
		throw std::runtime_error("Invalid proxyPass format: missing path");
	pp.port = proxyPassStr.substr(start, pos - start);
	pp.path = proxyPassStr.substr(pos);
	return pp;
}
