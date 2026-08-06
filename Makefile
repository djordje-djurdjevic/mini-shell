CC := gcc
CFLAGS := -Wall -Wextra -g
TARGET := shell
SRCS := $(wildcard *.c)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)