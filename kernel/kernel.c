/* =====================================================================
 * kernel.c - MyOS kernel entry point (kernel_main)
 *
 * boot/boot.S jumps here. This is the "conductor": it prints status
 * messages in text mode (so you can see each subsystem actually come
 * up), then switches to graphics mode and starts the GUI loop.
 * ===================================================================== */

#include "../include/types.h"
#include "../include/multiboot.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "drivers/vga_text.h"
#include "drivers/vga_gfx.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/timer.h"
#include "fs.h"
#include "gui/gui.h"

void kernel_main(multiboot_info_t *mb_info, uint32_t magic) {
    (void)mb_info;

    terminal_init();
    terminal_setcolor(10, 0);
    kprintf("MyOS kernel starting...\n");
    terminal_setcolor(7, 0);

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        terminal_setcolor(4, 0);
        kprintf("WARNING: multiboot magic mismatch (0x%x)\n", magic);
        terminal_setcolor(7, 0);
    }

    kprintf("[1/7] Setting up the GDT...\n");
    gdt_init();
    kprintf("      -> done\n");

    kprintf("[2/7] Setting up the IDT and PIC...\n");
    idt_init();
    kprintf("      -> done\n");

    kprintf("[3/7] Starting the PIT timer...\n");
    timer_init(100); /* 100 Hz */
    kprintf("      -> done\n");

    kprintf("[4/7] Keyboard driver...\n");
    keyboard_init();
    kprintf("      -> done\n");

    kprintf("[5/7] Mouse driver...\n");
    mouse_init();
    kprintf("      -> done\n");

    kprintf("[6/7] Mounting in-memory file system...\n");
    fs_init();
    kprintf("      -> done\n");

    kprintf("[7/7] Switching to graphics mode (320x200x256)...\n");
    kprintf("\nStarting the MyOS desktop, please wait...\n");

    for (volatile uint32_t i = 0; i < 30000000; i++) { }

    gfx_init();
    gui_init();
    gui_run();  /* never returns */

    for (;;) { __asm__ volatile ("hlt"); }
}
