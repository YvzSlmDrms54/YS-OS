/* drivers/vga.c - VGA text mode driver.
 *
 * The screen is memory. Address 0xB8000 is the top-left character cell.
 * Each cell is 2 bytes: the character, then the colour.
 */

#include "vga.h"
#include "io.h"

#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/* The CRT controller. You pick a register through 0x3D4, then read or
 * write its value through 0x3D5. */
#define CRTC_INDEX  0x3D4
#define CRTC_DATA   0x3D5

static size_t  row;
static size_t  col;
static uint8_t color;

static uint16_t entry(char c, uint8_t attr)
{
    return (uint16_t)(unsigned char)c | (uint16_t)(attr << 8);
}

/* Moves the blinking hardware cursor to the current position. The VGA
 * card wants one flat number, not an x and a y. */
static void update_cursor(void)
{
    uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);

    outb(CRTC_INDEX, 0x0F);                       /* cursor location low  */
    outb(CRTC_DATA,  (uint8_t)(pos & 0xFF));
    outb(CRTC_INDEX, 0x0E);                       /* cursor location high */
    outb(CRTC_DATA,  (uint8_t)((pos >> 8) & 0xFF));
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
    update_cursor();
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

/* Steps back one cell and erases what was there. */
static void backspace(void)
{
    if (col > 0) {
        col--;
    } else if (row > 0) {
        row--;
        col = VGA_WIDTH - 1;
    } else {
        return;                     /* already at the very start */
    }

    VGA_MEMORY[row * VGA_WIDTH + col] = entry(' ', color);
}

/* Advances to the next multiple of 8 columns. */
static void tab(void)
{
    size_t target = (col + 8) & ~(size_t)7;
    while (col < target && col < VGA_WIDTH) {
        VGA_MEMORY[row * VGA_WIDTH + col] = entry(' ', color);
        col++;
    }
    if (col >= VGA_WIDTH) {
        col = 0;
        if (++row == VGA_HEIGHT) scroll();
    }
}

void vga_putchar(char c)
{
    switch (c) {
    case '\n':
        col = 0;
        if (++row == VGA_HEIGHT) scroll();
        break;

    case '\r':
        col = 0;
        break;

    case '\b':
        backspace();
        break;

    case '\t':
        tab();
        break;

    default:
        VGA_MEMORY[row * VGA_WIDTH + col] = entry(c, color);
        if (++col == VGA_WIDTH) {
            col = 0;
            if (++row == VGA_HEIGHT) scroll();
        }
        break;
    }

    update_cursor();
}

void vga_write(const char *s)
{
    for (size_t i = 0; s[i] != '\0'; i++) vga_putchar(s[i]);
}

void vga_cursor_left(void)
{
    if (col > 0)        col--;
    else if (row > 0) { row--; col = VGA_WIDTH - 1; }
    update_cursor();
}

void vga_cursor_right(void)
{
    if (col < VGA_WIDTH - 1)       col++;
    else if (row < VGA_HEIGHT - 1) { row++; col = 0; }
    update_cursor();
}

void vga_cursor_up(void)
{
    if (row > 0) row--;
    update_cursor();
}

void vga_cursor_down(void)
{
    if (row < VGA_HEIGHT - 1) row++;
    update_cursor();
}