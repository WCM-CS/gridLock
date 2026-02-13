CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -g -Og
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
TARGET = main
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)