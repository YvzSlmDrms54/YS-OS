/* kernel/kernel.c - Yunix entry point. */

#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "timer.h"
#include "shell.h"
#include "user.h"
#include "fish.h"
#include "ata.h"

void kernel_main(void)
{
    vga_init();

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("Yunix v0.1.2\n");
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
        if (ata_init()) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_write("[ ok ] ATA disk found on the primary channel\n");

        if (fish_load() == FISH_OK) {
            vga_write("[ ok ] Fish filesystem restored from disk\n");
        } else {
            vga_set_color(VGA_YELLOW, VGA_BLACK);
            vga_write("[ .. ] No saved filesystem. Type format to make one.\n");
        }
    } else {
        vga_set_color(VGA_YELLOW, VGA_BLACK);
        vga_write("[ .. ] No disk found. Files will vanish on reboot.\n");
    }
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    shell_run();   /* never returns */
}