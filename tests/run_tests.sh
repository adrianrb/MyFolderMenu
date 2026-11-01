#!/usr/bin/env bash
# Executa a suite completa no WSL: build MinGW + testes unitarios + integracao.
set -euo pipefail

cd "$(dirname "$0")/.."

cmake --build build-mingw -j "${JOBS:-$(nproc)}"

echo
echo "== Testes unitarios =="
./build-mingw/mfm_tests.exe

echo
echo "== Testes de integracao =="
win_exe="$(wslpath -w "$(pwd)/build-mingw/MyFolderMenu.exe")"
win_script="$(wslpath -w "$(pwd)/tests/integration.ps1")"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$win_script" -Exe "$win_exe"
