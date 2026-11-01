# Executa a suite completa: testes unitarios + testes de integracao GUI.
[CmdletBinding()]
param(
    [string]$BuildDir = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    foreach ($candidate in @("build-msvc", "build-mingw", "build")) {
        $exe = Join-Path $Root (Join-Path $candidate "MyFolderMenu.exe")
        if (Test-Path -LiteralPath $exe) {
            $BuildDir = $candidate
            break
        }
    }
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    throw "Nenhum build encontrado. Execute build_msvc.bat ou build_mingw.bat."
}

$buildPath = Join-Path $Root $BuildDir
$exe = Join-Path $buildPath "MyFolderMenu.exe"
$unit = Join-Path $buildPath "mfm_tests.exe"
$integration = Join-Path $PSScriptRoot "integration.ps1"

if (-not (Test-Path -LiteralPath $exe)) {
    throw "Executavel nao encontrado: $exe"
}
if (-not (Test-Path -LiteralPath $unit)) {
    throw "Testes unitarios nao encontrados: $unit (compile com MFM_BUILD_TESTS=ON)"
}

Write-Host "MyFolderMenu - suite de testes ($BuildDir)" -ForegroundColor Cyan
Write-Host ""

Write-Host "== Testes unitarios ==" -ForegroundColor Cyan
& $unit
$unitExit = $LASTEXITCODE

Write-Host ""
Write-Host "== Testes de integracao ==" -ForegroundColor Cyan
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $integration -Exe $exe
$integrationExit = $LASTEXITCODE

Write-Host ""
if ($unitExit -eq 0 -and $integrationExit -eq 0) {
    Write-Host "SUITE COMPLETA: todos os testes passaram." -ForegroundColor Green
    exit 0
}

Write-Host ("SUITE COM FALHAS: unitarios=" + $unitExit + ", integracao=" + $integrationExit) -ForegroundColor Red
exit 1
