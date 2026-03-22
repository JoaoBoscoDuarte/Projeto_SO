# =============================================================================
# Dockerfile — Ambiente de desenvolvimento para o kernel x86 32-bit
#
# USO (macOS Apple Silicon):
#   docker build --platform linux/amd64 -t osdev-kernel .
#   docker run --rm -it --platform linux/amd64 -v "$(pwd)":/os -w /os osdev-kernel bash
#
# Ou simplesmente:
#   make docker-run
# =============================================================================

FROM --platform=linux/amd64 debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    # Compilador C 32-bit
    gcc \
    gcc-multilib \
    binutils \
    # Assembler
    nasm \
    # Build system
    make \
    # GRUB para gerar ISO bootável
    grub-pc-bin \
    grub-common \
    xorriso \
    mtools \
    # QEMU para emular x86
    qemu-system-x86 \
    # Utilitários de debug
    file \
    && rm -rf /var/lib/apt/lists/*

# Verifica que as ferramentas essenciais estão disponíveis
RUN gcc --version && nasm --version && qemu-system-i386 --version

WORKDIR /os