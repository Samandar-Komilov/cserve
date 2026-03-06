BUILD_DIR := build
CMAKE     := cmake
BINARY    := $(BUILD_DIR)/cserve

.PHONY: all build release configure test docs clean rm help

# Default target
all: build

## configure  — run CMake configuration (Debug mode)
configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

## build       — configure (if needed) + compile
build: $(BUILD_DIR)/CMakeCache.txt
	$(CMAKE) --build $(BUILD_DIR) --parallel

## release     — configure in Release mode + compile
release:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(BUILD_DIR) --parallel

## run         — build then start the server
run: build
	./$(BINARY)

## test        — build then run the test suite
test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

## docs        — build Doxygen HTML documentation
docs: build
	$(CMAKE) --build $(BUILD_DIR) --target docs

## clean       — remove compiled objects, keep CMake cache
clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean

## rm          — delete the entire build directory
rm:
	rm -rf $(BUILD_DIR)

## help        — show this message
help:
	@grep -E '^##' Makefile | sed 's/## /  /'

# Internal: only re-run cmake configure when CMakeCache.txt is absent
$(BUILD_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
