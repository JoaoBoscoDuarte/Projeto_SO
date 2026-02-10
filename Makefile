# ==============================================================================
# MAKEFILE - Sistema de Build do Sistema Operacional
# ==============================================================================
# Este Makefile automatiza o processo de compilação, linkagem e criação da
# imagem ISO bootável do sistema operacional.
# ==============================================================================

# ------------------------------------------------------------------------------
# Variáveis de Configuração
# ------------------------------------------------------------------------------

# Arquivos objeto que serão linkados para formar o kernel
OBJECTS = loader.o kmain.o

# Compilador C
CC = gcc

# Flags do compilador C:
# -m32: compila para arquitetura 32 bits
# -nostdlib: não usa biblioteca padrão do C
# -nostdinc: não usa headers padrão do C
# -fno-builtin: desabilita funções built-in do compilador
# -fno-stack-protector: desabilita proteção de pilha
# -nostartfiles: não usa arquivos de inicialização padrão
# -nodefaultlibs: não usa bibliotecas padrão
# -Wall -Wextra -Werror: habilita todos os warnings e os trata como erros
# -c: compila sem linkar
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
			-nostartfiles -nodefaultlibs -Wall -Wextra -Werror -c

# Flags do linker:
# -T link.ld: usa o script de linker customizado
# -melf_i386: gera binário no formato ELF 32 bits para i386
LDFLAGS = -T link.ld -melf_i386

# Assembler NASM
AS = nasm

# Flags do assembler:
# -f elf: gera arquivo objeto no formato ELF
ASFLAGS = -f elf

# ------------------------------------------------------------------------------
# Regras de Build
# ------------------------------------------------------------------------------

# Regra padrão: compila o kernel
all: kernel.elf

# Cria o executável do kernel linkando todos os arquivos objeto
kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

# Cria a imagem ISO bootável
os.iso: kernel.elf
	cp kernel.elf iso/boot/kernel.elf
	# genisoimage cria a imagem ISO com:
	# -R: usa extensões Rock Ridge (nomes longos)
	# -b: especifica o arquivo de boot (stage2_eltorito do GRUB)
	# -no-emul-boot: modo de boot sem emulação
	# -boot-load-size 4: carrega 4 setores de 512 bytes
	# -A os: nome da aplicação
	# -input-charset utf8: usa UTF-8 para nomes de arquivo
	# -quiet: modo silencioso
	# -boot-info-table: cria tabela de informações de boot
	# -o os.iso: arquivo de saída
	# iso: diretório fonte
	genisoimage -R                              \
				-b boot/grub/stage2_eltorito    \
				-no-emul-boot                   \
				-boot-load-size 4               \
				-A os                           \
				-input-charset utf8             \
				-quiet                          \
				-boot-info-table                \
				-o os.iso                       \
				iso

# Compila e executa o SO no emulador Bochs
run: os.iso
	bochs -f bochsrc.txt -q

# Regra genérica: compila arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS)  $< -o $@

# Regra genérica: monta arquivos .s (assembly) em .o
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# Remove todos os arquivos gerados
clean:
	rm -rf *.o kernel.elf os.iso