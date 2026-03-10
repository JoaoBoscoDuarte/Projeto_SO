# Criando uma Máquina Virtual

## Visão Geral

Este documento descreve como criar e configurar uma máquina virtual para executar o projeto. Por utilizar ferramentas específicas do Linux (`nasm`, `bochs`, `genisoimage`, `grub-legacy`), o projeto precisa rodar em um ambiente Debian/Ubuntu.

> **Nota sobre o Debian 13 "Trixie"**: Dependendo do momento em que este guia for seguido, o Debian 13 pode ainda estar na fase *testing* (candidato a estável). As imagens de instalação estão disponíveis no site oficial do Debian. Se preferir um ambiente 100% estável, utilize o Debian 12 "Bookworm" — o processo é idêntico.

---

## 1. Windows (VirtualBox)

### 1.1. Pré-requisitos

Faça o download dos seguintes itens ante


| Item                           | Link                                                                                                                            |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------- |
| VirtualBox (última versão)     | [https://www.virtualbox.org/wiki/Downloads](https://www.virtualbox.org/wiki/Downloads)                                          |
| Debian 13 "Trixie" ISO (amd64) | [https://cdimage.debian.org/cdimage/weekly-builds/amd64/iso-cd](https://cdimage.debian.org/cdimage/weekly-builds/amd64/iso-cd/) |


- Baixe o arquivo `VirtualBox-x.x.x-Win.exe` para Windows.
- Baixe o arquivo `debian-testing-amd64-netinst.iso` ou a ISO completa.

### 1.2. Instalando o VirtualBox

1. Execute o instalador `VirtualBox-x.x.x-Win.exe`.
2. Siga as etapas padrão do assistente de instalação (Next → Next → Install).
3. Aceite a instalação dos drivers de rede quando solicitado.
4. Ao finalizar, abra o **Oracle VirtualBox Manager**.

### 1.3. Criando a Máquina Virtual

1. Clique em **Novo** (ícone de estrela azul).
2. Preencha as configurações iniciais:
  - **Nome**: `Debian13`
  - **Pasta**: diretório de sua preferência
  - **Imagem ISO**: selecione o arquivo `.iso` baixado
  - Marque **"Pular instalação sem supervisão"** para instalar manualmente
3. Clique em **Próximo**.

#### Memória e CPU


| Configuração  | Valor recomendado |
| ------------- | ----------------- |
| Memória RAM   | 2048 MB (2 GB)    |
| Processadores | 2                 |


1. Clique em **Próximo**.

#### Disco Rígido Virtual

- Selecione **"Criar um disco rígido virtual agora"**
- **Tamanho**: `20 GB`
- Tipo: **VDI (VirtualBox Disk Image)**
- Alocação: **Dinamicamente alocado** (economiza espaço em disco real)

1. Clique em **Próximo** e depois em **Finalizar**.

### 1.4. Ajustes Adicionais (Opcional)

Antes de iniciar, acesse **Configurações → Sistema → Processador** e habilite:

- **PAE/NX** (para melhor compatibilidade)

Em **Configurações → Exibição**:

- **Memória de Vídeo**: aumente para `64 MB`

### 1.5. Instalando o Debian na VM

1. Com a VM selecionada, clique em **Iniciar**.
2. O sistema irá inicializar a partir da ISO do Debian.

#### Etapas de Instalação

**a) Menu inicial**

- Selecione **"Graphical install"** (ou `Install` para modo texto).

**b) Idioma e localização**

- Idioma: `Portuguese (Brazil)` (ou `English`)
- País: `Brazil`
- Layout de teclado: `Brazilian (ABNT2)` ou `US`

**c) Configuração de rede**

- **Hostname**: escolha um nome (ex: `debian-so`)
- **Domain name**: deixe em branco

**d) Usuário e senha**

- Defina uma senha para o **root**
- Crie um **usuário comum** com nome e senha

**e) Particionamento**

- Selecione **"Guiado – usar o disco inteiro"**
- Confirme o disco virtual apresentado
- Esquema: **"Todos os arquivos em uma partição"**
- Clique em **"Finalizar o particionamento e escrever mudanças no disco"** → **Sim**

**f) Gerenciador de pacotes**

- País do mirror: `Brazil` → `deb.debian.org`
- Proxy: deixe em branco (a não ser que sua rede exija)

**g) Seleção de software**

- Mantenha marcado: `standard system utilities`
- Marque `Debian desktop environment` se quiser interface gráfica
> **Nota**: Se você não quiser interface gráfica, a única coisa disponível para você será um terminal.

**h) Bootloader GRUB**

- Selecione **"Sim"** para instalar o GRUB
- Escolha o disco: `/dev/sda`

**i) Finalização**

- Clique em **"Continuar"** para reiniciar
- A VM irá remover a ISO automaticamente e inicializar no Debian instalado

### 1.6. Configurando o Ambiente do Projeto

Após o login no Debian recém-instalado:

#### Clonar o repositório e executar

```bash
git clone <url-do-repositorio>
cd Projeto_SO
bash install_deps.sh
make run
```

Lembre-se de configurar seu git! Recomendo usar chaves SSH.

#### Instalar as dependências

```bash
sudo apt update
sudo apt install -y nasm binutils gcc make grub-legacy genisoimage bochs bochs-x xorriso
```

#### Copiar o stage2_eltorito do GRUB

```bash
sudo cp /usr/lib/grub/i386-pc/stage2_eltorito /caminho/para/o/projeto/iso/boot/grub/
```

> Se usar o script `install_deps.sh` incluso no projeto, ele realiza este passo automaticamente. 
---

## 2. macOS (UTM)

O **UTM** é um software de virtualização gratuito e open-source baseado em QEMU, compatível com Macs Intel e Apple Silicon (M1/M2/M3/M4).

> **Apple Silicon (M1/M2/M3/M4)**: como o projeto é construído para arquitetura x86, o UTM irá **emular** um processador x86_64 via QEMU. Isso é mais lento do que a virtualização nativa, mas funciona corretamente para o propósito do projeto.

### 2.1. Pré-requisitos


| Item                           | Link                                                                                                                             |
| ------------------------------ | -------------------------------------------------------------------------------------------------------------------------------- |
| UTM (versão gratuita)          | [https://mac.getutm.app/](https://mac.getutm.app/)                                                                               |
| Debian 13 "Trixie" ISO (amd64) | [https://cdimage.debian.org/cdimage/weekly-builds/amd64/iso-cd/](https://cdimage.debian.org/cdimage/weekly-builds/amd64/iso-cd/) |


- Baixe o arquivo `UTM.dmg` pelo site oficial (gratuito).
  - O UTM também está disponível na Mac App Store por um valor simbólico para apoiar o desenvolvimento — funcionalidade idêntica.
- Baixe o arquivo `debian-testing-amd64-netinst.iso`.

### 2.2. Instalando o UTM

1. Abra o arquivo `UTM.dmg`.
2. Arraste o ícone do **UTM** para a pasta **Aplicativos**.
3. Abra o UTM a partir do Launchpad ou Spotlight.
4. Se aparecer aviso de segurança, vá em **Preferências do Sistema → Privacidade e Segurança** e clique em **"Abrir Mesmo Assim"**.

### 2.3. Criando a Máquina Virtual

1. Na tela inicial do UTM, clique em **"Criar nova máquina virtual"**.
2. Selecione **"Emulate"** (necessário para emular x86_64, independente do Mac).

#### Selecionar sistema operacional

1. Escolha **"Other"** na lista de sistemas operacionais.

> Você também pode escolher **Linux** e depois **"Other"** se nenhuma versão do Debian aparecer listada.

#### Configuração de hardware

1. Em **"Architecture"**, selecione **x86_64**.
2. Em **"System"**, selecione **Standard PC (Q35 + ICH9, 2009)** ou **PC (i440FX)**.
3. Defina a memória RAM:


| Configuração   | Valor recomendado |
| -------------- | ----------------- |
| Memória RAM    | 2048 MB (2 GB)    |
| Núcleos de CPU | 2                 |


1. Clique em **"Próximo"**.

#### Armazenamento

1. Em **"Storage"**, defina o tamanho do disco: `20 GB`.
2. Clique em **"Próximo"**.

#### ISO de boot

1. Em **"Boot ISO Image"**, clique em **"Browse"** e selecione o arquivo `.iso` do Debian baixado.
2. Clique em **"Próximo"**.

#### Nome e finalização

1. Dê um nome à VM (ex: `Debian13-SO`).
2. Clique em **"Save"** para criar a VM.

### 2.4. Instalando o Debian na VM

1. Selecione a VM criada e clique no botão **▶ Play** para iniciá-la.
2. A VM irá inicializar a partir da ISO do Debian.

#### Etapas de Instalação

Siga os mesmos passos descritos na **Seção 1.5** (Windows), pois o processo de instalação do Debian é idêntico independente do software de virtualização:

- **a)** Menu inicial → `Graphical install`
- **b)** Idioma, país e teclado
- **c)** Hostname e domínio
- **d)** Usuário e senha
- **e)** Particionamento (disco inteiro, partição única)
- **f)** Gerenciador de pacotes (mirror)
- **g)** Seleção de software
- **h)** Instalação do GRUB em `/dev/sda` (ou `/dev/vda` dependendo do controlador virtual)
- **i)** Reinicialização

> Após o reboot, acesse **CD/DVD** nas configurações da VM no UTM e remova a ISO para garantir que o sistema inicie pelo disco instalado.

### 2.5. Configurando o Ambiente do Projeto

Após o login no Debian:

#### Instalar as dependências

```bash
sudo apt update
sudo apt install -y nasm binutils gcc make grub-legacy genisoimage bochs bochs-x xorriso
```

#### Clonar o repositório e executar

```bash
git clone <url-do-repositorio>
cd Projeto_SO
bash install_deps.sh
make run
```

---

## Verificação Final

Após executar `make run`, o emulador **Bochs** deve iniciar dentro da VM e carregar o kernel do projeto. Se a janela do Bochs abrir e exibir a saída do kernel (texto na tela ou cursor piscando), o ambiente está configurado corretamente.

Em caso de erros com `bochs-x` (interface gráfica do Bochs), certifique-se de que o ambiente gráfico está ativo ou utilize `bochs` sem interface (`bochs -q`).
texto na tela ou cursor piscando), o ambiente está configurado corretamente.
