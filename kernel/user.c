/* kernel/user.c - who is logged in, and what this machine is called.
 *
 * The names are kept in a file at the root of the filesystem, so they
 * survive a reboot once the filesystem itself has been saved.
 */

#include "user.h"
#include "string.h"
#include "fish.h"

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


/* The file lives at the root and holds two lines: the user name, then
 * the host name. A plain text format on purpose - you can read it with
 * cat when something goes wrong. */
#define CONFIG_FILE "ys.conf"

int user_save(void)
{
    char text[USER_NAME_MAX * 2 + 4];
    int  saved = fish_save_cwd();
    int  ok = 0;
    size_t pos = 0;

    fish_chdir("/");
    fish_create(CONFIG_FILE);   /* fine if it already exists */

    for (size_t i = 0; name[i] != '\0'; i++) text[pos++] = name[i];
    text[pos++] = '\n';
    for (size_t i = 0; host[i] != '\0'; i++) text[pos++] = host[i];
    text[pos++] = '\n';
    text[pos] = '\0';

    if (fish_write(CONFIG_FILE, text) == FISH_OK) ok = 1;

    fish_restore_cwd(saved);
    return ok;
}

int user_load(void)
{
    const char *data;
    size_t size = 0;
    int    saved = fish_save_cwd();
    char   field[USER_NAME_MAX];
    size_t pos = 0;
    size_t i;
    int    line = 0;

    fish_chdir("/");
    data = fish_read(CONFIG_FILE, &size);
    fish_restore_cwd(saved);

    if (data == 0) return 0;

    /* Walk the text, cutting it at each newline. Anything malformed is
     * ignored, leaving that name at its default. */
    for (i = 0; i <= size; i++) {
        int end = (i == size) || (data[i] == '\n');

        if (!end) {
            if (pos < USER_NAME_MAX - 1) field[pos++] = data[i];
            continue;
        }

        field[pos] = '\0';
        if (pos > 0) {
            if (line == 0)      set_field(name, field);
            else if (line == 1) set_field(host, field);
        }

        pos = 0;
        line++;
        if (line > 1) break;
    }

    return 1;
}