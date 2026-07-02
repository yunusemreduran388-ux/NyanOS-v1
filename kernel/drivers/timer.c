/* =====================================================================
 * timer.c - Programmable Interval Timer (PIT) driver (IRQ0)
 *
 * The PIT is the classic x86 hardware timer. We program channel 0 to
 * fire IRQ0 at a fixed frequency, and count ticks in a handler. This
 * gives the rest of the kernel a real, interrupt-driven notion of
 * elapsed time (used for the taskbar clock and could later drive a
 * task scheduler).
 * ===================================================================== */

#include "../../include/io.h"
#include "../../include/idt.h"
#include "timer.h"

#define PIT_FREQUENCY 1193182

static volatile uint32_t ticks = 0;
static uint32_t configured_hz = 100;

static void timer_handler(registers_t *regs) {
    (void)regs;
    ticks++;
}

void timer_init(uint32_t frequency_hz) {
    configured_hz = frequency_hz;
    uint32_t divisor = PIT_FREQUENCY / frequency_hz;

    outb(0x43, 0x36);                    /* channel 0, low+high byte, mode 3 (square wave) */
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    irq_install_handler(0, timer_handler);
}

uint32_t timer_get_ticks(void) { return ticks; }
uint32_t timer_get_seconds(void) { return ticks / configured_hz; }
