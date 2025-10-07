/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lshein <lshein@student.42singapore.sg>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/07 07:41:50 by lshein            #+#    #+#             */
/*   Updated: 2025/10/07 11:38:16 by lshein           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./../include/WebServer.hpp"

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        WebServer ws;
        ws.setServer(argv[1]);
    }
    else
        std::cerr << "Usage: ./webserv [config file]" << std::endl;
}
