# Gera o pacote ZIP de release a partir de um build existente.
[CmdletBinding()]
param(
    [string]$BuildDir = "build-msvc",
    [string]$Version = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = ([System.IO.File]::ReadAllText((Join-Path $Root "version.txt"))).Trim()
}
if ($Version -notmatch '^\d+\.\d+\.\d+$') {
    throw "Versao invalida em version.txt: '$Version'"
}

$exe = Join-Path $Root (Join-Path $BuildDir "MyFolderMenu.exe")
if (-not (Test-Path -LiteralPath $exe)) {
    throw "Executavel nao encontrado: $exe. Compile primeiro."
}

$releaseDir = Join-Path $Root "release"
$packageName = "MyFolderMenu-$Version-win64"
$stage = Join-Path $releaseDir $packageName
$zip = Join-Path $releaseDir ($packageName + ".zip")

if (Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null

Copy-Item -LiteralPath $exe -Destination $stage
Copy-Item -LiteralPath (Join-Path $Root "README.md") -Destination $stage
Copy-Item -LiteralPath (Join-Path $Root "version.txt") -Destination $stage
$releaseNotes = Join-Path $Root ("RELEASE_" + $Version + ".md")
if (Test-Path -LiteralPath $releaseNotes) {
    Copy-Item -LiteralPath $releaseNotes -Destination $stage
}

if (Test-Path -LiteralPath $zip) {
    Remove-Item -LiteralPath $zip -Force
}
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -CompressionLevel Optimal
Remove-Item -LiteralPath $stage -Recurse -Force

$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
$size = (Get-Item -LiteralPath $zip).Length
$line = $hash + "  " + $packageName + ".zip"
Set-Content -LiteralPath (Join-Path $releaseDir "SHA256SUMS.txt") -Value $line -Encoding ASCII

Write-Host ("Pacote: " + $zip)
Write-Host ("Tamanho: " + $size + " bytes")
Write-Host ("SHA256: " + $hash)
