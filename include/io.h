#ifndef IO_H
#define IO_H

#include <stdint.h>

/* x86 has a second address space beside memory: 65536 I/O ports.
 * Devices live there. You reach them only with the in/out instructions,
 * so these are written in inline assembly. */

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* The 16-bit versions. ATA moves data a word at a time, not a byte. */

static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* Some old devices need a moment to react. Writing to port 0x80 is a
 * traditional way to waste exactly the right amount of time. */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

#endif