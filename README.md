# Projeto SO

Projeto da disciplina de Sistemas Operacionais — Ciência da Computação, UFPB.

Implementação de um sistema operacional x86 32-bit do zero, com base no livro *The Little Book About OS Development* de Erik Helin e Adam Renberg.

## O que está implementado

- Boot Multiboot via GRUB com kernel higher-half (`0xC0100000`)
- Paginação x86 two-level (4KB pages)
- Page Frame Allocator (bitmap)
- Heap do kernel (free-list com split e coalescing)
- GDT, IDT, PIC, PIT (100 Hz), TSS
- Driver de framebuffer VGA modo texto 80×25
- Driver de teclado PS/2 com buffer circular
- Gerenciamento de processos (PCB, tabela, context switch cooperativo)
- Scheduler round-robin cooperativo
- Mini-shell interativo com comandos: `help`, `clear`, `ps`, `top`, `info`, `spawn`, `kill`, `reboot`, `poweroff`
- Monitor de processos em tempo real (`top`) com CPU%, RAM, heap e info do CPU

## Dependências

### Ubuntu / Debian

```bash
./install_deps.sh
```

Ou manualmente:

```bash
sudo apt update
sudo apt install -y gcc gcc-multilib nasm binutils make \
    grub-pc-bin grub-common xorriso mtools bochs bochs-x
```

### macOS (Apple Silicon)

Requer Docker Desktop. Veja [docs/08-como-rodar.md](docs/08-como-rodar.md).

## Compilação e Execução

```bash
# Compilar e rodar no Bochs
make run

# Apenas compilar
make

# Gerar ISO
make os.iso

# Limpar artefatos
make clean
```

### Via Docker (macOS / qualquer plataforma)

```bash
make docker-build   # constrói a imagem (uma vez)
make docker-run     # compila + executa com QEMU headless
make docker-shell   # shell interativo no container
```

### Via VM ou Hardware Real

A ISO gerada é uma imagem híbrida bootável em qualquer ambiente:

```bash
# QEMU
qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot

# Pendrive (hardware real — BIOS legacy)
sudo dd if=os.iso of=/dev/sdX bs=4M status=progress
```

VirtualBox e VMware também funcionam sem configuração especial. Veja [docs/08-como-rodar.md](docs/08-como-rodar.md).

## Documentação

A pasta `docs/` contém a documentação técnica completa:

| # | Documento | Conteúdo |
|---|-----------|----------|
| 01 | [Estrutura do Projeto](docs/01-estrutura-do-projeto.md) | Organização de arquivos, build, layout de memória |
| 02 | [Processo de Boot](docs/02-processo-de-boot.md) | BIOS → GRUB → loader.s → kmain |
| 03 | [Drivers e Subsistemas](docs/04-drivers-e-subsistemas.md) | FB, teclado, PIT, serial, GDT, IDT, PIC |
| 04 | [Memória](docs/05-memoria.md) | Paginação, PFA, heap do kernel |
| 05 | [Processos e Scheduler](docs/06-processos-e-scheduler.md) | PCB, context switch, scheduler cooperativo |
| 06 | [Mini-Shell e Top](docs/07-mini-shell-e-top.md) | Comandos, monitor de processos, fluxo completo |
| 07 | [Como Rodar](docs/08-como-rodar.md) | Guia detalhado Linux e macOS |

## Referências

- Helin, E., & Renberg, A. **The Little Book About OS Development** — [littleosbook.github.io](https://littleosbook.github.io/)
- [OSDev Wiki](https://wiki.osdev.org/)
- [Intel® 64 and IA-32 Architectures Software Developer Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)

Contribuidores
<table>
<tr>
<td align="center" valign="top" width="25%">
<a href="https://github.com/JoaoBoscoDuarte">
<img src="https://github.com/JoaoBoscoDuarte.png" width="100"/>


<b>João Bosco Duarte</b>
</a>



GDT, PFA, shell, comandos (help, clear, ps, top, info, spawn, kill, reboot, poweroff)
</td>
<td align="center" valign="top" width="25%">
<a href="https://github.com/guilopeszw">
<img src="https://github.com/guilopeszw.png" width="100"/>


<b>Guilherme Lopes</b>
</a>



Recebimento do teclado, paginação, higher half linker, troca de contexto por timer
</td>
<td align="center" valign="top" width="25%">
<a href="https://github.com/Marcus-Vin">
<img src="https://github.com/Marcus-Vin.png" width="100"/>


<b>Marcus Vinícius</b>
</a>



User mode, GitHub CI, PCB, funções
</td>
<td align="center" valign="top" width="25%">
<a href="https://github.com/SamSantosidc">
<img src="https://github.com/SamSantosidc.png" width="100"/>


<b>Samuel Santos</b>
</a>



PFA dinâmico, IDT, Spawn de processos cooperativos, histórico de comandos, Kernel Heap
</td>
</tr>
</table>
