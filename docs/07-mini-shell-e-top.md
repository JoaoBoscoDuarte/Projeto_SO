# 07 — Mini-Shell e Top

## 1. Arquitetura

Shell e Top rodam em **ring 0** (kernel mode), com acesso direto a todas as APIs do kernel. Isso elimina a necessidade de syscalls.

```
kmain()
   │
   └─► shell_run()  ←──────────────────────────────┐
            │                                       │
            ├─► kbd_readline()                      │
            │       │                               │
            │       └─► kbd_getchar()               │
            │               │  buffer vazio?        │
            │               └─► yield() + hlt       │
            │                       │               │
            │               workers executam ───────┘
            │
            └─► shell_execute(cmd)
                    │
                    ├─► cmd_help()
                    ├─► fb_clear()
                    ├─► cmd_ps()
                    ├─► top_run()  ←── redesenha a cada 500ms
                    ├─► cmd_info()
                    ├─► cmd_spawn()  ←── process_create_kernel()
                    ├─► cmd_kill()   ←── process_kill(pid)
                    ├─► cmd_reboot()
                    └─► cmd_poweroff()
```

---

## 2. Mini-Shell (`shell.c`)

### Loop principal

```c
void shell_run(void) {
    char buf[128];
    while (1) {
        kprintf(OUTPUT_FB, "kernel> ");
        kbd_readline(buf, sizeof(buf));  // bloqueia até Enter
        shell_execute(buf);
    }
}
```

### Comandos

| Comando | Descrição |
|---------|-----------|
| `help` | Lista todos os comandos |
| `clear` | Limpa a tela (`fb_clear()`) |
| `ps` | Lista processos com PID, nome, estado e ticks |
| `top` | Abre o monitor de processos (sai com `q`) |
| `info` | Mostra uptime do sistema |
| `spawn [nome]` | Cria processo kernel de teste |
| `kill <pid>` | Mata processo pelo PID |
| `reboot` | Reinicia via pulso no controlador 8042 |
| `poweroff` | Desliga via porta 0x8900 (Bochs) / 0x604 (QEMU) |

### Processo worker (spawn)

```c
static void worker_proc(void) {
    volatile unsigned int counter = 0;
    while (1) {
        counter++;
        if (counter % 50000 == 0)
            yield();  // cede CPU a cada 50000 iterações
    }
}
```

O worker faz trabalho de CPU real entre yields, acumulando ticks enquanto está `RUNNING`.

### Saída posicionada no ps

O `kprintf` não suporta `%-15s`. O `cmd_ps` usa `fb_write_str_at` para posicionar nome e estado em colunas fixas:

```c
unsigned int row = fb_get_cursor_row();
kprintf(OUTPUT_FB, "%d", p->pid);           // coluna 0
fb_write_str_at(row, 5,  p->name, ...);     // coluna 5
fb_write_str_at(row, 21, state_name, ...);  // coluna 21
fb_set_cursor(row, 29);
kprintf(OUTPUT_FB, "%d\n", p->ticks_total); // coluna 29
```

---

## 3. Top (`top.c`)

### Layout da tela

```
Linha 0:  === TOP ================================================
Linha 1:  CPU: Intel(R) Core(TM) i7-...
Linha 2:  Uptime: 1234 ticks  (12 s)
Linha 3:  RAM:  total=32768 KB  used=1024 KB  free=31744 KB
Linha 4:  Heap: total=4 KB  used=0 KB
Linha 5:  --------------------------------------------------------
Linha 6:  PID  NOME             ESTADO   CPU%  MEM(KB)  TICKS
Linha 7:  ---  ---------------  -------  ----  -------  -----
Linha 8+: [dados dos processos]
Linha 24: Pressione 'q' para sair
```

### Cálculo de CPU%

A cada refresh, o top tira um **snapshot** dos ticks de cada processo e do total do sistema. No próximo refresh, calcula o delta:

```c
cpu_pct = (delta_proc / delta_total) * 100
```

Onde `delta_total = pit_get_ticks() - snap_total` e `delta_proc = p->ticks_total - snap_proc[i]`.

### Loop de refresh

```c
void top_run(void) {
    fb_clear();
    snapshot_take();
    while (1) {
        top_draw();
        unsigned int target = pit_get_ticks() + 50;  // ~500ms
        while (pit_get_ticks() < target) {
            char c = kbd_try_getchar();
            if (c == 'q') { fb_clear(); return; }
            asm volatile("hlt");
        }
    }
}
```

O `hlt` aguarda o próximo tick do PIT. `kbd_try_getchar()` é não-bloqueante — verifica o buffer sem fazer yield.

### Informações do CPU (cpuid.c)

```c
cpuid_vendor(buf);  // "GenuineIntel" ou "AuthenticAMD"
cpuid_brand(buf);   // "Intel(R) Core(TM) i7-..." (48 chars)
```

`cpuid_brand` usa os leaves `0x80000002–0x80000004` da instrução `CPUID`. Se não suportados, cai back para o vendor string.

---

## 4. Poweroff

### Bochs

```c
const char *s = "Shutdown";
while (*s) outb(0x8900, *s++);
```

O Bochs monitora a porta `0x8900` e encerra a emulação ao receber a string `"Shutdown"`.

### QEMU

```c
outb(0x604, 0x00);
outb(0x604, 0x20);
```

### Reboot

```c
outb(0x64, 0xFE);  // pulso no pino RESET do controlador 8042
```

---

## 5. Fluxo Completo de Execução

```
boot → kmain → shell_run
                   │
                   ├── [aguarda input] kbd_getchar → yield()
                   │                                    │
                   │                              schedule()
                   │                                    │
                   │                         context_switch → worker
                   │                                              │
                   │                              [executa loop]
                   │                                              │
                   │                              yield() → schedule()
                   │                                    │
                   │                         context_switch → shell
                   │
                   ├── [Enter pressionado] shell_execute(cmd)
                   │
                   ├── "spawn nome" → process_create_kernel(worker_proc)
                   │                  → processo entra na tabela como READY
                   │
                   ├── "top" → top_run()
                   │           │  redesenha tela a cada 500ms
                   │           │  mostra CPU%, RAM, heap, ticks por processo
                   │           └── 'q' → retorna ao shell
                   │
                   └── "kill 1" → process_kill(1) → state = ZOMBIE
```
