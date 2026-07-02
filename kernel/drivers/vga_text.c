/* =====================================================================
 * vga_text.c - VGA text mode (80x25) console driver
 *
 * BIOS/GRUB always starts the kernel in 80x25 text mode. Screen
 * memory sits at 0xB8000; every character is 2 bytes:
 * [ASCII code][color byte]. This driver gives us kprintf() without
 * relying on any libc.
 * ===================================================================== */

#include <stdarg.h>
#include "../../include/types.h"
#include "../lib/kstring.h"
#include "vga_text.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
static uint16_t * const VGA_MEMORY = (uint16_t*)0xB8000;

static size_t term_row, term_col;
static uint8_t term_color;
static uint16_t *term_buf;

static inline uint8_t vga_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void terminal_setcolor(uint8_t fg, uint8_t bg) {
    term_color = vga_color(fg, bg);
}

void terminal_init(void) {
    term_row = 0;
    term_col = 0;
    term_color = vga_color(7, 0); /* light grey on black */
    term_buf = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            term_buf[y * VGA_WIDTH + x] = vga_entry(' ', term_color);
}

static void terminal_scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            term_buf[(y - 1) * VGA_WIDTH + x] = term_buf[y * VGA_WIDTH + x];

    for (size_t x = 0; x < VGA_WIDTH; x++)
        term_buf[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', term_color);

    term_row = VGA_HEIGHT - 1;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\r') {
        term_col = 0;
    } else {
        term_buf[term_row * VGA_WIDTH + term_col] = vga_entry(c, term_color);
        if (++term_col == VGA_WIDTH) { term_col = 0; term_row++; }
    }
    if (term_row >= VGA_HEIGHT) terminal_scroll();
}

void terminal_write(const char *str) {
    for (size_t i = 0; str[i]; i++) terminal_putchar(str[i]);
}

/* A minimal printf: supports %s %d %x %c %% */
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char numbuf[32];

    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') { terminal_putchar(fmt[i]); continue; }
        i++;
        switch (fmt[i]) {
            case 's': terminal_write(va_arg(args, const char*)); break;
            case 'd': itoa(va_arg(args, int), numbuf, 10); terminal_write(numbuf); break;
            case 'x': itoa(va_arg(args, int), numbuf, 16); terminal_write(numbuf); break;
            case 'c': terminal_putchar((char)va_arg(args, int)); break;
            case '%': terminal_putchar('%'); break;
            default:  terminal_putchar('%'); terminal_putchar(fmt[i]); break;
        }
    }
    va_end(args);
}
