// Ertugrul :: Physics.cpp
// Parkur uchun sodda "vertikal quti" dunyosi.
//
// Haqiqiy fizika dvigateli YO'Q — har rekvizit XZ tekisligida to'g'ri
// to'rtburchak + [baseY, topY] balandlik oralig'i sifatida saqlanadi.
// Bu Assassin's Creed uslubidagi mexanika (devor topish, chekkaga osilish,
// past to'siqdan sakrab o'tish, tomga chiqish) uchun yetarli va, eng muhimi,
// determinatsiyalangan — bir xil daraja har doim bir xil qutilarni beradi.
//
// Barcha so'rovlar NaN / nol bo'lish / bo'sh ro'yxatdan himoyalangan:
// dunyo qurilmagan bo'lsa ham hech bir funksiya crash bermaydi.

#include "ertugrul/world/Physics.h"
#include "ertugrul/world/Level.h"
#include "ertugrul/gfx/Mesh.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace ert {

namespace {

// ---------------------------------------------------------------------------
// Doimiylar
// ---------------------------------------------------------------------------
constexpr float kBigY      = 1e9f;      // "topilmadi" qiymati
constexpr float kSurfEps   = 0.05f;     // yuza tanlashdagi bag'rikenglik
constexpr float kSlabEps   = 1e-3f;     // quti balandligini tekshirishdagi bo'shashma
constexpr float kMinDir    = 1e-6f;     // nol uzunlikdagi yo'nalish chegarasi
constexpr float kStepUp    = 0.35f;     // shundan past quti — "zina", itarilmaydi
constexpr float kMaxBoxW   = 40.0f;     // bundan keng qutilar tashlab yuboriladi
constexpr float kMinBoxH   = 0.20f;     // bundan past qutilar tashlab yuboriladi

inline bool finite1(float v) { return std::isfinite(v); }
inline bool finite3(const Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

// ---------------------------------------------------------------------------
// Mesh nomini tasniflash uchun yordamchilar
// ---------------------------------------------------------------------------

// Faqat fayl nomini (papkasiz, kichik harfda) qaytaradi — papka nomlari
// ("town", "nature") tasodifan kalit so'zga mos kelib qolmasligi uchun.
std::string baseNameLower(const std::string& path) {
    size_t cut = 0;
    for (size_t i = 0; i < path.size(); ++i)
        if (path[i] == '/' || path[i] == '\\') cut = i + 1;
    std::string out;
    out.reserve(path.size() - cut);
    for (size_t i = cut; i < path.size(); ++i) {
        const char c = path[i];
        out.push_back((c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c);
    }
    return out;
}

inline bool has(const std::string& s, const char* sub) {
    return s.find(sub) != std::string::npos;
}

// Daraxt / buta / o't — ularga chiqib bo'lmaydi va ular ustidan sakralmaydi.
bool isVegetation(const std::string& n) {
    return has(n, "tree")     || has(n, "plant")   || has(n, "grass")  ||
           has(n, "flower")   || has(n, "mushroom")|| has(n, "bush")   ||
           has(n, "fern")     || has(n, "moss")    || has(n, "crops")  ||
           has(n, "leaf")     || has(n, "branch");
}

// Ataylab past to'siq bo'lgan rekvizitlar (panjara, xoda, o'rindiq, gulxan...).
// Ular uchun sakrash chegarasi biroz kengroq: masalan fence.obj real darajalarda
// 1.30 m chiqadi — 1.25 chegarasidan atigi 5 sm baland, lekin uni sakrab
// o'tib bo'lmasa o'yinchi panjara ortida qamalib qoladi.
bool isLowObstacleName(const std::string& n) {
    return has(n, "fence")   || has(n, "log")    || has(n, "bench")  ||
           has(n, "stool")   || has(n, "campfire") || has(n, "rock_small") ||
           has(n, "planks")  || has(n, "stump")  || has(n, "crate")  ||
           has(n, "barrel")  || has(n, "sack")   || has(n, "hay");
}

// Devor / tom / minora — yopishib chiqiladigan yuzalar.
bool isClimbName(const std::string& n) {
    return has(n, "wall")     || has(n, "roof")    || has(n, "planks") ||
           has(n, "tower")    || has(n, "stall")   || has(n, "cart")   ||
           has(n, "tent")     || has(n, "rock_large") || has(n, "bridge") ||
           has(n, "log_stack")|| has(n, "stump_oldtall");
}

// ---------------------------------------------------------------------------
// Nur ↔ AABB (faqat XZ) — "slab" usuli.
// t0 — kirish, t1 — chiqish masofasi. axis: 0 = X yon, 1 = Z yon,
// -1 = nur boshlanishi quti ichida. sign — normal ishorasi (±1).
// ---------------------------------------------------------------------------
bool rayBoxXZ(const Box& b, float ox, float oz, float dx, float dz,
              float maxDist, float& t0, float& t1, int& axis, float& sign) {
    float tmin = 0.0f, tmax = maxDist;
    axis = -1;
    sign = 0.0f;

    // X o'qi bo'yicha plita
    if (std::fabs(dx) < kMinDir) {
        if (ox < b.minX || ox > b.maxX) return false;
    } else {
        const float inv = 1.0f / dx;
        float ta = (b.minX - ox) * inv;
        float tb = (b.maxX - ox) * inv;
        const float sgn = (dx > 0.0f) ? -1.0f : 1.0f;   // kirilgan yon normali
        if (ta > tb) std::swap(ta, tb);
        if (ta > tmin) { tmin = ta; axis = 0; sign = sgn; }
        if (tb < tmax) tmax = tb;
        if (tmin > tmax) return false;
    }

    // Z o'qi bo'yicha plita
    if (std::fabs(dz) < kMinDir) {
        if (oz < b.minZ || oz > b.maxZ) return false;
    } else {
        const float inv = 1.0f / dz;
        float ta = (b.minZ - oz) * inv;
        float tb = (b.maxZ - oz) * inv;
        const float sgn = (dz > 0.0f) ? -1.0f : 1.0f;
        if (ta > tb) std::swap(ta, tb);
        if (ta > tmin) { tmin = ta; axis = 1; sign = sgn; }
        if (tb < tmax) tmax = tb;
        if (tmin > tmax) return false;
    }

    if (tmax < 0.0f) return false;
    t0 = tmin;
    t1 = tmax;
    return true;
}

// ERT_PHYS_DRAW qo'yilganmi (bir marta o'qiladi)
bool physDrawEnabled() {
    static int cached = -1;
    if (cached < 0) {
        char buf[32] = {0};
        const DWORD n = GetEnvironmentVariableA("ERT_PHYS_DRAW", buf, (DWORD)sizeof(buf));
        cached = (n > 0 && n < sizeof(buf) && buf[0] != '0') ? 1 : 0;
    }
    return cached == 1;
}

} // namespace

// ===========================================================================
// Qurish
// ===========================================================================
void PhysicsWorld::clear() {
    boxes_.clear();
    level_ = nullptr;
}

void PhysicsWorld::build(const Level& level) {
    boxes_.clear();
    level_ = &level;

    const std::vector<Prop>& props = level.props();
    boxes_.reserve(props.size());

    for (size_t i = 0; i < props.size(); ++i) {
        const Prop& p = props[i];

        // Mesh yuklanmagan bo'lsa quti qurib bo'lmaydi (o'lchamlar noma'lum).
        // Bunday rekvizitlar uchun Level::resolveCollision doiraviy to'qnashuvi
        // ishlab turadi — parkur esa ularsiz ham xavfsiz.
        const Mesh* m = p.cached;
        if (m == nullptr || !m->valid()) continue;
        if (!finite3(p.pos) || !finite1(p.yaw) || !finite1(p.scale)) continue;

        const float sc = (p.scale > 0.0001f && std::isfinite(p.scale)) ? p.scale : 1.0f;
        const Vec3 lo = m->bbMin();
        const Vec3 hi = m->bbMax();
        if (!finite3(lo) || !finite3(hi)) continue;

        // 1) Mahalliy bbox ni masshtablaymiz
        const float x0 = lo.x * sc, x1 = hi.x * sc;
        const float z0 = lo.z * sc, z1 = hi.z * sc;

        // 2) 4 burchakni yaw bo'yicha aylantirib, XZ da o'rab oluvchi
        //    to'g'ri to'rtburchakni topamiz (Mat4::rotateY bilan bir xil konvensiya)
        const float rad = deg2rad(p.yaw);
        const float cs = std::cos(rad), sn = std::sin(rad);
        const float cx[4] = { x0, x0, x1, x1 };
        const float cz[4] = { z0, z1, z0, z1 };
        float rminX = 0.0f, rmaxX = 0.0f, rminZ = 0.0f, rmaxZ = 0.0f;
        for (int c = 0; c < 4; ++c) {
            const float rx =  cs * cx[c] + sn * cz[c];
            const float rz = -sn * cx[c] + cs * cz[c];
            if (c == 0) { rminX = rmaxX = rx; rminZ = rmaxZ = rz; }
            else {
                if (rx < rminX) rminX = rx;
                if (rx > rmaxX) rmaxX = rx;
                if (rz < rminZ) rminZ = rz;
                if (rz > rmaxZ) rmaxZ = rz;
            }
        }

        Box b;
        b.minX = p.pos.x + rminX;
        b.maxX = p.pos.x + rmaxX;
        b.minZ = p.pos.z + rminZ;
        b.maxZ = p.pos.z + rmaxZ;

        // 3) Balandlik. Rekvizit chizilganda ham xuddi shu asosdan boshlanadi.
        b.baseY = p.snapToGround ? level.groundAt(p.pos.x, p.pos.z) : p.pos.y;
        if (!finite1(b.baseY)) b.baseY = 0.0f;
        const float hgt = m->height() * sc;
        if (!finite1(hgt)) continue;
        b.topY = b.baseY + hgt;

        // 4) Foydasiz qutilarni tashlab yuboramiz
        const float wx = b.maxX - b.minX;
        const float wz = b.maxZ - b.minZ;
        if (!finite1(wx) || !finite1(wz)) continue;
        if (hgt < kMinBoxH) continue;                       // o't, mayda tosh, gul
        if (wx > kMaxBoxW || wz > kMaxBoxW) continue;        // ulkan "yer" plitalari

        // 5) Tasnif
        const std::string name = baseNameLower(p.mesh);
        const bool veg = isVegetation(name);

        b.climbable = !veg && isClimbName(name) && hgt >= 0.9f && hgt <= 8.0f;
        // Past to'siq (panjara, log, kursi, gulxan, kichik tosh, taxta).
        // Balandligi 0.9..1.25 bo'lgan devor ham sakrab o'tishga yaroqli —
        // ikkala bayroq bir vaqtda yoqilgan bo'lishi mumkin.
        b.vaultable = !veg && hgt >= 0.35f &&
                      (hgt <= 1.25f || (!b.climbable && hgt <= 1.45f && isLowObstacleName(name)));
        b.solid = p.collide || b.climbable;
        b.propIndex = (int)i;

        boxes_.push_back(b);
    }
}

// ===========================================================================
// Yer va platformalar
// ===========================================================================
float PhysicsWorld::groundAt(float x, float z) const {
    if (level_ == nullptr) return 0.0f;
    if (!finite1(x) || !finite1(z)) return 0.0f;
    const float g = level_->groundAt(x, z);
    return finite1(g) ? g : 0.0f;
}

float PhysicsWorld::supportBelow(float x, float z, float fromY) const {
    if (!finite1(x) || !finite1(z)) return 0.0f;
    if (!finite1(fromY)) fromY = 0.0f;

    float best = groundAt(x, z);
    // Quti tepasi fromY dan yuqori bo'lmasligi kerak; kSurfEps — oyoq
    // aynan quti ustida turganda uni "tayanch" deb hisoblash uchun bag'rikenglik.
    const float limit = fromY + kSurfEps;

    for (size_t i = 0; i < boxes_.size(); ++i) {
        const Box& b = boxes_[i];
        if (!b.solid && !b.vaultable) continue;   // bezak qutilari tayanch bermaydi
        if (!b.containsXZ(x, z)) continue;
        if (b.topY > limit) continue;
        if (b.topY > best) best = b.topY;
    }
    return best;
}

float PhysicsWorld::platformAbove(float x, float z, float fromY) const {
    if (!finite1(x) || !finite1(z)) return kBigY;
    if (!finite1(fromY)) return kBigY;

    float best = kBigY;
    for (size_t i = 0; i < boxes_.size(); ++i) {
        const Box& b = boxes_[i];
        if (!b.solid && !b.vaultable) continue;
        if (!b.containsXZ(x, z)) continue;
        if (b.topY <= fromY + kSurfEps) continue;
        if (b.topY < best) best = b.topY;
    }
    return best;
}

// ===========================================================================
// Nur tashlash (XZ)
// ===========================================================================
bool PhysicsWorld::rayXZ(const Vec3& origin, const Vec3& dir, float maxDist,
                         float atY, RayHit& out) const {
    out = RayHit();
    if (!finite3(origin) || !finite3(dir) || !finite1(maxDist) || !finite1(atY)) return false;
    if (!(maxDist > 0.0f)) return false;

    float dx = dir.x, dz = dir.z;
    const float len = std::sqrt(dx * dx + dz * dz);
    if (!(len > kMinDir)) return false;          // yo'nalish XZ da nolga teng
    dx /= len;
    dz /= len;

    float bestT = maxDist;
    int   bestI = -1, bestAxis = -1;
    float bestSign = 0.0f;

    for (size_t i = 0; i < boxes_.size(); ++i) {
        const Box& b = boxes_[i];
        if (atY < b.baseY - kSlabEps || atY > b.topY + kSlabEps) continue;

        float t0 = 0.0f, t1 = 0.0f, sg = 0.0f;
        int ax = -1;
        if (!rayBoxXZ(b, origin.x, origin.z, dx, dz, maxDist, t0, t1, ax, sg)) continue;
        if (t0 > maxDist) continue;
        if (t0 < bestT || bestI < 0) {
            bestT    = t0;
            bestI    = (int)i;
            bestAxis = ax;
            bestSign = sg;
        }
    }

    if (bestI < 0) return false;
    if (bestT > maxDist) return false;

    const Box& hb = boxes_[(size_t)bestI];
    out.hit    = true;
    out.dist   = bestT;
    out.point  = Vec3(origin.x + dx * bestT, origin.y, origin.z + dz * bestT);
    if (bestAxis == 0)      out.normal = Vec3(bestSign, 0.0f, 0.0f);
    else if (bestAxis == 1) out.normal = Vec3(0.0f, 0.0f, bestSign);
    else                    out.normal = Vec3(-dx, 0.0f, -dz);   // boshlanish quti ichida
    out.box       = bestI;
    out.topY      = hb.topY;
    out.climbable = hb.climbable;
    return true;
}

// ===========================================================================
// Yon to'qnashuv
// ===========================================================================
bool PhysicsWorld::resolve(Vec3& pos, float radius, float bodyHeight) const {
    if (boxes_.empty()) return false;
    if (!finite3(pos) || !finite1(radius) || !finite1(bodyHeight)) return false;

    const float r  = (radius > 0.01f) ? radius : 0.01f;
    const float bh = (bodyHeight > 0.2f) ? bodyHeight : 0.2f;
    const float bodyMin = pos.y + 0.1f;          // oyoqdan biroz yuqori
    const float bodyMax = pos.y + bh * 0.9f;     // yelka sathi
    const float stepTop = pos.y + kStepUp;       // shundan past — zina

    bool movedAny = false;

    // Bir necha marta takrorlaymiz: bir vaqtda ikki-uch qutiga tegib turgan
    // holatda bitta o'tish yetarli bo'lmaydi.
    for (int iter = 0; iter < 4; ++iter) {
        bool moved = false;

        for (size_t i = 0; i < boxes_.size(); ++i) {
            const Box& b = boxes_[i];
            if (!b.solid) continue;
            if (b.topY <= stepTop) continue;             // ustiga chiqib ketiladi
            if (b.topY < bodyMin || b.baseY > bodyMax) continue;   // vertikal kesishmaydi

            // Doira ↔ AABB (XZ)
            const float cx = clampf(pos.x, b.minX, b.maxX);
            const float cz = clampf(pos.z, b.minZ, b.maxZ);
            const float dx = pos.x - cx;
            const float dz = pos.z - cz;
            const float d2 = dx * dx + dz * dz;

            // Aynan yuzada turgan holat ham "tegmaydi" deb hisoblanadi —
            // aks holda nol uzunlikdagi itarish takrorlanishlarni behuda yeydi.
            if (d2 >= r * r - 1e-7f) continue;

            if (d2 > 1e-8f) {
                const float d = std::sqrt(d2);
                const float push = r - d;
                pos.x += (dx / d) * push;
                pos.z += (dz / d) * push;
            } else {
                // Markaz quti ichida — eng kam siljish yo'nalishini tanlaymiz
                const float pxMin = (pos.x - b.minX) + r;   // -X tomon
                const float pxMax = (b.maxX - pos.x) + r;   // +X tomon
                const float pzMin = (pos.z - b.minZ) + r;   // -Z tomon
                const float pzMax = (b.maxZ - pos.z) + r;   // +Z tomon
                float best = pxMin;
                int   axis = 0;
                if (pxMax < best) { best = pxMax; axis = 1; }
                if (pzMin < best) { best = pzMin; axis = 2; }
                if (pzMax < best) { best = pzMax; axis = 3; }
                if      (axis == 0) pos.x -= pxMin;
                else if (axis == 1) pos.x += pxMax;
                else if (axis == 2) pos.z -= pzMin;
                else                pos.z += pzMax;
            }
            moved = true;
            movedAny = true;
        }

        if (!moved) break;
    }

    if (!finite3(pos)) pos = Vec3(0.0f, pos.y, 0.0f);
    return movedAny;
}

// ===========================================================================
// Parkur zondlari
// ===========================================================================
bool PhysicsWorld::probeWall(const Vec3& feet, const Vec3& fwd, float atHeight,
                             float reach, RayHit& out) const {
    out = RayHit();
    if (!finite3(feet) || !finite3(fwd) || !finite1(atHeight) || !finite1(reach)) return false;

    const Vec3 org(feet.x, feet.y + atHeight, feet.z);
    if (!rayXZ(org, fwd, reach, feet.y + atHeight, out)) return false;
    if (!out.climbable) { out = RayHit(); return false; }
    return true;
}

bool PhysicsWorld::probeLedge(const Vec3& feet, const Vec3& fwd, float minH, float maxH,
                              float reach, float bodyRadius, LedgeInfo& out) const {
    out = LedgeInfo();
    if (!finite3(feet) || !finite3(fwd)) return false;
    if (!finite1(minH) || !finite1(maxH) || !finite1(reach) || !finite1(bodyRadius)) return false;
    if (!(reach > 0.0f)) return false;

    float lo = minH, hi = maxH;
    if (lo > hi) std::swap(lo, hi);
    const float br = (bodyRadius > 0.05f) ? bodyRadius : 0.05f;

    // 1) minH..maxH oralig'ida bir necha balandlikda oldinga nur — birinchi
    //    "chiqsa bo'ladigan" urilishni qidiramiz.
    RayHit hit;
    bool   got = false;
    const int kSteps = 6;
    for (int s = 0; s < kSteps; ++s) {
        const float t = (kSteps > 1) ? (float)s / (float)(kSteps - 1) : 0.0f;
        const float h = lerpf(lo, hi, t);
        RayHit tmp;
        const Vec3 org(feet.x, feet.y + h, feet.z);
        if (!rayXZ(org, fwd, reach, feet.y + h, tmp)) continue;
        if (!tmp.climbable) continue;
        hit = tmp;
        got = true;
        break;
    }
    if (!got) return false;

    // 2) Qutining tepasi ham shu oraliqda bo'lishi shart. Aks holda bu chekka
    //    emas, balki baland devor — unga osilib bo'lmaydi.
    const float rel = hit.topY - feet.y;
    if (rel < lo || rel > hi) return false;

    // 3) Ushlash nuqtasi va tepadagi oyoq nuqtasi
    Vec3 n = hit.normal;
    n.y = 0.0f;
    n = normalize(n);
    if (!(std::fabs(n.x) + std::fabs(n.z) > kMinDir)) return false;

    Vec3 grab = hit.point;
    grab.y = hit.topY - 0.08f;                 // qo'l qirradan biroz pastda

    Vec3 top = grab - n * (br + 0.35f);        // devor ortiga — tomga
    top.y = hit.topY;

    out.found       = true;
    out.grab        = grab;
    out.top         = top;
    out.normal      = n;
    out.height      = rel;
    out.box         = hit.box;
    // 4) Tepada tik turishga joy bormi
    out.mantleClear = mantleClear(top, br, 1.85f);
    return true;
}

bool PhysicsWorld::probeVault(const Vec3& feet, const Vec3& fwd, float maxHeight,
                              float reach, float bodyRadius, VaultInfo& out) const {
    out = VaultInfo();
    if (!finite3(feet) || !finite3(fwd)) return false;
    if (!finite1(maxHeight) || !finite1(reach) || !finite1(bodyRadius)) return false;
    if (!(reach > 0.0f)) return false;

    const float br = (bodyRadius > 0.05f) ? bodyRadius : 0.05f;

    // Bel sathida oldinga nur
    const float probeH = 0.45f;
    const Vec3  org(feet.x, feet.y + probeH, feet.z);
    RayHit hit;
    if (!rayXZ(org, fwd, reach, feet.y + probeH, hit)) return false;
    if (hit.box < 0 || (size_t)hit.box >= boxes_.size()) return false;

    const Box& b = boxes_[(size_t)hit.box];
    if (!b.vaultable) return false;
    if (hit.topY - feet.y > maxHeight) return false;

    // Yo'nalishni normallashtiramiz (chuqurlik va chiqish nuqtasi uchun)
    float dx = fwd.x, dz = fwd.z;
    const float len = std::sqrt(dx * dx + dz * dz);
    if (!(len > kMinDir)) return false;
    dx /= len;
    dz /= len;

    // To'siqning fwd yo'nalishidagi qalinligi = nurning kirish/chiqish farqi
    float t0 = 0.0f, t1 = 0.0f, sg = 0.0f;
    int   ax = -1;
    if (!rayBoxXZ(b, org.x, org.z, dx, dz, reach + 8.0f, t0, t1, ax, sg)) return false;
    float depth = t1 - t0;
    if (!finite1(depth) || depth < 0.0f) depth = 0.0f;
    if (depth > 2.0f) return false;              // bu to'siq emas, devor

    // Narigi tomondagi qo'nish nuqtasi
    Vec3 exit(hit.point.x + dx * (depth + br + 0.35f),
              feet.y,
              hit.point.z + dz * (depth + br + 0.35f));
    // Sakrab tushiladigan yuza oyoqdan biroz baland ham bo'lishi mumkin
    exit.y = supportBelow(exit.x, exit.z, feet.y + 0.75f);
    if (!finite3(exit)) return false;

    // U yerda turishga joy bo'lishi shart
    if (!mantleClear(exit, br, 1.70f)) return false;

    out.found = true;
    out.topY  = hit.topY;
    out.depth = depth;
    out.exit  = exit;
    out.box   = hit.box;
    return true;
}

bool PhysicsWorld::canShimmy(const Vec3& grab, const Vec3& normal, float dir, float step) const {
    if (!finite3(grab) || !finite3(normal) || !finite1(dir) || !finite1(step)) return false;
    if (!(std::fabs(step) > 1e-4f) || !(std::fabs(dir) > 1e-4f)) return false;

    Vec3 n(normal.x, 0.0f, normal.z);
    n = normalize(n);
    if (!(std::fabs(n.x) + std::fabs(n.z) > kMinDir)) return false;

    // Devor bo'ylab yon yo'nalish
    const Vec3 up(0.0f, 1.0f, 0.0f);
    Vec3 side = normalize(cross(up, n));
    if (!(std::fabs(side.x) + std::fabs(side.z) > kMinDir)) return false;

    const Vec3 p = grab + side * (dir * step);

    // Yangi nuqtadan devor tomon (normalga qarama-qarshi) qisqa nur.
    // Boshlanishni biroz tashqariga suramiz — aks holda nur aynan yuzadan
    // boshlanib, qutining ichida qolib ketadi.
    const float back = 0.25f;
    const Vec3  org  = p + n * back;
    RayHit hit;
    if (!rayXZ(org, -n, back + 0.6f, p.y, hit)) return false;
    if (!hit.climbable) return false;

    // Chekka sathi bir xil bo'lishi kerak (burchakdan burilib ketmaslik uchun)
    if (std::fabs(hit.topY - grab.y) > 0.15f) return false;
    return true;
}

bool PhysicsWorld::mantleClear(const Vec3& top, float bodyRadius, float bodyHeight) const {
    if (!finite3(top) || !finite1(bodyRadius) || !finite1(bodyHeight)) return false;

    const float r  = (bodyRadius > 0.05f) ? bodyRadius : 0.05f;
    const float bh = (bodyHeight > 0.2f)  ? bodyHeight : 0.2f;
    const float lo = top.y + 0.1f;
    const float hi = top.y + bh;

    for (size_t i = 0; i < boxes_.size(); ++i) {
        const Box& b = boxes_[i];
        if (!b.solid) continue;
        if (b.topY <= lo || b.baseY >= hi) continue;     // vertikal kesishmaydi

        const float cx = clampf(top.x, b.minX, b.maxX);
        const float cz = clampf(top.z, b.minZ, b.maxZ);
        const float dx = top.x - cx;
        const float dz = top.z - cz;
        if (dx * dx + dz * dz < r * r) return false;      // gavda joylashmaydi
    }
    return true;
}

float PhysicsWorld::fallDistance(const Vec3& feet) const {
    if (!finite3(feet)) return 0.0f;
    const float sup = supportBelow(feet.x, feet.z, feet.y);
    const float d = feet.y - sup;
    if (!finite1(d) || d < 0.0f) return 0.0f;
    return d;
}

// ===========================================================================
// Diagnostika: ERT_PHYS_DRAW=1 bo'lganda qutilarni simli chizadi
// ===========================================================================
void PhysicsWorld::debugDraw() const {
    if (boxes_.empty()) return;
    if (!physDrawEnabled()) return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT | GL_TEXTURE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glLineWidth(1.5f);

    glBegin(GL_LINES);
    for (size_t i = 0; i < boxes_.size(); ++i) {
        const Box& b = boxes_[i];

        if (b.climbable)      glColor3f(0.282f, 0.663f, 0.710f);   // feruza #48A9B5
        else if (b.vaultable) glColor3f(0.753f, 0.588f, 0.376f);   // zarhal #C09660
        else                  glColor3f(0.550f, 0.550f, 0.580f);   // kulrang

        const float x0 = b.minX, x1 = b.maxX;
        const float z0 = b.minZ, z1 = b.maxZ;
        const float y0 = b.baseY, y1 = b.topY;

        // Pastki to'rtburchak
        glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0);
        glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1);
        glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
        glVertex3f(x0, y0, z1); glVertex3f(x0, y0, z0);
        // Yuqorigi to'rtburchak
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
        glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
        // Vertikal qirralar
        glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0);
        glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
        glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
        glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1);
    }
    glEnd();

    glPopAttrib();
}

} // namespace ert
