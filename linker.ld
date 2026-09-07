/* linker.ld - tells the linker how to lay out the kernel in memory.
 *
 * Normally the linker uses a default script suited to programs running on
 * an OS. We have no OS, so we write our own.
 */

ENTRY(_start)

SECTIONS
{
    /* Load the kernel at 1 MiB. Everything below that is reserved for BIOS
     * things, memory-mapped devices and real-mode leftovers. */
    . = 1M;

    /* The multiboot header MUST come first so GRUB can find it. */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data: string literals, const arrays. */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Initialised global variables. */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* Uninitialised globals and our stack. Takes no space in the binary. */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
