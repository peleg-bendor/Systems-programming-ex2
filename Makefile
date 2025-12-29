# Makefile for my_copy program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99
TARGET = my_copy

# Default target: build the executable
all: $(TARGET)

# Build rule: compile my_copy.c into executable
$(TARGET): my_copy.c
	$(CC) $(CFLAGS) my_copy.c -o $(TARGET)

# Clean rule: remove the executable
clean:
	rm -f $(TARGET)

# Phony targets (not actual files)
.PHONY: all clean