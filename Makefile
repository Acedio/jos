OBJECTS = loader.o kmain.o io.o fb.o serial.o log.o string.o segmentation.o gdt.o interrupts.o interrupts_asm.o pic8259.o keyboard.o paging.o paging_asm.o
CC = i686-elf-gcc
CFLAGS = -c -std=gnu99 -ffreestanding -Wall -Wextra -Werror
LDFLAGS = -melf_i386
AS = nasm
ASFLAGS = -f elf32

all: kernel.elf program.flat

kernel.elf: $(OBJECTS) link.ld
		ld -T link.ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

jos.iso: kernel.elf program.flat
		cp kernel.elf iso/boot/kernel.elf
		cp program.flat iso/modules/program.flat
		genisoimage -R                              \
								-b boot/grub/stage2_eltorito    \
								-no-emul-boot                   \
								-boot-load-size 4               \
								-A jos                          \
								-input-charset utf8             \
								-quiet                          \
								-boot-info-table                \
								-o jos.iso                      \
								iso

run: jos.iso
		bochs -f bochsrc.txt -q

%.flat: %.s
	  $(AS) -f bin $< -o $@

%.o: %.c
		$(CC) $(CFLAGS) $< -o $@

%.o: %.s
		$(AS) $(ASFLAGS) $< -o $@

clean:
		rm -rf *.o kernel.elf jos.iso
