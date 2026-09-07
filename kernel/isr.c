/* kernel/isr.c - what actually runs when an interrupt fires.
 *
 * Both C functions here are called from boot/isr.s.
 */

#include "idt.h"
#include "io.h"
#include "vga.h"

static const char *exception_names[32] = {
    "Divide by zero",          "Debug",
    "Non-maskable interrupt",  "Breakpoint",
    "Overflow",                "Bound range exceeded",
    "Invalid opcode",          "Device not available",
    "Double fault",            "Coprocessor segment overrun",
    "Invalid TSS",             "Segment not present",
    "Stack-segment fault",     "General protection fault",
    "Page fault",              "Reserved",
    "x87 floating point",      "Alignment check",
    "Machine check",           "SIMD floating point",
    "Virtualization",          "Control protection",
    "Reserved",                "Reserved",
    "Reserved",               "Reserved",
    "Reserved",               "Reserved",
    "Hypervisor injection",   "VMM communication",
    "Security",               "Reserved"
};

/* One handler slot per IRQ. NULL means nobody registered one. */
static irq_handler_t irq_handlers[16] = { 0 };

void irq_install_handler(int num, irq_handler_t handler)
{
    if (num >= 0 && num < 16) irq_handlers[num] = handler;
}

/* Called for CPU exceptions (interrupts 0-31). */
void isr_handler(struct registers *regs)
{
    vga_set_color(VGA_WHITE, VGA_RED);
    vga_write("\n*** KERNEL PANIC ***\n");

    if (regs->int_no < 32) {
        vga_write(exception_names[regs->int_no]);
    } else {
        vga_write("Unknown exception");
    }
    vga_write("\nSystem halted.\n");

    /* Nothing sensible left to do. Stop for good. */
    for (;;) __asm__ volatile ("cli; hlt");
}

/* Called for hardware interrupts (32-47). */
void irq_handler(struct registers *regs)
{
    int irq = (int)regs->int_no - 32;

    if (irq >= 0 && irq < 16 && irq_handlers[irq] != 0) {
        irq_handlers[irq](regs);
    }

    /* Tell the controllers we are done, or they will never send again.
     * IRQs 8-15 come through the slave, so it needs telling too. */
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}