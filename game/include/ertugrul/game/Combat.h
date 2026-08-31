#pragma once
// Jang yadrosi: uch resurs va zarba jadvali.
//
// GDD (04_CORE_SYSTEMS) bo'yicha jang OG'IR va JAZOLOVCHI:
// uch kishi o'ldirishi mumkin, beshtasi — albatta.
// Referens uchburchagi: Ghost of Tsushima (oqim) x Kingdom Come (og'irlik) x Sekiro (poza).
//
// UCH RESURS:
//   Sog'liq (health)  — 0 ga tushsa o'lim
//   Nafas   (breath)  — har harakat sarflaydi; 0 da hujum qilib bo'lmaydi
//   Poza    (posture) — zarba/blok to'planadi; 100 da MUVOZANAT BUZILADI (stagger)
//   Qo'l    (hand)    — Mix tizimi; parry oynasini va og'ir qurolni cheklaydi
#include <string>
#include "ertugrul/core/Math.h"

namespace ert {

enum class DamageType {
    LightAttack = 0,   // yengil zarba (3 ta kombo)
    HeavyAttack,       // og'ir zarba
    Kick,              // tepish — qalqonni ochadi
    Arrow,             // kamon
    Assassinate,       // yashirin o'ldirish (bir zarbada)
    Fall,              // balandlikdan qulash
    Count
};

// Tana zonasi — o'q qayerga tekkani
enum class HitZone { Legs = 0, Torso, Head };

// Zona ko'paytiruvchisi: oyoq 0.55, ko'krak 1.00, bosh 2.60
float zoneMultiplier(HitZone z);
// Boshga tekkanda bir zarbada o'ladimi (dubulg'ali serjant/elita — yo'q)
bool  headshotKills(int enemyKindIndex);

// Masofa so'nishi: 25 m gacha to'liq, 55 m da 0.55 ga tushadi
inline float arrowFalloff(float d) {
    if (!(d > 25.0f)) return 1.0f;                 // NaN ham shu yerga tushadi
    return clampf(1.0f - 0.45f * (d - 25.0f) / 30.0f, 0.40f, 1.0f);
}

// Yakuniy o'q zarari. charge 0..1, distM metr.
// Tekshiruv: to'la tortilgan, 15 m, ko'krak = 22 (Footman 60 HP -> 3 o'q)
//            to'la tortilgan, 15 m, BOSH   = 57.2 -> headshotKills => darhol o'lim
//            yarim tortilgan, 40 m, oyoq   = 6.3 — deyarli behuda.
// Ya'ni «to'la tort yoki otma» tarbiyasi.
inline float arrowDamage(float base, float charge, HitZone z, float distM) {
    const float ch = clampf(charge, 0.0f, 1.0f);
    return base * (0.35f + 0.65f * ch) * zoneMultiplier(z) * arrowFalloff(distM);
}

// Bitta zarbaning to'liq ta'rifi
struct Attack {
    DamageType type = DamageType::LightAttack;
    float damage        = 0.0f;   // sog'liqqa
    float postureDamage = 0.0f;   // raqib pozasiga
    float breathCost    = 0.0f;   // o'z nafasiga
    float handCost      = 0.0f;   // o'z qo'liga (Mix)
    float selfPosture   = 0.0f;   // o'z pozasiga (og'ir zarba muvozanatni buzadi)
    float windup        = 0.0f;   // zarbagacha (s) — dushman shu payt ogohlantiradi
    float active        = 0.0f;   // zarba faol oynasi (s)
    float recovery      = 0.0f;   // zarbadan keyin (s)
    float reach         = 0.0f;   // metr
    float arcDeg        = 0.0f;   // oldindagi sektor kengligi
    float knockback     = 0.0f;   // m/s
    float duration() const { return windup + active + recovery; }
};

// Zarba jadvali. combo: 0..2 (yengil zarba zanjiri).
const Attack& attackDef(DamageType t, int combo = 0);

// Zarbaning natijasi
enum class HitOutcome { Miss = 0, Hit, Blocked, Parried, Dodged, Killed };

struct HitResult {
    HitOutcome outcome = HitOutcome::Miss;
    float      damage  = 0.0f;
    float      posture = 0.0f;
    Vec3       point{0, 0, 0};
};

// Zarba KUCHI — barcha darajalash uchun yagona manba.
// O'lchangan (Vitals::receive poza zararini 0.7 ga ko'paytirgandan KEYIN):
//   tepish 10.3 | yengil-1 13.2 | yengil-2 14.6 | yengil-3 19.6 | og'ir 36.5
inline float hitImpulse(const HitResult& r) { return r.damage + 0.5f * r.posture; }
inline float hitWeight (const HitResult& r) {
    return clampf((hitImpulse(r) - 8.0f) / 32.0f, 0.35f, 1.0f);
}

// Jangchining resurslari
struct Vitals {
    float health     = 100.0f, healthMax  = 100.0f;
    float breath     = 100.0f, breathMax  = 100.0f;
    float posture    = 0.0f,   postureMax = 100.0f;
    float hand       = 100.0f;              // Mix — qo'l butunligi
    float handCeil   = 100.0f;              // EP024 dan keyin 55 ga tushadi va qaytmaydi

    bool  staggered  = false;               // poza buzilgan — himoyasiz
    float staggerT   = 0.0f;
    float hurtT      = 0.0f;                // zarba yegan qisqa reaksiya
    float blockT     = 0.0f;                // blok ushlab turilgan vaqt
    float parryT     = 0.0f;                // parry oynasi qolgan vaqti
    float invulnT    = 0.0f;                // qisqa daxlsizlik (stagger'dan chiqishda)

    bool  alive() const { return health > 0.0f; }
    float healthPct()  const { return healthMax  > 0 ? clampf(health / healthMax, 0.f, 1.f) : 0.f; }
    float breathPct()  const { return breathMax  > 0 ? clampf(breath / breathMax, 0.f, 1.f) : 0.f; }
    float posturePct() const { return postureMax > 0 ? clampf(posture / postureMax, 0.f, 1.f) : 0.f; }
    float handPct()    const { return clampf(hand / 100.0f, 0.f, 1.f); }

    void reset(float hp = 100.0f);
    // resting = jang qilmayapti (nafas va poza tezroq tiklanadi)
    // breathRegenMul: Iymon darajasidan keladigan nafas tiklanish koeffitsiyenti.
    // Standart argument -> mavjud chaqiruvchilar (Enemy) o'zgarmaydi.
    void update(float dt, bool resting, float breathRegenMul = 1.0f,
                float postureRegenMul = 1.0f);
    bool spendBreath(float cost);           // yetarli bo'lmasa false
    // Parry oynasi: qo'l butunligi pasaysa oyna qisqaradi (GDD: 180ms -> 110ms)
    float parryWindow() const;

    // Zarbani qabul qilish. blocked/parried tashqarida aniqlanadi.
    HitResult receive(const Attack& a, bool blocked, bool parried, const Vec3& at);
};

// Iymon darajalari. Har birining ANIQ mexanik ta'siri bor — bu shunchaki
// "yaxshi/yomon" o'lchagichi emas, jangdagi joriy holatingiz.
enum class FaithTier {
    Adashgan = 0,   //  0..25  — nafas sust, poza sekin so'nadi
    Shubha,         // 26..50  — neytral
    Sobit,          // 51..75  — nafas +50%
    Sukunat         // 76..100 — nafas +50%, poza +50%, VAQT SEKINLASHUVI
};

// Iymonni o'zgartiradigan hodisalar. Qiymatlar bitta joyda — balanslash oson.
enum class FaithEvent {
    Finisher = 0,      // yakunlovchi zarba — jangchining sabri mukofoti
    PerfectParry,      // mukammal parry
    Assassinate,       // yashirin o'ldirish
    Kill,              // oddiy o'ldirish
    FlawlessWave,      // to'lqinni zarba yemasdan tugatish
    EpisodeDone,       // epizod bajarildi
    TookHit,           // zarba yedi (kichik jarima — hisob abadiy o'smaydi)
    Death,             // halok bo'ldi
    Count
};

float      faithDelta(FaithEvent e);
const char* faithTierLocKey(FaithTier t);

// Iymon — meta ko'rsatkich (GDD 04): asirni ozod qilish +8, va'dani buzish -10 ...
struct Faith {
    float value = 50.0f;                    // 0..100
    // «Sukunat» zaryadi: Sukunat darajasida mukammal parry yoki yakunlovchi
    // zarba vaqtni qisqa muddatga sekinlashtiradi. Sovish vaqti bor —
    // shuning uchun uni "yig'ib qo'yib" bo'lmaydi.
    float silenceT    = 0.0f;               // qolgan sekinlashuv vaqti (s)
    float silenceCd   = 0.0f;               // sovish
    float flashT      = 0.0f;               // HUD: o'zgarish porlashi
    float flashDelta  = 0.0f;               // oxirgi o'zgarish (ishorasi bilan)

    void  add(float d);
    void  event(FaithEvent e);
    void  update(float dt);
    void  reset(float v = 50.0f);
    // Sukunat zaryadini ishga tushirish (parry / finisher). Faqat Sukunat darajasida.
    bool  triggerSilence();

    FaithTier tier() const;
    bool  silence() const { return value >= 76.0f; }
    float breathRegenBonus()  const { return value >= 51.0f ? 1.5f : (value < 26.0f ? 0.75f : 1.0f); }
    float postureRegenBonus() const { return value >= 76.0f ? 1.5f : (value < 26.0f ? 0.80f : 1.0f); }
    // O'yin vaqti ko'paytuvchisi: 1.0 = normal, < 1 = sekinlashuv
    float timeScale() const { return (silenceT > 0.0f) ? 0.45f : 1.0f; }
};

const char* damageTypeName(DamageType t);

} // namespace ert
