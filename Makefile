CXX := g++
BUILD_DIR := build
SRC := $(shell find src -name '*.cpp')
TARGET := $(BUILD_DIR)/bench
DEBUG_TARGET := $(BUILD_DIR)/bench_debug
CXXFLAGS := -std=c++20 -O3 -march=native -pthread
DEBUG_CXXFLAGS := -std=c++20 -O0 -g -march=native -pthread

.PHONY: build clean

build:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

debug:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) $(SRC) -o $(DEBUG_TARGET)

clean:
	rm -rf $(BUILD_DIR)
