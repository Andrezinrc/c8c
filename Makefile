TARGET = c8asm
BUILD_DIR = build

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

SRCS = main.c lexer.c parser.c reserved_words.c
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR) *.ch8

.PHONY: all clean
