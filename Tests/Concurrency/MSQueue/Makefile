SRC = tests_ms_queue.cpp ../ThreadPool/ThreadPool.cpp

NAME = test

OBJ = $(SRC:.cpp=.o)

CPPFLAGS = -std=c++20 -Wall -Wextra -Werror -g -pthread -lpthread -fsanitize=thread -I./ -I../ -I../../ \
	-I../../../Concurrency/MSQueue/ -I../ThreadPool/

$(NAME): $(SRC)
	g++ $(CPPFLAGS) $(SRC) -o $(NAME)

all: $(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all fclean re
