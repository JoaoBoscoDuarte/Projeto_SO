# Documentação do Projeto SO

## Bem-vindo!

Esta pasta contém a documentação completa do projeto de Sistema Operacional. Os documentos estão organizados de forma progressiva, do básico ao avançado.

## Índice de Documentos

### 1. [Estrutura do Projeto](01-estrutura-do-projeto.md)
**Recomendado para**: Iniciantes

Explica a organização dos arquivos e diretórios do projeto:
- Árvore de diretórios
- Função de cada arquivo
- Arquivos gerados durante o build
- Fluxo de compilação

### 2. [Processo de Boot](02-processo-de-boot.md)
**Recomendado para**: Após entender a estrutura

Detalha como o sistema operacional é carregado:
- Etapas do boot (BIOS → GRUB → Kernel)
- Especificação Multiboot
- Organização da memória
- Formato ELF
- Seções do kernel

### 3. [Conceitos Fundamentais](03-conceitos-fundamentais.md)
**Recomendado para**: Aprofundamento teórico

Explica os conceitos essenciais de SO:
- Arquitetura x86 (registradores, modos)
- Gerenciamento de memória (pilha, heap)
- Assembly x86 (sintaxe NASM)
- Bootloaders e linkers
- Compilação freestanding
- Interrupções

### 4. [Análise Detalhada do Código](04-analise-do-codigo.md)
**Recomendado para**: Entendimento linha por linha

Análise completa de cada arquivo:
- loader.s (linha por linha)
- link.ld (cada seção)
- Makefile (cada flag e regra)
- Fluxo de execução completo

## Ordem de Leitura Recomendada

### Para Iniciantes
1. Estrutura do Projeto
2. Processo de Boot
3. Conceitos Fundamentais (seções 1-3)
4. Análise do Código (seção 1)

### Para Intermediários
1. Estrutura do Projeto (revisão rápida)
2. Processo de Boot
3. Conceitos Fundamentais (completo)
4. Análise do Código (completo)

### Para Avançados
- Use como referência conforme necessário
- Foque em Conceitos Fundamentais (seções 9-10)
- Análise do Código (seção 4 - Próximos Passos)

## Como Usar Esta Documentação

### Durante o Desenvolvimento
- Consulte **Estrutura do Projeto** ao adicionar novos arquivos
- Use **Conceitos Fundamentais** como referência técnica
- Revise **Análise do Código** ao modificar arquivos existentes

### Para Estudo
- Leia sequencialmente na primeira vez
- Faça anotações e experimente o código
- Execute o projeto após cada documento

### Para Apresentações
- **Estrutura do Projeto**: Visão geral
- **Processo de Boot**: Demonstração do funcionamento
- **Conceitos Fundamentais**: Base teórica
- **Análise do Código**: Detalhes de implementação

## Recursos Adicionais

### Dentro do Projeto
- `README.md`: Instruções de compilação e execução
- Comentários no código fonte
- Makefile comentado

### Referências Externas
- [The Little Book About OS Development](https://littleosbook.github.io/)
- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

## Contribuindo com a Documentação

Ao adicionar novas funcionalidades ao projeto:

1. **Atualize a documentação existente**:
   - Estrutura do Projeto (se adicionar arquivos)
   - Processo de Boot (se modificar o boot)
   - Conceitos Fundamentais (se usar novos conceitos)

2. **Crie novos documentos** para funcionalidades complexas:
   - Siga o padrão de numeração (05-, 06-, etc.)
   - Use Markdown para formatação
   - Inclua exemplos de código
   - Adicione diagramas quando possível

3. **Mantenha este índice atualizado**:
   - Adicione novos documentos à lista
   - Atualize recomendações de leitura

## Convenções de Documentação

### Formatação de Código
```assembly
; Assembly: comentários com ponto e vírgula
mov eax, 0x1234
```

```c
// C: comentários com barras
int main() {
    return 0;
}
```

### Termos Técnicos
- **Negrito**: Termos importantes
- `Código`: Comandos, arquivos, variáveis
- *Itálico*: Ênfase

### Estrutura de Documentos
1. Introdução
2. Seções numeradas
3. Exemplos práticos
4. Referências

## Glossário Rápido

- **BIOS**: Basic Input/Output System
- **GRUB**: Grand Unified Bootloader
- **ELF**: Executable and Linkable Format
- **BSS**: Block Started by Symbol
- **ISA**: Instruction Set Architecture
- **ESP**: Stack Pointer
- **EIP**: Instruction Pointer

## Perguntas Frequentes

**Q: Por que o kernel é carregado em 1 MB?**
A: Para evitar conflitos com BIOS e hardware mapeado nos primeiros 1 MB.

**Q: O que é Multiboot?**
A: Especificação que padroniza a interface entre bootloaders e kernels.

**Q: Por que usar Assembly?**
A: Para controle direto do hardware e implementar o ponto de entrada do kernel.

**Q: Posso usar bibliotecas do C?**
A: Não no kernel. Deve implementar tudo do zero (freestanding).

## Próximos Tópicos

À medida que o projeto evolui, novos documentos serão adicionados sobre:

- Driver de vídeo VGA
- Driver de teclado
- Gerenciamento de memória
- Processos e threads
- Sistema de arquivos
- Interface de usuário

## Feedback

Se encontrar erros ou tiver sugestões para melhorar a documentação, por favor:
- Abra uma issue no repositório
- Discuta com a equipe
- Contribua com melhorias

---

**Última atualização**: Documentação inicial do projeto
**Versão**: 1.0
