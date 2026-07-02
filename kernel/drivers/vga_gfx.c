/* =====================================================================
 * vga_gfx.c - VGA Mode 13h (320x200, 256 colors) graphics driver
 *
 * Important: a kernel booted via Multiboot already runs in 32-bit
 * protected mode, so we can NOT call real-mode BIOS services like
 * INT 0x10. Instead we program the VGA hardware registers (sequencer,
 * CRTC, graphics controller, attribute controller) directly through
 * port I/O to manually switch the screen into 320x200x256 graphics
 * mode. This is a well-known, standard technique that works on real
 * hardware as well as emulators like QEMU/Bochs.
 *
 * In Mode 13h, video memory is linear at 0xA0000; each byte selects
 * a pixel's color index (0-255) from the VGA palette.
 * ===================================================================== */

#include "../../include/io.h"
#include "../lib/kstring.h"
#include "vga_gfx.h"
#include "font.h"

static uint8_t * const VGA_FB = (uint8_t*)0xA0000;
static uint8_t back_buffer[SCREEN_W * SCREEN_H]; /* double buffering: no flicker */

static const uint8_t seq_regs[5]  = { 0x03, 0x01, 0x0F, 0x00, 0x0E };
static const uint8_t crtc_regs[25] = {
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF
};
static const uint8_t gc_regs[9]   = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF };
static const uint8_t ac_regs[21]  = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

static void set_mode_13h(void) {
    outb(0x3C2, 0x63);   /* Misc Output Register */

    for (int i = 0; i < 5; i++) { outb(0x3C4, i); outb(0x3C5, seq_regs[i]); }

    /* Unlock CRTC write protection (index 0x11, bit 7) */
    outb(0x3D4, 0x11);
    outb(0x3D5, inb(0x3D5) & 0x7F);
    for (int i = 0; i < 25; i++) { outb(0x3D4, i); outb(0x3D5, crtc_regs[i]); }

    for (int i = 0; i < 9; i++)  { outb(0x3CE, i); outb(0x3CF, gc_regs[i]); }

    for (int i = 0; i < 21; i++) {
        inb(0x3DA);                 /* reset the attribute controller flip-flop */
        outb(0x3C0, i);
        outb(0x3C0, ac_regs[i]);
    }
    inb(0x3DA);
    outb(0x3C0, 0x20);              /* re-enable video output */
}

void gfx_init(void) {
    set_mode_13h();
    memset(back_buffer, 0, sizeof(back_buffer));
}

void gfx_put_pixel(int x, int y, uint8_t color) {
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return;
    back_buffer[y * SCREEN_W + x] = color;
}

uint8_t gfx_get_pixel(int x, int y) {
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return 0;
    return back_buffer[y * SCREEN_W + x];
}

void gfx_fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            gfx_put_pixel(x + i, y + j, color);
}

void gfx_draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = 0; i < w; i++) {
        gfx_put_pixel(x + i, y, color);
        gfx_put_pixel(x + i, y + h - 1, color);
    }
    for (int j = 0; j < h; j++) {
        gfx_put_pixel(x, y + j, color);
        gfx_put_pixel(x + w - 1, y + j, color);
    }
}

/* Bresenham's line algorithm */
void gfx_draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = x1 - x0; if (dx < 0) dx = -dx;
    int dy = y1 - y0; if (dy < 0) dy = -dy;
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        gfx_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void gfx_draw_char(int x, int y, char c, uint8_t color) {
    const char* const* rows = font_get_glyph(c);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (rows[row][col] == '#')
                gfx_put_pixel(x + col, y + row, color);
        }
    }
}

void gfx_draw_string(int x, int y, const char *s, uint8_t color) {
    int cx = x;
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == '\n') { y += 9; cx = x; continue; }
        gfx_draw_char(cx, y, s[i], color);
        cx += 6; /* 5-pixel glyph + 1-pixel gap */
    }
}

void gfx_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);   /* palette index to write */
    outb(0x3C9, r & 0x3F);
    outb(0x3C9, g & 0x3F);
    outb(0x3C9, b & 0x3F);
}

/* Copy the back buffer to real VGA memory in one shot (flicker-free) */
void gfx_flip(void) {
    memcpy(VGA_FB, back_buffer, SCREEN_W * SCREEN_H);
}
