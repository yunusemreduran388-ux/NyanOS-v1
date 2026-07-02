/* =====================================================================
 * paint_app.c - a tiny bitmap paint program
 *
 * Holds its own off-screen canvas buffer. Dragging the (real, PS/2
 * IRQ-driven) mouse with the left button held paints pixels into it;
 * the window manager blits the canvas into the window every frame.
 * ===================================================================== */

#include "../../include/types.h"
#include "../drivers/vga_gfx.h"
#include "../lib/kstring.h"
#include "paint_app.h"

#define CANVAS_W 150
#define CANVAS_H 70
static uint8_t canvas[CANVAS_W * CANVAS_H];

static uint8_t current_color = 6; /* red */

/* A small custom palette of swatches, drawn along the top of the window */
static const uint8_t palette[] = { 0, 5, 6, 7, 9, 4, 3, 8 };
#define PALETTE_COUNT (sizeof(palette) / sizeof(palette[0]))
#define SWATCH_SIZE 10
#define SWATCH_Y    12
#define CANVAS_Y    24

void app_paint_init(void) {
    memset(canvas, 3 /* light grey background */, sizeof(canvas));
}

static void canvas_set(int cx, int cy, uint8_t color) {
    if (cx < 0 || cy < 0 || cx >= CANVAS_W || cy >= CANVAS_H) return;
    canvas[cy * CANVAS_W + cx] = color;
}

void app_paint_mouse(int x, int y, int w, int h, int mx, int my, int left_down) {
    (void)h;
    int lx = mx - x, ly = my - y;

    if (!left_down) return;

    /* clicking a palette swatch selects the draw color */
    if (ly >= SWATCH_Y && ly < SWATCH_Y + SWATCH_SIZE) {
        int idx = (lx - 4) / (SWATCH_SIZE + 2);
        if (idx >= 0 && (unsigned)idx < PALETTE_COUNT && lx >= 4) {
            current_color = palette[idx];
            return;
        }
    }

    /* Clear button, top-right */
    if (ly >= SWATCH_Y && ly < SWATCH_Y + SWATCH_SIZE && lx >= w - 30 && lx < w - 4) {
        memset(canvas, 3, sizeof(canvas));
        return;
    }

    /* drawing on the canvas (a small 3x3 brush) */
    int cx = lx - 4, cy = ly - CANVAS_Y;
    if (cx >= 0 && cy >= 0 && cx < CANVAS_W && cy < CANVAS_H) {
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                canvas_set(cx + dx, cy + dy, current_color);
    }
}

void app_paint_render(int x, int y, int w, int h) {
    (void)h;
    for (unsigned i = 0; i < PALETTE_COUNT; i++) {
        int sx = x + 4 + i * (SWATCH_SIZE + 2);
        gfx_fill_rect(sx, y + SWATCH_Y, SWATCH_SIZE, SWATCH_SIZE, palette[i]);
        gfx_draw_rect(sx, y + SWATCH_Y, SWATCH_SIZE, SWATCH_SIZE,
                       palette[i] == current_color ? 5 : 0);
    }
    gfx_fill_rect(x + w - 30, y + SWATCH_Y, 26, SWATCH_SIZE, 6);
    gfx_draw_string(x + w - 28, y + SWATCH_Y + 1, "CLR", 5);

    for (int cy = 0; cy < CANVAS_H; cy++)
        for (int cx = 0; cx < CANVAS_W; cx++)
            gfx_put_pixel(x + 4 + cx, y + CANVAS_Y + cy, canvas[cy * CANVAS_W + cx]);

    gfx_draw_rect(x + 4, y + CANVAS_Y, CANVAS_W, CANVAS_H, 0);
}
