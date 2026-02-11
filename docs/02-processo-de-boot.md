# Processo de Boot do Sistema Operacional

## Visão Geral

Este documento explica detalhadamente como o sistema operacional é carregado e inicializado, desde o momento em que o computador é ligado até a execução do kernel.

## Etapas do Boot

### 1. Power-On e BIOS

Quando o computador é ligado:

1. **POST (Power-On Self Test)**: BIOS verifica o hardware
2. **Busca de Bootloader**: BIOS procura por dispositivos bootáveis
3. **Carregamento do GRUB**: BIOS carrega o primeiro estágio do GRUB do CD/ISO

### 2. GRUB (Grand Unified Bootloader)

O GRUB é responsável por:

- Ler o arquivo de configuração `menu.lst`
- Apresentar menu de boot (se configurado)
- Carregar o kernel na memória
- Transferir controle para o kernel

#### Configuração do GRUB (`menu.lst`)

```
default=0
timeout=0

title os
kernel /boot/kernel.elf
```

- **default=0**: Primeira opção é padrão
- **timeout=0**: Sem espera, boot imediato
- **kernel /boot/kernel.elf**: Localização do kernel

### 3. Multiboot

O **Multiboot** é uma especificação que padroniza a interface entre bootloaders e kernels.

#### Cabeçalho Multiboot

O kernel deve ter um cabeçalho especial nos primeiros 8 KB:

```assembly
MAGIC_NUMBER equ 0x1BADB002     ; Identifica kernel Multiboot
FLAGS        equ 0x0            ; Sem requisitos especiais
CHECKSUM     equ -MAGIC_NUMBER  ; Validação
```

**Requisito**: `MAGIC_NUMBER + FLAGS + CHECKSUM = 0`

#### Por que Multiboot?

- Permite que diferentes bootloaders (GRUB, LILO) carreguem o kernel
- Padroniza informações passadas ao kernel
- Simplifica o desenvolvimento

### 4. Ponto de Entrada (loader.s)

Quando o GRUB transfere controle, o código em `loader.s` é executado:

#### 4.1. Configuração da Pilha

```assembly
mov esp, kernel_stack + KERNEL_STACK_SIZE
```

- **ESP (Stack Pointer)**: Registrador que aponta para o topo da pilha
- **Pilha**: Área de memória para chamadas de função e variáveis locais
- **Tamanho**: 4 KB (4096 bytes)
- **Crescimento**: De cima para baixo (ESP aponta para o fim)

#### 4.2. Valor de Debug

```assembly
mov eax, 0xCAFEBABE
```

- Coloca valor reconhecível em EAX
- Útil para debug com debuggers
- Pode ser verificado em dumps de memória

#### 4.3. Loop Infinito

```assembly
.loop:
    jmp .loop
```

- Mantém o kernel em execução
- Evita que o processador execute código inválido
- Será substituído por chamada ao código C

## Organização da Memória

### Layout de Memória no Boot

```
0x00000000 - 0x000003FF  : Tabela de Vetores de Interrupção (IVT)
0x00000400 - 0x000004FF  : BIOS Data Area (BDA)
0x00000500 - 0x00007BFF  : Área livre
0x00007C00 - 0x00007DFF  : Bootloader (512 bytes)
0x00007E00 - 0x0009FFFF  : Área livre
0x000A0000 - 0x000FFFFF  : Memória de vídeo e BIOS ROM
0x00100000 - ...         : Kernel (1 MB em diante)
```

### Por que carregar em 1 MB?

- **Primeiros 640 KB**: Memória convencional (pode ter conflitos)
- **640 KB - 1 MB**: Área reservada (vídeo, BIOS)
- **1 MB+**: Memória estendida (segura para o kernel)

## Seções do Kernel

O linker organiza o kernel em seções:

### .text (Código)

- Contém instruções executáveis
- Somente leitura
- Primeira seção após o endereço base

### .rodata (Dados Read-Only)

- Constantes e strings literais
- Somente leitura
- Protege dados contra modificação acidental

### .data (Dados Inicializados)

- Variáveis globais com valores iniciais
- Leitura e escrita
- Valores são copiados do arquivo ELF

### .bss (Dados Não Inicializados)

- Variáveis globais sem valor inicial
- Automaticamente zeradas
- Economiza espaço no arquivo (não armazena zeros)

## Formato ELF

**ELF (Executable and Linkable Format)** é o formato do executável do kernel.

### Estrutura do ELF

```
+------------------+
| ELF Header       | <- Informações básicas
+------------------+
| Program Headers  | <- Como carregar na memória
+------------------+
| .text section    | <- Código
+------------------+
| .rodata section  | <- Constantes
+------------------+
| .data section    | <- Dados inicializados
+------------------+
| .bss section     | <- Dados não inicializados
+------------------+
| Section Headers  | <- Metadados das seções
+------------------+
```

### Vantagens do ELF

- Suportado nativamente pelo GRUB
- Permite múltiplas seções
- Facilita debug e análise
- Padrão em sistemas Unix/Linux
