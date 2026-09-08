/* kernel/shell.c - the Yunix command shell. */

#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include <stdint.h>
#include "io.h"
#include "timer.h"
#include "user.h"
#include "fish.h"
#include "ata.h"

#define LINE_MAX 256

static char line[LINE_MAX];

/* --- commands --- */

/* Defined at the bottom: it walks the command table, which does not
 * exist yet at this point in the file. */
static void cmd_help(const char *arg);

static void cmd_clear(const char *arg)
{
    (void)arg;
    vga_clear();
}

static void cmd_echo(const char *arg)
{
    vga_write(arg);
    vga_putchar('\n');
}

static void cmd_neofetch(const char *arg)
{
    (void)arg;
    vga_write("\xDB\xDB\xBB   \xDB\xDB\xBB\xDB\xDB\xDB\xDB\xDB\xDB\xDB\xBB\n");
    vga_write("\xC8\xDB\xDB\xBB \xDB\xDB\xC9\xBC\xDB\xDB\xC9\xCD\xCD\xCD\xCD\xBC\n");
    vga_write(" \xC8\xDB\xDB\xDB\xDB\xC9\xBC \xDB\xDB\xDB\xDB\xDB\xDB\xDB\xBB\n");
    vga_write("  \xC8\xDB\xDB\xC9\xBC  \xC8\xCD\xCD\xCD\xCD\xDB\xDB\xBA\n");
    vga_write("   \xDB\xDB\xBA   \xDB\xDB\xDB\xDB\xDB\xDB\xDB\xBA\n");
    vga_write("   \xC8\xCD\xBC   \xC8\xCD\xCD\xCD\xCD\xCD\xCD\xBC\n");
}

static void cmd_about(const char *arg)
{
    (void)arg;
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_write("Yunix v0.1.2\n");
    vga_write("Kernel: Seaweed v0.1\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_write("A hobby operating system by Yavuz Selim.\n");
    vga_write("32-bit x86, booted by GRUB via multiboot.\n");
    vga_write("Written in C and assembly, no libc underneath.\n");
}

/* Prints a number followed by a word, adding "s" when it is not one. */
static void print_unit(uint32_t value, const char *word)
{
    char buffer[12];

    utoa(value, buffer, 10);
    vga_write(buffer);
    vga_putchar(' ');
    vga_write(word);
    if (value != 1) vga_putchar('s');
}

static void cmd_uptime(const char *arg)
{
    uint32_t total = timer_seconds();
    uint32_t hours = total / 3600;
    uint32_t minutes = (total / 60) % 60;
    uint32_t seconds = total % 60;

    (void)arg;

    vga_write("up ");
    if (hours > 0)   { print_unit(hours, "hour");     vga_write(", "); }
    if (minutes > 0) { print_unit(minutes, "minute"); vga_write(", "); }
    print_unit(seconds, "second");
    vga_putchar('\n');
}

static void cmd_ticks(const char *arg)
{
    char buffer[12];

    (void)arg;

    utoa(timer_ticks(), buffer, 10);
    vga_write(buffer);
    vga_write(" ticks at ");
    utoa(TIMER_HZ, buffer, 10);
    vga_write(buffer);
    vga_write(" Hz\n");
}

static void cmd_sleep(const char *arg)
{
    uint32_t seconds = 0;

    if (arg[0] == '\0') { vga_write("Usage: sleep <seconds>\n"); return; }

    for (size_t i = 0; arg[i] != '\0'; i++) {
        if (arg[i] < '0' || arg[i] > '9') {
            vga_write("sleep: not a number\n");
            return;
        }
        seconds = seconds * 10 + (uint32_t)(arg[i] - '0');
    }

    if (seconds > 60) { vga_write("sleep: 60 seconds maximum\n"); return; }

    timer_sleep(seconds * 1000);
}

static void cmd_whoami(const char *arg)
{
    (void)arg;
    vga_write(user_name());
    vga_putchar('\n');
}

static void cmd_user(const char *arg)
{
    if (arg[0] == '\0') {
        vga_write("Usage: user <name>\n");
        return;
    }
    if (!user_set_name(arg)) {
        vga_write("user: names cannot be empty or contain spaces\n");
        return;
    }
    user_save();
}

static void cmd_hostname(const char *arg)
{
    if (arg[0] == '\0') {
        vga_write(user_host());
        vga_putchar('\n');
        return;
    }
    if (!user_set_host(arg)) {
        vga_write("hostname: names cannot be empty or contain spaces\n");
        return;
    }
    user_save();
}

/* ---- Fish filesystem commands ---- */

static void fish_fail(const char *cmd, int code)
{
    vga_write(cmd);
    vga_write(": ");
    vga_write(fish_error(code));
    vga_putchar('\n');
}

static void cmd_pwd(const char *arg)
{
    char path[FISH_PATH_MAX];

    (void)arg;
    fish_path(path, sizeof(path));
    vga_write(path);
    vga_putchar('\n');
}

static void cmd_ls(const char *arg)
{
    int handle = fish_first();
    const char *name;
    int is_dir;
    size_t size;
    int count = 0;
    char buffer[12];

    (void)arg;

    while ((handle = fish_next(handle, &name, &is_dir, &size)) != 0) {
        if (is_dir) {
            vga_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
            vga_write("[DIR]  ");
        } else {
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            vga_write("[FILE] ");
        }

        vga_write(name);

        if (!is_dir) {
            vga_write("  (");
            utoa((unsigned int)size, buffer, 10);
            vga_write(buffer);
            vga_write(" bytes)");
        }

        vga_putchar('\n');
        count++;
    }

    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    if (count == 0) vga_write("Directory is empty.\n");
}

static void cmd_cd(const char *arg)
{
    int result;

    if (arg[0] == '\0') { fish_chdir("/"); return; }

    result = fish_chdir(arg);
    if (result < 0) fish_fail("cd", result);
}

static void cmd_mkdir(const char *arg)
{
    int result;

    if (arg[0] == '\0') { vga_write("Usage: mkdir <name>\n"); return; }

    result = fish_mkdir(arg);
    if (result < 0) fish_fail("mkdir", result);
}

static void cmd_touch(const char *arg)
{
    int result;

    if (arg[0] == '\0') { vga_write("Usage: touch <name>\n"); return; }

    result = fish_create(arg);
    if (result < 0) fish_fail("touch", result);
}

static void cmd_rm(const char *arg)
{
    int result;

    if (arg[0] == '\0') { vga_write("Usage: rm <name>\n"); return; }

    result = fish_remove(arg);
    if (result < 0) fish_fail("rm", result);
}

static void cmd_cat(const char *arg)
{
    size_t size;
    const char *data;

    if (arg[0] == '\0') { vga_write("Usage: cat <file>\n"); return; }

    data = fish_read(arg, &size);
    if (data == 0) { vga_write("cat: no such file\n"); return; }

    for (size_t i = 0; i < size; i++) vga_putchar(data[i]);
    if (size > 0 && data[size - 1] != '\n') vga_putchar('\n');
}

/* write <file> <text>  - replaces the contents
 * append <file> <text> - adds to the end
 *
 * Both need to split their argument again: the shell only splits off the
 * command name, so 'arg' still holds "file text here". */
static void write_common(const char *arg, int appending)
{
    char name[FISH_NAME_MAX];
    size_t i = 0;
    const char *text;
    int result;

    if (arg[0] == '\0') {
        vga_write(appending ? "Usage: append <file> <text>\n"
                            : "Usage: write <file> <text>\n");
        return;
    }

    while (arg[i] != '\0' && arg[i] != ' ' && i < FISH_NAME_MAX - 1) {
        name[i] = arg[i];
        i++;
    }
    name[i] = '\0';

    text = arg + i;
    while (*text == ' ') text++;

    result = appending ? fish_append(name, text) : fish_write(name, text);
    if (result < 0) fish_fail(appending ? "append" : "write", result);
}

static void cmd_write(const char *arg)  { write_common(arg, 0); }
static void cmd_append(const char *arg) { write_common(arg, 1); }

static void cmd_save(const char *arg)
{
    int result;

    (void)arg;

    user_save();

    result = fish_save();
    if (result < 0) { fish_fail("save", result); return; }
    vga_write("Filesystem written to disk.\n");
}

static void cmd_load(const char *arg)
{
    int result;

    (void)arg;

    result = fish_load();
    if (result < 0) { fish_fail("load", result); return; }
    vga_write("Filesystem restored from disk.\n");
}

static void cmd_format(const char *arg)
{
    int result;

    (void)arg;

    fish_init();
    result = fish_save();
    if (result < 0) { fish_fail("format", result); return; }
    vga_write("Filesystem erased and written to disk.\n");
}

static void cmd_disk(const char *arg)
{
    (void)arg;

    if (ata_present()) vga_write("Primary master: present\n");
    else               vga_write("Primary master: not found\n");
}

static void cmd_color(const char *arg)
{
    int value = 0;

    if (arg[0] == '\0') {
        vga_write("Usage: color <0-15>\n");
        return;
    }

    for (size_t i = 0; arg[i] != '\0'; i++) {
        if (arg[i] < '0' || arg[i] > '9') {
            vga_write("color: not a number\n");
            return;
        }
        value = value * 10 + (arg[i] - '0');
    }

    if (value > 15) {
        vga_write("color: pick a number from 0 to 15\n");
        return;
    }

    vga_set_color((enum vga_color)value, VGA_BLACK);
    vga_write("Color changed.\n");
}

/* Asks the old keyboard controller to pulse the CPU reset line. It is a
 * strange place for a reset button, but on a PC that is where it lives. */
static void cmd_reboot(const char *arg)
{
    (void)arg;
    vga_write("Rebooting...\n");
    while (inb(0x64) & 0x02) { }   /* wait for the input buffer to empty */
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile ("hlt");
}

static void cmd_halt(const char *arg)
{
    (void)arg;
    vga_write("System halted. You can close the system.\n");
    for (;;) __asm__ volatile ("cli; hlt");
}

/* --- the command table ---
 *
 * Name, how to call it, one line of help, and the function. Keeping the
 * help text here means a new command is one row, not a row plus a line
 * somewhere else that is easy to forget. */

struct command {
    const char *name;
    const char *args;   /* how to call it, or "" when it takes nothing */
    const char *help;   /* one line, shown by the help command */
    void      (*run)(const char *arg);
};

static const struct command commands[] = {
    { "help",     "[page]",         "show this list",                    cmd_help     },
    { "neofetch", "",               "show the logo",                     cmd_neofetch },
    { "clear",    "",               "clear the screen",                  cmd_clear    },
    { "echo",     "<text>",         "print text back",                   cmd_echo     },
    { "about",    "",               "information about this system",     cmd_about    },
    { "ls",       "",               "list files in the directory",       cmd_ls       },
    { "cd",       "<dir>",          "go in or out of a directory",       cmd_cd       },
    { "pwd",      "",               "print the current directory",       cmd_pwd      },
    { "mkdir",    "<name>",         "create a directory",                cmd_mkdir    },
    { "touch",    "<name>",         "create a file",                     cmd_touch    },
    { "cat",      "<file>",         "show what is inside a file",        cmd_cat      },
    { "write",    "<file> <text>",  "write text to a file",              cmd_write    },
    { "append",   "<file> <text>",  "add text to a file",                cmd_append   },
    { "rm",       "<name>",         "delete a file or empty directory",  cmd_rm       },
    { "save",     "",               "write the filesystem to disk",      cmd_save     },
    { "load",     "",               "read the filesystem from disk",     cmd_load     },
    { "format",   "",               "erase the filesystem and save it",  cmd_format   },
    { "disk",     "",               "is there a disk attached?",         cmd_disk     },
    { "color",    "<0-15>",         "change the text colour",            cmd_color    },
    { "whoami",   "",               "print the current user name",       cmd_whoami   },
    { "user",     "<name>",         "change the user name",              cmd_user     },
    { "hostname", "[name]",         "show or change the computer name",  cmd_hostname },
    { "uptime",   "",               "how long the system has run",       cmd_uptime   },
    { "ticks",    "",               "raw timer tick count",              cmd_ticks    },
    { "sleep",    "<secs>",         "wait for a while",                  cmd_sleep    },
    { "reboot",   "",               "restart the machine",               cmd_reboot   },
    { "halt",     "",               "stop the CPU",                      cmd_halt     },
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

/* Twenty rows plus a header and a footer fits inside 25 lines. */
#define HELP_PER_PAGE 20
#define HELP_COLUMN   23

static size_t help_pages(void)
{
    return (COMMAND_COUNT + HELP_PER_PAGE - 1) / HELP_PER_PAGE;
}

/* Writes "  name args" and pads it out so the descriptions line up. */
static void write_padded(const char *name, const char *args)
{
    size_t width = 0;

    vga_write("  ");
    vga_write(name);
    width += strlen(name);

    if (args[0] != '\0') {
        vga_putchar(' ');
        width++;
        vga_write(args);
        width += strlen(args);
    }

    while (width < HELP_COLUMN) { vga_putchar(' '); width++; }
}

static void cmd_help(const char *arg)
{
    size_t page = 1;
    size_t pages = help_pages();
    size_t start, i;
    char buffer[12];

    if (arg[0] != '\0') {
        page = 0;
        for (size_t j = 0; arg[j] != '\0'; j++) {
            if (arg[j] < '0' || arg[j] > '9') {
                vga_write("help: not a number\n");
                return;
            }
            page = page * 10 + (size_t)(arg[j] - '0');
        }
        if (page < 1 || page > pages) {
            vga_write("help: no such page\n");
            return;
        }
    }

    start = (page - 1) * HELP_PER_PAGE;

    vga_write("Available commands (page ");
    utoa((unsigned int)page, buffer, 10);
    vga_write(buffer);
    vga_write(" of ");
    utoa((unsigned int)pages, buffer, 10);
    vga_write(buffer);
    vga_write("):\n");

    for (i = start; i < start + HELP_PER_PAGE && i < COMMAND_COUNT; i++) {
        write_padded(commands[i].name, commands[i].args);
        vga_write(commands[i].help);
        vga_putchar('\n');
    }

    if (page < pages) vga_write("\nType help 2 for more.\n");
}

/* --- input --- */

/* Reads one line. Backspace can only delete what the user typed on this
 * line, never the prompt or anything printed before it. */
static void read_line(void)
{
    size_t length = 0;

    for (;;) {
        int key = keyboard_getchar();

        if (key == 0) {
            __asm__ volatile ("hlt");
            continue;
        }

        if (key == '\n') {
            vga_putchar('\n');
            line[length] = '\0';
            return;
        }

        if (key == '\b') {
            if (length > 0) {          /* this is the boundary */
                length--;
                vga_putchar('\b');
            }
            continue;
        }

        /* Ignore arrows and anything else without a character, and stop
         * accepting input once the buffer is full. */
        if (key > 255 || key < ' ') continue;
        if (length >= LINE_MAX - 1) continue;

        line[length++] = (char)key;
        vga_putchar((char)key);
    }
}

/* Splits the line into a command and the rest. Returns a pointer to the
 * argument, and cuts the command short with a '\0'. */
static char *split(char *text)
{
    size_t i = 0;

    while (text[i] != '\0' && text[i] != ' ') i++;

    if (text[i] == '\0') return text + i;   /* no argument: empty string */

    text[i] = '\0';
    i++;
    while (text[i] == ' ') i++;             /* skip extra spaces */
    return text + i;
}

static void execute(char *text)
{
    char *arg;

    /* Skip leading spaces, then ignore a line that holds nothing. */
    while (*text == ' ') text++;
    if (*text == '\0') return;

    arg = split(text);

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(text, commands[i].name) == 0) {
            commands[i].run(arg);
            return;
        }
    }

    vga_write(text);
    vga_write(": command not found. Type help.\n");
}

void shell_run(void)
{
    for (;;) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_write(user_name());
        vga_putchar('@');
        vga_write(user_host());
        vga_write(" >> ");

        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        read_line();

        execute(line);
    }
}