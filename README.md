# c8asm

A CHIP-8 assembler in pure C that compiles source code into executable ROMs.

## Building

Build the project using the provided `Makefile`:

```bash
    make
```

## Execution

Run the assembler by passing the source file and the output ROM path:

```bash
    ./c8asm source.c8 output.ch8
```

## Vim Syntax Highlighting

To automatically enable syntax highlighting for your `.c8` source files in Vim, run:

```bash
make vim
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

# Language Documentation

`This manual expects basic knowledge regarding the internal mechanics of the CHIP-8.`

## Identifiers & Lexicon

The assembler implements a dictionary of 45 restricted words. Deploying these tokens for custom label, alias, or sprite names triggers a validation failure.

20 mnemonics:

```
cls
ret
sys
jp
call
se
sne
ld
add
or
and
xor
sub
shr
subn
shl
rnd
drw
skp
sknp

```

9 internal pointers & directives:

```
spr
alias
i
dt
st
f
b
[i]
k

```

16 registers:

```
v0 v1 v2 v3 v4 v5 v6 v7 v8 v9 va vb vc vd ve vf

```

## Usage of mnemonics

* cls (clear screen)
`cls`
* ret (return from subroutine)
`ret`
* sys (system call)
`sys <constant>`
* jp (jump to address)
`jp (<label>|<constant>)`
* call (call subroutine at address)
`call (<label>|<constant>)`
* se (skip next instruction if equal)
`se <register>, (<register>|<constant>)`
* sne (skip next instruction if not equal)
`sne <register>, (<register>|<constant>)`
* skp (skip next instruction if key down)
`skp <register>`
* sknp (skip next instruction if key up)
`sknp <register>`
* ld (load value)
`ld <register>, (<register>|<constant>)`
* add (addition)
`add <register>, (<register>|<constant>)`
* or (bitwise or)
`or <register>, <register>`
* and (bitwise and)
`and <register>, <register>`
* xor (bitwise xor)
`xor <register>, <register>`
* sub (subtraction)
`sub <register>, <register>`
* shr (bitwise right shift)
`shr <register>, <register>`
* subn (subtraction, reversed)
`subn <register>, <register>`
* shl (bitwise left shift)
`shl <register>, <register>`
* rnd (RNG)
`rnd <register>, <constant>`
* drw (draw sprite)
`drw <register>, <register>, <constant>`

## License

MIT
