CXX := g++
BUILD_DIR := build
SRC := $(shell find src -name '*.cpp')
TARGET := $(BUILD_DIR)/bench
CXXFLAGS := -std=c++20 -O3 -march=native -pthread

.PHONY: build clean

build:
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf $(BUILD_DIR)