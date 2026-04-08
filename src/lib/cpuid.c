#include "cpuid.h"

/* Executa CPUID com leaf 'leaf', retorna eax/ebx/ecx/edx */
static void cpuid(unsigned int leaf,
                  unsigned int *eax, unsigned int *ebx,
                  unsigned int *ecx, unsigned int *edx)
{
    asm volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf)
        : );
}

int cpuid_supported(void)
{
    /* Tenta flipar o bit 21 do EFLAGS (ID bit).
     * Se conseguir, CPUID é suportado. */
    unsigned int before, after;
    asm volatile(
        "pushfl\n"
        "popl  %0\n"
        "movl  %0, %1\n"
        "xorl  $0x200000, %1\n"
        "pushl %1\n"
        "popfl\n"
        "pushfl\n"
        "popl  %1\n"
        "popfl\n"          /* restaura EFLAGS original */
        : "=r"(before), "=r"(after)
    );
    return (before != after);
}

void cpuid_vendor(char *buf)
{
    unsigned int eax, ebx, ecx, edx;
    cpuid(0, &eax, &ebx, &ecx, &edx);

    /* Vendor: EBX EDX ECX (nessa ordem, 4 bytes cada) */
    buf[ 0] = (char)( ebx        & 0xFF);
    buf[ 1] = (char)((ebx >>  8) & 0xFF);
    buf[ 2] = (char)((ebx >> 16) & 0xFF);
    buf[ 3] = (char)((ebx >> 24) & 0xFF);
    buf[ 4] = (char)( edx        & 0xFF);
    buf[ 5] = (char)((edx >>  8) & 0xFF);
    buf[ 6] = (char)((edx >> 16) & 0xFF);
    buf[ 7] = (char)((edx >> 24) & 0xFF);
    buf[ 8] = (char)( ecx        & 0xFF);
    buf[ 9] = (char)((ecx >>  8) & 0xFF);
    buf[10] = (char)((ecx >> 16) & 0xFF);
    buf[11] = (char)((ecx >> 24) & 0xFF);
    buf[12] = '\0';
}

void cpuid_brand(char *buf)
{
    unsigned int eax, ebx, ecx, edx;
    unsigned int max_ext;

    /* Verifica se extended CPUID (0x80000002-4) é suportado */
    cpuid(0x80000000, &max_ext, &ebx, &ecx, &edx);

    if (max_ext < 0x80000004) {
        /* Fallback: usa vendor string */
        cpuid_vendor(buf);
        return;
    }

    /* Brand string: 3 chamadas × 16 bytes = 48 bytes + '\0' */
    unsigned int *p = (unsigned int *)buf;
    unsigned int leaf;
    for (leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
        cpuid(leaf, &eax, &ebx, &ecx, &edx);
        *p++ = eax;
        *p++ = ebx;
        *p++ = ecx;
        *p++ = edx;
    }
    buf[48] = '\0';

    /* Remove espaços iniciais que alguns fabricantes incluem */
    char *start = buf;
    while (*start == ' ') start++;
    if (start != buf) {
        char *dst = buf;
        while (*start) *dst++ = *start++;
        *dst = '\0';
    }
}
