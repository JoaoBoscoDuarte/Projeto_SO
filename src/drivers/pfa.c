#include "pfa.h"

#define PAGE_SIZE 4096

/*
 * NOTA SOBRE ENDEREÇOS NO HIGHER-HALF:
 *
 * mbinfo é recebido já com endereço VIRTUAL (conversão feita em kmain).
 * Porém os CAMPOS dentro de mbinfo que são ponteiros (mmap_addr, mods_addr)
 * ainda contêm endereços FÍSICOS — o GRUB os preencheu antes da paginação.
 *
 * Para dereferenciar esses campos como ponteiros C, precisamos somar
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

static unsigned int *bitmap;
static unsigned int total_frames;
static unsigned int bitmap_words;
static unsigned int bitmap_phys_start;
static unsigned int bitmap_phys_end;

static unsigned int align_up(unsigned int addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static unsigned int align_down(unsigned int addr) {
    return addr & ~(PAGE_SIZE - 1);
}

static void set_used(unsigned int frame) { bitmap[frame / 32] |=  (1u << (frame % 32)); }
static void set_free(unsigned int frame) { bitmap[frame / 32] &= ~(1u << (frame % 32)); }
static int  is_used (unsigned int frame) { return bitmap[frame / 32] &   (1u << (frame % 32)); }

static void mark_range_used(unsigned int start, unsigned int end) {
    unsigned int frame = align_down(start) / PAGE_SIZE;
    unsigned int last  = align_up(end) / PAGE_SIZE;

    for (; frame < last && frame < total_frames; frame++)
        set_used(frame);
}

static void mark_range_free(unsigned int start, unsigned int end) {
    unsigned int frame = align_up(start) / PAGE_SIZE;
    unsigned int last  = align_down(end) / PAGE_SIZE;

    for (; frame < last && frame < total_frames; frame++)
        set_free(frame);
}

void pfa_init(unsigned int kphys_start, unsigned int kphys_end,
              multiboot_info_t *mbinfo) {
    unsigned int max_phys = 0;
    unsigned int reserved_top = kphys_end;

    /* 1) descobrir até onde vai a memória física via mmap */
    if (mbinfo->flags & MULTIBOOT_INFO_MEM_MAP) {
        multiboot_memory_map_t *mmap =
            (multiboot_memory_map_t *) PHYS_TO_VIRT(mbinfo->mmap_addr);
        multiboot_memory_map_t *end =
            (multiboot_memory_map_t *) PHYS_TO_VIRT(mbinfo->mmap_addr + mbinfo->mmap_length);

        while (mmap < end) {
            unsigned int start = (unsigned int) mmap->addr;
            unsigned int len   = (unsigned int) mmap->len;
            unsigned int region_end = start + len;

            if (region_end > max_phys)
                max_phys = region_end;

            mmap = (multiboot_memory_map_t *)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    total_frames = align_up(max_phys) / PAGE_SIZE;
    bitmap_words = (total_frames + 31) / 32;

    /* 2) garantir que o bitmap não fique em cima de módulos */
    if (mbinfo->flags & MULTIBOOT_INFO_MODS && mbinfo->mods_count > 0) {
        multiboot_module_t *mods =
            (multiboot_module_t *) PHYS_TO_VIRT(mbinfo->mods_addr);

        unsigned int mods_array_end =
            mbinfo->mods_addr + mbinfo->mods_count * sizeof(multiboot_module_t);

        if (mods_array_end > reserved_top)
            reserved_top = mods_array_end;

        for (unsigned int i = 0; i < mbinfo->mods_count; i++) {
            if (mods[i].mod_end > reserved_top)
                reserved_top = mods[i].mod_end;
        }
    }

    /* 3) colocar o bitmap logo após a maior região reservada já conhecida */
    {
        unsigned int bitmap_bytes = bitmap_words * sizeof(unsigned int);

        bitmap_phys_start = align_up(reserved_top);
        bitmap_phys_end   = align_up(bitmap_phys_start + bitmap_bytes);
        bitmap = (unsigned int *) PHYS_TO_VIRT(bitmap_phys_start);
    }

    /* tudo ocupado por padrão */
    for (unsigned int i = 0; i < bitmap_words; i++)
        bitmap[i] = 0xFFFFFFFFu;

    /* 4) libera regiões marcadas como disponíveis pelo GRUB */
    if (mbinfo->flags & MULTIBOOT_INFO_MEM_MAP) {
        multiboot_memory_map_t *mmap =
            (multiboot_memory_map_t *) PHYS_TO_VIRT(mbinfo->mmap_addr);
        multiboot_memory_map_t *end  =
            (multiboot_memory_map_t *) PHYS_TO_VIRT(mbinfo->mmap_addr + mbinfo->mmap_length);

        while (mmap < end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                unsigned int start = (unsigned int) mmap->addr;
                unsigned int len   = (unsigned int) mmap->len;
                mark_range_free(start, start + len);
            }

            mmap = (multiboot_memory_map_t *)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    /* 5) re-marca regiões que não podem ser usadas */
    mark_range_used(0x00000000, 0x00100000);      /* 1 MB baixo: BIOS/GRUB/I-O */
    mark_range_used(kphys_start, kphys_end);      /* o próprio kernel */
    mark_range_used(bitmap_phys_start, bitmap_phys_end); /* o próprio bitmap */

    /* módulos carregados pelo GRUB */
    if (mbinfo->flags & MULTIBOOT_INFO_MODS && mbinfo->mods_count > 0) {
        multiboot_module_t *mods =
            (multiboot_module_t *) PHYS_TO_VIRT(mbinfo->mods_addr);

        /* reserva também o array de descritores dos módulos */
        mark_range_used(mbinfo->mods_addr,
                        mbinfo->mods_addr + mbinfo->mods_count * sizeof(multiboot_module_t));

        for (unsigned int i = 0; i < mbinfo->mods_count; i++)
            mark_range_used(mods[i].mod_start, mods[i].mod_end);
    }
}

unsigned int pfa_alloc(void) {
    for (unsigned int i = 0; i < total_frames; i++) {
        if (!is_used(i)) {
            set_used(i);
            return i * PAGE_SIZE;
        }
    }
    return 0; /* sem frames livres */
}

unsigned int pfa_total_frames(void) {
    return total_frames;
}

unsigned int pfa_bitmap_start(void) {
    return bitmap_phys_start;
}

unsigned int pfa_bitmap_end(void) {
    return bitmap_phys_end;
}

void pfa_free(unsigned int addr) {
    unsigned int frame = addr / PAGE_SIZE;
    if (frame < total_frames)
        set_free(frame);
}