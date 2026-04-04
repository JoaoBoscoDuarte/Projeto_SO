# Documentação — Projeto SO

Documentação completa do sistema operacional desenvolvido na disciplina de Sistemas Operacionais da UFPB.

## Índice

| # | Documento | Conteúdo |
|---|-----------|----------|
| 1 | [Estrutura do Projeto](01-estrutura-do-projeto.md) | Organização de arquivos, fluxo de build |
| 2 | [Processo de Boot](02-processo-de-boot.md) | BIOS → GRUB → loader.s → kmain |
| 3 | [Conceitos Fundamentais](03-conceitos-fundamentais.md) | x86, memória, interrupções, paginação |
| 4 | [Drivers e Subsistemas](04-drivers-e-subsistemas.md) | FB, teclado, serial, GDT, IDT, PIC, PIT |
| 5 | [Memória](05-memoria.md) | Paginação higher-half, PFA, heap do kernel |
| 6 | [Processos e Scheduler](06-processos-e-scheduler.md) | PCB, context switch, scheduler cooperativo |
| 7 | [Mini-Shell e Top](07-mini-shell-e-top.md) | Shell, comandos, monitor de processos |

---

**Versão atual:** higher-half kernel + paginação 4KB + PFA + heap + processos cooperativos + mini-shell + top
