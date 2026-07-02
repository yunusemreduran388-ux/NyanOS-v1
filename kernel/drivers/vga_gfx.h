#ifndef VGA_GFX_H
#define VGA_GFX_H

#include "../../include/types.h"

#define SCREEN_W 320
#define SCREEN_H 200

void gfx_init(void);
void gfx_put_pixel(int x, int y, uint8_t color);
uint8_t gfx_get_pixel(int x, int y);
void gfx_fill_rect(int x, int y, int w, int h, uint8_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint8_t color);
void gfx_draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void gfx_draw_char(int x, int y, char c, uint8_t color);
void gfx_draw_string(int x, int y, const char *s, uint8_t color);
void gfx_flip(void);   /* copy the back buffer to the screen (flicker-free) */

/* Assign an RGB color (0-63 per channel) to a VGA DAC palette index */
void gfx_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

#endif
