CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -O2 -g -pthread
TARGET = lab05

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run valgrind clean
