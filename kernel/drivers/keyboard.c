/* =====================================================================
 * keyboard.c - PS/2 keyboard driver (IRQ1)
 *
 * Every key press/release fires hardware IRQ1. We catch that
 * interrupt, read a "scancode" byte from port 0x60, and translate it
 * to ASCII. A small ring buffer stores characters so the GUI/shell
 * can read them at its own pace.
 * ===================================================================== */

#include "../../include/io.h"
#include "../../include/idt.h"
#include "keyboard.h"

#define KBD_BUF_SIZE 256
static char kbd_buffer[KBD_BUF_SIZE];
static volatile int kbd_head = 0, kbd_tail = 0;
static int shift_pressed = 0;

/* Scancode Set 1 -> ASCII (lowercase / US QWERTY layout) */
static const char scancode_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ', 0,
};
static const char scancode_ascii_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ', 0,
};

static void kbd_push(char c) {
    int next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next != kbd_tail) {          /* only push if buffer isn't full */
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

static void keyboard_handler(registers_t *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);

    if (sc == 0x2A || sc == 0x36) { shift_pressed = 1; return; }        /* shift down */
    if (sc == 0xAA || sc == 0xB6) { shift_pressed = 0; return; }        /* shift up   */

    if (sc & 0x80) return; /* key-release events are ignored for now */

    if (sc < 128) {
        char c = shift_pressed ? scancode_ascii_shift[sc] : scancode_ascii[sc];
        if (c) kbd_push(c);
    }
}

void keyboard_init(void) {
    irq_install_handler(1, keyboard_handler);
}

char keyboard_get_char(void) {
    if (kbd_tail == kbd_head) return 0;
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return c;
}
