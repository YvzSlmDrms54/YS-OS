/* boot.s - the very first code that runs.
 *
 * GRUB loads us into 32-bit protected mode, but nothing else is set up:
 * no stack, no C runtime, no libraries. This file builds a stack and
 * then calls kernel_main().
 */

/* --- Multiboot header ---
 * GRUB scans the first 8 KiB of the file for this magic number. If it is
 * not there, GRUB refuses to boot us.
 */
.set ALIGN,    1 << 0            /* align loaded modules on page boundaries */
.set MEMINFO,  1 << 1            /* ask GRUB for a memory map */
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002        /* the number GRUB looks for */
.set CHECKSUM, -(MAGIC + FLAGS)  /* magic + flags + checksum must equal 0 */

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* --- Stack ---
 * The C compiler assumes a working stack exists. It does not yet, so we
 * reserve 16 KiB here. .bss means "zero-filled, takes no space in the file".
 */
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

/* --- Entry point --- */
.section .text
.global _start
.type _start, @function

_start:
    /* x86 stacks grow downward, so the stack pointer starts at the TOP. */
    mov $stack_top, %esp

    /* Everything is ready. Hand control to C. */
    call kernel_main

    /* kernel_main should never return. If it does, stop the CPU forever:
     * cli  - disable interrupts so nothing can wake us
     * hlt  - halt until an interrupt arrives (which can never happen now)
     * jmp  - if something still wakes us, halt again
     */
    cli
1:  hlt
    jmp 1b

.size _start, . - _start

/* Tell the linker we do not need an executable stack. */
.section .note.GNU-stack,"",@progbits
