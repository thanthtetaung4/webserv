/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/10/07 12:28:35 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../include/WebServer.hpp"

WebServer::WebServer(){}

WebServer::~WebServer(){}

// WebServer::WebServer(const WebServer &src) {}

// WebServer &WebServer::operator=(const WebServer &other) {}

void printRange(std::string::iterator it1, std::string::iterator it2)
{
    if (it1 == it2) {
        std::cout << "(empty range)" << std::endl;
        return;
    }

    for (std::string::iterator it = it1; it != it2; ++it) {
        std::cout << *it;
    }
    std::cout << std::endl;
}

void WebServer::setServer(std::string configFile)
{
	std::string::iterator it1;
	std::string::iterator it2;
	std::string target = "server {";

	std::ifstream config(configFile.c_str());
	if (!config)
		throw "Unable to open file!!";
	else
	{
		std::stringstream ss;
		ss << config.rdbuf();
		std::string content = ss.str();
		it1 = std::search(content.begin(), content.end(), target.begin(), target.end());
		if (it1 == content.end())
			throw "Invalid config file";
		while (it1 != content.end())
		{
			it2 = std::search(it1 + 1, content.end(), target.begin(), target.end());
			//substr here
			printRange(it1 + target.size() - 2, it2);
			if (it2 != content.end())
				it1 = it2;
			else
				break;
		}
		config.close();
	}
}

// void parser(std::string config, int type)
// {
// }

