# MyFolderMenu 1.0.0

Primeira versão estável da porta nativa em C++/Win32 do MyFolderMenu.

## Destaques

- **Sem .NET e sem dependências externas**: executável único, CRT estática.
- **Bandeja do sistema** com menu de pastas e submenus carregados sob demanda.
- **Pastas primeiro, arquivos depois**, ambos em ordem alfabética, com ícones
  reais do shell e ocultação de itens `hidden`/`system`.
- **Temas Claro, Escuro e Sistema**, incluindo barra de título escura e
  controles padrão no modo escuro.
- **Configuração INI** em `%APPDATA%\MyFolderMenu\folders.ini`, com migração
  automática do antigo `folders.json`.
- **Autostart** via `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.
- **Instância única** (`Local\MyFolderMenu`).
- **DPI PerMonitorV2** declarado no manifesto.
- Diálogos de **Configurações** e **Sobre**, com exportação/importação do INI.

## Estrutura do código

O código foi dividido em módulos testáveis:

- `src/core` — configuração, caminhos, strings, tema, startup, ícones e versão.
- `src/app` — estado, fontes, bandeja, modelo/desenho de menu e ações.
- `src/ui` — diálogos Configurações/Sobre e helpers comuns.
- `src/resources` — ícone, manifesto e `VERSIONINFO` gerado a partir de
  `version.txt`.

## Qualidade

- **74 testes unitários** (484 verificações) cobrindo config/INI/JSON, paths,
  strings, tema, startup, versão, modelo de menu, medição/desenho e diálogos.
- **16 testes de integração** que executam o binário real e validam versão/PE,
  criação/migração de configuração, diálogos, menus, bandeja, instância única
  e parâmetros de diagnóstico.
- Builds MSVC e MinGW-w64; cross-compile a partir do WSL.

## Instalação

1. Baixe `MyFolderMenu-1.0.0-win64.zip`.
2. Extraia em qualquer pasta.
3. Execute `MyFolderMenu.exe`. O app inicia na bandeja.

Não é necessário instalar runtime: o executável usa CRT estática.

## Requisitos

- Windows 10 ou 11, x64.
- Nenhuma dependência adicional.

## Verificação do download

```powershell
Get-FileHash .\MyFolderMenu-1.0.0-win64.zip -Algorithm SHA256
```

Compare com o hash em `SHA256SUMS.txt` anexado à release.

## Notas

- Menus são owner-draw para reproduzir fielmente as cores dos temas e preservar
  o canal alfa dos ícones do shell.
- A versão exibida em **Sobre** é lida do `VERSIONINFO` do executável.
- Configuração e logs não são enviados a lugar nenhum; tudo fica local.

## Agradecimentos

Baseado no comportamento do MyFolderMenu original em C# WinForms.
