#!/bin/bash
# Script para instalar dependências do projeto

echo "Instalando dependências..."
sudo apt install -y grub-legacy genisoimage bochs bochs-x nasm binutils gcc make xorriso

echo ""
echo "Copiando stage2_eltorito do GRUB..."
sudo cp /usr/lib/grub/i386-pc/stage2_eltorito iso/boot/grub/

echo ""
echo "Dependências instaladas com sucesso!"
echo "Execute: make run"
