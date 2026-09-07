/* kernel/gdt.c - Global Descriptor Table.
 *
 * We use a "flat" model: every segment starts at address 0 and covers the
 * whole 4 GiB address space. That means segmentation effectively does
 * nothing, which is what we want - x86 forces us to have a GDT, but modern
 * systems do memory protection with paging instead.
 *
 * We still need separate code and data entries, and separate kernel and
 * user entries, because the CPU checks the type and privilege bits.
 */

#include "gdt.h"

#define GDT_ENTRIES 5

static struct gdt_entry   gdt[GDT_ENTRIES];
static struct gdt_pointer gdt_ptr;

/* Implemented in boot/gdt_flush.s */
extern void gdt_flush(uint32_t gdt_ptr_address);

/* Fills one entry, hiding the ugly bit-splitting.
 *
 * access byte:
 *   bit 7  present        - 1 means this entry is valid
 *   bit 6-5 privilege     - 0 = kernel (ring 0), 3 = user (ring 3)
 *   bit 4  type           - 1 for code/data, 0 for system entries
 *   bit 3  executable     - 1 = code segment, 0 = data segment
 *   bit 2  direction      - 0 for our purposes
 *   bit 1  read/write     - code: readable, data: writable
 *   bit 0  accessed       - the CPU sets this itself
 *
 * granularity byte:
 *   bit 7  granularity    - 1 means the limit counts 4 KiB pages, not bytes
 *   bit 6  size           - 1 = 32-bit protected mode
 *   bit 5-4 unused here
 *   bit 3-0 limit bits 16-19
 */
static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t flags)
{
    gdt[index].base_low    = (uint16_t)(base & 0xFFFF);
    gdt[index].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[index].base_high   = (uint8_t)((base >> 24) & 0xFF);

    gdt[index].limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt[index].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt[index].granularity |= (uint8_t)(flags & 0xF0);

    gdt[index].access      = access;
}

void gdt_init(void)
{
    gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_ptr.base  = (uint32_t)&gdt;

    /* Entry 0 must be all zeros. The CPU refuses to use it, which catches
     * bugs where a segment register was never initialised. */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* limit 0xFFFFF with 4 KiB granularity = 0xFFFFF * 4096 = 4 GiB */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xC0);  /* kernel code, ring 0 */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);  /* kernel data, ring 0 */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xC0);  /* user code,   ring 3 */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);  /* user data,   ring 3 */

    gdt_flush((uint32_t)&gdt_ptr);
}