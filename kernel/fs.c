/* =====================================================================
 * fs.c - MyOS in-memory ("RAM") file system
 *
 * MyOS has no disk driver yet, so this is a simple, flat, in-memory
 * file table: fixed-size slots, each holding a name and a data
 * buffer. It's enough to give the shell, text editor and document
 * viewer a real, shared, persistent-for-the-session storage layer
 * with actual read/write/list/delete semantics - a genuine (if
 * simple) file system API, just backed by RAM instead of a disk.
 * A future improvement would be a real block-device driver (e.g.
 * ATA/IDE) plus an on-disk format such as FAT.
 * ===================================================================== */

#include "fs.h"
#include "lib/kstring.h"

static fs_file_t files[FS_MAX_FILES];

static fs_file_t* fs_find(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && strcmp(files[i].name, name) == 0)
            return &files[i];
    return 0;
}

static fs_file_t* fs_find_free(void) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (!files[i].used) return &files[i];
    return 0;
}

void fs_init(void) {
    memset(files, 0, sizeof(files));

    const char *welcome =
        "Welcome to MyOS!\n"
        "\n"
        "This is a text file stored in the in-memory file system.\n"
        "Try these Terminal commands:\n"
        "  help          - list all commands\n"
        "  ls            - list files\n"
        "  cat readme.txt- view this file\n"
        "  write <f> ... - create/overwrite a file\n"
        "  notepad       - open the text editor\n"
        "  paint         - open the paint app\n"
        "  browser       - open the document viewer\n";
    fs_write("readme.txt", welcome, strlen(welcome));

    const char *page =
        "# MyOS Browser\n"
        "\n"
        "This is a local document, not a real webpage.\n"
        "\n"
        "* No network stack is implemented\n"
        "* No HTML/CSS/JS rendering engine exists\n"
        "* This viewer only understands simple markup:\n"
        "  lines starting with '#' are headings,\n"
        "  lines starting with '*' are bullets\n"
        "\n"
        "# Why not a real browser?\n"
        "A real browser needs a network driver, a TCP/IP stack, a DNS\n"
        "resolver, TLS, and an HTML/CSS/JS engine - realistically\n"
        "years of engineering. This viewer is the honest, working\n"
        "equivalent that fits a hobby kernel.\n";
    fs_write("home.page", page, strlen(page));
}

int fs_write(const char *name, const char *data, uint32_t len) {
    if (len >= FS_MAX_FILE_SIZE) len = FS_MAX_FILE_SIZE - 1;

    fs_file_t *f = fs_find(name);
    if (!f) f = fs_find_free();
    if (!f) return -1; /* file table full */

    strncpy(f->name, name, FS_MAX_NAME - 1);
    f->name[FS_MAX_NAME - 1] = 0;
    memcpy(f->data, data, len);
    f->data[len] = 0;
    f->size = len;
    f->used = 1;
    return (int)len;
}

int fs_read(const char *name, char *out_buf, uint32_t max_len) {
    fs_file_t *f = fs_find(name);
    if (!f) return -1;
    uint32_t n = f->size < max_len ? f->size : max_len;
    memcpy(out_buf, f->data, n);
    if (n < max_len) out_buf[n] = 0;
    return (int)n;
}

int fs_delete(const char *name) {
    fs_file_t *f = fs_find(name);
    if (!f) return -1;
    f->used = 0;
    return 0;
}

int fs_exists(const char *name) {
    return fs_find(name) != 0;
}

int fs_list(char out[][FS_MAX_NAME], int max_count) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES && n < max_count; i++) {
        if (files[i].used) {
            strncpy(out[n], files[i].name, FS_MAX_NAME - 1);
            out[n][FS_MAX_NAME - 1] = 0;
            n++;
        }
    }
    return n;
}
