/* =====================================================================
 * isr.S - Interrupt Service Routine / IRQ entry stubs
 *
 * When the CPU takes an exception or a hardware IRQ fires, it can't
 * jump straight into a C function; we first need to save registers,
 * fix up segments, and only then call into a common C dispatcher
 * (isr_handler / irq_handler).
 * ===================================================================== */

.macro ISR_NOERR num
.global isr\num
isr\num:
    cli
    push $0            /* fake error code (this exception has none) */
    push $\num
    jmp isr_common_stub
.endm

.macro ISR_ERR num
.global isr\num
isr\num:
    cli
    push $\num         /* CPU already pushed a real error code */
    jmp isr_common_stub
.endm

.macro IRQ num, mapped_num
.global irq\num
irq\num:
    cli
    push $0
    push $\mapped_num
    jmp irq_common_stub
.endm

/* CPU exceptions (0-31). Vectors 8, 10-14, 17 push a real error code. */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

/* Hardware IRQs (remapped by the PIC to vectors 32-47) */
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.extern isr_handler
.extern irq_handler

isr_common_stub:
    pusha
    mov %ds, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp
    call isr_handler
    add $4, %esp

    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    popa
    add $8, %esp
    iret

irq_common_stub:
    pusha
    mov %ds, %ax
    push %eax

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp
    call irq_handler
    add $4, %esp

    pop %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    popa
    add $8, %esp
    iret
.section .note.GNU-stack,"",@progbits
