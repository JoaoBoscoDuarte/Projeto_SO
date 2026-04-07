# 1. Visão Geral

O projeto é um sistema operacional de 32 bits para arquitetura x86, desenvolvido em linguagem C e Assembly, capaz de:
* Dar boot em um computador real ou emulado (via GRUB, o mesmo bootloader do Linux)
* Gerenciar a memória RAM do hardware
* Receber e processar eventos do teclado e do relógio interno
* Executar múltiplos processos de forma concorrente
* Proteger o núcleo do sistema de acessos indevidos

Também foi criado um mini shell, uma interface interativa para que o usuário digite comandos, crie/mate processos, veja o uso da CPU real-time e até desligue a máquina.

# 2. Mapeamento da Arquitetura
## Capítulo 2 — Bootloader

Ao ligar um computador, ele não sabe nada. O processador acorda em um estado primitivo chamado modo real (16 bits) e começa a executar instruções a partir de um endereço de memória fixo, onde está a BIOS. A BIOS então procura um programa carregador (o bootloader) no disco. O papel de carregador é feito pelo GRUB, que localiza o kernel na imagem ISO e o coloca na memória RAM.

Para que o GRUB reconheça o kernel, o binário precisa ter uma ID identificação em seus primeiros bytes: o cabeçalho Multiboot. 

Mas há um problema a resolver: o kernel foi compilado para funcionar em endereços de memória altos (acima de 0xC0000000, ou seja, acima de 3 GB), mas o computador o carrega nos primeiros megabytes da RAM. A solução é a técnica higher-half kernel: ativar a paginação de memória imediatamente no boot para "enganar" o processador e fazê-lo achar que o kernel está lá em cima, enquanto fisicamente ele está embaixo.

**ARQUIVOS:**
* src/boot/loader.s: O ponto de entrada absoluto do sistema
* link.ld: O "mapa de construção" que diz ao compilador onde colocar cada peça do kernel na memória

**EM PRÁTICA:**
`loader.s` é o primeiro código executado após o GRUB terminar seu trabalho. Ele:

1. Coloca o cabeçalho Multiboot nos primeiros bytes para o GRUB reconhecer o kernel
2. Configura imediatamente a paginação de memória com páginas de 4 MB (usando um truque chamado PSE — Page Size Extension), criando dois mapeamentos temporários: um "espelho" de onde o kernel realmente está, e o mapeamento definitivo no endereço alto (0xC0000000)
3. Executa um salto para o endereço virtual alto, tornando o higher-half permanente
4. Remove o "espelho" temporário do início da memória e limpa o cache de tradução de endereços (TLB)
5. Configura a pilha de execução do kernel e chama a função `kmain()` em C

`link.ld` instrui o linker (o programa que une todos os arquivos .o compilados em um único executável) a organizar o código do kernel a partir do endereço virtual 0xC0100000, garantindo que tudo se encaixe no mapa de memória esperado.

## Capítulo 3 — Framebuffer
Em modo texto, o VGA disponibiliza uma região especial da memória no endereço 0xB8000. Cada posição na tela corresponde a 2 bytes nessa região: o primeiro byte é o caractere ASCII e o segundo define as cores (frente e fundo). 

**ARQUIVOS:**
- src/drivers/fb.c e src/include/fb.h: O driver de tela (framebuffer)
- src/lib/printf.c: Implementação própria do printf
- src/lib/utf8.c e src/include/utf8.h: Conversão de caracteres acentuados para o formato VGA

**EM PRÁTICA:**
`fb.c` implementa toda a lógica de escrita na tela: mover o cursor, escrever caracteres, lidar com quebras de linha, fazer o scroll automático quando o texto chega na última linha (copiando todo o conteúdo uma linha para cima), e até escrever texto em posições absolutas da tela (linha e coluna exatas), o que é usado pelo shell para criar tabelas alinhadas.

Como o sistema não usa nenhuma biblioteca padrão, printf.c reimplementa do zero uma versão simplificada do printf, capaz de formatar inteiros decimais, hexadecimais e strings. Ela pode enviar a saída para a tela, para a porta serial ou para ambos simultaneamente.

## Capítulo 4 — Porta Serial
A porta serial (COM1) é uma interface de comunicação. Ela é usada exclusivamente para debug: o emulador (Bochs) redireciona tudo que o kernel escreve na porta serial para um arquivo com1.out. Isso é extremamente útil porque permite ver mensagens de diagnóstico sem poluir a tela principal.

**ARQUIVOS:**
- src/drivers/serial.c e src/include/serial.h: Driver da porta serial
- src/drivers/io.s e src/include/io.h: Funções de leitura/escrita em portas de hardware

**EM PRÁTICA:**
`serial.c` configura a porta COM1 e implementa a função de envio de bytes. `io.s` fornece as funções outb() e inb(), as únicas formas de se comunicar com hardware, usando instruções especiais (out/in) que acessam um espaço de endereçamento separado do da memória RAM, reservado para dispositivos de hardware.

## Capítulo 5 — GDT

O processador x86, quando opera em modo protegido (32 bits), não deixa nenhum programa acessar qualquer pedaço de memória livremente. Existe uma tabela chamada Global Descriptor Table (GDT) que define "segmentos" de memória: cada um com seu endereço base, tamanho e nível de acesso.

Se pensarmos na GDT como um livro de crachás, cada crachá define: "quem tem esse crachá pode entrar nessa área, com essas permissões". O processador verifica esse livro antes de qualquer acesso à memória. O nível de privilégio vai de Ring 0 (o núcleo do sistema, acesso total) a Ring 3 (programas de usuário, muito restrito).

**ARQUIVOS:**
- src/drivers/gdt.c e src/include/gdt.h: Definição e inicialização da GDT
- src/drivers/gdt.s: Instrução para carregar a GDT no processador

**EM PRÁTICA:**
`gdt.c` cria uma tabela com 6 entradas: o obrigatório null descriptor, segmentos de código e dados para o kernel (Ring 0, acesso total), segmentos de código e dados para processos de usuário (Ring 3, acesso restrito), e uma entrada para o TSS (explicado mais adiante). `gdt.s` executa a instrução lgdt (load GDT) que registra essa tabela no processador, seguida de um salto especial para atualizar os registradores de segmento internos da CPU.

## Capítulo 6 — Interrupções
Quando o teclado detecta uma tecla pressionada, ele envia um sinal ao processador. Esse sinal chega primeiro ao PIC (Programmable Interrupt Controller, o controlador de interrupções), que o traduz para um número de vetor e avisa o processador. O processador então para o que está fazendo, consulta a IDT (Interrupt Descriptor Table, a lista de "o que fazer quando cada sinal chegar"), e executa a função handler correspondente.

Por padrão, o PIC mapeia os sinais do teclado e do timer para vetores que conflitam com exceções internas da CPU. Por isso, é necessário remapeá-lo para usar vetores mais altos.

**ARQUIVOS:**
- src/drivers/idt.c e src/include/idt.h: Tabela de descritores de interrupção
- src/drivers/interrupts.s: Handlers de interrupção em Assembly (o "atendedor" do sinal)
- src/drivers/pic.c e src/include/pic.h: Controlador de interrupções (remapeamento)
- src/drivers/keyboard.c e src/include/keyboard.h: Driver do teclado PS/2
- src/drivers/pit.c e src/include/pit.h: Relógio interno do sistema (timer PIT)

**EM PRÁTICA:**
`pic.c` remapeia o PIC para que IRQ0 (timer) use o vetor 32 e IRQ1 (teclado) use o vetor 33, evitando conflitos. `idt.c` inicializa a tabela com 256 entradas zeradas e registra os dois handlers ativos.

`interrupts.s` contém o código executado quando uma interrupção chega. Ele salva todos os registradores do processador na pilha (para que o processo interrompido não perceba que foi pausado), chama a função correspondente, e depois restaura tudo e retorna.

`keyboard.c` implementa o driver completo do teclado: ao receber a interrupção, lê o código da tecla pressionada da porta 0x60, converte para ASCII e insere o caractere em um buffer circular (como uma fila com 256 posições). O shell então consome esse buffer quando precisar de input do usuário.

`pit.c` configura o timer do hardware para disparar 100 vezes por segundo (100 Hz). A cada disparo, o handler incrementa um contador global de ticks e o contador de ticks do processo em execução, uma informação essencial para medir uso de CPU.

## Capítulo 7 — Paginação
O processador trabalha com dois tipos de endereços: o endereço virtual (o que o programa vê e usa) e o endereço físico (onde a informação realmente está na RAM). A paginação é o sistema que traduz um para o outro. Isso garante isolamento, já que um programa não consegue acessar a memória de outro.

A memória é dividida em blocos de 4 KB chamados páginas. Uma estrutura de dois níveis traduz qualquer endereço virtual de 32 bits para seu endereço físico real.

**ARQUIVOS:**
- src/drivers/paging.c e src/include/paging.h: Sistema de paginação (mapeamento virtual x físico)
- src/drivers/pfa.c e src/include/pfa.h: Page Frame Allocator (gerenciador de memória física)
- src/kernel/kheap.c e src/include/kheap.h: Heap do kernel (alocação dinâmica como malloc)

**EM PRÁTICA:**
`pfa.c` é o gerenciador de memória física. Ele consulta o mapa de memória fornecido pelo GRUB (via protocolo Multiboot) para saber exatamente quanta RAM o computador tem e quais regiões estão disponíveis. Armazena essa informação em um bitmap - um array onde cada bit representa uma página de 4 KB (bit 0 = livre, bit 1 = ocupado). Quando alguém precisa de memória, ele encontra o primeiro bit zero, marca como ocupado e retorna o endereço físico correspondente.

`paging.c` implementa a segunda camada: substitui o mapeamento de 4 MB do boot por page tables reais de 4 KB, oferecendo funções para mapear e desmapear páginas individuais. Também tem um "slot temporário" que permite acessar brevemente qualquer endereço físico para inicializar uma nova page table antes de mapeá-la definitivamente.

`kheap.c` fica no topo de tudo e oferece a interface mais amigável: as funções kmalloc() e kfree(). Ele mantém uma lista encadeada de blocos de memória, divide blocos grandes ao alocar (split) e funde blocos adjacentes livres ao desalocar (coalescing), expandindo automaticamente o espaço da heap pedindo novas páginas ao PFA conforme necessário.

## Capítulos 11 e 13 — GDT Expandida e TSS
Em um sistema operacional real, os programas do usuário não podem acessar diretamente o hardware nem a memória do kernel. Para isso, o processador x86 tem os rings de privilégio: Ring 0 (kernel, deus do sistema) e Ring 3 (aplicativos, cidadão comum).

Mas quando um programa em Ring 3 precisa de algo do kernel (ou quando uma interrupção ocorre enquanto um programa está em Ring 3), o processador precisa fazer uma troca segura de contexto, ou seja, e ele precisa saber para qual pilha do kernel trocar. Essa informação fica numa estrutura especial chamada TSS (Task State Segment).

**ARQUIVOS:**
- src/drivers/tss.c e src/include/tss.h: Task State Segment
- src/drivers/tss.s: Instrução para carregar o TSS
- src/boot/usermode.s: Transição do kernel (Ring 0) para programa de usuário (Ring 3)

**EM PRÁTICA:**
`tss.c` cria a estrutura TSS e a registra na entrada 5 da GDT. Os campos mais importantes são ss0 e esp0: eles dizem ao processador "quando uma interrupção acontecer enquanto um programa de usuário estiver rodando, troque para essa pilha do kernel". A cada troca de processo, tss_set_kernel_stack() atualiza esses campos para apontar para a pilha correta do processo atual.

`tss.s` executa a instrução ltr (Load Task Register) que ativa o TSS no hardware.

`usermode.s` implementa a transição de Ring 0 para Ring 3 usando a instrução iret. Ela monta cuidadosamente um frame na pilha com o endereço de início do programa, os seletores de segmento com privilégio de usuário, e as flags do processador e, só depois, executa iret, que completa a transição para o ring 3 sem possibilidade de retorno.

## Capítulo 13 — Processos e Escalonador
O processador não executa vários programas ao mesmo tempo, ele alterna muito rapidamente entre eles. A ilusão de simultaneidade é criada pelo escalonador (scheduler): um componente que decide qual processo roda agora e quando trocar.

Cada processo é uma tarefa independente com seu próprio conjunto de registradores, sua própria pilha e (em processos de usuário) seu próprio mapa de memória. Quando o escalonador troca de processo, ele salva o estado completo do processo atual e carrega o estado do próximo (isso se chama troca de contexto). É como pausar um filme e salvar exatamente onde estava para continuar depois.

Neste sistema, o modelo é cooperativo: os processos precisam voluntariamente chamar yield() para ceder a CPU ao próximo. A ordem de execução segue o algoritmo round-robin.

**ARQUIVOS:**
- src/kernel/process.c e src/include/process.h: Criação e gerenciamento de processos
- src/kernel/scheduler.c e src/include/scheduler.h: Escalonador (round-robin cooperativo)
- src/kernel/switch.s: Núcleo de troca de contexto

**EM PRÁTICA:**
`process.c` mantém uma tabela global com até 16 processos. Cada entrada é um PCB (Process Control Block) — um cartão de identidade do processo contendo: PID, nome, estado (UNUSED/READY/RUNNING/BLOCKED/ZOMBIE), endereço da pilha salvo, pilha do kernel, diretório de páginas e contador de ticks de CPU consumidos.

Existem dois tipos de processo criáveis: processos de kernel (process_create_kernel) que rodam em Ring 0 e compartilham o mapa de memória do kernel, e processos completos de usuário (process_create_full) que têm seu próprio page directory isolado.

`switch.s` é a função mais crítica de todo o sistema. Ela salva os registradores essenciais do processo atual na sua pilha, salva o ponteiro de pilha no PCB, carrega o ponteiro de pilha do próximo processo e restaura seus registradores — transferindo efetivamente a execução.

`scheduler.c` implementa o round-robin: percorre circularmente a tabela de processos procurando o próximo com estado READY, atualiza o TSS com a pilha do kernel do novo processo (essencial para interrupções Ring 3), e chama a troca de contexto.

# 3. Mini shell

## Arquitetura
O shell e o visualizador de processos (top) rodam diretamente em Ring 0, com acesso total às APIs do kernel. Isso elimina a necessidade de chamadas de sistema complexas: em vez de pedir ao kernel para escrever na tela, o shell escreve diretamente.

**ARQUIVOS:**
- src/kernel/shell.c e src/include/shell.h: Interpretador de comandos principal
- src/kernel/top.c e src/include/top.h: Monitor de processos em tempo real
- src/lib/string.c e src/include/string.h: Funções de string (strcmp, strlen, etc.)
- src/lib/cpuid.c e src/include/cpuid.h: Detecção do modelo do processador

**EM PRÁTICA:**
O fluxo completo, do pressionamento de tecla até a execução do comando, é o seguinte:

1. O usuário pressiona uma tecla 
2. O controlador PS/2 gera uma interrupção de hardware
3. `interrupts.s` captura e chama keyboard_handler_c()
4. O caractere é inserido no buffer circular do teclado
5. `shell.c` está aguardando em kbd_readline(), que consome do buffer caractere a caractere, ecoando cada um na tela
6. Ao pressionar Enter, shell_execute() recebe a string completa e a compara com os comandos conhecidos.

Enquanto o shell aguarda o Enter (o que pode ser muito tempo em termos de processador), ele chama yield() antes de cada hlt, cedendo a CPU para outros processos rodarem. É assim que workers criados com spawn conseguem executar em paralelo com o shell.


| Comando | O que faz | Como se comunica com o kernel |
|---------|----------|------------------------------|
| help | Lista os comandos | Chama diretamente kprintf() |
| clear | Limpa a tela | Chama fb_clear() do driver VGA |
| ps | Lista todos os processos | Lê process_table[] diretamente |
| top | Monitor ao vivo de processos | Combina pit_get_ticks(), process_table[] e cpuid_brand() |
| info | Exibe uptime do sistema | Lê pit_get_ticks() e divide por 100 |
| spawn [nome] | Cria um processo de teste | Chama process_create_kernel() |
| kill <pid> | Encerra um processo | Chama process_kill(pid) |
| reboot | Reinicia a máquina | Envia pulso na porta 0x64 (controlador 8042 do teclado) |
| poweroff | Desliga a máquina | Escreve "Shutdown" na porta 0x8900 (Bochs) ou na porta 0x604 (QEMU) |

## Top:
`top.c` implementa um monitor de processos similar ao htop do Linux. 

A cada ~500 ms, ele redesenha a tela mostrando o nome do processador (obtido via instrução CPUID do hardware), uptime, uso de RAM total e usada, uso da heap do kernel, e para cada processo: PID, nome, estado, percentual de CPU e ticks acumulados.

O cálculo de CPU é feito por diferença: a cada refresh, o top tira um "foto" dos ticks de cada processo e do total do sistema. Na próxima foto, calcula a diferença — quanto de CPU foi consumido no intervalo. Quem mais consumiu, maior o percentual.

## Infraestrutura do shell
- string.c: reimplementa funções essenciais que normalmente viriam da biblioteca padrão C (strcmp, strlen, strcpy, memset, memcpy, atoi).
- cpuid.c: usa a instrução especial CPUID, que retorna informações diretas do processador. Com os leaves, é possível extrair o nome completo do processador (ex: "Intel(R) Core(TM) i7-4770"), que aparece na tela do top.

```
┌─────────────────────────────────────────────────────┐
│                   SHELL (shell.c)                    │
│  help / clear / ps / top / spawn / kill / reboot    │
└────────────────────────┬────────────────────────────┘
                         │ usa diretamente
┌────────────────────────▼────────────────────────────┐
│              KERNEL CORE (kmain.c)                   │
├─────────┬──────────┬──────────┬──────────┬──────────┤
│ process │scheduler │  kheap   │  paging  │   pit    │
│   .c    │    .c    │    .c    │    .c    │   .c     │
└────┬────┴────┬─────┴────┬─────┴────┬─────┴────┬─────┘
     │         │          │          │          │
┌────▼─────────▼──────────▼──────────▼──────────▼─────┐
│              DRIVERS DE HARDWARE                      │
│  fb.c  │ keyboard.c │ gdt.c │ idt.c │ serial.c      │
└────────────────────────┬────────────────────────────┘
                         │ acessa diretamente
┌────────────────────────▼────────────────────────────┐
│                    HARDWARE                          │
│  Tela VGA │ Teclado PS/2 │ Timer PIT │ CPU x86      │
└─────────────────────────────────────────────────────┘
````

