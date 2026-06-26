#include "reserved_words.h"
#include "parser.h"
#include <string.h>

const char *const RESERVED_WORDS[] = {
    "cls", "ret", "sys", "jp", "call", "se", "sne", "ld",
    "add", "or", "and", "xor", "sub", "shr", "subn", "shl",
    "rnd", "drw", "skp", "sknp",
    "spr", "alias", "i", "dt", "st", "f", "b", "[i]", "k",
    "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", 
    "v8", "v9", "va", "vb", "vc", "vd", "ve", "vf"
};

int is_reserved_keyword(const char *name)
{
    for (size_t i = 0; i < ARRAY_SIZE(RESERVED_WORDS); i++)
        if (strcmp(name, RESERVED_WORDS[i]) == 0)
            return 1;
        
    return 0;
}
