# Projeto SO

Projeto da disciplina de Sistemas Operacionais do curso de Ciência da Computação da UFPB com o objetivo de desenvolver um Sistema Operacional do zero.

## Sumário

1. [Sobre o Projeto](#sobre-o-projeto)
2. [Dependências](#dependências)
3. [Compilação e Execução](#compilação-e-execução)
4. [Documentação](#documentação)
5. [Referências](#referências)
6. [Contribuidores](#contribuidores)

## Sobre o Projeto

Este projeto utiliza como principal referência o livro "The Little Book About OS Development" de Erik Helin e Adam Renberg, implementando conceitos fundamentais de sistemas operacionais.

## Dependências

### Instalação

Para instalar todas as dependências necessárias no Ubuntu/Debian:

```bash
sudo apt install nasm binutils gcc make grub-legacy genisoimage bochs bochs-x
```

#### Ferramentas de Compilação

- `nasm` - Assembler para código x86
- `ld` (binutils) - Linker
- `gcc` - Compilador C
- `make` - Automação de build

#### Ferramentas de Boot e Emulação

- `grub-legacy` (stage2_eltorito) - Bootloader
- `genisoimage` - Criação de imagem ISO
- `bochs` - Emulador x86

## Compilação e Execução

Certifique-se de estar na pasta principal do projeto e execute:

```bash
make run
```

Este comando irá compilar o sistema operacional e executá-lo automaticamente no emulador Bochs.

## Documentação

Além do código fonte, a pasta `docs/` contém documentos em Markdown com:

- Explicações detalhadas sobre a implementação
- Conceitos fundamentais de sistemas operacionais
- Guias de desenvolvimento

## Referências

- Helin, E., & Renberg, A. **The Little Book About OS Development**. Disponível em: [https://littleosbook.github.io/](https://littleosbook.github.io/)
- [OSDev Wiki](https://wiki.osdev.org/) - Recursos sobre desenvolvimento de sistemas operacionais
- [Intel® 64 and IA-32 Architectures Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

## Contribuidores

<table>
  <tr>
    <td align="center">
      <a href="https://github.com/JoaoBoscoDuarte">
        <img src="https://github.com/JoaoBoscoDuarte.png" width="100"/><br>
        <b>João Bosco Duarte</b>
      </a>
      <p>
      -
      </p>
    </td>
    <td align="center">
      <a href="https://github.com/guilopeszw">
        <img src="https://github.com/guilopeszw.png" width="100"/><br>
        <b>Guilherme Lopes</b>
      </a>
      <p>
      - 
      </p>
    </td>
    <td align="center">
      <a href="https://github.com/Marcus-Vin">
        <img src="https://github.com/Marcus-Vin.png" width="100"/><br>
        <b>Marcus Vinícius</b>
      </a>
      <p>
        -
      </p>
    </td>
    <td align="center">
      <a href="https://github.com/SamSantosidc">
        <img src="https://github.com/SamSantosidc.png" width="100"/><br>
        <b>Samuel Santos</b>
      </a>
      <p>
      -
      </p>
    </td>
  </tr>
</table>
