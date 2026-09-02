#pragma once
// Skeletsiz OBJ modelni chegaraviy quti bo'yicha avtomatik "rigging" qilib,
// yurish/gapirish/turish animatsiyalarini beradi (CPU skinning).
#include <string>
#include <vector>
#include "ertugrul/core/Math.h"
#include "ertugrul/gfx/Mesh.h"

namespace ert {

enum class Bone : unsigned char {
    Root = 0, Pelvis, Torso, Head,
    ArmUpperL, ArmLowerL, ArmUpperR, ArmLowerR,
    LegUpperL, LegLowerL, LegUpperR, LegLowerR,
    Count
};

enum class AnimClip {
    Idle = 0,      // tinch turish, nafas
    Walk,          // yurish
    Run,           // yugurish
    Talk,          // gapirish (qo'l ishoralari)
    Listen,        // tinglash (bosh egish)
    Point,         // ko'rsatish
    Salute,        // qo'l ko'ksiga
    Draw,          // qilich tortish
    Sit,           // o'tirish

    // --- Parkur va jang klipari (AC uslubi) ---
    CrouchIdle,    // cho'kkalab turish
    CrouchWalk,    // cho'kkalab yurish
    Vault,         // past to'siqdan sakrab o'tish (0.55 s)
    Mantle,        // chekkadan tepaga chiqish (0.85 s)
    ClimbUp,       // devorga yopishib ko'tarilish (siklik)
    Hang,          // chekkada osilib turish
    Shimmy,        // osilgan holda yon siljish
    Slide,         // yugurishdan cho'kkalab sirg'alish (0.7 s)
    Roll,          // qo'ngandan keyin dumalash (0.6 s)
    Fall,          // havoda
    WallRun,       // devor bo'ylab yugurish
    Dodge,         // chetlanish (0.45 s)
    Assassinate,   // yashirin o'ldirish (1.1 s)

    // --- Jang kliplari ---
    AttackLight1,  // yengil zarba, kombo 1 (0.55 s)
    AttackLight2,  // kombo 2 — teskari tomondan (0.50 s)
    AttackLight3,  // kombo 3 — yakunlovchi (0.70 s)
    AttackHeavy,   // og'ir zarba, tepadan (0.95 s)
    KickClip,      // tepish (0.45 s)
    Block,         // qalqon/qilich bilan himoya (ushlab turiladi)
    ParryHit,      // muvaffaqiyatli parry (0.35 s)
    Hurt,          // zarba yeyish reaksiyasi (0.35 s)
    Stagger,       // poza buzildi — muvozanatni yo'qotish (1.2 s)
    Death,         // halok bo'lish (1.4 s)
    BowAim,        // kamon tortish (ushlab turiladi)
    BowShoot,      // o'q otish (0.4 s)
    Execute,       // yakunlovchi zarba (staggered raqibga, 1.3 s)
    Count
};

const char* animClipName(AnimClip c);
AnimClip    animClipFromName(const std::string& s);   // topilmasa Idle

// Qadam sikli konteksti — poseStride() ga uzatiladi.
// strideN: to'liq sikl (ikki qadam) uzunligi / model balandligi
// duty:    tayanch ulushi. >0.5 = ikki tayanch (yurish), <0.5 = uchish fazasi (yugurish)
struct StrideCtx {
    float phase     = 0.0f;    // 0..1
    float strideN   = 0.79f;
    float duty      = 0.55f;
    float amp       = 1.0f;    // ustki tana amplitudasi
    float lean      = 3.0f;    // oldinga egilish (gradus)
    float bank      = 0.0f;    // yon egilish (burilish inersiyasi)
    float moveAngle = 0.0f;    // harakat / tana burchagi (strafe warping)
    float headLead  = 0.0f;    // bosh burilishni yetaklaydi
    float slope     = 0.0f;    // yer nishabi harakat yo'nalishida (ko'tarilish/m, tan)
    // --- Zarba reaksiyasi (poseHurt / poseStagger / poseDeath) ---
    float hitDir    = 0.0f;    // hujumchi burchagi tana o'qiga nisbatan:
                               //   0 = old, +90 = +X tomon, +-180 = orqa
    float hitWeight = 1.0f;    // 0.35 (o'q/tepish) .. 1.00 (og'ir zarba)
};

class SkinnedModel {
public:
    SkinnedModel() = default;

    // mesh egasi emas (Mesh::get() keshi egalik qiladi)
    bool init(Mesh* mesh);
    bool valid() const { return mesh_ != nullptr; }
    Mesh* mesh() const { return mesh_; }

    // Animatsiya
    // Klipni TANLAYDI. Agar shu klip allaqachon o'ynayotgan bo'lsa hech narsa
    // qilmaydi — ya'ni har kadr chaqirish xavfsiz.
    void     setClip(AnimClip c, float blendTime = 0.25f);
    // Klipni BOSHIDAN ishga tushiradi. Bir martalik kliplar (zarba, chetlanish,
    // yakunlovchi zarba) uchun MAJBURIY: yangi harakat boshlanganda chaqiriladi.
    void     playClip(AnimClip c, float blendTime = 0.25f);
    AnimClip clip() const { return clip_; }
    void     setSpeedScale(float s) { speedScale_ = s; }
    void     update(float dt);
    float    animTime() const { return time_; }
    // Yurish tezligiga qarab Walk/Run/Idle ni avtomatik tanlaydi (m/s).
    // Eski imzo SAQLANADI — Enemy.cpp va Cutscene.cpp shu yerdan kiradi.
    void     driveByLocomotion(float speedMs, float dt);
    // dsMeters — shu kadrda HAQIQATDA bosib o'tilgan gorizontal masofa (m).
    // Qadam fazasi vaqtdan emas, ANA SHU masofadan haydaladi — oyoq sirg'almaydi.
    // < 0 bo'lsa speedMs*dt ishlatiladi (eski chaqiruvchilar uchun moslik).
    void     driveByLocomotion(float speedMs, float dsMeters, float dt);
    // Klip tanlamasdan faqat lokomotsiya ma'lumotini berish (default: tarmog'i uchun)
    void     setLocomotion(float speedMs, float dsMeters);
    // Personaj bo'yi (m) — qadam uzunligi shunga masshtablanadi (standart 1.82)
    void     setBodyHeightMeters(float m) { bodyM_ = (m > 0.3f) ? m : 1.82f; }
    // Inersiyaning VIZUAL qismi (Character hisoblaydi). moveAngleDeg — tana o'qiga
    // nisbatan harakat burchagi (-180..180), lock-on strafe uchun.
    void     setLocomotionPose(float bankDeg, float leanDeg, float headLeadDeg,
                              float moveAngleDeg, float turnStepHz);
    // Yer nishabi harakat yo'nalishida (tan: +0.3 = 100 m da 30 m ko'tarilish).
    // Tepalikda oldingi panja BALANDROQ tushadi, tana qiyalikka egiladi.
    void     setGroundSlope(float rise) { slope_ = (rise == rise && rise < 9.0f && rise > -9.0f) ? rise : 0.0f; }
    // 0..1 qadam sikli (0 = chap tovon tegdi) — diagnostika va oyoq tovushi uchun
    float    stridePhase() const { return phase_; }
    // Zarba yo'nalishi va og'irligi. receiveHit() ichida bir marta chaqiriladi.
    void     setHitDir(float dirDeg, float weight);
    // Gapirish og'iz/qo'l intensivligi 0..1 (subtitr chiqib turganda 1 ga yaqin)
    void     setTalkIntensity(float t) { talk_ = saturate(t); }

    // Chizish: pos = oyoq ostidagi nuqta, yaw = gradus, scale = model balandligini ko'paytiruvchi
    void draw(const Vec3& pos, float yawDeg, float scale);
    // Faqat skinlash (chizmasdan) — masalan soya uchun
    const std::vector<MeshVertex>& skinnedVertices() const { return skinned_; }

    // Suyak jahon-fazodagi joylashuvi (qurol biriktirish, kamera nishoni uchun)
    Vec3 bonePosition(Bone b, const Vec3& pos, float yawDeg, float scale) const;

    // Ishlash: sekundiga necha marta qayta skinlash (standart 30)
    static void setSkinRateHz(float hz);

private:
    Mesh*  mesh_ = nullptr;
    std::vector<MeshVertex> skinned_;
    std::vector<unsigned char> boneIdx_;   // har verteks uchun asosiy suyak
    std::vector<unsigned char> boneIdx2_;  // ikkinchi suyak (segment rigida qo'shni, quti rigida ota)
    std::vector<float>         boneW_;     // asosiy suyak og'irligi (1-w ikkinchisiga)
    // Skelet ta'rifi: bo'g'im va suyak uchlari (normallashtirilgan), <obj>.rig.json
    // dan yuklanadi, bo'lmasa standart jadval. 12 ta Vec3 x 2 + blend.
    float rigJoint_[12][3] = {{0}};
    float rigEnd_[12][3]   = {{0}};
    float rigBlend_        = 0.06f;
    bool  rigSegment_      = true;        // false = eski quti (Y-band) rig
    AnimClip clip_ = AnimClip::Idle, prevClip_ = AnimClip::Idle;
    float time_ = 0.0f, blend_ = 1.0f, blendDur_ = 0.25f;
    float speedScale_ = 1.0f, talk_ = 0.0f, accum_ = 0.0f;
    float modelHeight_ = 1.0f;
    bool  dirty_ = true;
    // Past-poligonli mesh (< 2000 verteks): quti bo'yicha rigging meshni yirtadi,
    // shuning uchun faqat ildiz/tana harakati qo'llanadi (a'zolar aylanmaydi).
    bool  simpleRig_ = false;
    float neckN_ = 0.86f;       // meshdan o'lchangan bo'yin balandligi (0..1)
    float hitDir_ = 0.0f, hitW_ = 1.0f;   // oxirgi zarbaning yo'nalishi va og'irligi

    // --- Qadam sikli: MASOFA integrali (vaqt emas) ---
    float phase_      = 0.0f;   // 0..1
    float lastSkinPh_ = 0.0f;   // oxirgi skinlashdagi faza (adaptiv darvoza)
    float locoSpeed_  = 0.0f;   // m/s
    float locoDs_     = -1.0f;  // shu kadrdagi siljish (m); < 0 => speed*dt
    float bodyM_      = 1.82f;  // personaj bo'yi (m)
    float bankDeg_ = 0.0f, leanDeg_ = 0.0f, headLead_ = 0.0f;
    float moveAngle_ = 0.0f, turnStepHz_ = 0.0f;
    float slope_     = 0.0f;    // yer nishabi (tan)
    float phaseDir_  = 1.0f;    // +1 oldinga, -1 orqaga

    // Poza konteksti (bonePosition() const bo'lgani uchun bu ham const)
    StrideCtx buildStrideCtx() const;
};

} // namespace ert
