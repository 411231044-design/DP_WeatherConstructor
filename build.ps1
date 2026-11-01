# Сборка @KR_SandstormPort: config.cpp -> PBO через AddonBuilder (DayZ Tools).
# Использование: .\build.ps1 [-AddonBuilder <путь>] [-OutDir <куда>]
[CmdletBinding()]
param(
    [string]$AddonBuilder = "${env:ProgramFiles(x86)}\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe",
    [string]$OutDir = ""
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $OutDir) { $OutDir = Join-Path $root "@KR_SandstormPort" }
if (-not (Test-Path $AddonBuilder)) { throw "AddonBuilder не найден: $AddonBuilder" }

New-Item -ItemType Directory -Force -Path (Join-Path $OutDir "addons") | Out-Null
& $AddonBuilder (Join-Path $root "KR_SandstormPort") $OutDir -packonly -prefix=KR_SandstormPort -clear
if ($LASTEXITCODE -ne 0) { throw "AddonBuilder вернул $LASTEXITCODE" }
Get-ChildItem (Join-Path $OutDir "addons") -Filter *.pbo | ForEach-Object {
    Write-Host ("готово: " + $_.FullName) -ForegroundColor Green
}
