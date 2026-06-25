# Chip8 Compiler

A two-pass CHIP-8 compiler/assembler written in pure C, designed to assemble CHIP-8 assembly source code into executable binary ROMs.

## Features

* **Two-Pass Assembly:** Properly resolves forward and backward label references.
* **Register Aliasing:** Support for custom register names using the `alias` directive.
* **Sprite Inlining:** Easy bytecode embedding for graphics using the `spr` directive.

## Building

Build the project using the provided `Makefile`:

```bash
    make
```

## Execution

Run the compiler by passing the source file and the output ROM path:

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
