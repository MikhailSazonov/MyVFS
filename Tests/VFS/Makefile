SRC = tests_vfs.cpp ../../CacheManager.cpp ../../FileManager.cpp \
	../../Journal.cpp ../../MyVFS.cpp ../../SegmentSystem.cpp \
	../../Concurrency/TicketLock/RWTicketLock.cpp ../../DirTree.cpp \
	../../Concurrency/TicketLock/Spin.cpp ../Concurrency/ThreadPool/ThreadPool.cpp

NAME = test

FILE1 = 0
FILE2 = 1
FILE3 = 2
FILE4 = 3

OBJ = $(SRC:.cpp=.o)

CPPFLAGS = -std=c++20 -Wall -Wextra -Werror -g -fsanitize=address -pthread -lpthread -I./ -I../ -I../../ -I../Concurrency/ThreadPool/

$(NAME): $(SRC)
	g++ $(CPPFLAGS) $(SRC) -o $(NAME)

all: $(NAME)

test: all ./test

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME) $(FILE1) $(FILE2) $(FILE3) $(FILE4)

re: fclean all

.PHONY: all test clean re