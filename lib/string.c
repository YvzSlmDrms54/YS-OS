/* lib/string.c - the handful of libc functions the kernel needs. */

#include "string.h"

void *memset(void *dest, int value, size_t count)
{
    unsigned char *p = (unsigned char *)dest;
    while (count--) *p++ = (unsigned char)value;
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (count--) *d++ = *s++;
    return dest;
}

/* Like memcpy, but safe when the two areas overlap. Copying backwards
 * when dest sits after src stops us overwriting bytes we still need. */
void *memmove(void *dest, const void *src, size_t count)
{
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (d < s) {
        while (count--) *d++ = *s++;
    } else {
        d += count;
        s += count;
        while (count--) *--d = *--s;
    }
    return dest;
}

int memcmp(const void *a, const void *b, size_t count)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;

    while (count--) {
        if (*p != *q) return (int)*p - (int)*q;
        p++; q++;
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') n++;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t count)
{
    while (count > 0 && *a != '\0' && *a == *b) { a++; b++; count--; }
    if (count == 0) return 0;
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

char *strcpy(char *dest, const char *src)
{
    char *start = dest;
    while ((*dest++ = *src++) != '\0') { }
    return start;
}

/* Writes 'value' into 'buffer' in the given base (2 to 16).
 *
 * The digits come out backwards - the ones digit first - so we fill the
 * buffer in that order and reverse it at the end. */
char *utoa(unsigned int value, char *buffer, int base)
{
    static const char digits[] = "0123456789abcdef";
    size_t length = 0;
    size_t i;

    if (base < 2 || base > 16) { buffer[0] = '\0'; return buffer; }

    /* Zero has no digits by the loop below, so handle it separately. */
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer;
    }

    while (value > 0) {
        buffer[length++] = digits[value % (unsigned int)base];
        value /= (unsigned int)base;
    }
    buffer[length] = '\0';

    /* Reverse in place: swap the ends, walk inwards. */
    for (i = 0; i < length / 2; i++) {
        char tmp = buffer[i];
        buffer[i] = buffer[length - 1 - i];
        buffer[length - 1 - i] = tmp;
    }

    return buffer;
}

char *itoa(int value, char *buffer, int base)
{
    if (base == 10 && value < 0) {
        buffer[0] = '-';
        /* Cast before negating: -(-2147483648) does not fit in an int,
         * so negating it first would be undefined behaviour. */
        utoa((unsigned int)(-(long)value), buffer + 1, base);
        return buffer;
    }
    return utoa((unsigned int)value, buffer, base);
}