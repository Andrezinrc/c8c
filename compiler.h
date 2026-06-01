#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>

struct CompilerState {
    uint8_t rom[3584];
    uint16_t PC;
    int pass;
};

#endif
