#include "parser.h"
#include "lexer.h"
#include "assembler.h"
#include "emitter.h"
#include "reserved_words.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

void get_operand(FILE *src, char *buf, size_t size)
{
    next_token_resolved(src, buf, size);

    char peek[16];
    if (peek_next_token(src, peek, sizeof(peek)) && strcmp(peek, ",") == 0)
        next_token(src, peek, sizeof(peek));
}

uint16_t resolve_address(const char *target) 
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

void error_invalid_reg(const char *mnemonic)
{
    fprintf(stderr, "[ERROR] line %d: Invalid register operand for '%s'.\n", state.current_line, mnemonic);
    exit(1);
}


// Dispatch Table

typedef void (*InstructionHandler)(FILE *src, const char *mnemonic);

typedef struct {
    const char *mnemonic;
    InstructionHandler handler;
} InstructionDef;

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
