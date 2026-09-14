CXX = clang++
COMMON_CXXFLAGS := -std=c++2c -Wall -Wextra
CXXFLAGS ?= $(COMMON_CXXFLAGS) -O3 -DNDEBUG
DEBUG_CXXFLAGS ?= $(COMMON_CXXFLAGS) -O0 -g
LDFLAGS ?=

TARGET := 1brc
DEBUG_TARGET := 1brc.dbg
SRC := main.cpp

.PHONY: all debug clean

all: $(TARGET)

debug: $(SRC)
	$(CXX) $(DEBUG_CXXFLAGS) $< -o $(DEBUG_TARGET) $(LDFLAGS)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)
