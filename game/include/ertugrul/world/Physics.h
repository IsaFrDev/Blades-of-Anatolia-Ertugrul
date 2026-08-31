#pragma once
// Parkur uchun to'qnashuv va "zond" (probe) dunyosi.
//
// Daraja rekvizitlaridan sodda VERTIKAL QUTILAR quriladi (XZ da to'g'ri to'rtburchak
// + pastki/yuqori balandlik). Bu Assassin's Creed uslubidagi mexanika uchun yetarli:
// nur tashlash (raycast), devor topish, chekka (ledge) topish, sakrab o'tish (vault),
// tomga chiqish. Haqiqiy fizika dvigateli emas — ataylab sodda va determinatsiyalangan.
#include <vector>
#include <string>
#include "ertugrul/core/Math.h"

namespace ert {

class Level;

// Rekvizitning to'qnashuv qutisi
struct Box {
    float minX = 0, maxX = 0, minZ = 0, maxZ = 0;
    float baseY = 0, topY = 0;
    bool  climbable = false;      // devor/tom — yopishib chiqish mumkin
    bool  vaultable = false;      // past to'siq — sakrab o'tiladi
    bool  solid     = true;       // yon to'qnashuv beradimi
    int   propIndex = -1;
    bool  containsXZ(float x, float z) const { return x >= minX && x <= maxX && z >= minZ && z <= maxZ; }
    float centerX() const { return (minX + maxX) * 0.5f; }
    float centerZ() const { return (minZ + maxZ) * 0.5f; }
};

struct RayHit {
    bool  hit = false;
    float dist = 0.0f;
    Vec3  point{0, 0, 0};
    Vec3  normal{0, 0, 0};        // XZ tekisligidagi yuza normali
    int   box = -1;
    float topY = 0.0f;            // urilgan qutining tepasi
    bool  climbable = false;
};

// Chekka (ledge) haqidagi ma'lumot — osilib turish va tepaga chiqish uchun
struct LedgeInfo {
    bool  found = false;
    Vec3  grab{0, 0, 0};          // qo'l ushlaydigan nuqta (chekka qirrasi)
    Vec3  top{0, 0, 0};           // tepaga chiqqandagi oyoq nuqtasi
    Vec3  normal{0, 0, 0};        // devordan tashqariga yo'nalgan normal
    float height = 0.0f;          // oyoq sathidan chekkagacha
    bool  mantleClear = false;    // tepada turish uchun joy bormi
    int   box = -1;
};

// Past to'siqdan sakrab o'tish
struct VaultInfo {
    bool  found = false;
    float topY = 0.0f;            // to'siq tepasi
    float depth = 0.0f;           // to'siq qalinligi (harakat yo'nalishida)
    Vec3  exit{0, 0, 0};          // narigi tomondagi qo'nish nuqtasi
    int   box = -1;
};

class PhysicsWorld {
public:
    // Darajadan qutilarni quradi. Level o'zgarganda qayta chaqiriladi.
    void build(const Level& level);
    void clear();
    bool valid() const { return !boxes_.empty() || level_ != nullptr; }

    const std::vector<Box>& boxes() const { return boxes_; }

    // --- Yer ---
    float groundAt(float x, float z) const;
    // Berilgan nuqtadan YUQORIDA turgan eng past platforma (tom/quti tepasi).
    // Topilmasa juda katta qiymat qaytaradi.
    float platformAbove(float x, float z, float fromY) const;
    // Berilgan nuqtadan PASTDA turgan eng baland yuza (yer yoki quti tepasi).
    float supportBelow(float x, float z, float fromY) const;

    // --- Nur tashlash (faqat XZ tekisligida, berilgan balandlikda) ---
    bool rayXZ(const Vec3& origin, const Vec3& dir, float maxDist, float atY, RayHit& out) const;

    // --- Yon to'qnashuv: pos ni qutilardan itaradi ---
    bool resolve(Vec3& pos, float radius, float bodyHeight) const;

    // --- Parkur zondlari ---
    // Oldindagi devorni topadi (ko'krak balandligida)
    bool probeWall(const Vec3& feet, const Vec3& fwd, float atHeight, float reach, RayHit& out) const;
    // Oldindagi chekka: minH..maxH oralig'ida ushlab olsa bo'ladigan qirra
    bool probeLedge(const Vec3& feet, const Vec3& fwd, float minH, float maxH,
                    float reach, float bodyRadius, LedgeInfo& out) const;
    // Past to'siq (sakrab o'tish)
    bool probeVault(const Vec3& feet, const Vec3& fwd, float maxHeight,
                    float reach, float bodyRadius, VaultInfo& out) const;
    // Osilib turgan holatda yon tomonga siljish mumkinmi
    bool canShimmy(const Vec3& grab, const Vec3& normal, float dir, float step) const;
    // Osilgan chekkadan tepaga chiqish uchun joy bormi
    bool mantleClear(const Vec3& top, float bodyRadius, float bodyHeight) const;
    // Tushish uchun xavfsiz balandlik (past qism topilmasa 1e9)
    float fallDistance(const Vec3& feet) const;

    // --- Diagnostika ---
    void debugDraw() const;              // qutilarni simli chizadi (ERT_PHYS_DRAW=1)
    size_t boxCount() const { return boxes_.size(); }

private:
    std::vector<Box> boxes_;
    const Level* level_ = nullptr;
};

} // namespace ert
