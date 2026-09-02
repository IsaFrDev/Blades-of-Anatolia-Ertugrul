// Skin.cpp â€” skeletsiz OBJ modellar uchun chegaraviy quti bo'yicha avtomatik "rigging".
//
// Modellarda suyak yo'q: oddiy statik OBJ (balandligi ~1.0, oyoq y=0, bosh y=1).
// Shuning uchun har bir verteks bbox ichidagi normallashtirilgan joylashuviga qarab
// 12 ta virtual suyakdan biriga biriktiriladi, chegaralarda og'irlik yumshatiladi.
// Animatsiya CPU da hisoblanadi (skinning) va sekundiga 30 marta (standart) yangilanadi.
#include "ertugrul/gfx/Skin.h"

#include <windows.h>   // <GL/gl.h> dan OLDIN kelishi SHART
#include <GL/gl.h>

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

namespace ert {

namespace {

// ---------------------------------------------------------------------------
//  Suyak jadvali
// ---------------------------------------------------------------------------

constexpr int BCOUNT = static_cast<int>(Bone::Count);

constexpr int B_ROOT   = static_cast<int>(Bone::Root);
constexpr int B_PELVIS = static_cast<int>(Bone::Pelvis);
constexpr int B_TORSO  = static_cast<int>(Bone::Torso);
constexpr int B_HEAD   = static_cast<int>(Bone::Head);
constexpr int B_AUL    = static_cast<int>(Bone::ArmUpperL);
constexpr int B_ALL_   = static_cast<int>(Bone::ArmLowerL);
constexpr int B_AUR    = static_cast<int>(Bone::ArmUpperR);
constexpr int B_ALR    = static_cast<int>(Bone::ArmLowerR);
constexpr int B_LUL    = static_cast<int>(Bone::LegUpperL);
constexpr int B_LLL    = static_cast<int>(Bone::LegLowerL);
constexpr int B_LUR    = static_cast<int>(Bone::LegUpperR);
constexpr int B_LLR    = static_cast<int>(Bone::LegLowerR);

// Suyakning tayanch (aylanish) nuqtasi NORMALLASHTIRILGAN koordinatada:
//   px â€” bbox yarim kengligiga nisbatan (-1..1), py â€” balandlikka nisbatan (0=oyoq, 1=bosh),
//   pz â€” bbox yarim chuqurligiga nisbatan.
// parent < 0 => ildiz. Ota indeksi HAR DOIM boladan kichik (bitta o'tishda hisoblash uchun).
struct BoneDef { float px, py, pz; int parent; };

const BoneDef kBone[BCOUNT] = {
    /* Root      */ {  0.00f, 0.00f, 0.0f, -1        },
    /* Pelvis    */ {  0.00f, 0.50f, 0.0f, B_ROOT    },
    /* Torso     */ {  0.00f, 0.58f, 0.0f, B_PELVIS  },
    /* Head      */ {  0.00f, 0.86f, 0.0f, B_TORSO   },
    /* ArmUpperL */ { -0.70f, 0.82f, 0.0f, B_TORSO   },
    /* ArmLowerL */ { -0.78f, 0.70f, 0.0f, B_AUL     },
    /* ArmUpperR */ {  0.70f, 0.82f, 0.0f, B_TORSO   },
    /* ArmLowerR */ {  0.78f, 0.70f, 0.0f, B_AUR     },
    /* LegUpperL */ { -0.30f, 0.50f, 0.0f, B_PELVIS  },
    /* LegLowerL */ { -0.30f, 0.28f, 0.0f, B_LUL     },
    /* LegUpperR */ {  0.30f, 0.50f, 0.0f, B_PELVIS  },
    /* LegLowerR */ {  0.30f, 0.28f, 0.0f, B_LUR     },
};

// Chap/o'ng juftlikka ega suyaklar (o'rta chiziqda og'irlikni yumshatish uchun)
inline bool isPaired(int b) {
    return b == B_AUL || b == B_ALL_ || b == B_AUR || b == B_ALR ||
           b == B_LUL || b == B_LLL  || b == B_LUR || b == B_LLR;
}

// ---------------------------------------------------------------------------
//  Poza
// ---------------------------------------------------------------------------

// Ofsetlar model BALANDLIGIGA nisbatan (0.01 = balandlikning 1%), burchaklar gradusda.
struct BonePose { float ox, oy, oz, rx, ry, rz; };
struct Pose     { BonePose b[BCOUNT]; };

inline void clearPose(Pose& p) {
    for (int i = 0; i < BCOUNT; ++i) {
        p.b[i].ox = p.b[i].oy = p.b[i].oz = 0.0f;
        p.b[i].rx = p.b[i].ry = p.b[i].rz = 0.0f;
    }
}

inline void lerpPose(const Pose& a, const Pose& b, float t, Pose& o) {
    for (int i = 0; i < BCOUNT; ++i) {
        o.b[i].ox = lerpf(a.b[i].ox, b.b[i].ox, t);
        o.b[i].oy = lerpf(a.b[i].oy, b.b[i].oy, t);
        o.b[i].oz = lerpf(a.b[i].oz, b.b[i].oz, t);
        o.b[i].rx = lerpAngleDeg(a.b[i].rx, b.b[i].rx, t);
        o.b[i].ry = lerpAngleDeg(a.b[i].ry, b.b[i].ry, t);
        o.b[i].rz = lerpAngleDeg(a.b[i].rz, b.b[i].rz, t);
    }
}

// ---------------------------------------------------------------------------
//  Animatsiya pozalari
//  Burchak konventsiyasi (Math.h bilan bir xil, o'ng qo'l qoidasi):
//    +rx  -> bo'g'in ostidagi qism ORQAGA (-Z);   -rx -> OLDINGA (+Z)
//    +rz  -> chap a'zolar ichkariga, o'ng a'zolar tashqariga
// ---------------------------------------------------------------------------

void poseIdle(Pose& p, float t) {
    clearPose(p);
    const float br = std::sin(t * 1.6f);              // nafas olish
    p.b[B_PELVIS].oy = 0.004f * br;
    p.b[B_TORSO ].oy = 0.010f * br;                   // ko'krak ko'tarilishi
    p.b[B_TORSO ].rx = 1.2f * br;
    p.b[B_HEAD  ].ry = 6.0f * std::sin(t * 0.45f);    // bosh sekin buriladi
    p.b[B_HEAD  ].rx = 1.5f * std::sin(t * 0.90f) - 0.5f;

    p.b[B_AUL ].rx = 2.5f * std::sin(t * 1.50f);      // qo'llar ozgina tebranadi
    p.b[B_AUL ].rz = -3.0f + 1.5f * std::sin(t * 1.10f);
    p.b[B_AUR ].rx = 2.5f * std::sin(t * 1.50f + 0.6f);
    p.b[B_AUR ].rz =  3.0f - 1.5f * std::sin(t * 1.10f + 0.4f);
    p.b[B_ALL_].rx = -6.0f + 2.0f * std::sin(t * 1.30f);
    p.b[B_ALR ].rx = -6.0f + 2.0f * std::sin(t * 1.30f + 0.8f);
}

// ---------------------------------------------------------------------------
// QADAM GEOMETRIYASI
//
// Eski poza sinusoidal edi: oyoq foot_z = A*sin(ph). Tayanch oyog'ining DUNYO
// tezligi shunda v + A*2pi*f*cos(ph) bo'ladi. Uni nolga tenglash A = S/(2pi)
// ni talab qiladi, geometriya esa A = S/2 ni - ular NOMUVOFIQ. Ya'ni sinus
// bilan oyoq sirg'alishini yo'qotib bo'lmaydi.
//
// Yechim: tayanch fazasida panja DUNYODA QOTADI (ildizga nisbatan CHIZIQLI
// orqaga siljiydi), oyoq burchaklari esa analitik IK bilan hisoblanadi.
//
// Barcha uzunliklar MODEL BALANDLIGIGA nisbatan (kBone[] jadvalidan:
// son py 0.50 -> 0.28, boldir 0.28 -> 0.00).
// ---------------------------------------------------------------------------
constexpr float kThigh      = 0.22f;
constexpr float kShin       = 0.28f;
constexpr float kLegLen     = 0.50f;
constexpr float kSlack      = 0.006f;   // tizza to'liq yozilib qotmasin
constexpr float kContactMax = 0.643f;   // 2*kLegLen*sin(40 grad) - son burchagi chegarasi
constexpr float kRetract    = 0.85f;    // panjani tegishdan oldin qaytarish
constexpr float kRocker     = 0.030f;   // tovon/uch "rocker"
constexpr float kBobScale   = 1.0f;     // chanoq tebranishi kuchli bo'lsa 0.70 ga tushiring

// To'liq sikl (ikki qadam) uzunligi METRDA.
// Kalibrlash: 1.35 m/s -> 1.87 qadam/s, 3.30 -> 3.11, 6.40 -> 3.99 (real odam).
inline float strideLenM(float v, float bodyM) {
    if (!(v > 0.0f)) v = 0.0f;
    return clampf(0.97f + 0.35f * v, 0.75f, 3.40f) * ((bodyM > 0.3f) ? bodyM : 1.82f) / 1.82f;
}

// Tayanch ulushi. >0.5 = ikki tayanch (yurish), <0.5 = uchish fazasi (yugurish).
// 2.18 m/s da aynan 0.50 - ya'ni yurish/yugurish chegarasi FIZIKADAN kelib chiqadi,
// klip almashishidan emas.
inline float strideDuty(float v, float strideN) {
    const float base = clampf(0.62f - 0.055f * v, 0.30f, 0.62f);
    const float geo  = (strideN > 1e-4f) ? (kContactMax / strideN) : 0.62f;
    return clampf((base < geo) ? base : geo, 0.24f, 0.62f);
}

// Tayanch oyog'i "prujina": yugurishda tizza sikl o'rtasida zarbani yutadi
inline float legStanceLen(float u, float strideN) {
    const float comp = clampf(0.075f * (strideN - 0.50f), 0.004f, 0.105f);
    return kLegLen * (1.0f - kSlack - comp * std::sin(PI * u));
}

// Ikki bo'g'inli ANALITIK IK (kosinuslar teoremasi).
//   tz - panja nishoni SON pivotidan oldinga (+Z), ty - tepaga (manfiy = past)
//   Chiqish mavjud konventsiyada: -rx = oldinga, +rx = orqaga; tizza faqat orqaga.
inline void legIK(float tz, float ty, float& hipRx, float& kneeRx) {
    float d = std::sqrt(tz * tz + ty * ty);
    if (!(d > 0.0f)) d = 0.10f;
    d = clampf(d, kShin - kThigh + 0.02f, kLegLen * 0.999f);
    const float alpha = std::atan2(tz, -ty);
    const float cb = clampf((kThigh*kThigh + d*d - kShin*kShin) / (2.0f*kThigh*d), -1.0f, 1.0f);
    const float cg = clampf((kThigh*kThigh + kShin*kShin - d*d) / (2.0f*kThigh*kShin), -1.0f, 1.0f);
    hipRx  = -rad2deg(alpha + std::acos(cb));
    kneeRx =  180.0f - rad2deg(std::acos(cg));
    if (kneeRx <   0.0f) kneeRx =   0.0f;
    if (kneeRx > 150.0f) kneeRx = 150.0f;
    if (!std::isfinite(hipRx))  hipRx  = 0.0f;
    if (!std::isfinite(kneeRx)) kneeRx = 0.0f;
}

// Bitta panjaning sikl ichidagi yo'li (chanoqqa nisbatan, balandlik birligida)
struct FootTrack { float fz, fy, u; bool stance; };

FootTrack footTrack(float ph01, float strideN, float duty) {
    FootTrack r;
    ph01 -= std::floor(ph01);
    const float C = strideN * duty;
    if (ph01 < duty) {
        // TAYANCH: panja dunyoda qotgan => ildizga nisbatan CHIZIQLI orqaga.
        // Aynan CHIZIQLILIK sirg'alishni nolga tushiradi.
        r.u  = (duty > 1e-4f) ? (ph01 / duty) : 0.0f;
        r.fz = C * (0.5f - r.u);
        r.fy = kRocker * (2.0f*r.u - 1.0f) * (2.0f*r.u - 1.0f);
        r.stance = true;
        return r;
    }
    // SILJISH (swing): Hermite, ikkala uchda hosila -strideN*w*kRetract =>
    // panja yerga dunyo tezligi ~0 bo'lgan holda tushadi ("foot retraction").
    const float w  = 1.0f - duty;
    const float u  = (w > 1e-4f) ? ((ph01 - duty) / w) : 0.0f;
    const float u2 = u*u, u3 = u2*u;
    const float h00 =  2.0f*u3 - 3.0f*u2 + 1.0f;
    const float h10 =       u3 - 2.0f*u2 + u;
    const float h01 = -2.0f*u3 + 3.0f*u2;
    const float h11 =       u3 -      u2;
    const float m   = -strideN * w * kRetract;
    r.u  = u;
    r.fz = h00 * (-0.5f*C) + h01 * (0.5f*C) + (h10 + h11) * m;
    r.fy = kRocker + (0.060f*strideN + 0.012f) * std::sin(PI * u);
    r.stance = false;
    return r;
}

// Yurish/yugurish uchun umumiy poza - qadam fazasi MASOFAdan keladi (StrideCtx).
void poseStride(Pose& p, const StrideCtx& sc) {
    clearPose(p);
    float ph = sc.phase - std::floor(sc.phase);
    if (!(ph >= 0.0f && ph < 1.0f)) ph = 0.0f;
    const float amp = sc.amp;
    // ph = 0 da CHAP tovon tegadi => eski sin() o'rnini cos() bosadi
    const float s = std::cos(TAU * ph);

    // --- 1) Panja nishonlari ---
    FootTrack ft[2];
    ft[0] = footTrack(ph,        sc.strideN, sc.duty);   // chap
    ft[1] = footTrack(ph + 0.5f, sc.strideN, sc.duty);   // o'ng
    // NISHAB: panja ostidagi yer chanoqdan fz masofada slope*fz ga baland/past.
    // Ilgari tepalikda oldingi panja yerga botib, orqadagisi havoda qolardi.
    for (int k = 0; k < 2; ++k) ft[k].fy += ft[k].fz * sc.slope;

    // --- 2) Chanoq balandligi TAYANCH oyoqdan hisoblanadi ---
    // Eski kod teskari edi: oy = +0.015*(1-cos(2ph)) - oyoqlar ochilganda chanoq
    // eng BALAND bo'lardi. To'g'risi: ochilganda PASTGA tushadi.
    float hipY = 1.0e9f;
    for (int k = 0; k < 2; ++k) {
        if (!ft[k].stance) continue;
        const float lg = legStanceLen(ft[k].u, sc.strideN);
        const float r2 = lg*lg - ft[k].fz*ft[k].fz;
        const float h  = ft[k].fy + ((r2 > 1e-6f) ? std::sqrt(r2) : 0.0f);
        if (h < hipY) hipY = h;
    }
    if (hipY > 1.0e8f) {                       // uchish fazasi (yugurish)
        const float he = 0.5f * sc.strideN * sc.duty;
        const float lg = kLegLen * (1.0f - kSlack);
        const float r2 = lg*lg - he*he;
        hipY = 0.5f*(ft[0].fy + ft[1].fy) + ((r2 > 1e-6f) ? std::sqrt(r2) : 0.0f) + 0.010f;
    }
    if (!std::isfinite(hipY)) hipY = kLegLen;
    p.b[B_PELVIS].oy = (hipY - kLegLen) * kBobScale;

    // --- 3) Oyoqlar: analitik IK - panja aynan nishonga tushadi ---
    const int hipB[2] = { B_LUL, B_LUR };
    const int kneB[2] = { B_LLL, B_LLR };
    for (int k = 0; k < 2; ++k) {
        float hipRx = 0.0f, kneeRx = 0.0f;
        legIK(ft[k].fz, ft[k].fy - hipY, hipRx, kneeRx);
        p.b[hipB[k]].rx = hipRx;
        p.b[hipB[k]].ry = 1.35f * sc.moveAngle;      // orientation warping (strafe)
        p.b[hipB[k]].rz = (k == 0) ? -2.0f : 2.0f;
        p.b[kneB[k]].rx = kneeRx;
    }

    // TASHXIS (ERT_STRIDE_DBG=1): qadam geometriyasini sonli tekshirish
    {
        static const bool dbg = (std::getenv("ERT_STRIDE_DBG") != nullptr);
        if (dbg) {
            static int c = 0;
            if ((c++ % 40) == 0)
                std::printf("[stride] ph=%.3f strideN=%.2f duty=%.2f hipY=%.3f "
                            "L(fz=%+.3f st=%d rx=%+.1f kn=%+.1f) R(fz=%+.3f st=%d rx=%+.1f kn=%+.1f)\n",
                            ph, sc.strideN, sc.duty, hipY,
                            ft[0].fz, ft[0].stance ? 1 : 0, p.b[B_LUL].rx, p.b[B_LLL].rx,
                            ft[1].fz, ft[1].stance ? 1 : 0, p.b[B_LUR].rx, p.b[B_LLR].rx);
        }
    }

    // --- 4) Tana ---
    p.b[B_PELVIS].ox =  0.008f * amp * s;
    p.b[B_PELVIS].rz =  2.0f * amp * s + 0.35f * sc.bank;
    p.b[B_PELVIS].ry =  3.0f * amp * s - 0.35f * sc.moveAngle;
    p.b[B_TORSO ].rx =  sc.lean;
    p.b[B_TORSO ].ry = -4.0f * amp * s - 0.45f * sc.moveAngle;
    p.b[B_TORSO ].rz =  0.65f * sc.bank;
    p.b[B_TORSO ].oy =  0.004f * amp * (1.0f - std::cos(2.0f*TAU*ph + 1.0f));
    p.b[B_HEAD  ].rx = -0.6f * sc.lean;
    p.b[B_HEAD  ].ry =  2.0f * s - 0.20f * sc.moveAngle + sc.headLead;
    p.b[B_HEAD  ].rz = -0.55f * sc.bank;             // bosh gorizontni ushlaydi

    // --- 5) Qo'llar: oyoqlarga teskari fazada ---
    const float armA = lerpf(16.0f, 30.0f, saturate((sc.strideN - 0.79f) / 0.98f)) * amp;
    p.b[B_AUL].rx =  armA * s;   p.b[B_AUL].rz = -4.0f;
    p.b[B_AUR].rx = -armA * s;   p.b[B_AUR].rz =  4.0f;
    p.b[B_ALL_].rx = -(12.0f + 12.0f * amp * std::max(0.0f, -s));
    p.b[B_ALR ].rx = -(12.0f + 12.0f * amp * std::max(0.0f,  s));
}

void poseTalk(Pose& p, float t, float talk) {
    poseIdle(p, t);
    if (talk <= 0.0f) return;                         // talk_ == 0 => aynan Idle
    const float k = saturate(talk);

    // o'ng qo'l davriy ko'tariladi (asosiy ishora)
    const float g1 = 0.5f + 0.5f * std::sin(t * 2.2f);
    p.b[B_AUR ].rx += -30.0f * k * g1;
    p.b[B_AUR ].rz +=  -8.0f * k * std::sin(t * 1.7f);
    p.b[B_ALR ].rx += -35.0f * k * (0.4f + 0.6f * (0.5f + 0.5f * std::sin(t * 2.2f + 0.7f)));

    // chap qo'l kuchsizroq hamrohlik qiladi
    const float g2 = 0.5f + 0.5f * std::sin(t * 1.9f + 1.4f);
    p.b[B_AUL ].rx += -12.0f * k * g2;
    p.b[B_ALL_].rx += -18.0f * k * g2;

    // bosh va tana jonlanadi
    p.b[B_HEAD ].ry += 5.0f * k * std::sin(t * 2.6f);
    p.b[B_HEAD ].rx += 3.0f * k * std::sin(t * 3.1f);
    p.b[B_TORSO].ry += 2.5f * k * std::sin(t * 1.4f);
}

void poseListen(Pose& p, float t) {
    poseIdle(p, t * 0.6f);
    p.b[B_HEAD ].rx = 8.0f;                           // bosh 8 gradus egilgan
    p.b[B_HEAD ].rz = 4.0f;
    p.b[B_HEAD ].ry = 3.0f * std::sin(t * 0.5f);
    p.b[B_TORSO].ry = 8.0f;                           // tana ozgina buriladi
    // qo'llar oldida
    p.b[B_AUL ].rx = -16.0f;  p.b[B_AUL ].rz =  -6.0f;
    p.b[B_AUR ].rx = -16.0f;  p.b[B_AUR ].rz =   6.0f;
    p.b[B_ALL_].rx = -58.0f;  p.b[B_ALL_].rz =  16.0f;
    p.b[B_ALR ].rx = -58.0f;  p.b[B_ALR ].rz = -16.0f;
}

void posePoint(Pose& p, float t) {
    poseIdle(p, t * 0.7f);
    // o'ng qo'l oldinga cho'ziladi va shu holatda ushlab turiladi
    p.b[B_AUR  ].rx = -78.0f + 1.5f * std::sin(t * 1.4f);
    p.b[B_AUR  ].rz =   6.0f;
    p.b[B_ALR  ].rx =  -6.0f;
    p.b[B_ALR  ].rz =   0.0f;
    p.b[B_TORSO].ry =  -6.0f;
    p.b[B_HEAD ].ry =  -8.0f;
    p.b[B_HEAD ].rx =   2.0f;
}

void poseSalute(Pose& p, float t) {
    // Turkiy salomlashish: o'ng qo'l ko'krakka qo'yiladi va ushlab turiladi
    poseIdle(p, t * 0.6f);
    p.b[B_AUR  ].rx = -28.0f;
    p.b[B_AUR  ].rz = -22.0f;                         // ichkariga (ko'krak tomon)
    p.b[B_ALR  ].rx = -98.0f;
    p.b[B_ALR  ].rz = -26.0f;
    p.b[B_HEAD ].rx =   7.0f;                         // yengil ta'zim
    p.b[B_HEAD ].ry =   0.0f;
    p.b[B_TORSO].rx =   3.0f;
}

// Qilich tortish â€” bir martalik harakat, 0.8 s. lt = klip boshlanganidan beri o'tgan vaqt.
void poseDraw(Pose& p, float t, float lt) {
    poseIdle(p, t * 0.5f);
    const float u = clampf(lt / 0.8f, 0.0f, 1.0f);
    float aurRx, aurRz, alrRx, alrRz, torsoRy;
    if (u < 0.55f) {
        // 1-bosqich: qo'l belga tushadi
        const float k = easeInOut(u / 0.55f);
        aurRx   = lerpf(  0.0f,  -18.0f, k);
        aurRz   = lerpf(  0.0f,  -30.0f, k);
        alrRx   = lerpf(  0.0f,  -85.0f, k);
        alrRz   = lerpf(  0.0f,  -20.0f, k);
        torsoRy = lerpf(  0.0f,  -10.0f, k);
    } else {
        // 2-bosqich: tez yuqoriga (qilich tortiladi)
        const float k = easeOutCubic((u - 0.55f) / 0.45f);
        aurRx   = lerpf(-18.0f, -112.0f, k);
        aurRz   = lerpf(-30.0f,   14.0f, k);
        alrRx   = lerpf(-85.0f,  -26.0f, k);
        alrRz   = lerpf(-20.0f,    4.0f, k);
        torsoRy = lerpf(-10.0f,    6.0f, k);
    }
    p.b[B_AUR  ].rx = aurRx;
    p.b[B_AUR  ].rz = aurRz;
    p.b[B_ALR  ].rx = alrRx;
    p.b[B_ALR  ].rz = alrRz;
    p.b[B_TORSO].ry = torsoRy;
    p.b[B_AUL  ].rx =  -8.0f;
    p.b[B_ALL_ ].rx = -22.0f;
}

void poseSit(Pose& p, float t) {
    poseIdle(p, t * 0.7f);
    // Tana pastroq. Chanoqni to'liq -0.35 ga tushirish bbox-rig'da oyoqlarni yer ostiga
    // olib kirar edi, shuning uchun tushish chanoq (-0.22) va ko'krak (-0.06) o'rtasida
    // taqsimlangan â€” vizual natija bir xil, oyoq panjalari esa yer ustida qoladi.
    p.b[B_PELVIS].oy  = -0.22f;
    p.b[B_TORSO ].oy += -0.06f;
    p.b[B_TORSO ].rx +=  6.0f;
    // tizzalar bukilgan (son oldinga, boldir pastga)
    p.b[B_LUL].rx = -95.0f;  p.b[B_LUL].rz = -8.0f;
    p.b[B_LUR].rx = -95.0f;  p.b[B_LUR].rz =  8.0f;
    p.b[B_LLL].rx = 100.0f;
    p.b[B_LLR].rx = 100.0f;
    // qo'llar tizzada
    p.b[B_AUL ].rx = -14.0f;  p.b[B_AUL ].rz = -6.0f;
    p.b[B_AUR ].rx = -14.0f;  p.b[B_AUR ].rz =  6.0f;
    p.b[B_ALL_].rx = -36.0f;
    p.b[B_ALR ].rx = -36.0f;
}

// ---------------------------------------------------------------------------
//  Parkur va jang kliplari (AC uslubi)
//
//  DIQQAT - burchak belgisi haqida. Yuqoridagi qoida ("+rx -> ORQAGA") bo'g'in
//  OSTIDAGI qismlar (qo'l, oyoq) uchun. Torso/Head suyaklarining massasi esa
//  pivotdan YUQORIDA, shuning uchun ular uchun belgi teskari ishlaydi:
//      torso +rx -> tana OLDINGA egiladi   (poseStride dagi "lean" bilan bir xil)
//      torso -rx -> tana ORQAGA yotadi
//  Shu sababli quyida "orqaga" so'zi manfiy rx bilan yoziladi.
// ---------------------------------------------------------------------------

// Bir martalik klipning [a,b] oralig'idagi fazasi -> 0..1 (nolga bo'lishdan himoyalangan)
inline float seg(float u, float a, float b) {
    if (!(b > a)) return (u >= b) ? 1.0f : 0.0f;
    return saturate((u - a) / (b - a));
}

// 0 -> 1 -> 0 yoyi (harakat cho'qqisi)
inline float arc01(float u) { return std::sin(PI * saturate(u)); }

// Cho'kkalash asosi. k = 0 (tik turish) .. 1 (to'liq cho'kkalash).
// Geometriya: chanoq pivoti 0.50h, tizza 0.28h, to'piq ~0.00h.
// Chanoqni 0.28h ga tushirish uchun son 55 gradus oldinga buriladi, tizza esa
// unga mos ravishda ORQAGA yopiladi. Agar tizza kamroq bukilsa, oyoq panjalari
// yer ostiga kirib ketadi (poseSit dagi bilan bir xil muammo).
// Natijada bo'y ~0.27h ga pasayadi: 1.75 m odam ~1.27 m ga tushadi.
void poseCrouchBase(Pose& p, float t, float k) {
    clearPose(p);
    k = saturate(k);
    const float br = std::sin(t * 1.1f);              // sekin nafas

    // k*k: yarim yo'lda chanoq oyoq bukilishidan tezroq tushmasligi uchun
    // (chiziqli bo'lsa o'tish paytida panjalar yer ostiga kirib ketardi).
    p.b[B_PELVIS].oy = -0.28f * k * k + 0.004f * br;
    p.b[B_TORSO ].rx =  12.0f * k + 1.0f * br;        // tana 12 gradus oldinga
    p.b[B_TORSO ].oy =  0.006f * br;
    p.b[B_HEAD  ].rx =  -8.0f * k + 1.0f * std::sin(t * 0.9f);   // bosh tik qoladi
    p.b[B_HEAD  ].ry =   5.0f * std::sin(t * 0.45f);

    // oyoqlar: son oldinga (-rx), tizza orqaga (+rx)
    p.b[B_LUL].rx = -55.0f * k;   p.b[B_LUL].rz = -9.0f * k;     // tizzalar chetga
    p.b[B_LUR].rx = -55.0f * k;   p.b[B_LUR].rz =  9.0f * k;
    p.b[B_LLL].rx = 126.0f * k;
    p.b[B_LLR].rx = 126.0f * k;

    // qo'llar tizzalar oldida osilib turadi
    p.b[B_AUL ].rx = -20.0f * k -  2.0f;   p.b[B_AUL ].rz = -8.0f * k - 3.0f;
    p.b[B_AUR ].rx = -20.0f * k -  2.0f;   p.b[B_AUR ].rz =  8.0f * k + 3.0f;
    p.b[B_ALL_].rx = -55.0f * k -  6.0f;   p.b[B_ALL_].rz =  10.0f * k;
    p.b[B_ALR ].rx = -55.0f * k -  6.0f;   p.b[B_ALR ].rz = -10.0f * k;
}

void poseCrouchIdle(Pose& p, float t) {
    poseCrouchBase(p, t, 1.0f);
}

// Cho'kkalab yurish: CrouchIdle + kichik qadam (+-14 gradus) va tana chayqalishi.
void poseCrouchWalk(Pose& p, float t, const StrideCtx& sc) {
    poseCrouchBase(p, t, 1.0f);
    // Cho'kkalab yurishda ham faza MASOFAdan keladi - sirg'alish yo'q
    const float s = std::sin(TAU * (sc.phase - std::floor(sc.phase)));
    const float ph = TAU * (sc.phase - std::floor(sc.phase));

    p.b[B_LUL].rx += -14.0f * s;
    p.b[B_LUR].rx +=  14.0f * s;
    p.b[B_LLL].rx +=  10.0f * std::max(0.0f, std::sin(ph + 1.3f));
    p.b[B_LLR].rx +=  10.0f * std::max(0.0f, std::sin(ph + 1.3f + PI));

    p.b[B_PELVIS].ox +=  0.006f * s;
    p.b[B_PELVIS].oy +=  0.004f * (1.0f - std::cos(2.0f * ph));
    p.b[B_PELVIS].rz +=  2.5f * s;
    p.b[B_PELVIS].ry +=  3.5f * s;
    p.b[B_TORSO ].ry += -3.0f * s;
    p.b[B_TORSO ].rz +=  1.5f * s;
    p.b[B_HEAD  ].ry +=  2.0f * s;
    p.b[B_AUL   ].rx +=  9.0f * s;
    p.b[B_AUR   ].rx += -9.0f * s;
}

// Vault - past to'siqdan qo'l bilan tayanib o'tish. Bir martalik, 0.55 s.
//   0.00..0.35  qo'llar oldinga tayanadi
//   0.25..0.75  oyoqlar yon tomondan o'tadi
//   0.75..1.00  qo'nish
void poseVault(Pose& p, float lt) {
    clearPose(p);
    const float u = clampf(lt / 0.55f, 0.0f, 1.0f);

    const float plant = easeOutCubic(seg(u, 0.00f, 0.35f));   // tayanish
    const float over  = easeInOut  (seg(u, 0.30f, 0.80f));    // tana qo'l ustidan o'tadi
    const float land  = easeInOut  (seg(u, 0.75f, 1.00f));    // qo'nish
    const float side  = arc01      (seg(u, 0.25f, 0.75f));    // oyoqlar yon tomonda
    const float tuck  = arc01      (seg(u, 0.20f, 0.85f));    // oyoqlar yig'iladi
    const float soft  = arc01      (seg(u, 0.75f, 1.00f));    // qo'nishda tizza yumshatadi

    // chanoq: 0 -> +0.22 -> 0
    p.b[B_PELVIS].oy = 0.22f * arc01(u) - 0.06f * soft;
    p.b[B_PELVIS].ry = 10.0f * side;

    // qo'llar: oldinga tayanadi, keyin tana ostida orqada qoladi
    const float armRx = lerpf(-80.0f * plant, 55.0f, over) * (1.0f - land);
    p.b[B_AUL ].rx = armRx;   p.b[B_AUL ].rz = -10.0f * plant;
    p.b[B_AUR ].rx = armRx;   p.b[B_AUR ].rz =  10.0f * plant;
    p.b[B_ALL_].rx = -18.0f * plant * (1.0f - land);
    p.b[B_ALR ].rx = -18.0f * plant * (1.0f - land);

    // oyoqlar bir yonga (+X) yig'ilib o'tadi
    p.b[B_LUL].rx = -55.0f * tuck - 20.0f * soft;   p.b[B_LUL].rz = 32.0f * side;
    p.b[B_LUR].rx = -40.0f * tuck - 20.0f * soft;   p.b[B_LUR].rz = 32.0f * side;
    p.b[B_LLL].rx =  85.0f * tuck + 25.0f * soft;
    p.b[B_LLR].rx =  70.0f * tuck + 25.0f * soft;

    // tana oldinga egiladi, oxirida tiklanadi
    p.b[B_TORSO].rx =  30.0f * arc01(u) + 12.0f * soft;
    p.b[B_TORSO].ry = -12.0f * side;
    p.b[B_HEAD ].rx = -12.0f * arc01(u);
}

// Mantle - chekkadan tepaga chiqish. Bir martalik, 0.85 s.
//   0.00..0.30  qo'llar tepaga cho'ziladi
//   0.20..0.65  tana tortiladi (chanoq y 0 -> +0.5)
//   0.50..0.85  tizza chekkaga qo'yiladi
//   0.80..1.00  tik turish
// Chanoq oxirida +0.5 da qoladi: o'yin obyektning dunyodagi holatini ham
// ko'taradi deb hisoblanadi (ildiz harakati klipga singdirilgan).
void poseMantle(Pose& p, float lt) {
    clearPose(p);
    const float u = clampf(lt / 0.85f, 0.0f, 1.0f);

    const float reach = easeOutCubic(seg(u, 0.00f, 0.30f));
    const float pull  = easeInOut  (seg(u, 0.20f, 0.65f));
    const float knee  = arc01      (seg(u, 0.50f, 0.85f));
    const float rise  = easeInOut  (seg(u, 0.80f, 1.00f));
    const float act   = 1.0f - rise;

    p.b[B_PELVIS].oy = 0.50f * pull;

    // qo'llar tepaga (yelka -110, tirsak -60), tortishda tirsak yoziladi
    p.b[B_AUL ].rx = (-110.0f * reach + 45.0f * pull) * act;
    p.b[B_AUR ].rx = (-110.0f * reach + 45.0f * pull) * act;
    p.b[B_AUL ].rz = -12.0f * reach * act;
    p.b[B_AUR ].rz =  12.0f * reach * act;
    p.b[B_ALL_].rx = -60.0f * reach * (1.0f - 0.7f * pull) * act;
    p.b[B_ALR ].rx = -60.0f * reach * (1.0f - 0.7f * pull) * act;

    // tana devorga yopishgan, oxirida tik
    p.b[B_TORSO].rx = (18.0f * pull + 4.0f * reach) * act;
    p.b[B_HEAD ].rx = -10.0f * reach * act;

    // chap tizza chekkaga qo'yiladi, o'ng oyoq osilib qoladi
    p.b[B_LUL].rx = -(30.0f * pull + 65.0f * knee) * act;
    p.b[B_LLL].rx =  (40.0f * pull + 70.0f * knee) * act;
    p.b[B_LUL].rz = -14.0f * knee * act;
    p.b[B_LUR].rx = -10.0f * pull * act;
    p.b[B_LLR].rx =  25.0f * pull * act;
}

// ClimbUp - devorga yopishib ko'tarilish. Siklik, 1.2 s.
// Qarama-qarshi qo'l/oyoq navbat bilan yuqoriga cho'ziladi.
void poseClimbUp(Pose& p, float t) {
    clearPose(p);
    const float ph = t * TAU / 1.2f;
    const float sL = std::sin(ph);
    const float sR = -sL;                              // qarama-qarshi faza

    p.b[B_TORSO ].rx =   8.0f;                         // tana devorga yopishgan
    p.b[B_TORSO ].ry =   5.0f * sL;
    p.b[B_HEAD  ].rx = -10.0f;                         // yuqoriga qaraydi
    p.b[B_PELVIS].oy = 0.018f * std::sin(2.0f * ph);   // chanoq ozgina tebranadi
    p.b[B_PELVIS].ry =   4.0f * sL;

    // qo'llar navbat bilan yuqoriga: -70 .. -110
    p.b[B_AUL ].rx = -90.0f - 20.0f * sL;   p.b[B_AUL ].rz =  8.0f;
    p.b[B_AUR ].rx = -90.0f - 20.0f * sR;   p.b[B_AUR ].rz = -8.0f;
    p.b[B_ALL_].rx = -28.0f + 12.0f * sL;
    p.b[B_ALR ].rx = -28.0f + 12.0f * sR;

    // oyoqlar qarama-qarshi tomonda ko'tariladi
    p.b[B_LUL].rx = -16.0f - 26.0f * std::max(0.0f, sR);   p.b[B_LUL].rz = -7.0f;
    p.b[B_LUR].rx = -16.0f - 26.0f * std::max(0.0f, sL);   p.b[B_LUR].rz =  7.0f;
    p.b[B_LLL].rx =  22.0f + 45.0f * std::max(0.0f, sR);
    p.b[B_LLR].rx =  22.0f + 45.0f * std::max(0.0f, sL);
}

// Hang - chekkada osilib turish. Yelka -95, tirsak yana -70 => panjalar bosh ustida.
// Butun tana oyoq ostidagi Root pivoti atrofida 0.6 Hz da +-2 gradus tebranadi.
void poseHang(Pose& p, float t) {
    clearPose(p);
    const float sw = std::sin(t * TAU * 0.6f);

    p.b[B_ROOT  ].rz =  2.0f * sw;
    p.b[B_PELVIS].rz =  1.0f * sw;
    p.b[B_TORSO ].rx =  3.0f + 1.0f * std::sin(t * 1.1f);
    p.b[B_HEAD  ].rx = -6.0f + 1.5f * std::sin(t * 0.8f);

    p.b[B_AUL ].rx = -95.0f;   p.b[B_AUL ].rz =  6.0f;
    p.b[B_AUR ].rx = -95.0f;   p.b[B_AUR ].rz = -6.0f;
    p.b[B_ALL_].rx = -70.0f + 2.0f * sw;
    p.b[B_ALR ].rx = -70.0f - 2.0f * sw;

    // oyoqlar osilgan: ozgina bukilgan va rz bilan chetga
    p.b[B_LUL].rx = -8.0f;   p.b[B_LUL].rz = -7.0f + 1.5f * sw;
    p.b[B_LUR].rx =  6.0f;   p.b[B_LUR].rz =  7.0f + 1.5f * sw;
    p.b[B_LLL].rx = 18.0f;
    p.b[B_LLR].rx = 26.0f;
}

// Shimmy - osilgan holda yon siljish: qo'llar navbat bilan 1.4 Hz da yon tomonga.
void poseShimmy(Pose& p, float t) {
    poseHang(p, t);
    const float ph = t * TAU * 1.4f;
    const float s  = std::sin(ph);
    const float sp = std::max(0.0f,  s);
    const float sn = std::max(0.0f, -s);

    p.b[B_AUL ].rz += 16.0f * s;      // ikkala qo'l ham bir yonga siljiydi
    p.b[B_AUR ].rz += 16.0f * s;
    p.b[B_AUL ].rx += -8.0f * sp;     // navbat bilan qo'l qo'yiladi
    p.b[B_AUR ].rx += -8.0f * sn;

    p.b[B_PELVIS].ox += 0.020f * s;   // chanoq chayqaladi
    p.b[B_PELVIS].rz += 4.0f * s;
    p.b[B_TORSO ].ry += 5.0f * s;
    p.b[B_LUL].rz += 8.0f * s;
    p.b[B_LUR].rz += 8.0f * s;
}

// Slide - yugurishdan cho'kkalab sirg'alish. Bir martalik, 0.7 s.
// Chanoq -0.45h ga tushadi, shuning uchun oldingi oyoq deyarli gorizontal
// cho'ziladi (aks holda panjalar yer ostiga kirib ketadi).
void poseSlide(Pose& p, float lt) {
    clearPose(p);
    const float u   = clampf(lt / 0.7f, 0.0f, 1.0f);
    const float in  = easeOutCubic(seg(u, 0.00f, 0.20f));
    const float out = easeInOut   (seg(u, 0.75f, 1.00f));   // oxirida turishga qaytadi
    const float k   = in * (1.0f - out);

    p.b[B_PELVIS].oy = -0.45f * k * k;   // k*k: chanoq oyoq cho'zilishidan o'zib ketmasin
    p.b[B_TORSO ].rx = -35.0f * k;    // tana ORQAGA yotadi (torso uchun -rx = orqaga)
    p.b[B_TORSO ].ry =   8.0f * k;
    p.b[B_HEAD  ].rx =  20.0f * k;    // bosh oldinga qaraydi

    // oldingi (chap) oyoq cho'zilgan, orqadagi (o'ng) tanaga yig'ilgan
    p.b[B_LUL].rx = -85.0f * k;   p.b[B_LUL].rz =  -6.0f * k;
    p.b[B_LLL].rx =   8.0f * k;
    p.b[B_LUR].rx = -30.0f * k;   p.b[B_LUR].rz =  12.0f * k;
    p.b[B_LLR].rx = 140.0f * k;

    // chap qo'l orqada yerga tayanadi, o'ng qo'l oldinda muvozanat saqlaydi
    p.b[B_AUL ].rx =  45.0f * k;   p.b[B_AUL ].rz = -18.0f * k;
    p.b[B_ALL_].rx = -20.0f * k;
    p.b[B_AUR ].rx = -55.0f * k;   p.b[B_AUR ].rz =  10.0f * k;
    p.b[B_ALR ].rx = -35.0f * k;
}

// Roll - qo'ngandan keyin oldinga dumalash. Bir martalik, 0.6 s.
// Aylanish FAQAT chanoqqa beriladi: tana (va boshqa hamma suyaklar) uning
// bolasi bo'lgani uchun u bilan birga aylanadi. Ikkalasiga ham 360 berilsa,
// tana ikki marta aylanib ketardi.
// Belgisi: chanoq pivoti tananing OSTIDA, shuning uchun oldinga dumalash +360.
void poseRoll(Pose& p, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 0.6f, 0.0f, 1.0f);
    const float curl = arc01(u);

    p.b[B_PELVIS].rx = 360.0f * easeInOut(u);
    // Chanoq sinus bilan -0.35 gacha tushib qaytadi. Eng chuqur nuqta ataylab
    // dumalashning BOSHIGA suriladi: aks holda u tana teskari bo'lgan payt (u=0.5)
    // bilan mos tushib, bosh yerga chuqurroq kirib ketardi.
    p.b[B_PELVIS].oy = -0.35f * arc01(seg(u, 0.0f, 0.65f));

    p.b[B_TORSO].rx =  30.0f * curl;    // tana yumaloqlanadi
    p.b[B_HEAD ].rx =  25.0f * curl;    // iyak ko'krakka

    p.b[B_LUL].rx = -85.0f * curl;   p.b[B_LUL].rz = -6.0f * curl;
    p.b[B_LUR].rx = -85.0f * curl;   p.b[B_LUR].rz =  6.0f * curl;
    p.b[B_LLL].rx = 110.0f * curl;
    p.b[B_LLR].rx = 110.0f * curl;

    p.b[B_AUL ].rx = -60.0f * curl;   p.b[B_AUL ].rz =  14.0f * curl;
    p.b[B_AUR ].rx = -60.0f * curl;   p.b[B_AUR ].rz = -14.0f * curl;
    p.b[B_ALL_].rx = -95.0f * curl;
    p.b[B_ALR ].rx = -95.0f * curl;
}

// Fall - havoda uchish: qo'llar yon-tepaga, oyoqlar ozgina ochilgan, tana orqaga.
void poseFall(Pose& p, float t) {
    clearPose(p);
    const float f1 = std::sin(t * 1.3f);
    const float f2 = std::sin(t * 0.9f + 1.1f);

    p.b[B_PELVIS].oy = 0.006f * f2;
    p.b[B_TORSO ].rx =  -6.0f + 1.5f * f1;     // tana 6 gradus orqaga
    p.b[B_TORSO ].rz =   2.0f * f2;
    p.b[B_HEAD  ].rx =  -4.0f + 2.0f * f1;
    p.b[B_HEAD  ].ry =   4.0f * f2;

    p.b[B_AUL ].rx = -25.0f + 4.0f * f1;   p.b[B_AUL ].rz = -35.0f + 3.0f * f2;
    p.b[B_AUR ].rx = -25.0f - 4.0f * f1;   p.b[B_AUR ].rz =  35.0f - 3.0f * f2;
    p.b[B_ALL_].rx = -45.0f + 5.0f * f2;   p.b[B_ALL_].rz =  10.0f;
    p.b[B_ALR ].rx = -45.0f - 5.0f * f2;   p.b[B_ALR ].rz = -10.0f;

    p.b[B_LUL].rx = -14.0f + 3.0f * f2;   p.b[B_LUL].rz = -10.0f;
    p.b[B_LUR].rx =  10.0f - 3.0f * f2;   p.b[B_LUR].rz =  10.0f;
    p.b[B_LLL].rx =  22.0f;
    p.b[B_LLR].rx =  34.0f;
}

// WallRun - devor bo'ylab yugurish. Devor chapda deb olinadi:
// tana 25 gradus chapga qiyshayadi, chap qo'l devorga cho'ziladi.
void poseWallRun(Pose& p, float t, float rate) {
    clearPose(p);
    const float ph = t * TAU * ((rate > 0.05f) ? rate : 0.05f);
    const float s  = std::sin(ph);

    p.b[B_TORSO ].rz =  25.0f;              // yon tomonga qiyshaygan
    p.b[B_TORSO ].rx =   6.0f;
    p.b[B_TORSO ].ry =  -5.0f * s;
    p.b[B_PELVIS].rz =  10.0f;
    p.b[B_PELVIS].oy = 0.012f * (1.0f - std::cos(2.0f * ph));
    p.b[B_HEAD  ].rz = -12.0f;              // bosh tikroq qoladi
    p.b[B_HEAD  ].ry = -18.0f;

    // oyoqlar tez qadam, ikkalasi ham devor tomonga (-X) qiya
    p.b[B_LUL].rx = -30.0f * s;   p.b[B_LUL].rz = -12.0f;
    p.b[B_LUR].rx =  30.0f * s;   p.b[B_LUR].rz = -12.0f;
    p.b[B_LLL].rx = 10.0f + 40.0f * std::max(0.0f, std::sin(ph + 1.3f));
    p.b[B_LLR].rx = 10.0f + 40.0f * std::max(0.0f, std::sin(ph + 1.3f + PI));

    // chap qo'l devorga cho'zilgan, o'ng qo'l qadamga hamroh
    p.b[B_AUL ].rx = -35.0f;      p.b[B_AUL ].rz = -55.0f;
    p.b[B_ALL_].rx = -10.0f;
    p.b[B_AUR ].rx = -20.0f * s;  p.b[B_AUR ].rz =  14.0f;
    p.b[B_ALR ].rx = -30.0f - 15.0f * std::max(0.0f, s);
}

// Dodge - tez yon siljish (chapga). Bir martalik, 0.45 s.
void poseDodge(Pose& p, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 0.45f, 0.0f, 1.0f);
    const float k    = arc01(u);                        // 0 -> 1 -> 0
    const float open = arc01(seg(u, 0.0f, 0.75f));      // oyoqlar ochilib yopiladi

    p.b[B_PELVIS].ox = -0.12f * k;                      // butun tana chapga siljiydi
    p.b[B_PELVIS].oy = -0.025f * k;                     // ozgina cho'kish (panjalar yer ustida qolsin)
    p.b[B_PELVIS].rz =   6.0f * k;
    p.b[B_TORSO ].rz =  20.0f * k;                      // tana yo'nalish tomon egiladi
    p.b[B_TORSO ].rx =   8.0f * k;
    p.b[B_HEAD  ].rz = -10.0f * k;
    p.b[B_HEAD  ].ry = -12.0f * k;

    p.b[B_LUL].rx = -16.0f * open;   p.b[B_LUL].rz = -26.0f * open;
    p.b[B_LUR].rx =  10.0f * open;   p.b[B_LUR].rz =  18.0f * open;
    p.b[B_LLL].rx =  34.0f * open;
    p.b[B_LLR].rx =  20.0f * open;

    // qo'llar muvozanat uchun ochiladi
    p.b[B_AUL ].rx = -18.0f * k;   p.b[B_AUL ].rz = -45.0f * k;
    p.b[B_AUR ].rx =  12.0f * k;   p.b[B_AUR ].rz =  28.0f * k;
    p.b[B_ALL_].rx = -30.0f * k;
    p.b[B_ALR ].rx = -20.0f * k;
}

// Assassinate - yashirin o'ldirish. Bir martalik, 1.1 s.
//   0.00..0.35  cho'kkalab yaqinlashish
//   0.35..0.55  o'ng qo'l tez pastga zarba
//   0.55..0.85  ushlab turish
//   0.85..1.00  tik turish
void poseAssassinate(Pose& p, float t, float lt) {
    const float u      = clampf(lt / 1.1f, 0.0f, 1.0f);
    const float rise   = easeInOut(seg(u, 0.85f, 1.00f));
    const float act    = 1.0f - rise;
    const float crouch = smoothstepf(seg(u, 0.00f, 0.35f)) * act;

    poseCrouchBase(p, t, 0.85f * crouch);

    const float raise  = smoothstepf(seg(u, 0.20f, 0.35f));   // xanjar ko'tariladi
    const float strike = easeOutCubic(seg(u, 0.35f, 0.55f));  // tez zarba
    const float hold   = seg(u, 0.55f, 0.85f);
    const float grab   = smoothstepf(seg(u, 0.30f, 0.50f)) * act;

    // o'ng qo'l: yuqoriga ko'tarilib, keyin pastga uriladi
    p.b[B_AUR ].rx = lerpf(-95.0f * raise,  -6.0f, strike) * act;
    p.b[B_AUR ].rz = (-8.0f * raise + 16.0f * strike) * act;
    p.b[B_ALR ].rx = lerpf(-80.0f * raise, -12.0f, strike) * act;
    p.b[B_ALR ].rz = -10.0f * act;

    // chap qo'l nishonni ushlab turadi
    p.b[B_AUL ].rx = -70.0f * grab;   p.b[B_AUL ].rz = -10.0f * grab;
    p.b[B_ALL_].rx = -40.0f * grab;   p.b[B_ALL_].rz =  12.0f * grab;

    // tana zarbada oldinga bosadi, ushlab turishda ozgina qaltiraydi
    p.b[B_TORSO].rx += (16.0f * strike + 1.2f * hold * std::sin(t * 9.0f)) * act;
    p.b[B_TORSO].ry += -10.0f * strike * act;
    p.b[B_HEAD ].rx +=   8.0f * strike * act;
}

// ---------------------------------------------------------------------------
//  JANG KLIPLARI
//
//  DIQQAT - burchak belgisi haqida. Texnik topshiriqdagi burchaklar UMUMIY
//  konventsiyada ("+rx -> ORQAGA, -rx -> OLDINGA") berilgan. Ammo Torso/Head
//  suyaklarining massasi pivotdan YUQORIDA joylashgani uchun bu faylda ular
//  uchun belgi TESKARI ishlaydi (yuqoridagi izohga qarang):
//      torso +rx -> tana OLDINGA egiladi,   torso -rx -> tana ORQAGA yotadi
//  Shu sababli quyida TANA burchaklarining KATTALIGI topshiriqdagidek, BELGISI
//  esa fizik jihatdan to'g'ri variantda yozilgan (poseStride/poseSlide bilan
//  bir xil). A'zolar (qo'l/oyoq) uchun belgi topshiriqdagidek qoladi.
//
//  Oyoq burchaklari geometriya bo'yicha tanlangan: son 0.22h, boldir 0.28h,
//  ya'ni chanoq y ga tushganda 0.22*cos(son) + 0.28*cos(boldir) ~ (0.50h + y)
//  bo'lishi kerak - aks holda panjalar yer ostiga kirib ketadi (poseSit dagi
//  bilan bir xil muammo).
//
//  Barcha zarbalar bir martalik (oneShotDuration jadvalida) va oxirida neytral
//  pozaga qaytadi - shunda keyingi klipga o'tish silliq bo'ladi. Death istisno:
//  u oxirgi kadrda qotib qoladi.
// ---------------------------------------------------------------------------

// Chanoq 'drop' (model balandligiga nisbatan) ga tushganda oyoq panjalari yer
// ustida qolishi uchun kerakli son (hip, manfiy = oldinga) va tizza (knee,
// musbat = orqaga) burchaklari.
// Geometriya: son 0.22h, boldir 0.28h. Vertikal yetish
//   0.22*cos(A) + 0.28*cos(B) = 0.50h - drop,   B = knee - A.
// Kichik burchaklarda cos(x) ~ 1 - x^2/2, ya'ni burchak drop ning KVADRAT
// ILDIZIGA proportsional - shuning uchun chiziqli koeffitsiyent yaramaydi.
// Tekshirish: drop=0.06 -> (-24, +56); drop=0.28 -> (-51, +120), bu
// poseCrouchBase dagi (-55, +126) bilan deyarli mos tushadi.
inline void legBendFor(float drop, float& hipDeg, float& kneeDeg) {
    const float d = clampf(drop, 0.0f, 0.30f);
    const float s = std::sqrt(d);
    hipDeg  =  -99.0f * s;
    kneeDeg =  231.0f * s;
}

// AttackLight1 - kombo 1: o'ng qo'l yuqori-o'ngdan pastga-chapga kesadi. 0.55 s.
//   0.00..0.29  tayyorgarlik (qo'l orqaga, tana o'ngga ry +18)
//   0.29..0.47  tez zarba    (qo'l oldinga, tana ry -22, chanoq oldinga siljiydi)
//   0.47..1.00  neytralga qaytish
void poseAttackLight1(Pose& p, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 0.55f, 0.0f, 1.0f);
    const float wind = easeInOut   (seg(u, 0.00f, 0.29f));
    const float hit  = easeOutCubic(seg(u, 0.29f, 0.47f));
    const float back = easeInOut   (seg(u, 0.47f, 1.00f));
    const float act  = 1.0f - back;

    const float torsoRy = lerpf(18.0f * wind, -22.0f, hit) * act;

    // o'ng qo'l: orqa-yuqoridan oldin-pastga (rz manfiy => o'ng qo'l ichkariga)
    p.b[B_AUR ].rx = lerpf( 46.0f * wind, -74.0f, hit) * act;
    p.b[B_AUR ].rz = lerpf( 30.0f * wind, -26.0f, hit) * act;
    p.b[B_ALR ].rx = lerpf(-66.0f * wind, -16.0f, hit) * act;
    p.b[B_ALR ].rz = lerpf(-16.0f * wind,   6.0f, hit) * act;

    // chap qo'l muvozanat uchun teskari yo'nalishda ochiladi
    p.b[B_AUL ].rx = lerpf(-16.0f * wind,  30.0f, hit) * act;
    p.b[B_AUL ].rz = lerpf( -8.0f * wind, -20.0f, hit) * act;
    p.b[B_ALL_].rx = lerpf(-34.0f * wind, -18.0f, hit) * act;

    p.b[B_TORSO].ry = torsoRy;
    p.b[B_TORSO].rx = lerpf(-6.0f * wind, 12.0f, hit) * act;   // tayyorgarlikda orqaga, zarbada oldinga
    p.b[B_TORSO].rz = -4.0f * hit * act;
    p.b[B_HEAD ].ry = -0.45f * torsoRy;                        // bosh nishonda qoladi
    p.b[B_HEAD ].rx =  4.0f * hit * act;

    p.b[B_PELVIS].ry = lerpf(9.0f * wind, -11.0f, hit) * act;
    p.b[B_PELVIS].oz =  0.09f * hit * act;                     // chanoq oldinga
    p.b[B_PELVIS].oy = -0.02f * hit * act;

    p.b[B_LUR].rx = -14.0f * hit * act;   p.b[B_LUR].rz =  6.0f * wind * act;
    p.b[B_LUL].rx =  10.0f * hit * act;   p.b[B_LUL].rz = -6.0f * wind * act;
    p.b[B_LLR].rx =  18.0f * hit * act;
    p.b[B_LLL].rx =  14.0f * hit * act;
}

// AttackLight2 - kombo 2: TESKARI tomondan (chapdan o'ngga). 0.45 s.
// Tana ry -20 (tayyorgarlik) -> +16 (zarba).
void poseAttackLight2(Pose& p, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 0.45f, 0.0f, 1.0f);
    const float wind = easeInOut   (seg(u, 0.00f, 0.26f));
    const float hit  = easeOutCubic(seg(u, 0.26f, 0.46f));
    const float back = easeInOut   (seg(u, 0.46f, 1.00f));
    const float act  = 1.0f - back;

    const float torsoRy = lerpf(-20.0f * wind, 16.0f, hit) * act;

    // o'ng qo'l chap yelka oldiga yig'iladi, so'ng o'ngga ochilib kesadi
    p.b[B_AUR ].rx = lerpf(-26.0f * wind, -58.0f, hit) * act;
    p.b[B_AUR ].rz = lerpf(-36.0f * wind,  34.0f, hit) * act;
    p.b[B_ALR ].rx = lerpf(-74.0f * wind, -20.0f, hit) * act;
    p.b[B_ALR ].rz = lerpf(-26.0f * wind,  16.0f, hit) * act;

    p.b[B_AUL ].rx = lerpf( 10.0f * wind, -24.0f, hit) * act;
    p.b[B_AUL ].rz = lerpf( 14.0f * wind,  -6.0f, hit) * act;
    p.b[B_ALL_].rx = lerpf(-20.0f * wind, -44.0f, hit) * act;

    p.b[B_TORSO].ry = torsoRy;
    p.b[B_TORSO].rx = lerpf(-4.0f * wind, 9.0f, hit) * act;
    p.b[B_TORSO].rz =  4.0f * hit * act;
    p.b[B_HEAD ].ry = -0.45f * torsoRy;
    p.b[B_HEAD ].rx =  3.0f * hit * act;

    p.b[B_PELVIS].ry = lerpf(-10.0f * wind, 9.0f, hit) * act;
    p.b[B_PELVIS].oz =  0.06f * hit * act;
    p.b[B_PELVIS].oy = -0.015f * hit * act;

    p.b[B_LUL].rx = -12.0f * hit * act;   p.b[B_LUL].rz = -6.0f * wind * act;
    p.b[B_LUR].rx =   9.0f * hit * act;   p.b[B_LUR].rz =  6.0f * wind * act;
    p.b[B_LLL].rx =  16.0f * hit * act;
    p.b[B_LLR].rx =  12.0f * hit * act;
}

// AttackLight3 - kombo 3, yakunlovchi: ikki qo'l bilan tepadan pastga. 0.70 s.
// Oxirida oldinga bir qadam (chanoq oz +0.15), tana 14 gradus OLDINGA egiladi.
void poseAttackLight3(Pose& p, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 0.70f, 0.0f, 1.0f);
    const float lift = easeInOut   (seg(u, 0.00f, 0.32f));   // ikki qo'l tepaga
    const float chop = easeOutCubic(seg(u, 0.32f, 0.52f));   // pastga zarba
    const float step = smoothstepf (seg(u, 0.40f, 0.72f));   // oldinga qadam
    const float back = easeInOut   (seg(u, 0.80f, 1.00f));
    const float act  = 1.0f - back;

    // qo'llar bosh ustiga (-145) ko'tarilib, oldin-pastga (-48) uriladi
    p.b[B_AUL ].rx = lerpf(-145.0f * lift, -48.0f, chop) * act;
    p.b[B_AUR ].rx = lerpf(-145.0f * lift, -48.0f, chop) * act;
    p.b[B_AUL ].rz = lerpf(  12.0f * lift,   8.0f, chop) * act;   // chap ichkariga
    p.b[B_AUR ].rz = lerpf( -12.0f * lift,  -8.0f, chop) * act;   // o'ng ichkariga
    p.b[B_ALL_].rx = lerpf( -42.0f * lift, -10.0f, chop) * act;
    p.b[B_ALR ].rx = lerpf( -42.0f * lift, -10.0f, chop) * act;
    p.b[B_ALL_].rz =   8.0f * lift * act;
    p.b[B_ALR ].rz =  -8.0f * lift * act;

    p.b[B_TORSO].rx = lerpf(-9.0f * lift, 14.0f, chop) * act;    // oxirida 14 gradus oldinga
    p.b[B_TORSO].ry =  5.0f * lift * act - 4.0f * chop * act;
    p.b[B_HEAD ].rx = lerpf(-7.0f * lift, 15.0f, chop) * act;    // pastga qaraydi

    const float sink = 0.05f * chop * act;                       // zarbada cho'kish
    p.b[B_PELVIS].oz =  0.15f * step * act;                      // oldinga qadam
    p.b[B_PELVIS].oy = -sink;
    p.b[B_PELVIS].ry = -5.0f * chop * act;

    // Oyoqlar: avval cho'kishni qoplovchi asosiy bukilish, ustiga qadam.
    float hipB, kneeB;
    legBendFor(sink, hipB, kneeB);
    const float st = step * act;
    p.b[B_LUR].rx = hipB  - 20.0f * st;   p.b[B_LUR].rz =  7.0f * st;   // oldingi oyoq
    p.b[B_LLR].rx = kneeB + 10.0f * st;
    p.b[B_LUL].rx = hipB  + 14.0f * st;   p.b[B_LUL].rz = -7.0f * st;   // orqadagi oyoq
    p.b[B_LLL].rx = kneeB +  6.0f * st;
}

// AttackHeavy - og'ir zarba. 0.95 s.
//   0.00..0.40  uzun tayyorgarlik (qilich orqaga-tepaga, tana 18 gradus ORQAGA)
//   0.40..0.55  kuchli zarba      (tana 25 gradus OLDINGA)
//   0.55..1.00  sekin qaytish
// Chanoq y sinus bilan pastga-tepaga tebranadi (og'irlik ko'chishi).
void poseAttackHeavy(Pose& p, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 0.95f, 0.0f, 1.0f);
    const float wind = easeInOut   (seg(u, 0.00f, 0.40f));
    const float hit  = easeOutCubic(seg(u, 0.40f, 0.55f));
    const float back = easeInOut   (seg(u, 0.55f, 1.00f));
    const float act  = 1.0f - back;

    // o'ng qo'l: qilich orqa-tepaga tortiladi, so'ng butun vazn bilan pastga
    p.b[B_AUR ].rx = lerpf( 72.0f * wind, -86.0f, hit) * act;
    p.b[B_AUR ].rz = lerpf( 28.0f * wind, -16.0f, hit) * act;
    p.b[B_ALR ].rx = lerpf(-96.0f * wind, -12.0f, hit) * act;
    p.b[B_ALR ].rz = lerpf(-14.0f * wind,   8.0f, hit) * act;

    // chap qo'l dastani ushlab turadi (ikki qo'llab)
    p.b[B_AUL ].rx = lerpf( 40.0f * wind, -70.0f, hit) * act;
    p.b[B_AUL ].rz = lerpf( 20.0f * wind,  -8.0f, hit) * act;
    p.b[B_ALL_].rx = lerpf(-86.0f * wind, -20.0f, hit) * act;
    p.b[B_ALL_].rz = lerpf( 12.0f * wind,   6.0f, hit) * act;

    p.b[B_TORSO].rx = lerpf(-18.0f * wind, 25.0f, hit) * act;   // orqaga -> oldinga
    p.b[B_TORSO].ry = lerpf( 16.0f * wind, -12.0f, hit) * act;
    p.b[B_TORSO].rz = -5.0f * hit * act;
    p.b[B_HEAD ].rx = lerpf(  9.0f * wind, 12.0f, hit) * act;
    p.b[B_HEAD ].ry = lerpf( -8.0f * wind,  6.0f, hit) * act;

    // chanoq: sinus bilan pastga (tayyorgarlik) - tepaga (qaytish)
    const float sw   = -0.055f * std::sin(TAU * u) * act;
    const float sink = (sw < 0.0f) ? -sw : 0.0f;        // faqat tushish qoplanadi
    p.b[B_PELVIS].oy = sw;
    p.b[B_PELVIS].oz =  0.10f * hit * act;
    p.b[B_PELVIS].ry = lerpf(10.0f * wind, -8.0f, hit) * act;

    // chanoq tushganda tizzalar bukiladi (panjalar yer ustida qoladi)
    float hipB, kneeB;
    legBendFor(sink, hipB, kneeB);
    p.b[B_LUR].rx = hipB  - 16.0f * hit * act;   p.b[B_LUR].rz =  8.0f * wind * act;
    p.b[B_LLR].rx = kneeB + 10.0f * hit * act;
    p.b[B_LUL].rx = hipB  + 12.0f * hit * act;   p.b[B_LUL].rz = -8.0f * wind * act;
    p.b[B_LLL].rx = kneeB +  4.0f * hit * act;
}

// KickClip - o'ng oyoq oldinga tepadi (son rx -70). 0.45 s.
// Tana 14 gradus ORQAGA, qo'llar muvozanat uchun yon tomonga ochiladi.
void poseKick(Pose& p, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 0.45f, 0.0f, 1.0f);
    const float prep = easeInOut   (seg(u, 0.00f, 0.30f));   // tizza ko'kragga tortiladi
    const float kick = easeOutCubic(seg(u, 0.30f, 0.58f));   // boldir otiladi
    const float back = easeInOut   (seg(u, 0.58f, 1.00f));
    const float act  = 1.0f - back;
    const float bal  = (0.4f * prep + 0.6f * kick) * act;    // muvozanat egri chizig'i

    // tepuvchi (o'ng) oyoq
    p.b[B_LUR].rx = lerpf(-42.0f * prep, -70.0f, kick) * act;
    p.b[B_LUR].rz =   7.0f * prep * act;
    p.b[B_LLR].rx = lerpf( 74.0f * prep,  10.0f, kick) * act;

    // tayanch (chap) oyoq ozgina bukiladi
    p.b[B_LUL].rx =  10.0f * bal;   p.b[B_LUL].rz = -5.0f * bal;
    p.b[B_LLL].rx =  16.0f * bal;

    p.b[B_TORSO].rx = lerpf(-6.0f * prep, -14.0f, kick) * act;   // tana ORQAGA
    p.b[B_TORSO].ry = -8.0f * kick * act;
    p.b[B_TORSO].rz =  5.0f * kick * act;
    p.b[B_HEAD ].rx =  9.0f * kick * act;                        // bosh tik qoladi

    // qo'llar yon tomonga (chap tashqariga = -rz, o'ng tashqariga = +rz)
    p.b[B_AUL ].rx =  14.0f * bal;   p.b[B_AUL ].rz = -42.0f * bal;
    p.b[B_AUR ].rx =  14.0f * bal;   p.b[B_AUR ].rz =  42.0f * bal;
    p.b[B_ALL_].rx = -28.0f * bal;   p.b[B_ALL_].rz = -10.0f * bal;
    p.b[B_ALR ].rx = -28.0f * bal;   p.b[B_ALR ].rz =  10.0f * bal;

    p.b[B_PELVIS].oz =  0.04f * kick * act;
    p.b[B_PELVIS].oy = -0.02f * bal;
    p.b[B_PELVIS].ry = -6.0f * kick * act;
}

// Block - qilich ko'krak oldida ushlab turiladi (SIKLIK).
// Ikkala qo'l oldinga bukilgan, tana cho'kkan va yon tomonga 10 gradus burilgan.
void poseBlock(Pose& p, float t) {
    clearPose(p);
    const float br = std::sin(t * 1.9f);          // tarang, tez nafas
    const float sw = std::sin(t * 0.70f);         // sekin tebranish

    p.b[B_PELVIS].oy = -0.06f + 0.004f * br;      // tana biroz cho'kkan
    p.b[B_PELVIS].ry =  5.0f;
    p.b[B_TORSO ].rx =  8.0f + 1.0f * br;         // yengil oldinga (himoya holati)
    p.b[B_TORSO ].ry = 10.0f + 1.5f * sw;         // yon tomonga 10 gradus
    p.b[B_TORSO ].oy = 0.006f * br;
    p.b[B_HEAD  ].rx = -5.0f + 0.8f * br;
    p.b[B_HEAD  ].ry = -9.0f + 1.5f * sw;         // ko'zlar nishonda

    // qilich ko'krak oldida: yelka -55, tirsak -75, panjalar o'rtaga yig'ilgan
    p.b[B_AUL ].rx = -55.0f + 1.5f * br;   p.b[B_AUL ].rz =  16.0f;
    p.b[B_AUR ].rx = -55.0f + 1.5f * br;   p.b[B_AUR ].rz = -16.0f;
    p.b[B_ALL_].rx = -75.0f + 2.0f * br;   p.b[B_ALL_].rz =  22.0f;
    p.b[B_ALR ].rx = -75.0f + 2.0f * br;   p.b[B_ALR ].rz = -18.0f;

    // tizzalar chanoq tushishini qoplaydi (panjalar yer ustida qoladi)
    float hipB, kneeB;
    legBendFor(0.06f, hipB, kneeB);
    p.b[B_LUL].rx = hipB;    p.b[B_LUL].rz = -8.0f;
    p.b[B_LUR].rx = hipB;    p.b[B_LUR].rz =  8.0f;
    p.b[B_LLL].rx = kneeB;
    p.b[B_LLR].rx = kneeB;
}

// ParryHit - muvaffaqiyatli parry. 0.35 s. Block dan boshlanib, qo'llar keskin
// YON tomonga uriladi (rz +-30), tana ry +-14 silkinadi, so'ng Block ga qaytadi.
void poseParryHit(Pose& p, float t, float lt) {
    poseBlock(p, t);
    const float u  = clampf(lt / 0.35f, 0.0f, 1.0f);
    const float k  = arc01(seg(u, 0.00f, 0.90f));   // 0 -> 1 -> 0 (Block ga qaytadi)
    const float sh = std::sin(TAU * u);             // avval bir yonga, keyin ikkinchisiga

    // zarba qo'llarni tashqariga va orqaga itaradi
    p.b[B_AUL ].rz += -30.0f * k;
    p.b[B_AUR ].rz +=  30.0f * k;
    p.b[B_AUL ].rx +=  24.0f * k;
    p.b[B_AUR ].rx +=  24.0f * k;
    p.b[B_ALL_].rz +=  14.0f * k;
    p.b[B_ALR ].rz += -14.0f * k;
    p.b[B_ALL_].rx +=  18.0f * k;
    p.b[B_ALR ].rx +=  18.0f * k;

    p.b[B_TORSO ].ry += 14.0f * sh * k;
    p.b[B_TORSO ].rx += -9.0f * k;                  // tana ozgina orqaga
    p.b[B_TORSO ].rz +=  6.0f * sh * k;
    p.b[B_HEAD  ].ry += -7.0f * sh * k;
    p.b[B_PELVIS].ry +=  6.0f * sh * k;
    p.b[B_PELVIS].oz += -0.05f * k;
}

// Hurt - zarba yeyish reaksiyasi. 0.35 s. Tana 22 gradus ORQAGA, bosh orqaga,
// qo'llar ichkariga yig'iladi, chanoq 0.12 orqaga siljiydi. Tez qaytadi.
// Hurt — YO'NALISHLI zarba yeyish reaksiyasi. 0.35 s.
//   sc.hitDir:    0 = oldindan, +90 = +X tomondan, +-180 = orqadan
//   sc.hitWeight: 0.35 (o'q/tepish) .. 1.00 (og'ir zarba)
//
// GEOMETRIK BYUDJET (haqiqiy .obj dan o'lchangan):
//   TORSO pivoti 0.58h -> bosh markazi richagi 0.345h (ottoman) / 0.368h (crusader)
//   bo'yin radiusi 0.053h = 9.6 sm (ottoman) / 0.043h = 7.8 sm (crusader)
//   TORSO.rz = 5 gradus -> bosh yon siljishi 5.5 / 5.8 sm  < radius  => RUXSAT
//   Eski kodda TORSO.rz = 6 gradus va YO'NALISHSIZ edi: zarba qayerdan kelsa ham
//   bosh DOIM bir tomonga 6.6-7.0 sm surilardi — "kaltak yesa boshi qiyshayadi".
// Jami bosh pitch'i -26 gradus (eski -40): quti-rig'da bosh QATTIQ blok bo'lgani
// uchun -40 "bo'yin sindi" bo'lib ko'rinardi.
void poseHurt(Pose& p, float lt, const StrideCtx& sc) {
    clearPose(p);
    const float u   = clampf(lt / 0.35f, 0.0f, 1.0f);
    const float in  = easeOutCubic(seg(u, 0.00f, 0.22f));
    const float out = easeInOut   (seg(u, 0.22f, 1.00f));
    const float w   = clampf(sc.hitWeight, 0.35f, 1.0f);
    const float k   = in * (1.0f - out) * w;
    const float sh  = std::sin(TAU * 2.0f * u) * k;      // qisqa silkinish

    const float a    = deg2rad(wrapAngleDeg(sc.hitDir));
    const float fwd  = std::cos(a);      // +1 = oldindan -> tana ORQAGA
    const float side = std::sin(a);      // +1 = +X dan   -> tana -X ga

    p.b[B_PELVIS].oz = -0.085f * fwd  * k;
    p.b[B_PELVIS].ox = -0.055f * side * k;      // zarbadan UZOQLASHADI
    p.b[B_PELVIS].oy = -0.025f * k;
    p.b[B_PELVIS].ry =   5.0f * sh;

    p.b[B_TORSO ].rx = -16.0f * fwd  * k;
    p.b[B_TORSO ].ry =  -8.0f * side * k + 3.0f * sh;
    p.b[B_TORSO ].rz =   5.0f * side * k;       // BYUDJET: 5 dan oshirmang
    p.b[B_HEAD  ].rx = -10.0f * fwd  * k;       // jami -26 gradus
    p.b[B_HEAD  ].ry =  -4.0f * side * k;
    p.b[B_HEAD  ].rz =   3.0f * side * k;       // BYUDJET: 3 dan oshirmang

    // Qo'llar zarba joyini yopadi: zarba kelgan TOMON qo'li ko'proq yig'iladi
    const float gL = 0.55f + 0.45f * clampf( side, 0.0f, 1.0f);   // -X ("L") tomoni
    const float gR = 0.55f + 0.45f * clampf(-side, 0.0f, 1.0f);   // +X ("R") tomoni
    p.b[B_AUL ].rx = -24.0f * k * gL;   p.b[B_AUL ].rz =  20.0f * k * gL;
    p.b[B_AUR ].rx = -24.0f * k * gR;   p.b[B_AUR ].rz = -20.0f * k * gR;
    p.b[B_ALL_].rx = -56.0f * k * gL;   p.b[B_ALL_].rz =  18.0f * k * gL;
    p.b[B_ALR ].rx = -56.0f * k * gR;   p.b[B_ALR ].rz = -18.0f * k * gR;

    // Muvozanat qadami: oldindan zarba -> orqaga, orqadan -> oldinga
    p.b[B_LUL].rx =  16.0f * fwd * k;   p.b[B_LUL].rz = -6.0f * k - 6.0f * side * k;
    p.b[B_LUR].rx = -11.0f * fwd * k;   p.b[B_LUR].rz =  6.0f * k - 6.0f * side * k;
    p.b[B_LLL].rx =  24.0f * k * ((fwd > 0.0f) ? fwd : 0.35f);
    p.b[B_LLR].rx =  17.0f * k * ((fwd > 0.0f) ? fwd : 0.35f);
}

// Stagger - poza buzildi, muvozanat yo'qolgan. 1.2 s.
// Tana keng tebranadi (ry +-25, rz +-14), oyoqlar keng ochilgan, qo'llar pastda
// osilgan, chanoq y -0.18.
void poseStagger(Pose& p, float lt, const StrideCtx& sc) {
    clearPose(p);
    const float u   = clampf(lt / 1.2f, 0.0f, 1.0f);
    const float in  = easeOutCubic(seg(u, 0.00f, 0.12f));
    const float out = easeInOut   (seg(u, 0.80f, 1.00f));
    const float k   = in * (1.0f - out);
    const float w1  = std::sin(TAU * 1.60f * u);
    const float w2  = std::sin(TAU * 1.15f * u + 1.0f);

    const float drop = 0.18f * k;
    p.b[B_PELVIS].oy = -drop;
    p.b[B_PELVIS].ox =  0.05f * w1 * k;
    p.b[B_PELVIS].oz = -0.04f * w2 * k;
    p.b[B_PELVIS].ry =   8.0f * w1 * k;
    p.b[B_PELVIS].rz =   6.0f * w2 * k;

    p.b[B_TORSO].ry =  25.0f * w1 * k;             // keng tebranish
    p.b[B_TORSO].rz =  14.0f * w2 * k;
    p.b[B_TORSO].rx =   7.0f * k + 3.0f * w2 * k;  // ozgina bukchaygan
    p.b[B_HEAD ].ry = -12.0f * w1 * k;
    p.b[B_HEAD ].rz =  -8.0f * w2 * k;
    p.b[B_HEAD ].rx =  10.0f * k;

    // qo'llar bo'sh osilgan va tebranish bilan sudraladi
    p.b[B_AUL ].rx =   6.0f * k + 12.0f * w1 * k;   p.b[B_AUL ].rz = -20.0f * k - 6.0f * w2 * k;
    p.b[B_AUR ].rx =   6.0f * k - 12.0f * w1 * k;   p.b[B_AUR ].rz =  20.0f * k + 6.0f * w2 * k;
    p.b[B_ALL_].rx = -16.0f * k -  8.0f * w2 * k;
    p.b[B_ALR ].rx = -16.0f * k +  8.0f * w2 * k;

    // oyoqlar keng ochilgan (rz +-20); chanoq -0.18 ga tushgani uchun tizzalar
    // shunga mos ravishda bukilgan, ustiga muvozanatsiz qadamlash tebranishi
    float hipB, kneeB;
    legBendFor(drop, hipB, kneeB);
    p.b[B_LUL].rx = hipB  - 10.0f * w1 * k;   p.b[B_LUL].rz = -20.0f * k;
    p.b[B_LUR].rx = hipB  + 10.0f * w1 * k;   p.b[B_LUR].rz =  20.0f * k;
    p.b[B_LLL].rx = kneeB +  8.0f * w1 * k;
    p.b[B_LLR].rx = kneeB -  8.0f * w1 * k;
    // Boshlang'ich siljish zarba yo'nalishi bo'yicha; 0.25 u dan keyin
    // tebranish egallaydi. Ilgari muvozanat buzilishi qayerdan zarba
    // yeganingizdan qat'i nazar bir xil ko'rinardi.
    const float a2   = deg2rad(wrapAngleDeg(sc.hitDir));
    const float f2   = std::cos(a2), s2 = std::sin(a2);
    const float lead = (1.0f - smoothstepf(clampf(u / 0.25f, 0.0f, 1.0f))) * k;
    p.b[B_PELVIS].oz += -0.08f * f2 * lead;
    p.b[B_PELVIS].ox += -0.06f * s2 * lead;
    p.b[B_TORSO ].rx += -12.0f * f2 * lead;
    p.b[B_TORSO ].rz +=   8.0f * s2 * lead;
}

// Death - oldinga yiqilish. 1.4 s, OXIRGI KADRDA QOTADI (u = 1 da to'xtaydi).
//   0.00..0.35  tizzaga cho'kish (tizzalar 80 gradus)
//   0.35..0.75  oldinga qulash   (tana ~75 gradus OLDINGA)
//   0.75..1.00  yerda qotib qolish (tana ~88 gradus)
// Chanoq tushishi haqida: topshiriqdagi -0.50 / -0.85 qiymatlari bbox-rig'da
// modelni yer ostiga olib kirar edi (chanoq pivoti 0.50h, boldir uchi 0.00h).
// Shuning uchun tushish chanoq (-0.34 / -0.45) va oyoqlarning GORIZONTAL yotishi
// o'rtasida taqsimlangan - vizual natija bir xil: bosh 0.86h dan ~0.13h ga tushadi.
// O'lim — endi YO'NALISHLI. Ilgari personaj doim OLDINGA (yuzi bilan) qulardi,
// Enemy::receiveHit esa dushmanni o'lishdan oldin urgan tomonga burar edi:
// natijada jasad HAR DOIM o'yinchiga tomon yiqilardi.
void poseDeath(Pose& p, float lt, const StrideCtx& sc) {
    clearPose(p);
    // |hitDir| < 90 = zarba OLDINDAN keldi -> ORQAGA (chalqancha) yiqiladi
    const float back = (std::fabs(wrapAngleDeg(sc.hitDir)) < 90.0f) ? -1.0f : 1.0f;
    const float sdd  = std::sin(deg2rad(wrapAngleDeg(sc.hitDir)));
    const float u     = clampf(lt / 1.4f, 0.0f, 1.0f);
    const float kneel = smoothstepf(seg(u, 0.00f, 0.35f));
    const float fall  = easeInOut  (seg(u, 0.35f, 0.75f));
    const float rest  = smoothstepf(seg(u, 0.75f, 1.00f));

    p.b[B_PELVIS].oy = -0.34f * kneel - 0.11f * fall;
    p.b[B_PELVIS].oz =  0.06f * fall;
    p.b[B_PELVIS].ry = -5.0f * fall;
    p.b[B_PELVIS].rz = -4.0f * fall;

    // tana: cho'kkanda oldinga egiladi, so'ng yuzi bilan yerga qulaydi
    p.b[B_TORSO].rx = (14.0f * kneel + 61.0f * fall + 13.0f * rest) * back;
    p.b[B_TORSO].ry = -8.0f * fall;
    p.b[B_TORSO].rz =  6.0f * fall + 18.0f * sdd * (fall + rest);
    p.b[B_HEAD ].rx = (-6.0f * kneel + 20.0f * fall - 6.0f * rest) * back;
    p.b[B_HEAD ].rz = -9.0f * fall;

    // tizzalar 80 gradus bukiladi, so'ng butun oyoq orqaga gorizontal yotadi
    p.b[B_LUL].rx = -8.0f * kneel + 96.0f * fall;   p.b[B_LUL].rz = -7.0f * kneel - 5.0f * fall;
    p.b[B_LUR].rx = -8.0f * kneel + 96.0f * fall;   p.b[B_LUR].rz =  7.0f * kneel + 5.0f * fall;
    p.b[B_LLL].rx = 80.0f * kneel - 78.0f * fall;
    p.b[B_LLR].rx = 80.0f * kneel - 74.0f * fall;

    // qo'llar avval tanani tutmoqchi bo'ladi, oxirida yonlarda yoyilib qoladi
    p.b[B_AUL ].rx = -26.0f * kneel + 18.0f * fall;   p.b[B_AUL ].rz = -12.0f * kneel - 18.0f * fall;
    p.b[B_AUR ].rx = -22.0f * kneel + 15.0f * fall;   p.b[B_AUR ].rz =  12.0f * kneel + 20.0f * fall;
    p.b[B_ALL_].rx = -36.0f * kneel + 22.0f * fall;   p.b[B_ALL_].rz =  10.0f * fall;
    p.b[B_ALR ].rx = -32.0f * kneel + 18.0f * fall;   p.b[B_ALR ].rz = -10.0f * fall;
}

// BowAim - kamon tortilgan holat (SIKLIK). Chap qo'l oldinga cho'zilgan (rx -85),
// o'ng qo'l ko'krak yonida tortilgan (rx -60, rz -25), tana ry +25 (yon turish).
void poseBowAim(Pose& p, float t) {
    clearPose(p);
    const float br = std::sin(t * 1.5f);          // tarang nafas
    const float sw = std::sin(t * 0.55f);         // nishonni kuzatish

    p.b[B_PELVIS].oy = -0.015f + 0.003f * br;
    p.b[B_PELVIS].ry =  12.0f;
    p.b[B_TORSO ].ry =  25.0f + 1.5f * sw;        // yon turish
    p.b[B_TORSO ].rx =   4.0f + 0.8f * br;
    p.b[B_TORSO ].oy =  0.005f * br;
    p.b[B_HEAD  ].ry = -21.0f + 1.2f * sw;        // bosh nishonga qaraydi
    p.b[B_HEAD  ].rx =  -2.0f + 0.6f * br;

    p.b[B_AUL ].rx = -85.0f + 1.2f * br;   p.b[B_AUL ].rz =  14.0f;   // kamonni ushlaydi
    p.b[B_ALL_].rx =  -6.0f + 0.6f * br;   p.b[B_ALL_].rz =   4.0f;
    p.b[B_AUR ].rx = -60.0f + 1.5f * br;   p.b[B_AUR ].rz = -25.0f;   // ipni tortadi
    p.b[B_ALR ].rx = -94.0f - 2.0f * br;   p.b[B_ALR ].rz = -18.0f;

    p.b[B_LUL].rx = -12.0f;   p.b[B_LUL].rz = -9.0f;
    p.b[B_LUR].rx =   6.0f;   p.b[B_LUR].rz =  9.0f;
    p.b[B_LLL].rx =  22.0f;
    p.b[B_LLR].rx =  10.0f;
}

// BowShoot - o'q otish. 0.40 s. O'ng qo'l keskin orqaga ochiladi, tana ozgina orqaga.
void poseBowShoot(Pose& p, float t, float lt) {
    poseBowAim(p, t);
    const float u   = clampf(lt / 0.40f, 0.0f, 1.0f);
    const float rel = easeOutCubic(seg(u, 0.00f, 0.16f));   // ip qo'yib yuboriladi
    const float set = easeInOut   (seg(u, 0.45f, 1.00f));   // qo'l joyiga qaytadi
    const float k   = rel * (1.0f - set);
    const float rec = std::sin(TAU * 3.0f * u) * (1.0f - saturate(u * 1.7f));  // qaltirash

    // o'ng qo'l keskin orqaga ochiladi
    p.b[B_AUR ].rx +=  46.0f * k;
    p.b[B_AUR ].rz += -26.0f * k;
    p.b[B_ALR ].rx +=  56.0f * k;
    p.b[B_ALR ].rz += -14.0f * k;

    // chap (kamon) qo'l zarbadan qaltiraydi
    p.b[B_AUL ].rx +=  4.0f * k + 1.5f * rec;
    p.b[B_ALL_].rx += -6.0f * k + 1.0f * rec;

    p.b[B_TORSO ].rx += -7.0f * k;      // tana ozgina ORQAGA
    p.b[B_TORSO ].ry +=  6.0f * k;
    p.b[B_HEAD  ].rx += -3.0f * k;
    p.b[B_PELVIS].oz += -0.03f * k;
}

// Execute - yakunlovchi zarba (staggered raqibga). 1.3 s.
//   0.00..0.30  oldinga qadam va pastga cho'kish
//   0.30..0.55  ikki qo'l bilan pastga zarba
//   0.55..0.90  ushlab turish (kuchlanish qaltirashi)
//   0.90..1.00  tik turish
void poseExecute(Pose& p, float t, float lt) {
    clearPose(p);
    const float u    = clampf(lt / 1.3f, 0.0f, 1.0f);
    const float rise = easeInOut   (seg(u, 0.90f, 1.00f));
    const float act  = 1.0f - rise;
    const float step = smoothstepf (seg(u, 0.00f, 0.30f)) * act;
    const float lift = smoothstepf (seg(u, 0.10f, 0.30f));
    const float hit  = easeOutCubic(seg(u, 0.30f, 0.55f));
    const float hold = seg(u, 0.55f, 0.90f) * (1.0f - rise);
    const float trem = 1.2f * hold * std::sin(t * 11.0f);   // kuchlanish qaltirashi

    // ikki qo'l bosh ustiga ko'tarilib, pastga uriladi va shu holda qoladi
    p.b[B_AUL ].rx = lerpf(-132.0f * lift, -34.0f, hit) * act + trem;
    p.b[B_AUR ].rx = lerpf(-132.0f * lift, -34.0f, hit) * act + trem;
    p.b[B_AUL ].rz =  14.0f * lift * act;
    p.b[B_AUR ].rz = -14.0f * lift * act;
    p.b[B_ALL_].rx = lerpf(-46.0f * lift, -8.0f, hit) * act;
    p.b[B_ALR ].rx = lerpf(-46.0f * lift, -8.0f, hit) * act;
    p.b[B_ALL_].rz =  10.0f * lift * act;
    p.b[B_ALR ].rz = -10.0f * lift * act;

    p.b[B_TORSO].rx = (12.0f * step + 28.0f * hit) * act + trem;   // pastga bosadi
    p.b[B_TORSO].ry = -6.0f * hit * act;
    p.b[B_HEAD ].rx = ( 4.0f * step + 12.0f * hit) * act;          // nishonga qaraydi
    p.b[B_HEAD ].ry =  4.0f * hit * act;

    const float drop = 0.20f * step + 0.05f * hit * act;           // pastga cho'kish
    p.b[B_PELVIS].oz =  0.16f * step;                              // oldinga qadam
    p.b[B_PELVIS].oy = -drop;
    p.b[B_PELVIS].ry = -6.0f * hit * act;

    // oldingi (o'ng) oyoq chuqurroq bukilgan, orqadagi (chap) yozilganroq -
    // ikkalasining vertikal yetishi teng qoladi (panjalar yer ustida)
    float hipB, kneeB;
    legBendFor(drop, hipB, kneeB);
    p.b[B_LUR].rx = hipB  - 8.0f * step;   p.b[B_LUR].rz =  8.0f * step;
    p.b[B_LLR].rx = kneeB + 3.0f * step;
    p.b[B_LUL].rx = hipB  + 8.0f * step;   p.b[B_LUL].rz = -8.0f * step;
    p.b[B_LLL].rx = kneeB - 3.0f * step;
}

// ---------------------------------------------------------------------------
//  Bir martalik kliplar ro'yxati va davomiyligi (sekund). 0 => siklik klip.
// ---------------------------------------------------------------------------
float oneShotDuration(AnimClip c) {
    switch (c) {
        case AnimClip::Draw:        return 0.80f;
        case AnimClip::Vault:       return 0.55f;
        case AnimClip::Mantle:      return 0.85f;
        case AnimClip::Slide:       return 0.70f;
        case AnimClip::Roll:        return 0.60f;
        case AnimClip::Dodge:       return 0.45f;
        case AnimClip::Assassinate: return 1.10f;

        // --- Jang kliplari ---
        case AnimClip::AttackLight1: return 0.55f;
        case AnimClip::AttackLight2: return 0.45f;
        case AnimClip::AttackLight3: return 0.70f;
        case AnimClip::AttackHeavy:  return 0.95f;
        case AnimClip::KickClip:     return 0.45f;
        case AnimClip::ParryHit:     return 0.35f;
        case AnimClip::Hurt:         return 0.35f;
        case AnimClip::Stagger:      return 1.20f;
        case AnimClip::Death:        return 1.40f;
        case AnimClip::BowShoot:     return 0.40f;
        case AnimClip::Execute:      return 1.30f;
        // Block va BowAim - ushlab turiladigan (siklik) holatlar => 0

        default:                    return 0.0f;
    }
}

inline bool isOneShotClip(AnimClip c) { return oneShotDuration(c) > 0.0f; }

// Draw eski xulqini saqlaydi (istalgan payt uzilishi mumkin). Yangi parkur/jang
// kliplari esa tugamaguncha lokomotsiya tomonidan uzilmaydi.
inline bool isProtectedOneShot(AnimClip c) {
    return isOneShotClip(c) && c != AnimClip::Draw;
}

// Ushlab turiladigan (siklik) jang holatlari va yakuniy kadrda qotadigan Death.
// Ular bir martalik EMAS, shuning uchun isProtectedOneShot() ularni qamramaydi.
// driveByLocomotion() ularni Walk/Run ga almashtirib yubormasligi kerak, shuning
// uchun setClip() da Walk/Run so'rovi e'tiborsiz qoldiriladi. Chiqish uchun o'yin
// mantiqi boshqa klipni (masalan Idle yoki AttackLight1) ANIQ so'rashi kifoya.
inline bool isHeldCombatClip(AnimClip c) {
    return c == AnimClip::Block || c == AnimClip::BowAim || c == AnimClip::Death;
}

// Past-poligonli model uchun: a'zo suyaklarining LOKAL transformatsiyasini
// o'chiradi. A'zolar baribir ota suyak (chanoq/ko'krak) harakatini meros qilib
// oladi, ya'ni faqat ildiz/tana harakati qoladi va mesh yirtilmaydi.
void simplifyPose(Pose& p) {
    const int limb[8] = { B_AUL, B_ALL_, B_AUR, B_ALR, B_LUL, B_LLL, B_LUR, B_LLR };
    for (int i = 0; i < 8; ++i) {
        BonePose& b = p.b[limb[i]];
        b.ox = b.oy = b.oz = 0.0f;
        b.rx = b.ry = b.rz = 0.0f;
    }
}

void evalPose(AnimClip c, float t, float speedScale, float talk, float oneLocal,
              const StrideCtx& sc, Pose& out) {
    const float ss = clampf(speedScale, 0.2f, 3.0f);
    // Bir martalik kliplarning mahalliy vaqti hech qachon manfiy bo'lmasin
    const float ot = (oneLocal > 0.0f) ? oneLocal : 0.0f;
    switch (c) {
        // Walk va Run - BITTA uzluksiz poza. Chegara endi KO'RINMAYDI, chunki
        // qadam geometriyasi tezlikdan uzluksiz kelib chiqadi (klipdan emas).
        case AnimClip::Walk:
        case AnimClip::Run:    poseStride(out, sc);                       break;
        case AnimClip::Talk:   poseTalk  (out, t, talk);                  break;
        case AnimClip::Listen: poseListen(out, t);                        break;
        case AnimClip::Point:  posePoint (out, t);                        break;
        case AnimClip::Salute: poseSalute(out, t);                        break;
        case AnimClip::Draw:   poseDraw  (out, t, ot);                    break;
        case AnimClip::Sit:    poseSit   (out, t);                        break;

        // --- Parkur va jang kliplari ---
        case AnimClip::CrouchIdle:  poseCrouchIdle (out, t);                break;
        case AnimClip::CrouchWalk:  poseCrouchWalk (out, t, sc);            break;
        case AnimClip::Vault:       poseVault      (out, ot);               break;
        case AnimClip::Mantle:      poseMantle     (out, ot);               break;
        case AnimClip::ClimbUp:     poseClimbUp    (out, t);                break;
        case AnimClip::Hang:        poseHang       (out, t);                break;
        case AnimClip::Shimmy:      poseShimmy     (out, t);                break;
        case AnimClip::Slide:       poseSlide      (out, ot);               break;
        case AnimClip::Roll:        poseRoll       (out, ot);               break;
        case AnimClip::Fall:        poseFall       (out, t);                break;
        case AnimClip::WallRun:     poseWallRun    (out, t, 3.4f * ss);     break;
        case AnimClip::Dodge:       poseDodge      (out, ot);               break;
        case AnimClip::Assassinate: poseAssassinate(out, t, ot);            break;

        // --- Jang kliplari ---
        case AnimClip::AttackLight1: poseAttackLight1(out, ot);             break;
        case AnimClip::AttackLight2: poseAttackLight2(out, ot);             break;
        case AnimClip::AttackLight3: poseAttackLight3(out, ot);             break;
        case AnimClip::AttackHeavy:  poseAttackHeavy (out, ot);             break;
        case AnimClip::KickClip:     poseKick        (out, ot);             break;
        case AnimClip::Block:        poseBlock       (out, t);              break;
        case AnimClip::ParryHit:     poseParryHit    (out, t, ot);          break;
        case AnimClip::Hurt:         poseHurt        (out, ot, sc);         break;
        case AnimClip::Stagger:      poseStagger     (out, ot, sc);         break;
        case AnimClip::Death:        poseDeath       (out, ot, sc);         break;
        case AnimClip::BowAim:       poseBowAim      (out, t);              break;
        case AnimClip::BowShoot:     poseBowShoot    (out, t, ot);          break;
        case AnimClip::Execute:      poseExecute     (out, t, ot);          break;

        case AnimClip::Idle:
        default:               poseIdle  (out, t);                        break;
    }
}

// ---------------------------------------------------------------------------
//  3x3 matritsa yordamchilari (satr-tartibli: r.x = m[0]*x + m[1]*y + m[2]*z)
// ---------------------------------------------------------------------------

struct Xform {
    float m[9];
    float t[3];
    bool  ident;      // birlik transformatsiya (tez yo'l uchun)
};

const float kIdent3[9] = {1,0,0, 0,1,0, 0,0,1};

void eulerToMat(float rxDeg, float ryDeg, float rzDeg, float m[9]) {
    const float ax = deg2rad(rxDeg), ay = deg2rad(ryDeg), az = deg2rad(rzDeg);
    const float cx = std::cos(ax), sx = std::sin(ax);
    const float cy = std::cos(ay), sy = std::sin(ay);
    const float cz = std::cos(az), sz = std::sin(az);
    // R = Ry * Rx * Rz
    m[0] =  cy*cz + sy*sx*sz;   m[1] = -cy*sz + sy*sx*cz;   m[2] =  sy*cx;
    m[3] =  cx*sz;              m[4] =  cx*cz;              m[5] = -sx;
    m[6] = -sy*cz + cy*sx*sz;   m[7] =  sy*sz + cy*sx*cz;   m[8] =  cy*cx;
}

inline void mat3mul(const float a[9], const float b[9], float o[9]) {
    float r[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r[i*3 + j] = a[i*3 + 0]*b[0*3 + j] + a[i*3 + 1]*b[1*3 + j] + a[i*3 + 2]*b[2*3 + j];
    std::memcpy(o, r, sizeof(r));
}

inline void mat3apply(const float m[9], float x, float y, float z, float o[3]) {
    o[0] = m[0]*x + m[1]*y + m[2]*z;
    o[1] = m[3]*x + m[4]*y + m[5]*z;
    o[2] = m[6]*x + m[7]*y + m[8]*z;
}

// Pozadan har suyakning MODEL fazosidagi to'liq (ota-bola zanjiri bilan) transformatsiyasi.
//   pivot[] â€” model fazosidagi tayanch nuqtalar
//   h       â€” model balandligi (normallashtirilgan ofsetlar shunga ko'paytiriladi)
// Anatomik chegaralar. Har qanday poza (jang, parkur, cutscene) shu chegaradan
// o'ta olmaydi — bo'yin, tirsak va tizza teskari bukilmaydi.
// Bosh: real bo'yin yaw +-70, pitch +-50, roll +-40 gradus.
void clampBoneAngles(int b, float& rx, float& ry, float& rz) {
    switch (b) {
        case B_HEAD:
            rx = clampf(rx, -50.0f, 50.0f);
            ry = clampf(ry, -70.0f, 70.0f);
            rz = clampf(rz, -40.0f, 40.0f);
            break;
        case B_TORSO:                      // bel: orqaga kam, oldinga ko'proq
            rx = clampf(rx, -35.0f, 70.0f);
            ry = clampf(ry, -55.0f, 55.0f);
            rz = clampf(rz, -35.0f, 35.0f);
            break;
        case B_LLL: case B_LLR:            // tizza faqat ORQAGA bukiladi
            rx = clampf(rx, 0.0f, 150.0f);
            ry = clampf(ry, -12.0f, 12.0f);
            rz = clampf(rz, -12.0f, 12.0f);
            break;
        case B_ALL_: case B_ALR:           // tirsak faqat OLDINGA bukiladi
            rx = clampf(rx, -150.0f, 5.0f);
            break;
        default:
            break;
    }
}

void computeXforms(const Pose& p, const Vec3 pivot[BCOUNT], float h, Xform out[BCOUNT]) {
    for (int i = 0; i < BCOUNT; ++i) {
        const BonePose& bp = p.b[i];

        float arx = bp.rx, ary = bp.ry, arz = bp.rz;
        clampBoneAngles(i, arx, ary, arz);

        float R[9];
        const bool noRot = (arx == 0.0f && ary == 0.0f && arz == 0.0f);
        if (noRot) std::memcpy(R, kIdent3, sizeof(R));
        else       eulerToMat(arx, ary, arz, R);

        // Lokal: L(x) = R*(x - P) + P + O  =>  L(x) = R*x + (P + O - R*P)
        const Vec3& P = pivot[i];
        float rp[3];
        mat3apply(R, P.x, P.y, P.z, rp);
        const float T[3] = { P.x + bp.ox * h - rp[0],
                             P.y + bp.oy * h - rp[1],
                             P.z + bp.oz * h - rp[2] };

        const int par = kBone[i].parent;
        if (par < 0 || par >= i) {
            std::memcpy(out[i].m, R, sizeof(R));
            out[i].t[0] = T[0]; out[i].t[1] = T[1]; out[i].t[2] = T[2];
        } else {
            mat3mul(out[par].m, R, out[i].m);
            float pt[3];
            mat3apply(out[par].m, T[0], T[1], T[2], pt);
            out[i].t[0] = pt[0] + out[par].t[0];
            out[i].t[1] = pt[1] + out[par].t[1];
            out[i].t[2] = pt[2] + out[par].t[2];
        }

        // Birlik transformatsiyani aniqlaymiz (skinlashda tez yo'l)
        bool id = (std::fabs(out[i].t[0]) < 1e-7f &&
                   std::fabs(out[i].t[1]) < 1e-7f &&
                   std::fabs(out[i].t[2]) < 1e-7f);
        if (id) {
            for (int k = 0; k < 9 && id; ++k)
                if (std::fabs(out[i].m[k] - kIdent3[k]) > 1e-7f) id = false;
        }
        out[i].ident = id;
    }
}

// Suyak tayanch nuqtalarini model fazosiga o'tkazadi.
void buildPivots(const Vec3& bbMin, const Vec3& bbMax, Vec3 pivot[BCOUNT], float neckN) {
    const float h     = std::max(1e-6f, bbMax.y - bbMin.y);
    const float halfW = std::max(1e-6f, (bbMax.x - bbMin.x) * 0.5f);
    const float halfD = std::max(1e-6f, (bbMax.z - bbMin.z) * 0.5f);
    const float cx    = (bbMin.x + bbMax.x) * 0.5f;
    const float cz    = (bbMin.z + bbMax.z) * 0.5f;
    for (int i = 0; i < BCOUNT; ++i) {
        pivot[i].x = cx + kBone[i].px * halfW;
        pivot[i].y = bbMin.y + kBone[i].py * h;
        pivot[i].z = cz + kBone[i].pz * halfD;
    }
    // Bosh pivoti MESHDAN o'lchangan bo'yin balandligiga ko'chiriladi.
    // kBone[] dagi qat'iy 0.86 faqat ottoman'ga to'g'ri keladi; crusader'da
    // haqiqiy bo'yin 0.90 da va bosh o'z ICHIDAGI nuqta atrofida aylanardi.
    if (neckN > 0.5f && neckN < 0.99f)
        pivot[B_HEAD].y = bbMin.y + neckN * h;
}

// ---------------------------------------------------------------------------
//  Skinlash (dinamik ajratishsiz, oddiy float arifmetikasi)
// ---------------------------------------------------------------------------

// Diagnostika: har suyak uchun eng katta siljish (model balandligiga nisbatan)
void skinStats(const std::vector<MeshVertex>& src,
               const std::vector<MeshVertex>& dst,
               const std::vector<unsigned char>& bidx,
               float h) {
    if (src.size() != dst.size() || bidx.size() != src.size() || h <= 1e-6f) return;
    float maxd[BCOUNT] = {0};
    int   cnt [BCOUNT] = {0};
    for (size_t i = 0; i < src.size(); ++i) {
        const float dx = dst[i].px - src[i].px;
        const float dy = dst[i].py - src[i].py;
        const float dz = dst[i].pz - src[i].pz;
        const float d  = std::sqrt(dx*dx + dy*dy + dz*dz) / h;
        int b = bidx[i]; if (b >= BCOUNT) b = 0;
        if (d > maxd[b]) maxd[b] = d;
        ++cnt[b];
    }
    static const char* nm[BCOUNT] = {"Root","Pelvis","Torso","Head","ArmUL","ArmLL",
                                     "ArmUR","ArmLR","LegUL","LegLL","LegUR","LegLR"};
    std::printf("[SkinStats]");
    for (int b = 0; b < BCOUNT; ++b)
        std::printf(" %s=%.3f(%d)", nm[b], maxd[b], cnt[b]);
    std::printf("\n");
}

void skinAll(const std::vector<MeshVertex>& src,
             const std::vector<unsigned char>& bidx,
             const std::vector<float>& bw,
             const Xform xf[BCOUNT],
             std::vector<MeshVertex>& dst) {
    const size_t n = src.size();
    if (n == 0 || bidx.size() != n || bw.size() != n) return;
    if (dst.size() != n) dst = src;

    const MeshVertex*    s  = src.data();
    MeshVertex*          d  = dst.data();
    const unsigned char* bi = bidx.data();
    const float*         w  = bw.data();

    for (size_t i = 0; i < n; ++i) {
        const MeshVertex& v = s[i];
        int b = static_cast<int>(bi[i]);
        if (b >= BCOUNT) b = 0;
        const Xform& x = xf[b];
        const float ww = w[i];

        if (x.ident && ww >= 0.9995f) {
            d[i].px = v.px; d[i].py = v.py; d[i].pz = v.pz;
            d[i].nx = v.nx; d[i].ny = v.ny; d[i].nz = v.nz;
            continue;
        }

        const float ax = x.m[0]*v.px + x.m[1]*v.py + x.m[2]*v.pz + x.t[0];
        const float ay = x.m[3]*v.px + x.m[4]*v.py + x.m[5]*v.pz + x.t[1];
        const float az = x.m[6]*v.px + x.m[7]*v.py + x.m[8]*v.pz + x.t[2];
        float nx = x.m[0]*v.nx + x.m[1]*v.ny + x.m[2]*v.nz;
        float ny = x.m[3]*v.nx + x.m[4]*v.ny + x.m[5]*v.nz;
        float nz = x.m[6]*v.nx + x.m[7]*v.ny + x.m[8]*v.nz;

        if (ww >= 0.9995f) {                 // verteklarning ko'pi shu tez yo'ldan
            d[i].px = ax; d[i].py = ay; d[i].pz = az;
        } else {
            // CHOK: dam olish pozasi bilan emas, OTA SUYAK bilan aralashtiramiz.
            // Ilgari `v + (a - v) * ww` yozilgandi — og'irligi kamaygan verteks
            // hech qayerga ketmasdi. Bosh 0.86..1.00 oralig'ida yotadi va uning
            // pastki yarmi atigi 0.45 og'irlik olardi: zarba yeganda bosh
            // AYLANMASDAN QIRQILARDI (shear) — "boshi qiyshayadi".
            // Ota bilan aralashtirish esa klassik ikki suyakli skinlash:
            // bo'yin silliq egiladi, bosh QATTIQ jism bo'lib qoladi.
            const int par = kBone[b].parent;
            const Xform& x2 = xf[(par >= 0 && par < BCOUNT) ? par : b];
            float bx, by, bz, mx, my, mz;
            if (x2.ident) {
                bx = v.px; by = v.py; bz = v.pz;
                mx = v.nx; my = v.ny; mz = v.nz;
            } else {
                bx = x2.m[0]*v.px + x2.m[1]*v.py + x2.m[2]*v.pz + x2.t[0];
                by = x2.m[3]*v.px + x2.m[4]*v.py + x2.m[5]*v.pz + x2.t[1];
                bz = x2.m[6]*v.px + x2.m[7]*v.py + x2.m[8]*v.pz + x2.t[2];
                mx = x2.m[0]*v.nx + x2.m[1]*v.ny + x2.m[2]*v.nz;
                my = x2.m[3]*v.nx + x2.m[4]*v.ny + x2.m[5]*v.nz;
                mz = x2.m[6]*v.nx + x2.m[7]*v.ny + x2.m[8]*v.nz;
            }
            d[i].px = bx + (ax - bx) * ww;
            d[i].py = by + (ay - by) * ww;
            d[i].pz = bz + (az - bz) * ww;
            nx = mx + (nx - mx) * ww;
            ny = my + (ny - my) * ww;
            nz = mz + (nz - mz) * ww;
            const float l2 = nx*nx + ny*ny + nz*nz;
            if (l2 > 1e-12f) {
                const float inv = 1.0f / std::sqrt(l2);
                nx *= inv; ny *= inv; nz *= inv;
            }
        }
        d[i].nx = nx; d[i].ny = ny; d[i].nz = nz;
        // u/v o'zgarmaydi â€” dst init() da src dan nusxa olingan
    }
}

// ---------------------------------------------------------------------------
//  Bir martalik (one-shot) kliplar uchun yordamchi jadval.
//  Header'da qo'shimcha maydon yo'q, shuning uchun bir martalik klipning
//  (Draw, Vault, Mantle, Slide, Roll, Dodge, Assassinate) boshlanish vaqti shu
//  yerda saqlanadi - faqat ayni damda shunday holatdagi modellar uchun.
// ---------------------------------------------------------------------------

std::map<const void*, float>& oneShotTable() {
    static std::map<const void*, float> m;
    return m;
}

void oneShotSet(const void* k, float t) {
    std::map<const void*, float>& m = oneShotTable();
    if (m.size() > 1024u) m.clear();          // cheksiz o'sishdan himoya
    m[k] = t;
}

float oneShotGet(const void* k, float def) {
    std::map<const void*, float>& m = oneShotTable();
    std::map<const void*, float>::const_iterator it = m.find(k);
    return (it == m.end()) ? def : it->second;
}

void oneShotErase(const void* k) {
    std::map<const void*, float>& m = oneShotTable();
    if (!m.empty()) m.erase(k);
}

void oneShotShift(const void* k, float d) {
    std::map<const void*, float>& m = oneShotTable();
    std::map<const void*, float>::iterator it = m.find(k);
    if (it != m.end()) it->second += d;
}

// ---------------------------------------------------------------------------
//  Skinlash tezligi (barcha modellar uchun umumiy)
// ---------------------------------------------------------------------------

float g_skinPeriod = 1.0f / 30.0f;

} // anonim namespace

// ---------------------------------------------------------------------------
//  Klip nomlari
// ---------------------------------------------------------------------------

const char* animClipName(AnimClip c) {
    switch (c) {
        case AnimClip::Idle:   return "Idle";
        case AnimClip::Walk:   return "Walk";
        case AnimClip::Run:    return "Run";
        case AnimClip::Talk:   return "Talk";
        case AnimClip::Listen: return "Listen";
        case AnimClip::Point:  return "Point";
        case AnimClip::Salute: return "Salute";
        case AnimClip::Draw:   return "Draw";
        case AnimClip::Sit:    return "Sit";

        case AnimClip::CrouchIdle:  return "CrouchIdle";
        case AnimClip::CrouchWalk:  return "CrouchWalk";
        case AnimClip::Vault:       return "Vault";
        case AnimClip::Mantle:      return "Mantle";
        case AnimClip::ClimbUp:     return "ClimbUp";
        case AnimClip::Hang:        return "Hang";
        case AnimClip::Shimmy:      return "Shimmy";
        case AnimClip::Slide:       return "Slide";
        case AnimClip::Roll:        return "Roll";
        case AnimClip::Fall:        return "Fall";
        case AnimClip::WallRun:     return "WallRun";
        case AnimClip::Dodge:       return "Dodge";
        case AnimClip::Assassinate: return "Assassinate";

        case AnimClip::AttackLight1: return "AttackLight1";
        case AnimClip::AttackLight2: return "AttackLight2";
        case AnimClip::AttackLight3: return "AttackLight3";
        case AnimClip::AttackHeavy:  return "AttackHeavy";
        case AnimClip::KickClip:     return "Kick";
        case AnimClip::Block:        return "Block";
        case AnimClip::ParryHit:     return "ParryHit";
        case AnimClip::Hurt:         return "Hurt";
        case AnimClip::Stagger:      return "Stagger";
        case AnimClip::Death:        return "Death";
        case AnimClip::BowAim:       return "BowAim";
        case AnimClip::BowShoot:     return "BowShoot";
        case AnimClip::Execute:      return "Execute";

        default:               return "Idle";
    }
}

namespace {
// Registrga sezgir bo'lmagan solishtirish (fayl doirasida, ASCII uchun yetarli)
bool nameEquals(const std::string& s, const char* n) {
    if (n == nullptr || s.size() != std::strlen(n)) return false;
    for (size_t k = 0; k < s.size(); ++k) {
        char a = s[k], b = n[k];
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}
} // anonim namespace

AnimClip animClipFromName(const std::string& s) {
    // Registrga sezgir emas; topilmasa Idle
    for (int i = 0; i < static_cast<int>(AnimClip::Count); ++i) {
        const AnimClip c = static_cast<AnimClip>(i);
        if (nameEquals(s, animClipName(c))) return c;
    }
    // Taxalluslar: enum a'zosi nomi bilan yozilgan JSON ma'lumotlari uchun
    if (nameEquals(s, "KickClip"))                              return AnimClip::KickClip;
    if (nameEquals(s, "Blocking") || nameEquals(s, "Guard"))    return AnimClip::Block;
    if (nameEquals(s, "Parry")    || nameEquals(s, "Parrying")) return AnimClip::ParryHit;
    if (nameEquals(s, "Die")      || nameEquals(s, "Dead"))     return AnimClip::Death;
    if (nameEquals(s, "Attack")   || nameEquals(s, "AttackLight")) return AnimClip::AttackLight1;
    return AnimClip::Idle;
}

// ---------------------------------------------------------------------------
//  SkinnedModel
// ---------------------------------------------------------------------------

bool SkinnedModel::init(Mesh* mesh) {
    mesh_ = nullptr;
    skinned_.clear();
    boneIdx_.clear();
    boneW_.clear();
    clip_ = prevClip_ = AnimClip::Idle;
    time_ = 0.0f; blend_ = 1.0f; blendDur_ = 0.25f;
    speedScale_ = 1.0f; talk_ = 0.0f; accum_ = 0.0f;
    modelHeight_ = 1.0f;
    dirty_ = true;
    simpleRig_ = false;
    phase_ = 0.0f; lastSkinPh_ = 0.0f;
    locoSpeed_ = 0.0f; locoDs_ = -1.0f; bodyM_ = 1.82f;
    bankDeg_ = leanDeg_ = headLead_ = 0.0f;
    moveAngle_ = turnStepHz_ = 0.0f;
    phaseDir_ = 1.0f;
    hitDir_ = 0.0f; hitW_ = 1.0f;
    oneShotErase(this);

    if (mesh == nullptr || !mesh->valid()) return false;

    const std::vector<MeshVertex>& src = mesh->vertices();
    if (src.empty()) return false;

    mesh_ = mesh;

    // --- Past-poligonli model himoyasi ---
    // 200-500 verteksli modelda quti bo'yicha rigging a'zolarni ajratib,
    // meshni yirtib yuboradi (bir uchburchak bir vaqtda ikki suyakka tegadi).
    // Bunday modellarda faqat ildiz/tana harakati qo'llanadi.
    simpleRig_ = (src.size() < 2000u);

    const Vec3 mn = mesh->bbMin();
    const Vec3 mx = mesh->bbMax();
    const float h     = std::max(1e-6f, mx.y - mn.y);
    const float halfW = std::max(1e-6f, (mx.x - mn.x) * 0.5f);
    const float cx    = (mn.x + mx.x) * 0.5f;
    modelHeight_ = h;

    // --- BO'YINNI MESHDAN O'LCHASH ---
    // Bosh chegarasi ilgari qat'iy 0.86*h edi. Bu ottoman.obj ga to'g'ri keladi,
    // lekin crusader.obj da haqiqiy bo'yin 0.90 da — natijada bosh suyagiga
    // yelka usti ham kirib, bosh o'z ICHIDAGI nuqta atrofida aylanardi.
    //
    // DIQQAT: shunchaki "eng tor kesim" ISHLAMAYDI — u doim salla/dubulg'a UCHI
    // bo'lib chiqadi (u yerda kenglik nolga intiladi). To'g'ri belgi — pastdan
    // yuqoriga qarab birinchi LOKAL MINIMUM: bo'yin torayadi, keyin bosh yana
    // kengayadi. Tekshirilgan: ottoman -> 0.86, crusader -> 0.90.
    {
        const int   kLo = 36, kHi = 48;          // 2% li chiziqlar: 0.72 .. 0.96
        const int   kN  = kHi - kLo;
        float lo[kN], hi2[kN], loZ[kN], hiZ[kN], wid[kN];
        int   cnt[kN];
        for (int i = 0; i < kN; ++i) { cnt[i] = 0; wid[i] = 0.0f; }

        for (size_t i = 0; i < src.size(); ++i) {
            const float yn = (src[i].py - mn.y) / h;
            const int   k  = (int)(yn * 50.0f) - kLo;
            if (k < 0 || k >= kN) continue;
            if (cnt[k] == 0) {
                lo[k] = hi2[k] = src[i].px;
                loZ[k] = hiZ[k] = src[i].pz;
            } else {
                if (src[i].px < lo[k])  lo[k]  = src[i].px;
                if (src[i].px > hi2[k]) hi2[k] = src[i].px;
                if (src[i].pz < loZ[k]) loZ[k] = src[i].pz;
                if (src[i].pz > hiZ[k]) hiZ[k] = src[i].pz;
            }
            ++cnt[k];
        }
        for (int i = 0; i < kN; ++i) {
            if (cnt[i] < 30) { wid[i] = 1.0e9f; continue; }   // shovqindan himoya
            // FAQAT X kengligi: Z yoyilishiga plash, o'q sadog'i va yelka
            // bezaklari qo'shiladi va bo'yin minimumini yashiradi.
            // Tekshirildi: X bilan ottoman -> 0.86, crusader -> 0.90.
            wid[i] = (hi2[i] - lo[i]) / h;
        }

        // Yelka kengligi — nisbat chegarasi uchun. Ko'krak/bel ham lokal minimum
        // berishi mumkin (ottoman'da 0.74 da), lekin u yelkadan atigi 20% tor.
        // Haqiqiy bo'yin esa yelkadan kamida ikki baravar tor bo'ladi.
        float shoulder = 0.0f;
        for (size_t i = 0; i < src.size(); ++i) {
            const float yn = (src[i].py - mn.y) / h;
            if (yn < 0.58f || yn > 0.80f) continue;
            const float dx = std::fabs(src[i].px - cx) * 2.0f / h;
            if (dx > shoulder) shoulder = dx;
        }
        const float need = (shoulder > 0.05f) ? shoulder * 0.55f : 0.25f;

        int   best  = -1;
        float bestW = 0.0f;
        for (int i = 1; i < kN - 1; ++i) {
            if (wid[i] > 1.0e8f) continue;
            if (wid[i] >= need) continue;                    // hali yelka/ko'krak
            if (wid[i] < wid[i-1] && wid[i] < wid[i+1]) { best = i; bestW = wid[i]; break; }
        }
        neckN_ = (best >= 0) ? ((float)(best + kLo) / 50.0f) : 0.86f;
        neckN_ = clampf(neckN_, 0.76f, 0.93f);
        static const bool rigLog = (std::getenv("ERT_RIG_LOG") != nullptr);
        if (rigLog)
            std::printf("[rig] bo'yin = %.2f (kenglik %.3f, yelka %.3f, chegara %.3f), verteks %d\n",
                        neckN_, bestW, shoulder, need, (int)src.size());
    }

    // Bir martalik ajratish â€” keyin har kadrda hech narsa ajratilmaydi
    skinned_ = src;
    boneIdx_.resize(src.size());
    boneW_.resize(src.size());

    for (size_t i = 0; i < src.size(); ++i) {
        const float yn = (src[i].py - mn.y) / h;        // 0 = oyoq, 1 = bosh
        const float xn = (src[i].px - cx)  / halfW;     // -1 = chap, +1 = o'ng
        const float ax = std::fabs(xn);
        const bool  left = (xn < 0.0f);

        int b;
        if      (yn < 0.08f)  b = left ? B_LLL : B_LLR;   // oyoq panjasi
        else if (yn < 0.28f)  b = left ? B_LLL : B_LLR;   // boldir
        else if (yn < 0.50f)  b = left ? B_LUL : B_LUR;   // son
        else if (yn < 0.58f)  b = B_PELVIS;
        else if (yn < 0.82f && ax > 0.55f)
                              b = (yn > 0.70f) ? (left ? B_AUL : B_AUR)
                                               : (left ? B_ALL_ : B_ALR);
        else if (yn < neckN_) b = B_TORSO;
        else                  b = B_HEAD;

        // --- Chegaralarda yumshoq o'tish ---
        // Suyak chegarasiga 0.05 (normallashtirilgan balandlik) yaqinlikda og'irlik chiziqli kamayadi,
        // shunda deformatsiya keskin uzilmaydi. To'liq muzlab qolmasligi uchun quyi chegara 0.45.
        float w = 1.0f;
        w = std::min(w, saturate(std::fabs(yn - 0.28f) / 0.05f));   // tizza
        w = std::min(w, saturate(std::fabs(yn - 0.50f) / 0.05f));   // son / chanoq
        w = std::min(w, saturate(std::fabs(yn - 0.58f) / 0.05f));   // chanoq / ko'krak
        // Bo'yin choki: endi og'irlik OTA SUYAK (ko'krak) bilan aralashadi, shuning
        // uchun band ancha tor bo'lishi mumkin — bosh qattiq jism bo'lib qoladi.
        w = std::min(w, saturate(std::fabs(yn - neckN_) / 0.025f));  // ko'krak / bosh
        if (yn >= 0.58f && yn < 0.90f && ax > 0.45f)
            w = std::min(w, saturate(std::fabs(yn - 0.82f) / 0.05f));      // yelka / tirsak
        if (yn >= 0.55f && yn <= 0.90f)
            w = std::min(w, saturate(std::fabs(ax - 0.55f) / 0.10f));      // qo'l / tana
        if (isPaired(b))
            w = std::min(w, saturate(ax / 0.10f));                          // chap / o'ng o'rta chiziq

        // Quyi chegara 0.45 dan 0.15 ga tushirildi: endi og'irlik ILDIZ bilan emas,
        // OTA SUYAK bilan aralashadi, ya'ni past og'irlik "muzlab qolish" emas,
        // "ota bilan birga harakatlanish" degani — chok ancha tabiiy chiqadi.
        w = 0.15f + 0.85f * smoothstepf(w);

        boneIdx_[i] = static_cast<unsigned char>(b);
        boneW_[i]   = w;
    }

    return true;
}

void SkinnedModel::setClip(AnimClip c, float blendTime) {
    if (static_cast<int>(c) < 0 || static_cast<int>(c) >= static_cast<int>(AnimClip::Count))
        c = AnimClip::Idle;

    // --- Bir martalik parkur/jang klipini himoya qilish ---
    // driveByLocomotion() o'zgartirilmagan (u faqat Idle/Walk/Run bilan ishlaydi),
    // shuning uchun uzilishdan himoya SHU YERDA: Vault/Mantle/Slide/Roll/Dodge/
    // Assassinate tugamaguncha siklik klip so'rovi e'tiborsiz qoldiriladi.
    // Boshqa bir martalik klip esa uza oladi (masalan Vault -> Roll zanjiri).
    if (isProtectedOneShot(clip_) && c != clip_ && !isOneShotClip(c)) {
        const float dur = oneShotDuration(clip_);
        const float lt  = time_ - oneShotGet(this, time_);
        if (lt >= 0.0f && lt < dur) return;
    }

    // Ushlab turiladigan jang holatlari (Block, BowAim) va Death siklik bo'lgani
    // uchun yuqoridagi himoya ularni qamramaydi. driveByLocomotion() ularni
    // buzmasligi uchun FAQAT Walk/Run so'rovlari e'tiborsiz qoldiriladi -
    // Idle yoki boshqa har qanday klip bilan chiqish ochiq qoladi.
    if (isHeldCombatClip(clip_) && (c == AnimClip::Walk || c == AnimClip::Run)) return;

    // MUHIM: bu yerda bir martalik klipni QAYTA BOSHLAMAYMIZ.
    // Character::update() va Enemy::update() animatsiya klipini HAR KADR
    // so'raydi. Ilgari shu joyda oneShotSet(this, time_) turardi — ya'ni
    // klipning mahalliy vaqti har kadr nolga qaytardi va BARCHA jang
    // animatsiyalari (zarba, zarba yeyish, muvozanat buzilishi, yakunlovchi
    // zarba) birinchi kadrida MUZLAB qolardi. Qayta boshlash endi faqat
    // playClip() orqali — ya'ni yangi harakat boshlanganda.
    if (c == clip_) return;

    prevClip_ = clip_;
    clip_     = c;
    blendDur_ = (blendTime > 0.01f) ? blendTime : 0.01f;
    blend_    = 0.0f;
    if (isOneShotClip(c)) oneShotSet(this, time_);
    dirty_    = true;
}

// Klipni boshidan ishga tushiradi. setClip() dan farqi: shu klip allaqachon
// o'ynayotgan bo'lsa ham mahalliy vaqtni nolga qaytaradi.
void SkinnedModel::playClip(AnimClip c, float blendTime) {
    if (static_cast<int>(c) < 0 || static_cast<int>(c) >= static_cast<int>(AnimClip::Count))
        c = AnimClip::Idle;
    if (c == clip_) {
        if (isOneShotClip(c)) { oneShotSet(this, time_); dirty_ = true; }
        return;
    }
    setClip(c, blendTime);
    if (isOneShotClip(c)) oneShotSet(this, time_);
}

void SkinnedModel::update(float dt) {
    if (mesh_ == nullptr) return;

    // TASHXIS (ERT_FORCE_CLIP=<nom>): bitta pozani muzlatib ushlab turadi.
    // Poza nuqsonlarini ko'z bilan tekshirish uchun (masalan Hurt, Stagger).
    {
        static const char* const force = std::getenv("ERT_FORCE_CLIP");
        if (force != nullptr && *force != 0) {
            const AnimClip fc = animClipFromName(force);
            if (clip_ != fc) { clip_ = prevClip_ = fc; blend_ = 1.0f; oneShotSet(this, time_); }
            // bir martalik klip qayta-qayta boshlanib tursin
            if (isOneShotClip(fc)) {
                const float lt = time_ - oneShotGet(this, time_);
                if (lt > oneShotDuration(fc)) oneShotSet(this, time_);
            }
            dirty_ = true;
        }
    }

    // NaN / manfiy / juda katta dt dan himoya (masalan oyna ko'chirilganda)
    if (!(dt > 0.0f)) dt = 0.0f;
    if (dt > 0.25f)   dt = 0.25f;

    time_ += dt;
    // float aniqligini saqlash uchun vaqtni davriy ravishda o'raymiz
    if (time_ > 1000.0f) {
        time_ -= 1000.0f;
        oneShotShift(this, -1000.0f);
    }

    // --- Qadam FAZASI: t*rate ko'paytmasi EMAS, MASOFA integrali ---
    // Eski formulada dPh = TAU*t*dRate: t = 120 s da rate ning 0.02 Hz o'zgarishi
    // fazani 2.4 siklga sakratardi. Masofa integrali esa devorga tiralganda
    // (ds = 0) oyoqni ham to'xtatadi.
    {
        const float bm = (bodyM_ > 0.3f) ? bodyM_ : 1.82f;
        float S = strideLenM(locoSpeed_, bm);
        if (clip_ == AnimClip::CrouchWalk) S *= 0.62f;
        const float ds = (locoDs_ >= 0.0f) ? locoDs_ : locoSpeed_ * dt;

        if (locoSpeed_ < 0.15f && turnStepHz_ > 0.05f) {
            // Joyida burilish: ds ~ 0, lekin oyoq mayda qadam tashlaydi
            phase_ += turnStepHz_ * dt * phaseDir_;
        } else if (locoSpeed_ < 0.15f) {
            // FOOT PLANT: eng yaqin o'rta-tayanch holatiga (0.25 / 0.75) yopamiz -
            // u yerda panjalar yonma-yon, ya'ni Idle ga tabiiy o'tish.
            const float tgt = (phase_ < 0.5f) ? 0.25f : 0.75f;
            float d = tgt - phase_;
            d -= std::floor(d + 0.5f);
            phase_ += d * (1.0f - std::exp(-9.0f * dt));
        } else if (S > 1e-4f && ds > 0.0f) {
            phase_ += (ds / S) * phaseDir_;
        }
        phase_ -= std::floor(phase_);
        if (!(phase_ >= 0.0f && phase_ < 1.0f)) phase_ = 0.0f;   // NaN himoyasi
        locoDs_ = -1.0f;                                          // bir kadr = bir marta
    }

    if (blend_ < 1.0f) {
        blend_ += (blendDur_ > 1e-4f) ? (dt / blendDur_) : 1.0f;
        if (blend_ > 1.0f) blend_ = 1.0f;
        // dirty_ ni bu yerda YOQMAYMIZ: aralashtirish paytida ham skinlash tezligi
        // cheklovi kuchda qoladi (30 Hz o'tish uchun yetarli darajada silliq).
    }

    // Bir martalik klip tugagach yordamchi yozuvni tozalaymiz
    if (!isOneShotClip(clip_) && !isOneShotClip(prevClip_)) oneShotErase(this);

    // --- Skinlash tezligini cheklash ---
    accum_ += dt;
    // Faza sezilarli siljisa 30 Gts jadvalini kutmaymiz: sprintda 2.0 Hz sikl
    // 30 Gts da atigi 15 namuna beradi (past sifatda 10) - oyoq diskret sakraydi.
    // 0.05 sikl ~ 18 gradus son burilishi; yuqori chegara 45 Gts (CPU narxi).
    {
        float dph = phase_ - lastSkinPh_;
        dph -= std::floor(dph);
        if (dph > 0.5f) dph = 1.0f - dph;
        if (dph > 0.05f && accum_ >= (1.0f / 45.0f)) dirty_ = true;
        // Bir martalik klip (zarba, Hurt, parkur) — poza tez o'zgaradi, 30 Gts kam.
        if (isOneShotClip(clip_) && accum_ >= (1.0f / 45.0f)) dirty_ = true;
    }
    if (!dirty_ && accum_ < g_skinPeriod) return;      // oraliqda oxirgi natija qayta ishlatiladi
    if (accum_ > 1.0f) accum_ = 0.0f; else accum_ -= g_skinPeriod;
    if (accum_ < 0.0f) accum_ = 0.0f;
    dirty_ = false;
    lastSkinPh_ = phase_;

    if (boneIdx_.size() != mesh_->vertexCount()) return;   // init() bajarilmagan â€” zaxira yo'l

    // --- Pozani hisoblaymiz (kerak bo'lsa oldingi klip bilan aralashtiramiz) ---
    const float oneLocal = time_ - oneShotGet(this, time_);
    Pose cur, res;
    const StrideCtx sc_ = buildStrideCtx();
    evalPose(clip_, time_, speedScale_, talk_, oneLocal, sc_, cur);
    if (blend_ < 1.0f) {
        Pose prv;
        evalPose(prevClip_, time_, speedScale_, talk_, oneLocal, sc_, prv);
        lerpPose(prv, cur, smoothstepf(blend_), res);
    } else {
        res = cur;
    }

    // Past-poligonli model: quti bo'yicha rigging meshni yirtmasligi uchun
    // a'zolar aylanmaydi, faqat ildiz/tana harakati qoladi (skinAll dan OLDIN).
    if (simpleRig_) simplifyPose(res);

    Vec3 pivot[BCOUNT];
    buildPivots(mesh_->bbMin(), mesh_->bbMax(), pivot, neckN_);
    Xform xf[BCOUNT];
    computeXforms(res, pivot, modelHeight_, xf);

    skinAll(mesh_->vertices(), boneIdx_, boneW_, xf, skinned_);

    // Diagnostika (ERT_SKIN_STATS=1): har ~2 sekundda bir marta siljishlarni chop etadi
    {
        static const bool stats = (std::getenv("ERT_SKIN_STATS") != nullptr);
        if (stats) {
            static float acc = 0.0f;
            acc += dt;
            if (acc > 2.0f) {
                acc = 0.0f;
                std::printf("[clip=%s oneLocal=%.3f/%.2f] ", animClipName(clip_),
                            oneLocal, oneShotDuration(clip_));
                skinStats(mesh_->vertices(), skinned_, boneIdx_, modelHeight_);
            }
        }
    }
}

StrideCtx SkinnedModel::buildStrideCtx() const {
    StrideCtx sc;
    const float v  = clampf(locoSpeed_, 0.0f, 12.0f);
    const float bm = (bodyM_ > 0.3f) ? bodyM_ : 1.82f;
    sc.phase   = phase_;
    sc.strideN = strideLenM(v, bm) / bm;
    sc.duty    = strideDuty(v, sc.strideN);
    sc.amp     = clampf(sc.strideN / 0.79f, 0.55f, 1.95f);   // 0.79 = 1.35 m/s etaloni
    sc.lean    = clampf(2.0f + 1.15f * v, 2.0f, 10.0f) + leanDeg_;
    sc.bank    = bankDeg_;
    sc.moveAngle = moveAngle_;
    sc.headLead  = headLead_;
    sc.slope     = clampf(slope_, -0.7f, 0.7f);
    // Qiyalikka egilish: tepaga chiqishda oldinga, tushishda orqaga (ko'krak ochiladi)
    sc.lean     += sc.slope * 12.0f * clampf(v / 1.2f, 0.0f, 1.0f);
    if (clip_ == AnimClip::CrouchWalk) { sc.strideN *= 0.62f; sc.lean += 10.0f; }
    sc.hitDir    = hitDir_;
    sc.hitWeight = hitW_;
    return sc;
}

void SkinnedModel::setHitDir(float dirDeg, float weight) {
    hitDir_ = std::isfinite(dirDeg) ? wrapAngleDeg(dirDeg) : 0.0f;
    hitW_   = std::isfinite(weight) ? clampf(weight, 0.35f, 1.0f) : 1.0f;
}

void SkinnedModel::setLocomotionPose(float bankDeg, float leanDeg, float headLeadDeg,
                                     float moveAngleDeg, float turnStepHz) {
    bankDeg_    = clampf(bankDeg,     -25.0f, 25.0f);
    leanDeg_    = clampf(leanDeg,     -15.0f, 18.0f);
    headLead_   = clampf(headLeadDeg, -30.0f, 30.0f);
    moveAngle_  = clampf(wrapAngleDeg(moveAngleDeg), -180.0f, 180.0f);
    turnStepHz_ = clampf(turnStepHz,    0.0f,  2.5f);
    if (!std::isfinite(bankDeg_))    bankDeg_    = 0.0f;
    if (!std::isfinite(leanDeg_))    leanDeg_    = 0.0f;
    if (!std::isfinite(headLead_))   headLead_   = 0.0f;
    if (!std::isfinite(moveAngle_))  moveAngle_  = 0.0f;
    if (!std::isfinite(turnStepHz_)) turnStepHz_ = 0.0f;
}

void SkinnedModel::setLocomotion(float speedMs, float dsMeters) {
    if (!(speedMs > 0.0f)) speedMs = 0.0f;
    if (speedMs > 20.0f)   speedMs = 20.0f;
    locoSpeed_ = speedMs;
    locoDs_    = (dsMeters >= 0.0f) ? clampf(dsMeters, 0.0f, 2.0f) : -1.0f;
    // Orqaga yurish: faza TESKARI aylanadi. speedScale ni manfiy qilish ishlamaydi -
    // evalPose da clampf(ss, 0.2, 3.0) uni 0.2 ga qisadi.
    phaseDir_ = (std::fabs(moveAngle_) > 120.0f) ? -1.0f : 1.0f;
}

void SkinnedModel::driveByLocomotion(float speedMs, float dt) {
    driveByLocomotion(speedMs, -1.0f, dt);          // eski chaqiruvchilar uchun
}

void SkinnedModel::driveByLocomotion(float speedMs, float dsMeters, float dt) {
    setLocomotion(speedMs, dsMeters);

    // Klip tanlash faqat NOM uchun (HUD, Cutscene st.clip, isHeldCombatClip himoyasi).
    // Geometriya Walk va Run da AYNI - chegarada pop YO'Q. Gisterezis: clip() ni
    // o'qiydigan Cutscene chirillamasin.
    if (locoSpeed_ < 0.20f && turnStepHz_ <= 0.05f) {
        if (clip_ == AnimClip::Walk || clip_ == AnimClip::Run) setClip(AnimClip::Idle, 0.25f);
    } else {
        const bool wasRun = (clip_ == AnimClip::Run);
        const bool run    = wasRun ? (locoSpeed_ > 2.55f) : (locoSpeed_ > 3.05f);
        setClip(run ? AnimClip::Run : AnimClip::Walk, 0.20f);
    }
    setSpeedScale(1.0f);     // qadam endi speedScale ga BOG'LIQ EMAS
    update(dt);              // Cutscene.cpp dagi animTime() shartiga tegmaymiz
}

void SkinnedModel::draw(const Vec3& pos, float yawDeg, float scale) {
    if (mesh_ == nullptr || !mesh_->valid()) return;

    // Skinlangan massiv tayyor emasmi â€” dam olish pozasi bilan chizamiz (halokat bo'lmasin)
    if (skinned_.size() != mesh_->vertexCount()) skinned_ = mesh_->vertices();

    const float h = (modelHeight_ > 1e-6f) ? modelHeight_ : 1.0f;
    const float s = ((scale > 1e-6f) ? scale : 1.0f) / h;   // model balandligi 'scale' metrga teng

    const Vec3 mn = mesh_->bbMin();
    const Vec3 mx = mesh_->bbMax();
    const float cx = (mn.x + mx.x) * 0.5f;
    const float cz = (mn.z + mx.z) * 0.5f;

    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    glScalef(s, s, s);
    glTranslatef(-cx, -mn.y, -cz);      // oyoq ostini koordinata boshiga keltiramiz
    // Diagnostika: ERT_NO_SKIN=1 -> deformatsiyasiz xom mesh (skinning artefaktlarini ajratish uchun)
    static const bool noSkin = (std::getenv("ERT_NO_SKIN") != nullptr);
    if (noSkin) mesh_->draw();
    else        mesh_->drawDeformed(skinned_);
    glPopMatrix();
}

Vec3 SkinnedModel::bonePosition(Bone b, const Vec3& pos, float yawDeg, float scale) const {
    if (mesh_ == nullptr || !mesh_->valid()) return pos;

    int bi = static_cast<int>(b);
    if (bi < 0 || bi >= BCOUNT) bi = B_ROOT;

    // Joriy (aralashtirilgan) poza â€” draw() bilan bir xil natija
    const float oneLocal = time_ - oneShotGet(this, time_);
    Pose cur, res;
    const StrideCtx sc_ = buildStrideCtx();
    evalPose(clip_, time_, speedScale_, talk_, oneLocal, sc_, cur);
    if (blend_ < 1.0f) {
        Pose prv;
        evalPose(prevClip_, time_, speedScale_, talk_, oneLocal, sc_, prv);
        lerpPose(prv, cur, smoothstepf(blend_), res);
    } else {
        res = cur;
    }
    if (simpleRig_) simplifyPose(res);      // update() bilan bir xil rejim

    const Vec3 mn = mesh_->bbMin();
    const Vec3 mx = mesh_->bbMax();
    Vec3 pivot[BCOUNT];
    buildPivots(mn, mx, pivot, neckN_);
    Xform xf[BCOUNT];
    computeXforms(res, pivot, (modelHeight_ > 1e-6f) ? modelHeight_ : 1.0f, xf);

    // Suyak tayanch nuqtasi model fazosida
    float lp[3];
    mat3apply(xf[bi].m, pivot[bi].x, pivot[bi].y, pivot[bi].z, lp);
    lp[0] += xf[bi].t[0];
    lp[1] += xf[bi].t[1];
    lp[2] += xf[bi].t[2];

    // draw() dagi transformatsiyaning aynan o'zi: markazlash -> masshtab -> yaw -> siljish
    const float h = (modelHeight_ > 1e-6f) ? modelHeight_ : 1.0f;
    const float s = ((scale > 1e-6f) ? scale : 1.0f) / h;
    const float cx = (mn.x + mx.x) * 0.5f;
    const float cz = (mn.z + mx.z) * 0.5f;

    const float ux = (lp[0] - cx)   * s;
    const float uy = (lp[1] - mn.y) * s;
    const float uz = (lp[2] - cz)   * s;

    const float r  = deg2rad(yawDeg);
    const float cy = std::cos(r), sy = std::sin(r);
    return Vec3{ pos.x + ( cy * ux + sy * uz),
                 pos.y + uy,
                 pos.z + (-sy * ux + cy * uz) };
}

void SkinnedModel::setSkinRateHz(float hz) {
    if (!(hz > 0.5f)) hz = 0.5f;
    if (hz > 240.0f)  hz = 240.0f;
    g_skinPeriod = 1.0f / hz;
}

} // namespace ert

