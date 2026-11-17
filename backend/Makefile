CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
LDLIBS   := -lcurl
SRC      := src/main.cpp
HDRS     := $(wildcard include/*.hpp)
BIN      := build/gt_trmnl

all: $(BIN)

$(BIN): $(SRC) $(HDRS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDLIBS)

clean:
	rm -rf build

.PHONY: all clean
