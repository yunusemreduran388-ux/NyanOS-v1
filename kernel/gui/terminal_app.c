/* =====================================================================
 * terminal_app.c - A real, working command shell
 *
 * This isn't a decorative text box: keystrokes go through the actual
 * PS/2 keyboard IRQ handler, land in a line buffer, and on Enter get
 * tokenized and dispatched to real command implementations that read
 * and write the in-memory file system, open other apps, query the
 * PIT-based uptime, etc.
 * ===================================================================== */

#include "../../include/types.h"
#include "../drivers/vga_gfx.h"
#include "../drivers/timer.h"
#include "../lib/kstring.h"
#include "../fs.h"
#include "gui.h"
#include "terminal_app.h"

#define TERM_COLS   30
#define TERM_ROWS   14
#define LINE_MAX    64

static char screen[TERM_ROWS][TERM_COLS + 1];
static int  cur_row = 0;

static char line_buf[LINE_MAX];
static int  line_len = 0;

#define PROMPT "myos> "

static void scroll_up(void) {
    for (int i = 1; i < TERM_ROWS; i++) memcpy(screen[i - 1], screen[i], TERM_COLS + 1);
    memset(screen[TERM_ROWS - 1], 0, TERM_COLS + 1);
    cur_row = TERM_ROWS - 1;
}

static void new_line(void) {
    cur_row++;
    if (cur_row >= TERM_ROWS) scroll_up();
    memset(screen[cur_row], 0, TERM_COLS + 1);
}

static void print_str(const char *s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\n') { new_line(); continue; }
        int col = (int)strlen(screen[cur_row]);
        if (col >= TERM_COLS) new_line();
        col = (int)strlen(screen[cur_row]);
        screen[cur_row][col] = s[i];
        screen[cur_row][col + 1] = 0;
    }
}

static void print_prompt(void) {
    new_line();
    print_str(PROMPT);
}

/* ---- tiny argv tokenizer (splits on spaces, in place) ---- */
#define MAX_ARGS 8
static char *argv[MAX_ARGS];
static int argc;

static void tokenize(char *s) {
    argc = 0;
    int in_tok = 0;
    for (int i = 0; s[i] && argc < MAX_ARGS; i++) {
        if (s[i] == ' ') {
            s[i] = 0;
            in_tok = 0;
        } else if (!in_tok) {
            argv[argc++] = &s[i];
            in_tok = 1;
        }
    }
}

/* Joins argv[from..argc) back with single spaces, for commands like
   "write <file> <rest of the line>" */
static void join_rest(int from, char *out) {
    out[0] = 0;
    for (int i = from; i < argc; i++) {
        if (i > from) strcat(out, " ");
        strcat(out, argv[i]);
    }
}

static int simple_calc(int a, char op, int b) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b != 0 ? a / b : 0;
        default:  return 0;
    }
}

static void cmd_help(void) {
    print_str("Commands:\n");
    print_str(" help ls cat write rm echo\n");
    print_str(" clear about ver uptime whoami\n");
    print_str(" calc notepad paint browser\n");
    print_str(" open <app>  reboot\n");
}

static void run_command(char *cmdline) {
    tokenize(cmdline);
    if (argc == 0) return;

    if (strcmp(argv[0], "help") == 0) {
        cmd_help();
    } else if (strcmp(argv[0], "clear") == 0) {
        for (int i = 0; i < TERM_ROWS; i++) memset(screen[i], 0, TERM_COLS + 1);
        cur_row = -1; /* next new_line() brings it to 0 */
    } else if (strcmp(argv[0], "echo") == 0) {
        char joined[LINE_MAX]; join_rest(1, joined);
        print_str(joined); print_str("\n");
    } else if (strcmp(argv[0], "ls") == 0) {
        char names[FS_MAX_FILES][FS_MAX_NAME];
        int n = fs_list(names, FS_MAX_FILES);
        if (n == 0) print_str("(no files)\n");
        for (int i = 0; i < n; i++) { print_str(names[i]); print_str("\n"); }
    } else if (strcmp(argv[0], "cat") == 0) {
        if (argc < 2) { print_str("usage: cat <file>\n"); }
        else {
            char buf[FS_MAX_FILE_SIZE];
            int n = fs_read(argv[1], buf, sizeof(buf) - 1);
            if (n < 0) print_str("file not found\n");
            else { buf[n] = 0; print_str(buf); print_str("\n"); }
        }
    } else if (strcmp(argv[0], "write") == 0) {
        if (argc < 2) { print_str("usage: write <file> <text>\n"); }
        else {
            char joined[LINE_MAX]; join_rest(2, joined);
            fs_write(argv[1], joined, (uint32_t)strlen(joined));
            print_str("saved.\n");
        }
    } else if (strcmp(argv[0], "rm") == 0) {
        if (argc < 2) print_str("usage: rm <file>\n");
        else if (fs_delete(argv[1]) == 0) print_str("deleted.\n");
        else print_str("file not found\n");
    } else if (strcmp(argv[0], "about") == 0 || strcmp(argv[0], "ver") == 0) {
        print_str("MyOS v0.2 - a hobby kernel\n");
        print_str("Written in C and x86 assembly\n");
    } else if (strcmp(argv[0], "whoami") == 0) {
        print_str("root\n");
    } else if (strcmp(argv[0], "uptime") == 0) {
        char buf[16];
        itoa((int)timer_get_seconds(), buf, 10);
        print_str("up "); print_str(buf); print_str("s\n");
    } else if (strcmp(argv[0], "calc") == 0) {
        if (argc < 4) print_str("usage: calc <a> <op> <b>\n");
        else {
            int a = 0, sign = 1, i = 0;
            if (argv[1][0] == '-') { sign = -1; i = 1; }
            for (; argv[1][i]; i++) a = a * 10 + (argv[1][i] - '0');
            a *= sign;
            int b = 0; sign = 1; i = 0;
            if (argv[3][0] == '-') { sign = -1; i = 1; }
            for (; argv[3][i]; i++) b = b * 10 + (argv[3][i] - '0');
            b *= sign;
            int r = simple_calc(a, argv[2][0], b);
            char buf[16]; itoa(r, buf, 10);
            print_str("= "); print_str(buf); print_str("\n");
        }
    } else if (strcmp(argv[0], "notepad") == 0 || strcmp(argv[0], "open") == 0
               || strcmp(argv[0], "paint") == 0 || strcmp(argv[0], "browser") == 0) {
        const char *target = strcmp(argv[0], "open") == 0 ? (argc > 1 ? argv[1] : "") : argv[0];
        if (strcmp(target, "notepad") == 0) { gui_open_app(APP_NOTEPAD); print_str("opening Notepad...\n"); }
        else if (strcmp(target, "paint") == 0) { gui_open_app(APP_PAINT); print_str("opening Paint...\n"); }
        else if (strcmp(target, "browser") == 0) {
            if (argc > 1 && strcmp(argv[0], "browser") == 0) gui_browser_load(argv[1]);
            gui_open_app(APP_BROWSER);
            print_str("opening Browser...\n");
        }
        else if (strcmp(target, "about") == 0) { gui_open_app(APP_ABOUT); print_str("opening About...\n"); }
        else print_str("unknown app\n");
    } else if (strcmp(argv[0], "reboot") == 0 || strcmp(argv[0], "halt") == 0) {
        print_str("halting CPU...\n");
        gfx_fill_rect(0, 0, SCREEN_W, SCREEN_H, 0);
        gfx_draw_string(SCREEN_W/2 - 40, SCREEN_H/2, "SYSTEM HALTED", 5);
        gfx_flip();
        __asm__ volatile ("cli");
        for (;;) __asm__ volatile ("hlt");
    } else {
        print_str("unknown command: ");
        print_str(argv[0]);
        print_str("\n");
    }
}

void app_terminal_init(void) {
    memset(screen, 0, sizeof(screen));
    cur_row = -1;
    print_str("MyOS Terminal - type 'help'\n");
    print_prompt();
    line_len = 0;
    line_buf[0] = 0;
}

void app_terminal_key(char c) {
    if (c == '\n') {
        print_str("\n");
        line_buf[line_len] = 0;
        run_command(line_buf);
        line_len = 0;
        line_buf[0] = 0;
        print_prompt();
    } else if (c == '\b') {
        if (line_len > 0) {
            line_len--;
            line_buf[line_len] = 0;
            int col = (int)strlen(screen[cur_row]);
            if (col > 0) screen[cur_row][col - 1] = 0;
        }
    } else if (c >= 32 && c < 127 && line_len < LINE_MAX - 1) {
        line_buf[line_len++] = c;
        line_buf[line_len] = 0;
        char tmp[2] = { c, 0 };
        print_str(tmp);
    }
}

void app_terminal_render(int x, int y, int w, int h) {
    (void)w; (void)h;
    for (int r = 0; r < TERM_ROWS; r++) {
        if (12 + r * 8 > h - 4) break;
        gfx_draw_string(x + 3, y + 12 + r * 8, screen[r], 7 /* C_GREEN */);
    }
}
