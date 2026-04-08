# Diretórios
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso
LOG_DIR = logs

# Compiladores e Ferramentas
CC = gcc
# CORREÇÃO 1: adicionado -fno-pie -fno-pic
#   GCC moderno ativa PIE por padrão, gerando referências via GOT que são
#   incompatíveis com kernel bare-metal (sem dynamic linker).
#   Sintoma sem esses flags: kernel compila mas acessa endereços errados.
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -fno-pie -fno-pic \
         -Wall -Wextra -Werror -c \
         -I$(SRC_DIR)/include
LD = ld
LDFLAGS = -T link.ld -melf_i386 -z noexecstack
AS = nasm
# CORREÇÃO 2: trocado -f elf por -f elf32
#   -f elf é ambíguo em alguns ambientes; -f elf32 é explícito e correto
#   para objetos de 32 bits.
# -Wl,-z,noexecstack suprime o warning de stack executável gerado por
#   arquivos .s que não declaram .note.GNU-stack (ex.: interrupts.s)
ASFLAGS = -f elf32

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
          $(BUILD_DIR)/tss.o \
          $(BUILD_DIR)/tss_s.o \
          $(BUILD_DIR)/pic.o \
          $(BUILD_DIR)/idt.o \
          $(BUILD_DIR)/interrupts.o \
          $(BUILD_DIR)/keyboard.o \
          $(BUILD_DIR)/pfa.o \
          $(BUILD_DIR)/paging.o \
		  $(BUILD_DIR)/kheap.o \
          $(BUILD_DIR)/process.o \
          $(BUILD_DIR)/usermode.o \
          $(BUILD_DIR)/scheduler.o \
          $(BUILD_DIR)/switch_s.o \
          $(BUILD_DIR)/string.o \
          $(BUILD_DIR)/cpuid.o \
          $(BUILD_DIR)/pit.o \
          $(BUILD_DIR)/shell.o \
          $(BUILD_DIR)/top.o

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
	@mkdir -p $(LOG_DIR)
	bochs -f bochsrc.txt -q

# =============================================================================
# Targets para macOS Apple Silicon (Docker)
#
# Uso:
#   make docker-build   → constrói a imagem uma vez
#   make docker-iso     → compila o kernel e gera os.iso dentro do container
#   make docker-run     → compila + roda com QEMU headless (serial no terminal)
#   make docker-shell   → abre shell interativo para debug manual
# =============================================================================
DOCKER_IMAGE = osdev-kernel

docker-build:
	docker build --platform linux/amd64 -t $(DOCKER_IMAGE) .

docker-iso: docker-build
	docker run --rm \
		--platform linux/amd64 \
		-v "$(PWD)":/os \
		-w /os \
		$(DOCKER_IMAGE) \
		make clean os.iso

docker-run: docker-build
	docker run --rm -it \
		--platform linux/amd64 \
		-v "$(PWD)":/os \
		-w /os \
		$(DOCKER_IMAGE) \
		bash -c "make clean os.iso && \
		         qemu-system-i386 \
		           -cdrom os.iso \
		           -display none \
		           -serial stdio \
		           -no-reboot \
		           -d int,cpu_reset \
		           2>&1 | tee logs/qemu.log"

docker-shell: docker-build
	docker run --rm -it \
		--platform linux/amd64 \
		-v "$(PWD)":/os \
		-w /os \
		$(DOCKER_IMAGE) bash

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

$(BUILD_DIR)/tss.o: $(SRC_DIR)/drivers/tss.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tss_s.o: $(SRC_DIR)/drivers/tss.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/pfa.o: $(SRC_DIR)/drivers/pfa.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/paging.o: $(SRC_DIR)/drivers/paging.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/kheap.o: $(SRC_DIR)/kernel/kheap.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/process.o: $(SRC_DIR)/kernel/process.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/usermode.o: $(SRC_DIR)/boot/usermode.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/scheduler.o: $(SRC_DIR)/kernel/scheduler.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/switch_s.o: $(SRC_DIR)/kernel/switch.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/string.o: $(SRC_DIR)/lib/string.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/cpuid.o: $(SRC_DIR)/lib/cpuid.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/pit.o: $(SRC_DIR)/drivers/pit.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/shell.o: $(SRC_DIR)/kernel/shell.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/top.o: $(SRC_DIR)/kernel/top.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR) kernel.elf os.iso
	rm -f $(LOG_DIR)/bochslog.txt $(LOG_DIR)/com1.out \
	      $(LOG_DIR)/qemu.log $(LOG_DIR)/serial.log

.PHONY: all run clean docker-build docker-iso docker-run docker-shell