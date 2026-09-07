/* drivers/timer.c - the Programmable Interval Timer, on IRQ 0.
 *
 * The PIT is a chip that counts down from a number you give it. When it
 * reaches zero it raises IRQ 0 and starts again. By choosing the starting
 * number you choose how often that happens.
 */

#include "timer.h"
#include "idt.h"
#include "io.h"

/* The chip runs from a fixed 1.193182 MHz clock. That odd number comes
 * from the original IBM PC, where one crystal was divided down to serve
 * both the timer and the video circuitry. */
#define PIT_FREQUENCY 1193182

#define PIT_CHANNEL0  0x40
#define PIT_COMMAND   0x43

static volatile uint32_t ticks;

uint32_t timer_ticks(void)
{
    return ticks;
}

uint32_t timer_seconds(void)
{
    return ticks / TIMER_HZ;
}

static void timer_callback(struct registers *regs)
{
    (void)regs;
    ticks++;
}

void timer_sleep(uint32_t ms)
{
    /* Rounding up means sleep(1) waits at least one tick rather than none. */
    uint32_t wanted = (ms * TIMER_HZ + 999) / 1000;
    uint32_t target = ticks + wanted;

    /* Comparing the difference instead of the values keeps this correct
     * even when the counter wraps around back to zero. */
    while ((int32_t)(target - ticks) > 0) {
        __asm__ volatile ("hlt");
    }
}

void timer_init(void)
{
    uint32_t divisor = PIT_FREQUENCY / TIMER_HZ;

    ticks = 0;

    /* 0x36 = channel 0, send low byte then high byte, mode 3 (square
     * wave), binary counting. */
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_install_handler(0, timer_callback);
}