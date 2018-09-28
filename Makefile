SOURCES = weebitc.c weebitc_test.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS = -Wall -Wextra -Werror -pedantic --std=gnu11
CC = gcc

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

weebitc: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o weebitc

all: weebitc

test: all
	python tests.py
