// ArrowPool::collect va jasad yonidagi o'q uchun mustaqil sinov.
// PhysicsWorld va Enemy — soxta (stub): tekis yer y=0, devor yo'q.
#include <cstdio>
#include <vector>
#include "ertugrul/game/Projectile.h"
#include "ertugrul/world/Physics.h"
#include "ertugrul/game/Enemy.h"

namespace ert {
float PhysicsWorld::supportBelow(float, float, float) const { return 0.0f; }
bool  PhysicsWorld::rayXZ(const Vec3&, const Vec3&, float, float, RayHit&) const { return false; }
const EnemyStats& enemyStats(EnemyKind) { static EnemyStats st; return st; }
}

using namespace ert;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("  XATO: %s (%d)\n", #c, __LINE__); ++fails; } \
                      else std::printf("  ok: %s\n", #c); } while (0)

int main() {
    PhysicsWorld phys;
    std::vector<Enemy>& none = *new std::vector<Enemy>();   // Enemy dtor ga bog'lanmaslik uchun
    std::vector<ArrowHit> hits;
    ArrowPool pool;

    // 1) O'q pastga otiladi -> yerga qadaladi
    BowShot s; s.origin = Vec3{5.0f, 1.5f, 5.0f}; s.dir = Vec3{0.3f, -1.0f, 0.0f}; s.speed = 40.0f;
    CHECK(pool.spawn(s));
    for (int i = 0; i < 30 && hits.empty(); ++i) pool.update(1.0f / 60.0f, phys, none, hits);
    CHECK(hits.size() == 1 && hits[0].kind == ArrowHitKind::World);
    CHECK(pool.live() == 0);                       // uchayotgan yo'q, lekin qadalgan bor

    // 2) Uzoqdan yig'ib bo'lmaydi, yaqindan bo'ladi
    CHECK(pool.collect(Vec3{20.0f, 0.0f, 20.0f}, 1.15f) == 0);
    const Vec3 at = hits[0].point;
    CHECK(pool.collect(Vec3{at.x + 0.8f, 0.0f, at.z}, 1.15f) == 1);
    CHECK(pool.collect(Vec3{at.x, 0.0f, at.z}, 1.15f) == 0);   // ikkinchi marta yo'q

    // 3) Boshqa qavatdagi o'q olinmaydi (dy > 2 m)
    hits.clear();
    CHECK(pool.spawn(s));
    for (int i = 0; i < 30 && hits.empty(); ++i) pool.update(1.0f / 60.0f, phys, none, hits);
    CHECK(pool.collect(Vec3{hits[0].point.x, -3.0f, hits[0].point.z}, 1.15f) == 0);
    CHECK(pool.collect(Vec3{hits[0].point.x,  0.5f, hits[0].point.z}, 1.15f) == 1);

    // 4) 40 s dan keyin o'q o'zi yo'qoladi
    hits.clear();
    CHECK(pool.spawn(s));
    for (int i = 0; i < 30 && hits.empty(); ++i) pool.update(1.0f / 60.0f, phys, none, hits);
    for (int i = 0; i < 60 * 41; ++i) pool.update(1.0f / 60.0f, phys, none, hits);
    CHECK(pool.collect(hits[0].point, 1.15f) == 0);

    std::printf(fails ? "NATIJA: %d xato\n" : "NATIJA: hammasi o'tdi\n", fails);
    return fails ? 1 : 0;
}
