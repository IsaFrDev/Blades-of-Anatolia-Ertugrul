<#
  ESKIRGAN — ishlatmang.
  Bu skript Windows SAPI ni ishlatadi va hamma personajni ayol rus
  ovozi bilan o'qiydi. O'rniga: python tools/gen_voice.py --lang uz,tr,en
#>
<#
================================================================================
 gen_voice.ps1 — Cutscene replikalari uchun oldindan WAV ovoz fayllarini yaratadi
================================================================================

 NIMA QILADI:
   1. data/cutscenes/*.json fayllarini o'qiydi va har bir sahnaning "lines"
      massividan replika identifikatorini oladi (voId, bo'lmasa locKey).
   2. Replika matnini localization/*.csv dan tanlangan til ustunidan topadi
      (CSV sarlavhasi: keys,uz,tr,en).
   3. System.Speech.Synthesis.SpeechSynthesizer bilan matnni o'qib,
      assets/audio/vo/<til>/<voId>.wav fayliga yozadi.

 ISHLATISH:
   powershell -ExecutionPolicy Bypass -File scripts\gen_voice.ps1
   powershell -ExecutionPolicy Bypass -File scripts\gen_voice.ps1 -Lang uz -Rate 1 -Force
   powershell -ExecutionPolicy Bypass -File scripts\gen_voice.ps1 -Lang uz,tr

 PARAMETRLAR:
   -Lang   uz | tr | en | all  (vergul yoki probel bilan ajratiladi;
           standart: hammasi).  DIQQAT: powershell.exe -File orqali
           chaqirilganda "-Lang uz,tr" bitta satr sifatida keladi, shuning
           uchun skript o'zi vergul bo'yicha ajratadi.
   -Rate   -10..10, nutq tezligi (standart 0 = normal)
   -Force  mavjud WAV fayllarni qayta yaratadi

--------------------------------------------------------------------------------
 OVOZLAR HAQIDA MUHIM ESLATMA
--------------------------------------------------------------------------------
 Bu kompyuterda faqat 2 ta SAPI ovozi o'rnatilgan:
     * Microsoft Zira Desktop  (en-US)
     * Microsoft Irina Desktop (ru-RU)
 O'ZBEK va TURK ovozi YO'Q. Shuning uchun:

     -Lang en  ->  Zira  (matn o'zgarishsiz o'qiladi)
     -Lang uz  ->  Irina (matn LOTINDAN KIRILLGA o'giriladi)
     -Lang tr  ->  Irina (matn LOTINDAN KIRILLGA o'giriladi)

 Sababi: rus TTS mexanizmi lotin harflarini rus tili qoidalari bilan emas,
 balki "begona so'z" sifatida g'alat o'qiydi. Matnni kirillga o'girsak,
 o'zbek/turk fonetikasi ancha yaqin va tushunarli chiqadi.

--------------------------------------------------------------------------------
 TRANSLITERATSIYA JADVALI (lotin -> kirill, rus TTS uchun)
--------------------------------------------------------------------------------
 Qoidalar YUQORIDAN PASTGA, ketma-ket qo'llanadi. Uzun birikmalar avval
 turadi, aks holda "sh" -> "с"+"х" bo'lib buzilardi. Natija har doim kirill
 bo'lgani uchun keyingi (lotin) qoidalar unga tegmaydi.

   BIRIKMALAR (avval):
     yo'      -> ё      (o'zbek "yo'l" -> "ёл")
     o' oʻ oʼ -> о      (o'zbek ў tovushi; "ё" glayd qo'shib yuboradi, shuning
                         uchun toza "о" tanlandi)
     g' gʻ gʼ -> г      (o'zbek ғ)
     sh       -> ш
     ch       -> ч
     kh       -> х
     ts       -> ц
     ng       -> нг     (o'zbek/turk burun tovushi)

   Y + UNLI (yumshoq unlilar):
     ya -> я    yo -> ё    yu -> ю    yü -> ю
     ye -> е    yö -> ё    yi -> йи   yı -> йы
     y  -> й    (qolgan barcha holatda)

   TURK DIAKRITIKASI:
     ğ -> г     ş -> ш     ç -> ч     ı -> ы
     ü -> ю     ö -> ё     â -> а     î -> и     û -> у     é -> е

   SO'Z BOSHIDAGI E:
     \be -> э   ("Ertugrul" -> "эртугрул"; aks holda "ертугрул" = "йертугрул")

   YAKKA HARFLAR:
     a->а  b->б  c->дж  d->д  e->е  f->ф  g->г  h->х  i->и  j->ж
     k->к  l->л  m->м   n->н  o->о  p->п  q->к  r->р  s->с  t->т
     u->у  v->в  w->в   x->х  z->з

   TOZALASH:
     qolgan apostroflar (' ʻ ʼ ` ´ ' ') olib tashlanadi

 IZOH: q->к va h/x->х birlashishi ataylab — rus tilida ق va ҳ uchun alohida
 harf yo'q. j->ж (o'zbek "jang" -> "жанг"), c->дж (turk "cami" -> "джами").
================================================================================
#>

[CmdletBinding()]
param(
    # ValidateSet ataylab ishlatilmadi: powershell.exe -File orqali "-Lang uz,tr"
    # bitta satr bo'lib keladi va ValidateSet uni rad etadi. Quyida o'zimiz
    # ajratamiz va tekshiramiz.
    [string[]] $Lang = @('uz', 'tr', 'en'),

    [ValidateRange(-10, 10)]
    [int] $Rate = 0,

    [switch] $Force
)

# Skript hech qachon build ni buzmasligi kerak -> xatolarni o'zimiz ushlaymiz
$ErrorActionPreference = 'Stop'

# ------------------------------------------------------------------------------
# Loyiha ildizini aniqlash
# ------------------------------------------------------------------------------
$scriptDir = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    Write-Host "XATO: skript joylashgan papkani aniqlab bo'lmadi." -ForegroundColor Red
    exit 0
}
$Root = Split-Path -Parent $scriptDir

# ------------------------------------------------------------------------------
# Til ro'yxatini normallashtirish ("uz,tr", "uz tr", "all" — hammasi ishlaydi)
# ------------------------------------------------------------------------------
$Known = @('uz', 'tr', 'en')
$LangList = New-Object System.Collections.ArrayList

foreach ($item in @($Lang)) {
    if ($null -eq $item) { continue }
    $tokens = ([string]$item).Split(@(',', ';', ' ', "`t"), [System.StringSplitOptions]::RemoveEmptyEntries)
    foreach ($tok in $tokens) {
        $t = $tok.Trim().ToLowerInvariant()
        if ($t -eq 'all' -or $t -eq '*') {
            foreach ($k in $Known) { if (-not $LangList.Contains($k)) { [void] $LangList.Add($k) } }
            continue
        }
        if ($Known -contains $t) {
            if (-not $LangList.Contains($t)) { [void] $LangList.Add($t) }
        } else {
            Write-Host ("OGOHLANTIRISH: noma'lum til '{0}' — e'tiborsiz qoldirildi (ruxsat: uz, tr, en, all)" -f $tok) -ForegroundColor Yellow
        }
    }
}
if ($LangList.Count -eq 0) { foreach ($k in $Known) { [void] $LangList.Add($k) } }

Write-Host ""
Write-Host "=== Ertugrul: ovoz (VO) generatori ===" -ForegroundColor Cyan
Write-Host ("Loyiha ildizi : {0}" -f $Root)
Write-Host ("Tillar        : {0}" -f ($LangList -join ', '))
Write-Host ("Tezlik (Rate) : {0}" -f $Rate)
Write-Host ("Force         : {0}" -f [bool]$Force)
Write-Host ""

# ------------------------------------------------------------------------------
# Transliteratsiya qoidalari (tartib MUHIM — yuqoridagi izohga qarang)
# ------------------------------------------------------------------------------
$TranslitPairs = @(
    # --- apostrofli birikmalar ---
    "yo'", 'ё',
    "o'",  'о',
    "g'",  'г',
    # --- lotin birikmalari ---
    'sh',  'ш',
    'ch',  'ч',
    'kh',  'х',
    'ts',  'ц',
    'ng',  'нг',
    # --- y + unli ---
    'ya',  'я',
    'yo',  'ё',
    'yu',  'ю',
    'yü',  'ю',
    'ye',  'е',
    'yö',  'ё',
    'yi',  'йи',
    'yı',  'йы',
    'y',   'й',
    # --- turk diakritikasi ---
    'ğ',   'г',
    'ş',   'ш',
    'ç',   'ч',
    'ı',   'ы',
    'ü',   'ю',
    'ö',   'ё',
    'â',   'а',
    'î',   'и',
    'û',   'у',
    'é',   'е',
    # --- so'z boshidagi e ---
    '\be', 'э',
    # --- yakka harflar ---
    'a',   'а',
    'b',   'б',
    'c',   'дж',
    'd',   'д',
    'e',   'е',
    'f',   'ф',
    'g',   'г',
    'h',   'х',
    'i',   'и',
    'j',   'ж',
    'k',   'к',
    'l',   'л',
    'm',   'м',
    'n',   'н',
    'o',   'о',
    'p',   'п',
    'q',   'к',
    'r',   'р',
    's',   'с',
    't',   'т',
    'u',   'у',
    'v',   'в',
    'w',   'в',
    'x',   'х',
    'z',   'з',
    # --- tozalash ---
    "'",   ''
)

function Convert-LatinToCyrillic {
    param([string] $Text)

    if ([string]::IsNullOrWhiteSpace($Text)) { return '' }

    $s = $Text

    # 1) Apostrof variantlarini bitta ASCII apostrofga keltiramiz
    #    U+2018 U+2019 U+02BB U+02BC U+0060 U+00B4
    #    DIQQAT: bitta tirnoq ichida yozilgan — qo'sh tirnoqda ` belgisi
    #    PowerShell uchun escape bo'lib qolar edi.
    $s = $s -replace '[‘’ʻʼ`´]', "'"

    # 2) Turk bosh harflarini oldindan moslashtiramiz
    $s = $s -creplace 'İ', 'i'      # nuqtali bosh İ -> i
    $s = $s -creplace 'I', 'i'      # I -> i (turkcha ı emas: aralash matn uchun xavfsizroq)

    # 3) Kichik harfga (TTS uchun registr ahamiyatsiz)
    $s = $s.ToLowerInvariant()

    # 4) Qoidalarni ketma-ket qo'llaymiz (case-sensitive: matn allaqachon kichik)
    for ($i = 0; $i -lt $TranslitPairs.Count; $i += 2) {
        $pat = $TranslitPairs[$i]
        $rep = $TranslitPairs[$i + 1]
        $s = $s -creplace $pat, $rep
    }

    return $s
}

# ------------------------------------------------------------------------------
# 1-qadam: cutscene JSON larini o'qish
# ------------------------------------------------------------------------------
$cutDir = Join-Path $Root 'data\cutscenes'
$jsonFiles = @()
if (Test-Path -LiteralPath $cutDir) {
    $jsonFiles = @(Get-ChildItem -LiteralPath $cutDir -Filter '*.json' -File -ErrorAction SilentlyContinue)
}

if ($jsonFiles.Count -eq 0) {
    Write-Host "OGOHLANTIRISH: '$cutDir' papkasida cutscene JSON fayllari topilmadi." -ForegroundColor Yellow
    Write-Host "Yaratiladigan ovoz yo'q. Skript muvaffaqiyatli tugadi (build buzilmaydi)."
    Write-Host ""
    exit 0
}

# Obyektdan berilgan nomlardan BIRINCHI mavjudini o'qiydi.
# Loyihada ikki xil yozuv uchraydi: camelCase ("voId"/"locKey") va
# qisqa/snake_case ("vo"/"loc", "vo_id"/"loc_key") — ikkalasini ham qo'llaymiz.
function Get-FirstProp {
    param($Obj, [string[]] $Names)

    if ($null -eq $Obj) { return '' }
    $have = $Obj.PSObject.Properties.Name
    foreach ($n in $Names) {
        if ($have -contains $n) {
            $v = $Obj.PSObject.Properties[$n].Value
            if ($null -ne $v) {
                $s = ([string]$v).Trim()
                if (-not [string]::IsNullOrWhiteSpace($s)) { return $s }
            }
        }
    }
    return ''
}

# Sahna obyektidan replikalarni ajratib olish (turli JSON ko'rinishlarini qo'llaydi)
function Get-CutLines {
    param($Node)

    $result = New-Object System.Collections.ArrayList
    if ($null -eq $Node) { return $result }

    $scenes = @()
    if ($Node -is [System.Object[]]) {
        $scenes = @($Node)
    } elseif ($Node.PSObject.Properties.Name -contains 'scenes') {
        $scenes = @($Node.scenes)
    } else {
        $scenes = @($Node)
    }

    foreach ($sc in $scenes) {
        if ($null -eq $sc) { continue }
        if ($sc.PSObject.Properties.Name -notcontains 'lines') { continue }
        foreach ($ln in @($sc.lines)) {
            if ($null -eq $ln) { continue }

            $voId   = Get-FirstProp -Obj $ln -Names @('voId', 'vo_id', 'vo', 'voiceId', 'voice', 'audio', 'clip')
            $locKey = Get-FirstProp -Obj $ln -Names @('locKey', 'loc_key', 'loc', 'key', 'textKey', 'text_key')

            if ([string]::IsNullOrWhiteSpace($voId)) { $voId = $locKey }
            if ([string]::IsNullOrWhiteSpace($locKey)) { $locKey = $voId }
            if ([string]::IsNullOrWhiteSpace($voId)) { continue }

            $voId = $voId.Trim()
            # O'yin tomoni (Voice.cpp::safeId) faqat [A-Za-z0-9_.-] ni qabul qiladi.
            # Mos kelmagan id lardan WAV yaratsak, o'yin uni baribir topa olmaydi.
            if ($voId -notmatch '^[A-Za-z0-9_.\-]{1,190}$') {
                Write-Host ("  O'TKAZILDI (noto'g'ri voId): '{0}' — faqat A-Z a-z 0-9 _ . - ruxsat" -f $voId) -ForegroundColor Yellow
                continue
            }

            [void] $result.Add([pscustomobject]@{ VoId = $voId; LocKey = $locKey.Trim() })
        }
    }
    return $result
}

$entries = New-Object System.Collections.ArrayList
$seen = @{}

foreach ($jf in $jsonFiles) {
    try {
        $raw = Get-Content -LiteralPath $jf.FullName -Raw -Encoding UTF8
        if ([string]::IsNullOrWhiteSpace($raw)) { continue }
        $obj = $raw | ConvertFrom-Json
    } catch {
        Write-Host ("  O'TKAZILDI (buzuq JSON): {0} — {1}" -f $jf.Name, $_.Exception.Message) -ForegroundColor Yellow
        continue
    }

    foreach ($e in (Get-CutLines -Node $obj)) {
        if ($seen.ContainsKey($e.VoId)) { continue }
        $seen[$e.VoId] = $true
        [void] $entries.Add($e)
    }
}

Write-Host ("JSON fayllar   : {0}" -f $jsonFiles.Count)
Write-Host ("Noyob replika  : {0}" -f $entries.Count)

if ($entries.Count -eq 0) {
    Write-Host "OGOHLANTIRISH: cutscene fayllarida 'lines' replikalari topilmadi." -ForegroundColor Yellow
    Write-Host ""
    exit 0
}

# ------------------------------------------------------------------------------
# 2-qadam: lokalizatsiya CSV larini o'qish
# ------------------------------------------------------------------------------
$locDir = Join-Path $Root 'localization'
$loc = @{}
$csvCount = 0

if (Test-Path -LiteralPath $locDir) {
    foreach ($cf in @(Get-ChildItem -LiteralPath $locDir -Filter '*.csv' -File -ErrorAction SilentlyContinue)) {
        try {
            $rows = Import-Csv -LiteralPath $cf.FullName -Encoding UTF8
        } catch {
            Write-Host ("  O'TKAZILDI (buzuq CSV): {0}" -f $cf.Name) -ForegroundColor Yellow
            continue
        }
        $csvCount++
        foreach ($r in $rows) {
            if ($null -eq $r) { continue }
            $names = $r.PSObject.Properties.Name
            if ($names -notcontains 'keys') { continue }
            $k = [string]$r.'keys'
            if ([string]::IsNullOrWhiteSpace($k)) { continue }
            $loc[$k.Trim()] = $r
        }
    }
}
Write-Host ("CSV fayllar    : {0} ({1} kalit)" -f $csvCount, $loc.Count)
Write-Host ""

function Get-LocText {
    param([string] $Key, [string] $LangCode)

    if ([string]::IsNullOrWhiteSpace($Key)) { return '' }
    if (-not $loc.ContainsKey($Key)) { return '' }
    $row = $loc[$Key]
    $prop = $row.PSObject.Properties[$LangCode]
    if ($null -eq $prop) { return '' }
    $v = [string]$prop.Value
    if ([string]::IsNullOrWhiteSpace($v)) { return '' }
    return $v.Trim()
}

# ------------------------------------------------------------------------------
# 3-qadam: nutq sintezatorini tayyorlash
# ------------------------------------------------------------------------------
try {
    Add-Type -AssemblyName System.Speech
} catch {
    Write-Host "XATO: System.Speech assembly yuklanmadi. Ovoz yaratib bo'lmaydi." -ForegroundColor Red
    Write-Host ("Sabab: {0}" -f $_.Exception.Message)
    exit 0
}

$installed = @()
try {
    $probe = New-Object System.Speech.Synthesis.SpeechSynthesizer
    $installed = @($probe.GetInstalledVoices() | Where-Object { $_.Enabled })
    $probe.Dispose()
} catch {
    Write-Host "XATO: o'rnatilgan ovozlar ro'yxatini olib bo'lmadi." -ForegroundColor Red
    Write-Host ("Sabab: {0}" -f $_.Exception.Message)
    exit 0
}

if ($installed.Count -eq 0) {
    Write-Host "XATO: tizimda birorta ham yoqilgan SAPI ovozi yo'q." -ForegroundColor Red
    Write-Host "Yechim: Windows Sozlamalari -> Vaqt va til -> Nutq -> Ovoz qo'shish."
    exit 0
}

Write-Host "O'rnatilgan ovozlar:" -ForegroundColor Cyan
foreach ($v in $installed) {
    Write-Host ("  - {0}  [{1}]" -f $v.VoiceInfo.Name, $v.VoiceInfo.Culture.Name)
}
Write-Host ""

# Til -> ovoz tanlash. uz/tr uchun ruscha ovoz (transliteratsiya bilan).
function Select-VoiceForLang {
    param([string] $LangCode)

    if ($LangCode -eq 'en') { $prefName = 'Zira';  $prefCulture = 'en' }
    else                    { $prefName = 'Irina'; $prefCulture = 'ru' }

    $hit = $installed | Where-Object { $_.VoiceInfo.Name -like ('*' + $prefName + '*') } | Select-Object -First 1
    if ($hit) { return $hit.VoiceInfo.Name }

    $hit = $installed | Where-Object { $_.VoiceInfo.Culture.TwoLetterISOLanguageName -eq $prefCulture } | Select-Object -First 1
    if ($hit) { return $hit.VoiceInfo.Name }

    return $null
}

# ------------------------------------------------------------------------------
# 4-qadam: WAV fayllarni yaratish
# ------------------------------------------------------------------------------
$totalCreated = 0
$totalSkipped = 0
$totalFailed  = 0
$report = New-Object System.Collections.ArrayList

foreach ($lc in $LangList) {

    $voiceName = Select-VoiceForLang -LangCode $lc
    if ([string]::IsNullOrWhiteSpace($voiceName)) {
        Write-Host ("XATO [{0}]: mos SAPI ovozi topilmadi (kerak: {1})." -f $lc,
            $(if ($lc -eq 'en') { 'en-US, masalan Microsoft Zira Desktop' } else { 'ru-RU, masalan Microsoft Irina Desktop' })) -ForegroundColor Red
        [void] $report.Add([pscustomobject]@{ Til = $lc; Ovoz = '(yo''q)'; Yaratildi = 0; Otkazildi = 0; Xato = 0 })
        continue
    }

    $translit = ($lc -ne 'en')
    $outDir = Join-Path $Root ('assets\audio\vo\' + $lc)
    try {
        if (-not (Test-Path -LiteralPath $outDir)) {
            [void] (New-Item -ItemType Directory -Path $outDir -Force)
        }
    } catch {
        Write-Host ("XATO [{0}]: '{1}' papkasini yaratib bo'lmadi." -f $lc, $outDir) -ForegroundColor Red
        continue
    }

    Write-Host ("--- [{0}] ovoz: {1}{2} ---" -f $lc, $voiceName,
        $(if ($translit) { '  (lotin -> kirill transliteratsiya)' } else { '' })) -ForegroundColor Green

    $syn = $null
    try {
        $syn = New-Object System.Speech.Synthesis.SpeechSynthesizer
        $syn.SelectVoice($voiceName)
        $syn.Rate = $Rate
    } catch {
        Write-Host ("XATO [{0}]: sintezatorni sozlab bo'lmadi — {1}" -f $lc, $_.Exception.Message) -ForegroundColor Red
        if ($null -ne $syn) { $syn.Dispose() }
        continue
    }

    $created = 0
    $skipped = 0
    $failed  = 0
    $idx     = 0
    $count   = $entries.Count

    foreach ($e in $entries) {
        $idx++
        $outPath = Join-Path $outDir ($e.VoId + '.wav')

        if ((Test-Path -LiteralPath $outPath) -and (-not $Force)) {
            $skipped++
            Write-Host ("  [{0,4}/{1}] {2}  — mavjud, o'tkazildi" -f $idx, $count, $e.VoId) -ForegroundColor DarkGray
            continue
        }

        $text = Get-LocText -Key $e.LocKey -LangCode $lc
        if ([string]::IsNullOrWhiteSpace($text)) {
            $skipped++
            Write-Host ("  [{0,4}/{1}] {2}  — '{3}' kaliti '{4}' ustunida yo'q" -f $idx, $count, $e.VoId, $e.LocKey, $lc) -ForegroundColor Yellow
            continue
        }

        $spoken = $text
        if ($translit) { $spoken = Convert-LatinToCyrillic -Text $text }
        if ([string]::IsNullOrWhiteSpace($spoken)) {
            $skipped++
            continue
        }

        try {
            $syn.SetOutputToWaveFile($outPath)
            $syn.Speak($spoken)
            $syn.SetOutputToNull()
            $created++
            Write-Host ("  [{0,4}/{1}] {2}  -> {3}" -f $idx, $count, $e.VoId, (Split-Path -Leaf $outPath))
        } catch {
            $failed++
            try { $syn.SetOutputToNull() } catch { }
            # Chala yozilgan faylni o'chiramiz — o'yin buzuq WAV ni o'qimasin
            try { if (Test-Path -LiteralPath $outPath) { Remove-Item -LiteralPath $outPath -Force -ErrorAction SilentlyContinue } } catch { }
            Write-Host ("  [{0,4}/{1}] {2}  — XATO: {3}" -f $idx, $count, $e.VoId, $_.Exception.Message) -ForegroundColor Red
        }
    }

    try { $syn.Dispose() } catch { }

    Write-Host ("    yaratildi={0}  o'tkazildi={1}  xato={2}" -f $created, $skipped, $failed)
    Write-Host ""

    $totalCreated += $created
    $totalSkipped += $skipped
    $totalFailed  += $failed
    [void] $report.Add([pscustomobject]@{ Til = $lc; Ovoz = $voiceName; Yaratildi = $created; Otkazildi = $skipped; Xato = $failed })
}

# ------------------------------------------------------------------------------
# Yakuniy hisobot
# ------------------------------------------------------------------------------
Write-Host "=== HISOBOT ===" -ForegroundColor Cyan
if ($report.Count -gt 0) { $report | Format-Table -AutoSize | Out-String | Write-Host }
Write-Host ("JAMI: yaratildi={0}  o'tkazildi={1}  xato={2}" -f $totalCreated, $totalSkipped, $totalFailed)
Write-Host ("Chiqish papkasi: {0}" -f (Join-Path $Root 'assets\audio\vo'))
Write-Host ""

# Build hech qachon buzilmasin
exit 0
