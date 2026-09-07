/* kernel/kernel.c - YS-OS entry point. */

#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "timer.h"
#include "shell.h"
#include "user.h"
#include "fish.h"

void kernel_main(void)
{
    vga_init();

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("YS-OS\n");
    vga_write("Kernel: Seaweed v0.1\n");

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_write("A hobby operating system by Yavuz Selim\n\n");

    gdt_init();
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_write("[ ok ] GDT loaded\n");

    idt_init();
    vga_write("[ ok ] IDT loaded, PIC remapped to 32-47\n");

    timer_init();
    vga_write("[ ok ] PIT running at 100 Hz on IRQ 0\n");

    keyboard_init();
    vga_write("[ ok ] Keyboard driver installed on IRQ 1\n");

    __asm__ volatile ("sti");
    vga_write("[ ok ] Interrupts enabled\n");

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_write("\nType help for a list of commands.\n\n");

    user_init();
    fish_init();

    shell_run();   /* never returns */
}