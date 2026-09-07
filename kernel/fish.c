/* fs/fish.c - the Fish filesystem, version 1.
 *
 * A tree of nodes held in one fixed array. Each node knows its parent by
 * index rather than by pointer, which keeps the whole thing relocatable
 * and easy to write out to a disk later.
 *
 * Node 0 is always the root and is never freed.
 */

#include "fish.h"
#include "string.h"

struct node {
    char   name[FISH_NAME_MAX];
    int    used;
    int    is_dir;
    int    parent;                 /* index of the parent, -1 for the root */
    size_t size;                   /* bytes used in data, files only */
    char   data[FISH_FILE_MAX];
};

static struct node nodes[FISH_MAX_NODES];
static int current;                /* index of the current directory */

/* --- helpers --- */

/* Names may not be empty, too long, or contain a slash or a space. */
static int valid_name(const char *name)
{
    size_t i;

    if (name == 0 || name[0] == '\0') return 0;

    for (i = 0; name[i] != '\0'; i++) {
        if (i >= FISH_NAME_MAX - 1) return 0;
        if (name[i] == '/' || name[i] <= ' ') return 0;
    }

    /* "." and ".." are reserved for navigation. */
    if (strcmp(name, ".") == 0)  return 0;
    if (strcmp(name, "..") == 0) return 0;

    return 1;
}

/* Finds a child of 'parent' by name. Returns its index, or -1. */
static int find_child(int parent, const char *name)
{
    for (int i = 0; i < FISH_MAX_NODES; i++) {
        if (!nodes[i].used) continue;
        if (nodes[i].parent != parent) continue;
        if (strcmp(nodes[i].name, name) == 0) return i;
    }
    return -1;
}

/* Takes an unused slot from the pool. Returns its index, or -1 if the
 * filesystem is full. */
static int allocate(void)
{
    for (int i = 1; i < FISH_MAX_NODES; i++) {   /* 0 is the root */
        if (!nodes[i].used) return i;
    }
    return -1;
}

/* Creates a child of the current directory. */
static int make_node(const char *name, int is_dir)
{
    int index;

    if (!valid_name(name))                    return FISH_ERR_NAME;
    if (find_child(current, name) >= 0)       return FISH_ERR_EXISTS;

    index = allocate();
    if (index < 0)                            return FISH_ERR_FULL;

    strcpy(nodes[index].name, name);
    nodes[index].used   = 1;
    nodes[index].is_dir = is_dir;
    nodes[index].parent = current;
    nodes[index].size   = 0;

    return index;
}

/* --- public API --- */

void fish_init(void)
{
    memset(nodes, 0, sizeof(nodes));

    strcpy(nodes[0].name, "/");
    nodes[0].used   = 1;
    nodes[0].is_dir = 1;
    nodes[0].parent = -1;
    nodes[0].size   = 0;

    current = 0;
}

int fish_mkdir(const char *name)
{
    int result = make_node(name, 1);
    return result < 0 ? result : FISH_OK;
}

int fish_create(const char *name)
{
    int result = make_node(name, 0);
    return result < 0 ? result : FISH_OK;
}

int fish_chdir(const char *name)
{
    int index;

    if (name == 0 || name[0] == '\0') return FISH_ERR_NOENT;

    if (strcmp(name, "/") == 0)  { current = 0; return FISH_OK; }
    if (strcmp(name, ".") == 0)  { return FISH_OK; }

    if (strcmp(name, "..") == 0) {
        /* The root's parent is itself, as far as the user is concerned. */
        if (nodes[current].parent >= 0) current = nodes[current].parent;
        return FISH_OK;
    }

    index = find_child(current, name);
    if (index < 0)            return FISH_ERR_NOENT;
    if (!nodes[index].is_dir) return FISH_ERR_NOTDIR;

    current = index;
    return FISH_OK;
}

int fish_remove(const char *name)
{
    int index = find_child(current, name);

    if (index < 0) return FISH_ERR_NOENT;

    /* Refuse to delete a directory that still holds something. Deleting
     * it anyway would strand its children: they would still be marked
     * used, but nothing could reach them. */
    if (nodes[index].is_dir) {
        for (int i = 0; i < FISH_MAX_NODES; i++) {
            if (nodes[i].used && nodes[i].parent == index) {
                return FISH_ERR_NOTEMPTY;
            }
        }
    }

    nodes[index].used = 0;
    return FISH_OK;
}

int fish_write(const char *name, const char *text)
{
    int index = find_child(current, name);
    size_t length;

    if (index < 0)           return FISH_ERR_NOENT;
    if (nodes[index].is_dir) return FISH_ERR_ISDIR;

    length = strlen(text);
    if (length > FISH_FILE_MAX) return FISH_ERR_SPACE;

    memcpy(nodes[index].data, text, length);
    nodes[index].size = length;
    return FISH_OK;
}

int fish_append(const char *name, const char *text)
{
    int index = find_child(current, name);
    size_t length;

    if (index < 0)           return FISH_ERR_NOENT;
    if (nodes[index].is_dir) return FISH_ERR_ISDIR;

    length = strlen(text);
    if (nodes[index].size + length > FISH_FILE_MAX) return FISH_ERR_SPACE;

    memcpy(nodes[index].data + nodes[index].size, text, length);
    nodes[index].size += length;
    return FISH_OK;
}

const char *fish_read(const char *name, size_t *size_out)
{
    int index = find_child(current, name);

    if (index < 0 || nodes[index].is_dir) return 0;

    if (size_out != 0) *size_out = nodes[index].size;
    return nodes[index].data;
}

/* Listing works with a plain integer cursor: fish_first() gives you a
 * starting point, fish_next() hands back one entry and moves it along. */
int fish_first(void)
{
    return 0;
}

int fish_next(int handle, const char **name_out, int *is_dir_out,
              size_t *size_out)
{
    for (int i = handle; i < FISH_MAX_NODES; i++) {
        if (!nodes[i].used) continue;
        if (nodes[i].parent != current) continue;

        if (name_out   != 0) *name_out   = nodes[i].name;
        if (is_dir_out != 0) *is_dir_out = nodes[i].is_dir;
        if (size_out   != 0) *size_out   = nodes[i].size;

        return i + 1;      /* where to carry on from next time */
    }
    return 0;              /* nothing left */
}

void fish_path(char *buffer, size_t size)
{
    /* Walk from the current directory up to the root, collecting the
     * indices, then write them out backwards. */
    int chain[FISH_MAX_NODES];
    int depth = 0;
    int node = current;
    size_t pos = 0;

    if (size == 0) return;

    while (node > 0 && depth < FISH_MAX_NODES) {
        chain[depth++] = node;
        node = nodes[node].parent;
    }

    if (depth == 0) {                      /* we are at the root */
        if (size >= 2) { buffer[0] = '/'; buffer[1] = '\0'; }
        else buffer[0] = '\0';
        return;
    }

    for (int i = depth - 1; i >= 0; i--) {
        const char *name = nodes[chain[i]].name;

        if (pos + 1 < size) buffer[pos++] = '/';

        for (size_t j = 0; name[j] != '\0'; j++) {
            if (pos + 1 < size) buffer[pos++] = name[j];
        }
    }

    buffer[pos] = '\0';
}

const char *fish_error(int code)
{
    switch (code) {
    case FISH_OK:            return "ok";
    case FISH_ERR_EXISTS:    return "already exists";
    case FISH_ERR_NOTDIR:    return "not a directory";
    case FISH_ERR_NOENT:     return "no such file or directory";
    case FISH_ERR_FULL:      return "filesystem is full";
    case FISH_ERR_NAME:      return "bad name";
    case FISH_ERR_ISDIR:     return "is a directory";
    case FISH_ERR_NOTEMPTY:  return "directory is not empty";
    case FISH_ERR_SPACE:     return "file is too big";
    default:                 return "unknown error";
    }
}