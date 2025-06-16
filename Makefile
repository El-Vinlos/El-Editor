# if on mac use gcc-15 or other version
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
# editor.o terminal.o utils.o
OBJ = main.o 
TARGET = El-Editor

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

debug: CFLAGS += -g -O0
debug: clean $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean debug
