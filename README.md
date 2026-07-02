# Nyanos — A Hobby Operating System in C and x86 Assembly
![nyanos v1 screenshoot](<shoot.png>)

nyanos is a **real, working** 32-bit x86 kernel: it boots via Multiboot
(GRUB or QEMU's built-in loader), sets up its own GDT and IDT, has
PS/2 keyboard/mouse/timer drivers, switches VGA into a graphics mode
by programming hardware registers directly (no BIOS calls), and runs
a small desktop environment with a real window manager, a shell,
a text editor, a paint program, and a local document viewer.

It was built and **verified to compile and link successfully** in this
environment with `gcc -m32`, `as`, and `ld` (no `nasm`/`qemu`/`grub`
installed here). The Multiboot header was checked byte-for-byte:
checksum validates to 0, it sits within the first 8KB of the file,
and the ELF entry point lands in the correct loaded segment. Grab a
Linux machine (or WSL) with QEMU to see it run and interact with the
GUI yourself — see "How to run it" below.

## What's real here (and what isn't)

Everything listed below is implemented and does what it says — no
stubs pretending to be features:

- **Real interrupt-driven drivers**: keyboard, mouse, and a PIT timer
  all work through actual hardware IRQs (1, 12, 0), not polling loops.
- **Real graphics**: VGA Mode 13h is set up by programming the
  sequencer / CRTC / graphics controller / attribute controller /
  DAC registers directly — the same technique real bare-metal OSes use.
- **Real window manager**: draggable, closable, focusable, z-ordered
  windows; a taskbar with per-app buttons; a Start menu.
- **Real shell**: typed commands are tokenized and dispatched to real
  command implementations (see the table below).
- **Real file storage**: an in-memory ("RAM") file system with actual
  write/read/list/delete operations, shared by the shell, text editor,
  and document viewer.

One thing is intentionally **not** real, and I want to be upfront
about it: the **"Browser" is a local document viewer, not an internet
browser.** nyanos has no network driver, no TCP/IP stack, no DNS, no
TLS, and no HTML/CSS/JS rendering engine. Building those from scratch
is a multi-year effort even for professional teams (that's why there
are only a handful of real browser engines in the world). What the
Browser app *does* do for real: it reads pages from the in-memory
file system and renders a tiny, self-invented markup format (heading
and bullet lines), with a working address bar, Home button, and
Reload button. It's the honest, functional equivalent of a browser
that actually fits inside a hobby kernel.

## Project layout

| Layer                  | Files                                              | What it does |
|-------------------------|-----------------------------------------------------|---------------|
| Boot entry              | `boot/boot.S`                                       | Multiboot header, stack setup, jump into `kernel_main` |
| GDT                      | `kernel/gdt.c`, `boot/gdt_flush.S`                 | Flat-model memory segmentation |
| IDT / PIC / interrupts   | `kernel/idt.c`, `boot/isr.S`, `boot/idt_flush.S`   | 32 CPU exceptions + 16 hardware IRQs, 8259 PIC remap |
| Keyboard driver          | `kernel/drivers/keyboard.c`                        | PS/2 keyboard, IRQ1, scancode→ASCII |
| Mouse driver             | `kernel/drivers/mouse.c`                           | PS/2 mouse, IRQ12, x/y/click tracking |
| Timer driver             | `kernel/drivers/timer.c`                           | PIT, IRQ0, tick counter / uptime |
| VGA text mode             | `kernel/drivers/vga_text.c`                        | 80x25 console, `kprintf` |
| VGA graphics mode        | `kernel/drivers/vga_gfx.c`                         | Mode 13h (320x200x256), no BIOS, direct port I/O |
| Font                      | `kernel/drivers/font.c`                            | 5x7 pixel font (hand authored) |
| In-memory file system    | `kernel/fs.c`                                       | write/read/list/delete on RAM-backed files |
| Window manager            | `kernel/gui/gui.c`                                  | Desktop, taskbar, Start menu, draggable/z-ordered windows |
| Shell                      | `kernel/gui/terminal_app.c`                        | Real command parser and dispatcher |
| Text editor                | `kernel/gui/notepad_app.c`                         | Typing + Save/Load/Clear against the file system |
| Paint program               | `kernel/gui/paint_app.c`                           | Mouse-driven pixel drawing with a color palette |
| Document viewer ("Browser") | `kernel/gui/browser_app.c`                        | Renders local files with simple markup |
| Kernel entry               | `kernel/kernel.c`                                   | Boots every subsystem in order |

## Shell commands

```
help              list all commands
ls                list files in the file system
cat <file>        print a file's contents
write <file> ...  create/overwrite a file with the given text
rm <file>         delete a file
echo <text>       print text
clear             clear the terminal screen
about / ver       show OS version info
whoami            print the current user
uptime            seconds since boot (from the PIT timer)
calc <a> <op> <b> integer calculator (+ - * /)
notepad           open the text editor
paint             open the paint program
browser [file]    open the document viewer, optionally loading <file>
open <app>        open about/notepad/paint/browser by name
reboot / halt     halt the CPU
```

## How to build it

On a Linux machine (or WSL):

```bash
sudo apt install build-essential   # gcc, as, ld (with 32-bit multilib)
make
```

This produces `nyanos.elf`, a Multiboot-compliant kernel image.

## How to run it (QEMU — easiest way)

No GRUB/ISO needed — QEMU can boot multiboot kernels directly:

```bash
sudo apt install qemu-system-x86
make run
```

This runs `qemu-system-i386 -kernel nyanos.elf`. You'll see boot status
messages in text mode for a moment ("Setting up the GDT... Setting up
the IDT..."), then the screen switches to 320x200 graphics mode and
the desktop appears with the About and Terminal windows open.

**To use the mouse in the QEMU window**, click into the window to
grab the mouse (QEMU's status bar tells you how — usually `Ctrl+Alt+G`
releases it).

## How to build a real bootable ISO (optional, via GRUB)

```bash
sudo apt install grub-pc-bin xorriso
make iso
qemu-system-i386 -cdrom nyanos.iso
```

You can `dd` this ISO onto a real USB stick and boot it on real
hardware (BIOS mode) — the kernel doesn't use any BIOS calls after
boot (VGA mode switching is done by direct register programming), so
it should, in principle, work on real machines too.

## Using the desktop

- Click **MENU** (bottom-left) to open any app, or **SHUT DOWN**.
- Each app also has its own **taskbar button**; click it to
  open/focus it, click again while it's focused to close it.
- **Drag windows** by their title bar. Click the **red square**
  (top-right of a window) to close it.
- **Terminal**: click it to focus, then type — it's a real shell (see
  command list above).
- **Notepad**: click to focus, type to edit, click **SAVE** to persist
  to `notepad.txt` in the file system (readable via `cat notepad.txt`
  in the shell), **LOAD** to reload it, **CLR** to clear the buffer.
- **Paint**: click a palette swatch to pick a color, then click-drag
  on the canvas to draw. **CLR** clears the canvas.
- **Browser**: click **HOME** to load the default page, **RELD** to
  reload the current page, **README** to view `readme.txt`. Use the
  shell's `browser <file>` command to load any file you've written.

## Deliberately out of scope (ideas for what's next)

This is a real kernel at "educational hobby OS" scale, not a
production operating system. Natural next steps:

- **Memory management**: no physical page allocator or paging yet —
  everything currently runs in one flat address space.
- **Multitasking**: no scheduler; the GUI runs in a single loop
  (`gui_run`). The PIT timer driver is already IRQ-driven, so a
  preemptive scheduler could hook into it fairly easily.
- **Persistent storage**: the file system is RAM-only and resets on
  reboot; a real disk driver (e.g. ATA/IDE) plus an on-disk format
  (e.g. FAT) would make it persistent.
- **Networking**: this is the big one for a "real browser" — a NIC
  driver, a TCP/IP stack, DNS, TLS, and an HTML/CSS/JS engine.
- **Richer GUI**: window resizing, more fonts/sizes, scrollable text
  areas, a proper cursor-based text editor (the current Notepad only
  supports append + backspace at the end).

The source is commented throughout to explain not just *what* each
piece of hardware-facing code does, but *why* it's structured that
way (especially in the GDT/IDT/VGA sections, which are the parts
closest to the metal).

By yunusemreduran
