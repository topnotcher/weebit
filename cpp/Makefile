
WEEBITCPP_SOURCES = test.cpp
WEEBITCPP_OBJECTS = $(WEEBITCPP_SOURCES:.cpp=.o)

CXXFLAGS = -Wall -Wextra -Werror -pedantic -I --std=gnu++17 -I ../rapidjson/include
CXX = g++

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

weebit: $(WEEBITCPP_OBJECTS)
	$(CXX) $(CXXFLAGS) $(WEEBITCPP_OBJECTS) -o weebit

all: weebit
