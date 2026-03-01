# Diretórios
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso

# Compiladores
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c \
         -I$(SRC_DIR)/include
LDFLAGS = -T link.ld -melf_i386
AS = nasm
ASFLAGS = -f elf

# Arquivos objeto (ADICIONADOS: gdt.o e gdt_s.o)
OBJECTS = $(BUILD_DIR)/loader.o $(BUILD_DIR)/kmain.o \
          $(BUILD_DIR)/io.o $(BUILD_DIR)/fb.o \
          $(BUILD_DIR)/serial.o $(BUILD_DIR)/printf.o \
          $(BUILD_DIR)/gdt.o $(BUILD_DIR)/gdt_s.o

all: kernel.elf

kernel.elf: $(BUILD_DIR) $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

os.iso: kernel.elf
	cp kernel.elf $(ISO_DIR)/boot/kernel.elf
	genisoimage -R -b boot/grub/stage2_eltorito -no-emul-boot \
                -boot-load-size 4 -A os -input-charset utf8 -quiet \
                -boot-info-table -o os.iso $(ISO_DIR)

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

# Regra para o código C da GDT
$(BUILD_DIR)/gdt.o: $(SRC_DIR)/drivers/gdt.c
	$(CC) $(CFLAGS) $< -o $@

# Regra para o código Assembly da GDT (gdt_flush)
$(BUILD_DIR)/gdt_s.o: $(SRC_DIR)/drivers/gdt.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/fb.o: $(SRC_DIR)/drivers/fb.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/serial.o: $(SRC_DIR)/drivers/serial.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/printf.o: $(SRC_DIR)/lib/printf.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR) kernel.elf os.iso

.PHONY: all run clean