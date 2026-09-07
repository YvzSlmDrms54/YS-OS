#ifndef STRING_H
#define STRING_H

#include <stddef.h>

/* Our own libc replacements.
 *
 * These keep their standard names on purpose: even with -ffreestanding,
 * gcc is allowed to turn a struct assignment or an array initialisation
 * into a call to memset or memcpy. If we named them differently the
 * kernel would fail to link with a confusing error. */

void  *memset(void *dest, int value, size_t count);
void  *memcpy(void *dest, const void *src, size_t count);
void  *memmove(void *dest, const void *src, size_t count);
int    memcmp(const void *a, const void *b, size_t count);

size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t count);
char  *strcpy(char *dest, const char *src);

/* Turns a number into text. 'buffer' must be big enough: 12 bytes is
 * always sufficient for a 32-bit value in base 10. Returns 'buffer'. */
char  *itoa(int value, char *buffer, int base);
char  *utoa(unsigned int value, char *buffer, int base);

#endif