/* =====================================================================
 * boot.S - MyOS boot entry point (x86, 32-bit, Multiboot 1 compliant)
 *
 * This is the very first code that runs. When the machine powers on,
 * BIOS -> GRUB (or QEMU's own built-in multiboot loader) jumps to the
 * _start label defined here. At this point the CPU is already in
 * 32-bit protected mode; the multiboot loader has taken care of that
 * for us, along with providing an initial (throwaway) GDT.
 *
 * Responsibilities:
 *   1) Place the Multiboot header at the very start of the binary
 *      (GRUB/QEMU scan the first 8KB of the file for it)
 *   2) Set up a stack
 *   3) Jump into the C entry point, kernel_main()
 * ===================================================================== */

.set MB_MAGIC,    0x1BADB002      /* Multiboot magic number            */
.set MB_ALIGN,    1 << 0          /* align loaded modules to 4KB       */
.set MB_MEMINFO,  1 << 1          /* ask the bootloader for a memory map */
.set MB_FLAGS,    (MB_ALIGN | MB_MEMINFO)
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

/* ---- Multiboot header: must live in the very first bytes of .text ---- */
.section .multiboot
.align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM

/* ---- Kernel stack - 16KB, lives in .bss (which gets zeroed) ---- */
.section .bss
.align 16
stack_bottom:
    .skip 16384        /* 16 KiB */
stack_top:

/* ---- Entry point ---- */
.section .text
.global _start
.type _start, @function
_start:
    /* Set up the stack pointer (x86 stacks grow downward) */
    mov $stack_top, %esp
    mov %esp, %ebp

    /* The multiboot loader leaves the magic number in eax and a
       pointer to the multiboot info struct in ebx. Pass both to C. */
    push %eax              /* magic number (2nd argument to kernel_main) */
    push %ebx               /* multiboot info struct (1st argument)      */

    /* Keep interrupts off until the C code has set up its own IDT */
    cli

    call kernel_main

    /* kernel_main should never return, but just in case: halt forever */
halt_loop:
    cli
    hlt
    jmp halt_loop

.size _start, . - _start
.section .note.GNU-stack,"",@progbits
