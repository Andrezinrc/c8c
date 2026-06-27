# c8asm

A CHIP-8 assembler in pure C that turns text files into executable ROMs.

## Building

Build the project using the provided `Makefile`:

```bash
    make
```

## Execution

Run passing the source file and the output ROM path:

```bash
    ./c8c source.c8 output.ch8
```

## Syntax Example

```assembly
; Register alias definition
alias square_x v0
alias square_y v1

ld square_x, 0x0A
ld square_y, 0x14

main:
    cls

    call draw_square

    jp main

draw_square:
    ld i, my_sprite
    drw square_x, square_y, 4
    ret

; Sprite data directive
spr my_sprite:
    0xF0 0x90 0x90 0xF0

```

## License

MIT
