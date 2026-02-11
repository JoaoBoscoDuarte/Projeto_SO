# Conceitos Fundamentais de Sistemas Operacionais

## Introdução

Este documento explica os conceitos essenciais para entender o desenvolvimento de um sistema operacional, com foco nos aspectos implementados neste projeto.

## 1. Arquitetura x86

### 1.1. Registradores

Registradores são pequenas áreas de memória extremamente rápidas dentro do processador.

#### Registradores de Propósito Geral (32 bits)

- **EAX**: Acumulador (operações aritméticas, retorno de funções)
- **EBX**: Base (endereçamento de memória)
- **ECX**: Contador (loops)
- **EDX**: Dados (operações de I/O)
- **ESI**: Source Index (operações com strings)
- **EDI**: Destination Index (operações com strings)

#### Registradores de Ponteiro

- **ESP**: Stack Pointer (aponta para o topo da pilha)
- **EBP**: Base Pointer (base do stack frame)
- **EIP**: Instruction Pointer (próxima instrução a executar)

#### Registradores de Segmento

- **CS**: Code Segment
- **DS**: Data Segment
- **SS**: Stack Segment
- **ES, FS, GS**: Segmentos extras

### 1.2. Modos de Operação

#### Real Mode (16 bits)

- Modo inicial do processador
- Acesso direto à memória
- Limitado a 1 MB de RAM
- Usado pela BIOS

#### Protected Mode (32 bits)

- Proteção de memória
- Multitarefa
- Acesso a mais de 1 MB de RAM
- Usado por sistemas operacionais modernos

#### Long Mode (64 bits)

- Extensão do Protected Mode
- Registradores de 64 bits
- Mais memória endereçável

## 2. Memória

### 2.1. Pilha (Stack)

A pilha é uma estrutura de dados LIFO (Last In, First Out) usada para:

- Armazenar endereços de retorno de funções
- Passar parâmetros para funções
- Armazenar variáveis locais
- Salvar estado de registradores

#### Operações

```assembly
push eax    ; Coloca EAX na pilha (ESP -= 4)
pop ebx     ; Remove topo da pilha para EBX (ESP += 4)
```

#### Crescimento

A pilha cresce **para baixo** (de endereços altos para baixos):

```
Endereços Altos
    ↓
[ESP] ← Topo da pilha
[   ]
[   ]
[   ] ← Base da pilha
    ↓
Endereços Baixos
```

### 2.2. Heap

Área de memória para alocação dinâmica:

- Gerenciada por malloc/free (em C)
- Cresce para cima (endereços crescentes)
- Requer gerenciador de memória

### 2.3. Segmentação

Divisão da memória em segmentos lógicos:

- **Code Segment**: Código executável
- **Data Segment**: Dados
- **Stack Segment**: Pilha

### 2.4. Paginação

Sistema de memória virtual que:

- Divide memória em páginas (geralmente 4 KB)
- Mapeia endereços virtuais para físicos
- Permite proteção de memória
- Facilita multitarefa

## 3. Assembly x86

### 3.1. Sintaxe NASM

#### Diretivas

```assembly
global symbol    ; Exporta símbolo
extern symbol    ; Importa símbolo
section .text    ; Define seção
equ             ; Define constante
```

#### Instruções Básicas

```assembly
mov dest, src   ; Move dados
add dest, src   ; Soma
sub dest, src   ; Subtrai
jmp label       ; Salta incondicionalmente
call func       ; Chama função
ret             ; Retorna de função
```

#### Tamanhos de Dados

- **byte**: 8 bits
- **word**: 16 bits
- **dword**: 32 bits (double word)
- **qword**: 64 bits (quad word)

### 3.2. Convenções de Chamada

#### cdecl (C Declaration)

- Parâmetros empilhados da direita para esquerda
- Chamador limpa a pilha
- Retorno em EAX
- Usado por compiladores C

## 4. Bootloader

### 4.1. Função

O bootloader é responsável por:

1. Ser carregado pela BIOS
2. Configurar ambiente básico
3. Carregar o kernel na memória
4. Transferir controle para o kernel

### 4.2. GRUB

**GRUB (Grand Unified Bootloader)** é um bootloader flexível que:

- Suporta múltiplos sistemas operacionais
- Entende vários sistemas de arquivos
- Implementa especificação Multiboot
- Fornece menu de boot

### 4.3. Multiboot

Especificação que define:

- Formato do cabeçalho do kernel
- Informações passadas ao kernel
- Estado do processador no boot

## 5. Linker e Loader

### 5.1. Linker (ld)

Combina arquivos objeto em executável:

- Resolve referências entre arquivos
- Organiza seções na memória
- Aplica relocações
- Gera arquivo final (ELF)

### 5.2. Linker Script

Define:

- Endereço de carregamento
- Ordem das seções
- Alinhamento de memória
- Símbolos especiais

### 5.3. Loader

Carrega executável na memória:

- Lê cabeçalho ELF
- Mapeia seções na memória
- Aplica relocações
- Transfere controle ao entry point

## 6. Compilação Freestanding

### 6.1. O que é?

Compilação **freestanding** significa compilar sem dependências do sistema operacional host.

### 6.2. Flags Importantes

```makefile
-nostdlib       # Não usa biblioteca padrão
-nostdinc       # Não usa headers padrão
-fno-builtin    # Desabilita funções built-in
-nostartfiles   # Não usa arquivos de inicialização
```

### 6.3. Implicações

- Não há printf, malloc, etc.
- Deve implementar tudo do zero
- Controle total sobre o ambiente
- Necessário para kernel

## 7. Formato ELF

### 7.1. Estrutura

- **ELF Header**: Metadados do arquivo
- **Program Headers**: Como carregar na memória
- **Section Headers**: Informações de debug/link

### 7.2. Tipos de Seções

- **.text**: Código executável
- **.rodata**: Dados read-only
- **.data**: Dados inicializados
- **.bss**: Dados não inicializados

## 8. Emulação vs Virtualização

### 8.1. Bochs (Emulador)

- Simula hardware x86 em software
- Mais lento
- Melhor para debug
- Funciona em qualquer arquitetura

### 8.2. QEMU/VirtualBox (Virtualização)

- Executa código diretamente no processador
- Mais rápido
- Requer mesma arquitetura
- Menos controle para debug

## 9. Interrupções

### 9.1. O que são?

Sinais que pausam a execução normal para tratar eventos:

- Hardware (teclado, timer)
- Software (syscalls)
- Exceções (divisão por zero)

### 9.2. IDT (Interrupt Descriptor Table)

Tabela que mapeia números de interrupção para handlers.
