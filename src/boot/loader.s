; =============================================================================
; loader.s — Ponto de entrada do kernel
;
; RESPONSABILIDADES:
;   1. Fornecer o cabeçalho Multiboot para o GRUB reconhecer o kernel
;   2. Ativar paginação higher-half antes de qualquer código C
;   3. Configurar a stack do kernel
;   4. Chamar kmain() com os argumentos corretos
;
; FLUXO DE EXECUÇÃO:
;   GRUB carrega kernel.elf em 0x00100000 (físico)
;     → executa 'loader' com paginação DESLIGADA
;     → ativa PSE + paginação (identity map + higher-half)
;     → salta para endereço virtual 0xC010xxxx
;     → remove identity map, flush TLB
;     → configura ESP, empurra argumentos
;     → call kmain()
;     → loop infinito (kmain nunca retorna)
;
; ESTADO DA CPU AO ENTRAR EM 'loader' (garantido pelo GRUB/Multiboot):
;   EAX = 0x2BADB002  (magic number — confirma boot via Multiboot)
;   EBX = endereço FÍSICO da struct multiboot_info_t
;   CPU em modo protegido 32-bit, paginação DESLIGADA, interrupções DESLIGADAS
; =============================================================================

global loader           ; ponto de entrada — referenciado em link.ld (ENTRY)
global page_directory   ; exportado para paging.c (extern pde_t page_directory[])
global kernel_stack     ; exportado para tss_init() calcular o topo da stack

; -----------------------------------------------------------------------------
; Constantes Multiboot (especificação: https://www.gnu.org/software/grub/manual/multiboot)
;
; O GRUB procura nos primeiros 8 KB do binário uma sequência de 3 dwords
; alinhada em 4 bytes onde: MAGIC + FLAGS + CHECKSUM == 0.
; -----------------------------------------------------------------------------
MAGIC_NUMBER      equ 0x1BADB002   ; identifica kernel Multiboot
FLAGS             equ 0x00000003   ; bit0=page-align modules, bit1=memory info
CHECKSUM          equ -(MAGIC_NUMBER + FLAGS)   ; garante soma == 0

KERNEL_STACK_SIZE equ 4096         ; 4 KB — suficiente para o boot e kmain
KERNEL_VIRT_BASE  equ 0xC0000000   ; base virtual do kernel (3 GB)

; -----------------------------------------------------------------------------
; Seção .bss — stack do kernel
;
; Reservada aqui (não inicializada) para economizar espaço no binário.
; O endereço é VIRTUAL (0xC01xxxxx) — válido após paginação ser ativada.
; Alinhamento em 4096 é boa prática (coincide com tamanho de página).
; -----------------------------------------------------------------------------
section .bss
align 4096
kernel_stack:
    resb KERNEL_STACK_SIZE

; -----------------------------------------------------------------------------
; Seção .multiboot — cabeçalho Multiboot
;
; DEVE estar nos primeiros 8 KB do binário. O linker script (link.ld) garante
; isso colocando .multiboot antes de .text.
; -----------------------------------------------------------------------------
section .multiboot
align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

; -----------------------------------------------------------------------------
; Seção .data — page directory inicial
;
; Criado em compile-time com duas entradas ativas (PSE = páginas de 4 MB):
;
;   Entrada 0   (virt 0x00000000): identity map temporário → físico 0x0
;               Necessário para continuar executando após ligar paginação,
;               enquanto ainda estamos no endereço físico baixo.
;               Removido logo após o salto para o higher-half.
;
;   Entrada 768 (virt 0xC0000000): higher-half → físico 0x0
;               Mapeia os primeiros 4 MB físicos no endereço virtual 0xC0000000.
;               Permite acessar kernel, VGA (0xB8000) e BIOS via endereços altos.
;
; ALINHAMENTO EM 4096 É OBRIGATÓRIO:
;   CR3 usa os 20 bits altos do endereço — os 12 bits baixos são flags.
;   Se page_directory não estiver alinhado, o MMU lê o PD do lugar errado.
;
; 0x83 = bit0(Present) | bit1(R/W) | bit7(PageSize=4MB)
;   PageSize requer PSE ativo em CR4 (feito antes de ligar CR0.PG).
; -----------------------------------------------------------------------------
section .data
align 4096
page_directory:
    dd 0x00000083                   ; entrada 0:   identity map (temporário)
    times (768 - 1) dd 0            ; entradas 1–767: não mapeadas
    dd 0x00000083                   ; entrada 768: higher-half kernel
    times (1024 - 768 - 1) dd 0     ; entradas 769–1023: não mapeadas

; -----------------------------------------------------------------------------
; Símbolos definidos pelo linker script (link.ld)
; Representam os limites do kernel em memória virtual e física.
; Usados em kmain() para exibir o layout e em pfa_init() para marcar
; a região do kernel como ocupada no bitmap do alocador de frames.
; -----------------------------------------------------------------------------
extern kmain
extern kernel_virtual_start
extern kernel_virtual_end
extern kernel_physical_start
extern kernel_physical_end

; =============================================================================
; Ponto de entrada — executa em endereço FÍSICO 0x00100000, paginação DESLIGADA
; =============================================================================
section .text
loader:
    ; Salva EBX (endereço físico do multiboot_info) em EDI antes de qualquer
    ; operação que possa sobrescrever EBX. Será passado como arg1 para kmain.
    mov edi, ebx

    ; -------------------------------------------------------------------------
    ; Passo 1: Carrega CR3 com o endereço FÍSICO do page_directory
    ;
    ; O linker atribui a 'page_directory' seu endereço VIRTUAL (0xC01xxxxx).
    ; Como paginação ainda está DESLIGADA, CR3 precisa do endereço FÍSICO.
    ; Subtraímos KERNEL_VIRT_BASE para converter VMA → LMA.
    ; -------------------------------------------------------------------------
    mov eax, page_directory - KERNEL_VIRT_BASE
    mov cr3, eax

    ; -------------------------------------------------------------------------
    ; Passo 2: Habilita PSE (Page Size Extension) em CR4
    ;
    ; PSE permite páginas de 4 MB (bit PS=1 nas entradas do PD).
    ; Deve ser ativado ANTES de ligar paginação (CR0.PG), caso contrário
    ; as entradas com bit7=1 seriam interpretadas como PTEs inválidas.
    ; -------------------------------------------------------------------------
    mov eax, cr4
    or  eax, 0x00000010             ; bit 4 = CR4.PSE
    mov cr4, eax

    ; -------------------------------------------------------------------------
    ; Passo 3: Liga paginação em CR0
    ;
    ; A partir deste ponto, TODOS os acessos à memória passam pelo MMU.
    ; O identity map (entrada 0) garante que a próxima instrução (que está
    ; em endereço físico ~0x00100000) continue sendo acessível.
    ; -------------------------------------------------------------------------
    mov eax, cr0
    or  eax, 0x80000000             ; bit 31 = CR0.PG
    mov cr0, eax

    ; -------------------------------------------------------------------------
    ; Passo 4: Salta para o endereço virtual ALTO (0xC010xxxx)
    ;
    ; OBRIGATÓRIO usar salto INDIRETO via registrador.
    ; Um 'jmp .higher_half' direto geraria um offset relativo de 32 bits
    ; que apontaria para o endereço físico, não para o virtual.
    ; LEA carrega o endereço absoluto (VMA) que o linker atribuiu ao label.
    ; -------------------------------------------------------------------------
    lea eax, [.higher_half]
    jmp eax

.higher_half:
    ; Agora executamos em 0xC010xxxx (endereço virtual).
    ; Tanto o identity map (entrada 0) quanto o higher-half (entrada 768)
    ; estão ativos — ambos mapeiam para a mesma memória física 0x0–0x3FFFFF.

    ; -------------------------------------------------------------------------
    ; Passo 5: Remove o identity map (entrada 0 do page directory)
    ;
    ; O identity map foi necessário apenas para sobreviver ao momento em que
    ; paginação foi ligada. Agora que estamos no higher-half, ele não é mais
    ; necessário e deve ser removido para liberar o espaço virtual baixo
    ; para futuros processos de usuário.
    ; -------------------------------------------------------------------------
    mov dword [page_directory], 0

    ; -------------------------------------------------------------------------
    ; Passo 6: Flush COMPLETO do TLB via reload de CR3
    ;
    ; INVLPG [0] NÃO é suficiente para páginas PSE de 4 MB — ele invalida
    ; apenas uma entrada de 4 KB. Para páginas de 4 MB, o único jeito
    ; garantido de invalidar é recarregar CR3 inteiro.
    ; -------------------------------------------------------------------------
    mov eax, cr3
    mov cr3, eax

    ; -------------------------------------------------------------------------
    ; Passo 7: Configura a stack do kernel
    ;
    ; kernel_stack é um símbolo no .bss com endereço VIRTUAL (0xC01xxxxx).
    ; A stack cresce para baixo, então ESP aponta para o TOPO (endereço alto).
    ; Deve ser feito ANTES de qualquer push ou call.
    ; -------------------------------------------------------------------------
    mov esp, kernel_stack + KERNEL_STACK_SIZE

    ; -------------------------------------------------------------------------
    ; Passo 8: Empurra argumentos para kmain (convenção cdecl)
    ;
    ; Ordem: da direita para a esquerda (último argumento primeiro).
    ; Assinatura:
    ;   void kmain(multiboot_addr, virt_start, virt_end, phys_start, phys_end)
    ;
    ; kernel_physical_start/end: calculados em link.ld como expressões numéricas
    ; (não como símbolos de endereço), então têm valor LMA correto diretamente.
    ; -------------------------------------------------------------------------
    push kernel_physical_end        ; arg5: fim físico do kernel
    push kernel_physical_start      ; arg4: início físico do kernel
    push kernel_virtual_end         ; arg3: fim virtual do kernel
    push kernel_virtual_start       ; arg2: início virtual do kernel
    push edi                        ; arg1: endereço FÍSICO do multiboot_info

    call kmain
    add esp, 20                     ; limpa os 5 argumentos (5 × 4 bytes)

    ; -------------------------------------------------------------------------
    ; Fallback — kmain nunca deve retornar (shell_run é um loop infinito).
    ; Se retornar por algum bug, desabilita interrupções e trava a CPU.
    ; -------------------------------------------------------------------------
.loop:
    cli
    hlt
    jmp .loop
