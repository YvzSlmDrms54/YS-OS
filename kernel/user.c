/* kernel/user.c - who is logged in, and what this machine is called.
 *
 * There is no disk yet, so these live in memory and go back to their
 * defaults on every boot. Once Seaweed has a filesystem this is the
 * first thing that should be saved to it.
 */

#include "user.h"
#include "string.h"

static char name[USER_NAME_MAX];
static char host[USER_NAME_MAX];

/* Copies at most USER_NAME_MAX-1 characters and always terminates.
 * Rejects an empty name, and anything with a space or control character
 * in it - those would make the prompt unreadable. */
static int set_field(char *field, const char *value)
{
    size_t i;

    if (value == 0 || value[0] == '\0') return 0;

    for (i = 0; value[i] != '\0'; i++) {
        if (value[i] <= ' ') return 0;
    }

    for (i = 0; i < USER_NAME_MAX - 1 && value[i] != '\0'; i++) {
        field[i] = value[i];
    }
    field[i] = '\0';
    return 1;
}

void user_init(void)
{
    strcpy(name, "user");
    strcpy(host, "yunix");
}

const char *user_name(void) { return name; }
const char *user_host(void) { return host; }

int user_set_name(const char *value) { return set_field(name, value); }
int user_set_host(const char *value) { return set_field(host, value); }