#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "assembler.h"
#include "parser.h"

struct AssemblerState state;

static void reset_assembler_pass(int pass)
{
    state.pass = pass;
    state.current_line = 1;
    state.pc = ROM_START_ADDRESS;
    state.token_count = 0;
    memset(state.rom, 0, MAX_ROM_SIZE);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("%s <source.c8> <output.ch8>\n", argv[0]);
        return 1;
    }

    FILE* src = fopen(argv[1], "r");
    if (!src) {
        perror("Error opening source file");
        return 1;
    }

    // Pass 1: Labels and aliases
    reset_assembler_pass(1);
    state.label_count = 0;
    parse_source(src);

    // Pass 2: Bytecode generation
    rewind(src);
    reset_assembler_pass(2);
    parse_source(src);
    fclose(src);

    FILE* out = fopen(argv[2], "wb");
    if (!out) {
        perror("Error creating output file");
        return 1;
    }

    uint16_t bytes_written = state.pc - ROM_START_ADDRESS;
    fwrite(state.rom, 1, bytes_written, out);
    fclose(out);

    printf("\033[1;32mSuccess! \033[0m\033[1;37m%d tokens processed.\n%d bytes written to %s\033[0m\n", state.token_count, bytes_written, argv[2]);

    return 0;
}
