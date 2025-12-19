/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/20 02:24:13 by taung             #+#    #+#             */
/*   Updated: 2025/12/20 02:46:07 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include <csignal>
# include <iostream>

// Global flag to signal graceful shutdown
volatile sig_atomic_t g_shutdown = 0;

void	handleSignal(int signum) {
	if (signum == SIGINT) {
		std::cout << "\nReceived SIGINT - shutting down gracefully..." << std::endl;
		g_shutdown = 1;
	}
	else if (signum == SIGTERM) {
		std::cout << "\nReceived SIGTERM - shutting down gracefully..." << std::endl;
		g_shutdown = 1;
	}
}
