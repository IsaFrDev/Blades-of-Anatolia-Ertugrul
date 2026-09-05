param([string[]]$Episodes, [int]$TimeoutSec = 540, [string]$Report = "D:\temp\claude\episode_report.txt")
${env:UE-LocalDataCachePath} = "D:\UE_DDC"
"Epizod sinovi $(Get-Date)" | Out-File $Report -Encoding utf8
foreach ($ep in $Episodes) {
  $logName = "ErtAuto_$ep.log"
  $args = @('"D:\Unreal_projects\Ertugrul\Ertugrul.uproject"','-game','-windowed','-ResX=640','-ResY=360',"-ErtEpisode=$ep",'-ErtUnlockAll','-ErtCutscene','-ErtAutoPlay',"-log=$logName",'-NoSound','-unattended','-nosplash')
  $sw = [Diagnostics.Stopwatch]::StartNew()
  $p = Start-Process -FilePath "D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -ArgumentList $args -WindowStyle Hidden -PassThru
  while (-not $p.HasExited -and $sw.Elapsed.TotalSeconds -lt $TimeoutSec) { Start-Sleep 5 }
  $timedOut = -not $p.HasExited
  if ($timedOut) { Stop-Process -Id $p.Id -Force -Confirm:$false -ErrorAction SilentlyContinue; Start-Sleep 2 }
  $log = "D:\Unreal_projects\Ertugrul\Saved\Logs\$logName"
  $lines = Select-String -Path $log -Pattern "\[Missiya\]|\[AutoPlay\]|Fatal|Assertion|Ensure condition|LogErtugrul: Warning|LogErtugrul: Error" | ForEach-Object { $_.Line -replace '^\[[^\]]+\]\[\s*\d+\]', '' }
  $status = if ($timedOut) { "TIMEOUT" } elseif ($p.ExitCode -eq 0) { "OK" } else { "EXIT $($p.ExitCode)" }
  "`n=== $ep : $status ($([int]$sw.Elapsed.TotalSeconds) s) ===" | Out-File $Report -Append -Encoding utf8
  $lines | Select-Object -First 60 | Out-File $Report -Append -Encoding utf8
  "$ep $status $([int]$sw.Elapsed.TotalSeconds)s"
}
