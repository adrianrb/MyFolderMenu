# MyFolderMenu - testes de integracao (Windows PowerShell 5.1+)
#
# Executa o binario real e valida janelas, dialogos, bandeja, instancia unica,
# criacao/migracao de configuracao e recursos de versao. Todos os testes usam
# MFM_APPDATA apontando para uma pasta temporaria, sem tocar na configuracao
# real do usuario.
#
# Uso:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tests\integration.ps1
#   powershell ... -File tests\integration.ps1 -Exe build-msvc\MyFolderMenu.exe

[CmdletBinding()]
param(
    [string]$Exe = "",
    [int]$TimeoutMs = 8000
)

$ErrorActionPreference = "Stop"

Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class MfmTestNative
{
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr parent, EnumWindowsProc callback, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder text, int maxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int maxCount);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint message, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hWnd);

    public static string ClassName(IntPtr hWnd)
    {
        StringBuilder text = new StringBuilder(256);
        GetClassName(hWnd, text, text.Capacity);
        return text.ToString();
    }

    public static string Title(IntPtr hWnd)
    {
        StringBuilder text = new StringBuilder(1024);
        GetWindowText(hWnd, text, text.Capacity);
        return text.ToString();
    }

    public static IntPtr FindWindowForProcess(string className, int processId)
    {
        IntPtr found = IntPtr.Zero;
        EnumWindows(delegate(IntPtr hWnd, IntPtr lParam)
        {
            uint owner;
            GetWindowThreadProcessId(hWnd, out owner);
            if (owner == (uint)processId && ClassName(hWnd) == className)
            {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static string[] ChildTexts(IntPtr parent)
    {
        List<string> texts = new List<string>();
        EnumChildWindows(parent, delegate(IntPtr hWnd, IntPtr lParam)
        {
            string title = Title(hWnd);
            if (title.Length > 0)
            {
                texts.Add(title);
            }
            return true;
        }, IntPtr.Zero);
        return texts.ToArray();
    }

    public static int[] ChildIds(IntPtr parent)
    {
        List<int> ids = new List<int>();
        EnumChildWindows(parent, delegate(IntPtr hWnd, IntPtr lParam)
        {
            ids.Add(GetDlgCtrlID(hWnd));
            return true;
        }, IntPtr.Zero);
        return ids.ToArray();
    }
}
"@

$Root = Split-Path -Parent $PSScriptRoot
$script:ExePath = $Exe
if ([string]::IsNullOrWhiteSpace($script:ExePath)) {
    $candidates = @(
        (Join-Path $Root "build-msvc\MyFolderMenu.exe"),
        (Join-Path $Root "build-mingw\MyFolderMenu.exe"),
        (Join-Path $Root "build\MyFolderMenu.exe"),
        (Join-Path $Root "MyFolderMenu.exe")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            $script:ExePath = $candidate
            break
        }
    }
}
if ([string]::IsNullOrWhiteSpace($script:ExePath) -or -not (Test-Path -LiteralPath $script:ExePath)) {
    throw "MyFolderMenu.exe nao encontrado. Compile com build_msvc.bat/build_mingw.bat ou use -Exe."
}
$script:ExePath = (Resolve-Path -LiteralPath $script:ExePath).Path

$script:Passed = 0
$script:Failed = 0
$script:Failures = New-Object System.Collections.Generic.List[string]
$script:Processes = New-Object System.Collections.Generic.List[System.Diagnostics.Process]
$script:TempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("mfm_integration_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $script:TempRoot -Force | Out-Null

function Assert-Condition {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message (obtido: '$Actual', esperado: '$Expected')"
    }
}

function New-TestDir {
    param([string]$Name)
    $path = Join-Path $script:TempRoot $Name
    New-Item -ItemType Directory -Path $path -Force | Out-Null
    return $path
}

function Start-Mfm {
    param(
        [string]$AppData,
        [string[]]$Arguments = @()
    )
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $script:ExePath
    $info.Arguments = ($Arguments -join " ")
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $false
    $info.WorkingDirectory = Split-Path -Parent $script:ExePath
    if (-not [string]::IsNullOrWhiteSpace($AppData)) {
        $info.EnvironmentVariables["MFM_APPDATA"] = $AppData
    }
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    if (-not $process.Start()) {
        throw "Falha ao iniciar $($script:ExePath)"
    }
    $script:Processes.Add($process)
    return $process
}

function Wait-Window {
    param(
        [int]$ProcessId,
        [string]$ClassName,
        [int]$Timeout = $TimeoutMs
    )
    $deadline = [DateTime]::UtcNow.AddMilliseconds($Timeout)
    while ([DateTime]::UtcNow -lt $deadline) {
        $handle = [MfmTestNative]::FindWindowForProcess($ClassName, $ProcessId)
        if ($handle -ne [IntPtr]::Zero) {
            return $handle
        }
        Start-Sleep -Milliseconds 40
    }
    return [IntPtr]::Zero
}

function Wait-Exit {
    param(
        [System.Diagnostics.Process]$Process,
        [int]$Timeout = $TimeoutMs
    )
    return $Process.WaitForExit($Timeout)
}

function Close-Window {
    param([IntPtr]$Handle)
    if ($Handle -ne [IntPtr]::Zero) {
        [void][MfmTestNative]::PostMessage($Handle, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    }
}

function Stop-ProcessSafely {
    param([System.Diagnostics.Process]$Process)
    try {
        if (-not $Process.HasExited) {
            $Process.Kill()
            [void]$Process.WaitForExit(3000)
        }
    } catch {
    }
}

function Stop-AllProcesses {
    foreach ($process in $script:Processes) {
        Stop-ProcessSafely $process
    }
}

function Invoke-Test {
    param([string]$Name, [scriptblock]$Body)
    try {
        & $Body
        $script:Passed++
        Write-Host ("[ PASS ] " + $Name) -ForegroundColor Green
    } catch {
        $script:Failed++
        $script:Failures.Add($Name + ": " + $_.Exception.Message)
        Write-Host ("[ FAIL ] " + $Name) -ForegroundColor Red
        Write-Host ("         " + $_.Exception.Message) -ForegroundColor DarkYellow
    }
}

function Close-And-Wait {
    param(
        [IntPtr]$Handle,
        [System.Diagnostics.Process]$Process
    )
    Close-Window $Handle
    Assert-Condition (Wait-Exit $Process 4000) "O processo nao encerrou apos WM_CLOSE"
}

function Read-IniText {
    param([string]$Directory)
    $path = Join-Path $Directory "folders.ini"
    Assert-Condition (Test-Path -LiteralPath $path) "folders.ini nao foi criado em $Directory"
    return [System.IO.File]::ReadAllText($path)
}

function Write-TestIni {
    param(
        [string]$Directory,
        [string[]]$Folders,
        [string]$Theme = "Sistema",
        [bool]$HideExtensions = $true
    )
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("[Folders]")
    for ($i = 0; $i -lt $Folders.Count; $i++) {
        $lines.Add("Folder" + ($i + 1) + "=" + $Folders[$i])
    }
    $lines.Add("")
    $lines.Add("[Settings]")
    $lines.Add("HideFileExtensions=" + $(if ($HideExtensions) { "1" } else { "0" }))
    $lines.Add("Theme=" + $Theme)
    $text = ($lines -join "`r`n") + "`r`n"
    [System.IO.File]::WriteAllText((Join-Path $Directory "folders.ini"), $text)
}

# ---------------------------------------------------------------------------
# Testes estaticos
# ---------------------------------------------------------------------------

Invoke-Test "Recurso de versao 1.0.0" {
    $info = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($script:ExePath)
    Assert-Equal $info.FileVersion "1.0.0" "FileVersion"
    Assert-Equal $info.ProductVersion "1.0.0" "ProductVersion"
    Assert-Equal $info.ProductName "MyFolderMenu" "ProductName"
    Assert-Equal $info.CompanyName "adrian.barbosa@gmail.com" "CompanyName"
}

Invoke-Test "Executavel PE x64 GUI" {
    $bytes = [System.IO.File]::ReadAllBytes($script:ExePath)
    Assert-Condition ($bytes.Length -gt 512) "Executavel muito pequeno"
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    Assert-Condition ($peOffset -gt 0 -and ($peOffset + 0x60) -lt $bytes.Length) "Cabecalho PE invalido"
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    $subsystem = [BitConverter]::ToUInt16($bytes, $peOffset + 0x5C)
    Assert-Equal $machine 0x8664 "Machine (x64)"
    Assert-Equal $subsystem 2 "Subsystem (Windows GUI)"
}

# ---------------------------------------------------------------------------
# Configuracao
# ---------------------------------------------------------------------------

Invoke-Test "Cria configuracao padrao no primeiro uso" {
    $dir = New-TestDir "defaults"
    $process = Start-Mfm -AppData $dir -Arguments @("--about")
    $window = Wait-Window $process.Id "MFMAboutDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Sobre nao abriu"
    Close-And-Wait $window $process

    $text = Read-IniText $dir
    Assert-Condition ($text.Contains("[Folders]")) "Secao [Folders] ausente"
    Assert-Condition ($text.Contains("[Settings]")) "Secao [Settings] ausente"
    Assert-Condition ($text.Contains("Theme=Sistema")) "Tema padrao incorreto"
    Assert-Condition ($text.Contains("HideFileExtensions=1")) "Padrao de extensoes incorreto"
    Assert-Condition ($text.Contains("Folder1=")) "Nenhuma pasta padrao gravada"
}

Invoke-Test "Migra folders.json legado para folders.ini" {
    $dir = New-TestDir "legacy"
    $legacyA = Join-Path $dir "Legacy A"
    $legacyB = Join-Path $dir "Legacy B"
    New-Item -ItemType Directory -Path $legacyA -Force | Out-Null
    New-Item -ItemType Directory -Path $legacyB -Force | Out-Null
    $json = '{"Folders":["' + $legacyA.Replace('\', '\\') + '","' + $legacyB.Replace('\', '\\') + '"]}'
    [System.IO.File]::WriteAllText((Join-Path $dir "folders.json"), $json)

    $process = Start-Mfm -AppData $dir -Arguments @("--about")
    $window = Wait-Window $process.Id "MFMAboutDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Sobre nao abriu"
    Close-And-Wait $window $process

    $text = Read-IniText $dir
    Assert-Condition ($text.IndexOf($legacyA, [StringComparison]::OrdinalIgnoreCase) -ge 0) "Pasta A nao migrada"
    Assert-Condition ($text.IndexOf($legacyB, [StringComparison]::OrdinalIgnoreCase) -ge 0) "Pasta B nao migrada"
    Assert-Condition (-not $text.Contains("Downloads")) "Migracao usou os padroes em vez do JSON"
}

Invoke-Test "JSON legado invalido cai nos padroes" {
    $dir = New-TestDir "legacy-invalid"
    [System.IO.File]::WriteAllText((Join-Path $dir "folders.json"), '{"Folders":["C:\\A"')
    $process = Start-Mfm -AppData $dir -Arguments @("--about")
    $window = Wait-Window $process.Id "MFMAboutDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Sobre nao abriu"
    Close-And-Wait $window $process
    $text = Read-IniText $dir
    Assert-Condition ($text.Contains("Folder1=")) "Configuracao padrao nao foi criada"
}

Invoke-Test "Configuracao existente nao e sobrescrita" {
    $dir = New-TestDir "preserve"
    $custom = "C:\Windows"
    Write-TestIni -Directory $dir -Folders @($custom) -Theme "Escuro" -HideExtensions $false
    $before = [System.IO.File]::ReadAllText((Join-Path $dir "folders.ini"))

    $process = Start-Mfm -AppData $dir -Arguments @("--about")
    $window = Wait-Window $process.Id "MFMAboutDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Sobre nao abriu"
    Close-And-Wait $window $process

    $after = [System.IO.File]::ReadAllText((Join-Path $dir "folders.ini"))
    Assert-Equal $after $before "O INI existente foi alterado"
}

Invoke-Test "INI sem secao [Folders] nao impede a inicializacao" {
    $dir = New-TestDir "invalid-ini"
    [System.IO.File]::WriteAllText((Join-Path $dir "folders.ini"), "[Settings]`r`nTheme=Claro`r`n")
    $process = Start-Mfm -AppData $dir -Arguments @("--about")
    $window = Wait-Window $process.Id "MFMAboutDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Sobre nao abriu"
    Close-And-Wait $window $process
}

# ---------------------------------------------------------------------------
# Dialogos
# ---------------------------------------------------------------------------

Invoke-Test "Dialogo Sobre mostra nome, versao e copyright" {
    $dir = New-TestDir "about"
    $process = Start-Mfm -AppData $dir -Arguments @("--about")
    $window = Wait-Window $process.Id "MFMAboutDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Sobre nao abriu"
    Start-Sleep -Milliseconds 350
    Assert-Equal ([MfmTestNative]::Title($window)) "Sobre o MyFolderMenu" "Titulo do dialogo Sobre"

    $texts = [MfmTestNative]::ChildTexts($window)
    Assert-Condition ($texts -contains "MyFolderMenu") "Nome do aplicativo ausente"
    Assert-Condition (($texts | Where-Object { $_ -like "Versão 1.0.0*" }).Count -gt 0) "Versao 1.0.0 ausente"
    Assert-Condition (($texts | Where-Object { $_ -like "*(2026) Copyleft by adrian.barbosa@gmail.com*" }).Count -gt 0) "Copyright ausente"
    Assert-Condition ($texts -contains "OK") "Botao OK ausente"
    Close-And-Wait $window $process
}

Invoke-Test "Dialogo Configuracoes mostra todos os controles" {
    $dir = New-TestDir "settings"
    $process = Start-Mfm -AppData $dir -Arguments @("--settings")
    $window = Wait-Window $process.Id "MFMSettingsDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Configuracoes nao abriu"
    Start-Sleep -Milliseconds 350
    Assert-Equal ([MfmTestNative]::Title($window)) "Configurações" "Titulo do dialogo Configuracoes"

    $texts = [MfmTestNative]::ChildTexts($window)
    $expected = @(
        "Iniciar com o Windows",
        "Não exibir a extensão do arquivo",
        "Tema",
        "Claro",
        "Escuro",
        "Sistema",
        "Exportar configurações",
        "Importar configurações",
        "OK",
        "Cancelar"
    )
    foreach ($label in $expected) {
        Assert-Condition ($texts -contains $label) ("Controle ausente: " + $label)
    }

    $ids = [MfmTestNative]::ChildIds($window)
    Assert-Condition ($ids -contains 1) "Botao OK (IDOK) ausente"
    Assert-Condition ($ids -contains 2) "Botao Cancelar (IDCANCEL) ausente"

    Close-And-Wait $window $process
}

Invoke-Test "Cancelar Configuracoes nao altera o INI" {
    $dir = New-TestDir "settings-cancel"
    Write-TestIni -Directory $dir -Folders @("C:\Windows") -Theme "Sistema"
    $before = [System.IO.File]::ReadAllText((Join-Path $dir "folders.ini"))

    $process = Start-Mfm -AppData $dir -Arguments @("--settings")
    $window = Wait-Window $process.Id "MFMSettingsDialog"
    Assert-Condition ($window -ne [IntPtr]::Zero) "Dialogo Configuracoes nao abriu"
    Close-And-Wait $window $process

    $after = [System.IO.File]::ReadAllText((Join-Path $dir "folders.ini"))
    Assert-Equal $after $before "Cancelar/close alterou a configuracao"
}

# ---------------------------------------------------------------------------
# Menu e bandeja
# ---------------------------------------------------------------------------

Invoke-Test "Menu abre com pastas configuradas" {
    $dir = New-TestDir "menu"
    $folder = Join-Path $dir "Pasta ç ão"
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    New-Item -ItemType File -Path (Join-Path $folder "arquivo.txt") -Force | Out-Null
    Write-TestIni -Directory $dir -Folders @($folder)

    $process = Start-Mfm -AppData $dir -Arguments @("--menu")
    $menu = Wait-Window $process.Id "#32768" 6000
    Assert-Condition ($menu -ne [IntPtr]::Zero) "Menu popup nao apareceu"
    Start-Sleep -Milliseconds 300
    Assert-Condition ([MfmTestNative]::IsWindowVisible($menu)) "Menu popup nao esta visivel"
    Assert-Condition (-not $process.HasExited) "Processo encerrou com o menu aberto"
    Stop-ProcessSafely $process
}

Invoke-Test "Menu abre com lista de pastas vazia" {
    $dir = New-TestDir "menu-empty"
    Write-TestIni -Directory $dir -Folders @()

    $process = Start-Mfm -AppData $dir -Arguments @("--menu")
    $menu = Wait-Window $process.Id "#32768" 6000
    Assert-Condition ($menu -ne [IntPtr]::Zero) "Menu popup nao apareceu"
    Stop-ProcessSafely $process
}

Invoke-Test "Menu abre nos temas Claro e Escuro" {
    foreach ($theme in @("Claro", "Escuro")) {
        $dir = New-TestDir ("menu-theme-" + $theme)
        Write-TestIni -Directory $dir -Folders @("C:\Windows") -Theme $theme
        $process = Start-Mfm -AppData $dir -Arguments @("--menu")
        $menu = Wait-Window $process.Id "#32768" 6000
        Assert-Condition ($menu -ne [IntPtr]::Zero) ("Menu nao apareceu no tema " + $theme)
        Stop-ProcessSafely $process
    }
}

Invoke-Test "Modo bandeja permanece ativo com janela oculta" {
    $dir = New-TestDir "tray"
    $process = Start-Mfm -AppData $dir
    $main = Wait-Window $process.Id "MFMMainWindow"
    Assert-Condition ($main -ne [IntPtr]::Zero) "Janela principal nao foi criada"
    Start-Sleep -Milliseconds 800
    Assert-Condition (-not $process.HasExited) "Processo da bandeja encerrou sozinho"
    Assert-Condition (-not [MfmTestNative]::IsWindowVisible($main)) "Janela principal nao deveria ser visivel"
    Close-And-Wait $main $process
}

Invoke-Test "Instancia unica encerra a segunda copia" {
    $dir = New-TestDir "single-instance"
    $first = Start-Mfm -AppData $dir
    $main = Wait-Window $first.Id "MFMMainWindow"
    Assert-Condition ($main -ne [IntPtr]::Zero) "Primeira instancia nao criou a janela"

    $second = Start-Mfm -AppData $dir
    Assert-Condition (Wait-Exit $second 5000) "Segunda instancia nao encerrou"
    Assert-Equal $second.ExitCode 0 "Codigo de saida da segunda instancia"
    Assert-Condition (-not $first.HasExited) "Primeira instancia encerrou indevidamente"
    Close-And-Wait $main $first
}

Invoke-Test "Parametros de diagnostico ignoram o mutex" {
    $dir = New-TestDir "diagnostic-mutex"
    $first = Start-Mfm -AppData $dir
    $main = Wait-Window $first.Id "MFMMainWindow"
    Assert-Condition ($main -ne [IntPtr]::Zero) "Primeira instancia nao criou a janela"

    $diagnostic = Start-Mfm -AppData $dir -Arguments @("--about")
    $about = Wait-Window $diagnostic.Id "MFMAboutDialog"
    Assert-Condition ($about -ne [IntPtr]::Zero) "Diagnostico nao abriu com o mutex ocupado"
    Close-And-Wait $about $diagnostic
    Assert-Condition (-not $first.HasExited) "Primeira instancia encerrou"
    Close-And-Wait $main $first
}

# ---------------------------------------------------------------------------
# Limpeza
# ---------------------------------------------------------------------------

$exitCode = 0
try {
    Write-Host ""
    Write-Host ("Testes de integracao: " + $script:Passed + " passaram, " + $script:Failed + " falharam.")
    if ($script:Failed -gt 0) {
        Write-Host "Falhas:" -ForegroundColor Red
        foreach ($failure in $script:Failures) {
            Write-Host (" - " + $failure) -ForegroundColor Red
        }
        $exitCode = 1
    }
} finally {
    Stop-AllProcesses
    if (Test-Path -LiteralPath $script:TempRoot) {
        Remove-Item -LiteralPath $script:TempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}

exit $exitCode
