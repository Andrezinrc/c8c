#include "parser.h"
#include "lexer.h"
#include "assembler.h"
#include "reserved_words.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

void validate_identifier(const char *name, const char *context)
{
    if (is_reserved_keyword(name)) {
        fprintf(stderr, "[ERROR] line %d: '%s' is a reserved keyword and cannot be used as a %s name.\n", state.current_line, name, context);
        exit(1);
    }
}

int is_hex_value(const char *token)
{
    if (token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))
        return strspn(token + 2, "0123456789abcdefABCDEF") == strlen(token + 2);
    
    return 0;
}
void str_to_lower(char *str) 
{
    for (unsigned char *p = (unsigned char *)str; *p; p++)
        *p = tolower(*p);
}

int peek_next_token(FILE *src, char *buf, size_t size)
{
    // Save current position to look ahead without consuming the token
    long pos = ftell(src);
    int result = next_token(src, buf, size);
    fseek(src, pos, SEEK_SET);
    return result;
}

void emit_instruction(uint16_t op)
{
    state.rom[state.pc - ROM_START_ADDRESS] = (op >> 8) & 0xFF;
    state.rom[state.pc - ROM_START_ADDRESS + 1] = op & 0xFF;
    state.pc += 2;
}

void emit_byte(uint8_t byte)
{
    state.rom[state.pc - ROM_START_ADDRESS] = byte;
    state.pc += 1;
}

int16_t find_label(const char *name)
{
    for (int i = 0; i < state.label_count; i++)
        if (strcmp(state.labels[i].name, name) == 0)
            return state.labels[i].address;
    
    return -1;
}

int next_token_resolved(FILE *f, char *token_buffer, int max_size)
{
    if (!next_token(f, token_buffer, max_size)) return 0;

    str_to_lower(token_buffer);

    for (int i = 0; i < state.alias_count; i++)
        if (strcmp(token_buffer, state.aliases[i].name) == 0) {
            strncpy(token_buffer, state.aliases[i].reg_id, max_size - 1);
            token_buffer[max_size - 1] = '\0';
            break;
        }

    return 1;
}

static void get_operand(FILE *src, char *buf, size_t size)
{
    next_token_resolved(src, buf, size);

    char peek[16];
    if (peek_next_token(src, peek, sizeof(peek)) && strcmp(peek, ",") == 0)
        next_token(src, peek, sizeof(peek));
}

static uint16_t resolve_address(const char *target) 
{
    if (isdigit((unsigned char)target[0]) || (target[0] == '0' && target[1] == 'x'))
        return (uint16_t)strtol(target, NULL, 0) & 0x0FFF;
    
    int16_t label_addr = find_label(target);
    if (label_addr == -1 && state.pass == 2) {
        fprintf(stderr, "[ERROR] line %d: undefined label '%s'\n", state.current_line, target);
        exit(1);
    }
    return (label_addr == -1) ? 0x000 : (uint16_t)label_addr;
}

static void parse_label_directive(char *token) 
{
    if (state.pass == 1) {
        validate_identifier(token, "label");
        strcpy(state.labels[state.label_count].name, token);
        state.labels[state.label_count].address = state.pc;
        state.label_count++;
    }
}

static void parse_alias_directive(FILE *src) 
{
    char name[32];
    char reg[8];
    next_token(src, name, sizeof(name));
    next_token(src, reg, sizeof(reg));
    
    str_to_lower(name);
    str_to_lower(reg);

    if (state.pass == 1) {
        validate_identifier(name, "alias");

        if (!IS_REG(reg)) {
            fprintf(stderr, "[ERROR] line %d: alias destination '%s' must be a valid register\n", state.current_line, reg);
            exit(1);
        }

        strcpy(state.aliases[state.alias_count].name, name);
        strcpy(state.aliases[state.alias_count].reg_id, reg);
        state.alias_count++;
    }
}

static void parse_sprite_directive(FILE *src) 
{
    char name[32];
    next_token(src, name, sizeof(name));
    str_to_lower(name);

    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == ':')
        name[len - 1] = '\0';

    if (state.pass == 1) {
        validate_identifier(name, "sprite");
        strcpy(state.labels[state.label_count].name, name);
        state.labels[state.label_count].address = state.pc;
        state.label_count++;
    }

    char next_t[16];
    while (peek_next_token(src, next_t, sizeof(next_t))) {
        if (is_hex_value(next_t)) {
            next_token(src, next_t, sizeof(next_t));
            emit_byte((uint8_t)strtol(next_t, NULL, 16));
        } else {
            break;
        }
    }
    
    if (state.pc % 2 != 0)
        emit_byte(0x00); // Pad with a null byte to maintain 16-bit alignment
}

static void error_invalid_reg(const char *mnemonic)
{
    fprintf(stderr, "[ERROR] line %d: Invalid register operand for '%s'.\n", state.current_line, mnemonic);
    exit(1);
}


// Instruction Handles

static void handle_cls(FILE *src, const char *mnemonic)
{
    (void)src; (void)mnemonic;
    emit_instruction(0x00E0);
}

static void handle_ret(FILE *src, const char *mnemonic)
{
    (void)src; (void)mnemonic;
    emit_instruction(0x00EE);
}

static void handle_sys(FILE *src, const char *mnemonic)
{
    (void)mnemonic;
    char op[32];
    get_operand(src, op, sizeof(op));
    emit_instruction(0x0000 | resolve_address(op));
}

static void handle_jp(FILE *src, const char *mnemonic)
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

static void handle_call(FILE *src, const char *mnemonic)
{
    (void)mnemonic;
    char op[32];
    get_operand(src, op, sizeof(op));
    emit_instruction(0x2000 | resolve_address(op));
}

static void handle_se_sne(FILE *src, const char *mnemonic)
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

static void handle_skp_sknp(FILE *src, const char *mnemonic)
{
    char op[32];
    get_operand(src, op, sizeof(op));
    if (!IS_REG(op))
        error_invalid_reg(mnemonic);
    
    int regX = PARSE_REG(op);
    uint16_t suffix = (strcmp(mnemonic, "skp") == 0) ? 0x009E : 0x00A1;
    emit_instruction(0xE000 | (regX << 8) | suffix);
}

static void handle_ld(FILE *src, const char *mnemonic)
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

static void handle_add(FILE *src, const char *mnemonic)
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

static void handle_alu(FILE *src, const char *mnemonic)
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

static void handle_rnd(FILE *src, const char *mnemonic)
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

static void handle_drw(FILE *src, const char *mnemonic)
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


// Dispatch Table

static const InstructionDef INSTRUCTIONS[] = {
    {"cls", handle_cls},
    {"ret", handle_ret},
    {"sys", handle_sys},
    {"jp", handle_jp},
    {"call", handle_call},
    {"se", handle_se_sne},
    {"sne", handle_se_sne},
    {"skp", handle_skp_sknp},
    {"sknp", handle_skp_sknp},
    {"ld", handle_ld},
    {"add", handle_add},
    {"or", handle_alu},
    {"and", handle_alu},
    {"xor", handle_alu},
    {"sub", handle_alu},
    {"shr", handle_alu},
    {"subn", handle_alu},
    {"shl", handle_alu},
    {"rnd", handle_rnd},
    {"drw", handle_drw}
};

static void parse_instruction(FILE *src, const char *mnemonic)
{
    for (size_t i = 0; i < ARRAY_SIZE(INSTRUCTIONS); i++)
        if (strcmp(mnemonic, INSTRUCTIONS[i].mnemonic) == 0) {
            INSTRUCTIONS[i].handler(src, mnemonic);
            return;
        }

    fprintf(stderr, "[ERROR] line %d: Unknown mnemonic '%s'.\n", state.current_line, mnemonic);
    exit(1);
}

void parse_source(FILE *src) 
{
    char token[64];

    while (next_token_resolved(src, token, sizeof(token))) {
        size_t len = strlen(token);

#if PDEBUG
        printf("[DEBUG] Pass: %d | Token read: '\033[1;37m%s\033[0m'\n", state.pass, token);
#endif

        if (len > 0 && token[len - 1] == ':') {
            token[len - 1] = '\0';
            parse_label_directive(token);
            continue;
        }
        if (strcmp(token, "alias") == 0) {
            parse_alias_directive(src);
            continue;
        }
        if (strcmp(token, "spr") == 0) {
            parse_sprite_directive(src);
            continue;
        }

        parse_instruction(src, token);
    }
}
