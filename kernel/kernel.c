/* kernel/kernel.c - YS-OS entry point.
 *
 * Called from boot/boot.s once a stack exists.
 */

#include "vga.h"

void kernel_main(void)
{
    vga_init();

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("YS-OS\n");

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_write("A hobby operating system by Yavuz Selim\n\n");

    vga_write("Booted successfully.\n");
    vga_write("No BIOS, no libc, no operating system underneath.\n\n");

    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_write("Next up: GDT, interrupts, keyboard driver.\n");

    for (;;) __asm__ volatile ("hlt");
}
