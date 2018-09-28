SOURCES = weebit.c main.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS = -Wall -Wextra -Werror -pedantic --std=gnu11
CC = gcc

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

weebit_c: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o weebit_c

all: weebit_c

test: all
	python tests.py
