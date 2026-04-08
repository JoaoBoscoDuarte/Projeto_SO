# Documentação Técnica — Projeto SO

Documentação de referência da implementação. Para instruções de instalação e execução rápida, veja o [README principal](../README.md).

## Índice

| # | Documento | Conteúdo |
|---|-----------|----------|
| 01 | [Estrutura do Projeto](01-estrutura-do-projeto.md) | Árvore de arquivos, flags de compilação, layout de memória virtual |
| 02 | [Processo de Boot](02-processo-de-boot.md) | BIOS → GRUB → loader.s (higher-half) → kmain, ordem de inicialização |
| 03 | [Drivers e Subsistemas](04-drivers-e-subsistemas.md) | Framebuffer, teclado, PIT, serial, GDT, IDT, PIC, kprintf |
| 04 | [Memória](05-memoria.md) | Paginação two-level, temp_map, PFA (bitmap), heap (free-list) |
| 05 | [Processos e Scheduler](06-processos-e-scheduler.md) | PCB, estados, context switch assembly, scheduler cooperativo |
| 06 | [Mini-Shell e Top](07-mini-shell-e-top.md) | Comandos do shell, monitor top, fluxo completo de execução |
| 07 | [Como Rodar](08-como-rodar.md) | Guia detalhado para Linux e macOS (Docker + QEMU) |
