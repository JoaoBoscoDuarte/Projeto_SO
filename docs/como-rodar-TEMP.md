# Como Rodar o Projeto

> Guia rápido para apresentar o SO ao professor.
> Tempo estimado: 2 minutos no Linux, 5 minutos no macOS (após Docker instalado).

---

## 🐧 Linux (Debian/Ubuntu) — Sem Docker

### Pré-requisitos

Instalar as dependências uma vez:

```bash
sudo apt update
sudo apt install -y gcc nasm make grub-pc-bin grub-common xorriso mtools qemu-system-x86
```

---

### Passo a Passo

#### 1. Entrar na pasta do projeto

```bash
cd caminho/para/Projeto_SO
```

#### 2. Compilar e gerar a ISO

```bash
make clean && make os.iso
```

#### 3. Rodar com Bochs (padrão do projeto)

```bash
make run
```

A janela do Bochs abre com o framebuffer do kernel. A saída serial é salva em `com1.out`:

```bash
cat com1.out
```

#### 3. Alternativa — rodar com QEMU (exibe serial no terminal)

```bash
qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot
```

A saída serial aparece diretamente no terminal:

```
[INFO] Sistema Operacional Iniciado
kernel virtual:  0xC0100000 - 0xC010A000
kernel physical: 0x100000 - 0x10A000
Tamanho do kernel: 40 KB
frame1: 0x10B000
frame2: 0x10C000
frame3: 0x10D000
frame4 (reuso f1): 0x10B000
temp_map[0]: 0xDEADBEEF
temp_map[1]: 0xC0FFEE00
Modulo em phys: 0x10A000
```

#### 4. Para encerrar

- **Bochs**: fechar a janela ou pressionar `Ctrl + C` no terminal
- **QEMU**: pressionar `Ctrl + C` no terminal

---

---

## 🍎 macOS Apple Silicon (M1/M2/M3/M4) — Com Docker

### Pré-requisitos

- **Docker Desktop** instalado e **rodando** (ícone na barra superior do Mac)
- **RealVNC Viewer** instalado:
  ```bash
  brew install --cask vnc-viewer
  ```
- Estar na pasta raiz do projeto (`Projeto_SO/`)

---

### Passo a Passo

#### 1. Entrar na pasta do projeto

```bash
cd caminho/para/Projeto_SO
```

#### 2. Construir a imagem Docker (só precisa fazer uma vez)

```bash
make docker-build
```

Aguarda terminar. Aparece `FINISHED` quando pronto.

#### 3. Abrir o shell do container

```bash
make docker-shell
```

Um novo prompt aparece dentro do container:
```
root@xxxxxxxxx:/os#
```

#### 4. Compilar e rodar o kernel com QEMU

**Dentro do container**, rodar:

```bash
make clean && make os.iso && qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot -vnc :0
```

Aguarda o QEMU iniciar. A saída serial aparece no terminal:

```
[INFO] Sistema Operacional Iniciado
kernel virtual:  0xC0100000 - 0xC010A000
kernel physical: 0x100000 - 0x10A000
Tamanho do kernel: 40 KB
frame1: 0x10B000
frame2: 0x10C000
frame3: 0x10D000
frame4 (reuso f1): 0x10B000
temp_map[0]: 0xDEADBEEF
temp_map[1]: 0xC0FFEE00
Modulo em phys: 0x10A000
```

O QEMU fica rodando (não fecha — isso é correto).

#### 5. Ver o framebuffer (tela do kernel)

**Abrir um segundo terminal no Mac** (fora do Docker) e rodar:

```bash
open -a "VNC Viewer"
```

No VNC Viewer que abrir, digitar no campo de endereço:

```
localhost:5900
```

Pressionar **Connect**. Deixar a senha em branco. A tela do kernel aparece.

#### 6. Para encerrar

No terminal do container, pressionar `Ctrl + C` para parar o QEMU.

Para sair do container:
```bash
exit
```

---

---

## O que mostrar ao professor

| O que | Onde aparece |
|---|---|
| Boot + higher-half funcionando | Terminal (serial) |
| Endereços virtuais `0xC01xxxxx` | Terminal — linha `kernel virtual` |
| PFA alocando frames acima do kernel | Terminal — `frame1`, `frame2`, `frame3` |
| `pfa_free` + reuso de frame | Terminal — `frame4 (reuso f1)` |
| `temp_map` escrevendo em frame físico | Terminal — `0xDEADBEEF` / `0xC0FFEE00` |
| Framebuffer VGA funcionando | Janela do Bochs (Linux) ou VNC Viewer (Mac) |
| Módulo GRUB executando | Terminal — `Modulo em phys: 0x10A000` |

---

## Se algo der errado

### Linux

**`grub-mkrescue: command not found`**
→ `sudo apt install grub-pc-bin grub-common xorriso`

**Bochs abre mas tela preta**
→ Normal — aguarda 2 segundos. Se persistir, verificar se `os.iso` foi gerado com `ls -lh os.iso`.

**`make run` falha com erro do Bochs**
→ Usar QEMU como alternativa: `qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot`

### macOS

**Docker não abre / "Cannot connect"**
→ Abrir o Docker Desktop pelo Launchpad e aguardar o ícone ficar estável.

**`make docker-build` falha**
→ Verificar conexão com internet (precisa baixar a imagem Debian).

**VNC Viewer não conecta**
→ Confirmar que o QEMU ainda está rodando no container (não pode ter fechado).
→ Confirmar que usou `make docker-shell` e não `make docker-iso` (precisa da porta `-p 5900:5900`).

**Tela preta no VNC**
→ Normal — o framebuffer é preto com texto. Role para cima se necessário.