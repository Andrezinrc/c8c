#ifndef EMITTER_H
#define EMITTER_H

#include <stdint.h>
#include <stdio.h>

void emit_instruction(uint16_t op);
void emit_byte(uint8_t byte);

void handle_cls(FILE *src, const char *mnemonic);
void handle_ret(FILE *src, const char *mnemonic);
void handle_sys(FILE *src, const char *mnemonic);
void handle_jp(FILE *src, const char *mnemonic);
void handle_call(FILE *src, const char *mnemonic);
void handle_se_sne(FILE *src, const char *mnemonic);
void handle_skp_sknp(FILE *src, const char *mnemonic);
void handle_ld(FILE *src, const char *mnemonic);
void handle_add(FILE *src, const char *mnemonic);
void handle_alu(FILE *src, const char *mnemonic);
void handle_rnd(FILE *src, const char *mnemonic);
void handle_drw(FILE *src, const char *mnemonic);

#endif
