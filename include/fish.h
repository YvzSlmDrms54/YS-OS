#ifndef FISH_H
#define FISH_H

#include <stddef.h>

/* Fish - the Seaweed filesystem.
 *
 * Version 1 lives entirely in RAM and disappears on reboot. There is no
 * kmalloc yet, so everything comes from a fixed pool decided at compile
 * time. When a disk driver exists, this is the layer that gains a
 * save and load path.
 */

#define FISH_NAME_MAX   32
#define FISH_MAX_NODES  64
#define FISH_FILE_MAX   512
#define FISH_PATH_MAX   256

/* Return codes. Zero is success, negatives are errors, so callers can
 * write "if (fish_mkdir(name) < 0)". */
#define FISH_OK          0
#define FISH_ERR_EXISTS -1
#define FISH_ERR_NOTDIR -2
#define FISH_ERR_NOENT  -3
#define FISH_ERR_FULL   -4
#define FISH_ERR_NAME   -5
#define FISH_ERR_ISDIR  -6
#define FISH_ERR_NOTEMPTY -7
#define FISH_ERR_SPACE  -8

void fish_init(void);

/* Directory operations, all relative to the current directory. */
int  fish_mkdir(const char *name);
int  fish_chdir(const char *name);   /* ".." goes up, "/" goes to the root */
int  fish_remove(const char *name);

/* File operations. */
int  fish_create(const char *name);
int  fish_write(const char *name, const char *text);   /* replaces contents */
int  fish_append(const char *name, const char *text);
const char *fish_read(const char *name, size_t *size_out);

/* Listing. Call fish_first() then fish_next() until it returns 0.
 * 'is_dir_out' may be NULL if you do not care. */
int  fish_first(void);
int  fish_next(int handle, const char **name_out, int *is_dir_out,
               size_t *size_out);

/* Writes the current directory as a path like "/notes/2026". */
void fish_path(char *buffer, size_t size);

/* Turns an error code into something a human can read. */
const char *fish_error(int code);

#endif