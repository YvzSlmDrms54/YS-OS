/* boot/isr.s - the 48 entry points the CPU jumps to.
 *
 * The CPU does not tell the handler which interrupt fired, so each stub
 * pushes its own number and jumps to shared code.
 *
 * Some exceptions push an error code automatically and some do not. To
 * keep one single stack layout, the ones that do not push a dummy zero.
 */

.section .text

/* Exception without an error code: push a fake one. */
.macro ISR_NOERR num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    push $0
    push $\num
    jmp isr_common_stub
.endm

/* Exception where the CPU already pushed an error code. */
.macro ISR_ERR num
.global isr\num
.type isr\num, @function
isr\num:
    cli
    push $\num
    jmp isr_common_stub
.endm

/* Hardware interrupt. 'vec' is the IDT slot, 'num' is the IRQ number. */
.macro IRQ num, vec
.global irq\num
.type irq\num, @function
irq\num:
    cli
    push $0
    push $\vec
    jmp irq_common_stub
.endm

/* CPU exceptions 0-31. Numbers 8, 10-14 and 17 carry an error code. */
ISR_NOERR 0     /* divide by zero            */
ISR_NOERR 1     /* debug                     */
ISR_NOERR 2     /* non-maskable interrupt    */
ISR_NOERR 3     /* breakpoint                */
ISR_NOERR 4     /* overflow                  */
ISR_NOERR 5     /* bound range exceeded      */
ISR_NOERR 6     /* invalid opcode            */
ISR_NOERR 7     /* device not available      */
ISR_ERR   8     /* double fault              */
ISR_NOERR 9
ISR_ERR   10    /* invalid TSS               */
ISR_ERR   11    /* segment not present       */
ISR_ERR   12    /* stack-segment fault       */
ISR_ERR   13    /* general protection fault  */
ISR_ERR   14    /* page fault                */
ISR_NOERR 15
ISR_NOERR 16    /* x87 floating point        */
ISR_ERR   17    /* alignment check           */
ISR_NOERR 18    /* machine check             */
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

/* Hardware IRQs, remapped to IDT slots 32-47. */
IRQ 0,  32      /* timer     */
IRQ 1,  33      /* keyboard  */
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

/* --- shared tail --- */

.extern isr_handler
.extern irq_handler

isr_common_stub:
    pusha                   /* eax ecx edx ebx esp ebp esi edi */
    mov %ds, %ax
    push %eax               /* remember the old data segment */

    mov $0x10, %ax          /* switch to kernel data segment */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp               /* pass a pointer to the saved registers */
    call isr_handler
    add $4, %esp

    pop %eax                /* restore the old data segment */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    popa
    add $8, %esp            /* drop int_no and err_code */
    sti
    iret

irq_common_stub:
    pusha
    mov %ds, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp
    call irq_handler
    add $4, %esp

    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    popa
    add $8, %esp
    sti
    iret

.section .note.GNU-stack,"",@progbits