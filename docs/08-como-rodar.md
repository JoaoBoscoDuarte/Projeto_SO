# 08 — Como Rodar o Projeto

> Guia detalhado de configuração e execução.
> Para o caminho rápido, veja o [README principal](../README.md).

---

## Linux (Ubuntu / Debian)

### 1. Instalar dependências

```bash
./install_deps.sh
```

O script detecta o sistema e instala tudo automaticamente. Alternativamente:

```bash
sudo apt update
sudo apt install -y gcc gcc-multilib nasm binutils make \
    grub-pc-bin grub-common xorriso mtools bochs bochs-x
```

Pacotes e suas funções:

| Pacote | Função |
|--------|--------|
| `gcc`, `gcc-multilib` | Compilador C com suporte a 32 bits |
| `nasm` | Assembler para código x86 |
| `binutils` | Linker (`ld`) e utilitários |
| `make` | Automação de build |
| `grub-pc-bin`, `grub-common` | `grub-mkrescue` para gerar ISO bootável |
| `xorriso` | Backend de criação de ISO |
| `mtools` | Manipulação de imagens FAT (usado pelo grub-mkrescue) |
| `bochs`, `bochs-x` | Emulador x86 com interface gráfica |

### 2. Compilar e executar

```bash
# Compila o kernel e gera a ISO
make clean && make os.iso

# Executa no Bochs (abre janela gráfica)
make run
```

A saída serial é salva em `com1.out`:

```bash
cat com1.out
```

### 3. Alternativa com QEMU

Se preferir QEMU (saída serial direto no terminal):

```bash
sudo apt install -y qemu-system-x86
qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot
```

### 4. Encerrar

- **Bochs**: fechar a janela ou `Ctrl+C` no terminal
- **QEMU**: `Ctrl+C` no terminal

---

## macOS Apple Silicon (M1/M2/M3/M4)

O GCC e o NASM para x86 32-bit não rodam nativamente em ARM. A solução é usar Docker com uma imagem Debian amd64 e QEMU para emulação.

### Pré-requisitos

1. **Docker Desktop** instalado e em execução (ícone estável na barra de menu)
2. **VNC Viewer** para ver o framebuffer (opcional):
   ```bash
   brew install --cask vnc-viewer
   ```

### Passo a passo

#### 1. Construir a imagem Docker (uma vez)

```bash
make docker-build
```

Isso cria uma imagem Debian amd64 com gcc, nasm, grub-mkrescue e qemu instalados.

#### 2. Compilar e rodar (headless)

```bash
make docker-run
```

A saída serial aparece diretamente no terminal. O kernel está rodando quando aparecer:

```
[INFO] Sistema pronto. Iniciando shell...
```

#### 3. Ver o framebuffer via VNC

Para ver a tela gráfica do kernel, rode com VNC habilitado:

```bash
make docker-shell
# dentro do container:
make clean && make os.iso && \
  qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot -vnc :0
```

Em outro terminal no Mac:

```bash
open -a "VNC Viewer"
# endereço: localhost:5900  (senha: em branco)
```

#### 4. Shell interativo no container

```bash
make docker-shell
# prompt: root@xxxxxxxxx:/os#
make clean && make os.iso
qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot
```

---

## Solução de Problemas

### Linux

| Erro | Solução |
|------|---------|
| `grub-mkrescue: command not found` | `sudo apt install grub-pc-bin grub-common xorriso` |
| `make run` falha com erro do Bochs | Use QEMU: `qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot` |
| Tela preta no Bochs | Aguarde 2 segundos. Verifique se `os.iso` foi gerado: `ls -lh os.iso` |
| Erro de compilação 32-bit | `sudo apt install gcc-multilib` |

### macOS

| Erro | Solução |
|------|---------|
| Docker não conecta | Abrir Docker Desktop pelo Launchpad e aguardar ícone estável |
| `make docker-build` falha | Verificar conexão com internet (baixa imagem Debian ~200MB) |
| VNC não conecta | Confirmar que QEMU ainda está rodando no container |
| Tela preta no VNC | Normal — o framebuffer é preto com texto branco |

---

## Verificando o funcionamento

Após o boot, o shell exibe `kernel>`. Comandos para verificar cada subsistema:

```
kernel> info          # uptime do sistema
kernel> ps            # lista processos (deve mostrar PID 0 = kernel)
kernel> spawn teste   # cria processo de teste
kernel> top           # monitor em tempo real (sair com 'q')
kernel> kill 1        # mata o processo 1
kernel> poweroff      # desliga o emulador
```

A saída serial (`com1.out` no Bochs) contém logs de debug com endereços de memória, alocações de frames e trocas de contexto.
