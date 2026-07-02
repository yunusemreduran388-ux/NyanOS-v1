/* gdt_flush - loads a new GDT and reloads all segment registers */
.section .text
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    mov 4(%esp), %eax    /* argument: address of the gdt_ptr struct */
    lgdt (%eax)

    mov $0x10, %ax        /* 0x10 = data segment selector (2nd GDT entry) */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* far jump to reload CS with the new code segment (0x08) */
    ljmp $0x08, $.flush_done
.flush_done:
    ret
.size gdt_flush, . - gdt_flush
.section .note.GNU-stack,"",@progbits
