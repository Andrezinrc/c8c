#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdint.h>

#define PDEBUG 0

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define IS_REG(tok) ((tok)[0] == 'v' && isxdigit((unsigned char)(tok)[1]) && (tok)[2] == '\0')

#define PARSE_REG(tok) ((int)strtol(&(tok)[1], NULL, 16))

void get_operand(FILE *src, char *buf, size_t size);
uint16_t resolve_address(const char *target);
void error_invalid_reg(const char *mnemonic);
void parse_source(FILE *src);

#endif
