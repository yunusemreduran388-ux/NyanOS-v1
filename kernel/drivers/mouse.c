/* =====================================================================
 * mouse.c - PS/2 mouse driver (IRQ12)
 *
 * The PS/2 mouse lives on the second channel of the "8042" keyboard
 * controller. Enabling it requires sending a handful of setup
 * commands to the controller. After that, the mouse sends a 3-byte
 * packet on every movement/click (IRQ12); we assemble those bytes
 * and update the cursor's x/y position and left-button state.
 * ===================================================================== */

#include "../../include/io.h"
#include "../../include/idt.h"
#include "mouse.h"

#define MOUSE_SCREEN_W 320
#define MOUSE_SCREEN_H 200

static int mx = MOUSE_SCREEN_W / 2, my = MOUSE_SCREEN_H / 2;
static int left_btn = 0;

static uint8_t mouse_cycle = 0;
static int8_t  mouse_byte[3];

static void mouse_wait_write(void) {
    uint32_t timeout = 100000;
    while (timeout--) if ((inb(0x64) & 2) == 0) return;
}
static void mouse_wait_read(void) {
    uint32_t timeout = 100000;
    while (timeout--) if (inb(0x64) & 1) return;
}

static void mouse_write(uint8_t val) {
    mouse_wait_write();
    outb(0x64, 0xD4);   /* "next command goes to the mouse channel" */
    mouse_wait_write();
    outb(0x60, val);
}
static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(0x60);
}

static void mouse_handler(registers_t *regs) {
    (void)regs;
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) return; /* this data isn't from the mouse */

    switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = inb(0x60);
            if (!(mouse_byte[0] & 0x08)) return; /* out of sync, skip */
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = inb(0x60);
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = inb(0x60);
            mouse_cycle = 0;

            left_btn = mouse_byte[0] & 0x01;

            mx += mouse_byte[1];
            my -= mouse_byte[2]; /* the Y axis is inverted on PS/2 */

            if (mx < 0) mx = 0;
            if (my < 0) my = 0;
            if (mx > MOUSE_SCREEN_W - 1)  mx = MOUSE_SCREEN_W - 1;
            if (my > MOUSE_SCREEN_H - 1) my = MOUSE_SCREEN_H - 1;
            break;
    }
}

void mouse_init(void) {
    mouse_wait_write();
    outb(0x64, 0xA8);              /* enable the auxiliary (mouse) port */

    mouse_wait_write();
    outb(0x64, 0x20);              /* read controller status byte */
    mouse_wait_read();
    uint8_t status = inb(0x60) | 2;   /* enable IRQ12 (bit 1) */
    status &= ~0x20;                  /* enable the mouse clock          */
    mouse_wait_write();
    outb(0x64, 0x60);
    mouse_wait_write();
    outb(0x60, status);

    mouse_write(0xF6);  /* reset the mouse to defaults */
    mouse_read();        /* ACK (0xFA) */

    mouse_write(0xF4);  /* enable data streaming */
    mouse_read();        /* ACK */

    irq_install_handler(12, mouse_handler);
}

int mouse_get_x(void) { return mx; }
int mouse_get_y(void) { return my; }
int mouse_left_button(void) { return left_btn; }
