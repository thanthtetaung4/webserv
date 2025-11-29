# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/10/07 11:32:53 by lshein            #+#    #+#              #
#    Updated: 2025/11/29 17:06:36 by taung            ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ./webserv

CLASSES = ./src/classes
EXCEPTIONS = ./src/exceptions
UTILS = ./src/utils

SRCS = ./src/main.cpp \
	${CLASSES}/WebServer.cpp ${CLASSES}/Server.cpp ${CLASSES}/Socket.cpp ${CLASSES}/Cgi.cpp\
	${CLASSES}/Response.cpp ${CLASSES}/Request.cpp ${CLASSES}/Validator.cpp\
	${EXCEPTIONS}/ServerExceptions.cpp ${UTILS}/parseProxyPass.cpp ${UTILS}/buildPath.cpp \
	${UTILS}/intToString.cpp ${UTILS}/searchMapLongestMatch.cpp ${UTILS}/searchMapLongestMatchIt.cpp ${UTILS}/checkPath.cpp
OBJS = ${SRCS:.cpp=.o}

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $(<:.cpp=.o)

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(NAME)

clean:
	@$(RM)  $(OBJS)

fclean: clean
	@$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
