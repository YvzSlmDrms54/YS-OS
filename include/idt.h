#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* One IDT entry ("gate"): where to jump when interrupt N happens. */
struct idt_entry {
    uint16_t offset_low;   /* handler address, bits 0-15  */
    uint16_t selector;     /* which GDT code segment to use */
    uint8_t  zero;         /* always 0 */
    uint8_t  flags;        /* present bit, privilege, gate type */
    uint16_t offset_high;  /* handler address, bits 16-31 */
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* A snapshot of the CPU when the interrupt fired.
 *
 * The field order here MUST match the push order in boot/isr.s exactly.
 * Lowest address first, because the stack grows downwards. */
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by pusha */
    uint32_t int_no, err_code;                        /* pushed by our stub */
    uint32_t eip, cs, eflags, useresp, ss;            /* pushed by the CPU */
};

typedef void (*irq_handler_t)(struct registers *regs);

void idt_init(void);

/* Registers a function to run when IRQ 'num' (0-15) fires. */
void irq_install_handler(int num, irq_handler_t handler);

#endif