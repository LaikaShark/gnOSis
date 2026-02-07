

```
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
░░      ░░░   ░░░  ░░░      ░░░░      ░░░        ░░░      ░░
▒  ▒▒▒▒▒▒▒▒    ▒▒  ▒▒  ▒▒▒▒  ▒▒  ▒▒▒▒▒▒▒▒▒▒▒  ▒▒▒▒▒  ▒▒▒▒▒▒▒
▓  ▓▓▓   ▓▓  ▓  ▓  ▓▓  ▓▓▓▓  ▓▓▓      ▓▓▓▓▓▓  ▓▓▓▓▓▓      ▓▓
█  ████  ██  ██    ██  ████  ████████  █████  ███████████  █
██      ███  ███   ███      ████      ███        ███      ██
████████████████████████████████████████████████████████████
```
A bare-metal x86 (i386) hobby operating system written in C++ and assembly. It boots via Multiboot (GRUB-compatible), initializes hardware, and drops into a Forth interpreter.

## Building and Running

**Requirements:** `nasm`, `g++` (with 32-bit support), `ld`, `qemu-system-i386`

```bash
make            # Build kernel -> build/gnOSis.bin
make run        # Build (if needed) and launch in QEMU
make clean      # Remove object files (keeps build/gnOSis.bin)
```

On first boot, the disk will be unformatted. Type `FORMAT-DISK .` at the Forth prompt to initialize the filesystem.

## Architecture

### Boot Sequence

1. **`kernel/boot/boot.s`** -- Multiboot header, pushes multiboot struct pointer, calls `kmain`
2. **`kernel/main.cc:kmain()`** -- Clears screen, prints banner, initializes GDT+IDT, enables interrupts, initializes keyboard/timer/ATA/filesystem, enters `forth()`
3. **`forth()`** -- Runs the init script (predefined Forth words), then enters the interactive `QUIT` loop

### Source Layout

```
kernel/
  boot/boot.s            Multiboot header, stack setup, entry point
  asm/
    gdt.s                GDT flush (lgdt + far jump)
    idt.s                IDT flush (lidt)
    interrupt.s          ISR/IRQ entry trampolines (0-47)
  core/
    gdt.cc               Global Descriptor Table setup
    idt.cc               Interrupt Descriptor Table setup
    io.cc                Port I/O: inb, inw, outb, outw
    isr.cc               ISR/IRQ dispatch, handler registration
  module/
    monitor.cc           VGA text-mode driver (80x25, 16 colors)
    keyboard.cc          PS/2 keyboard driver (IRQ1), US layout
    timer.cc             PIT timer (IRQ0)
    memory.cc            memset, memcpy
    string.cc            strlen, strcmp, etc.
    common.cc            Descriptor table init, boot banner
    ata.cc               ATA PIO polling disk driver
    fs.cc                gnFS filesystem
    editor.cc            Nano-like text editor
    forth.cc             Forth interpreter/compiler
  include/
    system.h             Umbrella header (typedefs + all includes)
    *.h                  Headers for each module
linker.ld                Links kernel at 0x100000, entry point `start`
Makefile                 Build system
```

### Key Conventions

- **No standard library.** Compiled with `-nostdlib -nostdinc -fno-builtin -ffreestanding`. All libc-like functions are reimplemented.
- **`system.h` is the universal include.** Every `.cc` file includes it; it pulls in all other headers and defines integer types (`u8int`, `u16int`, `u32int`, etc.).
- **Interrupt handlers** are registered with `register_interrupt_handler(irq, callback)`. timer is IRQ0 (ISR 32), Keyboard is IRQ1 (ISR 33).

### Hardware Drivers

| Driver | File | Description |
|--------|------|-------------|
| VGA | `monitor.cc` | 80x25 text mode, 16 foreground/background colors, scrolling |
| Keyboard | `keyboard.cc` | PS/2 scancode-to-ASCII via IRQ1, 256-byte circular buffer, US layout with shift/caps |
| Timer | `timer.cc` | Programmable Interval Timer on IRQ0, tick counter |
| ATA | `ata.cc` | PIO polling mode, 28-bit LBA, single-sector read/write on primary master |

### Memory Map

The kernel is linked at `0x100000` (1MB). VGA framebuffer is at `0xB8000`. The Forth interpreter uses a 64KB byte array for all its data structures.

---

## gnFS Filesystem

A simple flat filesystem stored on the ATA disk. The disk image is 2MB (4096 sectors of 512 bytes).

### Disk Layout

| Sectors | Content |
|---------|---------|
| 0 | Superblock (magic `0x674E4653` "gNFS", version, geometry) |
| 1--16 | Directory (256 entries x 32 bytes each) |
| 17 | Allocation bitmap (4096 bits = 4096 sectors) |
| 18--4095 | Data sectors |

### Limits

- 256 files maximum
- 18-character filenames
- 4 simultaneously open files
- Contiguous allocation (files cannot grow beyond their initial allocation)

### Forth File I/O Words

All file I/O words follow ANS Forth conventions. An `ior` of `0` means success; `-1` means failure.

| Word | Stack effect | Description |
|------|-------------|-------------|
| `CREATE-FILE` | ( c-addr u fam -- fileid ior ) | Create and open a new file |
| `OPEN-FILE` | ( c-addr u fam -- fileid ior ) | Open an existing file |
| `CLOSE-FILE` | ( fileid -- ior ) | Close a file |
| `READ-FILE` | ( c-addr u1 fileid -- u2 ior ) | Read up to u1 bytes into buffer |
| `WRITE-FILE` | ( c-addr u fileid -- ior ) | Write u bytes from buffer |
| `DELETE-FILE` | ( c-addr u -- ior ) | Delete a file by name |
| `FILE-SIZE` | ( fileid -- ud ior ) | Get file size as double-cell |
| `FILE-LIST` | ( -- ) | Print all files and sizes |
| `FORMAT-DISK` | ( -- ior ) | Format the disk (destroys all data) |
| `FLUSH` | ( -- ) | Flush directory and bitmap to disk |

**File access modes:** `R/O` (1), `W/O` (2), `R/W` (3)

**Example session:**

```forth
FORMAT-DISK .
s" hello.txt" R/W CREATE-FILE .
s" Hello, gnOSis!" ROT WRITE-FILE .
0 CLOSE-FILE .
s" hello.txt" R/O OPEN-FILE .
HERE @ 64 ROT READ-FILE . .
FILE-LIST
```

### Script Execution

Files can be executed as Forth scripts using `INCLUDE`:

| Word | Stack effect | Description |
|------|-------------|-------------|
| `INCLUDED` | ( c-addr u -- ) | Execute a file as Forth source |
| `INCLUDE` | ( "name" -- ) | Parse filename and execute it |

Scripts are read into a 4KB buffer and fed to the interpreter character by character, just like the built-in init script in `forth.cc`. The file must contain valid Forth source, each word is interpreted or compiled as if it were typed at the prompt.

```forth
INCLUDE mylib.4th
```

**Warning:** Scripts cannot nest, if a script calls `INCLUDE`, the outer script is replaced.

---

## Text Editor

A nano-like text editor for creating and editing files on the gnFS filesystem. Launch it with the `ED` word or the lower-level `EDIT` builtin.

| Word | Stack effect | Description |
|------|-------------|-------------|
| `EDIT` | ( c-addr u -- ) | Open file by name from stack |
| `ED` | ( "name" -- ) | Parse filename and open editor |

```forth
ED hello.txt
```

### Key Bindings

| Key | Action |
|-----|--------|
| PageUp / PageDown | Scroll 23 lines |
| Ctrl+S | Save file |
| Ctrl+X | Exit (double-press if there are unsaved changes) |

### Details

- **Buffer:** 8KB flat character array. Insert and delete shift bytes in place.
- **Tabs:** Stored as literal `\t` characters, expanded to 4-space-aligned tab stops for display.
- **Save:** Deletes the old file and creates a new one with the buffer contents.
- **New files:** If the file doesn't exist, the editor opens with an empty buffer. The file is created on first save.

---

## Forth Language Reference

gnOSis Forth is a 16-bit cell Forth interpreter/compiler. It is case-insensitive and loosely follows ANS Forth conventions.

### Data Types

| Type | Width | Description |
|------|-------|-------------|
| Cell | 16-bit | `short` -- basic stack value |
| Double-cell | 32-bit | `int` -- two cells, high on top |
| Character | 8-bit | Byte-sized value |

Number literals are parsed in the current `BASE` (default 10). A `.` in a number makes it a double-cell literal (e.g., `123.` pushes two cells).

### Stacks

| Stack | Depth | Description |
|-------|-------|-------------|
| Data stack | 192 cells | General computation |
| Return stack | 64 cells | Subroutine return addresses, loop counters |

### Memory

The Forth system uses a single 65,536-byte memory array with this layout:

```
Offset          Content
0               Input word buffer (32 bytes)
32              LATEST (cell) -- pointer to newest dictionary entry
34              HERE (cell) -- next free dictionary byte
36              BASE (cell) -- current number base
38              STATE (cell) -- 0=interpreting, 1=compiling
40              Data stack (192 cells = 384 bytes)
424             Return stack (64 cells = 128 bytes)
552+            Dictionary / HERE space (free memory)
```

### Defining Words

```forth
: name ... ;                  \ Define a new colon definition
VARIABLE name                 \ Create a variable (1 cell)
2VARIABLE name                \ Create a double-cell variable
CONSTANT name                 \ ( n -- ) Define a named constant
2CONSTANT name                \ ( d -- ) Define a double-cell constant
CREATE name                   \ Create a dictionary entry with no behavior
```

### Stack Manipulation

| Word | Stack effect | Description |
|------|-------------|-------------|
| `DUP` | ( a -- a a ) | Duplicate top |
| `DROP` | ( a -- ) | Discard top |
| `SWAP` | ( a b -- b a ) | Swap top two |
| `OVER` | ( a b -- a b a ) | Copy second to top |
| `ROT` | ( a b c -- b c a ) | Rotate third to top |
| `?DUP` | ( a -- a a ) or ( 0 -- 0 ) | Duplicate if non-zero |
| `NIP` | ( a b -- b ) | Drop second |
| `TUCK` | ( a b -- b a b ) | Copy top under second |
| `2DUP` | ( a b -- a b a b ) | Duplicate top pair |
| `2DROP` | ( a b -- ) | Drop top pair |
| `2SWAP` | ( a b c d -- c d a b ) | Swap top two pairs |
| `2OVER` | ( a b c d -- a b c d a b ) | Copy second pair to top |
| `2ROT` | ( a b c d e f -- c d e f a b ) | Rotate third pair to top |
| `2NIP` | ( a b c d -- c d ) | Drop second pair |

### Return Stack

| Word | Stack effect | Description |
|------|-------------|-------------|
| `>R` | ( a -- ) (R: -- a ) | Move to return stack |
| `R>` | (R: a -- ) ( -- a ) | Move from return stack |
| `R@` | (R: a -- a ) ( -- a ) | Copy top of return stack |
| `I` | ( -- n ) | Current loop index (alias for `R@`) |
| `J` | ( -- n ) | Outer loop index |

### Arithmetic

| Word | Stack effect | Description |
|------|-------------|-------------|
| `+` | ( a b -- a+b ) | Add |
| `-` | ( a b -- a-b ) | Subtract |
| `*` | ( a b -- a*b ) | Multiply |
| `/` | ( a b -- a/b ) | Divide |
| `MOD` | ( a b -- a%b ) | Modulus |
| `/MOD` | ( a b -- rem quot ) | Division with remainder |
| `*/` | ( a b c -- a*b/c ) | Scaled multiply (intermediate is 32-bit) |
| `*/MOD` | ( a b c -- rem a*b/c ) | Scaled multiply with remainder |
| `NEGATE` | ( a -- -a ) | Negate |
| `ABS` | ( a -- \|a\| ) | Absolute value |
| `1+` | ( a -- a+1 ) | Increment |
| `1-` | ( a -- a-1 ) | Decrement |
| `2+` | ( a -- a+2 ) | Add 2 |
| `2-` | ( a -- a-2 ) | Subtract 2 |
| `2*` | ( a -- a*2 ) | Double |
| `2/` | ( a -- a/2 ) | Halve |
| `CELLS` | ( n -- n*2 ) | Convert cell count to bytes |

### Double-Cell Arithmetic

| Word | Stack effect | Description |
|------|-------------|-------------|
| `D+` | ( d1 d2 -- d1+d2 ) | Add doubles |
| `D-` | ( d1 d2 -- d1-d2 ) | Subtract doubles |
| `D*` | ( d1 d2 -- d1*d2 ) | Multiply doubles |
| `D/` | ( d1 d2 -- d1/d2 ) | Divide doubles |
| `DNEGATE` | ( d -- -d ) | Negate double |
| `DABS` | ( d -- \|d\| ) | Absolute value of double |
| `D2/` | ( d -- d/2 ) | Halve double |

### Comparison

| Word | Stack effect | Description |
|------|-------------|-------------|
| `=` | ( a b -- flag ) | Equal |
| `<` | ( a b -- flag ) | Less than (signed) |
| `>` | ( a b -- flag ) | Greater than (signed) |
| `<>` | ( a b -- flag ) | Not equal |
| `<=` | ( a b -- flag ) | Less than or equal |
| `>=` | ( a b -- flag ) | Greater than or equal |
| `0=` | ( a -- flag ) | Equal to zero |
| `0<` | ( a -- flag ) | Less than zero |
| `0>` | ( a -- flag ) | Greater than zero |
| `0<=` | ( a -- flag ) | Less than or equal to zero |
| `0>=` | ( a -- flag ) | Greater than or equal to zero |

Flags are `-1` (true) or `0` (false).

### Double-Cell Comparison

| Word | Stack effect | Description |
|------|-------------|-------------|
| `D=` | ( d1 d2 -- flag ) | Equal |
| `D<` | ( d1 d2 -- flag ) | Less than (signed) |
| `D>` | ( d1 d2 -- flag ) | Greater than |
| `DU<` | ( ud1 ud2 -- flag ) | Less than (unsigned) |
| `D0=` | ( d -- flag ) | Equal to zero |
| `DMIN` | ( d1 d2 -- d ) | Minimum |
| `DMAX` | ( d1 d2 -- d ) | Maximum |

### Logic and Bitwise

| Word | Stack effect | Description |
|------|-------------|-------------|
| `AND` | ( a b -- a&b ) | Bitwise AND |
| `OR` | ( a b -- a\|b ) | Bitwise OR |
| `XOR` | ( a b -- a^b ) | Bitwise XOR |
| `NOT` | ( a -- ~a ) | Bitwise complement |
| `TRUE` | ( -- -1 ) | True flag |
| `FALSE` | ( -- 0 ) | False flag |

### Memory Access

| Word | Stack effect | Description |
|------|-------------|-------------|
| `@` | ( addr -- n ) | Fetch cell |
| `!` | ( n addr -- ) | Store cell |
| `C@` | ( addr -- c ) | Fetch byte |
| `C!` | ( c addr -- ) | Store byte |
| `+!` | ( n addr -- ) | Add n to cell at addr |
| `,` | ( n -- ) | Compile cell to HERE, advance |
| `C,` | ( c -- ) | Compile byte to HERE, advance |
| `ALLOT` | ( n -- ) | Advance HERE by n bytes |
| `ALIGN` | ( -- ) | Align HERE to cell boundary |

### Control Flow (compile-time only)

```forth
IF ... THEN
IF ... ELSE ... THEN
BEGIN ... UNTIL              \ loop until flag is true
BEGIN ... AGAIN              \ infinite loop
BEGIN ... WHILE ... REPEAT   \ loop while flag is true
DO ... LOOP                  \ counted loop (limit start DO ... LOOP)
DO ... +LOOP                 \ counted loop with custom increment
RECURSE                      \ call the word being defined
```

`I` returns the current loop index inside `DO...LOOP`. `J` returns the outer loop index in nested loops.

### Input and Output

| Word | Stack effect | Description |
|------|-------------|-------------|
| `EMIT` | ( c -- ) | Print character (white on black) |
| `CEMIT` | ( c fg bg -- ) | Print colored character |
| `CR` | ( -- ) | Print newline |
| `SPACE` | ( -- ) | Print space |
| `SPACES` | ( n -- ) | Print n spaces |
| `BL` | ( -- 32 ) | Space character code |
| `.` | ( n -- ) | Print signed number and space |
| `.R` | ( n width -- ) | Print right-justified in field |
| `.S` | ( -- ) | Print entire data stack |
| `?` | ( addr -- ) | Fetch and print cell |
| `TYPE` | ( c-addr u -- ) | Print u characters starting at c-addr |
| `KEY` | ( -- c ) | Wait for and return a keypress |
| `BUFFKEY` | ( -- c ) | Read next character from input buffer |
| `KEY?` | ( -- flag ) | Is a key waiting in the buffer? |

### Strings

```forth
s" text"                     \ ( -- c-addr u ) Parse string to HERE or compile inline
." text"                     \ Compile: print string at runtime
```

`s"` works in both interpret and compile modes. In interpret mode, the string is placed at `HERE` temporarily and its address and length are pushed. In compile mode, the string is compiled inline with `LITSTRING`.

### Compiler and Dictionary

| Word | Stack effect | Description |
|------|-------------|-------------|
| `WORD` | ( -- c-addr u ) | Read next whitespace-delimited word |
| `FIND` | ( c-addr u -- xt ) | Look up word in dictionary (0 if not found) |
| `>CFA` | ( xt -- cfa ) | Convert dictionary entry to code field |
| `NUMBER` | ( c-addr u -- n 0 ) or ( c-addr u -- d 0 ) | Parse number |
| `'` | ( "name" -- xt ) | Get execution token of next word |
| `[COMPILE]` | Compile-time | Force compilation of immediate word |
| `[CHAR]` | Compile-time | Compile character literal |
| `[` | ( -- ) | Switch to interpret mode (immediate) |
| `]` | ( -- ) | Switch to compile mode |
| `IMMEDIATE` | ( -- ) | Toggle immediate flag on latest word |
| `HIDE` | ( -- ) | Toggle hidden flag on latest word |
| `STATE` | ( -- addr ) | Address of state variable |
| `LATEST` | ( -- addr ) | Address of latest dictionary pointer |
| `HERE` | ( -- addr ) | Address of HERE pointer |
| `BASE` | ( -- addr ) | Address of BASE variable |
| `CELL` | ( -- 2 ) | Cell size in bytes |
| `FREE` | ( -- n ) | Bytes of unused dictionary space |

### Number Base

```forth
DECIMAL                      \ Set BASE to 10
HEX                         \ Set BASE to 16
OCTAL                        \ Set BASE to 8
```

Numbers are parsed using the current `BASE`. Bases up to 36 are supported (digits 0-9, A-Z).

### Comments

```forth
\ This is a line comment (everything to end of line)
( This is a parenthesized comment )
```

### System Words

| Word | Stack effect | Description |
|------|-------------|-------------|
| `CLEAR` | ( -- ) | Clear the screen |
| `TIME` | ( -- n ) | Current timer tick count |
| `PAUSE` | ( -- ) | Wait one timer tick |
| `SLEEP` | ( n -- ) | Wait n timer ticks |
| `BYE` | ( -- ) | Exit Forth (shows shutdown screen) |
| `QUIT` | ( -- ) | Reset stacks, enter interpreter loop |
| `MIN` | ( a b -- min ) | Minimum of two values |
| `MAX` | ( a b -- max ) | Maximum of two values |
| `COUNT` | ( c-addr -- c-addr+1 u ) | Get counted string |

### Dictionary Entry Format

Each word in the dictionary is a linked list node:

```
+--------+----------+----------------+---------+--------+
| LINK   | FLAGS+LEN| NAME bytes...  | padding | CODE   |
| (cell) | (byte)   |                | to cell |        |
+--------+----------+----------------+---------+--------+
```

- **LINK**: Cell pointing to the previous dictionary entry (0 for the first word)
- **FLAGS+LEN**: One byte -- bit 7 = IMMEDIATE, bit 6 = HIDDEN, bits 4-0 = name length (max 31)
- **NAME**: ASCII characters (case-insensitive lookup)
- **Padding**: Aligned to cell boundary
- **CODE**: For builtins, a cell containing the builtin ID followed by EXIT. For colon definitions, starts with `DOCOL` followed by a sequence of code field addresses, ending with `EXIT`.

### Inner Interpreter

Colon definitions are indirect threaded. When a colon word executes:

1. `DOCOL` pushes the current instruction pointer onto the return stack
2. Execution proceeds through the compiled cell addresses
3. Each cell is either a builtin ID (dispatched directly) or a code field address (entered recursively)
4. `EXIT` pops the return stack and resumes the caller

### Compilation

When `STATE` is 1 (compiling), the outer interpreter compiles words rather than executing them:

- Known words: their CFA is appended to the current definition with `,`
- Numbers: compiled as `LIT n`
- Double-cell numbers: compiled as `LIT lo LIT hi`
- Immediate words: executed even during compilation (used for control flow, `[`, `;`, etc.)

---

## Goals

- [x] Functioning shell
- [x] Forth interpreter
- [x] File system
- [x] Text editor
- [x] Script execution
- [ ] Make more goals

## License

Hobby project. Do whatever you want with it.
