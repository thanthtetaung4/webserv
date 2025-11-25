# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/10/07 11:32:53 by lshein            #+#    #+#              #
#    Updated: 2025/11/25 21:04:30 by hthant           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv

OBJDIR = obj

CLASSES = src/classes
EXCEPTIONS = src/exceptions
UTILS = src/utils

SRCS = src/main.cpp \
    ${CLASSES}/WebServer.cpp ${CLASSES}/Server.cpp ${CLASSES}/Socket.cpp ${CLASSES}/Cgi.cpp \
    ${CLASSES}/Response.cpp ${CLASSES}/Request.cpp ${CLASSES}/Validator.cpp ${CLASSES}/Client.cpp \
    ${EXCEPTIONS}/ServerExceptions.cpp ${UTILS}/parseProxyPass.cpp ${UTILS}/buildPath.cpp \
    ${UTILS}/intToString.cpp ${UTILS}/searchMapLongestMatch.cpp

OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

# Create folder structure
$(OBJDIR):
	mkdir -p $(OBJDIR)/src/classes
	mkdir -p $(OBJDIR)/src/exceptions
	mkdir -p $(OBJDIR)/src/utils

# Compile sources into obj/ folder
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(NAME)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
