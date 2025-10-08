/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:51:13 by lshein            #+#    #+#             */
/*   Updated: 2025/10/08 12:11:28 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../include/WebServer.hpp"

WebServer::WebServer(){}

WebServer::~WebServer(){}

// WebServer::WebServer(const WebServer &src) {}

// WebServer &WebServer::operator=(const WebServer &other) {}

t_its getIts(std::string &content, std::string::iterator start, const std::string &target)
{
	t_its it;
	
	it.it1 = std::search(start, content.end(), target.begin(), target.end());
	if (it.it1 == content.end())
		throw "Invalid config file";
	else
		it.it2 = std::search(it.it1 + 1, content.end(), target.begin(), target.end());
	return it;
}

void getServerBlock(t_its it, std::vector<Server> servers)
{
	std::string content(it.it1, it.it2);
	std::stringstream serverString(content);
	std::string line;
	Server server;

	while (std::getline(serverString, line))
	{
		std::stringstream ss(line);              // Tokenize line
		std::string token;
		std::vector<std::string> lineVec;
		while (ss >> token)
		{
			if (!token.empty() && !(token.at(token.size() - 1) == ';'))
				lineVec.push_back(token);
			else if (!token.empty() && (token.at(token.size() - 1) == ';'))
			{
				token = token.substr(0, token.size() - 1);
				lineVec.push_back(token);
				setAttributes(lineVec, server);
				lineVec.clear();
			}
		}
	}
	servers.push_back(server);
	std::cout << server;
}

void setAttributes(const std::vector<std::string>& line, Server& server)
{
    if (line.empty()) return;
    if (line[0] == "listen" && line.size() >= 2)
        server.setPort(line[1]);
    else if (line[0] == "server_name" && line.size() >= 2)
        server.setServerName(line[1]);
    else if (line[0] == "error_page" && line.size() >= 3)
        server.setErrorPage(line[1], line[2]);
	else if (line[0] == "client_max_body_size" && line.size() >= 2)
        server.setMaxBytes(line[1]);
}

void WebServer::setServer(std::string configFile)
{
	t_its it;
	std::string target = "server {";

	std::ifstream config(configFile.c_str());
	if (!config)
		throw "Unable to open file!!";
	else
	{
		std::stringstream ss;
		ss << config.rdbuf();
		std::string content = ss.str();
		it = getIts(content, content.begin(), target);
		while (it.it1 != content.end())
		{
			getServerBlock(it, _servers);
			if (it.it2 != content.end())
				it = getIts(content, it.it2, target);
			else
				break;
		}
		config.close();
	}
}
