WEEBITC_SOURCES = weebitc.c weebitc_test.c
WEEBITC_OBJECTS = $(WEEBITC_SOURCES:.c=.o)

WEEBITCPP_SOURCES = weebitcpp.cpp weebitcpp_test.cpp
WEEBITCPP_OBJECTS = $(WEEBITCPP_SOURCES:.cpp=.o)

CFLAGS = -Wall -Wextra -Werror -pedantic --std=gnu11
CXXFLAGS = -Wall -Wextra -Werror -pedantic -I rapidjson/include --std=gnu++17
CC = gcc
CXX = g++

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

weebitc: $(WEEBITC_OBJECTS)
	$(CC) $(CFLAGS) $(WEEBITC_OBJECTS) -o weebitc

weebitcpp: $(WEEBITCPP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(WEEBITCPP_OBJECTS) -o weebitcpp

test_progs: weebitc weebitcpp
all: test_progs

test: test_progs
	python tests.py
