SRC = tests_ticket_lock.cpp ../../../Concurrency/TicketLock/RWTicketLock.cpp \
	../../../Concurrency/TicketLock/Spin.cpp ../ThreadPool/ThreadPool.cpp

NAME = test

OBJ = $(SRC:.cpp=.o)

CPPFLAGS = -std=c++20 -Wall -Wextra -Werror -g -pthread -lpthread -fsanitize=thread -I./ -I../ -I../../ \
	-I../../../Concurrency/TicketLock/ -I../ThreadPool/

$(NAME): $(SRC)
	g++ $(CPPFLAGS) $(SRC) -o $(NAME)

all: $(NAME)

.PHONY: test clean re

test: all ./test

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all