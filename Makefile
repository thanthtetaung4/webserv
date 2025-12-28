# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: taung <taung@student.42singapore.sg>       +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/10/07 11:32:53 by lshein            #+#    #+#              #
#    Updated: 2025/12/20 02:45:43 by taung            ###   ########.fr        #
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
    ${UTILS}/intToString.cpp ${UTILS}/searchMapLongestMatch.cpp ${UTILS}/searchMapLongestMatchIt.cpp \
	${UTILS}/checkPath.cpp ${UTILS}/parseFile.cpp ${UTILS}/freeAll.cpp ${UTILS}/signal.cpp

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
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(NAME)

clean:
	@$(RM) $(OBJS)
	@echo clean

fclean: clean
	@$(RM) $(NAME)
	@$(RM) -r $(OBJDIR)
	@echo fclean

re: fclean all

.PHONY: all clean fclean re
