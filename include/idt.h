#ifndef IDT_H
#define IDT_H

#include "types.h"

typedef struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*isr_handler_t)(registers_t*);

void idt_init(void);
void irq_install_handler(int irq, isr_handler_t handler);
void pic_send_eoi(uint8_t irq);

#endif
