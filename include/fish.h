#ifndef FISH_H
#define FISH_H

#include <stddef.h>

/* Fish - the Seaweed filesystem.
 *
 * The tree lives in RAM and is written to disk in one piece by
 * fish_save(), then read back by fish_load(). Nothing is saved
 * automatically: changes are lost unless you save them.
 *
 * There is no kmalloc yet, so the node pool is a fixed size decided at
 * compile time. Nodes refer to their parent by index rather than by
 * pointer, which is what lets the whole array go to disk unchanged.
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
#define FISH_ERR_NODISK -9
#define FISH_ERR_IO     -10
#define FISH_ERR_FORMAT -11

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

/* Remembers the current directory so you can move away and come back.
 * The handle is only meaningful to Fish - do not interpret it. */
int  fish_save_cwd(void);
void fish_restore_cwd(int handle);

/* Writes the current directory as a path like "/notes/2026". */
void fish_path(char *buffer, size_t size);

/* Writes the whole filesystem to disk, or reads it back. Both need a
 * disk on the primary IDE channel; without one they fail cleanly. */
int  fish_save(void);
int  fish_load(void);

/* Turns an error code into something a human can read. */
const char *fish_error(int code);

#endif