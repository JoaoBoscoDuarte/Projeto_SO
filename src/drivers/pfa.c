#include "pfa.h"

#define PAGE_SIZE     4096
#define MAX_FRAMES    8192          /* 32 MB / 4 KB */
#define BITMAP_SIZE   (MAX_FRAMES / 32)

/*
 * NOTA SOBRE ENDEREÇOS NO HIGHER-HALF:
 *
 * mbinfo é recebido já com endereço VIRTUAL (conversão feita em kmain).
 * Porém os CAMPOS dentro de mbinfo que são ponteiros (mmap_addr, mods_addr)
 * ainda contêm endereços FÍSICOS — o GRUB os preencheu antes da paginação.
 *
 * Para derreferenciar esses campos como ponteiros C, precisamos somar
 * KERNEL_VIRT_BASE (0xC0000000) a cada um.
 *
 * Campos afetados:
 *   mbinfo->mmap_addr  → físico, aponta para a lista de regiões de memória
 *   mbinfo->mods_addr  → físico, aponta para a lista de módulos
 *
 * A macro PHYS_TO_VIRT faz essa conversão.
 */
#define KERNEL_VIRT_BASE 0xC0000000u
#define PHYS_TO_VIRT(p)  ((unsigned int)(p) + KERNEL_VIRT_BASE)

static unsigned int bitmap[BITMAP_SIZE];

static void set_used(unsigned int frame) { bitmap[frame / 32] |=  (1u << (frame % 32)); }
static void set_free(unsigned int frame) { bitmap[frame / 32] &= ~(1u << (frame % 32)); }
static int  is_used (unsigned int frame) { return bitmap[frame / 32] &   (1u << (frame % 32)); }

static void mark_range_used(unsigned int start, unsigned int end) {
    unsigned int frame = start / PAGE_SIZE;
    unsigned int last  = (end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (; frame < last && frame < MAX_FRAMES; frame++)
        set_used(frame);
}

void pfa_init(unsigned int kphys_start, unsigned int kphys_end,
              multiboot_info_t *mbinfo) {
    /* tudo ocupado por padrão */
    for (unsigned int i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0xFFFFFFFF;

    /* libera regiões marcadas como disponíveis pelo GRUB */
    if (mbinfo->flags & 0x40) {
        /*
         * CORREÇÃO: mbinfo->mmap_addr é FÍSICO — converte para virtual
         * antes de usar como ponteiro.
         */
        multiboot_memory_map_t *mmap =
            (multiboot_memory_map_t *) PHYS_TO_VIRT(mbinfo->mmap_addr);
        multiboot_memory_map_t *end  =
            (multiboot_memory_map_t *) PHYS_TO_VIRT(mbinfo->mmap_addr + mbinfo->mmap_length);

        while (mmap < end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                unsigned int start = (unsigned int) mmap->addr;
                unsigned int len   = (unsigned int) mmap->len;
                unsigned int frame = start / PAGE_SIZE;
                unsigned int last  = (start + len) / PAGE_SIZE;
                for (; frame < last && frame < MAX_FRAMES; frame++)
                    set_free(frame);
            }
            mmap = (multiboot_memory_map_t *)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    /* re-marca regiões que não podem ser usadas */
    mark_range_used(0x00000000, 0x00100000); /* 1 MB baixo: BIOS/GRUB/I-O */
    mark_range_used(kphys_start, kphys_end); /* o próprio kernel */

    /* módulos carregados pelo GRUB */
    if (mbinfo->flags & MULTIBOOT_INFO_MODS && mbinfo->mods_count > 0) {
        /*
         * CORREÇÃO: mbinfo->mods_addr é FÍSICO — converte para virtual.
         */
        multiboot_module_t *mods =
            (multiboot_module_t *) PHYS_TO_VIRT(mbinfo->mods_addr);
        for (unsigned int i = 0; i < mbinfo->mods_count; i++)
            mark_range_used(mods[i].mod_start, mods[i].mod_end);
    }
}

unsigned int pfa_alloc(void) {
    for (unsigned int i = 0; i < MAX_FRAMES; i++) {
        if (!is_used(i)) {
            set_used(i);
            return i * PAGE_SIZE;
        }
    }
    return 0; /* sem frames livres */
}

void pfa_free(unsigned int addr) {
    set_free(addr / PAGE_SIZE);
}