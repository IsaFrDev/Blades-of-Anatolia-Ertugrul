// Dushmanlar: statistika jadvali, sezish (ko'rish + eshitish) va jang AI si.
//
// GDD 04_CORE_SYSTEMS — jang OG'IR: "uch kishi sizni o'ldirishi mumkin,
// beshtasi — albatta". Bu yerdagi ikki qaror shu og'irlikni beradi:
//
//   1. HUJUM SLOTI (Assassin's Creed uslubi). Bir vaqtda faqat IKKI yaqin
//      jangchi zarba beradi, qolganlari aylanib bosim o'tkazadi. Bu jangni
//      "adolatli" qilmaydi — o'qiladigan qiladi: har zarbani ko'rasiz, lekin
//      uchtasidan qochib ulgurmaysiz.
//   2. OGOHLANTIRISH OYNASI. Windup davomida dushman ochiq ravishda tayyorlanadi
//      (App bosh ustida qizil belgi chizadi) — parry oynasi atigi 110..180 ms.
//
// Sezish AC modelidagi kabi ikki kanal:
//   KO'RISH  — konus (yarim burchak + masofa) + devor orqali nur tekshiruvi;
//              cho'kkalagan o'yinchi 45% kamroq masofadan ko'rinadi
//   ESHITISH — Character chiqargan shovqin hodisalari
#include "ertugrul/game/Enemy.h"

#include "ertugrul/game/Character.h"
#include "ertugrul/world/Physics.h"
#include "ertugrul/gfx/Mesh.h"

#include <cmath>
#include <cstring>
#include <vector>

namespace ert {

// ===========================================================================
//  Ichki yordamchilar
// ===========================================================================
namespace {

constexpr float kMaxDt      = 0.10f;   // AI uchun kadr qadami cheklovi
constexpr float kBodyRadius = 0.42f;   // yon to'qnashuv radiusi
constexpr float kTurnRate   = 8.0f;    // yaw yaqinlashuv tezligi (1/s)
constexpr float kBigF       = 1.0e8f;

// Hujum slotlari: bir vaqtda nechta dushman zarba berishi mumkin.
// Yaqin jang uchun ATIGI 2 (AC uslubi). Kamonchilar boshqa o'qda bosim
// o'tkazadi, shuning uchun ular alohida (ham 2 ta) slotga ega — aks holda
// ikki piyoda yaqinlashsa butun kamonchilar guruhi jim bo'lib qolardi.
constexpr int kMeleeSlots  = 2;
constexpr int kRangedSlots = 2;

// Holat vaqtlari
constexpr float kSuspiciousWait = 1.5f;
constexpr float kSearchTime     = 4.0f;
constexpr float kBlockTime      = 0.8f;
constexpr float kHurtTime       = 0.35f;

inline bool goodF(float v) { return std::isfinite(v) && v > -kBigF && v < kBigF; }
inline bool goodV(const Vec3& v) { return goodF(v.x) && goodF(v.y) && goodF(v.z); }

// Umumiy determinatsiyalangan tasodif manbayi (patrul nuqtalari, blok tanlovi).
Rng& rng() {
    static Rng r(0x51ED2701u);
    return r;
}
inline float rnd01() { return rng().nextFloat(); }
inline float rndRange(float a, float b) { return a + rnd01() * (b - a); }

// ---------------------------------------------------------------------------
// Dushmanning qo'shimcha ish holati.
//
// Enemy.h — KONTRAKT, unga maydon qo'sha olmaymiz. Patrul radiusi (EnemySpawn
// dan keladi), hujum sloti ruxsati (EnemyManager beradi) va o'yinchi hujumining
// "qirrasi" shu yon jadvalda saqlanadi. Kalit — Enemy* ko'rsatkichi.
// std::vector<Enemy> qayta joylashganda EnemyManager::spawn() jadvalni
// rtRemap() bilan yangi manzillarga ko'chiradi, shuning uchun osilgan
// ko'rsatkich qolmaydi.
// ---------------------------------------------------------------------------
struct Rt {
    const Enemy* owner        = nullptr;
    float        patrolRadius = 0.0f;
    bool         mayAttack    = true;    // hujum sloti ruxsati
    bool         prevPlayerAtk = false;  // o'yinchi hujumi qirrasini aniqlash uchun
    // Zarbadan orqaga surish (Attack::knockback butun loyihada o'lik maydon edi)
    float        pushVX = 0.0f, pushVZ = 0.0f, pushT = 0.0f;
};

std::vector<Rt>& rtTable() {
    static std::vector<Rt> t;
    return t;
}

Rt& rtFor(const Enemy* e) {
    std::vector<Rt>& t = rtTable();
    for (size_t i = 0; i < t.size(); ++i)
        if (t[i].owner == e) return t[i];
    if (t.size() >= 512) {              // cheksiz o'sishdan himoya
        t[0] = Rt();
        t[0].owner = e;
        return t[0];
    }
    Rt s;
    s.owner = e;
    t.push_back(s);
    return t.back();
}

// vector<Enemy> qayta joylashdi — eski manzillarni yangisiga ko'chiramiz
void rtRemap(const Enemy* oldBase, const Enemy* newBase, size_t n) {
    if (oldBase == nullptr || newBase == nullptr || oldBase == newBase || n == 0) return;
    std::vector<Rt>& t = rtTable();
    for (size_t i = 0; i < t.size(); ++i) {
        const Enemy* o = t[i].owner;
        if (o == nullptr) continue;
        if (o >= oldBase && o < oldBase + n)
            t[i].owner = newBase + (o - oldBase);
    }
}

// Manager tozalanganda shu diapazondagi yozuvlarni olib tashlaymiz
void rtDropRange(const Enemy* base, size_t n) {
    if (base == nullptr || n == 0) return;
    std::vector<Rt>& t = rtTable();
    for (size_t i = 0; i < t.size(); ) {
        const Enemy* o = t[i].owner;
        if (o != nullptr && o >= base && o < base + n) {
            t[i] = t.back();
            t.pop_back();
        } else {
            ++i;
        }
    }
}

// ---------------------------------------------------------------------------
// Statistika jadvali (GDD VII Bestiary)
// ---------------------------------------------------------------------------
const EnemyStats* statsTable() {
    static EnemyStats t[(int)EnemyKind::Count];
    static bool built = false;
    if (built) return t;
    built = true;

    // Footman — oddiy piyoda. Yakka holda zaif, uchtasi bilan o'lasiz.
    {
        EnemyStats& s = t[(int)EnemyKind::Footman];
        s.health = 55.0f;  s.breath = 80.0f;  s.posture = 90.0f;
        s.moveSpeed = 2.4f; s.chaseSpeed = 4.4f;
        s.sightRange = 24.0f; s.sightAngle = 60.0f; s.hearRange = 16.0f;
        s.attackRange = 2.2f; s.aggression = 0.55f; s.blockChance = 0.20f;
        s.scale = 1.80f;
        s.model = "assets/models/crusader/crusader.obj";
        s.tint[0] = 0.80f; s.tint[1] = 0.82f; s.tint[2] = 0.86f;
    }
    // Sergeant — qalqonli. Yengil zarba o'tmaydi: TEPING, keyin uring.
    {
        EnemyStats& s = t[(int)EnemyKind::Sergeant];
        s.health = 85.0f;  s.breath = 90.0f;  s.posture = 130.0f;
        s.moveSpeed = 2.0f; s.chaseSpeed = 3.8f;
        s.sightRange = 22.0f; s.sightAngle = 55.0f; s.hearRange = 16.0f;
        s.attackRange = 2.3f; s.aggression = 0.40f; s.blockChance = 0.55f;
        s.scale = 1.90f;
        s.model = "assets/models/crusader/crusader.obj";
        s.tint[0] = 0.62f; s.tint[1] = 0.66f; s.tint[2] = 0.74f;
    }
    // Crossbow — uzoqdan otadi, yaqinda deyarli himoyasiz.
    {
        EnemyStats& s = t[(int)EnemyKind::Crossbow];
        s.health = 45.0f;  s.breath = 70.0f;  s.posture = 70.0f;
        s.moveSpeed = 2.2f; s.chaseSpeed = 3.6f;
        s.sightRange = 32.0f; s.sightAngle = 45.0f; s.hearRange = 14.0f;
        s.attackRange = 14.0f; s.aggression = 0.30f; s.blockChance = 0.05f;
        s.scale = 1.78f;
        s.model = "assets/models/crusader/crusader.obj";
        s.tint[0] = 0.86f; s.tint[1] = 0.80f; s.tint[2] = 0.66f;
    }
    // Assassin — tez, kam sog'liq, keng ko'rish burchagi.
    {
        EnemyStats& s = t[(int)EnemyKind::Assassin];
        s.health = 40.0f;  s.breath = 85.0f;  s.posture = 70.0f;
        s.moveSpeed = 3.4f; s.chaseSpeed = 5.6f;
        s.sightRange = 20.0f; s.sightAngle = 70.0f; s.hearRange = 20.0f;
        s.attackRange = 2.0f; s.aggression = 0.85f; s.blockChance = 0.10f;
        s.scale = 1.74f;
        s.model = "assets/models/characters/character-h.obj";
        s.tint[0] = 0.55f; s.tint[1] = 0.52f; s.tint[2] = 0.58f;
    }
    // HorseArcher — mo'g'ul otliq kamonchi (S3+): tez va uzoqdan uradi.
    {
        EnemyStats& s = t[(int)EnemyKind::HorseArcher];
        s.health = 70.0f;  s.breath = 80.0f;  s.posture = 90.0f;
        s.moveSpeed = 4.0f; s.chaseSpeed = 6.4f;
        s.sightRange = 30.0f; s.sightAngle = 50.0f; s.hearRange = 18.0f;
        s.attackRange = 12.0f; s.aggression = 0.50f; s.blockChance = 0.10f;
        s.scale = 1.86f;
        s.model = "assets/models/ottoman/ottoman.obj";
        s.tint[0] = 0.88f; s.tint[1] = 0.74f; s.tint[2] = 0.55f;
    }
    // Elite — keshikten gvardiya (S4): og'ir, bloki kuchli, pozasi katta.
    {
        EnemyStats& s = t[(int)EnemyKind::Elite];
        s.health = 120.0f; s.breath = 110.0f; s.posture = 160.0f;
        s.moveSpeed = 2.6f; s.chaseSpeed = 4.8f;
        s.sightRange = 26.0f; s.sightAngle = 60.0f; s.hearRange = 18.0f;
        s.attackRange = 2.4f; s.aggression = 0.65f; s.blockChance = 0.45f;
        s.scale = 1.94f;
        s.model = "assets/models/crusader/crusader.obj";
        s.tint[0] = 0.48f; s.tint[1] = 0.52f; s.tint[2] = 0.60f;
    }
    // Deer — kiyik. Ov bosqichi nishoni: hujum qilmaydi, ko'rsa/eshitsa qochadi.
    // Faqat kamon bilan yetib bo'ladi (qochish tezligi yugurishdan yuqori).
    {
        EnemyStats& s = t[(int)EnemyKind::Deer];
        s.health = 34.0f;  s.breath = 200.0f; s.posture = 40.0f;
        s.moveSpeed = 1.4f; s.chaseSpeed = 7.4f;
        s.sightRange = 30.0f; s.sightAngle = 120.0f; s.hearRange = 26.0f;
        s.attackRange = 0.0f; s.aggression = 0.0f; s.blockChance = 0.0f;
        s.scale = 1.55f;
        s.model = "assets/models/nature/deer.obj";
        s.tint[0] = 1.0f; s.tint[1] = 0.95f; s.tint[2] = 0.85f;
        s.flees = true;
    }
    return t;
}

inline int kindIndex(EnemyKind k) {
    const int i = (int)k;
    return (i >= 0 && i < (int)EnemyKind::Count) ? i : 0;
}

inline bool isRanged(EnemyKind k) {
    return k == EnemyKind::Crossbow || k == EnemyKind::HorseArcher;
}

// ---------------------------------------------------------------------------
// Geometriya yordamchilari
// ---------------------------------------------------------------------------

// from -> to yo'nalishidagi yaw. Nuqtalar ustma-ust bo'lsa false qaytaradi.
bool aimYaw(const Vec3& from, const Vec3& to, float& outYaw) {
    Vec3 d = to - from;
    d.y = 0.0f;
    if (!goodV(d) || lengthSq(d) < 1e-8f) return false;
    outYaw = yawFromDir(normalize(d));
    return goodF(outYaw);
}

// a nuqtasi 'yaw' ga qaragan jismning oldidan necha gradus chetda? (0..180)
float offAngle(const Vec3& from, float yawDeg, const Vec3& to) {
    float y = 0.0f;
    if (!aimYaw(from, to, y)) return 0.0f;
    return std::fabs(wrapAngleDeg(y - yawDeg));
}

// Oyoq ostidagi tayanch balandligi (dunyo yo'q bo'lsa — nol sathi)
float supportY(const PhysicsWorld* w, float x, float z, float fromY) {
    if (w == nullptr) return 0.0f;
    if (!goodF(x) || !goodF(z) || !goodF(fromY)) return 0.0f;
    float g = w->supportBelow(x, z, fromY);
    if (!goodF(g)) {
        g = w->groundAt(x, z);
        if (!goodF(g)) g = 0.0f;
    }
    return g;
}

// Nishonga bir qadam siljitadi: to'qnashuv + yerga yopishish.
// Qaytaradi — shu kadrdagi haqiqiy tezlik (m/s).
float stepToward(Vec3& pos, const Vec3& dst, float speed, float dt,
                 const PhysicsWorld* w, float bodyH) {
    if (!goodV(pos) || !goodV(dst) || !goodF(speed) || !goodF(dt)) return 0.0f;
    if (dt <= 0.0f || speed <= 0.0f) return 0.0f;

    Vec3 d = dst - pos;
    d.y = 0.0f;
    const float len = length(d);
    if (!goodF(len) || len < 1e-4f) return 0.0f;

    float step = speed * dt;
    if (step > len) step = len;

    const Vec3 dir = d / len;
    Vec3 np(pos.x + dir.x * step, pos.y, pos.z + dir.z * step);

    if (w != nullptr) w->resolve(np, kBodyRadius, bodyH);
    if (!goodV(np)) return 0.0f;

    np.y = supportY(w, np.x, np.z, pos.y + 0.6f);
    if (!goodF(np.y)) np.y = pos.y;

    const float moved = distanceXZ(pos, np);
    pos = np;
    return (dt > 1e-5f) ? (moved / dt) : 0.0f;
}

} // namespace

// ===========================================================================
//  Statistika va nomlar
// ===========================================================================
const EnemyStats& enemyStats(EnemyKind k) {
    return statsTable()[kindIndex(k)];
}

const char* enemyKindName(EnemyKind k) {
    switch (k) {
        case EnemyKind::Footman:     return "Footman";
        case EnemyKind::Sergeant:    return "Sergeant";
        case EnemyKind::Crossbow:    return "Crossbowman";
        case EnemyKind::Assassin:    return "Assassin";
        case EnemyKind::HorseArcher: return "Horse Archer";
        case EnemyKind::Elite:       return "Elite Guard";
        case EnemyKind::Deer:        return "Deer";
        default:                     return "Unknown";
    }
}

const char* enemyKindLocKey(EnemyKind k) {
    switch (k) {
        case EnemyKind::Footman:     return "ui.enemy.footman";
        case EnemyKind::Sergeant:    return "ui.enemy.sergeant";
        case EnemyKind::Crossbow:    return "ui.enemy.crossbow";
        case EnemyKind::Assassin:    return "ui.enemy.assassin";
        case EnemyKind::HorseArcher: return "ui.enemy.horse_archer";
        case EnemyKind::Elite:       return "ui.enemy.elite";
        case EnemyKind::Deer:        return "ui.enemy.deer";
        default:                     return "ui.enemy.footman";
    }
}

EnemyKind enemyKindFromName(const std::string& s) {
    // Kichik harfga keltiramiz va ajratuvchilarni tashlaymiz: "Horse Archer",
    // "horse_archer", "horse-archer" — hammasi bir xil tushuniladi.
    std::string k;
    k.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = (unsigned char)s[i];
        if (c == ' ' || c == '_' || c == '-') continue;
        k.push_back((char)((c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c));
    }
    if (k == "deer" || k == "kiyik" || k == "geyik")            return EnemyKind::Deer;
    if (k == "footman"  || k == "soldier" || k == "piyoda")   return EnemyKind::Footman;
    if (k == "sergeant" || k == "shield"  || k == "serjant")  return EnemyKind::Sergeant;
    if (k == "crossbow" || k == "crossbowman" || k == "archer" || k == "arbalet")
        return EnemyKind::Crossbow;
    if (k == "assassin" || k == "qotil")                      return EnemyKind::Assassin;
    if (k == "horsearcher" || k == "rider" || k == "otliq")   return EnemyKind::HorseArcher;
    if (k == "elite" || k == "keshikten" || k == "guard")     return EnemyKind::Elite;
    return EnemyKind::Footman;
}

// ===========================================================================
//  Enemy
// ===========================================================================
bool Enemy::init(EnemyKind kind, PhysicsWorld* world) {
    kind_  = (int)kind >= 0 && (int)kind < (int)EnemyKind::Count ? kind : EnemyKind::Footman;
    st_    = enemyStats(kind_);
    world_ = world;

    // Vitals ni turga moslaymiz
    vitals.breathMax  = (st_.breath  > 1.0f) ? st_.breath  : 80.0f;
    vitals.postureMax = (st_.posture > 1.0f) ? st_.posture : 100.0f;
    vitals.reset(st_.health);

    state_        = EnemyState::Idle;
    stateT_       = 0.0f;
    alert_        = 0.0f;
    patrolT_      = 0.0f;
    decideT_      = 0.0f;
    combo_        = 0;
    strikeFired_  = false;
    strikePending_= false;
    speed_        = 0.0f;
    circleDir_    = (rng().next() & 1u) ? 1.0f : -1.0f;

    Mesh* m = Mesh::get(st_.model ? st_.model : "");
    if (m == nullptr) return false;              // model yo'q — dushman baribir ishlaydi
    if (!model_.init(m)) return false;
    model_.setClip(AnimClip::Idle, 0.0f);
    return true;
}

void Enemy::reset(const Vec3& pos, float yaw) {
    Vec3 p = goodV(pos) ? pos : Vec3(0.0f, 0.0f, 0.0f);
    p.y = supportY(world_, p.x, p.z, p.y + 1.0f);
    if (!goodF(p.y)) p.y = 0.0f;

    pos_    = p;
    home_   = p;
    target_ = p;

    const float y = goodF(yaw) ? wrapAngleDeg(yaw) : 0.0f;
    yaw_       = y;
    targetYaw_ = y;

    speed_        = 0.0f;
    state_        = EnemyState::Idle;
    stateT_       = 0.0f;
    { Rt& rk = rtFor(this); rk.pushVX = 0.0f; rk.pushVZ = 0.0f; rk.pushT = 0.0f; }
    alert_        = 0.0f;
    patrolT_      = rndRange(0.5f, 2.0f);
    decideT_      = 0.0f;
    combo_        = 0;
    strikeFired_  = false;
    strikePending_= false;
    circleDir_    = (rng().next() & 1u) ? 1.0f : -1.0f;

    vitals.breathMax  = (st_.breath  > 1.0f) ? st_.breath  : 80.0f;
    vitals.postureMax = (st_.posture > 1.0f) ? st_.posture : 100.0f;
    vitals.reset(st_.health);

    Rt& rt = rtFor(this);
    rt.mayAttack     = true;
    rt.prevPlayerAtk = false;

    model_.setClip(AnimClip::Idle, 0.0f);
}

Vec3 Enemy::headPos() const {
    const float h = (st_.scale > 0.2f && goodF(st_.scale)) ? st_.scale : 1.8f;
    return Vec3(pos_.x, pos_.y + h * 0.95f, pos_.z);
}

const Attack& Enemy::currentAttack() const {
    int c = combo_;
    if (c < 0) c = 0;
    if (c > 2) c = 2;

    switch (kind_) {
        case EnemyKind::Crossbow:
        case EnemyKind::HorseArcher:
            return attackDef(DamageType::Arrow, 0);
        case EnemyKind::Sergeant:
            // Qalqonli serjant zanjirni og'ir zarba bilan yakunlaydi
            return (c >= 2) ? attackDef(DamageType::HeavyAttack, 0)
                            : attackDef(DamageType::LightAttack, c);
        case EnemyKind::Elite:
            // Elita ikkinchi zarbani og'ir qiladi — o'qish qiyinroq
            return (c == 1) ? attackDef(DamageType::HeavyAttack, 0)
                            : attackDef(DamageType::LightAttack, c);
        default:
            return attackDef(DamageType::LightAttack, c);
    }
}

bool Enemy::consumeStrike() {
    if (!strikePending_) return false;
    strikePending_ = false;
    return true;
}

// ---------------------------------------------------------------------------
// Sezish
// ---------------------------------------------------------------------------
bool Enemy::canSee(const Character& player) const {
    if (!vitals.alive() || state_ == EnemyState::Dead) return false;
    if (player.dead()) return false;

    const Vec3 pp = player.position();
    if (!goodV(pp) || !goodV(pos_)) return false;

    const float d = distanceXZ(pos_, pp);
    if (!goodF(d)) return false;

    // Profil: cho'kkalagan o'yinchi juda kam ko'rinadi, past profil ham yordam beradi
    float range = (st_.sightRange > 0.1f) ? st_.sightRange : 20.0f;
    const MoveState ms = player.state();
    const bool crouching = (ms == MoveState::CrouchIdle || ms == MoveState::CrouchWalk ||
                            ms == MoveState::Slide);
    if (crouching)                          range *= 0.55f;
    else if (player.profile() == Profile::Low) range *= 0.85f;

    if (d > range) return false;
    if (d < 0.35f) return true;             // burun tagida — burchak ahamiyatsiz

    // Ko'rish konusi (sightAngle — YARIM burchak)
    if (offAngle(pos_, yaw_, pp) > st_.sightAngle) return false;

    // Devor orqali ko'rmasin: ko'krak balandligida nur tashlaymiz
    if (world_ != nullptr) {
        Vec3 dir = pp - pos_;
        dir.y = 0.0f;
        if (lengthSq(dir) > 1e-8f) {
            dir = normalize(dir);
            const float chest = pos_.y + ((st_.scale > 0.2f) ? st_.scale * 0.62f : 1.1f);
            RayHit h;
            if (world_->rayXZ(Vec3(pos_.x, chest, pos_.z), dir, d - 0.30f, chest, h) && h.hit)
                return false;
        }
    }
    return true;
}

void Enemy::hearNoise(const Vec3& at, float radius, float strength) {
    if (!vitals.alive() || state_ == EnemyState::Dead) return;
    if (!goodV(at) || !goodF(radius) || !goodF(strength)) return;
    if (radius <= 0.0f || strength <= 0.0f) return;

    // Shovqin radiusi ham, dushmanning eshitish qobiliyati ham chegaralaydi
    float r = radius;
    if (st_.hearRange > 0.1f && r > st_.hearRange) r = st_.hearRange;

    const float d = distanceXZ(pos_, at);
    if (!goodF(d) || d >= r) return;

    alert_ = clampf(alert_ + strength * 0.5f, 0.0f, 1.0f);
    target_ = at;                        // shovqin joyiga boradi
}

// ---------------------------------------------------------------------------
// Zarba qabul qilish
// ---------------------------------------------------------------------------
HitResult Enemy::receiveHit(const Attack& a, const Vec3& from, bool playerBehind) {
    const float h = (st_.scale > 0.2f && goodF(st_.scale)) ? st_.scale : 1.8f;
    const Vec3 hitPoint(pos_.x, pos_.y + h * 0.6f, pos_.z);

    HitResult r;
    r.point = hitPoint;

    if (!vitals.alive() || state_ == EnemyState::Dead) {
        r.outcome = HitOutcome::Miss;
        return r;
    }

    // --- Yashirin o'ldirish: orqadan va sezmagan holda — bir zarbada ---
    if (a.type == DamageType::Assassinate && playerBehind && !aware()) {
        killOutright();
        r.outcome = HitOutcome::Killed;
        r.damage  = vitals.healthMax;
        r.posture = vitals.postureMax;
        alert_    = 1.0f;
        return r;
    }

    // Blok: TEPISH qalqonni ochadi — uni blok qilib bo'lmaydi (GDD)
    // Blok: TEPISH qalqonni ochadi (GDD) va blok faqat OLDINGI 120 gradus
    // sektorda ishlaydi — ilgari burchak umuman tekshirilmasdi va qalqonli
    // serjant ORQADAN kelgan zarbani ham to'sardi.
    const bool blocked = (state_ == EnemyState::Block) &&
                         (a.type != DamageType::Kick) &&
                         (!goodV(from) || offAngle(pos_, yaw_, from) <= 60.0f);

    r = vitals.receive(a, blocked, false, hitPoint);

    // Zarba burchagi HOZIRGI yaw_ ga nisbatan. Quyida targetYaw_ urgan tomonga
    // buriladi, lekin poza zarba TEKKAN LAHZAdagi holatdan kelib chiqishi kerak.
    {
        float hd = 0.0f;
        const Vec3 dv{from.x - pos_.x, 0.0f, from.z - pos_.z};
        if (lengthSq(dv) > 1.0e-4f) hd = wrapAngleDeg(yawFromDir(dv) - yaw_);
        model_.setHitDir(hd, hitWeight(r));
    }

    // ORQAGA SURISH. Attack::knockback butun loyihada FAQAT o'yinchi tomonida
    // o'qilardi — dushman zarba yeganda HAYKAL edi.
    // Masofa = v0*(1-e^(-9T))/9:  yengil 0.21 m | 3-kombo 0.27 | og'ir 0.42 | tepish 0.52
    if (r.outcome == HitOutcome::Hit || r.outcome == HitOutcome::Blocked ||
        r.outcome == HitOutcome::Killed) {
        Vec3 away{pos_.x - from.x, 0.0f, pos_.z - from.z};
        const float L = length(away);
        away = (L > 1.0e-3f) ? Vec3{away.x / L, 0.0f, away.z / L}
                             : dirFromYaw(yaw_ + 180.0f);
        const float kb = goodF(a.knockback) ? clampf(a.knockback, 0.0f, 8.0f) : 0.0f;
        float v0 = clampf(2.4f + 0.62f * kb, 1.6f, 5.2f);
        float T  = clampf(0.10f + 0.036f * kb, 0.10f, 0.26f);
        if (r.outcome == HitOutcome::Blocked) { v0 *= 0.45f; T *= 0.45f; }
        else if (vitals.staggered)            { v0 *= 1.35f; T *= 1.35f; }
        Rt& rk = rtFor(this);
        rk.pushVX = away.x * v0;  rk.pushVZ = away.z * v0;  rk.pushT = T;
    }

    // Zarba yegan dushman ALBATTA ogohlanadi va urgan tomonga buriladi
    alert_  = 1.0f;
    if (goodV(from)) {
        target_ = from;
        float y = 0.0f;
        if (aimYaw(pos_, from, y)) targetYaw_ = y;
    }

    if (!vitals.alive()) {
        state_         = EnemyState::Dead;
        stateT_        = 0.0f;
        speed_         = 0.0f;
        strikePending_ = false;
        strikeFired_   = false;
        model_.setClip(AnimClip::Death, 0.10f);
    } else if (vitals.staggered) {
        state_         = EnemyState::Stagger;
        stateT_        = 0.0f;
        speed_         = 0.0f;
        strikePending_ = false;
        strikeFired_   = false;
    } else if (r.outcome == HitOutcome::Hit) {
        state_         = EnemyState::Hurt;
        stateT_        = 0.0f;
        speed_         = 0.0f;
        strikePending_ = false;
        strikeFired_   = false;
        vitals.hurtT   = kHurtTime;
    }
    // Blocked / Parried / Dodged holatida dushman o'z holatida qoladi
    return r;
}

bool Enemy::assassinable(const Vec3& playerPos, float playerYaw) const {
    if (!vitals.alive() || state_ == EnemyState::Dead) return false;
    if (aware()) return false;                       // sezgan raqibni pichoqlab bo'lmaydi
    if (!goodV(playerPos) || !goodF(playerYaw)) return false;

    const float d = distanceXZ(pos_, playerPos);
    if (!goodF(d) || d >= 1.8f) return false;

    // O'yinchi dushmanning ORQA sektorida bo'lishi kerak (120 gradusdan keng)
    if (offAngle(pos_, yaw_, playerPos) <= 120.0f) return false;

    // Va o'yinchi nishonga qarab turishi kerak
    if (offAngle(playerPos, playerYaw, pos_) > 75.0f) return false;
    return true;
}

// O'q urilishi. Qilich zarbasidan uch farqi bor:
//   1) tana ZONASI muhim (bosh 2.6x, oyoq 0.55x) va masofa zararni so'ndiradi
//   2) o'ldirsa dushman QICHQIRMAYDI — alert_ ga tegilmaydi, qo'shnilar bilmaydi
//   3) tirik qolsa O'Q KELGAN tomonga qaraydi (o'yinchining hozirgi joyiga emas)
HitResult Enemy::receiveArrow(const Attack& a, const Vec3& from, HitZone zone,
                              float charge, float distM, bool silent) {
    const float h = (st_.scale > 0.2f) ? st_.scale : 1.8f;
    HitResult r;
    r.point = Vec3{pos_.x, pos_.y + h * 0.6f, pos_.z};
    if (!vitals.alive() || state_ == EnemyState::Dead) {
        r.outcome = HitOutcome::Miss;
        return r;
    }

    // Qalqonli serjant: ogoh va o'yinchiga qaragan bo'lsa o'q qalqonga tegadi.
    // Determinatsiyalangan (tasodif emas) — o'yinchi buni o'rganib, yon tomondan otadi.
    bool shielded = false;
    if (kind_ == EnemyKind::Sergeant && aware() && offAngle(pos_, yaw_, from) < 55.0f)
        shielded = true;

    // Bosh: dubulg'asiz dushmanni bir o'q o'ldiradi
    if (zone == HitZone::Head && !shielded && headshotKills((int)kind_)) {
        killOutright();
        r.outcome = HitOutcome::Killed;
        r.damage  = vitals.healthMax;
        return r;                       // alert_ ga TEGMAYMIZ — jimgina yiqiladi
    }

    // Zarba burchagi — poza uni ko'rsatishi uchun
    {
        float hd = 0.0f;
        const Vec3 dv{from.x - pos_.x, 0.0f, from.z - pos_.z};
        if (lengthSq(dv) > 1e-4f) hd = wrapAngleDeg(yawFromDir(dv) - yaw_);
        model_.setHitDir(hd, clampf(0.35f + charge * 0.5f, 0.35f, 1.0f));
    }

    Attack shot = a;
    shot.damage = arrowDamage(a.damage, charge, zone, distM);
    if (shielded) { shot.damage *= 0.15f; shot.postureDamage = 10.0f; }

    r = vitals.receive(shot, false, false, r.point);

    if (!vitals.alive()) {
        killOutright();
        r.outcome = HitOutcome::Killed;
        return r;
    }

    // Tirik qoldi — ogohlanadi, LEKIN o'q kelgan tomonga qaraydi
    alert_ = 1.0f;
    if (goodV(from)) {
        target_ = from;
        float y = 0.0f;
        if (aimYaw(pos_, from, y)) targetYaw_ = y;
    }
    if (vitals.staggered) {
        state_ = EnemyState::Stagger; stateT_ = 0.0f; speed_ = 0.0f;
        strikePending_ = false; strikeFired_ = false;
    }
    (void)silent;
    return r;
}

// Qat'iy o'ldirish. Yashirin o'ldirish ham, yakunlovchi zarba ham shu yerdan o'tadi.
// alert_ ga TEGMAYDI — chaqiruvchi kerak bo'lsa o'zi o'rnatadi.
void Enemy::killOutright() {
    vitals.health    = 0.0f;
    vitals.posture   = vitals.postureMax;
    vitals.staggered = false;
    vitals.staggerT  = 0.0f;
    vitals.invulnT   = 0.0f;
    state_         = EnemyState::Dead;
    stateT_        = 0.0f;
    speed_         = 0.0f;
    strikePending_ = false;
    strikeFired_   = false;
    model_.setClip(AnimClip::Death, 0.10f);
}

// ---------------------------------------------------------------------------
// Holat mashinasi
// ---------------------------------------------------------------------------
void Enemy::update(const Character& player, float dt) {
    if (!goodF(dt)) return;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > kMaxDt) dt = kMaxDt;
    if (!goodV(pos_)) pos_ = home_;
    if (!goodV(pos_)) pos_ = Vec3(0.0f, 0.0f, 0.0f);
    if (!goodF(yaw_)) yaw_ = 0.0f;
    if (!goodF(targetYaw_)) targetYaw_ = yaw_;
    if (!goodF(alert_)) alert_ = 0.0f;

    const float bodyH = (st_.scale > 0.2f && goodF(st_.scale)) ? st_.scale : 1.8f;

    // ---------------- O'LIM ----------------
    if (!vitals.alive() || state_ == EnemyState::Dead) {
        if (state_ != EnemyState::Dead) {
            state_  = EnemyState::Dead;
            stateT_ = 0.0f;
            model_.setClip(AnimClip::Death, 0.12f);
        }
        stateT_ += dt;
        speed_ = 0.0f;
        strikePending_ = false;
        pos_.y = supportY(world_, pos_.x, pos_.z, pos_.y + 0.6f);
        model_.update(dt);                       // ANIQ BIR MARTA
        return;
    }

    stateT_ += dt;

    Rt& rt = rtFor(this);

    // ---------------- SEZISH ----------------
    const Vec3  pp        = player.position();
    const bool  pAlive    = !player.dead();
    const float dist      = goodV(pp) ? distanceXZ(pos_, pp) : kBigF;
    const bool  see       = pAlive && canSee(player);

    if (see) {
        const float rr = (st_.sightRange > 0.1f) ? clampf(dist / st_.sightRange, 0.0f, 1.0f) : 1.0f;
        alert_ += dt * (2.2f - 1.4f * rr);        // yaqinda tezroq ogohlanadi
        target_ = pp;                             // oxirgi ma'lum joy
    } else {
        alert_ -= dt * 0.35f;
    }
    alert_ = clampf(alert_, 0.0f, 1.0f);

    // O'yinchi halok bo'lsa jang tugaydi
    if (!pAlive && state_ != EnemyState::Patrol && state_ != EnemyState::Idle) {
        alert_ = 0.0f;
        state_ = EnemyState::Patrol;
        stateT_ = 0.0f;
        strikeFired_ = false;
        strikePending_ = false;
    }

    // Hayvon: shubha/qidiruv/aylanish yo'q — sezdi = qochdi
    if (st_.flees && alert_ > 0.25f &&
        (state_ == EnemyState::Suspicious || state_ == EnemyState::Search ||
         state_ == EnemyState::Alert || state_ == EnemyState::Circle ||
         state_ == EnemyState::Windup || state_ == EnemyState::Idle || state_ == EnemyState::Patrol)) {
        state_ = EnemyState::Approach; stateT_ = 0.0f;
    }

    // Poza buzilgan bo'lsa hamma narsa to'xtaydi
    if (vitals.staggered && state_ != EnemyState::Stagger) {
        state_  = EnemyState::Stagger;
        stateT_ = 0.0f;
        speed_  = 0.0f;
        strikeFired_   = false;
        strikePending_ = false;
    }

    // O'yinchi hujum qilyaptimi (blok qarorini bir marta olish uchun qirra)
    const MoveState pms = player.state();
    const bool playerAtk = (pms == MoveState::AttackLight || pms == MoveState::AttackHeavy ||
                            pms == MoveState::KickState);
    const bool atkEdge = playerAtk && !rt.prevPlayerAtk;
    rt.prevPlayerAtk = playerAtk;

    // ---------------- HOLATLAR ----------------
    bool  locomotion = true;                  // Walk/Run/Idle avtomatik tanlanadimi
    float moved      = 0.0f;

    switch (state_) {

    // --- Tinch: patrul yoki joyida turish ---
    case EnemyState::Idle:
    case EnemyState::Patrol: {
        const float pr = rt.patrolRadius;
        if (pr > 0.1f) {
            if (patrolT_ > 0.0f) {
                patrolT_ -= dt;               // nuqtada kutadi
            } else {
                if (distanceXZ(pos_, target_) < 0.7f) {
                    // Yangi tasodifiy nuqta (home_ atrofida)
                    const float ang = rndRange(0.0f, TAU);
                    const float rad = rndRange(pr * 0.25f, pr);
                    Vec3 np(home_.x + std::cos(ang) * rad, home_.y, home_.z + std::sin(ang) * rad);
                    np.y = supportY(world_, np.x, np.z, home_.y + 2.0f);
                    target_  = np;
                    patrolT_ = rndRange(1.5f, 3.5f);
                } else {
                    moved = stepToward(pos_, target_, st_.moveSpeed * 0.55f, dt, world_, bodyH);
                    float y = 0.0f;
                    if (aimYaw(pos_, target_, y)) targetYaw_ = y;
                }
            }
        } else {
            // Post: uyidan uzoqlashsa qaytadi
            if (distanceXZ(pos_, home_) > 0.8f) {
                moved = stepToward(pos_, home_, st_.moveSpeed * 0.5f, dt, world_, bodyH);
                float y = 0.0f;
                if (aimYaw(pos_, home_, y)) targetYaw_ = y;
            }
        }
        if (state_ == EnemyState::Idle) { state_ = EnemyState::Patrol; stateT_ = 0.0f; }
        if (alert_ > 0.35f) { state_ = EnemyState::Suspicious; stateT_ = 0.0f; }
        break;
    }

    // --- Shubha: to'xtaydi va qaraydi ---
    case EnemyState::Suspicious: {
        float y = 0.0f;
        if (aimYaw(pos_, see ? pp : target_, y)) targetYaw_ = y;
        if (alert_ >= 1.0f) {
            state_ = EnemyState::Alert;  stateT_ = 0.0f;
        } else if (alert_ < 0.2f) {
            state_ = EnemyState::Patrol; stateT_ = 0.0f; patrolT_ = rndRange(0.5f, 1.5f);
        } else if (stateT_ >= kSuspiciousWait) {
            state_ = EnemyState::Search; stateT_ = 0.0f; decideT_ = kSearchTime;
        }
        break;
    }

    // --- Qidiruv: oxirgi ma'lum joyga boradi ---
    case EnemyState::Search: {
        if (alert_ >= 1.0f) { state_ = EnemyState::Alert; stateT_ = 0.0f; break; }
        if (distanceXZ(pos_, target_) > 1.0f) {
            moved = stepToward(pos_, target_, st_.moveSpeed, dt, world_, bodyH);
            float y = 0.0f;
            if (aimYaw(pos_, target_, y)) targetYaw_ = y;
        } else {
            targetYaw_ = wrapAngleDeg(targetYaw_ + 55.0f * dt);   // atrofga qaraydi
        }
        if (stateT_ >= ((decideT_ > 0.1f) ? decideT_ : kSearchTime)) {
            state_  = EnemyState::Patrol;
            stateT_ = 0.0f;
            patrolT_ = rndRange(0.5f, 1.5f);
            if (alert_ > 0.30f) alert_ = 0.30f;
        }
        break;
    }

    // --- Ogoh: darhol yaqinlashishga o'tadi ---
    case EnemyState::Alert: {
        alert_  = 1.0f;
        state_  = EnemyState::Approach;
        stateT_ = 0.0f;
        break;
    }

    // --- Yaqinlashish ---
    case EnemyState::Approach: {
        float y = 0.0f;
        if (aimYaw(pos_, see ? pp : target_, y)) targetYaw_ = y;

        // HAYVON: yaqinlashish o'rniga o'yinchidan QOCHADI. Ko'rmay qolsa va
        // ogohlik so'nsa yana o'tlashga (Patrol) qaytadi.
        if (st_.flees) {
            Vec3 away{pos_.x - pp.x, 0.0f, pos_.z - pp.z};
            if (lengthSq(away) < 1.0e-4f) away = Vec3{1.0f, 0.0f, 0.0f};
            away = normalize(away);
            const Vec3 dest = pos_ + away * 12.0f;
            float fy = 0.0f;
            if (aimYaw(pos_, dest, fy)) targetYaw_ = fy;
            moved = stepToward(pos_, dest, st_.chaseSpeed, dt, world_, bodyH);
            if (!see) alert_ -= dt * 0.5f;
            if (!see && alert_ < 0.15f) { state_ = EnemyState::Patrol; stateT_ = 0.0f; }
            break;
        }

        if (atkEdge && dist < st_.attackRange * 1.6f && rnd01() < st_.blockChance) {
            state_ = EnemyState::Block; stateT_ = 0.0f; speed_ = 0.0f;
            break;
        }

        if (dist > st_.attackRange) {
            moved = stepToward(pos_, see ? pp : target_, st_.chaseSpeed, dt, world_, bodyH);
            // O'yinchini butunlay yo'qotdi — qidiruvga
            if (!see && alert_ < 0.35f) {
                state_ = EnemyState::Search; stateT_ = 0.0f; decideT_ = kSearchTime;
            }
        } else {
            const bool canSwing = rt.mayAttack && vitals.breath >= currentAttack().breathCost;
            if (canSwing && rnd01() < st_.aggression) {
                state_ = EnemyState::Windup; stateT_ = 0.0f; strikeFired_ = false;
            } else {
                state_    = EnemyState::Circle;
                stateT_   = 0.0f;
                decideT_  = rndRange(0.6f, 1.4f);
            }
        }
        break;
    }

    // --- Aylanish: bosim bor, lekin zarba yo'q (hujum sloti band) ---
    case EnemyState::Circle: {
        const Vec3 focus = see ? pp : target_;
        float y = 0.0f;
        if (aimYaw(pos_, focus, y)) targetYaw_ = y;   // yon yursa ham o'yinchiga qaraydi

        if (atkEdge && dist < st_.attackRange * 1.6f && rnd01() < st_.blockChance) {
            state_ = EnemyState::Block; stateT_ = 0.0f; speed_ = 0.0f;
            break;
        }

        Vec3 toE = pos_ - focus;
        toE.y = 0.0f;
        float r = length(toE);
        Vec3 radial = (r > 0.05f) ? (toE / r) : dirFromYaw(yaw_ + 180.0f);
        if (r <= 0.05f) r = 0.05f;
        // Yon yo'nalish (circleDir_ ga qarab soat strelkasi bo'yicha yoki teskari)
        const Vec3 tangent(radial.z * circleDir_, 0.0f, -radial.x * circleDir_);
        const float keep = st_.attackRange * 0.95f;
        const Vec3 dst = focus + radial * keep + tangent * 1.8f;

        moved = stepToward(pos_, dst, st_.moveSpeed, dt, world_, bodyH);

        if (dist > st_.attackRange * 2.4f) {
            state_ = EnemyState::Approach; stateT_ = 0.0f;
            break;
        }
        if (stateT_ >= ((decideT_ > 0.1f) ? decideT_ : 1.0f)) {
            const bool canSwing = rt.mayAttack && vitals.breath >= currentAttack().breathCost;
            if (canSwing && rnd01() < (0.30f + 0.60f * st_.aggression)) {
                state_ = EnemyState::Windup; stateT_ = 0.0f; strikeFired_ = false;
            } else {
                stateT_  = 0.0f;
                decideT_ = rndRange(0.6f, 1.4f);
                if (rnd01() < 0.35f) circleDir_ = -circleDir_;
            }
        }
        break;
    }

    // --- Tayyorgarlik: dushman OCHIQ ogohlantiradi (App qizil belgi chizadi) ---
    case EnemyState::Windup: {
        locomotion = false;
        const Attack& a = currentAttack();
        float y = 0.0f;
        if (aimYaw(pos_, see ? pp : target_, y)) targetYaw_ = y;

        // Yaqin jangchi zarba oldidan bir qadam bosadi
        if (!isRanged(kind_) && dist > st_.attackRange * 0.65f)
            moved = stepToward(pos_, pp, st_.moveSpeed * 0.45f, dt, world_, bodyH);

        // Slot yo'qolsa (masalan yaqinroq dushman keldi) — hujumdan voz kechadi
        if (!rt.mayAttack) {
            state_ = EnemyState::Circle; stateT_ = 0.0f; decideT_ = rndRange(0.6f, 1.4f);
            break;
        }
        if (stateT_ >= a.windup) { state_ = EnemyState::Strike; stateT_ = 0.0f; }
        break;
    }

    // --- Zarba: faol oyna ---
    case EnemyState::Strike: {
        locomotion = false;
        const Attack& a = currentAttack();
        if (!strikeFired_) {
            strikeFired_   = true;
            strikePending_ = true;               // EnemyManager shu kadrda o'qiydi
            vitals.spendBreath(a.breathCost);
            vitals.hand    = clampf(vitals.hand - a.handCost, 0.0f, 100.0f);
            vitals.posture = clampf(vitals.posture + a.selfPosture, 0.0f, vitals.postureMax);
        }
        if (stateT_ >= a.active) { state_ = EnemyState::Recover; stateT_ = 0.0f; }
        break;
    }

    // --- Tiklanish: shu payt jazolash mumkin ---
    case EnemyState::Recover: {
        locomotion = false;
        const Attack& a = currentAttack();
        if (stateT_ >= a.recovery) {
            combo_ = (combo_ + 1) % 3;
            strikeFired_ = false;
            state_   = EnemyState::Circle;
            stateT_  = 0.0f;
            decideT_ = rndRange(0.6f, 1.4f);
        }
        break;
    }

    // --- Blok ---
    case EnemyState::Block: {
        locomotion = false;
        float y = 0.0f;
        if (aimYaw(pos_, see ? pp : target_, y)) targetYaw_ = y;
        if (stateT_ >= kBlockTime) {
            state_  = aware() ? EnemyState::Approach : EnemyState::Suspicious;
            stateT_ = 0.0f;
        }
        break;
    }

    // --- Poza buzildi: yakunlovchi zarbaga ochiq ---
    case EnemyState::Stagger: {
        locomotion = false;
        if (!vitals.staggered) {
            state_  = aware() ? EnemyState::Approach : EnemyState::Suspicious;
            stateT_ = 0.0f;
        }
        break;
    }

    // --- Zarba yedi ---
    case EnemyState::Hurt: {
        locomotion = false;
        if (stateT_ >= kHurtTime) {
            state_  = EnemyState::Approach;
            stateT_ = 0.0f;
        }
        break;
    }

    case EnemyState::Dead:
    default:
        locomotion = false;
        break;
    }

    // ---------------- Harakat natijasi ----------------
    speed_ = damp(speed_, moved, 12.0f, dt);

    // Orqaga surish — holatdan mustaqil, devor bilan to'qnashuvni hisobga oladi
    {
        Rt& rtk = rtFor(this);
        if (rtk.pushT > 0.0f) {
            rtk.pushT -= dt;
            if (rtk.pushT < 0.0f) rtk.pushT = 0.0f;
            Vec3 np{pos_.x + rtk.pushVX * dt, pos_.y, pos_.z + rtk.pushVZ * dt};
            if (world_ != nullptr) world_->resolve(np, kBodyRadius, bodyH);
            if (goodV(np)) { np.y = pos_.y; pos_ = np; }
            rtk.pushVX = damp(rtk.pushVX, 0.0f, 9.0f, dt);
            rtk.pushVZ = damp(rtk.pushVZ, 0.0f, 9.0f, dt);
            if (rtk.pushT <= 0.0f) { rtk.pushVX = 0.0f; rtk.pushVZ = 0.0f; }
        }
    }
    if (!goodF(speed_) || speed_ < 0.0f) speed_ = 0.0f;

    yaw_ = wrapAngleDeg(lerpAngleDeg(yaw_, targetYaw_, saturate(dt * kTurnRate)));
    if (!goodF(yaw_)) yaw_ = 0.0f;

    // Yerga yopishish (harakatsiz holatlarda ham)
    pos_.y = supportY(world_, pos_.x, pos_.z, pos_.y + 0.6f);
    if (!goodV(pos_)) pos_ = home_;

    // ---------------- Resurslar ----------------
    const bool resting = (state_ == EnemyState::Idle || state_ == EnemyState::Patrol ||
                          state_ == EnemyState::Suspicious || state_ == EnemyState::Search) &&
                         !vitals.staggered;
    vitals.update(dt, resting);

    // ---------------- Animatsiya (model_.update ANIQ BIR MARTA) ----------------
    if (locomotion) {
        model_.driveByLocomotion(speed_, dt);     // ichida update(dt) bor
    } else {
        AnimClip c = AnimClip::Idle;
        switch (state_) {
            case EnemyState::Windup:
            case EnemyState::Strike:
            case EnemyState::Recover: c = AnimClip::AttackHeavy; break;
            case EnemyState::Block:   c = AnimClip::Block;       break;
            case EnemyState::Hurt:    c = AnimClip::Hurt;        break;
            case EnemyState::Stagger: c = AnimClip::Stagger;     break;
            case EnemyState::Dead:    c = AnimClip::Death;       break;
            default:                  c = AnimClip::Idle;        break;
        }
        // Yangi holatga o'tdikmi? Unda klip BOSHIDAN o'ynaydi (ilgari setClip
        // har kadr qayta boshlab, zarba/zarba yeyish animatsiyasi muzlab qolardi).
        const bool freshAnim = (state_ != animState_) || (stateT_ < animTime_);
        animState_ = state_;
        animTime_  = stateT_;
        if (freshAnim) model_.playClip(c, 0.12f);
        else           model_.setClip(c, 0.12f);
        model_.update(dt);
    }
}

void Enemy::draw() {
    if (!model_.valid()) return;
    const float sc = (goodF(st_.scale) && st_.scale > 0.2f) ? st_.scale : 1.8f;

    Vec3 p = pos_;
    if (state_ == EnemyState::Dead) {
        // Death klipi tugagach jasad yerga singadi (turgan holda qotib qolmasin).
        // Skin.cpp da Death pozasi paydo bo'lgach bu chekinish tabiiy ko'rinadi.
        const float t = clampf((stateT_ - 1.2f) / 0.8f, 0.0f, 1.0f);
        p.y -= sc * 0.42f * smoothstepf(t);
    }
    if (!goodV(p)) return;
    model_.draw(p, yaw_, sc);
}

// ===========================================================================
//  EnemyManager
// ===========================================================================
void EnemyManager::clear() {
    rtDropRange(enemies_.data(), enemies_.size());
    enemies_.clear();
    incoming_.clear();
}

Enemy* EnemyManager::spawn(const EnemySpawn& s, PhysicsWorld* world) {
    if (enemies_.size() >= 256) return nullptr;          // aqlga sig'adigan chegara
    if (enemies_.capacity() == 0) enemies_.reserve(64);

    const Enemy* oldBase = enemies_.data();
    const size_t oldN    = enemies_.size();

    enemies_.push_back(Enemy());

    // vector qayta joylashgan bo'lsa yon jadvaldagi kalitlarni ko'chiramiz
    const Enemy* newBase = enemies_.data();
    if (newBase != oldBase) rtRemap(oldBase, newBase, oldN);

    Enemy& e = enemies_.back();
    e.init(s.kind, world);
    e.reset(s.pos, s.yaw);

    Rt& rt = rtFor(&e);
    rt.patrolRadius = (std::isfinite(s.patrolRadius) && s.patrolRadius > 0.0f)
                        ? clampf(s.patrolRadius, 0.0f, 60.0f) : 0.0f;
    rt.mayAttack     = true;
    rt.prevPlayerAtk = false;
    return &e;
}

void EnemyManager::update(const Character& player, float dt) {
    incoming_.clear();
    if (enemies_.empty()) return;
    if (!std::isfinite(dt)) return;
    if (dt < 0.0f) dt = 0.0f;

    const Vec3 pp = player.position();
    const size_t n = enemies_.size();

    // -----------------------------------------------------------------------
    // HUJUM SLOTLARI (AC uslubi): bir vaqtda faqat 2 ta yaqin jangchi uradi.
    // Allaqachon Windup/Strike da bo'lganlar slotni ushlab turadi, qolgan
    // ruxsatlar ENG YAQIN dushmanlarga beriladi. Kamonchilar alohida hisoblanadi.
    // -----------------------------------------------------------------------
    int meleeUsed = 0, rangedUsed = 0;
    for (size_t i = 0; i < n; ++i) {
        Enemy& e = enemies_[i];
        Rt& rt = rtFor(&e);
        const EnemyState st = e.state();
        const bool committed = e.alive() &&
                               (st == EnemyState::Windup || st == EnemyState::Strike ||
                                st == EnemyState::Recover);
        if (committed) {
            rt.mayAttack = true;
            if (isRanged(e.kind())) ++rangedUsed; else ++meleeUsed;
        } else {
            rt.mayAttack = false;
        }
    }

    // Qolgan slotlarni takroriy tanlash bilan eng yaqinlarga beramiz
    // (dushmanlar soni kichik — O(n*slot) yetarli).
    for (int pass = 0; pass < kMeleeSlots + kRangedSlots; ++pass) {
        if (meleeUsed >= kMeleeSlots && rangedUsed >= kRangedSlots) break;

        int   bestI = -1;
        float bestD = kBigF;
        for (size_t i = 0; i < n; ++i) {
            Enemy& e = enemies_[i];
            if (!e.alive() || !e.aware()) continue;
            const EnemyState st = e.state();
            if (st == EnemyState::Dead || st == EnemyState::Stagger || st == EnemyState::Hurt)
                continue;
            Rt& rt = rtFor(&e);
            if (rt.mayAttack) continue;                    // allaqachon ruxsat oldi
            const bool rangedOne = isRanged(e.kind());
            if (rangedOne ? (rangedUsed >= kRangedSlots) : (meleeUsed >= kMeleeSlots)) continue;

            const float d = distanceXZ(e.position(), pp);
            if (!std::isfinite(d)) continue;
            if (d < bestD) { bestD = d; bestI = (int)i; }
        }
        if (bestI < 0) break;

        Enemy& e = enemies_[(size_t)bestI];
        rtFor(&e).mayAttack = true;
        if (isRanged(e.kind())) ++rangedUsed; else ++meleeUsed;
    }

    // ----------------------- Yangilash -----------------------
    for (size_t i = 0; i < n; ++i) enemies_[i].update(player, dt);

    // ----------------------- Faol zarbalarni yig'ish -----------------------
    const Vec3 pAt = player.position();
    for (size_t i = 0; i < n; ++i) {
        Enemy& e = enemies_[i];
        if (!e.consumeStrike()) continue;

        // MASOFA va BURCHAK tekshiruvi. Ilgari consumeStrike() true bo'lsa bas
        // edi: og'ir zarbaning 0.38 s tayyorgarligi davomida o'yinchi 4.4 m/s
        // bilan qochib ketsa ham zarba baribir tegardi. Endi zarba faqat
        // yetib boradigan masofada va oldingi sektorda tegadi.
        const Attack& atk = e.currentAttack();
        const Vec3 d{pAt.x - e.position().x, 0.0f, pAt.z - e.position().z};
        const float dist = length(d);
        const float reach = (atk.reach > 0.2f ? atk.reach : 2.2f) + 0.55f;   // tana radiusi
        if (dist > reach) continue;
        if (dist > 1.0e-3f && atk.arcDeg > 1.0f) {
            const float off = offAngle(e.position(), e.yaw(), pAt);
            if (off > atk.arcDeg * 0.5f + 12.0f) continue;   // yon zaxira 12 gradus
        }

        IncomingHit h;
        h.attack = &atk;                   // statik jadvalga havola — barqaror
        h.from   = e.position();
        h.src    = &e;
        incoming_.push_back(h);
    }
}

void EnemyManager::draw() {
    for (size_t i = 0; i < enemies_.size(); ++i) {
        // Jasadlar ham chiziladi: Dead holati "keyin yerda yotadi" deb
        // ta'riflangan, shuning uchun ular birdan yo'qolmaydi.
        enemies_[i].draw();
    }
}

void EnemyManager::broadcastNoise(const Vec3& at, float radius, float strength) {
    for (size_t i = 0; i < enemies_.size(); ++i)
        enemies_[i].hearNoise(at, radius, strength);
}

HitResult EnemyManager::playerAttack(const Vec3& from, float yaw, const Attack& a, bool& outKilled) {
    outKilled = false;
    HitResult miss;
    miss.point = from;
    miss.outcome = HitOutcome::Miss;

    if (!goodV(from) || !std::isfinite(yaw)) return miss;
    if (!std::isfinite(a.reach) || a.reach <= 0.0f) return miss;

    const float halfArc = (std::isfinite(a.arcDeg) && a.arcDeg > 0.0f)
                            ? clampf(a.arcDeg * 0.5f, 1.0f, 180.0f) : 45.0f;

    int   bestI = -1;
    float bestD = kBigF;

    for (size_t i = 0; i < enemies_.size(); ++i) {
        Enemy& e = enemies_[i];
        if (!e.alive()) continue;

        const Vec3 ep = e.position();
        const float d = distanceXZ(from, ep);
        if (!std::isfinite(d) || d > a.reach) continue;
        // BALANDLIK: ilgari faqat XZ masofa tekshirilardi — tomdagi yoki
        // jardagi dushmanga ham qilich yetardi. Dushmanning bo'yi kamida
        // yarmi o'yinchining zarba sathi bilan kesishishi kerak.
        {
            const EnemyStats& est = enemyStats(e.kind());
            const float eh = (est.scale > 0.2f && goodF(est.scale)) ? est.scale : 1.8f;
            const float dy = ep.y - from.y;
            if (!goodF(dy) || dy > eh * 0.65f || dy < -eh * 0.65f) continue;
        }
        // Juda yaqinda burchak shovqinli bo'ladi — o'tkazib yuboramiz
        if (d > 0.15f && offAngle(from, yaw, ep) > halfArc) continue;
        if (d < bestD) { bestD = d; bestI = (int)i; }
    }
    if (bestI < 0) return miss;

    Enemy& e = enemies_[(size_t)bestI];

    // O'yinchi dushmanning orqasidami? (yashirin o'ldirish uchun)
    const bool behind = offAngle(e.position(), e.yaw(), from) > 120.0f;

    const bool wasAlive = e.alive();
    const HitResult r = e.receiveHit(a, from, behind);
    outKilled = wasAlive && !e.alive();
    return r;
}

Enemy* EnemyManager::nearestTarget(const Vec3& from, float maxDist) const {
    if (!goodV(from) || !std::isfinite(maxDist) || maxDist <= 0.0f) return nullptr;

    const Enemy* best = nullptr;
    float bestD = maxDist;
    for (size_t i = 0; i < enemies_.size(); ++i) {
        const Enemy& e = enemies_[i];
        if (!e.alive()) continue;
        const float d = distanceXZ(from, e.position());
        if (!std::isfinite(d) || d > bestD) continue;
        bestD = d;
        best  = &e;
    }
    return const_cast<Enemy*>(best);
}

HitResult EnemyManager::arrowHit(int index, HitZone zone, float charge, float distM,
                                 const Vec3& from, bool silent, bool& outKilled) {
    outKilled = false;
    HitResult miss;
    miss.point = from;
    if (index < 0 || index >= (int)enemies_.size()) return miss;
    Enemy& e = enemies_[(size_t)index];
    const bool wasAlive = e.alive();
    const HitResult r = e.receiveArrow(attackDef(DamageType::Arrow), from,
                                       zone, charge, distM, silent);
    outKilled = wasAlive && !e.alive();
    return r;
}

// Yakunlovchi zarba uchun nishon tanlash. App.cpp ilgari massivdagi BIRINCHI
// staggered dushmanni olardi — burchak ham, staggerT ham tekshirilmasdi, natijada
// o'yinchi orqasidagi dushmanga "yakunlovchi zarba" berishi mumkin edi.
int EnemyManager::findExecutable(const Vec3& from, float yaw,
                                 float maxDist, float maxAngleDeg) const {
    int   best  = -1;
    float bestD = maxDist;
    const Vec3 face = dirFromYaw(yaw);
    for (size_t i = 0; i < enemies_.size(); ++i) {
        const Enemy& e = enemies_[i];
        if (!e.alive() || !e.vitals.staggered) continue;
        // Zarba kadri ~0.43 s da tushadi (windup * actionDur/duration). Undan kam
        // qolgan bo'lsa stagger tugab invulnT = 0.6 yonadi va 999 zarar jimgina
        // "Dodged" bo'lib yo'qoladi. Shuning uchun 0.55 s zaxira talab qilamiz.
        if (e.vitals.staggerT < 0.55f) continue;
        const Vec3 d{e.position().x - from.x, 0.0f, e.position().z - from.z};
        const float dist = length(d);
        if (dist > bestD || dist < 1e-3f) continue;
        const Vec3 dn = normalize(d);
        const float ang = rad2deg(std::acos(clampf(dot(face, dn), -1.0f, 1.0f)));
        if (ang > maxAngleDeg) continue;
        best  = (int)i;
        bestD = dist;
    }
    return best;
}

// Indeks bo'yicha qat'iy o'ldirish. playerAttack dagi konus qidiruvi ishlatilmaydi,
// shuning uchun zarba boshqa dushmanga "sakrab" ketmaydi.
bool EnemyManager::deathblow(int idx, HitResult& out) {
    if (idx < 0 || idx >= (int)enemies_.size()) return false;
    Enemy& e = enemies_[(size_t)idx];
    if (!e.alive()) return false;
    e.killOutright();
    out.outcome = HitOutcome::Killed;
    out.damage  = e.vitals.healthMax;
    out.posture = e.vitals.postureMax;
    out.point   = e.position() + Vec3{0.0f, 1.1f, 0.0f};
    return true;
}

int EnemyManager::aliveCount() const {
    int c = 0;
    for (size_t i = 0; i < enemies_.size(); ++i)
        if (enemies_[i].alive()) ++c;
    return c;
}

int EnemyManager::awareCount() const {
    int c = 0;
    for (size_t i = 0; i < enemies_.size(); ++i)
        if (enemies_[i].alive() && enemies_[i].aware() && !enemyStats(enemies_[i].kind()).flees) ++c;
    return c;
}

} // namespace ert
