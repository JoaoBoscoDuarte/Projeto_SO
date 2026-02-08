# Projeto_SO

## Dependências

- nasm
- ld (binutils)
- grub legacy (stage2_eltorito)
- genisoimage
- bochs

## Compilação

### 1. Compilar o loader

```bash
nasm -f elf32 loader.s -o loader.o
```

### 2. Linkar o kernel

```bash
ld -T link.ld -melf_i386 loader.o -o kernel.elf
```

### 3.Criar estrutura da ISO

```bash
cp kernel.elf iso/boot/
```

### 4. Gerar ISO

```bash
genisoimage -R \
  -b boot/grub/stage2_eltorito \
  -no-emul-boot \
  -boot-load-size 4 \
  -A os \
  -input-charset utf8 \
  -quiet \
  -boot-info-table \
  -o os.iso \
  iso
```

### 5. Executar no Bochs

```bash
bochs -f bochsrc.txt -q
```
