# ==============================================================================
# MAKEFILE - Projeto_SO (Kernel ELF + ISO GRUB2 + QEMU)
# ==============================================================================

# -----------------------------
# Toolchain
# -----------------------------
CC      = gcc
AS      = nasm
LD      = ld

CFLAGS  = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
          -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c

ASFLAGS = -f elf
LDFLAGS = -T link.ld -melf_i386

QEMU    = qemu-system-i386

# -----------------------------
# Artifacts
# -----------------------------
ISO     = os.iso
HDD_IMG = hdd.img
FD_IMG  = floppy.img

# Objetos do kernel
OBJECTS = loader.o kmain.o idt.o isr.o idt_load.o isr_stubs.o screen.o

# -----------------------------
# Default
# -----------------------------
all: $(ISO)

# -----------------------------
# Build kernel
# -----------------------------
kernel.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# -----------------------------
# GRUB2 ISO layout
# -----------------------------
iso/boot/kernel.elf: kernel.elf
	mkdir -p iso/boot
	cp kernel.elf iso/boot/kernel.elf

iso/boot/grub/grub.cfg:
	mkdir -p iso/boot/grub
	printf '%s\n' \
		'set timeout_style=menu' \
		'set timeout=5' \
		'set default=0' \
		'' \
		'menuentry "Meu Kernel" {' \
		'  multiboot /boot/kernel.elf' \
		'  boot' \
		'}' \
		> iso/boot/grub/grub.cfg

$(ISO): iso/boot/kernel.elf iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso

# -----------------------------
# Run (QEMU)
# -----------------------------
run: run-cd

run-cd: $(ISO)
	$(QEMU) -cdrom $(ISO) -boot order=d -no-reboot -no-shutdown

# HD/Floppy: existem pra testar ordem, mas imagens vazias não bootam (normal).
$(HDD_IMG):
	qemu-img create -f raw $(HDD_IMG) 50M

$(FD_IMG):
	dd if=/dev/zero of=$(FD_IMG) bs=512 count=2880

run-hd: $(ISO) $(HDD_IMG)
	$(QEMU) -hda $(HDD_IMG) -cdrom $(ISO) -boot order=c -no-reboot -no-shutdown

run-fd: $(ISO) $(FD_IMG)
	$(QEMU) -fda $(FD_IMG) -cdrom $(ISO) -boot order=a -no-reboot -no-shutdown

# -----------------------------
# Clean
# -----------------------------
clean:
	rm -rf *.o kernel.elf $(ISO) $(HDD_IMG) $(FD_IMG)
	rm -rf iso/boot/kernel.elf iso/boot/grub/grub.cfg