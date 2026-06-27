#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>

#define ROM_START_ADDRESS 0x200
#define MAX_ROM_SIZE 3584

struct Label {
    char name[64];
    uint16_t address;
};

struct RegAlias {
    char name[32];
    char reg_id[8];
};

struct AssemblerState {
    uint8_t rom[MAX_ROM_SIZE];
    uint16_t pc;
    
    struct Label labels[256];
    struct RegAlias aliases[64];
    
    int label_count;
    int alias_count;
    int token_count;

    int current_line;
    int pass;
};

extern struct AssemblerState state;

#endif
