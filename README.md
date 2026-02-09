# Projeto SO

Projeto da disciplina de Sistemas Operacionais do curso de Ciência da Computação da UFPB com o objetivo de desenvolver um Sistema Operacional do zero.

## Sumário

1. [Sobre o Projeto](#sobre-o-projeto)
2. [Dependências](#dependências)
3. [Compilação e Execução](#compilação-e-execução)
4. [Documentação](#documentação)
5. [Contribuidores](#contribuidores)

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

## Contribuidores

<table>
  <tr>
    <td align="center">
      <a href="https://github.com/JoaoBoscoDuarte">
        <img src="https://github.com/JoaoBoscoDuarte.png" width="100px;" alt="João Bosco Duarte"/><br />
        <sub><b>João Bosco Duarte</b></sub>
      </a><br />
      <details>
        <summary>Contribuições</summary>
        <i>Adicione aqui as contribuições</i>
      </details>
    </td>
    <td align="center">
      <a href="https://github.com/guilopeszw">
        <img src="https://github.com/guilopeszw.png" width="100px;" alt="Guilherme Lopes"/><br />
        <sub><b>Guilherme Lopes</b></sub>
      </a><br />
      <details>
        <summary>Contribuições</summary>
        <i>Adicione aqui as contribuições</i>
      </details>
    </td>
    <td align="center">
      <a href="https://github.com/Marcus-Vin">
        <img src="https://github.com/Marcus-Vin.png" width="100px;" alt="Marcus Vinícius"/><br />
        <sub><b>Marcus Vinícius</b></sub>
      </a><br />
      <details>
        <summary>Contribuições</summary>
        <i>Adicione aqui as contribuições</i>
      </details>
    </td>
    <td align="center">
      <a href="https://github.com/SamSantosidc">
        <img src="https://github.com/SamSantosidc.png" width="100px;" alt="Samuel Santos"/><br />
        <sub><b>Samuel Santos</b></sub>
      </a><br />
      <details>
        <summary>Contribuições</summary>
        <i>Adicione aqui as contribuições</i>
      </details>
    </td>
  </tr>
</table>
