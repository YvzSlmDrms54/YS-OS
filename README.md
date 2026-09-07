# YS-OS

_A hobby operating system for x86, built on the Seaweed kernel._

Early development: multiboot header, kernel entry, VGA text-mode driver.

## Requirements

    sudo apt install build-essential gcc-multilib qemu-system-x86 grub-pc-bin xorriso

## Building

    make            # builds ys-os.bin
    make run        # boots it in QEMU (fastest)
    make iso        # builds ys-os.iso for VirtualBox / USB
    make run-iso    # boots the ISO in QEMU
    make clean

## Files

| File        | Purpose                                    |
| ----------- | ------------------------------------------ |
| `boot.s`    | Multiboot header, stack setup, entry point |
| `linker.ld` | Memory layout: kernel loads at 1 MiB       |
| `kernel.c`  | VGA text driver and `kernel_main()`        |
| `grub.cfg`  | GRUB menu entry for the ISO                |

## Roadmap

- [x] Boot with GRUB, print to screen
- [x] GDT (Global Descriptor Table)
- [x] IDT and interrupt handling
- [x] PS/2 keyboard driver
- [ ] `kmalloc` / physical memory manager
- [x] Shell
