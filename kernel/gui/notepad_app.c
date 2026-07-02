/* =====================================================================
 * notepad_app.c - a small text editor
 *
 * Typed characters go straight into a text buffer via the real
 * keyboard IRQ path. Clicking "SAVE" writes the buffer out through
 * fs_write() into the in-memory file system (so it can be read back
 * with the Terminal's "cat notepad.txt", or reloaded with "LOAD").
 * ===================================================================== */

#include "../../include/types.h"
#include "../drivers/vga_gfx.h"
#include "../lib/kstring.h"
#include "../fs.h"
#include "notepad_app.h"

#define NOTE_MAX 1024
#define NOTE_FILE "notepad.txt"

static char buf[NOTE_MAX];
static int  len = 0;
static char status[32] = "";

void app_notepad_init(void) {
    int n = fs_read(NOTE_FILE, buf, NOTE_MAX - 1);
    if (n < 0) {
        buf[0] = 0;
        len = 0;
    } else {
        len = n;
        buf[len] = 0;
    }
    strcpy(status, "");
}

void app_notepad_key(char c) {
    if (c == '\b') {
        if (len > 0) buf[--len] = 0;
    } else if (c == '\n') {
        if (len < NOTE_MAX - 1) { buf[len++] = '\n'; buf[len] = 0; }
    } else if (c >= 32 && c < 127 && len < NOTE_MAX - 1) {
        buf[len++] = c;
        buf[len] = 0;
    }
    strcpy(status, "");
}

/* button geometry, window-relative, shared between render() and click() */
#define BTN_SAVE_X 4
#define BTN_SAVE_Y 12
#define BTN_W 34
#define BTN_H 9
#define BTN_LOAD_X (BTN_SAVE_X + BTN_W + 4)
#define BTN_CLEAR_X (BTN_LOAD_X + BTN_W + 4)

int app_notepad_click(int x, int y, int w, int h, int mx, int my) {
    (void)w; (void)h;
    int lx = mx - x, ly = my - y;

    if (ly >= BTN_SAVE_Y && ly < BTN_SAVE_Y + BTN_H) {
        if (lx >= BTN_SAVE_X && lx < BTN_SAVE_X + BTN_W) {
            fs_write(NOTE_FILE, buf, (uint32_t)len);
            strcpy(status, "Saved!");
            return 1;
        }
        if (lx >= BTN_LOAD_X && lx < BTN_LOAD_X + BTN_W) {
            int n = fs_read(NOTE_FILE, buf, NOTE_MAX - 1);
            len = (n < 0) ? 0 : n;
            buf[len] = 0;
            strcpy(status, "Loaded!");
            return 1;
        }
        if (lx >= BTN_CLEAR_X && lx < BTN_CLEAR_X + BTN_W) {
            len = 0; buf[0] = 0;
            strcpy(status, "Cleared");
            return 1;
        }
    }
    return 0;
}

void app_notepad_render(int x, int y, int w, int h) {
    gfx_fill_rect(x + BTN_SAVE_X,  y + BTN_SAVE_Y, BTN_W, BTN_H, 7 /* green */);
    gfx_draw_string(x + BTN_SAVE_X + 3, y + BTN_SAVE_Y + 1, "SAVE", 5);

    gfx_fill_rect(x + BTN_LOAD_X,  y + BTN_SAVE_Y, BTN_W, BTN_H, 9 /* yellow */);
    gfx_draw_string(x + BTN_LOAD_X + 3, y + BTN_SAVE_Y + 1, "LOAD", 0);

    gfx_fill_rect(x + BTN_CLEAR_X, y + BTN_SAVE_Y, BTN_W, BTN_H, 6 /* red */);
    gfx_draw_string(x + BTN_CLEAR_X + 1, y + BTN_SAVE_Y + 1, "CLR", 5);

    if (status[0]) gfx_draw_string(x + w - 40, y + BTN_SAVE_Y + 1, status, 9);

    /* text area: word-wrap into the window at ~ (w-8)/6 chars per line */
    int max_cols = (w - 8) / 6;
    if (max_cols < 4) max_cols = 4;
    int cx = 0, cy = BTN_SAVE_Y + BTN_H + 4;
    int col = 0;
    for (int i = 0; i < len; i++) {
        char c = buf[i];
        if (c == '\n' || col >= max_cols) {
            cy += 8; col = 0; cx = 0;
            if (c == '\n') continue;
        }
        if (cy > h - 8) break;
        gfx_draw_char(x + 4 + cx, y + cy, c, 0);
        cx += 6; col++;
    }
    /* blinking-less cursor marker at the end of the text */
    gfx_draw_char(x + 4 + cx, y + cy, '_', 4);
}
