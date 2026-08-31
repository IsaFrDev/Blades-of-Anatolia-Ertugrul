# Java backendni ishga tushirish (http://localhost:8080)
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$java = "C:\Program Files\Microsoft\jdk-21.0.9.10-hotspot\bin"
if (Test-Path $java) { $env:PATH = "$java;$env:PATH" }
java -cp backend\out uz.ertugrul.backend.ErtugrulServer --port 8080 --data data --store backend\data
