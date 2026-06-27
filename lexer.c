#include "lexer.h"
#include "assembler.h"
#include <ctype.h>
#include <string.h>

int next_token(FILE *f, char *token_buffer, int max_size) {
    int ch;
    int len = 0;

    while ((ch = fgetc(f)) != EOF) {
        if (ch == '\n')
            state.current_line++;
        
        if (isspace(ch)) {
            if (len > 0) break;
            continue;
        }
        if (ch == ';') {
            while ((ch = fgetc(f)) != EOF && ch != '\n') {}
            if (ch == '\n')
                state.current_line++;
            if (len > 0) break;
            continue;
        }
        if (ch == ',') {
            if (len > 0) {
                ungetc(ch, f);
                break;
            } else {
                token_buffer[len++] = ch;
                break;
            }
        }
        if (len < max_size - 1) {
            token_buffer[len++] = ch;
        } else {
            ungetc(ch, f);
            break;
        }
    }

    token_buffer[len] = '\0';

    if (len > 0 && state.pass == 2)
        state.token_count++;

    return len > 0;
}
