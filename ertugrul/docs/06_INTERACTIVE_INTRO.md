# 06 — O'YNALADIGAN INTRO TIZIMI ⭐
### «Interactive Prologue Director» — kinematik emas, gameplay

> Siz aytdingiz: **"hamma intro videosi bunda Assassin'ga o'xshab o'yinchilar o'zi harakatlanadi"**
> va **"intro videolarda ham yillari bilan keltirilsin, chunki tarixiy o'yin bu"**.
>
> Bu hujjat — buning to'liq tizimi. **48 ta epizod × o'ynaladigan intro.**

---

## 1. MUAMMO: sizning JSON'ingizdagi ziddiyat

```jsonc
"intro_video": "assets/videos/ep1_intro.mp4"   // ❌ Bu — pre-rendered video
"...o'yinchilar o'zi harakatlanadi"            // ✅ Bu — gameplay
```

Bu ikkisi bir vaqtda bo'lolmaydi. **Yechim: `.mp4` ni butunlay olib tashlash.**

| Nima uchun video yomon | Nima uchun o'ynaladigan yaxshi |
|---|---|
| 48 × 3 daq video = **~35 GB** disk | ~0 GB (dvijok real-time renderlaydi) |
| Grafika farqi (video 1080p, o'yin 4K) | Bir xil sifat, o'yinchi sozlamalarida |
| O'yinchi `Skip` bosadi (85% hollarda) | O'yinchi **boshqaradi** — skip qilmaydi |
| Lokalizatsiya = 48×N ta video render | Lokalizatsiya = faqat subtitr/VO |
| Patch qilish = qayta render | Patch = kod o'zgarishi |
| O'yinchi tanlovlari ko'rsatilmaydi | **Tanlovlar ko'rinadi** (siz Titusni o'ldirganmisiz?) |

---

## 2. «INTERACTIVE PROLOGUE DIRECTOR» arxitekturasi

### 2.1 Asosiy qoidalar (buzilmas)

| # | Qoida |
|---|---|
| **1** | **Kamera hech qachon o'yinchidan olinmaydi.** Cutscene yo'q — faqat "kamerani boshqaruvchi" (`Director Blend`) 0.4 sek uchun 30% ta'sir qilishi mumkin. |
| **2** | **Harakat imkoniyati doim bor** — hatto bog'langan holatda ham (kamera burish). |
| **3** | **Yuklanish ekrani yo'q** — intro **oldingi epizodning yakuniy holatidan** to'g'ridan-to'g'ri boshlanadi (Level Streaming). |
| **4** | **Yil har doim ko'rsatiladi** — diegetik yoki minimal overlay bilan (3-bo'lim). |
| **5** | **Intro → gameplay o'tishi ko'rinmaydi.** Hech qanday "endi o'ynang" signali yo'q. |
| **6** | Intro **2–6 daqiqa**. Undan uzun bo'lsa — u epizodning bir qismi, intro emas. |
| **7** | **Skip bor**, lekin u faqat **qayta o'ynashda** ko'rinadi (birinchi marta yashiringan). |

### 2.2 7 xil intro arxetipi

Har epizod introsi shu 7 tadan biri. Bu — **xilma-xillikni kafolatlaydi**.

| # | Arxetip | Nima bo'ladi | Namuna |
|---|---|---|---|
| **A** | 🚶 **YURISH** (The Walk) | O'yinchi bir joydan boshqasiga yuradi, dunyo o'zini ko'rsatadi. Jang yo'q. | EP01 (o'lik podadan tepalikka), EP20 (Konya tongda) |
| **B** | 👐 **QO'L** (The Hands) | O'yinchi **narsa yasaydi/ochadi/bog'laydi** — birinchi shaxs, qo'llar ko'rinadi | EP05 (chodir yig'ish), EP15 (qoziq qoqish), EP26 (choy olish) |
| **C** | 👁 **BOSHQA KO'Z** (The Other) | O'yinchi **boshqa personaj** bo'lib o'ynaydi, keyin uni yo'qotadi | EP10 (Turgut), EP19 (ziyofat kuzatuvi) |
| **D** | 🌊 **SHO'NG'ISH** (The Plunge) | O'yinchi to'g'ridan-to'g'ri harakat o'rtasiga tashlanadi | EP11 (suv ostida), EP25 (qorda yotib) |
| **E** | 🎭 **KUZATUV** (The Watch) | O'yinchi harakatsiz, lekin **kamerani boshqaradi** — nima ko'rish uning tanlovi | EP19 (ziyofat), EP24 (bog'langan holat), EP33 (kengash) |
| **F** | 🏗 **MEHNAT** (The Labour) | O'yinchi jamoa bilan jismoniy ish qiladi (arqon tortish, tosh ko'tarish) | EP12 (mancınık), EP30 (devorga tosh) |
| **G** | 🕯 **NAFAS** (The Breath) | Hech narsa bo'lmaydi. O'yinchi shunchaki yashaydi. | EP18 (yaylov), EP34 (jang oldi), EP44 (tinch Söğüt) |

**Taqsimot (48 epizod):**
`A` 10 · `B` 8 · `C` 5 · `D` 7 · `E` 6 · `F` 5 · `G` 7

**Qoida:** bir xil arxetip **ketma-ket 2 martadan ko'p** bo'lmasin.

---

## 3. YIL KO'RSATISH — 3 usul ⭐

Siz aytdingiz: **"intro videolarda ham yillari bilan keltirilsin"**. Mana uch xil usul, har biri boshqa his beradi:

### 3.1 Usul A — «Diegetik» (eng yaxshi, 40% epizodlarda)

Yil **dunyoning o'zida** ko'rinadi, UI'da emas:

| Namuna | Epizod |
|---|---|
| Karvonsaroy toshidagi arabcha kitoba: **«٦٢٦»** (626 AH). Yaqinlashsangiz — tarjima chiqadi | EP06 |
| Tanga — o'yinchi olib ko'radi, ustida zarb yili | EP37 |
| Kotib **sanani yozadi** — qalam harakatini ko'rasiz | EP17 |
| Qabr toshi | EP27 |
| Farmondagi sana + muhr | EP20 |

### 3.2 Usul B — «Minimal overlay» (50% epizodlarda)

```
                                                            ╭─────────────────────────────────╮
                                                            │                                 │
     ┌─────────────────────────────────────────┐            │   ٦٤١ محرم                      │
     │                                         │            │   641 MUHARRAM                  │
     │                                         │            │   26 IYUN 1243                  │
     │        [gameplay davom etmoqda]         │            │                                 │
     │                                         │            │   KÖSE DAĞ                      │
     │                                         │            │   Sivas — Erzincan yo'li        │
     │   ٦٤١ محرم · 26 IYUN 1243               │            ╰─────────────────────────────────╯
     │   KÖSE DAĞ                              │
     └─────────────────────────────────────────┘                    ↑
                                                          `Tarix ko'rsatkichi: TO'LIQ` rejimida
```

**Texnik spetsifikatsiya:**
- Shrift: `Amiri` (arabcha) + `Inter Tight` (lotin) — ikkalasi ham ochiq litsenziya
- Rang: `#D4A853` (oltin), 78% opacity
- Animatsiya: **fade-in 1.2s → 4.5s turadi → fade-out 1.8s**
- Pozitsiya: pastki chap (16:9 safe area, gamepad'da HUD bilan to'qnashmaydi)
- ⚠️ **Hech qachon ekran markazida katta harflar bilan emas** — bu kinematik hissini beradi, biz undan qochamiz

### 3.3 Usul C — «Ovoz» (10% epizodlarda, muhim lahzalarda)

Personaj **sanani gapiradi** — tabiiy dialogda:

> **KOTIB:** *"Olti yuz o'ttiz to'rtinchi yil, zulhijja oyi. Sulton Alauddin ibn Kayxusrav... "* — (qalam to'xtaydi) — *"...marhum."*
> → *(EP19, sulton o'limi)*

### 3.4 Sozlama

| `Tarix ko'rsatkichi` | Ta'siri |
|---|---|
| **TO'LIQ** (default) | Yil + oy + joy + qisqa kontekst |
| **MINIMAL** | Faqat yil |
| **DIEGETIK** | Faqat dunyo ichida (immersiya maksimum) |
| **OFF** | Hech nima |

| `Taqvim` | Ta'siri |
|---|---|
| **IKKALASI** (default) | `٦٤١ محرم · 26 iyun 1243` |
| **MILODIY** | `26 iyun 1243` |
| **HIJRIY** | `641 Muharram` |

---

## 4. TEXNIK IMPLEMENTATSIYA (UE5)

### 4.1 Data Asset

```cpp
// ErtugrulGame/Public/Narrative/ErtIntroSequenceAsset.h
#pragma once
#include "Engine/DataAsset.h"
#include "ErtIntroSequenceAsset.generated.h"

UENUM(BlueprintType)
enum class EErtIntroArchetype : uint8
{
    Walk, Hands, OtherEyes, Plunge, Watch, Labour, Breath
};

UENUM(BlueprintType)
enum class EErtDateDisplayMode : uint8
{
    Diegetic,      // faqat dunyoda (kitoba, tanga, hujjat)
    Overlay,       // minimal overlay
    Spoken,        // dialogda aytiladi
    Combined
};

USTRUCT(BlueprintType)
struct FErtIntroBeat
{
    GENERATED_BODY()

    /** Nima bo'lishi kerak — trigger tag */
    UPROPERTY(EditAnywhere) FGameplayTag BeatTag;

    /** O'yinchi buni bajarmaguncha keyingi beat boshlanmaydi */
    UPROPERTY(EditAnywhere) bool bRequiresPlayerAction = true;

    /** Agar o'yinchi 25 sek ichida bajarmasa — yumshoq turtki (ovoz/nur) */
    UPROPERTY(EditAnywhere) float NudgeAfterSeconds = 25.f;

    /** Kamera ta'siri: 0 = to'liq o'yinchida, 1 = to'liq direktorda.
        ⚠️ Loyiha qoidasi: 0.35 dan oshmasin */
    UPROPERTY(EditAnywhere, meta=(ClampMax="0.35"))
    float DirectorCameraWeight = 0.f;

    UPROPERTY(EditAnywhere) TSoftObjectPtr<class ULevelSequence> OptionalAmbientSeq;
    UPROPERTY(EditAnywhere) FGameplayTagContainer CodexUnlocks;
};

UCLASS(BlueprintType)
class ERTUGRULGAME_API UErtIntroSequenceAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Meta") FName EpisodeId;                 // "EP024"
    UPROPERTY(EditAnywhere, Category="Meta") EErtIntroArchetype Archetype;
    UPROPERTY(EditAnywhere, Category="Meta") float TargetDurationSec = 210.f; // 2-6 daq

    // ── Sana (siz so'ragan "yillar bilan") ───────────────────────
    UPROPERTY(EditAnywhere, Category="Date") EErtDateDisplayMode DateMode;
    UPROPERTY(EditAnywhere, Category="Date") FString HijriDate;      // "641 Muharram"
    UPROPERTY(EditAnywhere, Category="Date") FString GregorianDate;  // "26 Iyun 1243"
    UPROPERTY(EditAnywhere, Category="Date") FText  PlaceName;       // loc-key orqali
    UPROPERTY(EditAnywhere, Category="Date") FText  SubCaption;      // "Sivas-Erzincan yo'li"
    /** Diegetik rejimda: qaysi aktyorda sana yozilgan (kitoba/tanga/hujjat) */
    UPROPERTY(EditAnywhere, Category="Date") FGameplayTag DiegeticDateActorTag;

    // ── Beat'lar ─────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, Category="Flow") TArray<FErtIntroBeat> Beats;

    /** Boshqariladigan personaj — EP10 da Turgut, EP19 da Ertug'rul */
    UPROPERTY(EditAnywhere, Category="Flow") TSoftClassPtr<APawn> OverridePawnClass;

    /** Boshqaruv cheklovlari (EP24: faqat kamera) */
    UPROPERTY(EditAnywhere, Category="Flow") bool bAllowMovement = true;
    UPROPERTY(EditAnywhere, Category="Flow") bool bAllowCombat   = false;

    /** «Oldingi epizodda...» — o'yinchining O'Z tanlovlaridan montaj */
    UPROPERTY(EditAnywhere, Category="Recap") bool bBuildDynamicRecap = true;
};
```

### 4.2 Direktor subsystem

```cpp
// ErtugrulGame/Private/Narrative/ErtIntroDirector.cpp
void UErtIntroDirector::BeginIntro(const UErtIntroSequenceAsset* Asset)
{
    check(Asset);
    ActiveAsset  = Asset;
    CurrentBeat  = 0;
    ElapsedSec   = 0.f;

    // 1) Boshqaruvni CHEKLA, lekin OLMA
    if (auto* PC = GetPlayerController())
    {
        PC->SetIgnoreMoveInput(!Asset->bAllowMovement);
        PC->SetIgnoreLookInput(false);              // ⚠️ HECH QACHON true emas
        ErtInputSubsystem->SetContextEnabled("Combat", Asset->bAllowCombat);
    }

    // 2) Pawn almashtirish (EP10 — Turgut)
    if (Asset->OverridePawnClass.IsValid())
        PossessTemporaryPawn(Asset->OverridePawnClass.LoadSynchronous());

    // 3) Sana ko'rsatish — o'yinchi sozlamasiga qarab
    PresentDate(Asset);

    // 4) Dinamik "Oldingi epizodda..." (o'yinchining o'z tanlovlaridan)
    if (Asset->bBuildDynamicRecap)
        RecapBuilder->BuildFromWorldState(WorldStateSubsystem->Snapshot());

    AdvanceBeat();
}

void UErtIntroDirector::PresentDate(const UErtIntroSequenceAsset* A)
{
    const EErtDateFormat Fmt = Settings->GetDateFormat();   // Ikkalasi/Milodiy/Hijriy
    const EErtDateVerbosity V = Settings->GetDateVerbosity();// To'liq/Minimal/Diegetik/Off

    if (V == EErtDateVerbosity::Off) return;

    if (A->DateMode == EErtDateDisplayMode::Diegetic ||
        V == EErtDateVerbosity::Diegetic)
    {
        // Dunyodagi obyektni yoritish (kitoba, tanga, hujjat)
        HighlightDiegeticActor(A->DiegeticDateActorTag);
        return;
    }

    FErtDateCardParams P;
    P.Hijri     = (Fmt != EErtDateFormat::GregorianOnly) ? A->HijriDate     : FString();
    P.Gregorian = (Fmt != EErtDateFormat::HijriOnly)     ? A->GregorianDate : FString();
    P.Place     = (V == EErtDateVerbosity::Full) ? A->PlaceName  : FText();
    P.Sub       = (V == EErtDateVerbosity::Full) ? A->SubCaption : FText();
    P.FadeIn = 1.2f; P.Hold = 4.5f; P.FadeOut = 1.8f;

    HUD->ShowDateCard(P);   // pastki chap, hech qachon markazda
}

void UErtIntroDirector::TickIntro(float DT)
{
    ElapsedSec += DT;
    const FErtIntroBeat& B = ActiveAsset->Beats[CurrentBeat];

    // Yumshoq turtki — o'yinchi qotib qolmasin
    if (B.bRequiresPlayerAction && BeatElapsed > B.NudgeAfterSeconds)
        NudgeSubsystem->SoftNudge(B.BeatTag);   // ovoz, nur, NPC qaraydi

    if (BeatCompleted(B)) AdvanceBeat();
}

void UErtIntroDirector::EndIntro()
{
    // ⚠️ HECH QANDAY "endi o'ynang" signali yo'q.
    // Faqat: boshqaruv cheklovlari olib tashlanadi, HUD sekin paydo bo'ladi.
    GetPlayerController()->SetIgnoreMoveInput(false);
    ErtInputSubsystem->SetContextEnabled("Combat", true);
    HUD->FadeInGameplayHUD(1.5f);
    OnIntroFinished.Broadcast(ActiveAsset->EpisodeId);
}
```

### 4.3 «Oldingi epizodda...» — dinamik rekap ⭐

Bu — **AC'da yo'q, va sizga katta ustunlik beradi**.

Har epizod boshida 25–40 soniyalik montaj, lekin u **oldindan render qilinmagan** — u **o'yinchining o'z o'yin holatidan** quriladi:

```cpp
void UErtRecapBuilder::BuildFromWorldState(const FErtWorldSnapshot& S)
{
    TArray<FErtRecapShot> Shots;

    // 1) Oxirgi epizodning cliffhanger'i — har doim
    Shots.Add(MakeShot(S.LastEpisodeCliffhangerTag));

    // 2) O'yinchi TANLOVLARI — faqat u qilganlari
    if (S.HasFlag("Titus.Spared"))   Shots.Add(MakeShot("Recap.TitusSpared"));
    if (S.HasFlag("Titus.Killed"))   Shots.Add(MakeShot("Recap.TitusKilled"));
    if (S.HasFlag("Nail.Taken"))     Shots.Add(MakeShot("Recap.NailInPocket"));

    // 3) O'lgan personajlar — o'yinchi kimni yo'qotgan bo'lsa
    for (const FName& Fallen : S.FallenCompanions)
        Shots.Add(MakeMemorialShot(Fallen));

    // 4) Hozirgi epizod uchun kerakli kontekst (setup)
    Shots.Append(MakeSetupShots(S.CurrentEpisodeId));

    // 5) Mix holati — EP24 dan keyin HAR rekapda
    if (S.HandPhase != EErtHandPhase::Intact)
        Shots.Add(MakeShot("Recap.TheHand"));   // 2 soniya, qo'l kadri

    Sequencer->PlayProceduralMontage(Shots, /*MaxDurationSec=*/40.f);
}
```

> **Natija:** ikki xil o'yinchi bir xil epizodni boshlaydi, lekin **butunlay boshqa rekap** ko'radi. Bu — 48 epizodli o'yinda hikoyani ushlab turishning eng samarali usuli.

---

## 5. 48 EPIZOD INTRO XARITASI

| EP | Arxetip | Sana usuli | Nima bo'ladi | Davomiylik |
|---|---|---|---|---|
| 01 | 🚶 A | Overlay | Quruq daryodan tepalikka, o'lik podalar | 3:00 |
| 02 | 🚶 A | Diegetik | Kiyik ovi — sabr sinovi | 2:00 |
| 03 | 🌊 D | Overlay | Tush → uyg'onish → hujum | 3:00 |
| 04 | 🎭 E | Ovoz | Kengash — kimga qarash muhim | 4:00 |
| 05 | 👐 B | Overlay | Chodir yig'ish (interaktiv) | 3:00 |
| 06 | 🚶 A | **Diegetik** | Karvonsaroy — toshda «٦٢٦» | 3:00 |
| 07 | 🚶 A | Overlay | Halab bozori tomdan, markersiz | 4:00 |
| 08 | 🚶 A | Overlay | Tuya karvoni cho'lda | 3:00 |
| 09 | 🎭 E | — | Obaga qaytish, farqni topish | 2:00 |
| 10 | 👁 C | Overlay | **Turgut bo'lib o'ynash** | 3:00 |
| 11 | 🌊 D | — | **Suv ostida**, nafas tugayapti | 4:00 |
| 12 | 🏗 F | Overlay | Mancınık arqonini tortish | 5:00 |
| 13 | 🚶 A | Overlay | Jang safi orasida yurish | 4:00 |
| 14 | 🚶 A | — | Sulton ovi, Köpek bilan | 3:00 |
| 15 | 👐 B | Overlay | **Birinchi qoziqni qoqish** | 4:00 |
| 16 | 🚶 A | — | Ochlik lagerida, 3 non | 3:00 |
| 17 | 🚶 A | **Diegetik** | Kutubxona — kotib sanani yozadi | 4:00 |
| 18 | 🕯 G | Overlay | Yaylov, o'g'ilga kamon | 3:00 |
| 19 | 🎭 E | **Ovoz** | **Ziyofat — hamma narsani ko'rish** | 5:00 |
| 20 | 🚶 A | Overlay | Konya tongda, jasadlar orasida | 4:00 |
| 21 | 🚶 A | **Diegetik** | Bo'sh Zazadin Han, stol, Köpek | 4:00 |
| 22 | 🎭 E | — | Qorovulda, ufqda qizil yorug'lik | 3:00 |
| 23 | 🚶 A | Overlay | Osilgan jasad, kech qolgansiz | 4:00 |
| **24** | 🎭 **E** | **Overlay** | 🩸 **Bog'langan holat, faqat kamera** | **6:00** |
| 25 | 🌊 D | Overlay | Qorda yotib, uch marta turishga urinish | 4:00 |
| 26 | 👐 B | — | **Choyni chap qo'l bilan olish** | 5:00 |
| 27 | 🚶 A | — | O'z obangizda begona, o'z qabringiz | 4:00 |
| 28 | 🎭 E | — | Bozorda paizani ko'rish | 3:00 |
| 29 | 🚶 A | — | Dashtda — doira yopilyapti | 4:00 |
| 30 | 🏗 F | Overlay | Devorga tosh ko'tarish | 4:00 |
| 31 | 🚶 A | Overlay | Bagras — 13 yil keyin, mehmon sifatida | 4:00 |
| 32 | 🚶 A | Overlay | **Mo'g'ul lageri ichida erkin yurish** | 6:00 |
| 33 | 🎭 E | — | Harbiy kengash, 40 amir | 4:00 |
| 34 | 🕯 G | — | Oxirgi tinch kecha, musiqasiz | 3:00 |
| 35 | 🎭 E | **Overlay** | ⚔️ Tuman ko'tariladi, 30,000 ko'rinadi | 5:00 |
| 36 | 🚶 A | — | Asirlar navbati — hunaringiz nima? | 5:00 |
| 37 | 👐 B | **Diegetik** | O'lponni sanash, tangada yil | 4:00 |
| 38 | 🚶 A | Overlay | 1,200 kishi yo'lda | 4:00 |
| 39 | 🚶 A | Overlay | Bo'sh Söğüt, eshik qoqish | 4:00 |
| 40 | 👐 B | — | Axiy lodjasi, bepul ovqat | 4:00 |
| 41 | 👐 B | Overlay | Devorni qo'l bilan tekshirish | 5:00 |
| 42 | 🕯 G | — | O'g'il 19 yoshda, kamon | 3:00 |
| 43 | 🚶 A | — | Vizantiya monastyri, tarjimasiz yunoncha | 4:00 |
| 44 | 🕯 G | Overlay | **6 daqiqa tinchlik**, keyin chang | 5:00 |
| 45 | 🚶 A | — | Yomg'ir, xarobada Hamza | 3:00 |
| 46 | 🌊 D | — | Tunda qorda, qadam ritmi | 5:00 |
| 47 | 🎭 E | Overlay | Yonayotgan lager, Noyan mixni ochadi | 4:00 |
| 48 | 🚶 A | **Diegetik** | Qari Ertug'rul Söğütni aylanadi | 5:00 |

**Jami intro vaqti:** ~3 soat 15 daqiqa — barchasi **o'ynaladigan**.

---

## 6. PRODUCTION XARAJATI (video bilan taqqoslash)

| | Pre-rendered video | O'ynaladigan intro |
|---|---|---|
| **Disk hajmi** | ~35 GB (48 × 3 daq × 4K) | ~0 GB |
| **Render vaqti** | ~2,400 soat GPU farm | 0 |
| **Lokalizatsiya (8 til)** | 8 × 35 GB = 280 GB | Faqat VO + subtitr |
| **Patch qilish** | Qayta render + 35 GB patch | Kod/data o'zgarishi (~MB) |
| **Animatsiya ishi** | Alohida sinematik pipeline | Mavjud gameplay animatsiyalari |
| **Mocap** | Alohida sessiya | Umumiy sessiya |
| **Xarajat (baholash)** | ~$1.2M | ~$380K |
| **O'yinchi skip qiladi** | 85% | ~8% |

> **Xulosa: o'ynaladigan intro arzonroq, tezroq, ta'sirchanroq va texnik jihatdan to'g'riroq.** Sizning `.mp4` yondashuvingizni butunlay tashlash kerak.

---

## 7. QO'SHIMCHA: «SINXRONLASH NUQTALARI» (AC'dan)

Har mintaqada 4–7 ta **baland nuqta** (minora, tepalik, daraxt, minora). Chiqsangiz:

1. Kamera aylanadi (panorama) — **lekin o'yinchi boshqaradi**
2. Xarita ochiladi
3. ⭐ **`BilgeGöz` avtomatik 3 ta tarixiy nuqtani belgilaydi** — kodeks beradi
4. Ertug'rul qisqa jumla aytadi (yil va joy haqida)

**AC'dan farqi:** biz `Leap of Faith` (imon sakrashi) **qilmaymiz** — bu tarixiy jihatdan bema'ni. O'rniga: **arqon bilan tushish** yoki **oddiy yo'l**. Realizm ustunligi.
