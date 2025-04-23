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
APP_SRC = $(SRC_DIR)/main.c $(SRC_DIR)/Server.c $(SRC_DIR)/HTTPServer.c        # your main program
LIB_SRC = $(wildcard $(SRC_DIR)/DataStructures/*.c)    # dynamic array and other libs
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)                 # test files

APP_OBJ = $(APP_SRC:.c=.o)
LIB_OBJ = $(LIB_SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)

APP_BIN = $(BIN_DIR)/cserver
TEST_BIN = $(BIN_DIR)/test_runner

.PHONY: all app test clean valgrind

all: app test

app: $(APP_BIN)

test: $(TEST_BIN)

# Application build
$(APP_BIN): $(APP_SRC) $(LIB_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ 

# Test runner build
$(TEST_BIN): $(TEST_SRC) $(LIB_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

run: app
	./$(APP_BIN)

runtest: test
	./$(TEST_BIN)

valgrind: test
	valgrind --leak-check=full ./$(TEST_BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
