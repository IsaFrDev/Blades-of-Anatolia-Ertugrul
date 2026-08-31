# Testlarni ishga tushirish
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
& build\ertugrul_tests.exe
exit $LASTEXITCODE
