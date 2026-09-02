#include "ertugrul/game/Projectile.h"
#include "ertugrul/game/Enemy.h"
#include "ertugrul/world/Physics.h"
#include <windows.h>
#include <GL/gl.h>
#include <cmath>

namespace ert {
namespace {

constexpr float kArrowG      = 9.81f;   // m/s^2
constexpr float kArrowLife   = 3.0f;    // s
constexpr float kArrowRange  = 65.0f;   // m — aniq otish masofasi chegarasi
constexpr float kArrowStep   = 0.40f;   // m — integratsiya qadami
constexpr float kBodyRadius  = 0.42f;   // dushman silindri
constexpr float kStuckShow   = 40.0f;   // qadalgan o'q shuncha vaqt yig'ib olinadi (s)
constexpr float kStuckFade   = 5.0f;    // oxirgi soniyalarda so'nadi

inline bool goodF(float v) { return std::isfinite(v) && v > -1.0e8f && v < 1.0e8f; }
inline bool goodV(const Vec3& v) { return goodF(v.x) && goodF(v.y) && goodF(v.z); }

// Segment (a->b) vertikal silindr (markaz c, radius r, balandlik h) bilan
// kesishadimi. XZ da kvadrat tenglama + Y oralig'i.
bool segmentHitsCylinder(const Vec3& a, const Vec3& b, const Vec3& c,
                         float r, float h, float& outT) {
    const float dx = b.x - a.x, dz = b.z - a.z;
    const float ex = a.x - c.x, ez = a.z - c.z;
    const float A = dx * dx + dz * dz;
    if (A < 1.0e-9f) {                       // tik uchayotgan o'q
        if (ex * ex + ez * ez > r * r) return false;
        outT = 0.0f;
    } else {
        const float B = 2.0f * (ex * dx + ez * dz);
        const float C = ex * ex + ez * ez - r * r;
        const float D = B * B - 4.0f * A * C;
        if (D < 0.0f) return false;
        const float sq = std::sqrt(D);
        float t = (-B - sq) / (2.0f * A);
        if (t < 0.0f) t = (-B + sq) / (2.0f * A);
        if (t < 0.0f || t > 1.0f) return false;
        outT = t;
    }
    const float y = a.y + (b.y - a.y) * outT;
    return (y >= c.y - 0.05f) && (y <= c.y + h);
}

// Balandlik ulushidan zonani aniqlaydi (headPos ~ 0.95*h, Enemy.cpp:428)
HitZone zoneFromHeight(float yRel, float h) {
    const float f = (h > 0.1f) ? clampf(yRel / h, 0.0f, 1.0f) : 0.5f;
    if (f >= 0.84f) return HitZone::Head;
    if (f >= 0.45f) return HitZone::Torso;
    return HitZone::Legs;
}

} // anonim namespace

bool ArrowPool::spawn(const BowShot& s) {
    if (!goodV(s.origin) || !goodV(s.dir)) return false;
    for (int i = 0; i < kMax; ++i) {
        Arrow& r = a_[i];
        if (r.active) continue;
        r = Arrow();
        r.active  = true;
        r.pos = r.prev = r.from = s.origin;
        r.vel     = normalize(s.dir) * clampf(s.speed, 5.0f, 90.0f);
        r.charge  = clampf(s.charge, 0.0f, 1.0f);
        r.silent  = s.silent;
        r.stuckDir= normalize(s.dir);
        return true;
    }
    return false;                            // hovuz to'la — o'q yo'qoladi
}

void ArrowPool::update(float dt, const PhysicsWorld& phys,
                       const std::vector<Enemy>& enemies,
                       std::vector<ArrowHit>& outHits) {
    if (!goodF(dt) || dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f;                // sakrash himoyasi

    for (int i = 0; i < kMax; ++i) {
        Arrow& r = a_[i];
        if (!r.active) continue;

        if (r.stuck) {                       // qadalgan o'q — faqat so'nadi
            r.stuckT += dt;
            if (r.stuckT >= kStuckShow) r.active = false;
            continue;
        }

        r.life += dt;
        if (r.life > kArrowLife || distance(r.pos, r.from) > kArrowRange) {
            r.active = false;
            continue;
        }

        // Qadam uzunligi bilan cheklangan integratsiya
        int n = (int)std::ceil(length(r.vel) * dt / kArrowStep);
        if (n < 1) n = 1;
        if (n > 8) n = 8;
        const float h = dt / (float)n;

        for (int k = 0; k < n && r.active && !r.stuck; ++k) {
            r.prev = r.pos;
            r.vel.y -= kArrowG * h;
            r.pos = r.pos + r.vel * h;
            if (!goodV(r.pos)) { r.active = false; break; }

            const Vec3 seg = r.pos - r.prev;
            const float segLen = length(seg);
            if (segLen < 1.0e-5f) continue;

            // 1) Dushmanlar — eng yaqin kesishuv (ular yerdan/devordan ustun)
            int   bestI = -1;
            float bestT = 2.0f;
            for (size_t e = 0; e < enemies.size(); ++e) {
                if (!enemies[e].alive()) continue;
                const EnemyStats& st = enemyStats(enemies[e].kind());
                const float eh = (st.scale > 0.2f) ? st.scale : 1.8f;
                float t = 0.0f;
                if (segmentHitsCylinder(r.prev, r.pos, enemies[e].position(),
                                        kBodyRadius, eh, t) && t < bestT) {
                    bestT = t; bestI = (int)e;
                }
            }
            if (bestI >= 0) {
                const Vec3 p = r.prev + seg * bestT;
                const EnemyStats& st = enemyStats(enemies[(size_t)bestI].kind());
                const float eh = (st.scale > 0.2f) ? st.scale : 1.8f;
                ArrowHit hit;
                hit.kind = ArrowHitKind::Enemy;
                hit.point = p;  hit.dir = normalize(seg);
                hit.enemyIndex = bestI;
                hit.zone = zoneFromHeight(p.y - enemies[(size_t)bestI].position().y, eh);
                hit.dist = distance(r.from, p);
                hit.charge = r.charge;  hit.silent = r.silent;
                outHits.push_back(hit);
                // O'q tanadan o'tmaydi — jasad YONIDA yerga tushadi va uni
                // o'yinchi yig'ib olishi mumkin (ilgari shunchaki yo'qolardi).
                {
                    const float gy = phys.supportBelow(p.x, p.z, p.y + 0.3f);
                    Vec3 g{p.x, goodF(gy) ? gy : p.y, p.z};
                    g.x += hit.dir.x * 0.6f;  g.z += hit.dir.z * 0.6f;
                    Vec3 d = hit.dir;  d.y = -0.55f;
                    r.pos = g;  r.stuck = true;  r.stuckT = 0.0f;
                    r.stuckDir = normalize(d);
                }
                break;
            }

            // 2) Yer / tom
            const float gy = phys.supportBelow(r.pos.x, r.pos.z, r.prev.y + 0.05f);
            if (goodF(gy) && r.pos.y <= gy) {
                const float dy = r.prev.y - r.pos.y;
                const float t  = (dy > 1.0e-5f) ? clampf((r.prev.y - gy) / dy, 0.0f, 1.0f) : 0.0f;
                Vec3 p = r.prev + seg * t;
                p.y = gy;
                ArrowHit hit;
                hit.kind = ArrowHitKind::World;
                hit.point = p;  hit.dir = normalize(seg);
                hit.dist = distance(r.from, p);
                hit.charge = r.charge;  hit.silent = r.silent;
                outHits.push_back(hit);
                r.pos = p;  r.stuck = true;  r.stuckT = 0.0f;
                r.stuckDir = hit.dir;
                break;
            }

            // 3) Devor — shu bo'lakning O'RTACHA balandligida XZ nuri.
            //    (Physics.h:77 rayXZ faqat bitta Y sathida ishlaydi — shuning
            //     uchun qadam 0.40 m dan oshmasligi shart.)
            const float segXZ = std::sqrt(seg.x * seg.x + seg.z * seg.z);
            if (segXZ > 1.0e-4f) {
                RayHit rh;
                if (phys.rayXZ(r.prev, seg, segXZ, (r.prev.y + r.pos.y) * 0.5f, rh) && rh.hit) {
                    const float t = clampf(rh.dist / segXZ, 0.0f, 1.0f);
                    const Vec3 p{rh.point.x, lerpf(r.prev.y, r.pos.y, t), rh.point.z};
                    ArrowHit hit;
                    hit.kind = ArrowHitKind::World;
                    hit.point = p;  hit.dir = normalize(seg);
                    hit.dist = distance(r.from, p);
                    hit.charge = r.charge;  hit.silent = r.silent;
                    outHits.push_back(hit);
                    r.pos = p;  r.stuck = true;  r.stuckT = 0.0f;
                    r.stuckDir = hit.dir;
                    break;
                }
            }
        }
    }
}

// Chizish: uchayotgan o'q — 0.55 m chiziq, qadalgani — 0.35 m va so'nadi.
// Chaqiruvchi (Encounter::draw) lighting/texture ni allaqachon o'chirgan.
void ArrowPool::draw() const {
    glLineWidth(1.6f);
    glBegin(GL_LINES);
    for (int i = 0; i < kMax; ++i) {
        const Arrow& r = a_[i];
        if (!r.active) continue;
        if (r.stuck) {
            const float f = saturate((kStuckShow - r.stuckT) / kStuckFade);
            // Yig'ib olinadigan o'q: sekin "nafas oluvchi" yorug'lik bilan ajralib turadi
            const float pulse = 0.80f + 0.20f * std::sin(r.stuckT * 5.0f);
            glColor4f(0.92f * pulse, 0.84f * pulse, 0.55f, 0.95f * f);
            glVertex3f(r.pos.x, r.pos.y, r.pos.z);
            glVertex3f(r.pos.x - r.stuckDir.x * 0.35f,
                       r.pos.y - r.stuckDir.y * 0.35f,
                       r.pos.z - r.stuckDir.z * 0.35f);
        } else {
            const Vec3 d = normalize(r.vel);
            glColor4f(0.90f, 0.86f, 0.72f, 0.95f);
            glVertex3f(r.pos.x, r.pos.y, r.pos.z);
            glColor4f(0.60f, 0.55f, 0.42f, 0.35f);
            glVertex3f(r.pos.x - d.x * 0.55f, r.pos.y - d.y * 0.55f, r.pos.z - d.z * 0.55f);
        }
    }
    glEnd();
}

// HUD uchun: 40 qadam x 0.05 s zararsiz oldindan hisob (o'sha integratsiya)
bool ArrowPool::predictImpact(const BowShot& s, const PhysicsWorld& phys, Vec3& out) {
    if (!goodV(s.origin) || !goodV(s.dir)) return false;
    Vec3 p = s.origin, v = normalize(s.dir) * clampf(s.speed, 5.0f, 90.0f);
    const float h = 0.05f;
    for (int i = 0; i < 40; ++i) {
        const Vec3 q = p;
        v.y -= kArrowG * h;
        p = p + v * h;
        if (!goodV(p)) return false;
        const float gy = phys.supportBelow(p.x, p.z, q.y + 0.05f);
        if (goodF(gy) && p.y <= gy) { out = Vec3{p.x, gy, p.z}; return true; }
        const Vec3 seg = p - q;
        const float segXZ = std::sqrt(seg.x * seg.x + seg.z * seg.z);
        RayHit rh;
        if (segXZ > 1.0e-4f && phys.rayXZ(q, seg, segXZ, (q.y + p.y) * 0.5f, rh) && rh.hit) {
            out = Vec3{rh.point.x, lerpf(q.y, p.y, clampf(rh.dist / segXZ, 0.0f, 1.0f)), rh.point.z};
            return true;
        }
    }
    out = p;
    return true;
}

void ArrowPool::clear() { for (int i = 0; i < kMax; ++i) a_[i] = Arrow(); }

int ArrowPool::collect(const Vec3& pos, float radius) {
    if (!goodV(pos)) return 0;
    int n = 0;
    for (int i = 0; i < kMax; ++i) {
        Arrow& r = a_[i];
        if (!r.active || !r.stuck) continue;
        const float dx = r.pos.x - pos.x, dz = r.pos.z - pos.z, dy = r.pos.y - pos.y;
        if (dx * dx + dz * dz > radius * radius) continue;
        if (dy < -1.0f || dy > 2.0f) continue;    // boshqa qavat / tom
        r.active = false;
        ++n;
    }
    return n;
}
int  ArrowPool::live() const {
    int c = 0;
    for (int i = 0; i < kMax; ++i) if (a_[i].active && !a_[i].stuck) ++c;
    return c;
}

} // namespace ert
