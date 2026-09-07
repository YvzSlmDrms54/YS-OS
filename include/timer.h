#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* How many times per second the timer fires. */
#define TIMER_HZ 100

void timer_init(void);

/* Number of ticks since boot. At 100 Hz this wraps after ~497 days. */
uint32_t timer_ticks(void);

/* Whole seconds since boot. */
uint32_t timer_seconds(void);

/* Waits for the given number of milliseconds. Sleeps between ticks
 * instead of spinning, so the CPU stays cool. */
void timer_sleep(uint32_t ms);

#endif