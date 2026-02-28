# Projeto_SO — Registro Técnico Completo (Boot + IDT + ISR + Correções)

> Comandos utilizados durante os testes:
>
> make clean && make && make run
>
> Em alguns momentos também foi usado:
>
> clear && make run
>
> Emulador final: QEMU  
> Bootloader: GRUB2 (Multiboot)  
> Arquitetura: x86 32 bits (i386)

---

# Objetivo da Etapa

Implementar corretamente:

- Boot do kernel via GRUB2
- Escrita direta em VGA
- Instalação da IDT
- Implementação das ISR 0..31
- Tratamento correto de exceções
- Eliminação de double e triple fault

Essa etapa corresponde ao fechamento completo da parte de IDT + ISR + exceções da CPU.

---

# Problema Principal Encontrado

Ao executar:

```c
__asm__("int $0");
```

O sistema não entrava corretamente no handler.

O QEMU mostrava:

```
check_exception old: 0xffffffff new 0xd
...
Triple fault
```

Sequência real do erro:

1. #GP (General Protection Fault – 0x0D)
2. Double Fault (0x08)
3. Triple Fault (reset da CPU)

---

# Diagnóstico Real

Dump do QEMU indicava:

```
CS = 0010
DS = 0018
```

Mas o código estava utilizando:

- Selector incorreto no idt_set_gate (0x08)
- Segmento incorreto dentro do stub
- Inconsistência entre GDT real e selectors usados

Isso fazia a CPU gerar #GP ao tentar entregar a interrupção.

---

# Correções Implementadas

## ✔ Correção 1 — Selector correto na IDT

Antes:

```c
idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
```

Depois:

```c
idt_set_gate(0, (uint32_t)isr0, 0x10, 0x8E);
```

Motivo:  
O CS real do kernel é 0x10.  
O gate precisa apontar para o segmento de código correto.

---

## ✔ Correção 2 — Segmentos corretos no Stub

Antes:

```asm
mov ax, 0x10
mov ds, ax
```

Depois:

```asm
mov ax, 0x18
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
```

Motivo:  
0x18 é o data segment válido do GDT.  
O handler em C usa DS implicitamente.

---

## ✔ Correção 3 — Instalação das ISRs 0..31

Inicialmente apenas isr0 estava configurado.

Agora:

- ISR 0 até 31 são instaladas
- Todas apontam para isr_common_stub
- Evita double/triple fault caso outra exceção ocorra durante o tratamento

---

## ✔ Correção 4 — idtp exportado corretamente

Erro anterior:

```
undefined reference to idtp
```

Correção:

- idtp não pode ser static
- Declarado globalmente
- extern idtp no assembly

Confirmado com:

```
nm -n kernel.elf | grep idtp
```

---

# Estrutura Completa do Projeto (Arquivo por Arquivo)

---

## 📂 loader.s

Função:
- Entry point do kernel
- Header multiboot
- Inicialização básica

Importância:
É o primeiro código executado após o GRUB carregar o kernel.

---

## 📂 link.ld

Função:
- Define layout de memória
- Define entry point
- Organiza seções (.text, .data, .bss)

Importância:
Controla onde o kernel será carregado na memória.

---

## 📂 screen.c / screen.h

Função:
- Escrita direta em VGA (0xB8000)
- Implementa kprint

Exemplo:

```c
static volatile uint16_t* vga = (uint16_t*)0xB8000;
```

Importância:
Permite debug visual do kernel.

---

## 📂 types.h

Função:
Define:
- uint8_t
- uint16_t
- uint32_t

Importância:
Não utilizamos libc, então precisamos definir tipos básicos manualmente.

---

## 📂 idt.h

Define:

```c
struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));
```

Importância:
Define o layout binário exato exigido pela CPU para a IDT.

---

## 📂 idt.c

Contém:
- IDT[256]
- idt_set_gate
- idt_install

Função:
- Inicializa a IDT
- Configura gates 0..31
- Executa lidt

Importância:
Sem IDT válida, qualquer exceção gera triple fault.

---

## 📂 idt_load.s

```asm
lidt [idtp]
ret
```

Importância:
Carrega o ponteiro da IDT na CPU.

---

## 📂 isr_stubs.s

Contém:
- ISR 0..31
- Macros para exceções com/sem err_code
- isr_common_stub

Fluxo:

1. cli
2. push err_code e int_no
3. pusha
4. Ajusta segmentos
5. call isr_handler
6. Restaura contexto
7. iret

Importância:
Faz a ponte entre hardware e código C.

---

## 📂 isr.h

Define:

```c
typedef struct regs {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} regs_t;
```

Importância:
Representa exatamente o layout da pilha criado pelo stub.

---

## 📂 isr.c

```c
void isr_handler(regs_t* r)
{
    kprint("CPU Exception!\n");
    for(;;) __asm__("hlt");
}
```

Função:
Recebe exceção e imprime mensagem.

---

## 📂 kmain.c

Fluxo validado:

```c
kprint("kmain entrou!\n");
idt_install();
kprint("IDT ok\n");
__asm__("int $0");
```

Importância:
É o “main” do kernel.

---

## 📂 Makefile

Responsável por:

- Compilar C e ASM
- Linkar via ld
- Gerar ISO via GRUB2
- Executar no QEMU

Garante build reprodutível.

---

# 6️⃣ Fluxo Final do Sistema

1. GRUB carrega o kernel
2. loader executa
3. kmain inicia
4. IDT é instalada
5. int $0 executa
6. CPU consulta IDT
7. isr0 executa
8. isr_common_stub prepara contexto
9. isr_handler imprime mensagem
10. CPU entra em loop HLT

---

# 7️⃣ Conceitos Fundamentais Aprendidos

- Selector errado → #GP
- #GP durante tratamento → Double Fault
- Double Fault sem handler → Triple Fault
- Gate precisa apontar para CS correto
- DS precisa ser data segment válido
- Stub precisa preservar contexto corretamente
- 0x8E = interrupt gate ring 0

---

# 8️⃣ Estado Atual do Kernel

✔ Boot estável  
✔ VGA funcional  
✔ IDT válida  
✔ ISR 0..31 instaladas  
✔ Exceções capturadas corretamente  
✔ Sem triple fault  

---

# 9️⃣ Próximos Passos Naturais

- Remapear PIC
- Implementar IRQ (timer/keyboard)
- Melhorar mensagens por int_no
- Implementar GDT completa

---

# 🔚 Conclusão

O kernel agora:

- Entra corretamente em modo protegido
- Instala IDT válida
- Entrega interrupções corretamente
- Trata exceções em C
- Evita triple fault

Essa etapa conclui completamente a parte de IDT + ISR + exceções da CPU.