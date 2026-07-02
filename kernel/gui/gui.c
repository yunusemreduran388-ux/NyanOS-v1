/* =====================================================================
 * gui.c - window manager / desktop environment
 *
 * This is the core that turns the individual apps (Terminal, Notepad,
 * Paint, Browser, About) into an actual desktop: it owns window
 * geometry, z-order (which window is "on top" and gets focus/input),
 * dragging, close buttons, a taskbar with per-app buttons, and a
 * Start menu. Keyboard input goes to whichever window currently has
 * focus; mouse clicks are hit-tested top-down through the z-order.
 * ===================================================================== */

#include "../../include/types.h"
#include "../drivers/vga_gfx.h"
#include "../drivers/mouse.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../lib/kstring.h"
#include "gui.h"
#include "terminal_app.h"
#include "notepad_app.h"
#include "paint_app.h"
#include "browser_app.h"

/* ---- palette indices (assigned once via gfx_set_palette in gui_init) ---- */
#define C_BLACK      0
#define C_DESKTOP    1
#define C_TASKBAR    2
#define C_WIN_BG     3
#define C_WIN_TITLE  4
#define C_WHITE      5
#define C_RED        6
#define C_GREEN      7
#define C_SHADOW     8
#define C_YELLOW     9
#define C_CURSOR     10
#define C_FOCUS      11   /* brighter title bar for the focused window */

typedef struct {
    int x, y, w, h;
    const char *title;
    const char *short_label; /* used on the taskbar button */
    int visible;
} window_t;

/* index 0 unused (APP_NONE); indices 1..APP_COUNT-1 map to app_id_t */
static window_t windows[APP_COUNT];
static int z_order[APP_COUNT - 1];   /* app ids, back-to-front       */
static int z_count = 0;
static app_id_t focused_app = APP_NONE;

static int dragging_app = APP_NONE;
static int drag_off_x, drag_off_y;
static int start_menu_open = 0;
static int prev_left = 0;

static void setup_palette(void) {
    gfx_set_palette(C_BLACK,     0,  0,  0);
    gfx_set_palette(C_DESKTOP,   6, 28, 34);
    gfx_set_palette(C_TASKBAR,  12, 14, 18);
    gfx_set_palette(C_WIN_BG,   50, 50, 52);
    gfx_set_palette(C_WIN_TITLE, 8, 22, 48);
    gfx_set_palette(C_WHITE,   63, 63, 63);
    gfx_set_palette(C_RED,     46,  8,  8);
    gfx_set_palette(C_GREEN,    8, 40, 12);
    gfx_set_palette(C_SHADOW,  20, 20, 22);
    gfx_set_palette(C_YELLOW,  60, 55, 10);
    gfx_set_palette(C_CURSOR,  63, 63, 63);
    gfx_set_palette(C_FOCUS,   16, 40, 63);
}

static void z_bring_to_front(app_id_t id) {
    int pos = -1;
    for (int i = 0; i < z_count; i++) if (z_order[i] == (int)id) { pos = i; break; }
    if (pos >= 0) {
        for (int i = pos; i < z_count - 1; i++) z_order[i] = z_order[i + 1];
        z_order[z_count - 1] = id;
    } else {
        z_order[z_count++] = id;
    }
    focused_app = id;
}

void gui_open_app(app_id_t id) {
    if (id <= APP_NONE || id >= APP_COUNT) return;
    windows[id].visible = 1;
    z_bring_to_front(id);
}

void gui_close_app(app_id_t id) {
    if (id <= APP_NONE || id >= APP_COUNT) return;
    windows[id].visible = 0;
    if (focused_app == id) {
        focused_app = APP_NONE;
        for (int i = z_count - 1; i >= 0; i--) {
            if (windows[z_order[i]].visible) { focused_app = z_order[i]; break; }
        }
    }
}

void gui_browser_load(const char *filename) {
    app_browser_load(filename);
}

void gui_init(void) {
    setup_palette();

    windows[APP_ABOUT]    = (window_t){  8,   8, 118,  74, "About",    "ABOUT",   1 };
    windows[APP_TERMINAL] = (window_t){ 55,   6, 210, 132, "Terminal", "TERM",    1 };
    windows[APP_NOTEPAD]  = (window_t){ 90,  20, 176, 110, "Notepad",  "NOTEPAD", 0 };
    windows[APP_PAINT]    = (window_t){ 40,  40, 176, 110, "Paint",    "PAINT",   0 };
    windows[APP_BROWSER]  = (window_t){ 65,  15, 200, 132, "Browser",  "WEB",     0 };

    app_terminal_init();
    app_notepad_init();
    app_paint_init();
    app_browser_init();

    z_count = 0;
    z_bring_to_front(APP_ABOUT);
    z_bring_to_front(APP_TERMINAL); /* terminal opens focused, on top */
}

static void draw_about(int x, int y, int w, int h) {
    (void)w; (void)h;
    gfx_draw_string(x + 6, y + 16, "MYOS V0.2", C_WHITE);
    gfx_draw_string(x + 6, y + 28, "C + X86 ASM", C_WHITE);
    gfx_draw_string(x + 6, y + 40, "REAL KERNEL", C_YELLOW);
    gfx_draw_string(x + 6, y + 52, "GUI + SHELL", C_YELLOW);
}

static void draw_window_chrome(app_id_t id) {
    window_t *w = &windows[id];
    if (!w->visible) return;

    uint8_t title_color = (id == focused_app) ? C_FOCUS : C_WIN_TITLE;

    gfx_fill_rect(w->x + 3, w->y + 3, w->w, w->h, C_SHADOW);
    gfx_fill_rect(w->x, w->y, w->w, w->h, C_WIN_BG);
    gfx_fill_rect(w->x, w->y, w->w, 10, title_color);
    gfx_draw_rect(w->x, w->y, w->w, w->h, C_BLACK);
    gfx_draw_string(w->x + 3, w->y + 2, w->title, C_WHITE);

    gfx_fill_rect(w->x + w->w - 9, w->y + 1, 8, 8, C_RED);
    gfx_draw_rect(w->x + w->w - 9, w->y + 1, 8, 8, C_BLACK);

    int cx = w->x, cy = w->y, cw = w->w, ch = w->h;
    switch (id) {
        case APP_ABOUT:    draw_about(cx, cy, cw, ch); break;
        case APP_TERMINAL: app_terminal_render(cx, cy, cw, ch); break;
        case APP_NOTEPAD:  app_notepad_render(cx, cy, cw, ch); break;
        case APP_PAINT:    app_paint_render(cx, cy, cw, ch); break;
        case APP_BROWSER:  app_browser_render(cx, cy, cw, ch); break;
        default: break;
    }
}

/* ---- taskbar with a Start button, one button per app, and a clock ---- */
#define TASKBAR_H 14
#define START_W 34

static void draw_taskbar(void) {
    gfx_fill_rect(0, SCREEN_H - TASKBAR_H, SCREEN_W, TASKBAR_H, C_TASKBAR);
    gfx_draw_line(0, SCREEN_H - TASKBAR_H, SCREEN_W - 1, SCREEN_H - TASKBAR_H, C_WHITE);

    uint8_t btn_color = start_menu_open ? C_GREEN : C_WIN_TITLE;
    gfx_fill_rect(2, SCREEN_H - 12, START_W, 10, btn_color);
    gfx_draw_rect(2, SCREEN_H - 12, START_W, 10, C_BLACK);
    gfx_draw_string(5, SCREEN_H - 11, "MENU", C_WHITE);

    int bx = 2 + START_W + 4;
    for (int id = 1; id < APP_COUNT; id++) {
        int active = windows[id].visible;
        uint8_t c = (id == (int)focused_app) ? C_FOCUS : (active ? C_WIN_TITLE : C_SHADOW);
        int bw = 36;
        gfx_fill_rect(bx, SCREEN_H - 12, bw, 10, c);
        gfx_draw_rect(bx, SCREEN_H - 12, bw, 10, C_BLACK);
        gfx_draw_string(bx + 2, SCREEN_H - 11, windows[id].short_label, C_WHITE);
        bx += bw + 2;
    }

    char clock[12];
    uint32_t secs = timer_get_seconds();
    char buf[8]; itoa((int)secs, buf, 10);
    strcpy(clock, "T+"); strcat(clock, buf); strcat(clock, "s");
    gfx_draw_string(SCREEN_W - (int)strlen(clock) * 6 - 3, SCREEN_H - 11, clock, C_WHITE);
}

#define MENU_X 2
#define MENU_ITEM_H 11

static void draw_start_menu(void) {
    if (!start_menu_open) return;
    int count = APP_COUNT; /* apps + "Shut Down" */
    int mh = count * MENU_ITEM_H + 2;
    int my = SCREEN_H - TASKBAR_H - mh;
    int mw = 76;

    gfx_fill_rect(MENU_X, my, mw, mh, C_WIN_BG);
    gfx_draw_rect(MENU_X, my, mw, mh, C_BLACK);

    for (int id = 1; id < APP_COUNT; id++) {
        gfx_draw_string(MENU_X + 4, my + 2 + (id - 1) * MENU_ITEM_H, windows[id].title, C_BLACK);
    }
    gfx_draw_string(MENU_X + 4, my + 2 + (APP_COUNT - 1) * MENU_ITEM_H, "SHUT DOWN", C_RED);
}

static void draw_cursor(int x, int y) {
    for (int i = 0; i < 8; i++) gfx_draw_line(x, y, x, y + i, C_CURSOR);
    for (int i = 0; i < 6; i++) gfx_draw_line(x, y + i, x + i, y + 6, C_CURSOR);
    gfx_put_pixel(x, y, C_BLACK);
}

static int point_in(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void shutdown_screen(void) {
    gfx_fill_rect(0, 0, SCREEN_W, SCREEN_H, C_BLACK);
    gfx_draw_string(SCREEN_W / 2 - 48, SCREEN_H / 2, "SYSTEM HALTED", C_WHITE);
    gfx_draw_string(SCREEN_W / 2 - 60, SCREEN_H / 2 + 10, "IT IS SAFE TO CLOSE", C_WHITE);
    gfx_flip();
    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}

static void handle_input(void) {
    int mx = mouse_get_x();
    int my = mouse_get_y();
    int left = mouse_left_button();
    int clicked = (left && !prev_left);

    if (dragging_app != APP_NONE) {
        if (left) {
            windows[dragging_app].x = mx - drag_off_x;
            windows[dragging_app].y = my - drag_off_y;
        } else {
            dragging_app = APP_NONE;
        }
    } else if (clicked) {
        if (point_in(mx, my, 2, SCREEN_H - 12, START_W, 10)) {
            start_menu_open = !start_menu_open;
        } else if (start_menu_open) {
            int mh = APP_COUNT * MENU_ITEM_H + 2;
            int mmy = SCREEN_H - TASKBAR_H - mh;
            int handled = 0;
            for (int id = 1; id < APP_COUNT && !handled; id++) {
                if (point_in(mx, my, MENU_X, mmy + (id - 1) * MENU_ITEM_H, 76, MENU_ITEM_H)) {
                    gui_open_app((app_id_t)id);
                    handled = 1;
                }
            }
            if (!handled && point_in(mx, my, MENU_X, mmy + (APP_COUNT - 1) * MENU_ITEM_H, 76, MENU_ITEM_H)) {
                shutdown_screen();
            }
            start_menu_open = 0;
        } else {
            int bx = 2 + START_W + 4;
            int taskbar_handled = 0;
            for (int id = 1; id < APP_COUNT; id++) {
                int bw = 36;
                if (point_in(mx, my, bx, SCREEN_H - 12, bw, 10)) {
                    if (windows[id].visible && focused_app == (app_id_t)id) gui_close_app((app_id_t)id);
                    else gui_open_app((app_id_t)id);
                    taskbar_handled = 1;
                    break;
                }
                bx += bw + 2;
            }

            if (!taskbar_handled) {
                for (int i = z_count - 1; i >= 0; i--) {
                    app_id_t id = (app_id_t)z_order[i];
                    window_t *w = &windows[id];
                    if (!w->visible) continue;
                    if (!point_in(mx, my, w->x, w->y, w->w, w->h)) continue;

                    if (point_in(mx, my, w->x + w->w - 9, w->y + 1, 8, 8)) {
                        gui_close_app(id);
                    } else if (point_in(mx, my, w->x, w->y, w->w, 10)) {
                        z_bring_to_front(id);
                        dragging_app = id;
                        drag_off_x = mx - w->x;
                        drag_off_y = my - w->y;
                    } else {
                        z_bring_to_front(id);
                        if (id == APP_NOTEPAD) app_notepad_click(w->x, w->y, w->w, w->h, mx, my);
                        else if (id == APP_BROWSER) app_browser_click(w->x, w->y, w->w, w->h, mx, my);
                    }
                    break; /* topmost window under the cursor consumes the click */
                }
            }
        }
    }

    /* continuous painting while the left button is held (not just on click) */
    if (left && dragging_app == APP_NONE && !start_menu_open) {
        for (int i = z_count - 1; i >= 0; i--) {
            app_id_t id = (app_id_t)z_order[i];
            window_t *w = &windows[id];
            if (!w->visible || !point_in(mx, my, w->x, w->y, w->w, w->h)) continue;
            if (id == APP_PAINT) app_paint_mouse(w->x, w->y, w->w, w->h, mx, my, 1);
            break;
        }
    }

    prev_left = left;

    char c;
    while ((c = keyboard_get_char()) != 0) {
        if (focused_app == APP_TERMINAL) app_terminal_key(c);
        else if (focused_app == APP_NOTEPAD) app_notepad_key(c);
    }
}

static void render_frame(void) {
    gfx_fill_rect(0, 0, SCREEN_W, SCREEN_H, C_DESKTOP);

    for (int i = 0; i < z_count; i++) draw_window_chrome((app_id_t)z_order[i]);

    draw_taskbar();
    draw_start_menu();
    draw_cursor(mouse_get_x(), mouse_get_y());

    gfx_flip();
}

void gui_run(void) {
    for (;;) {
        handle_input();
        render_frame();
    }
}
