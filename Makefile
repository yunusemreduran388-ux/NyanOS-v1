# =====================================================================
# Makefile - MyOS
#
# Usage:
#   make        -> builds myos.elf
#   make run    -> runs it in QEMU (requires qemu-system-i386)
#   make iso    -> builds a bootable myos.iso via GRUB (requires grub-mkrescue)
#   make clean  -> removes build artifacts
# =====================================================================

CC      = gcc
AS      = as
LD      = ld

CFLAGS  = -m32 -std=gnu99 -ffreestanding -fno-pie -fno-stack-protector \
          -fno-builtin -nostdlib -Wall -Wextra -O2 -g
ASFLAGS = --32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

BOOT_ASM_SRCS = boot/boot.S boot/gdt_flush.S boot/idt_flush.S boot/isr.S
BOOT_OBJS     = $(BOOT_ASM_SRCS:.S=.o)

C_SRCS = kernel/kernel.c \
         kernel/gdt.c \
         kernel/idt.c \
         kernel/fs.c \
         kernel/lib/kstring.c \
         kernel/drivers/vga_text.c \
         kernel/drivers/vga_gfx.c \
         kernel/drivers/font.c \
         kernel/drivers/keyboard.c \
         kernel/drivers/mouse.c \
         kernel/drivers/timer.c \
         kernel/gui/gui.c \
         kernel/gui/terminal_app.c \
         kernel/gui/notepad_app.c \
         kernel/gui/paint_app.c \
         kernel/gui/browser_app.c

C_OBJS = $(C_SRCS:.c=.o)

OBJS = $(BOOT_OBJS) $(C_OBJS)

KERNEL = myos.elf

all: $(KERNEL)

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)
	@echo ""
	@echo "==> $(KERNEL) built successfully."

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

iso: $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/myos.elf
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir

clean:
	rm -f $(OBJS) $(KERNEL) myos.iso
	rm -rf isodir

.PHONY: all run iso clean
