CC = gcc
CFLAGS = -Wall -Wextra -g
LDLIBS = -lcheck

# Directories
SRC_DIR = src
TEST_DIR = tests
OBJ_DIR = obj
BIN_DIR = bin

# Files
APP_SRC = $(SRC_DIR)/main.c $(SRC_DIR)/Server.c        # your main program
LIB_SRC = $(wildcard $(SRC_DIR)/DataStructures/*.c)    # dynamic array and other libs
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)                 # test files

APP_OBJ = $(APP_SRC:.c=.o)
LIB_OBJ = $(LIB_SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)

APP_BIN = $(BIN_DIR)/server_app
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

valgrind: test
	valgrind --leak-check=full ./$(TEST_BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
