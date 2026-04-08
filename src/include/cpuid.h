#ifndef INCLUDE_CPUID_H
#define INCLUDE_CPUID_H

/* Preenche buf (min 13 bytes) com o vendor string do CPU, ex: "GenuineIntel" */
void cpuid_vendor(char *buf);

/* Preenche buf (min 49 bytes) com o nome completo do CPU.
 * Requer suporte a CPUID extended (0x80000002-0x80000004).
 * Se não suportado, copia o vendor string em vez disso. */
void cpuid_brand(char *buf);

/* Retorna 1 se CPUID é suportado, 0 caso contrário */
int cpuid_supported(void);

#endif /* INCLUDE_CPUID_H */
