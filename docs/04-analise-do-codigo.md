# Análise Detalhada do Código

## Introdução

Este documento fornece uma análise linha por linha dos arquivos de código do projeto, explicando cada instrução e sua função no contexto do sistema operacional.

## 1. loader.s - Bootloader Assembly

### 1.1. Cabeçalho e Exportação

```assembly
global loader
```

**Explicação**: 
- `global` torna o símbolo `loader` visível para o linker
- Permite que o linker script defina `loader` como ponto de entrada
- Sem isso, o linker não saberia onde começar a execução

### 1.2. Constantes Multiboot

```assembly
MAGIC_NUMBER equ 0x1BADB002
FLAGS        equ 0x0
CHECKSUM     equ -MAGIC_NUMBER
```

**Explicação**:
- `equ` define constantes em tempo de montagem
- **0x1BADB002**: Número mágico definido pela especificação Multiboot
- **FLAGS = 0x0**: Sem requisitos especiais (sem alinhamento de módulos, sem informações de memória)
- **CHECKSUM**: Calculado para que `MAGIC + FLAGS + CHECKSUM = 0`

**Por que isso é necessário?**
- O GRUB procura por este padrão nos primeiros 8 KB do kernel
- Se encontrado, o GRUB sabe que pode carregar este kernel
- É um "handshake" entre bootloader e kernel

### 1.3. Tamanho da Pilha

```assembly
KERNEL_STACK_SIZE equ 4096
```

**Explicação**:
- Define pilha de 4 KB (4096 bytes)
- Suficiente para chamadas de função iniciais
- Pode ser expandida posteriormente

**Por que 4 KB?**
- Tamanho de uma página de memória
- Facilita gerenciamento futuro
- Suficiente para boot inicial

### 1.4. Seção BSS

```assembly
section .bss
align 4
kernel_stack:
    resb KERNEL_STACK_SIZE
```

**Explicação**:
- `.bss`: Seção para dados não inicializados
- `align 4`: Alinha em 4 bytes (requisito x86)
- `kernel_stack`: Label que marca o início
- `resb`: Reserva bytes (não inicializa)

**Por que BSS?**
- Não ocupa espaço no arquivo ELF
- Automaticamente zerada pelo loader
- Economiza espaço em disco

### 1.5. Cabeçalho Multiboot na Seção Text

```assembly
section .text:
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM
```

**Explicação**:
- `.text`: Seção de código executável
- `align 4`: Alinhamento obrigatório do Multiboot
- `dd`: Define double word (32 bits)
- Estes 12 bytes formam o cabeçalho Multiboot

**Ordem importa?**
- Sim! Deve estar nos primeiros 8 KB
- Deve estar nesta ordem exata
- GRUB procura este padrão específico

### 1.6. Função loader

```assembly
loader:
    mov eax, 0xCAFEBABE
```

**Explicação**:
- `loader`: Label do ponto de entrada
- `mov`: Move valor para registrador
- `eax`: Registrador de propósito geral
- `0xCAFEBABE`: Valor hexadecimal reconhecível

**Por que este valor?**
- Fácil de identificar em dumps de memória
- Padrão comum em desenvolvimento de SO
- Útil para verificar se o código foi executado

### 1.7. Configuração da Pilha

```assembly
mov esp, kernel_stack + KERNEL_STACK_SIZE
```

**Explicação**:
- `esp`: Stack Pointer (registrador que aponta para o topo da pilha)
- `kernel_stack`: Endereço base da pilha
- `+ KERNEL_STACK_SIZE`: Soma 4096 bytes

**Por que somar?**
- Pilha cresce para baixo (de endereços altos para baixos)
- ESP deve apontar para o topo (endereço mais alto)
- À medida que empilhamos, ESP diminui

**Visualização**:
```
kernel_stack + 4096 → [ESP] ← Topo (vazio)
                      [   ]
                      [   ]
kernel_stack        → [   ] ← Base
```

### 1.8. Loop Infinito

```assembly
.loop:
    jmp .loop
```

**Explicação**:
- `.loop`: Label local (ponto começa com .)
- `jmp`: Salta incondicionalmente
- Cria loop infinito

**Por que loop infinito?**
- Evita que o processador execute código inválido
- Mantém o kernel "vivo"
- Será substituído por chamada ao código C

## 2. link.ld - Linker Script

### 2.1. Ponto de Entrada

```ld
ENTRY(loader)
```

**Explicação**:
- Define `loader` como primeira função a executar
- O ELF header armazena este endereço
- GRUB salta para este endereço após carregar

### 2.2. Endereço Base

```ld
. = 0x00100000;
```

**Explicação**:
- `.`: Contador de localização (location counter)
- `0x00100000`: 1 MB em hexadecimal
- Todo o kernel será carregado a partir deste endereço

**Por que 1 MB?**
- Primeiros 640 KB: Memória convencional (pode ter conflitos)
- 640 KB - 1 MB: Área reservada (BIOS, vídeo)
- 1 MB+: Seguro para o kernel

### 2.3. Seção .text

```ld
.text ALIGN (0x1000) : {
    *(.text)
}
```

**Explicação**:
- `.text`: Nome da seção de saída
- `ALIGN (0x1000)`: Alinha em 4 KB (4096 bytes)
- `*(.text)`: Inclui todas as seções .text de todos os arquivos objeto

**Fluxo**:
1. Linker coleta todas as seções .text
2. Alinha o início em 4 KB
3. Coloca tudo na seção .text do executável

### 2.4. Seção .rodata

```ld
.rodata ALIGN (0x1000) : {
    *(.rodata*)
}
```

**Explicação**:
- `.rodata`: Read-only data
- `*(.rodata*)`: Inclui .rodata, .rodata.str, etc.
- Asterisco é wildcard

**Conteúdo típico**:
- Strings literais: `"Hello, World!"`
- Constantes: `const int MAX = 100;`

### 2.5. Seção .data

```ld
.data ALIGN (0x1000) : {
    *(.data)
}
```

**Explicação**:
- `.data`: Dados inicializados
- Valores são copiados do arquivo ELF para memória

**Exemplo**:
```c
int counter = 42;  // Vai para .data
```

### 2.6. Seção .bss

```ld
.bss ALIGN (0x1000) : {
    *(COMMON)
    *(.bss)
}
```

**Explicação**:
- `.bss`: Block Started by Symbol
- `COMMON`: Símbolos comuns (variáveis não inicializadas em C)
- Automaticamente zerada

**Exemplo**:
```c
int counter;  // Vai para .bss
```

## 3. Makefile - Sistema de Build

### 3.1. Variáveis

```makefile
OBJECTS = loader.o kmain.o
```

**Explicação**:
- Lista de arquivos objeto necessários
- Usada como dependência para o kernel

### 3.2. Flags do GCC

```makefile
-m32
```
- Compila para arquitetura 32 bits
- Necessário mesmo em sistemas 64 bits

```makefile
-nostdlib -nostdinc
```
- Não usa biblioteca padrão do C
- Não usa headers padrão
- Compilação freestanding

```makefile
-fno-builtin
```
- Desabilita funções built-in (memcpy, strlen, etc.)
- Evita dependências ocultas

```makefile
-fno-stack-protector
```
- Desabilita proteção de pilha
- Proteção requer suporte do SO (que não existe ainda)

```makefile
-nostartfiles -nodefaultlibs
```
- Não usa arquivos de inicialização (crt0.o)
- Não linka bibliotecas padrão

```makefile
-Wall -Wextra -Werror
```
- Habilita todos os warnings
- Warnings extras
- Trata warnings como erros

### 3.3. Regra de Linkagem

```makefile
kernel.elf: $(OBJECTS)
    ld $(LDFLAGS) $(OBJECTS) -o kernel.elf
```

**Explicação**:
- `kernel.elf` depende de `loader.o` e `kmain.o`
- Se qualquer .o mudar, kernel.elf é reconstruído
- `ld` linka os objetos usando script `link.ld`

### 3.4. Criação da ISO

```makefile
genisoimage -R \
    -b boot/grub/stage2_eltorito \
    -no-emul-boot \
    -boot-load-size 4 \
    -boot-info-table \
    -o os.iso \
    iso
```

**Flags explicadas**:
- `-R`: Rock Ridge extensions (nomes longos)
- `-b`: Arquivo de boot (GRUB stage 2)
- `-no-emul-boot`: Não emula disquete
- `-boot-load-size 4`: Carrega 4 setores (2 KB)
- `-boot-info-table`: Tabela de informações para GRUB
- `-o os.iso`: Arquivo de saída
- `iso`: Diretório fonte

### 3.5. Regras Pattern

```makefile
%.o: %.c
    $(CC) $(CFLAGS) $< -o $@
```

**Explicação**:
- `%`: Wildcard (qualquer nome)
- `$<`: Primeiro prerequisito (arquivo .c)
- `$@`: Target (arquivo .o)
- Compila qualquer .c em .o

```makefile
%.o: %.s
    $(AS) $(ASFLAGS) $< -o $@
```

**Explicação**:
- Monta qualquer .s em .o
- Usa NASM como assembler

## 4. Fluxo de Execução Completo

### Passo a Passo

1. **Compilação**:
   ```
   nasm -f elf loader.s -o loader.o
   gcc -m32 ... -c kmain.c -o kmain.o
   ```

2. **Linkagem**:
   ```
   ld -T link.ld loader.o kmain.o -o kernel.elf
   ```
   - Linker lê link.ld
   - Coloca loader.o primeiro (contém entry point)
   - Organiza seções conforme script
   - Gera ELF em 0x00100000

3. **Criação da ISO**:
   ```
   cp kernel.elf iso/boot/
   genisoimage ... -o os.iso iso
   ```

4. **Boot**:
   - Bochs carrega os.iso
   - BIOS encontra GRUB
   - GRUB lê menu.lst
   - GRUB carrega kernel.elf em 0x00100000
   - GRUB salta para `loader`

5. **Execução**:
   - `mov eax, 0xCAFEBABE`
   - `mov esp, kernel_stack + 4096`
   - Loop infinito

## Próximos Passos

Para expandir o kernel:

1. **Adicionar código C**:
   ```assembly
   extern kmain
   call kmain
   ```

2. **Implementar I/O**:
   - Driver de vídeo (VGA)
   - Driver de teclado

3. **Configurar GDT**:
   - Segmentação de memória

4. **Habilitar interrupções**:
   - IDT (Interrupt Descriptor Table)
   - ISRs (Interrupt Service Routines)
