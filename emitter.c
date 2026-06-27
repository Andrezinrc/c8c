#include "emitter.h"
#include "assembler.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void emit_instruction(uint16_t op)
{
    if ((state.pc - ROM_START_ADDRESS) + 1 >= MAX_ROM_SIZE) {
        fprintf(stderr, "[ERROR] ROM limit exceeded.\n");
        exit(1);
    }
    state.rom[state.pc - ROM_START_ADDRESS] = (op >> 8) & 0xFF;
    state.rom[state.pc - ROM_START_ADDRESS + 1] = op & 0xFF;
    state.pc += 2;
}

void emit_byte(uint8_t byte)
{
    if (state.pc - ROM_START_ADDRESS >= MAX_ROM_SIZE) {
        fprintf(stderr, "[ERROR] ROM limit exceeded.\n");
        exit(1);
    }
    state.rom[state.pc - ROM_START_ADDRESS] = byte;
    state.pc += 1;
}


// Instruction Handles

void handle_cls(FILE *src, const char *mnemonic)
{
    (void)src; (void)mnemonic;
    emit_instruction(0x00E0);
}

void handle_ret(FILE *src, const char *mnemonic)
{
    (void)src; (void)mnemonic;
    emit_instruction(0x00EE);
}

void handle_sys(FILE *src, const char *mnemonic)
{
    (void)mnemonic;
    char op[32];
    get_operand(src, op, sizeof(op));
    emit_instruction(0x0000 | resolve_address(op));
}

void handle_jp(FILE *src, const char *mnemonic)
{
    (void)mnemonic;
    char op1[32], op2[32];
    get_operand(src, op1, sizeof(op1));

    if (strcmp(op1, "v0") == 0) {
        get_operand(src, op2, sizeof(op2));
        emit_instruction(0xB000 | resolve_address(op2));
    } else {
        emit_instruction(0x1000 | resolve_address(op1));
    }
}

void handle_call(FILE *src, const char *mnemonic)
{
    (void)mnemonic;
    char op[32];
    get_operand(src, op, sizeof(op));
    emit_instruction(0x2000 | resolve_address(op));
}

void handle_se_sne(FILE *src, const char *mnemonic)
{
    char op1[32], op2[32];
    get_operand(src, op1, sizeof(op1));
    get_operand(src, op2, sizeof(op2));
    
    if (!IS_REG(op1))
        error_invalid_reg(mnemonic);
    int regX = PARSE_REG(op1);

    if (IS_REG(op2)) {
        int regY = PARSE_REG(op2);
        uint16_t prefix = (strcmp(mnemonic, "se") == 0) ? 0x5000 : 0x9000;
        emit_instruction(prefix | (regX << 8) | (regY << 4));
    } else {
        uint8_t val = (uint8_t)strtol(op2, NULL, 0);
        uint16_t prefix = (strcmp(mnemonic, "se") == 0) ? 0x3000 : 0x4000;
        emit_instruction(prefix | (regX << 8) | val);
    }
}

void handle_skp_sknp(FILE *src, const char *mnemonic)
{
    char op[32];
    get_operand(src, op, sizeof(op));
    if (!IS_REG(op))
        error_invalid_reg(mnemonic);
    
    int regX = PARSE_REG(op);
    uint16_t suffix = (strcmp(mnemonic, "skp") == 0) ? 0x009E : 0x00A1;
    emit_instruction(0xE000 | (regX << 8) | suffix);
}

void handle_ld(FILE *src, const char *mnemonic)
{
    char op1[32], op2[32];
    get_operand(src, op1, sizeof(op1));
    get_operand(src, op2, sizeof(op2));

    if (strcmp(op1, "i") == 0) {
        emit_instruction(0xA000 | resolve_address(op2));
        return;
    }
    if (strcmp(op1, "dt") == 0) {
        if (!IS_REG(op2))
            error_invalid_reg(mnemonic);
        emit_instruction(0xF015 | (PARSE_REG(op2) << 8));
        return;
    }
    if (strcmp(op1, "st") == 0) {
        if (!IS_REG(op2))
            error_invalid_reg(mnemonic);
        emit_instruction(0xF018 | (PARSE_REG(op2) << 8));
        return;
    }
    if (strcmp(op1, "f") == 0) {
        if (!IS_REG(op2))
            error_invalid_reg(mnemonic);
        emit_instruction(0xF029 | (PARSE_REG(op2) << 8));
        return;
    }
    if (strcmp(op1, "b") == 0) {
        if (!IS_REG(op2))
            error_invalid_reg(mnemonic);
        emit_instruction(0xF033 | (PARSE_REG(op2) << 8));
        return;
    }
    if (strcmp(op1, "[i]") == 0) {
        if (!IS_REG(op2))
            error_invalid_reg(mnemonic);
        emit_instruction(0xF055 | (PARSE_REG(op2) << 8));
        return;
    }

    if (!IS_REG(op1))
        error_invalid_reg(mnemonic);
    int regX = PARSE_REG(op1);

    if      (strcmp(op2, "dt")  == 0)
        emit_instruction(0xF007 | (regX << 8));
    else if (strcmp(op2, "k")   == 0)
        emit_instruction(0xF00A | (regX << 8));
    else if (strcmp(op2, "[i]") == 0)
        emit_instruction(0xF065 | (regX << 8));
    else if (IS_REG(op2))
        emit_instruction(0x8000 | (regX << 8) | (PARSE_REG(op2) << 4));
    else
        emit_instruction(0x6000 | (regX << 8) | (uint8_t)strtol(op2, NULL, 0));
}

void handle_add(FILE *src, const char *mnemonic)
{
    char op1[32], op2[32];
    get_operand(src, op1, sizeof(op1));
    get_operand(src, op2, sizeof(op2));

    if (strcmp(op1, "i") == 0) {
        if (!IS_REG(op2))
            error_invalid_reg(mnemonic);
        emit_instruction(0xF01E | (PARSE_REG(op2) << 8));
        return;
    }

    if (!IS_REG(op1))
        error_invalid_reg(mnemonic);
    int regX = PARSE_REG(op1);

    if (IS_REG(op2))
        emit_instruction(0x8004 | (regX << 8) | (PARSE_REG(op2) << 4)); 
    else
        emit_instruction(0x7000 | (regX << 8) | (uint8_t)strtol(op2, NULL, 0));
}

void handle_alu(FILE *src, const char *mnemonic)
{
    char op1[32], op2[32];
    get_operand(src, op1, sizeof(op1));
    get_operand(src, op2, sizeof(op2));

    if (!IS_REG(op1) || (!IS_REG(op2) && op2[0] != '\0'))
        error_invalid_reg(mnemonic);
    
    int regX = PARSE_REG(op1);
    // If 2nd operand is missing, default to regX for shifts
    int regY = (op2[0] != '\0') ? PARSE_REG(op2) : regX;

    uint16_t suffix = 0;
    if      (strcmp(mnemonic, "or")   == 0)
        suffix = 0x1;
    else if (strcmp(mnemonic, "and")  == 0)
        suffix = 0x2;
    else if (strcmp(mnemonic, "xor")  == 0)
        suffix = 0x3;
    else if (strcmp(mnemonic, "sub")  == 0)
        suffix = 0x5;
    else if (strcmp(mnemonic, "shr")  == 0)
        suffix = 0x6;
    else if (strcmp(mnemonic, "subn") == 0)
        suffix = 0x7;
    else if (strcmp(mnemonic, "shl")  == 0)
        suffix = 0xE;

    emit_instruction(0x8000 | (regX << 8) | (regY << 4) | suffix);
}

void handle_rnd(FILE *src, const char *mnemonic)
{
    char op1[32], op2[32];
    get_operand(src, op1, sizeof(op1));
    get_operand(src, op2, sizeof(op2));
    
    if (!IS_REG(op1))
        error_invalid_reg(mnemonic);
    int regX = PARSE_REG(op1);
    
    uint8_t val = (uint8_t)strtol(op2, NULL, 0);
    emit_instruction(0xC000 | (regX << 8) | val);
}

void handle_drw(FILE *src, const char *mnemonic)
{
    char op1[32], op2[32], op3[32];
    get_operand(src, op1, sizeof(op1));
    get_operand(src, op2, sizeof(op2));
    get_operand(src, op3, sizeof(op3));

    if (!IS_REG(op1) || !IS_REG(op2))
        error_invalid_reg(mnemonic);
    
    int regX = PARSE_REG(op1);
    int regY = PARSE_REG(op2);

    uint8_t nibble = (uint8_t)strtol(op3, NULL, 0) & 0x0F;
    emit_instruction(0xD000 | (regX << 8) | (regY << 4) | nibble);
}
