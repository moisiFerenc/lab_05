# ============================================================================
#  LAB 05 - Makefile
# ============================================================================
#
#  Használat:
#    make          - Fordítás
#    make run      - Futtatás
#    make valgrind - Memória ellenőrzés
#    make clean    - Takarítás
#
# ============================================================================

CC       = gcc
CFLAGS   = -Wall -Wextra -Wpedantic -std=c11 -O2 -g -pthread
TARGET   = lab05

# Alapértelmezett target
all: $(TARGET)

# Fordítás
$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

# Futtatás
run: $(TARGET)
	./$(TARGET)

# Valgrind ellenőrzés (memória szivárgás)
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Takarítás
clean:
	rm -f $(TARGET)

.PHONY: all run valgrind clean
