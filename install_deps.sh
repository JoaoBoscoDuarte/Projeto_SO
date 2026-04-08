#!/bin/bash
set -e

# =============================================================================
# install_deps.sh — Instala dependências do Projeto SO
# Suporte: Ubuntu / Debian (apt)
# =============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()    { echo -e "${GREEN}[+]${NC} $1"; }
warn()    { echo -e "${YELLOW}[!]${NC} $1"; }
error()   { echo -e "${RED}[x]${NC} $1"; exit 1; }

# Verifica se está rodando como root ou com sudo disponível
check_sudo() {
    if [ "$EUID" -eq 0 ]; then
        SUDO=""
    elif command -v sudo &>/dev/null; then
        SUDO="sudo"
    else
        error "Este script precisa de privilégios de root. Instale sudo ou rode como root."
    fi
}

# Detecta o gerenciador de pacotes
detect_distro() {
    if command -v apt-get &>/dev/null; then
        DISTRO="debian"
    else
        error "Distribuição não suportada. Este script requer apt (Ubuntu/Debian)."
    fi
}

install_debian() {
    info "Atualizando lista de pacotes..."
    $SUDO apt-get update -qq

    PACKAGES=(
        # Compilação
        gcc
        gcc-multilib       # suporte a -m32 em sistemas 64-bit
        binutils           # ld, objdump, etc.
        nasm               # assembler x86
        make

        # Geração de ISO bootável
        grub-pc-bin        # grub-mkrescue
        grub-common
        xorriso            # backend ISO
        mtools             # manipulação FAT (requerido pelo grub-mkrescue)

        # Emulação
        bochs
        bochs-x            # interface gráfica do Bochs
    )

    info "Instalando pacotes: ${PACKAGES[*]}"
    $SUDO apt-get install -y "${PACKAGES[@]}"
}

verify_tools() {
    info "Verificando ferramentas instaladas..."
    local ok=1
    for tool in gcc nasm ld make grub-mkrescue xorriso bochs; do
        if command -v "$tool" &>/dev/null; then
            echo "  ✓ $tool ($(command -v $tool))"
        else
            warn "  ✗ $tool não encontrado"
            ok=0
        fi
    done

    # Verifica suporte a 32-bit
    if echo 'int main(){}' | gcc -m32 -x c - -o /dev/null 2>/dev/null; then
        echo "  ✓ gcc -m32 (compilação 32-bit OK)"
    else
        warn "  ✗ gcc -m32 falhou — instale gcc-multilib"
        ok=0
    fi

    return $ok
}

main() {
    echo "============================================"
    echo "  Projeto SO — Instalação de Dependências  "
    echo "============================================"
    echo ""

    check_sudo
    detect_distro

    case "$DISTRO" in
        debian) install_debian ;;
    esac

    echo ""
    verify_tools
    echo ""
    info "Instalação concluída!"
    echo ""
    echo "  Para compilar e executar:"
    echo "    make run"
    echo ""
    echo "  Para mais informações:"
    echo "    cat docs/08-como-rodar.md"
}

main "$@"
