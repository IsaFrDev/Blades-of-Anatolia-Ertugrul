Add-Type -AssemblyName System.Speech
$syn = New-Object System.Speech.Synthesis.SpeechSynthesizer
$syn.SelectVoice("Microsoft Zira Desktop")
$syn.Rate = -1
$fmt = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(22050, [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen, [System.Speech.AudioFormat.AudioChannel]::Mono)
$root = "D:\Unreal_projects\Ertugrul\Content\Ertugrul\Data"
$csv = Join-Path $root "npc_loc.csv"
$lines = Get-Content $csv -Encoding UTF8 | Select-Object -Skip 1
$langs = @("uz", "tr", "en")
$n = 0
foreach ($line in $lines) {
  if (-not $line.StartsWith('"')) { continue }
  $m = [regex]::Matches($line, '"((?:[^"]|"")*)"')
  if ($m.Count -lt 4) { continue }
  $key = $m[0].Groups[1].Value
  if (-not ($key.StartsWith("DLG_") -or $key.StartsWith("greet."))) { continue }
  for ($i = 0; $i -lt 3; $i++) {
    $text = $m[$i + 1].Groups[1].Value.Replace('""', '"')
    if ($text.Length -lt 2) { continue }
    $dir = Join-Path $root ("audio\vo\" + $langs[$i])
    $out = Join-Path $dir ($key + ".wav")
    if (Test-Path $out) { continue }
    try { $syn.SetOutputToWaveFile($out, $fmt); $syn.Speak($text); $syn.SetOutputToNull(); $n++ } catch { }
  }
}
"generated: $n"
