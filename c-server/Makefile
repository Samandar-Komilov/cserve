CC = gcc
CFLAGS = -Wall -Wextra -g
LDLIBS = -lcheck

# Directories
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Files
APP_SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/sock/*.c) $(wildcard $(SRC_DIR)/http/*.c) $(wildcard $(SRC_DIR)/utils/*.c) $(wildcard $(SRC_DIR)/configs/*.c)
TEST_SRC = $(wildcard $(TEST_DIR)/*.c) $(wildcard $(SRC_DIR)/http/*.c) $(wildcard $(SRC_DIR)/sock/*.c) $(wildcard $(SRC_DIR)/utils/*.c) $(wildcard $(SRC_DIR)/configs/*.c)

APP_OBJ = $(APP_SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)

APP_BIN = $(BIN_DIR)/cserver
TEST_BIN = $(BIN_DIR)/test_runner

.PHONY: all app test runtest clean valgrindtest valgrindmain

all: app test

app: $(APP_BIN)

test: $(TEST_BIN)

# Application build
$(APP_BIN): $(APP_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $@ $^ 

# Test runner build
$(TEST_BIN): $(TEST_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $@ $^ $(LDLIBS)

run: app
	./$(APP_BIN)

runtest: test
	./$(TEST_BIN)

valgrindtest: test
	valgrind --leak-check=full ./$(TEST_BIN)

valgrindmain: app
	valgrind --leak-check=full ./$(APP_BIN) -s

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
