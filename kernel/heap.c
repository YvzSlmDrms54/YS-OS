/* kernel/heap.c - kmalloc and kfree.
 *
 * The arena is one big array, carved into blocks. Every block carries a
 * small header just before the memory you get back, so kfree only needs
 * the pointer you were given: it steps backwards to find the header.
 *
 * Blocks form a single linked list in address order, which makes merging
 * neighbours after a free straightforward.
 */

#include "heap.h"
#include "string.h"

#define HEAP_SIZE  (256 * 1024)
#define ALIGNMENT  8

struct block {
    size_t        size;   /* usable bytes in this block, not counting the header */
    int           free;
    struct block *next;   /* the next block in memory, or NULL at the end */
};

static char          arena[HEAP_SIZE];
static struct block *first;

/* Rounds up to the next multiple of ALIGNMENT. Misaligned pointers are
 * slower on x86 and outright illegal on some other processors, so it is
 * worth doing properly from the start. */
static size_t align_up(size_t value)
{
    return (value + (ALIGNMENT - 1)) & ~(size_t)(ALIGNMENT - 1);
}

void heap_init(void)
{
    first = (struct block *)arena;
    first->size = HEAP_SIZE - sizeof(struct block);
    first->free = 1;
    first->next = 0;
}

/* Returns a pointer to the memory that follows a header. */
static void *payload(struct block *b)
{
    return (void *)((char *)b + sizeof(struct block));
}

/* If a block is much bigger than needed, cut it in two and leave the
 * remainder free. Without this the first large block would be handed out
 * whole and the rest of the arena wasted. */
static void split(struct block *b, size_t wanted)
{
    struct block *rest;

    /* Only worth splitting if the leftover can hold a header plus
     * something useful. */
    if (b->size < wanted + sizeof(struct block) + ALIGNMENT) return;

    rest = (struct block *)((char *)payload(b) + wanted);
    rest->size = b->size - wanted - sizeof(struct block);
    rest->free = 1;
    rest->next = b->next;

    b->size = wanted;
    b->next = rest;
}

void *kmalloc(size_t size)
{
    struct block *b;

    if (size == 0) return 0;

    size = align_up(size);

    for (b = first; b != 0; b = b->next) {
        if (b->free && b->size >= size) {
            split(b, size);
            b->free = 0;
            return payload(b);
        }
    }

    return 0;   /* out of memory */
}

void *kcalloc(size_t count, size_t size)
{
    size_t total = count * size;
    void *p;

    /* Catch the multiplication wrapping around, which would otherwise
     * hand back a block far smaller than the caller believes it has. */
    if (count != 0 && total / count != size) return 0;

    p = kmalloc(total);
    if (p != 0) memset(p, 0, total);
    return p;
}

/* Merges every free block with the free block after it. Running this on
 * each free keeps the list from filling up with unusable fragments. */
static void coalesce(void)
{
    struct block *b;

    for (b = first; b != 0 && b->next != 0; ) {
        if (b->free && b->next->free) {
            b->size += sizeof(struct block) + b->next->size;
            b->next = b->next->next;
            /* Do not advance: the merged block may join the next one too. */
        } else {
            b = b->next;
        }
    }
}

void kfree(void *pointer)
{
    struct block *b;

    if (pointer == 0) return;

    /* Step back over the header to find the block this memory belongs to. */
    b = (struct block *)((char *)pointer - sizeof(struct block));
    b->free = 1;

    coalesce();
}

void heap_stats(size_t *used_out, size_t *free_out, size_t *blocks_out)
{
    size_t used = 0, available = 0, blocks = 0;
    struct block *b;

    for (b = first; b != 0; b = b->next) {
        if (b->free) available += b->size;
        else         used += b->size;
        blocks++;
    }

    if (used_out   != 0) *used_out   = used;
    if (free_out   != 0) *free_out   = available;
    if (blocks_out != 0) *blocks_out = blocks;
}