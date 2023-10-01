# Compiler and flags
CC = gcc
CFLAGS = -Wall -I./lib
LDFLAGS = -L./build
LIBS = -lthreadpool

# Directories
SRC_DIR = lib
EXAMPLES_DIR = examples
BUILD_DIR = build

# List of source files
LIB_SRCS = $(wildcard $(SRC_DIR)/*.c)
EXAMPLE_SRCS = $(wildcard $(EXAMPLES_DIR)/*.c)

# Object files
LIB_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))
EXAMPLE_OBJS = $(patsubst $(EXAMPLES_DIR)/%.c,$(BUILD_DIR)/%.o,$(EXAMPLE_SRCS))

# Executables
EXAMPLES = $(patsubst $(EXAMPLES_DIR)/%.c,$(BUILD_DIR)/%,$(EXAMPLE_SRCS))

# Targets
all: lib $(EXAMPLES)

lib: $(LIB_OBJS) 
	ar rcs $(BUILD_DIR)/libthreadpool.a $(LIB_OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

$(BUILD_DIR)/%: $(EXAMPLES_DIR)/%.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all lib clean

lint:
	clang-format -i lib/*.c lib/*.h examples/*.c
