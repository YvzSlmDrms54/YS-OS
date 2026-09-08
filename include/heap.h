#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

/* A first-fit heap over one fixed arena.
 *
 * This is not a physical memory manager: there is no paging yet, so the
 * arena is simply a large static array. When Seaweed learns about paging
 * this file is what changes, and everything calling kmalloc stays as it
 * is.
 */

void  heap_init(void);

/* Returns a block of at least 'size' bytes, or NULL if there is no room.
 * Always check the result - there is nobody else to catch it. */
void *kmalloc(size_t size);

/* Same, but the block is filled with zeros. */
void *kcalloc(size_t count, size_t size);

/* Gives a block back. Passing NULL is allowed and does nothing.
 * Freeing the same block twice corrupts the heap. */
void  kfree(void *pointer);

/* For a 'mem' command: how much is in use and how much is left. */
void  heap_stats(size_t *used_out, size_t *free_out, size_t *blocks_out);

#endif