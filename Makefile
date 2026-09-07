CC      = gcc
LD      = ld
CFLAGS  = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -Iinclude
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL  = seaweed.bin
ISO     = yunix.iso

ASRC    = $(wildcard boot/*.s)
CSRC    = $(wildcard kernel/*.c drivers/*.c lib/*.c fs/*.c)
OBJS    = $(patsubst %.s,build/%.o,$(ASRC)) $(patsubst %.c,build/%.o,$(CSRC))

all: $(KERNEL)

build/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) -m32 -c $< -o $@

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)
	@echo "Built $(KERNEL)"

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -display curses

run-gui: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

$(ISO): $(KERNEL) grub.cfg
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/$(KERNEL)
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso 2>/dev/null
	@echo "Built $(ISO)"

iso: $(ISO)

clean:
	rm -rf build $(KERNEL) $(ISO) iso

.PHONY: all run run-gui iso clean
