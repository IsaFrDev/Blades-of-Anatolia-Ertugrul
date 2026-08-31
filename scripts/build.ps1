# Dirilish: Ertug'rul - 3D o'yinni yig'ish (CMake + Ninja + MinGW)
# Ishlatish:
#   .\scripts\build.ps1              # Release
#   .\scripts\build.ps1 -Debug       # Debug (belgilar bilan)
#   .\scripts\build.ps1 -Clean       # build\ ni tozalab qaytadan
param(
    [switch]$Debug,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# --- asboblarni topish ---
$mingw = "D:\gcc\mingw64\bin"
if (Test-Path $mingw) { $env:PATH = "$mingw;$env:PATH" }

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue)
if ($null -eq $cmake) { throw "cmake topilmadi. MinGW/CMake PATH ga qo'shilganini tekshiring." }
$ninja = (Get-Command ninja -ErrorAction SilentlyContinue)
$gen = if ($null -ne $ninja) { "Ninja" } else { "MinGW Makefiles" }

$cfg = if ($Debug) { "Debug" } else { "Release" }
$bdir = "build"

if ($Clean -and (Test-Path $bdir)) {
    Write-Host "[0/3] build\ tozalanmoqda..." -ForegroundColor DarkGray
    Remove-Item -Recurse -Force $bdir
}
New-Item -ItemType Directory -Force -Path $bdir, saves | Out-Null

Write-Host "[1/3] CMake sozlanmoqda ($gen / $cfg)..." -ForegroundColor Yellow
& cmake -S . -B $bdir -G $gen -DCMAKE_BUILD_TYPE=$cfg
if ($LASTEXITCODE -ne 0) { throw "CMake sozlash muvaffaqiyatsiz" }

Write-Host "[2/3] Yig'ilmoqda..." -ForegroundColor Yellow
& cmake --build $bdir --parallel
if ($LASTEXITCODE -ne 0) { throw "Yig'ish muvaffaqiyatsiz" }

Write-Host "[3/3] Kontent diagnostikasi..." -ForegroundColor Yellow
& "$bdir\ertugrul.exe" --check
# --check tarjima yetishmasa 1 qaytaradi; bu qurilishni buzmaydi
Write-Host ""
Write-Host "Tayyor:" -ForegroundColor Green
Write-Host "  build\ertugrul.exe                 3D o'yin"
Write-Host "  build\ertugrul.exe --lang tr       turkcha interfeys"
Write-Host "  build\ertugrul.exe --episode EP001 to'g'ridan-to'g'ri epizod"
Write-Host "  build\ertugrul.exe --check         kontent diagnostikasi"
Write-Host ""
Write-Host "Ovoz fayllarini oldindan tayyorlash:" -ForegroundColor DarkGray
Write-Host "  .\scripts\gen_voice.ps1 -Lang uz"
