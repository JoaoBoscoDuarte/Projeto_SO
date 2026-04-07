# 10 - Ajuste do escalonador e inicialização de processos kernel

## Resumo da Mudança

Foi identificado um problema ao criar processos via spawn: o sistema aparentava travar e o shell parava de responder.

O problema estava relacionado a dois pontos:

1. Uso simultâneo de escalonamento cooperativo e preemptivo
2. Inicialização incompleta dos processos kernel (interrupções desabilitadas)

---

## Problema 1 — Preempção incompatível

O sistema utilizava:

- yield (cooperativo)
- schedule() dentro do PIT (preemptivo)

Porém, os processos criados por process_create_kernel foram projetados para o modelo cooperativo.

Quando o schedule era chamado dentro da interrupção do timer:

- a troca de contexto acontecia em contexto de IRQ
- o processo não iniciava corretamente
- o shell deixava de receber CPU
- o sistema aparentava travar

---

## Problema 2 — Interrupções desabilitadas no início do processo

Os processos kernel eram iniciados via:

- context_switch → ret → função do processo

Ou seja, não passavam por iret.

Com isso, o flag de interrupção (IF) podia estar desabilitado, fazendo com que:

- timer parasse de funcionar
- teclado não respondesse

---

## Solução 1 — Remover preempção do PIT

Foi removida a chamada ao scheduler dentro do handler do timer.

Antes:
```c
    if (system_ticks % PREEMPT_INTERVAL == 0) {
        schedule();
    }
```

Depois:

    /* preempção removida temporariamente */

Agora o sistema funciona apenas com yield (modo cooperativo).

---

## Solução 2 — Stub de inicialização de processos kernel

Foi criada uma função intermediária para garantir que todo processo kernel:

- habilite interrupções
- execute corretamente sua função
- finalize de forma segura

### Implementação
```c
    static void kernel_process_start_stub(void)
    {
        void (*entry)(void);

        entry = (void (*)(void))current_process->eip;

        /* garante que timer e teclado funcionem */
        asm volatile("sti");

        entry();

        process_exit();

        while (1)
            yield();
    }
```
---

## Alteração no process_create_kernel

Antes:
```c
    *(--sp) = (unsigned int)func;
```
Depois:
```c
    *(--sp) = (unsigned int)kernel_process_start_stub;
```
A função real continua sendo armazenada em:
```c
    proc->eip = (unsigned int)func;
```
---

## Novo comportamento

- processos kernel sempre iniciam com interrupções habilitadas
- execução ocorre de forma consistente
- shell continua funcionando após spawn
- escalonamento ocorre via yield

---

## Resultado

- spawn funciona corretamente
- processos executam normalmente
- ticks_total aumenta corretamente
- teclado continua responsivo
- sistema estável

---

## Limitação

Sistema ainda não possui preempção real por timer.

---

## Conclusão

A combinação das duas correções (remoção da preempção e uso da stub de inicialização) resolveu o problema de forma simples e robusta, mantendo o sistema estável para demonstração.