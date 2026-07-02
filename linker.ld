/* linker.ld - MyOS memory layout
 *
 * The kernel is loaded starting at 1MB (0x100000). On x86 this is the
 * standard load address for multiboot-compliant kernels, sitting
 * above the BIOS/real-mode reserved area (0-1MB).
 */
ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* IMPORTANT: the multiboot header must live in the same output
       section as .text, and first. If it were its own output
       section, ld might not include it in any loadable segment and
       could push it past the file's first 8KB - the exact region
       GRUB/QEMU scan for the header. So we make it the first bytes
       of .text instead. */
    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata*)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }

    kernel_end = .;

    /* These sections aren't needed at runtime; explicitly discarding
       them prevents ld from silently resetting the location counter
       to 0 and corrupting the address of every section after it. */
    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.comment)
        *(.eh_frame)
        *(.debug*)
    }
}
