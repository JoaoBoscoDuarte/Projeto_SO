#!/bin/bash
# Script para instalar dependências do projeto

echo "Instalando dependências..."
sudo apt install -y genisoimage bochs bochs-x nasm binutils gcc make xorriso mtools

echo ""
echo "Dependências instaladas com sucesso!"
echo "Execute: make run"
