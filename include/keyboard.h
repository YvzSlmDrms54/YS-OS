#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Keys that have no ASCII character get a code above 255, so they can
 * never be confused with a real one. */
#define KEY_UP     0x100
#define KEY_DOWN   0x101
#define KEY_LEFT   0x102
#define KEY_RIGHT  0x103
#define KEY_HOME   0x104
#define KEY_END    0x105
#define KEY_DELETE 0x106
#define KEY_PGUP   0x107
#define KEY_PGDN   0x108

void keyboard_init(void);

/* Returns the next key: an ASCII character, or one of the KEY_* codes
 * above, or 0 when nothing is waiting. Never blocks. */
int keyboard_getchar(void);

#endif