# O'yinni ishga tushirish (ishchi papka loyiha ildizi bo'lishi SHART - data/, assets/, localization/ shu yerda)
# Misollar:
#   .\scripts\run-game.ps1
#   .\scripts\run-game.ps1 --lang tr
#   .\scripts\run-game.ps1 --episode EP005 --fullscreen
#   .\scripts\run-game.ps1 --check          # kontent diagnostikasi, oynasiz
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$exe = Join-Path $root "build\ertugrul.exe"
if (-not (Test-Path $exe)) {
    Write-Host "build\ertugrul.exe topilmadi. Avval yig'ing:" -ForegroundColor Yellow
    Write-Host "  .\scripts\build.ps1"
    exit 1
}
& $exe @args
exit $LASTEXITCODE
