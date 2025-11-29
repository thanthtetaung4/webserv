/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parseProxyPass.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 14:36:13 by taung             #+#    #+#             */
/*   Updated: 2025/11/25 02:56:46 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../../include/proxyPass.h"

t_proxyPass	parseProxyPass(const std::string &proxyPassStr) {
	t_proxyPass	pp;
	size_t		pos;
	size_t		start;

	pos = proxyPassStr.find("://");
	if (pos == std::string::npos)
		throw std::runtime_error("Invalid proxyPass format: missing '://'");
	std::string scheme = proxyPassStr.substr(0, pos);
	start = pos + 3;

	// authority is host[:port] before the first '/'
	size_t slash = proxyPassStr.find('/', start);
	std::string authority;
	if (slash == std::string::npos)
		authority = proxyPassStr.substr(start);
	else
		authority = proxyPassStr.substr(start, slash - start);

	// parse optional :port (ignore IPv6)
	size_t colon = authority.rfind(':');
	if (colon == std::string::npos) {
		pp.host = authority;
		pp.port = (scheme == "https" ? "443" : "80");
	} else {
		pp.host = authority.substr(0, colon);
		pp.port = authority.substr(colon + 1);
	}

	if (pp.host.empty())
		throw std::runtime_error("Invalid proxyPass format: empty host");

	// path (default to "/")
	if (slash == std::string::npos)
		pp.path = "/";
	else {
		pp.path = proxyPassStr.substr(slash);
		if (pp.path.empty())
			pp.path = "/";
	}

	return pp;
}
