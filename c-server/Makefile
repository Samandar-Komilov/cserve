CC = gcc
CFLAGS = -Wall -Wextra -g
LDLIBS = -lcheck
SRC_DIR = src
TEST_DIR = tests
OBJ_DIR = obj
BIN_DIR = bin

SRC = $(wildcard $(SRC_DIR)/**/*.c) $(SRC_DIR)/Server.c
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC:.c=.o))
TEST_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_SRC))

TARGET = $(BIN_DIR)/test_runner

# Object rule
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

all: $(TARGET)

$(TARGET): $(SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full ./$(TARGET)

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

.PHONY: all test valgrind clean