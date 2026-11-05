# MyFolderMenu — versão C++ nativa (Win32)

[![English](https://img.shields.io/badge/lang-English-blue.svg)](README.md)
[![Português](https://img.shields.io/badge/lang-Português-green.svg)](README.pt.md)

Porta nativa em C++ da versão C# WinForms do MyFolderMenu. Usa apenas a API
Win32 e a biblioteca padrão do C++; **nenhuma dependência externa** e nenhum
runtime adicional (não precisa de .NET instalado).

## Funcionalidades

- Ícone na bandeja do sistema, sem janela principal e sem entrada na barra de tarefas.
- Menu com as pastas configuradas; submenus carregados sob demanda (lazy).
- Pastas primeiro, arquivos depois, ambos em ordem alfabética.
- Ícones reais do shell para pastas e arquivos.
- Oculta arquivos/pastas com atributos *hidden* e *system*.
- Abre arquivos com o aplicativo padrão do Windows (`ShellExecute`).
- Temas Claro, Escuro e Sistema (segue `AppsUseLightTheme`), incluindo barra de título escura.
- Opção de ocultar extensões de arquivo no menu.
- Configuração persistida em INI em `%APPDATA%\MyFolderMenu\folders.ini`.
- Migração automática do antigo `folders.json`.
- Inicialização com o Windows via `HKCU\...\CurrentVersion\Run`.
- Diálogos de Configurações e Sobre, com exportação/importação de configurações.
- Instância única via mutex `Local\MyFolderMenu`.
- DPI: PerMonitorV2 declarado no manifesto (layout dimensionado pela DPI do sistema).

## Estrutura do projeto

```text
MyFolderMenu/
├── CMakeLists.txt              # build, testes, install e CPack
├── version.txt                 # fonte única da versão (1.0.0)
├── build_msvc.bat              # build x64 com MSVC (CRT estática)
├── build_mingw.bat             # build x64 com MinGW-w64 (link estático)
├── build.sh                    # cross-compile MinGW a partir do WSL/Linux
├── cmake/
│   └── toolchain-mingw-w64.cmake
├── src/
│   ├── main.cpp                # entrada, linha de comando e loop de mensagens
│   ├── core/                   # config, paths, strings, tema, startup, ícones, versão
│   ├── app/                    # estado, fontes, bandeja, menu (modelo/desenho), ações
│   ├── ui/                     # diálogos Configurações/Sobre e helpers comuns
│   └── resources/              # ícone, manifesto, resource.h e .rc.in
├── tests/
│   ├── test_*.cpp              # 74 testes unitários (484 verificações)
│   ├── integration.ps1         # 16 testes de integração GUI/bandeja
│   ├── run_tests.ps1           # suite completa no Windows
│   ├── run_tests.sh            # suite completa no WSL (build MinGW)
│   └── test_framework.h        # mini framework de testes
└── tools/
    └── package_release.ps1     # ZIP de release + SHA-256
```

Cada tela/recurso tem seu próprio par `.h`/`.cpp`; não há mais um arquivo
monolítico. O `CMakeLists.txt` gera o `.rc` com a versão lida de `version.txt`.

## Compilação

Requisitos: CMake 3.20+ e um dos toolchains abaixo.

### MSVC (recomendado para release)

O script localiza o Visual Studio com `vswhere`, inicializa o `vcvars64` e
compila com Ninja:

```bat
build_msvc.bat
```

Saída: `build-msvc\MyFolderMenu.exe` (CRT estática, sem DLLs do VC++).

### MinGW-w64

Com `cmake`, `g++` e `windres` no `PATH`:

```bat
build_mingw.bat
```

Saída: `build-mingw\MyFolderMenu.exe`.

### Cross-compile a partir do WSL/Linux

```sh
./build.sh
```

Usa `x86_64-w64-mingw32-g++` e `cmake/toolchain-mingw-w64.cmake`. A saída é
`build-mingw/MyFolderMenu.exe` (~800 KB por causa das bibliotecas estáticas).

## Testes

No Windows (PowerShell 5.1+):

```powershell
tests\run_tests.ps1 -BuildDir build-msvc
```

O script executa primeiro os testes unitários (`mfm_tests.exe`) e depois os
testes de integração, que abrem o executável real e verificam:

- recurso de versão 1.0.0 e cabeçalho PE x64 GUI;
- criação da configuração padrão e migração do `folders.json`;
- JSON/INI inválidos e preservação de configuração existente;
- diálogos Configurações/Sobre, inclusive cancelamento;
- menus com pastas, vazio e nos temas Claro/Escuro;
- modo bandeja, instância única e parâmetros de diagnóstico.

Os testes usam a variável `MFM_APPDATA` para redirecionar toda a configuração
para uma pasta temporária; a configuração real do usuário nunca é tocada.

No WSL, `tests/run_tests.sh` compila com MinGW e executa as duas suítes.

## Execução

```bat
MyFolderMenu.exe
```

O aplicativo inicia direto na bandeja. Clique com o botão direito no ícone para
abrir o menu; duplo clique abre a primeira pasta disponível.

## Configuração

Arquivo: `%APPDATA%\MyFolderMenu\folders.ini`

```ini
[Folders]
Folder1=C:\Users\Adrian\Desktop
Folder2=C:\Users\Adrian\Downloads
Folder3=C:\Users\Adrian\Documents

[Settings]
HideFileExtensions=1
Theme=Sistema
```

- `Theme`: `Sistema`, `Claro` ou `Escuro` (também aceita `light`/`dark`).
- `HideFileExtensions`: `1`/`0` (aceita `true`/`false`, `sim`/`nao`).
- Caminhos aceitam variáveis de ambiente (ex.: `%USERPROFILE%\Documents`).
- Duplicatas são removidas (comparação sem diferenciar maiúsculas).
- Se o INI não existir, é criado com Desktop, Downloads e Documentos; se houver
  um `folders.json` antigo, ele é migrado.

As configurações também podem ser alteradas pelo diálogo **Configurações...**,
que permite exportar/importar o INI.

## Diagnóstico (para testes)

Estes parâmetros abrem uma janela/menu direto e **ignoram o mutex de instância única**:

| Parâmetro | Efeito |
| --- | --- |
| `--menu` | Abre o menu no centro da área de trabalho e encerra. |
| `--settings` | Abre o diálogo de Configurações. |
| `--about` | Abre o diálogo Sobre. |
| `--version` | Imprime a versão e encerra. |

```bat
MyFolderMenu.exe --about
```

## Geração do pacote de release

```powershell
tools\package_release.ps1 -BuildDir build-msvc
```

Gera `release\MyFolderMenu-1.0.0-win64.zip` (executável + README + versão +
notas) e `release\SHA256SUMS.txt`.

## Diferenças em relação à versão C#

- Menus são desenhados pelo próprio código (owner-draw), para reproduzir as
  cores exatas dos temas claro/escuro e preservar o alfa dos ícones do shell.
- Sem tooltips por item de menu (o WinForms original também não exibia).
- O seletor de pasta usa o navegador clássico do shell (`SHBrowseForFolder`);
  o WinForms usava o mesmo diálogo via `FolderBrowserDialog`.
- Layout dos diálogos dimensionado pela DPI do monitor no momento da abertura
  (não há reposicionamento dinâmico ao mover entre monitores com DPI diferente).
- O tema escuro de controles padrão usa `SetWindowTheme(..., L"DarkMode_Explorer")`
  e `SetPreferredAppMode`; em versões muito antigas do Windows 10 alguns
  controles podem aparecer com o tema claro do sistema.
- Versão exibida em **Sobre** é lida do `VERSIONINFO` do executável.

## Licença

Copyleft — adrian.barbosa@gmail.com.
