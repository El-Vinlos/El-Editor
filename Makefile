CC = gcc-15
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
OBJ = main.o editor.o terminal.o utils.o
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
