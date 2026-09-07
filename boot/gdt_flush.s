/* boot/gdt_flush.s - loads the new GDT and refreshes the segment registers.
 *
 * lgdt only tells the CPU where the table is. The segment registers still
 * hold their old values until we reload them, so we do that here.
 */

.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
    /* The argument (address of our gdt_pointer) sits on the stack.
     * esp points at the return address, so the argument is at esp+4. */
    mov 4(%esp), %eax
    lgdt (%eax)

    /* Reload the data segment registers with selector 0x10 (kernel data). */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* cs cannot be loaded with mov. The only way to change it is a jump
     * that specifies a segment. A "far jump" does exactly that: it sets
     * cs to 0x08 (kernel code) and continues at the given label. */
    ljmp $0x08, $.flush_done

.flush_done:
    ret

.size gdt_flush, . - gdt_flush

.section .note.GNU-stack,"",@progbits
