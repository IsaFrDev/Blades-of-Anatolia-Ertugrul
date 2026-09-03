#pragma once
// Dushmanlar va ularning sun'iy intellekti.
//
// Sezish AC modelidagi kabi ikki kanal:
//   KO'RISH  — konus (burchak + masofa), o'yinchining profili va cho'kkalashi ta'sir qiladi
//   ESHITISH — Character chiqargan shovqin hodisalari (yugurish, sakrash, qo'nish)
//
// Ogohlik uch bosqich: Tinch -> Shubha (tekshiradi) -> Ogoh (jang).
#include <vector>
#include <string>
#include "ertugrul/core/Math.h"
#include "ertugrul/game/Combat.h"
#include "ertugrul/gfx/Skin.h"

namespace ert {

class PhysicsWorld;
class Character;

enum class EnemyKind {
    Footman = 0,   // oddiy piyoda — qilich
    Sergeant,      // qalqonli serjant — bloki kuchli, tepish kerak
    Crossbow,      // arbaletchi — uzoqdan otadi, yaqinda zaif
    Assassin,      // saroy qotili — tez, kam sog'liq
    HorseArcher,   // mo'g'ul otliq kamonchi (S3+)
    Elite,         // keshikten — elita gvardiya (S4)
    Deer,          // kiyik — ov nishoni: hujum qilmaydi, sezsa qochadi
    Count
};

enum class EnemyState {
    Idle = 0, Patrol, Suspicious, Search, Alert,
    Approach, Circle, Windup, Strike, Recover,
    Block, Stagger, Hurt, Dead, Count
};

struct EnemyStats {
    float health      = 60.0f;
    float breath      = 80.0f;
    float posture     = 100.0f;
    float moveSpeed   = 2.6f;
    float chaseSpeed  = 4.4f;
    float sightRange  = 26.0f;
    float sightAngle  = 62.0f;    // yarim burchak (gradus)
    float hearRange   = 18.0f;
    float attackRange = 2.2f;
    float aggression  = 0.5f;     // 0..1 — qanchalik tez hujum qiladi
    float blockChance = 0.25f;
    float scale       = 1.80f;
    const char* model = "assets/models/crusader/crusader.obj";
    bool  flees       = false;    // hayvon: hujum o'rniga qochadi, ogohlikka kirmaydi
    float tint[3]     = { 1.0f, 1.0f, 1.0f };
};

const EnemyStats& enemyStats(EnemyKind k);
const char*       enemyKindName(EnemyKind k);
const char*       enemyKindLocKey(EnemyKind k);
EnemyKind         enemyKindFromName(const std::string& s);

struct EnemySpawn {
    EnemyKind kind = EnemyKind::Footman;
    Vec3      pos{0, 0, 0};
    float     yaw = 0.0f;
    float     patrolRadius = 0.0f;   // 0 = joyida turadi
};

// Bitta dushman
class Enemy {
public:
    bool init(EnemyKind kind, PhysicsWorld* world);
    void reset(const Vec3& pos, float yaw);

    // player — o'yinchi; playerNoise 0..1; dt sekundda
    void update(const Character& player, float dt);
    void draw();

    // --- Holat ---
    EnemyKind  kind()  const { return kind_; }
    EnemyState state() const { return state_; }
    Vec3       position() const { return pos_; }
    float      yaw() const { return yaw_; }
    bool       alive() const { return vitals.alive(); }
    float      alertness() const { return alert_; }        // 0..1
    bool       aware() const { return alert_ >= 1.0f; }
    // Bosh ustidagi belgi uchun: 0 = tinch, 0..1 = shubha to'lmoqda, 1 = ogoh
    Vec3       headPos() const;

    // --- Jang ---
    Vitals vitals;
    // Shu kadrda dushman zarbasi FAOL bo'ldimi (bir marta true qaytaradi)
    bool  consumeStrike();
    const Attack& currentAttack() const;
    // O'yinchidan zarba olish
    HitResult receiveHit(const Attack& a, const Vec3& from, bool playerBehind);
    // Yashirin o'ldirish mumkinmi (orqadan va sezmagan)
    bool assassinable(const Vec3& playerPos, float playerYaw) const;
    // QAT'IY o'ldirish — invulnT/blok/parry to'sa olmaydi (yakunlovchi zarba)
    void killOutright();
    // O'q urildi: zona va tortish kuchi ALLAQACHON ma'lum (ArrowPool topgan).
    // silent = otganda sezilmagan; o'lsa dushman qichqirmaydi.
    HitResult receiveArrow(const Attack& a, const Vec3& from, HitZone zone,
                           float charge, float distM, bool silent);

    // --- Sezish ---
    void hearNoise(const Vec3& at, float radius, float strength);
    bool canSee(const Character& player) const;

    SkinnedModel& model() { return model_; }

private:
    EnemyKind    kind_ = EnemyKind::Footman;
    EnemyStats   st_;
    PhysicsWorld* world_ = nullptr;
    SkinnedModel model_;

    Vec3  pos_{0, 0, 0}, home_{0, 0, 0}, target_{0, 0, 0};
    float yaw_ = 0.0f, targetYaw_ = 0.0f, speed_ = 0.0f;
    EnemyState state_ = EnemyState::Idle;
    float stateT_ = 0.0f, alert_ = 0.0f, patrolT_ = 0.0f, decideT_ = 0.0f;
    int   combo_ = 0;
    bool  strikeFired_ = false, strikePending_ = false;
    float circleDir_ = 1.0f;
    EnemyState animState_ = EnemyState::Idle;   // klipni qachon boshidan qo'yish
    float      animTime_  = -1.0f;
};

// Dushmanlar to'plami: yaratish, yangilash, chizish, zarba hal qilish
class EnemyManager {
public:
    void clear();
    Enemy* spawn(const EnemySpawn& s, PhysicsWorld* world);
    void   update(const Character& player, float dt);
    void   draw();
    // Shovqinni hammaga tarqatadi
    void   broadcastNoise(const Vec3& at, float radius, float strength);

    // O'yinchi zarbasini hal qiladi: oldindagi sektordagi eng yaqin tirik dushman
    HitResult playerAttack(const Vec3& from, float yaw, const Attack& a, bool& outKilled);
    // O'yinchiga eng yaqin nishon (lock-on uchun); yo'q bo'lsa nullptr
    Enemy* nearestTarget(const Vec3& from, float maxDist) const;
    // Shu kadrda o'yinchiga tegadigan dushman zarbalari
    struct IncomingHit { const Attack* attack; Vec3 from; Enemy* src; };
    const std::vector<IncomingHit>& incoming() const { return incoming_; }

    // Yakunlovchi zarba uchun mos ENG YAQIN nishon indeksi (-1 = yo'q).
    // Shart: tirik + pozasi buzilgan + staggerT yetarli + masofa + burchak.
    int  findExecutable(const Vec3& from, float yaw, float maxDist, float maxAngleDeg) const;
    // O'q urilishini zararga aylantiradi (ArrowPool topgan indeks bo'yicha)
    HitResult arrowHit(int index, HitZone zone, float charge, float distM,
                       const Vec3& from, bool silent, bool& outKilled);
    // Indeks bo'yicha qat'iy o'ldirish — konus qidiruvi YO'Q, boshqa dushmanga tegmaydi
    bool deathblow(int idx, HitResult& out);

    int  aliveCount() const;
    int  awareCount() const;
    int  total() const { return (int)enemies_.size(); }
    std::vector<Enemy>& all() { return enemies_; }
    const std::vector<Enemy>& all() const { return enemies_; }

private:
    std::vector<Enemy>       enemies_;
    std::vector<IncomingHit> incoming_;
};

} // namespace ert
