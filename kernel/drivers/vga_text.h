#ifndef VGA_TEXT_H
#define VGA_TEXT_H

void terminal_init(void);
void terminal_setcolor(uint8_t fg, uint8_t bg);
void terminal_putchar(char c);
void terminal_write(const char *str);
void kprintf(const char *fmt, ...);

#endif
