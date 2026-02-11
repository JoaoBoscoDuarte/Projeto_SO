# Estrutura do Projeto

## Visão Geral

Este documento descreve a organização e estrutura do projeto do Sistema Operacional, explicando a função de cada arquivo e diretório.

## Árvore de Diretórios

```
projeto_so/
├── docs/                    # Documentação do projeto
├── iso/                     # Estrutura para criação da imagem ISO bootável
│   └── boot/
│       ├── grub/           # Configuração do bootloader GRUB
│       │   ├── menu.lst    # Menu de boot do GRUB
│       │   └── stage2_eltorito  # Stage 2 do GRUB para boot em CD/ISO
│       └── kernel.elf      # Kernel compilado (copiado durante build)
├── loader.s                # Código assembly do bootloader
├── kmain.c                 # Código C principal do kernel
├── link.ld                 # Script de linker
├── Makefile               # Automação de build
├── bochsrc.txt            # Configuração do emulador Bochs
└── README.md              # Documentação principal
```

## Descrição dos Arquivos

### Arquivos de Código Fonte

#### `loader.s`

- **Linguagem**: Assembly x86 (NASM)
- **Função**: Ponto de entrada do sistema operacional
- **Responsabilidades**:
  - Implementar o cabeçalho Multiboot
  - Configurar a pilha do kernel
  - Transferir controle para o código C

#### `kmain.c`

- **Linguagem**: C
- **Função**: Código principal do kernel
- **Responsabilidades**:
  - Implementar a lógica principal do SO
  - Gerenciar recursos do sistema
  - Atualmente vazio (será expandido)

### Arquivos de Configuração

#### `link.ld`

- **Tipo**: Linker Script
- **Função**: Define como o kernel é organizado na memória
- **Especifica**:
  - Endereço de carregamento (1 MB)
  - Ordem das seções (.text, .rodata, .data, .bss)
  - Alinhamento de memória

#### `Makefile`

- **Tipo**: Script de build
- **Função**: Automatiza compilação e execução
- **Comandos principais**:
  - `make all`: Compila o kernel
  - `make os.iso`: Cria imagem ISO bootável
  - `make run`: Compila e executa no Bochs
  - `make clean`: Remove arquivos gerados

#### `bochsrc.txt`

- **Tipo**: Configuração do emulador
- **Função**: Define parâmetros do Bochs
- **Configura**:
  - Memória RAM
  - Dispositivos de boot
  - Periféricos emulados

### Diretório ISO

O diretório `iso/` contém a estrutura necessária para criar uma imagem ISO bootável:

- **boot/grub/menu.lst**: Configuração do menu do GRUB
- **boot/grub/stage2_eltorito**: Bootloader GRUB para CD-ROM
- **boot/kernel.elf**: Kernel compilado (copiado durante o build)

## Arquivos Gerados

Durante o processo de compilação, os seguintes arquivos são gerados:

- **`*.o`**: Arquivos objeto intermediários
- **`kernel.elf`**: Executável do kernel no formato ELF
- **`os.iso`**: Imagem ISO bootável final

Estes arquivos podem ser removidos com `make clean`.

## Fluxo de Build

1. **Montagem**: `loader.s` → `loader.o` (NASM)
2. **Compilação**: `kmain.c` → `kmain.o` (GCC)
3. **Linkagem**: `loader.o` + `kmain.o` → `kernel.elf` (LD)
4. **Empacotamento**: `kernel.elf` + estrutura ISO → `os.iso` (genisoimage)
5. **Execução**: `os.iso` → Bochs
