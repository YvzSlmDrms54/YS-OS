/* boot/idt_flush.s - hands the IDT address to the CPU. */

.section .text
.global idt_flush
.type idt_flush, @function

idt_flush:
    mov 4(%esp), %eax
    lidt (%eax)
    ret

.size idt_flush, . - idt_flush

.section .note.GNU-stack,"",@progbits