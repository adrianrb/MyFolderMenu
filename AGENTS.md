# AGENTS.md — MyFolderMenu (C++/Win32)

## Status

- Versão **1.0.0**, funcional e testada.
- Repositório independente da versão C# original; a árvore `cpp/` do repositório
  C# antigo foi substituída por este projeto.
- Branch principal: `master`. Builds gerados (`build*/`, `release/`) são ignorados.

## Visão geral

Aplicativo de bandeja nativo do Windows, escrito em C++17 com Win32 puro, sem
dependências externas. Reproduz o comportamento da versão C# WinForms:
menu de pastas, submenus lazy, temas claro/escuro, configuração INI, migração do
`folders.json`, autostart e diálogos de Configurações/Sobre.

## Layout

- `src/main.cpp` — entrada, parsing de linha de comando, loop de mensagens.
- `src/core/` — lógica sem UI: config, paths, strings, tema, startup, ícones, versão.
- `src/app/` — estado da aplicação, fontes, bandeja, modelo/desenho de menu e ações.
- `src/ui/` — diálogos e helpers de tema/desenho comuns.
- `src/resources/` — ícone, manifesto, `resource.h` e `MyFolderMenu.rc.in`
  (versão vem de `version.txt` via CMake).
- `tests/` — 74 testes unitários (484 verificações) + 16 testes de integração GUI.
- `cmake/toolchain-mingw-w64.cmake` — cross-compile a partir do WSL/Linux.
- `tools/package_release.ps1` — ZIP de release e SHA-256.

## Build

| Ambiente | Comando | Saída |
| --- | --- | --- |
| Windows/MSVC | `build_msvc.bat` | `build-msvc\MyFolderMenu.exe` |
| Windows/MinGW | `build_mingw.bat` | `build-mingw\MyFolderMenu.exe` |
| WSL/Linux | `./build.sh` | `build-mingw/MyFolderMenu.exe` |

- `version.txt` é a fonte única da versão (X.Y.Z). Alterar exige rebuild/reconfigure.
- MSVC usa `/MT` + `/W4 /permissive- /utf-8`; MinGW usa link estático.
- Não introduza dependências de terceiros nem runtime .NET.

## Testes

```powershell
tests\run_tests.ps1 -BuildDir build-msvc
```

- Unitários: `build-*/mfm_tests.exe` (também via `ctest`).
- Integração: `tests\integration.ps1` (PowerShell 5.1+, janelas reais).
- `MFM_APPDATA` redireciona toda a configuração para pasta temporária — nunca
  use o perfil real nos testes.
- Os testes de integração exigem sessão interativa do Windows (bandeja/GUI).
- Scripts `.ps1` com acentos precisam ser gravados em **UTF-8 com BOM**.

No WSL: `tests/run_tests.sh` (compila MinGW e roda as duas suítes).

## Convenções

- Código e comentários em inglês; UI e mensagens em português.
- `clang-format` não é obrigatório; mantenha o estilo existente (4 espaços,
  nomes em PascalCase para funções, `mfm` namespace).
- Toda configuração passa por `core/config`; não acesse `%APPDATA%` diretamente.
- Mantenha a paridade comportamental com a versão C# documentada no README.

## Release

1. Atualizar `version.txt` (e `project(... VERSION ...)` se necessário).
2. `build_msvc.bat`
3. `tests\run_tests.ps1 -BuildDir build-msvc`
4. `tools\package_release.ps1 -BuildDir build-msvc`
5. Publicar `release\MyFolderMenu-1.0.0-win64.zip` + `SHA256SUMS.txt`.
