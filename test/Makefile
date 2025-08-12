CC = gcc

SRC_DIR = ../src/malloc/malloctlsf
INCLUDE_DIR = ../include/tlsf
TEST_DIR = .
BUILD_DIR = ./build
LIB_DIR = ./lib

CFLAGS = -std=c99 -g -Wall -Wextra -I$(INCLUDE_DIR) -ffreestanding

LIB_NAME = libtlsf.a
LIB_SRC = $(SRC_DIR)/tlsf.c
LIB_OBJ = $(BUILD_DIR)/tlsf.o
LIB_PATH = $(LIB_DIR)/$(LIB_NAME)

TEST_SRC = $(TEST_DIR)/main.c
TEST_OBJ = $(BUILD_DIR)/main.o
TEST_BIN = tlsf_test

all: $(BUILD_DIR) $(LIB_DIR) $(LIB_PATH) $(TEST_BIN)

$(BUILD_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

# Cria biblioteca estática lib/libtlsf.a
$(LIB_PATH): $(LIB_OBJ)
	ar rcs $@ $^

# Compila binário de teste tlsf_test
$(TEST_BIN): $(TEST_OBJ) $(LIB_PATH)
	$(CC) $(CFLAGS) -o $@ $^

# Compila tlsf.c
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compila main.c
$(BUILD_DIR)/main.o: $(TEST_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) $(TEST_BIN)

