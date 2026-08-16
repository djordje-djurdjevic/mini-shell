CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
TARGET = shell
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f src/*.o $(TARGET)

.PHONY: run clean