CC = i686-elf-gcc
AS = i686-elf-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I.
LDFLAGS = -T linker.ld -nostdlib -lgcc

OBJS = boot.o \
       cpu/gdt.o \
       cpu/gdt_flush.o \
       cpu/idt.o \
       cpu/isr.o \
       cpu/interrupt.o \
       cpu/ports.o \
       drivers/keyboard.o \
       drivers/timer.o \
       crypto/sha256.o \
       fs/initrd.o \
       fs/vfs.o \
       kernel/auth.o \
       kernel/kernel.o \
       kernel/shell.o \
       kernel/tui.o \
       kernel/util.o \
       kernel/vga.o \
       mm/memory.o \
       neuro/neuro.o \
       task/switch.o \
       task/task.o

all: neuro-os.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $< -o $@

neuro-os.bin: $(OBJS)
	$(CC) -T linker.ld -o neuro-os.bin -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

clean:
	rm -f $(OBJS) neuro-os.bin neuro-os.iso
