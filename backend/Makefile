CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -02 -Iinclude
LDLIBS := -Icurl
SRC := src/main.cpp
HDRS := $(wildcard include/*.hpp)
BIN := build/gt_trmnl

all: $(BIN)

$(BIN) :$(SRC) $(HDRS)
 @mkdir -p build
 $(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDLIBS)

clean:
	rm -f build

.PHONY: all clean
