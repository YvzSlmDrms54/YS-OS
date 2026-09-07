/* drivers/vga.c - VGA text mode driver.
 *
 * The screen is memory. Address 0xB8000 is the top-left character cell.
 * Each cell is 2 bytes: the character, then the colour.
 */

#include "vga.h"

#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

static size_t  row;
static size_t  col;
static uint8_t color;

static uint16_t entry(char c, uint8_t attr)
{
    return (uint16_t)(unsigned char)c | (uint16_t)(attr << 8);
}

void vga_set_color(enum vga_color fg, enum vga_color bg)
{
    color = (uint8_t)fg | (uint8_t)(bg << 4);
}

void vga_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_MEMORY[y * VGA_WIDTH + x] = entry(' ', color);
    row = 0;
    col = 0;
}

void vga_init(void)
{
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

/* Moves every line up one and blanks the bottom line. */
static void scroll(void)
{
    for (size_t y = 1; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];

    for (size_t x = 0; x < VGA_WIDTH; x++)
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = entry(' ', color);

    row = VGA_HEIGHT - 1;
}

void vga_putchar(char c)
{
    if (c == '\n') {
        col = 0;
        if (++row == VGA_HEIGHT) scroll();
        return;
    }

    VGA_MEMORY[row * VGA_WIDTH + col] = entry(c, color);

    if (++col == VGA_WIDTH) {
        col = 0;
        if (++row == VGA_HEIGHT) scroll();
    }
}

void vga_write(const char *s)
{
    for (size_t i = 0; s[i] != '\0'; i++) vga_putchar(s[i]);
}
