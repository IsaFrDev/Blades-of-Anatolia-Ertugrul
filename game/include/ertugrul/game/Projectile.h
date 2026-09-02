#pragma once
// O'q — yagona uchuvchi jism turi. Ballistik (gravitatsiya bor), CPU da
// qadamlab yuradi va PhysicsWorld ning XZ nurlari bilan to'qnashadi.
// Bu fayl Character ni BILMAYDI — bir tomonlama bog'liqlik, halqa yo'q.
#include <vector>
#include "ertugrul/core/Math.h"
#include "ertugrul/game/Combat.h"

namespace ert {

class PhysicsWorld;
class Enemy;

// Character otgan o'qning "buyurtmasi" (consumeShot qaytaradi)
struct BowShot {
    Vec3  origin{0, 0, 0};    // o'q chiqadigan nuqta (chap qo'l balandligi)
    Vec3  dir{0, 0, 1};       // normallashgan yo'nalish (titrash QO'SHILGAN)
    float speed  = 52.0f;     // m/s — tortish kuchiga bog'liq
    float charge = 1.0f;      // 0..1 tortish kuchi (zarar ko'paytiruvchisi)
    float spread = 0.0f;      // gradus — HUD nishoni uchun
    bool  silent = true;      // otganda sezilmagan edikmi
};

struct Arrow {
    Vec3  pos{0, 0, 0}, prev{0, 0, 0}, vel{0, 0, 0};
    Vec3  from{0, 0, 0};      // otilgan nuqta (masofa so'nishi uchun)
    float life   = 0.0f;      // uchgan vaqti (s)
    float charge = 1.0f;
    bool  active = false;
    bool  silent = true;
    bool  stuck  = false;     // yerga/devorga qadaldi — YIG'IB OLISH mumkin
    float stuckT = 0.0f;
    Vec3  stuckDir{0, 0, 1};
};

enum class ArrowHitKind { None = 0, World, Enemy };

// Bir kadrdagi urilish hodisasi — Encounter uni zararga aylantiradi
struct ArrowHit {
    ArrowHitKind kind = ArrowHitKind::None;
    Vec3     point{0, 0, 0};
    Vec3     dir{0, 0, 1};
    int      enemyIndex = -1;      // EnemyManager::all() indeksi
    HitZone  zone   = HitZone::Torso;
    float    dist   = 0.0f;        // otilgan joydan urilgan joygacha (m)
    float    charge = 1.0f;
    bool     silent = true;
};

class ArrowPool {
public:
    static constexpr int kMax = 16;      // bir vaqtda uchadigan o'qlar
    void clear();
    bool spawn(const BowShot& s);        // bo'sh slot yo'q bo'lsa false
    void update(float dt, const PhysicsWorld& phys,
                const std::vector<Enemy>& enemies,
                std::vector<ArrowHit>& outHits);
    void draw() const;                   // GL_LINES — uchayotgan va qadalgan o'qlar
    int  live() const;
    // O'yinchi yaqinidagi qadalgan o'qlarni yig'adi — nechta olinganini qaytaradi.
    // Yerdagi va jasad yonidagi o'qlar ham shu yerdan qaytadi.
    int  collect(const Vec3& pos, float radius);

    // HUD uchun: zararsiz oldindan hisob — birinchi to'qnashuv nuqtasi
    static bool predictImpact(const BowShot& s, const PhysicsWorld& phys, Vec3& out);
private:
    Arrow a_[kMax];
};

} // namespace ert
