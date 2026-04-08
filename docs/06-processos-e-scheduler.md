# 06 — Processos e Scheduler

## 1. Process Control Block (PCB)

Cada processo é representado por uma `process_t` na tabela global `process_table[16]`:

```c
typedef struct process {
    unsigned int  pid;
    char          name[32];
    proc_state_t  state;

    // Contexto salvo para context switch
    unsigned int  esp;          // kernel stack pointer salvo
    unsigned int  ebp;
    unsigned int  eip;          // entry point / resume point

    // Memória
    unsigned int  page_directory_phys;
    unsigned int  kernel_stack_base;
    unsigned int  kernel_stack_top;
    unsigned int  mem_frames;   // frames físicos alocados

    // Contagem de tempo
    unsigned int  ticks_total;  // ticks de CPU consumidos

    // User-space (processos ring 3)
    unsigned int  user_eip;
    unsigned int  user_esp;
} process_t;
```

### Estados

```
PROC_UNUSED  → slot vazio na tabela
PROC_READY   → pronto para executar, aguardando scheduler
PROC_RUNNING → em execução (apenas um por vez)
PROC_BLOCKED → aguardando evento (E/S, sleep)
PROC_ZOMBIE  → terminou, aguardando coleta de recursos
```

### Transições de estado

```
                  schedule()
READY ──────────────────────► RUNNING
  ▲                               │
  │         yield() / schedule()  │
  └───────────────────────────────┘
                                   │ process_kill() / process_exit()
                                   ▼
                                ZOMBIE
```

---

## 2. Criação de Processos

### Processo kernel (`process_create_kernel`)

Cria um processo que executa uma função C em ring 0, compartilhando o page directory do kernel.

**Passos:**
1. Reserva slot na tabela (`process_next_pid`)
2. Aloca e mapeia 1 frame para a kernel stack em `0xC2000000 + pid * 0x2000`
3. Monta o frame inicial na stack
4. Preenche o PCB com `state = PROC_READY`

**Frame inicial na kernel stack:**

O `context_switch` faz `pop edi, pop esi, pop ebx, pop ebp, ret`. O frame deve estar na ordem inversa dos pops (topo = primeiro a ser popado = edi):

```
endereço alto  →  func   ← endereço de retorno (lido pelo ret)
                  ebp=0  ← pop ebp
                  ebx=0  ← pop ebx
                  esi=0  ← pop esi
ESP →             edi=0  ← pop edi  (topo da stack)
```

`proc->esp` aponta para `edi` (topo).

### Processo user-mode (`process_create_full`)

Cria processo completo com espaço de endereçamento próprio:

1. Aloca page directory próprio
2. Copia entradas do kernel (índices 768–1023) para o novo PD
3. Aloca frame para código user, copia binário
4. Mapeia código em `0x00000000` (PAGE_USER_RW)
5. Aloca e mapeia stack user em `0xBFFFF000`
6. Aloca kernel stack em `0xC2000000 + pid * 0x2000`
7. Preenche PCB

---

## 3. Context Switch (`switch.s`)

O `context_switch` é a função mais crítica do sistema. Ela troca a execução de um processo para outro salvando e restaurando apenas os registradores **callee-saved** (os que o chamado deve preservar pela convenção cdecl).

```nasm
context_switch:
    ; Salva callee-saved do processo atual na sua kernel stack
    push ebp
    push ebx
    push esi
    push edi

    ; Salva ESP atual em *old_esp_ptr (proc->esp no PCB)
    ; [esp+20] = old_esp_ptr (4 pushes × 4 + ret addr = 20)
    mov eax, [esp + 20]
    mov [eax], esp

    ; Carrega ESP do novo processo
    ; [esp+24] = new_esp
    mov esp, [esp + 24]

    ; Restaura callee-saved do novo processo
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret   ; salta para o endereço no topo da nova stack
```

### O que acontece na primeira execução de um processo

Quando um processo é escalonado pela primeira vez, sua stack contém o frame inicial montado por `process_create_kernel`. O `context_switch` faz os 4 pops (todos zero) e o `ret` salta para `func` — o entry point do processo.

### O que acontece nas execuções seguintes

O processo foi suspenso dentro de `context_switch` (chamado via `yield → schedule`). Quando é reescalonado, o `ret` final do `context_switch` retorna para dentro de `schedule()`, que retorna para `yield()`, que retorna para onde o processo estava quando chamou `yield()`.

---

## 4. Scheduler Cooperativo (`scheduler.c`)

### Modelo cooperativo

Neste modelo, **o processo decide quando ceder a CPU** chamando `yield()`. Não há preempção automática — um processo que nunca chama `yield()` monopoliza a CPU.

O `kbd_getchar()` chama `yield()` antes de cada `hlt`, garantindo que workers recebam CPU enquanto o shell aguarda input.

### Algoritmo: round-robin

```c
static process_t *find_next_ready(void) {
    // começa no slot após o processo atual
    // percorre a tabela em ordem circular
    // retorna o primeiro com state == PROC_READY
}
```

### schedule()

```c
void schedule(void) {
    process_t *next = find_next_ready();
    if (!next) return;

    process_t *old = current_process;
    old->state  = PROC_READY;
    next->state = PROC_RUNNING;
    tss_set_kernel_stack(next->kernel_stack_top);
    current_process = next;
    context_switch(&old->esp, next->esp);
    // retorna aqui quando 'old' for reescalonado
}
```

### Por que tss_set_kernel_stack?

Se o processo `next` for interrompido em ring 3, a CPU precisa saber para qual kernel stack trocar. O TSS armazena esse endereço em `esp0`. Deve ser atualizado antes de cada troca de processo.

---

## 5. Contagem de Ticks por Processo

O PIT dispara 100 vezes por segundo. A cada tick, `pit_handler_c` incrementa `current_process->ticks_total`:

```c
void pit_handler_c(void) {
    system_ticks++;
    if (current_process)
        current_process->ticks_total++;
    outb(0x20, 0x20);  // EOI
}
```

`ticks_total` representa o tempo de CPU consumido pelo processo — quanto maior, mais CPU ele usou.

---

## 6. Gerenciamento de Processos

### process_kill(pid)

```c
int process_kill(unsigned int pid) {
    // PID 0 (kernel) não pode ser morto
    // Marca o processo como ZOMBIE
    // Se for o processo atual, chama yield() imediatamente
}
```

Processos ZOMBIE permanecem na tabela (seus recursos não são liberados automaticamente nesta versão — garbage collection não implementado).

### Limites

| Parâmetro | Valor |
|-----------|-------|
| MAX_PROCESSES | 16 |
| PROC_NAME_LEN | 32 chars |
| Kernel stack por processo | 4 KB |
| Região de kernel stacks | 0xC2000000+ |
| Stride entre stacks | 0x2000 (8 KB) |
