WEEBITC_SOURCES = weebit.c test.c
WEEBITC_OBJECTS = $(WEEBITC_SOURCES:.c=.o)

CFLAGS = -Wall -Wextra -Werror -pedantic --std=gnu11
CC = gcc

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

weebit: $(WEEBITC_OBJECTS)
	$(CC) $(CFLAGS) $(WEEBITC_OBJECTS) -o weebit

all: weebit
