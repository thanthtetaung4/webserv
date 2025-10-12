/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerException.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 19:03:46 by taung             #+#    #+#             */
/*   Updated: 2025/10/11 02:51:50 by taung            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# ifndef SERVEXCEPTION_HPP
# define SERVEXCEPTION_HPP

# include <exception>

class InvalidSocket : public std::exception {
	public:
	virtual const char* what() const throw();
};

class UnableToOpenSocket : public std::exception {
	public:
	virtual const char* what() const throw();
};

class UnableToBind : public std::exception {
	public:
	virtual const char* what() const throw();
};

class UnableToListen : public std::exception {
	public:
	virtual const char* what() const throw();
};

# endif
