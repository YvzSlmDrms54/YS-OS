/* drivers/keyboard.c - PS/2 keyboard driver.
 *
 * The keyboard does not send letters. It sends "scancodes": a number for
 * the physical key, not the symbol printed on it. Key 0x1E is the one
 * left of 'S' - which is 'A' on a QWERTY board and something else on
 * other layouts. Turning scancodes into characters is our job.
 *
 * Pressing a key sends its scancode. Releasing it sends the same code
 * with bit 7 set, so 0x1E pressed becomes 0x9E released.
 */

#include "keyboard.h"
#include "idt.h"
#include "io.h"

#define KBD_DATA   0x60   /* read scancodes here */
#define KBD_STATUS 0x64

/* Scancode set 1, US layout. Index = scancode, value = character. */
static const char keymap[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,                                        /* left control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,                                        /* left shift */
    '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,                                        /* right shift */
    '*',
    0,                                        /* left alt */
    ' ',
    0                                         /* caps lock */
};

/* The same keys with shift held down. */
static const char keymap_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,
    '*',
    0,
    ' ',
    0
};

#define SC_LSHIFT   0x2A
#define SC_RSHIFT   0x36
#define SC_CAPSLOCK 0x3A
#define SC_RELEASED 0x80

static int shift_held;
static int caps_on;
static int extended;   /* set when the previous byte was 0xE0 */

/* Ring buffer: the interrupt writes at 'head', the main loop reads at
 * 'tail'. Size is a power of two so the wrap-around is a cheap AND. */
#define BUFFER_SIZE 128
static volatile int buffer[BUFFER_SIZE];
static volatile unsigned head;
static volatile unsigned tail;

static void buffer_push(int c)
{
    unsigned next = (head + 1) & (BUFFER_SIZE - 1);
    if (next == tail) return;      /* full - drop the key rather than
                                      overwrite what was not read yet */
    buffer[head] = c;
    head = next;
}

int keyboard_getchar(void)
{
    int c;
    if (head == tail) return 0;    /* nothing waiting */
    c = buffer[tail];
    tail = (tail + 1) & (BUFFER_SIZE - 1);
    return c;
}

/* Runs on every IRQ 1. Keep it short: interrupts are disabled meanwhile. */
static void keyboard_callback(struct registers *regs)
{
    uint8_t scancode = inb(KBD_DATA);
    char c;

    (void)regs;   /* we do not need the CPU state here */

    /* Arrow keys, Home, Delete and friends send 0xE0 first, then their
     * own code. Remember the prefix and deal with the next byte. */
    if (scancode == 0xE0) { extended = 1; return; }

    if (extended) {
        extended = 0;
        if (scancode & SC_RELEASED) return;   /* ignore releases */

        switch (scancode) {
        case 0x48: buffer_push(KEY_UP);     return;
        case 0x50: buffer_push(KEY_DOWN);   return;
        case 0x4B: buffer_push(KEY_LEFT);   return;
        case 0x4D: buffer_push(KEY_RIGHT);  return;
        case 0x47: buffer_push(KEY_HOME);   return;
        case 0x4F: buffer_push(KEY_END);    return;
        case 0x53: buffer_push(KEY_DELETE); return;
        case 0x49: buffer_push(KEY_PGUP);   return;
        case 0x51: buffer_push(KEY_PGDN);   return;
        default:   return;
        }
    }

    if (scancode & SC_RELEASED) {
        scancode &= 0x7F;                       /* clear the release bit */
        if (scancode == SC_LSHIFT || scancode == SC_RSHIFT) shift_held = 0;
        return;
    }

    if (scancode == SC_LSHIFT || scancode == SC_RSHIFT) { shift_held = 1; return; }
    if (scancode == SC_CAPSLOCK) { caps_on = !caps_on; return; }

    if (scancode >= 128) return;

    c = shift_held ? keymap_shift[scancode] : keymap[scancode];
    if (c == 0) return;                         /* a key we do not map */

    /* Caps lock affects letters only, and flips whatever shift decided. */
    if (caps_on) {
        if (c >= 'a' && c <= 'z')      c = (char)(c - 'a' + 'A');
        else if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    }

    buffer_push(c);
}

void keyboard_init(void)
{
    head = 0;
    tail = 0;
    shift_held = 0;
    caps_on = 0;
    extended = 0;

    /* Throw away anything the BIOS left in the buffer, or the first key
     * press will look like whatever was pressed before we booted. */
    while (inb(KBD_STATUS) & 1) inb(KBD_DATA);

    irq_install_handler(1, keyboard_callback);
}