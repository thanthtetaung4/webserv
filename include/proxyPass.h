/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   proxyPass.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 14:34:13 by taung             #+#    #+#             */
/*   Updated: 2025/11/10 14:43:47 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef PROXYPASS_H
# define PROXYPASS_H

# include <iostream>

typedef struct s_proxyPass {
	std::string	host;
	std::string	port;
	std::string	path;
}	t_proxyPass;

# endif
