# 04 — Drivers e Subsistemas

## 1. Framebuffer VGA (`fb.c`)

O VGA em modo texto mapeia a tela em `0xB8000` (virtual: `0xC00B8000`). Cada célere ocupa 2 bytes: `[caractere][atributo]`.

```
Tela: 80 colunas × 25 linhas = 2000 células
Cada célula: byte 0 = ASCII, byte 1 = [bg(4) | fg(4)]
```

### API

| Função | Descrição |
|--------|-----------|
| `fb_clear()` | Limpa a tela inteira, cursor para (0,0) |
| `fb_putchar(c)` | Escreve `c` na posição atual, avança cursor, faz scroll se necessário |
| `fb_write(buf, len)` | Escreve buffer na posição atual |
| `fb_write_cell(i, c, fg, bg)` | Escreve na posição linear `i` sem mover cursor |
| `fb_write_at(row, col, c, fg, bg)` | Escreve em coordenadas absolutas |
| `fb_write_str_at(row, col, str, fg, bg)` | Escreve string em coordenadas absolutas |
| `fb_clear_line(row)` | Limpa a linha `row` com espaços |
| `fb_set_cursor(row, col)` | Move cursor para (row, col) |
| `fb_scroll()` | Rola tela uma linha para cima |

### Scroll

Quando o cursor ultrapassa a linha 24, `fb_scroll()` copia as linhas 1–24 para 0–23 e limpa a linha 24. Implementado movendo bytes diretamente na memória VGA.

## 2. Driver de Teclado PS/2 (`keyboard.c`)

### Arquitetura

```
Tecla pressionada
      │
      ▼
Controlador PS/2 (porta 0x60)
      │  gera IRQ1
      ▼
interrupt_handler_33 (assembly)
      │  chama keyboard_handler_c()
      ▼
keyboard_handler_c()
      │  lê scancode de 0x60
      │  converte via kbd_map[]
      │  deposita no buffer circular
      ▼
kbd_getchar() / kbd_readline()
      │  consome do buffer
      ▼
Shell / aplicação
```

### Buffer circular

```c
static volatile char         kbd_buffer[256];
static volatile unsigned int kbd_head = 0;  // escrita (ISR)
static volatile unsigned int kbd_tail = 0;  // leitura (shell)
```

Buffer cheio quando `(head+1) % 256 == tail`. Caracteres excedentes são descartados silenciosamente.

### API

| Função | Comportamento |
|--------|---------------|
| `kbd_getchar()` | Bloqueia (yield + hlt) até ter caractere |
| `kbd_try_getchar()` | Retorna 0 se buffer vazio, sem bloquear |
| `kbd_readline(buf, max)` | Lê linha com eco, trata backspace |

## 3. PIT — Timer (`pit.c`)

O **Programmable Interval Timer** 8253/8254 gera interrupções periódicas (IRQ0).

### Configuração

```c
pit_init(100);  // 100 Hz → 1 tick = 10ms
// divisor = 1193182 / 100 = 11931
```

### pit_handler_c()

Chamado a cada tick (100×/segundo):

```c
void pit_handler_c(void) {
    system_ticks++;
    if (current_process)
        current_process->ticks_total++;
    outb(0x20, 0x20);  // EOI — deve ser antes de qualquer yield/schedule
}
```

O EOI é enviado **dentro** do handler C, antes de qualquer operação que possa trocar de contexto. Se o EOI ficasse no assembly após o `call`, e o handler fizesse um context switch, o EOI nunca seria enviado e o PIC pararia de gerar interrupções.

### API

| Função | Descrição |
|--------|-----------|
| `pit_init(hz)` | Configura frequência |
| `pit_get_ticks()` | Retorna ticks desde o boot |
| `sleep_ticks(n)` | Bloqueia por `n` ticks (usa hlt) |

## 4. Porta Serial (`serial.c`)

Usada exclusivamente para debug. Output vai para `com1.out` no Bochs.

```c
log_debug("mensagem");   // apenas serial
log_info("mensagem");    // tela + serial
log_error("mensagem");   // tela + serial
kprintf(OUTPUT_SERIAL, "valor: %d\n", x);
```

## 5. GDT (`gdt.c` / `gdt.s`)

Configura 6 descritores na GDT e recarrega os registradores de segmento via `lgdt` + far jump.

Após `gdt_init()`, os seletores ativos são:
- `CS = 0x08` (código kernel ring 0)
- `DS = ES = SS = 0x10` (dados kernel ring 0)

## 6. IDT (`idt.c`)

Inicializa 256 entradas zeradas (interrupções não tratadas não causam crash — simplesmente não fazem nada) e registra os handlers ativos:

```c
idt_set_gate(32, interrupt_handler_32, 0x08, 0x8E);  // IRQ0 timer
idt_set_gate(33, interrupt_handler_33, 0x08, 0x8E);  // IRQ1 teclado
```

`0x8E = Present | DPL=0 | 32-bit interrupt gate`

## 7. PIC (`pic.c`)

Remapeia o PIC 8259A para que IRQ0–IRQ7 usem vetores 32–39:

```c
// Máscara final: 0xFC = 11111100
// bit 0 = 0 → IRQ0 (timer) habilitado
// bit 1 = 0 → IRQ1 (teclado) habilitado
// bits 2–7 = 1 → demais IRQs mascarados
outb(PIC1_DATA, 0xFC);
outb(PIC2_DATA, 0xFF);  // PIC escravo: tudo mascarado
```

## 8. kprintf (`printf.c`)

Implementação própria de printf sem dependências do sistema:

```c
kprintf(OUTPUT_FB,     "texto na tela: %d\n", valor);
kprintf(OUTPUT_SERIAL, "debug serial: 0x%x\n", addr);
kprintf(OUTPUT_BOTH,   "tela e serial\n");
```

Especificadores suportados: `%d` (decimal), `%x` (hexadecimal), `%s` (string).

**Limitação:** não suporta especificadores de largura como `%-15s`. Para saída posicionada use `fb_write_str_at()`.
