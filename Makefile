# Diretórios
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso

# Compiladores e Ferramentas
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c \
         -I$(SRC_DIR)/include
LD = ld
LDFLAGS = -T link.ld -melf_i386
AS = nasm
ASFLAGS = -f elf

# CAMINHO DO GRUB (Atualizado conforme seu retorno)
GENISOIMAGE = genisoimage

# Arquivos objeto
OBJECTS = $(BUILD_DIR)/loader.o \
          $(BUILD_DIR)/kmain.o \
          $(BUILD_DIR)/io.o \
          $(BUILD_DIR)/fb.o \
          $(BUILD_DIR)/serial.o \
          $(BUILD_DIR)/printf.o \
          $(BUILD_DIR)/gdt.o \
          $(BUILD_DIR)/gdt_s.o \
          $(BUILD_DIR)/pic.o \
          $(BUILD_DIR)/idt.o \
          $(BUILD_DIR)/interrupts.o \
          $(BUILD_DIR)/keyboard.o \
          $(BUILD_DIR)/pfa.o

all: kernel.elf

# Módulo carregado pelo GRUB (binário plano para Multiboot)
MODULE_PROGRAM = $(BUILD_DIR)/program
kernel.elf: $(BUILD_DIR) $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o kernel.elf

os.iso: kernel.elf $(MODULE_PROGRAM)
	@mkdir -p $(ISO_DIR)/boot/grub
	@mkdir -p $(ISO_DIR)/modules
	cp kernel.elf $(ISO_DIR)/boot/kernel.elf
	cp $(MODULE_PROGRAM) $(ISO_DIR)/modules/program
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "os" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.elf' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '  module /modules/program' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o os.iso $(ISO_DIR)


run: os.iso
	bochs -f bochsrc.txt -q

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# --- REGRAS DE COMPILAÇÃO ---

$(BUILD_DIR)/loader.o: $(SRC_DIR)/boot/loader.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kmain.o: $(SRC_DIR)/kernel/kmain.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/io.o: $(SRC_DIR)/drivers/io.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/gdt.o: $(SRC_DIR)/drivers/gdt.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/gdt_s.o: $(SRC_DIR)/drivers/gdt.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/fb.o: $(SRC_DIR)/drivers/fb.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/serial.o: $(SRC_DIR)/drivers/serial.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/printf.o: $(SRC_DIR)/lib/printf.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/pic.o: $(SRC_DIR)/drivers/pic.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/idt.o: $(SRC_DIR)/drivers/idt.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupts.o: $(SRC_DIR)/drivers/interrupts.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: $(SRC_DIR)/drivers/keyboard.c
	$(CC) $(CFLAGS) $< -o $@

# Módulo program: binário plano para o GRUB carregar como Multiboot module
$(MODULE_PROGRAM): program.s | $(BUILD_DIR)
	$(AS) -f bin $< -o $@

$(BUILD_DIR)/pfa.o: $(SRC_DIR)/drivers/pfa.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR) kernel.elf os.iso

.PHONY: all run clean