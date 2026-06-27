TARGET = c8asm
BUILD_DIR = build

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

SRCS = main.c lexer.c parser.c emitter.c reserved_words.c
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR) *.ch8

vim:
	@echo "Installing Vim syntax highlighting..."
	@mkdir -p $(HOME)/.vim/syntax $(HOME)/.vim/ftdetect
	@cp c8asm-theme/syntax/chip8.vim $(HOME)/.vim/syntax/chip8.vim
	@cp c8asm-theme/ftdetect/chip8.vim $(HOME)/.vim/ftdetect/chip8.vim
	@echo "Vim syntax configuration completed."

.PHONY: all clean vim
