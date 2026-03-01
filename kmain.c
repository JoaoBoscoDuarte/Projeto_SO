int sum_of_three(int a, int b, int c) {
    return a + b + c;
}

void kmain(void) {
    char *video = (char*)0xB8000;  // Endereço da memória de vídeo VGA
    int result = sum_of_three(1, 2, 3);
    
    // Converte o resultado para caractere ('6' = 54 em ASCII)
    video[0] = '0' + result;  // Caractere
    video[1] = 0x0F;          // Atributo: branco no fundo preto
}

struct example {
    unsigned char config;   /* bit 0 - 7   */
    unsigned short address; /* bit 8 - 23  */
    unsigned char index;    /* bit 24 - 31 */
} __attribute__((packed));
