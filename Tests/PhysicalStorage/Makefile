SRC = tests_storage.cpp ../../FileManager.cpp ../../CacheManager.cpp ../../Journal.cpp \
	../../Concurrency/TicketLock/Spin.cpp ../Concurrency/ThreadPool/ThreadPool.cpp \
	../../SegmentSystem.cpp ../../Concurrency/TicketLock/RWTicketLock.cpp

NAME = test

FILE = storage_test

OBJ = $(SRC:.cpp=.o)

CPPFLAGS = -std=c++20 -Wall -Wextra -Werror -g -pthread -lpthread -I./ -I../ -I../../ -I../Concurrency/ThreadPool/

$(NAME): $(SRC)
	g++ $(CPPFLAGS) $(SRC) -o $(NAME)

all: $(NAME)

test: all ./test

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME) $(FILE)

re: fclean all

.PHONY: all test clean re