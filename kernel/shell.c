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

static void cmd_help(const char *arg)
{
    (void)arg;
    vga_write("Available commands:\n");
    vga_write("  help                  show this list\n");
    vga_write("  clear                 clear the screen\n");
    vga_write("  neofetch              show the logo\n");
    vga_write("  echo <text>           print text back\n");
    vga_write("  about                 information about this system\n");
    vga_write("  ls                    lists the directories and files in the directory\n");
    vga_write("  cd <directory>        goes in/out of a directory\n");
    vga_write("  pwd                   i forgot what this did\n");
    vga_write("  mkdir <name>          creates a directory\n");
    vga_write("  touch <name>          creates a file\n");
    vga_write("  cat <file>            displays whats writing in the file\n");
    vga_write("  write <file> <text>   writes text to a file\n");
    vga_write("  append <file> <text>  adds text to a file\n");
    vga_write("  rm <file/directory>   deletes a file/directory\n");
    vga_write("  color <0-15>          change the text colour\n");
    vga_write("  whoami                print the current user name\n");
    vga_write("  user <name>           change the user name\n");
    vga_write("  hostname [name]       show or change the computer name\n");
    vga_write("  uptime                how long the system has been running\n");
    vga_write("  ticks                 raw timer tick count\n");
    vga_write("  sleep <secs>          wait for a while\n");
    vga_write("  reboot                restart the machine\n");
    vga_write("  halt                  stop the CPU\n");
}

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
    }
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
    }
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

struct command {
    const char *name;
    void      (*run)(const char *arg);
};

static const struct command commands[] = {
    { "help",       cmd_help     },
    { "neofetch",   cmd_neofetch },
    { "clear",      cmd_clear    },
    { "echo",       cmd_echo     },
    { "about",      cmd_about    },
    { "ls",         cmd_ls       },
    { "cd",         cmd_cd       },
    { "pwd",        cmd_pwd      },
    { "mkdir",      cmd_mkdir    },
    { "touch",      cmd_touch    },
    { "cat",        cmd_cat      },
    { "write",      cmd_write    },
    { "append",     cmd_append   },
    { "rm",         cmd_rm       },
    { "save",       cmd_save     },
    { "load",       cmd_load     },
    { "format",     cmd_format   },
    { "disk",       cmd_disk     },
    { "color",      cmd_color    },
    { "whoami",     cmd_whoami   },
    { "user",       cmd_user     },
    { "hostname",   cmd_hostname },
    { "uptime",     cmd_uptime   },
    { "ticks",      cmd_ticks    },
    { "sleep",      cmd_sleep    },
    { "reboot",     cmd_reboot   },
    { "halt",       cmd_halt     },
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

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