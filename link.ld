/*
 * ============================================================================
 * LINK.LD - Script de Linker do Sistema Operacional
 * ============================================================================
 * Este script define como o linker deve organizar as seções do kernel
 * na memória. Ele especifica o endereço de carregamento e a ordem das seções.
 * ============================================================================
 */

ENTRY(loader)                /* Define 'loader' como ponto de entrada do programa */

SECTIONS {
    /* Endereço base: 1 MB (0x00100000)
     * O kernel é carregado em 1 MB para evitar conflitos com:
     * - BIOS e hardware mapeado (primeiros 1 MB)
     * - Área de memória reservada do sistema
     */
    . = 0x00100000;          /* Define o contador de localização para 1 MB */

    /* Seção .text - Código executável
     * Contém todas as instruções do programa (assembly e C compilado)
     */
    .text ALIGN (0x1000) :
    {
        *(.multiboot)
        *(.text)
    }

    /* Seção .rodata - Dados somente leitura
     * Contém constantes e strings literais do programa
     */
    .rodata ALIGN (0x1000) : /* Alinha em 4 KB */
    {
        *(.rodata*)          /* Inclui todas as seções .rodata de todos os arquivos */
    }

    /* Seção .data - Dados inicializados
     * Contém variáveis globais e estáticas com valores iniciais
     */
    .data ALIGN (0x1000) :   /* Alinha em 4 KB */
    {
        *(.data)             /* Inclui todas as seções .data de todos os arquivos */
    }

    /* Seção .bss - Dados não inicializados
     * Contém variáveis globais e estáticas sem valor inicial (zeradas)
     * BSS = Block Started by Symbol
     */
    .bss ALIGN (0x1000) :    /* Alinha em 4 KB */
    {
        *(COMMON)            /* Inclui seções COMMON (variáveis não inicializadas) */
        *(.bss)              /* Inclui todas as seções .bss de todos os arquivos */
    }
}