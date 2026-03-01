# Como Compilar e Executar

## 1. Instalar Dependências

Execute o script de instalação:

```bash
./install_deps.sh
```

Ou instale manualmente:

```bash
sudo apt install grub-legacy genisoimage bochs bochs-x nasm binutils gcc make
sudo cp /usr/lib/grub/i386-pc/stage2_eltorito iso/boot/grub/
```

## 2. Compilar e Executar

```bash
make run
```

## 3. Limpar Arquivos Compilados

```bash
make clean
```

## Estrutura do Projeto

- `loader.s` - Bootloader (assembly)
- `kmain.c` - Função principal do kernel
- `idt.c/h` - Interrupt Descriptor Table
- `isr.c/h` - Interrupt Service Routines
- `screen.c/h` - Driver de vídeo VGA
- `types.h` - Definições de tipos
- `link.ld` - Linker script
- `Makefile` - Sistema de build
