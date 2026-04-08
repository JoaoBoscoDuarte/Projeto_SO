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

## Rodando em Máquina Virtual (VirtualBox / VMware)

A ISO gerada pelo `make os.iso` é uma imagem híbrida padrão — qualquer virtualizador consegue bootar sem configuração especial.

### Emulação vs Virtualização

Antes de escolher a ferramenta, é útil entender a diferença:

| | Emulação (Bochs) | Virtualização (QEMU/KVM, VirtualBox, VMware) |
|---|---|---|
| Como funciona | Simula o hardware inteiro em software | Executa código diretamente na CPU do host |
| Velocidade | Lenta (cada instrução é interpretada) | Rápida (instruções rodam no hardware real) |
| Debug | Excelente (inspeciona registradores, memória) | Limitado |
| Dependência de CPU | Nenhuma — roda em qualquer arquitetura | Requer mesma arquitetura (x86) |
| Uso neste projeto | `make run` (padrão) | QEMU, VirtualBox, VMware |

### O que é KVM?

**KVM (Kernel-based Virtual Machine)** é um módulo do kernel Linux que transforma o próprio Linux em um hypervisor. Ele usa as extensões de virtualização do hardware diretamente:
- **AMD-V** (AMD Virtualization) — processadores AMD
- **Intel VT-x** (Virtualization Technology) — processadores Intel

Com KVM ativo, o QEMU consegue executar código de VM quase na velocidade nativa. O problema é que **KVM e VirtualBox não podem usar essas extensões ao mesmo tempo** — cada um precisa de acesso exclusivo ao hardware de virtualização.

### Erro: `VERR_SVM_IN_USE` (VirtualBox + AMD)

Se ao iniciar uma VM no VirtualBox aparecer:

```
AMD-V is being used by another hypervisor (VERR_SVM_IN_USE).
VirtualBox can't enable the AMD-V extension.
```

Significa que o KVM está carregado e ocupando o AMD-V. Solução: descarregar o KVM antes de usar o VirtualBox.

**Desabilitar KVM temporariamente (até o próximo reboot):**

```bash
sudo modprobe -r kvm_amd   # AMD — ou kvm_intel para Intel
sudo modprobe -r kvm
```

Agora inicie a VM no VirtualBox normalmente. Para reativar o KVM depois:

```bash
sudo modprobe kvm
sudo modprobe kvm_amd      # ou kvm_intel
```

**Desabilitar KVM permanentemente (se não usar QEMU/libvirt):**

```bash
echo 'blacklist kvm_amd' | sudo tee /etc/modprobe.d/blacklist-kvm.conf
echo 'blacklist kvm'     | sudo tee -a /etc/modprobe.d/blacklist-kvm.conf
sudo update-initramfs -u
# reinicie o sistema
```

> Se você usa Docker, libvirt ou QEMU com aceleração no dia a dia, prefira a solução temporária — desabilitar permanentemente vai deixar essas ferramentas mais lentas.

**Alternativa: usar QEMU diretamente** (sem conflito, pois ele já usa KVM nativamente):

```bash
qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot
```

### VirtualBox (sem o erro acima)

1. **Nova VM** → Nome: `projeto-so` → Tipo: `Other` → Versão: `Other/Unknown (32-bit)`
2. **Memória**: 32 MB (mínimo)
3. **Disco rígido**: pular (não é necessário)
4. Com a VM criada: **Configurações → Armazenamento → Controlador IDE**
   → clica no ícone de CD → **Escolher arquivo de disco** → seleciona `os.iso`
5. **Iniciar** — o shell `kernel>` aparece na janela da VM

### VMware Workstation / Fusion

1. **Create a New Virtual Machine** → **I will install the operating system later**
2. Guest OS: **Other** → **Other**
3. Após criar: **VM Settings → CD/DVD → Use ISO image file** → seleciona `os.iso`
4. **Power On**

### QEMU (linha de comando)

```bash
# Com janela gráfica
qemu-system-i386 -cdrom os.iso -no-reboot

# Headless — saída serial no terminal
qemu-system-i386 -cdrom os.iso -display none -serial stdio -no-reboot
```

---

## Rodando em Hardware Real (pendrive)

A ISO também é bootável em hardware físico com BIOS legacy (não UEFI puro).

```bash
# Substitua /dev/sdX pelo dispositivo do pendrive (verifique com lsblk)
sudo dd if=os.iso of=/dev/sdX bs=4M status=progress
sync
```

> **Atenção:** o `dd` apaga tudo no dispositivo de destino. Confirme o dispositivo correto antes de executar.

No boot da máquina, entre na BIOS/UEFI e selecione o pendrive como dispositivo de boot. O kernel requer apenas suporte a **BIOS legacy** e **modo VGA texto** — presente em praticamente qualquer hardware x86 fabricado nos últimos 20 anos.

---

## Solução de Problemas

### Linux

| Erro | Solução |
|------|---------|
| `grub-mkrescue: command not found` | `sudo apt install grub-pc-bin grub-common xorriso` |
| `make run` falha com erro do Bochs | Use QEMU: `qemu-system-i386 -cdrom os.iso -serial stdio -no-reboot` |
| Tela preta no Bochs | Aguarde 2 segundos. Verifique se `os.iso` foi gerado: `ls -lh os.iso` |
| Erro de compilação 32-bit | `sudo apt install gcc-multilib` |
| `VERR_SVM_IN_USE` no VirtualBox | KVM em conflito — rode `sudo modprobe -r kvm_amd kvm` antes de abrir o VirtualBox |

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
