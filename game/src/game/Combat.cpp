// Jang yadrosi: zarba jadvali va uch resursning mantiqi.
//
// GDD 04_CORE_SYSTEMS — jang OG'IR va JAZOLOVCHI bo'lishi kerak:
// "uch kishi sizni o'ldirishi mumkin, beshtasi — albatta". Shu sababli:
//   · har zarba nafas yeydi, nafas jangda juda sekin tiklanadi (9/s)
//   · blok bepul emas: poza +25 va nafas −15 (uzoq blok o'ldiradi)
//   · og'ir zarba o'z pozangizni ham buzadi (selfPosture)
//   · qo'l (Mix) shikastlangani sari parry oynasi 180 ms dan 110 ms ga qisqaradi
//
// Referens uchburchagi: Ghost of Tsushima (oqim) x Kingdom Come (og'irlik) x
// Sekiro (poza). Poza — asosiy zarba kanali: sog'liq emas, MUVOZANAT sindiriladi,
// keyin yakunlovchi zarba beriladi.
#include "ertugrul/game/Combat.h"

#include <cmath>

namespace ert {

// ---------------------------------------------------------------------------
// Ichki yordamchilar
// ---------------------------------------------------------------------------
namespace {

constexpr float kMaxDt = 0.25f;   // bitta kadrda hisoblanadigan eng katta qadam

inline bool goodF(float v) { return std::isfinite(v); }
inline float sane(float v, float fallback = 0.0f) { return std::isfinite(v) ? v : fallback; }

// Nol bo'lmagan, chekli chegara qiymati (nolga bo'lishdan himoya)
inline float posMax(float v, float fallback) {
    return (std::isfinite(v) && v > 1e-4f) ? v : fallback;
}

// Zarba jadvali — GDD 04_CORE_SYSTEMS.
// Statik umr: attackDef() qaytaradigan havolalar butun o'yin davomida yaroqli
// (EnemyManager::IncomingHit ularga ko'rsatkich saqlaydi).
struct AttackTable {
    Attack light[3];
    Attack heavy;
    Attack kick;
    Attack arrow;
    Attack assassinate;
    Attack fall;
    Attack none;          // noma'lum tur — butunlay zararsiz zaxira

    AttackTable() {
        // --- Yengil kombo 0: tez ochilish ---
        light[0].type = DamageType::LightAttack;
        light[0].damage = 9.0f;   light[0].postureDamage = 12.0f;
        light[0].breathCost = 8.0f; light[0].handCost = 0.0f; light[0].selfPosture = 0.0f;
        light[0].windup = 0.16f;  light[0].active = 0.10f;  light[0].recovery = 0.24f;
        light[0].reach = 2.1f;    light[0].arcDeg = 90.0f;  light[0].knockback = 0.6f;

        // --- Yengil kombo 1: teskari tomondan, biroz uzunroq ---
        light[1].type = DamageType::LightAttack;
        light[1].damage = 10.0f;  light[1].postureDamage = 13.0f;
        light[1].breathCost = 8.0f; light[1].handCost = 0.0f; light[1].selfPosture = 0.0f;
        light[1].windup = 0.13f;  light[1].active = 0.10f;  light[1].recovery = 0.22f;
        light[1].reach = 2.2f;    light[1].arcDeg = 100.0f; light[1].knockback = 0.7f;

        // --- Yengil kombo 2: yakunlovchi — sekin va jazolanadigan ---
        light[2].type = DamageType::LightAttack;
        light[2].damage = 14.0f;  light[2].postureDamage = 16.0f;
        light[2].breathCost = 10.0f; light[2].handCost = 1.0f; light[2].selfPosture = 2.0f;
        light[2].windup = 0.20f;  light[2].active = 0.12f;  light[2].recovery = 0.34f;
        light[2].reach = 2.4f;    light[2].arcDeg = 110.0f; light[2].knockback = 1.4f;

        // --- Og'ir zarba: qalqonni ham silkitadi, lekin o'zingizni ochadi ---
        heavy.type = DamageType::HeavyAttack;
        heavy.damage = 26.0f;     heavy.postureDamage = 30.0f;
        heavy.breathCost = 22.0f; heavy.handCost = 8.0f;    heavy.selfPosture = 5.0f;
        heavy.windup = 0.38f;     heavy.active = 0.14f;     heavy.recovery = 0.42f;
        heavy.reach = 2.6f;       heavy.arcDeg = 80.0f;     heavy.knockback = 3.2f;

        // --- Tepish: zarar kam, lekin qalqonni ochadi (poza 18) ---
        kick.type = DamageType::Kick;
        kick.damage = 4.0f;       kick.postureDamage = 18.0f;
        kick.breathCost = 10.0f;  kick.handCost = 0.0f;     kick.selfPosture = 0.0f;
        kick.windup = 0.14f;      kick.active = 0.10f;      kick.recovery = 0.20f;
        kick.reach = 1.8f;        kick.arcDeg = 60.0f;      kick.knockback = 4.5f;

        // --- Kamon: uzoq, aniq, lekin tortish sekin (windup tashqarida) ---
        arrow.type = DamageType::Arrow;
        arrow.damage = 22.0f;     arrow.postureDamage = 6.0f;
        arrow.breathCost = 18.0f; arrow.handCost = 3.0f;    arrow.selfPosture = 0.0f;
        arrow.windup = 0.0f;      arrow.active = 0.05f;     arrow.recovery = 0.30f;
        arrow.reach = 60.0f;      arrow.arcDeg = 4.0f;      arrow.knockback = 1.0f;

        // --- Yashirin o'ldirish: bir zarbada (999) ---
        assassinate.type = DamageType::Assassinate;
        assassinate.damage = 999.0f; assassinate.postureDamage = 999.0f;
        assassinate.breathCost = 6.0f; assassinate.handCost = 0.0f; assassinate.selfPosture = 0.0f;
        assassinate.windup = 0.35f;  assassinate.active = 0.15f;  assassinate.recovery = 0.55f;
        assassinate.reach = 1.6f;    assassinate.arcDeg = 60.0f;  assassinate.knockback = 0.0f;

        // --- Qulash: zararni chaqiruvchi o'zi hisoblaydi (balandlikka qarab) ---
        fall.type = DamageType::Fall;

        none.type = DamageType::Fall;
    }
};

const AttackTable& table() {
    static const AttackTable t;
    return t;
}

} // namespace

// ---------------------------------------------------------------------------
// Zarba jadvali
// ---------------------------------------------------------------------------
const Attack& attackDef(DamageType t, int combo) {
    const AttackTable& T = table();
    switch (t) {
        case DamageType::LightAttack: {
            int c = combo;
            if (c < 0) c = 0;
            if (c > 2) c = 2;
            return T.light[c];
        }
        case DamageType::HeavyAttack: return T.heavy;
        case DamageType::Kick:        return T.kick;
        case DamageType::Arrow:       return T.arrow;
        case DamageType::Assassinate: return T.assassinate;
        case DamageType::Fall:        return T.fall;
        default:                      return T.none;
    }
}

const char* damageTypeName(DamageType t) {
    switch (t) {
        case DamageType::LightAttack: return "Light Attack";
        case DamageType::HeavyAttack: return "Heavy Attack";
        case DamageType::Kick:        return "Kick";
        case DamageType::Arrow:       return "Arrow";
        case DamageType::Assassinate: return "Assassination";
        case DamageType::Fall:        return "Fall";
        default:                      return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// Vitals — uch resurs
// ---------------------------------------------------------------------------
void Vitals::reset(float hp) {
    if (!goodF(hp) || hp <= 0.0f) hp = 100.0f;

    healthMax = hp;
    health    = hp;

    breathMax = posMax(breathMax, 100.0f);
    breath    = breathMax;

    postureMax = posMax(postureMax, 100.0f);
    posture    = 0.0f;

    // Mix: handCeil EP024 dan keyin 55 ga tushadi va HECH QACHON qaytmaydi,
    // shuning uchun reset uni saqlaydi — faqat joriy butunlik tiklanadi.
    if (!goodF(handCeil)) handCeil = 100.0f;
    handCeil = clampf(handCeil, 0.0f, 100.0f);
    hand     = handCeil;

    staggered = false;
    staggerT  = 0.0f;
    hurtT     = 0.0f;
    blockT    = 0.0f;
    parryT    = 0.0f;
    invulnT   = 0.0f;
}

void Vitals::update(float dt, bool resting, float breathRegenMul, float postureRegenMul) {
    if (!goodF(dt) || dt <= 0.0f) return;
    if (dt > kMaxDt) dt = kMaxDt;

    // Buzilgan (NaN) qiymatlarni jimgina tiklaymiz — halokat bo'lmasin
    breathMax  = posMax(breathMax, 100.0f);
    postureMax = posMax(postureMax, 100.0f);
    healthMax  = posMax(healthMax, 100.0f);
    if (!goodF(health))  health  = 0.0f;
    if (!goodF(breath))  breath  = 0.0f;
    if (!goodF(posture)) posture = 0.0f;
    if (!goodF(hand))    hand    = 0.0f;
    if (!goodF(handCeil)) handCeil = 100.0f;

    // Nafas: jangdan tashqarida tez (26/s), jang ichida ataylab sekin (9/s).
    const float bMul = goodF(breathRegenMul) ? clampf(breathRegenMul, 0.5f, 2.0f) : 1.0f;
    breath = clampf(breath + (resting ? 26.0f : 9.0f) * bMul * dt, 0.0f, breathMax);
    // Poza: tinchlikda 22/s so'nadi, jangda atigi 9/s — bosim to'planib boradi.
    const float pMul = goodF(postureRegenMul) ? clampf(postureRegenMul, 0.5f, 2.0f) : 1.0f;
    posture = clampf(posture - (resting ? 22.0f : 9.0f) * pMul * dt, 0.0f, postureMax);
    // Qo'l butunligi o'z-o'zidan tiklanmaydi (Mix — doimiy narx).
    hand = clampf(hand, 0.0f, clampf(handCeil, 0.0f, 100.0f));

    // Taymerlar
    if (staggerT > 0.0f) { staggerT -= dt; if (staggerT < 0.0f) staggerT = 0.0f; }
    if (hurtT    > 0.0f) { hurtT    -= dt; if (hurtT    < 0.0f) hurtT    = 0.0f; }
    if (parryT   > 0.0f) { parryT   -= dt; if (parryT   < 0.0f) parryT   = 0.0f; }
    if (invulnT  > 0.0f) { invulnT  -= dt; if (invulnT  < 0.0f) invulnT  = 0.0f; }

    if (staggered) {
        // Muvozanat tiklandi — qisqa daxlsizlik beriladi (tepilib yotib qolmaslik uchun)
        if (staggerT <= 0.0f) {
            staggered = false;
            posture   = 0.0f;
            invulnT   = 0.6f;
        }
    } else if (posture >= postureMax) {
        // MUVOZANAT BUZILDI — yakunlovchi zarbaga ochiq
        staggered = true;
        staggerT  = 2.2f;
        posture   = postureMax;
    }
}

bool Vitals::spendBreath(float cost) {
    if (!goodF(cost) || cost <= 0.0f) return true;      // bepul harakat
    breathMax = posMax(breathMax, 100.0f);
    if (!goodF(breath)) breath = 0.0f;
    if (breath < cost) return false;                    // nafas yetmadi — hujum yo'q
    breath = clampf(breath - cost, 0.0f, breathMax);
    return true;
}

float Vitals::parryWindow() const {
    // GDD: sog'lom qo'l — 180 ms, shikastlangan qo'l — 110 ms.
    return 0.11f + 0.07f * handPct();
}

HitResult Vitals::receive(const Attack& a, bool blocked, bool parried, const Vec3& at) {
    HitResult r;
    r.point = Vec3(sane(at.x), sane(at.y), sane(at.z));

    if (!alive()) {                       // allaqachon o'lgan — zarba behuda
        r.outcome = HitOutcome::Miss;
        return r;
    }

    healthMax  = posMax(healthMax, 100.0f);
    breathMax  = posMax(breathMax, 100.0f);
    postureMax = posMax(postureMax, 100.0f);
    if (!goodF(health))  health  = 0.0f;
    if (!goodF(breath))  breath  = 0.0f;
    if (!goodF(posture)) posture = 0.0f;

    const float inDmg  = sane(a.damage);
    const float inPost = sane(a.postureDamage);

    float dmg  = 0.0f;
    float post = 0.0f;

    if (parried) {
        // MUKAMMAL PARRY: zarar yo'q, poza deyarli tegmaydi (GDD: +4).
        // Hujum qiluvchining jazosi chaqiruvchi tomonda hal qilinadi.
        r.outcome = HitOutcome::Parried;
        post      = 4.0f;
        parryT    = 0.0f;                 // oyna yopiladi — bir bosishga bitta parry
    } else if (blocked) {
        // BLOK bepul emas: zarar 25% o'tadi, poza va nafas qattiq yeyiladi.
        r.outcome = HitOutcome::Blocked;
        dmg       = inDmg * 0.25f;
        post      = 25.0f;
        breath    = clampf(breath - 15.0f, 0.0f, breathMax);
    } else if (invulnT > 0.0f) {
        // Stagger'dan chiqishdagi qisqa daxlsizlik — "o'tkazib yubordi" emas, CHETLANDI
        r.outcome = HitOutcome::Dodged;
    } else {
        r.outcome = HitOutcome::Hit;
        dmg       = inDmg;
        post      = inPost * 0.7f;
    }

    if (dmg  < 0.0f) dmg  = 0.0f;
    if (post < 0.0f) post = 0.0f;

    posture = clampf(posture + post, 0.0f, postureMax);
    health  = clampf(health - dmg, 0.0f, healthMax);

    r.damage  = dmg;
    r.posture = post;

    if (health <= 0.0f) {
        health    = 0.0f;
        staggered = false;
        staggerT  = 0.0f;
        r.outcome = HitOutcome::Killed;
    }
    return r;
}


// ---------------------------------------------------------------------------
// Iymon
// ---------------------------------------------------------------------------
// Balans mantiqi: KO'TARILISH mahoratli harakatlardan keladi (parry, yakunlovchi
// zarba, zarba yemasdan tugatilgan to'lqin), TUSHISH esa zarba yeyish va o'limdan.
// Ya'ni Iymon "yig'iladigan ochko" emas, JORIY FORMA ko'rsatkichi — o'yinchi uni
// maksimalga chiqarib qo'yib, keyin beparvo o'ynay olmaydi.
float faithDelta(FaithEvent e) {
    switch (e) {
        case FaithEvent::Finisher:      return  2.0f;
        case FaithEvent::PerfectParry:  return  0.4f;
        case FaithEvent::Assassinate:   return  0.5f;
        case FaithEvent::Kill:          return  0.2f;
        case FaithEvent::FlawlessWave:  return  5.0f;
        case FaithEvent::EpisodeDone:   return  6.0f;
        case FaithEvent::TookHit:       return -0.6f;
        case FaithEvent::Death:         return -8.0f;
        default:                        return  0.0f;
    }
}

const char* faithTierLocKey(FaithTier t) {
    switch (t) {
        case FaithTier::Adashgan: return "ui.faith.adashgan";
        case FaithTier::Shubha:   return "ui.faith.shubha";
        case FaithTier::Sobit:    return "ui.faith.sobit";
        case FaithTier::Sukunat:  return "ui.faith.sukunat";
        default:                  return "ui.faith.shubha";
    }
}

FaithTier Faith::tier() const {
    if (value >= 76.0f) return FaithTier::Sukunat;
    if (value >= 51.0f) return FaithTier::Sobit;
    if (value >= 26.0f) return FaithTier::Shubha;
    return FaithTier::Adashgan;
}

void Faith::add(float d) {
    if (!goodF(d) || d == 0.0f) return;
    if (!goodF(value)) value = 50.0f;
    const float before = value;
    value = clampf(value + d, 0.0f, 100.0f);
    const float actual = value - before;
    if (std::fabs(actual) > 0.05f) {
        flashT     = 1.6f;
        flashDelta = actual;
    }
}

void Faith::event(FaithEvent e) { add(faithDelta(e)); }

void Faith::update(float dt) {
    if (!goodF(dt) || dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f;
    if (!goodF(value)) value = 50.0f;
    if (silenceT  > 0.0f) { silenceT  -= dt; if (silenceT  < 0.0f) silenceT  = 0.0f; }
    if (silenceCd > 0.0f) { silenceCd -= dt; if (silenceCd < 0.0f) silenceCd = 0.0f; }
    if (flashT    > 0.0f) { flashT    -= dt; if (flashT    < 0.0f) flashT    = 0.0f; }
}

void Faith::reset(float v) {
    value      = clampf(goodF(v) ? v : 50.0f, 0.0f, 100.0f);
    silenceT   = 0.0f;
    silenceCd  = 0.0f;
    flashT     = 0.0f;
    flashDelta = 0.0f;
}

// Sukunat: 1.2 s davomida vaqt 0.45x ga sekinlashadi. 9 s sovish vaqti bor,
// aks holda har parryda cheksiz sekinlashuv bo'lib jang qiymatini yo'qotardi.
bool Faith::triggerSilence() {
    if (value < 76.0f || silenceCd > 0.0f || silenceT > 0.0f) return false;
    silenceT  = 1.2f;
    silenceCd = 9.0f;
    return true;
}


float zoneMultiplier(HitZone z) {
    switch (z) {
        case HitZone::Legs:  return 0.55f;
        case HitZone::Head:  return 2.60f;
        default:             return 1.00f;
    }
}

// Dubulg'ali dushmanlar (Sergeant = 1, Elite = 5) boshga tekkanda ham
// bir zarbada o'lmaydi — o'q dubulg'aga tegib sirg'aladi.
bool headshotKills(int enemyKindIndex) {
    return enemyKindIndex != 1 && enemyKindIndex != 5;
}

} // namespace ert
