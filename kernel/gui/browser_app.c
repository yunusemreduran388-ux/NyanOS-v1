/* =====================================================================
 * browser_app.c - a local document viewer ("Browser")
 *
 * HONEST SCOPE NOTE: this is NOT a real internet browser. MyOS has no
 * network driver, no TCP/IP stack, no DNS, no TLS, and no HTML/CSS/JS
 * engine - building those from scratch is a multi-year engineering
 * effort even for professional teams. What this app *does* do for
 * real: it reads pages out of the in-memory file system (fs.c) and
 * renders a tiny, self-invented markup language (lines starting with
 * '#' are headings, lines starting with '*' are bullets) with a
 * working address bar, Home button, and Reload button. It's an
 * honest, working stand-in that fits what a hobby kernel can
 * realistically support.
 * ===================================================================== */

#include "../../include/types.h"
#include "../drivers/vga_gfx.h"
#include "../lib/kstring.h"
#include "../fs.h"
#include "browser_app.h"

#define PAGE_MAX 4096
static char page_buf[PAGE_MAX];
static char current_file[FS_MAX_NAME] = "home.page";

#define BTN_Y 12
#define BTN_H 9

void app_browser_load(const char *filename) {
    strncpy(current_file, filename, FS_MAX_NAME - 1);
    current_file[FS_MAX_NAME - 1] = 0;
    int n = fs_read(current_file, page_buf, PAGE_MAX - 1);
    if (n < 0) {
        strcpy(page_buf, "404 - page not found in the local file system.");
    } else {
        page_buf[n] = 0;
    }
}

void app_browser_init(void) {
    app_browser_load("home.page");
}

void app_browser_click(int x, int y, int w, int h, int mx, int my) {
    (void)h;
    int lx = mx - x, ly = my - y;
    if (ly >= BTN_Y && ly < BTN_Y + BTN_H) {
        if (lx >= 4 && lx < 40) app_browser_load("home.page");          /* Home   */
        else if (lx >= 44 && lx < 80) app_browser_load(current_file);   /* Reload */
        else if (lx >= w - 44 && lx < w - 4) app_browser_load("readme.txt");
    }
}

void app_browser_render(int x, int y, int w, int h) {
    gfx_fill_rect(x + 4, y + BTN_Y, 34, BTN_H, 4);
    gfx_draw_string(x + 6, y + BTN_Y + 1, "HOME", 5);

    gfx_fill_rect(x + 44, y + BTN_Y, 34, BTN_H, 9);
    gfx_draw_string(x + 46, y + BTN_Y + 1, "RELD", 0);

    gfx_fill_rect(x + w - 44, y + BTN_Y, 40, BTN_H, 7);
    gfx_draw_string(x + w - 42, y + BTN_Y + 1, "README", 0);

    /* "address bar" showing which local file is loaded */
    gfx_fill_rect(x + 4, y + BTN_Y + BTN_H + 2, w - 8, 9, 5);
    gfx_draw_string(x + 6, y + BTN_Y + BTN_H + 3, current_file, 0);

    int max_cols = (w - 8) / 6;
    if (max_cols < 4) max_cols = 4;
    int cy = BTN_Y + BTN_H + 14;
    int col = 0, cx = 0;
    uint8_t color = 0;
    int line_start = 1;

    for (int i = 0; page_buf[i]; i++) {
        char c = page_buf[i];

        if (line_start) {
            color = 0;
            if (c == '#') color = 4;       /* heading */
            else if (c == '*') color = 7;  /* bullet  */
            line_start = 0;
        }

        if (c == '\n') {
            cy += 9; col = 0; cx = 0; line_start = 1;
            continue;
        }
        if (col >= max_cols) { cy += 9; col = 0; cx = 0; }
        if (cy > h - 8) break;

        gfx_draw_char(x + 4 + cx, y + cy, c, color);
        cx += 6; col++;
    }
}
