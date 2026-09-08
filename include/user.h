#ifndef USER_H
#define USER_H

#include <stddef.h>

#define USER_NAME_MAX 32

void user_init(void);

const char *user_name(void);
const char *user_host(void);

/* Both refuse empty names and silently cut anything too long.
 * Return 1 on success, 0 if the name was rejected. */
int user_set_name(const char *name);
int user_set_host(const char *name);

/* Keeps the names in a file at the root of the filesystem. Neither one
 * touches the disk itself - fish_save() still has to be called for the
 * change to survive a reboot. Return 1 on success, 0 on failure. */
int user_save(void);
int user_load(void);

#endif