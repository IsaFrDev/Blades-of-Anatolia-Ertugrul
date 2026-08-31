// Assassin's Creed uslubidagi personaj holat mashinasi.
//
// Boshqaruv modeli — klassik AC "profil" tizimi:
//   PAST PROFIL  (Shift bosilmagan) : yurish / jog, jimgina, shovqin ~0.1
//   YUQORI PROFIL (Shift ushlangan) : yugurish + free-run, avtomatik parkur,
//                                     shovqin ~0.8
//
// Bu yerda haqiqiy fizika dvigateli yo'q: to'qnashuv — vertikal qutilar
// (PhysicsWorld), animatsiya — chegaraviy quti bo'yicha avtomatik rigging
// (SkinnedModel). Shuning uchun barcha parkur harakatlari "boshlanish nuqtasi ->
// tugash nuqtasi" orasida smoothstep bilan interpolyatsiya qilinadi.
#include "ertugrul/game/Character.h"
#include "ertugrul/world/Physics.h"
#include "ertugrul/gfx/Skin.h"

#include <vector>
#include <cmath>

namespace ert {

// ---------------------------------------------------------------------------
// Ichki yordamchilar
// ---------------------------------------------------------------------------
namespace {

constexpr float kGravity     = 22.0f;   // m/s^2
constexpr float kJumpVel     = 6.0f;    // sakrash boshlang'ich tezligi
constexpr float kAirControl  = 0.35f;   // havoda boshqaruv koeffitsiyenti
constexpr float kTurnRate    = 12.0f;   // yaw burilish tezligi (1/s)
constexpr float kAccel       = 12.0f;   // tezlanish damp lambda
constexpr float kDecel       = 16.0f;   // sekinlanish damp lambda
constexpr float kStepUp      = 0.45f;   // "zina" balandligi
constexpr float kShimmySpeed = 0.9f;    // osilgan holda yon siljish (m/s)
constexpr float kClimbSpeed  = 1.6f;    // devorga ko'tarilish (m/s)
constexpr float kHangDrop    = 1.75f;   // chekkadan oyoqqacha masofa
constexpr float kNoiseFade   = 2.0f;    // shovqin so'nishi (1/s)
constexpr float kFarAway     = 1.0e8f;  // "topilmadi" chegarasi
constexpr float kMaxFallVel  = 60.0f;

// --- Yer lokomotsiyasi (AC his-tuyg'usi). Eski kTurnRate/kAccel/kDecel
//     saqlanadi - ular parkur, havo va jang tarmoqlarida ishlatiladi. ---
constexpr float kAccelSprint = 4.5f;    // 90% ga 0.51 s - "og'ir start"
constexpr float kAccelJog    = 9.0f;    // 90% ga 0.26 s
constexpr float kAccelWalk   = 11.0f;   // 90% ga 0.21 s
constexpr float kDecelBase   = 6.0f;    // m/s^2 doimiy tormoz
constexpr float kStopWalk    = 0.30f;   // m - yurishdan to'xtash masofasi
constexpr float kStopSprint  = 1.50f;   // m - sprintdan to'xtash ("run-out")
constexpr float kLatAccelMax = 12.0f;   // m/s^2 (~1.2 g) -> sprintda radius 3.4 m
constexpr float kOmegaStand  = 540.0f;  // deg/s - joyida burilish
constexpr float kOmegaMin    = 110.0f;  // deg/s - sprintdagi eng past chegara
constexpr float kTurnGain    = 9.0f;
constexpr float kYawAccelLam = 14.0f;   // burchak TEZLANISHI silliqligi
constexpr float kPivotWin    = 0.25f;   // s - 180 gradus pivot oynasi
constexpr float kWalkRampT   = 0.28f;   // s - klaviaturada walk -> jog

// --- Kamon (XIII asr turk kompozit yoyi) ---
constexpr float kBowDraw       = 0.85f;  // to'la tortish vaqti (s) — zihgir bilan tez
constexpr float kBowDrawBreath = 6.5f;   // tortayotganda nafas (1/s)
constexpr float kBowHoldBreath = 11.0f;  // TO'LA tortib ushlaganda (1/s)
constexpr float kBowSilenceMul = 0.60f;  // «Sukunat» nafasni tinchlantiradi
constexpr float kBowShakeStart = 22.0f;  // shu nafasdan past — qo'l titray boshlaydi
constexpr float kBowShakeMaxDeg= 3.2f;   // eng katta tebranish (gradus)
constexpr float kBowRecover    = 0.35f;  // otgandan keyingi tiklanish (s)
constexpr float kBowAimSpeed   = 1.1f;   // nishonlab yurish (m/s)
constexpr float kBowAimTurn    = 14.0f;  // nishonlashda yaw kameraga yopishadi (1/s)
constexpr float kBowSpeedMin   = 30.0f;  // yarim tortilgan o'q (m/s)
constexpr float kBowSpeedMax   = 52.0f;  // to'la tortilgan

// NaN / cheksizlik / bema'ni katta qiymatlardan himoya
inline bool goodF(const float v) {
    return std::isfinite(v) && v > -kFarAway && v < kFarAway;
}
inline bool goodV(const Vec3& v) {
    return goodF(v.x) && goodF(v.y) && goodF(v.z);
}

// Oyoq ostidagi tayanch balandligi. Dunyo bo'lmasa — tekis nol sathi.
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

// Normalni ishonchli qilish: uzunligi yo'qolgan bo'lsa zaxira yo'nalish
Vec3 safeNormal(const Vec3& n, const Vec3& fallback) {
    if (!goodV(n)) return fallback;
    const Vec3 f{n.x, 0.0f, n.z};
    if (lengthSq(f) < 0.04f) return fallback;
    return normalize(f);
}

// --- Kiritish qirralarini (press) aniqlash ---
// Character.h kontrakt — unga maydon qo'sha olmaymiz, shuning uchun oldingi kadr
// tugmalari kichik yon jadvalda saqlanadi. Bu chaqiruvchi tugmani "bosilgan
// payt" (edge) yoki "ushlab turilgan" (level) sifatida bersa ham to'g'ri
// ishlashini kafolatlaydi: cho'kkalash almashuvi hech qachon titramaydi.
struct EdgeState {
    bool crouch = false, dodge = false, up = false, down = false, ass = false;
    bool bow = false;          // oldingi kadrda kamon tugmasi ushlanganmi
    // jang tugmalari (bosilgan payt aniqlash uchun)
    bool atkL = false, atkH = false, parry = false, kick = false;
    // oxirgi jang harakatidan (bergan yoki yegan zarbadan) beri o'tgan vaqt.
    // 3 sekunddan oshsa — "dam olish": nafas va poza tezroq tiklanadi.
    float restT = 9.0f;
    // kombo buferi: zarba tugamasdan bosilgan keyingi yengil zarba
    bool  queued = false;
};
struct EdgeSlot {
    const void* owner = nullptr;
    EdgeState   st;
};

std::vector<EdgeSlot>& edgeTable() {
    static std::vector<EdgeSlot> t;
    return t;
}

EdgeState& edgesFor(const void* owner) {
    std::vector<EdgeSlot>& t = edgeTable();
    for (size_t i = 0; i < t.size(); ++i)
        if (t[i].owner == owner) return t[i].st;
    if (t.size() >= 64) {          // cheksiz o'sishdan himoya
        t[0].owner = owner;
        t[0].st = EdgeState();
        return t[0].st;
    }
    EdgeSlot s;
    s.owner = owner;
    t.push_back(s);
    return t.back().st;
}

// Bir martalik (interpolyatsiyalanadigan) harakatmi?
bool isActionState(MoveState s) {
    switch (s) {
        case MoveState::Vault:
        case MoveState::Mantle:
        case MoveState::Slide:
        case MoveState::RollLand:
        case MoveState::Dodge:
        case MoveState::Assassinate:
        case MoveState::Eject:
        case MoveState::Land:
        case MoveState::WallRun:
            return true;
        default:
            return false;
    }
}

// Jang holati — taymer bilan tugaydi, kirish e'tiborsiz qoldiriladi.
// Bu holatlar parkur mantig'idan butunlay chetda ishlanadi.
bool isCombatLock(MoveState s) {
    switch (s) {
        case MoveState::AttackLight:
        case MoveState::AttackHeavy:
        case MoveState::KickState:
        case MoveState::Parrying:
        case MoveState::ParrySuccess:
        case MoveState::Hurt:
        case MoveState::Staggered:
        case MoveState::Executing:
        case MoveState::BowShoot:   // o'q uchdi — 0.35 s qulf
        case MoveState::Dead:
            return true;
        default:
            return false;
    }
}

// Zarba beradigan holatlar — consumeAttack faqat shularda ishlaydi
bool isStrikeState(MoveState s) {
    return s == MoveState::AttackLight || s == MoveState::AttackHeavy ||
           s == MoveState::KickState   || s == MoveState::Executing   ||
           s == MoveState::Assassinate;
}

// Holat nomlari (ingliz tilida — HUD va jurnal uchun)
const char* const kStateNames[] = {
    "Idle", "Walk", "Jog", "Sprint", "CrouchIdle", "CrouchWalk", "Slide",
    "Vault", "Mantle", "Climb", "Hang", "Shimmy", "Eject", "JumpUp",
    "Fall", "Land", "RollLand", "WallRun", "Dodge", "Assassinate", "LeapOfFaith",
    // jang
    "AttackLight", "AttackHeavy", "Kick", "Blocking", "Parrying",
    "ParrySuccess", "Hurt", "Staggered", "Executing",
    "BowAim", "BowShoot", "Dead"
};

// Tarjima kalitlari
const char* const kStateKeys[] = {
    "ui.state.idle", "ui.state.walk", "ui.state.jog", "ui.state.sprint",
    "ui.state.crouch_idle", "ui.state.crouch_walk", "ui.state.slide",
    "ui.state.vault", "ui.state.mantle", "ui.state.climb", "ui.state.hang",
    "ui.state.shimmy", "ui.state.eject", "ui.state.jump_up", "ui.state.fall",
    "ui.state.land", "ui.state.roll_land", "ui.state.wall_run",
    "ui.state.dodge", "ui.state.assassinate", "ui.state.leap_of_faith",
    // jang
    "ui.state.attack_light", "ui.state.attack_heavy", "ui.state.kick",
    "ui.state.blocking", "ui.state.parrying", "ui.state.parry_success",
    "ui.state.hurt", "ui.state.staggered", "ui.state.executing",
    "ui.state.bow_aim", "ui.state.bow_shoot", "ui.state.dead"
};

constexpr int kStateCount = static_cast<int>(MoveState::Count);
static_assert(sizeof(kStateNames) / sizeof(kStateNames[0]) == kStateCount,
              "kStateNames MoveState bilan mos emas");
static_assert(sizeof(kStateKeys) / sizeof(kStateKeys[0]) == kStateCount,
              "kStateKeys MoveState bilan mos emas");

} // anonim namespace

const char* moveStateName(MoveState s) {
    const int i = static_cast<int>(s);
    if (i < 0 || i >= kStateCount) return "Idle";
    return kStateNames[i];
}

// ---------------------------------------------------------------------------
// Hayot sikli
// ---------------------------------------------------------------------------
bool Character::init(SkinnedModel* model, PhysicsWorld* world) {
    // Ikkalasi ham ixtiyoriy: model bo'lmasa chizilmaydi, dunyo bo'lmasa
    // parkursiz tekis yurish rejimida ishlaydi.
    model_ = model;
    world_ = world;
    reset(pos_, yaw_);
    return true;
}

void Character::reset(const Vec3& feetPos, float yawDeg) {
    pos_ = goodV(feetPos) ? feetPos : Vec3{0.0f, 0.0f, 0.0f};
    yaw_ = targetYaw_ = goodF(yawDeg) ? yawDeg : 0.0f;
    velY_ = 0.0f;
    speed_ = 0.0f;
    state_ = MoveState::Idle;
    profile_ = Profile::Low;
    grounded_ = true;
    crouched_ = false;
    stateTime_ = 0.0f;
    actionDur_ = 0.0f;
    actionFrom_ = actionTo_ = pos_;
    actionFromYaw_ = actionToYaw_ = yaw_;
    fallStartY_ = pos_.y;
    coyote_ = 0.0f;
    jumpBuffer_ = 0.0f;
    hangNormal_ = Vec3{0.0f, 0.0f, 1.0f};
    climbT_ = 0.0f;
    noise_ = 0.0f;
    noiseEv_ = NoiseEvent();
    noiseEv_.pos = pos_;

    // jang holati (vitals/faith ga tegmaymiz — ularni revive() boshqaradi)
    lock_        = nullptr;
    combo_       = 0;
    comboWindow_ = 0.0f;
    strikeFired_ = false;
    pending_     = Attack();
    execTarget_  = pos_;
    execVictim_  = -1;
    animState_   = MoveState::Idle;
    animTime_    = -1.0f;

    // kamon
    drawT_ = aimBlend_ = shake_ = shakeT_ = 0.0f;
    camYaw_ = yaw_; camPitch_ = 0.0f;
    shotFired_ = true;                 // bo'sh buyurtma iste'mol qilinmasin
    pendingShot_ = BowShot();
    arrows_ = arrowsMax_;              // epizod boshida sadoq to'la

    // AC lokomotsiyasi holati
    vel_ = Vec3{0.0f, 0.0f, 0.0f};
    yawRate_ = 0.0f; prevSpeed_ = 0.0f;
    bank_ = 0.0f; lean_ = 0.0f;
    walkRamp_ = 0.0f; pivotT_ = 0.0f; turnStepHz_ = 0.0f;
    strideYaw_ = 0.0f; groundDs_ = 0.0f;
    visYOff_ = 0.0f; visYVel_ = 0.0f; prevPosY_ = pos_.y;

    // yerga o'tqazamiz
    const float gy = supportY(world_, pos_.x, pos_.z, pos_.y + 1.0f);
    if (goodF(gy) && std::fabs(gy - pos_.y) < 50.0f) pos_.y = gy;

    edgesFor(this) = EdgeState();
}

// ---------------------------------------------------------------------------
// Asosiy yangilanish
// ---------------------------------------------------------------------------
void Character::update(const CharacterInput& in, float dt) {
    using MS = MoveState;

    // --- 0) Xavfsizlik ---
    if (!goodF(dt) || dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;                 // sekin kadrlarda tunnel bo'lmasin
    if (!goodV(pos_))  pos_ = Vec3{0.0f, 0.0f, 0.0f};
    if (!goodF(yaw_))  yaw_ = 0.0f;
    if (!goodF(speed_) || speed_ < 0.0f) speed_ = 0.0f;
    if (!goodV(vel_)) vel_ = Vec3{0.0f, 0.0f, 0.0f};
    if (crouchSpeed_ <= 0.1f || crouchSpeed_ >= walkSpeed_) crouchSpeed_ = walkSpeed_ * 0.89f;
    if (!goodF(velY_)) velY_ = 0.0f;
    if (!goodF(fallStartY_)) fallStartY_ = pos_.y;
    if (walkSpeed_   <= 0.01f) walkSpeed_   = 1.6f;
    if (jogSpeed_    <= walkSpeed_) jogSpeed_ = walkSpeed_ + 0.1f;
    if (sprintSpeed_ <= jogSpeed_)  sprintSpeed_ = jogSpeed_ + 0.1f;
    if (radius_ <= 0.05f) radius_ = 0.42f;
    if (bodyH_  <= 0.3f)  bodyH_  = 1.82f;

    // --- 1) Kirishdan jahon yo'nalishi ---
    const float mx = goodF(in.move.x) ? clampf(in.move.x, -1.0f, 1.0f) : 0.0f;
    const float my = goodF(in.move.y) ? clampf(in.move.y, -1.0f, 1.0f) : 0.0f;
    const float camYaw = goodF(in.camYaw) ? in.camYaw : yaw_;

    const Vec3  fwd{std::sin(deg2rad(camYaw)), 0.0f, std::cos(deg2rad(camYaw))};
    const Vec3  right{-fwd.z, 0.0f, fwd.x};          // ekran o'ngi
    const Vec3  wishRaw = fwd * my + right * mx;
    const float wishLen = clampf(length(wishRaw), 0.0f, 1.0f);
    const Vec3  wish    = normalize(wishRaw);

    profile_ = in.highProfile ? Profile::High : Profile::Low;

    // tugma qirralari
    EdgeState& prev = edgesFor(this);
    const bool pCrouch = in.crouch      && !prev.crouch;
    const bool pDodge  = in.dodge       && !prev.dodge;
    const bool pUp     = in.parkourUp   && !prev.up;
    const bool pDown   = in.parkourDown && !prev.down;
    const bool pAss    = in.assassinate && !prev.ass;
    const bool pAtkL   = in.attackLight && !prev.atkL;
    const bool pAtkH   = in.attackHeavy && !prev.atkH;
    const bool pParry  = in.parry       && !prev.parry;
    const bool pKick   = in.kick        && !prev.kick;
    prev.crouch = in.crouch;
    prev.dodge  = in.dodge;
    prev.up     = in.parkourUp;
    prev.down   = in.parkourDown;
    prev.ass    = in.assassinate;
    const bool rBow = !in.bow && prev.bow;    // qo'yib yuborish -> OTISH
    prev.bow = in.bow;
    // Kamerani eslab qolamiz — o'q yo'nalishi ANA SHUNDAN quriladi, tanadan emas
    camYaw_   = goodF(in.camYaw)   ? in.camYaw : camYaw_;
    camPitch_ = goodF(in.camPitch) ? clampf(in.camPitch, -80.0f, 80.0f) : 0.0f;
    shakeT_  += dt;
    if (shakeT_ > 1000.0f) shakeT_ -= 1000.0f;
    prev.atkL   = in.attackLight;
    prev.atkH   = in.attackHeavy;
    prev.parry  = in.parry;
    prev.kick   = in.kick;

    // --- Shovqin so'nishi (yangi qiymatlar quyida ustidan yoziladi) ---
    noise_ = clampf(noise_ - kNoiseFade * dt, 0.0f, 1.0f);
    if (noiseEv_.life > 0.0f) {
        noiseEv_.life -= dt;
        if (noiseEv_.life < 0.0f) noiseEv_.life = 0.0f;
    }

    // Shovqin chiqarish: kuchliroq hodisa eskisini almashtiradi
    auto emitNoise = [&](float lvl, float radius) {
        lvl = saturate(lvl);
        if (lvl < noise_) return;
        noise_ = lvl;
        noiseEv_.pos    = pos_;
        noiseEv_.radius = clampf(radius, 0.0f, 40.0f);
        noiseEv_.life   = 0.6f;
    };

    // Bir martalik harakatni boshlash (from -> to interpolyatsiyasi)
    auto beginAction = [&](MS s, float dur, const Vec3& to, float toYaw,
                           float arc, bool groundDst) {
        Vec3 dst = goodV(to) ? to : pos_;
        if (groundDst && world_ != nullptr) {
            world_->resolve(dst, radius_, bodyH_);
            if (!goodV(dst)) dst = pos_;
            const float gy = supportY(world_, dst.x, dst.z, pos_.y + 0.6f);
            if (goodF(gy) && std::fabs(gy - pos_.y) < 2.0f) dst.y = gy;
        }
        actionFrom_    = pos_;
        actionTo_      = dst;
        actionFromYaw_ = yaw_;
        actionToYaw_   = goodF(toYaw) ? toYaw : yaw_;
        actionDur_     = (dur > 0.05f) ? dur : 0.05f;
        climbT_        = (arc > 0.0f && arc < 3.0f) ? arc : 0.0f;   // yoy balandligi
        stateTime_     = 0.0f;
        state_         = s;
        velY_          = 0.0f;
        speed_ = clampf(distanceXZ(actionFrom_, actionTo_) / actionDur_, 0.0f, 14.0f);
        // Harakat tugagach eski impuls qaytib personaj uchib ketmasin
        {
            const Vec3 dxz{actionTo_.x - actionFrom_.x, 0.0f, actionTo_.z - actionFrom_.z};
            vel_ = (lengthSq(dxz) > 1e-6f) ? normalize(dxz) * speed_ : dirFromYaw(yaw_) * speed_;
        }
        yawRate_ = 0.0f; pivotT_ = 0.0f; groundDs_ = 0.0f;
    };

    // Chekkaga osilib qolish
    auto grabLedge = [&](const Vec3& grab, const Vec3& normal, const Vec3& fallbackN) {
        const Vec3 n = safeNormal(normal, fallbackN);
        hangNormal_ = n;
        pos_ = Vec3{grab.x + n.x * (radius_ * 0.7f),
                    grab.y - kHangDrop,
                    grab.z + n.z * (radius_ * 0.7f)};
        yaw_ = targetYaw_ = yawFromDir(-n);
        state_      = MS::Hang;
        stateTime_  = 0.0f;
        velY_       = 0.0f;
        speed_      = 0.0f;
        grounded_   = false;
    };

    // Zarbani boshlash. Nafas yetmasa umuman boshlanmaydi (GDD: jang og'ir).
    // Zarba paytida personaj biroz OLDINGA siljiydi — qulflangan nishon tomon.
    auto startAttack = [&](DamageType type, MS ms, int comboIdx) -> bool {
        if (comboIdx < 0 || comboIdx > 2) comboIdx = 0;
        const Attack& a = attackDef(type, comboIdx);
        if (!vitals.spendBreath(a.breathCost)) return false;

        pending_     = a;
        strikeFired_ = false;
        combo_       = comboIdx;
        comboWindow_ = 0.0f;

        float dur = a.duration();
        if (!goodF(dur) || dur < 0.12f) dur = 0.5f;      // jadval bo'sh bo'lsa ham ishlasin
        actionDur_ = dur;
        stateTime_ = 0.0f;

        // Yo'nalish: qulf bo'lsa nishonga, aks holda qaragan tomonga
        Vec3 dir = dirFromYaw(yaw_);
        float tdist = kFarAway;
        if (lock_ != nullptr && goodV(*lock_)) {
            const Vec3 d{lock_->x - pos_.x, 0.0f, lock_->z - pos_.z};
            if (lengthSq(d) > 0.04f) {
                dir   = normalize(d);
                tdist = length(d);
            }
        }

        // Oldinga siljish: yengil 0.50/0.65/0.80, tepish 0.70, og'ir 0.90 m
        float lunge = 0.5f + 0.15f * static_cast<float>(comboIdx);
        if (type == DamageType::HeavyAttack) lunge = 0.9f;
        else if (type == DamageType::Kick)   lunge = 0.7f;
        if (tdist < kFarAway) {                          // nishon ichiga kirib ketmaylik
            const float room = tdist - 1.05f;
            if (room < lunge) lunge = (room > 0.0f) ? room : 0.0f;
        }
        lunge = clampf(lunge, 0.0f, 0.9f);

        actionFrom_    = pos_;
        actionTo_      = pos_ + dir * lunge;
        actionFromYaw_ = yaw_;
        actionToYaw_   = yawFromDir(dir);
        climbT_        = 0.0f;
        speed_         = 0.0f;
        velY_          = 0.0f;
        grounded_      = true;
        state_         = ms;
        prev.queued    = false;
        prev.restT     = 0.0f;
        emitNoise((type == DamageType::HeavyAttack) ? 0.55f : 0.40f, 8.0f);
        return true;
    };

    // =======================================================================
    // 1.5) JANG YADROSI — resurslar, o'lim, muvozanat
    //      Jang parkurdan USTUN: bu blok holat mashinasidan OLDIN ishlaydi.
    // =======================================================================
    // "Dam olish" — oxirgi 3 sekundda na zarba berdik, na yedik
    if (inCombatState()) {
        prev.restT = 0.0f;
    } else {
        if (!goodF(prev.restT) || prev.restT < 0.0f) prev.restT = 0.0f;
        if (prev.restT < 30.0f) prev.restT += dt;
    }
    const bool resting = (prev.restT >= 3.0f) && !inCombatState();
    // Iymon darajasi nafas va poza tiklanishiga ta'sir qiladi (GDD 04)
    faith.update(dt);
    vitals.update(dt, resting, faith.breathRegenBonus(), faith.postureRegenBonus());

    // Kombo oynasi so'nadi
    if (combo_ < 0 || combo_ > 2) combo_ = 0;
    if (!goodF(comboWindow_)) comboWindow_ = 0.0f;
    if (comboWindow_ > 0.0f) {
        comboWindow_ -= dt;
        if (comboWindow_ <= 0.0f) { comboWindow_ = 0.0f; combo_ = 0; }
    }

    // O'lim — undan keyin hech qanday kirish qabul qilinmaydi
    if (state_ != MS::Dead && vitals.health <= 0.0f) kill();

    // Muvozanat buzilishi hujumdan ham, parkurdan ham ustun
    if (state_ != MS::Dead && vitals.staggered) {
        if (state_ != MS::Staggered) {
            state_         = MS::Staggered;
            stateTime_     = 0.0f;
            speed_         = 0.0f;
            strikeFired_   = true;               // boshlangan zarba bekor bo'ladi
            combo_         = 0;
            comboWindow_   = 0.0f;
            prev.queued    = false;
            prev.restT     = 0.0f;
            actionFrom_    = actionTo_ = pos_;
            actionFromYaw_ = actionToYaw_ = yaw_;
            actionDur_     = clampf(vitals.staggerT, 0.6f, 3.0f);
        }
    }

    const bool combatLock = isCombatLock(state_);

    const Vec3 faceDir = dirFromYaw(yaw_);
    const MS   st = state_;

    // =======================================================================
    // 2) HOLATGA QARAB TARMOQLANISH
    // =======================================================================
    if (combatLock) {
        // ---------------- JANG: bir martalik holatlar ----------------
        // Kirish butunlay e'tiborsiz. Harakat faqat oldindan hisoblangan
        // actionFrom_ -> actionTo_ oralig'ida (hujumda oldinga, zarba yeganda
        // orqaga). Bu Kingdom Come dagi "og'irlik" hissini beradi.
        stateTime_ += dt;
        const float dur = (actionDur_ > 1e-4f) ? actionDur_ : 0.05f;
        const float t   = saturate(stateTime_ / dur);

        const bool glide = (st == MS::AttackLight || st == MS::AttackHeavy ||
                            st == MS::KickState   || st == MS::Hurt);
        if (glide) {
            // hujumda siljish windup+active ichida tugaydi, itarilish esa silliq
            const float k = (st == MS::Hurt) ? smoothstepf(t)
                                             : smoothstepf(saturate(t / 0.55f));
            const Vec3 prevPos = pos_;
            const float nx = lerpf(actionFrom_.x, actionTo_.x, k);
            const float nz = lerpf(actionFrom_.z, actionTo_.z, k);
            if (goodF(nx) && goodF(nz)) { pos_.x = nx; pos_.z = nz; }
            if (world_ != nullptr) world_->resolve(pos_, radius_, bodyH_);
            if (!goodV(pos_)) pos_ = prevPos;
            speed_ = clampf(distanceXZ(actionFrom_, actionTo_) / dur, 0.0f, 6.0f);
        } else {
            speed_ = 0.0f;
        }

        // Vertikal: jang yerda kechadi, lekin havoda qolib ketmaymiz
        {
            const float gy = supportY(world_, pos_.x, pos_.z, pos_.y + kStepUp + 0.02f);
            if (goodF(gy) && (pos_.y - gy) <= kStepUp) {
                pos_.y    = gy;
                velY_     = 0.0f;
                grounded_ = true;
            } else {
                velY_ -= kGravity * dt;
                if (velY_ < -kMaxFallVel) velY_ = -kMaxFallVel;
                if (!goodF(velY_)) velY_ = 0.0f;
                pos_.y += velY_ * dt;
                if (goodF(gy) && pos_.y <= gy) {
                    pos_.y    = gy;
                    velY_     = 0.0f;
                    grounded_ = true;
                } else {
                    grounded_ = false;
                }
            }
            if (!goodV(pos_)) { pos_ = actionFrom_; velY_ = 0.0f; }
        }

        // Nishonga qarab burilish (o'lganda burilmaymiz)
        if (st != MS::Dead) {
            float ty = actionToYaw_;
            if (lock_ != nullptr && goodV(*lock_)) {
                const Vec3 d{lock_->x - pos_.x, 0.0f, lock_->z - pos_.z};
                if (lengthSq(d) > 0.09f) ty = yawFromDir(normalize(d));
            }
            if (goodF(ty)) {
                targetYaw_ = ty;
                yaw_ = lerpAngleDeg(yaw_, targetYaw_, 1.0f - std::exp(-kTurnRate * dt));
            }
        }

        // Kombo buferi — zarba oxirida bosilgan tugma yo'qolmaydi
        if (st == MS::AttackLight && pAtkL && stateTime_ > dur * 0.35f)
            prev.queued = true;

        if (st == MS::Staggered) {
            // Faqat vitals staggerT ni tugatgach chiqamiz
            if (!vitals.staggered) {
                vitals.staggerT = 0.0f;
                stateTime_ = 0.0f;
                speed_     = 0.0f;
                state_     = MS::Idle;
            }
        } else if (st == MS::Dead) {
            speed_ = 0.0f;                      // klip oxirida qotib qoladi
        } else if (stateTime_ >= dur) {
            const bool wantBlock = in.block;
            stateTime_ = 0.0f;
            speed_     = 0.0f;
            switch (st) {
                case MS::AttackLight: {
                    // Zarbadan keyin 0.45 s kombo oynasi ochiladi
                    const bool q = prev.queued;
                    prev.queued  = false;
                    comboWindow_ = 0.45f;
                    state_       = wantBlock ? MS::Blocking : MS::Idle;
                    if (q && !startAttack(DamageType::LightAttack, MS::AttackLight,
                                          (combo_ + 1) % 3)) {
                        combo_       = 0;
                        comboWindow_ = 0.0f;
                    }
                    break;
                }
                case MS::AttackHeavy:
                case MS::KickState:
                    combo_       = 0;
                    comboWindow_ = 0.0f;
                    prev.queued  = false;
                    state_       = wantBlock ? MS::Blocking : MS::Idle;
                    break;
                case MS::Parrying:
                case MS::ParrySuccess:
                    prev.queued = false;
                    state_      = wantBlock ? MS::Blocking : MS::Idle;
                    break;
                default:                        // Hurt, Executing va boshqalar
                    combo_       = 0;
                    comboWindow_ = 0.0f;
                    prev.queued  = false;
                    state_       = MS::Idle;
                    break;
            }
        }

    } else if (st == MS::Hang || st == MS::Shimmy) {
        // ---------------- Osilib turish / yon siljish ----------------
        grounded_ = false;
        velY_     = 0.0f;

        const Vec3 n = safeNormal(hangNormal_, Vec3{0.0f, 0.0f, 1.0f});
        hangNormal_ = n;
        const Vec3 grab{pos_.x - n.x * (radius_ * 0.7f),
                        pos_.y + kHangDrop,
                        pos_.z - n.z * (radius_ * 0.7f)};

        // Yuqori profilda devor tomon bosib turish — avtomatik tepaga chiqish
        // (AC dagi free-run: osilib qolib turmaydi)
        const bool autoUp = (profile_ == Profile::High) && (dot(wishRaw, -n) > 0.4f);

        if (in.parkourUp || pUp || autoUp) {
            // Tepaga chiqish: joy bo'lsa Mantle, bo'lmasa devor bo'ylab ko'tarilish
            Vec3 top{grab.x - n.x * (radius_ + 0.25f),
                     grab.y + 0.02f,
                     grab.z - n.z * (radius_ + 0.25f)};
            if (world_ != nullptr) {
                const float ty = supportY(world_, top.x, top.z, grab.y + 0.7f);
                if (goodF(ty) && ty > grab.y - 0.7f && ty < grab.y + 0.9f) top.y = ty;
            }
            const bool clear = (world_ == nullptr) || world_->mantleClear(top, radius_, bodyH_);
            if (clear) {
                beginAction(MS::Mantle, 0.85f, top, yawFromDir(-n), 0.0f, false);
                emitNoise(0.7f, 11.0f);
            } else {
                state_     = MS::Climb;
                stateTime_ = 0.0f;
                climbT_    = 0.0f;
                emitNoise(0.3f, 5.0f);
            }
        } else if (in.parkourDown || pDown) {
            // Qo'yib yuborish
            state_      = MS::Fall;
            stateTime_  = 0.0f;
            velY_       = 0.0f;
            speed_      = 0.0f;
            fallStartY_ = pos_.y;
        } else {
            // Yon siljish — devor tangensi bo'ylab
            const Vec3  tang{-n.z, 0.0f, n.x};
            const float sdir = dot(wishRaw, tang);
            if (std::fabs(sdir) > 0.25f) {
                const float d  = (sdir > 0.0f) ? 1.0f : -1.0f;
                const bool  ok = (world_ == nullptr) || world_->canShimmy(grab, n, d, 0.45f);
                if (ok) {
                    pos_  += tang * (d * kShimmySpeed * dt);
                    state_ = MS::Shimmy;
                } else {
                    state_ = MS::Hang;
                }
            } else {
                state_ = MS::Hang;
            }
        }

        if (state_ == MS::Hang || state_ == MS::Shimmy) {
            speed_     = (state_ == MS::Shimmy) ? kShimmySpeed : 0.0f;
            targetYaw_ = yawFromDir(-n);
            yaw_ = lerpAngleDeg(yaw_, targetYaw_, 1.0f - std::exp(-kTurnRate * dt));
        }

    } else if (st == MS::Climb) {
        // ---------------- Devorga yopishib ko'tarilish ----------------
        grounded_ = false;
        velY_     = 0.0f;

        if (world_ == nullptr) {
            // Parkursiz dunyoda bu holatda qolib ketmaymiz
            state_      = MS::Fall;
            fallStartY_ = pos_.y;
            stateTime_  = 0.0f;
        } else {
            const Vec3 n = safeNormal(hangNormal_, Vec3{0.0f, 0.0f, 1.0f});
            hangNormal_ = n;
            const Vec3 into = -n;                         // devor tomon
            // devorga bosish = tepaga, o'zidan tortish = pastga
            const float up = clampf(dot(wishRaw, into), -1.0f, 1.0f);

            if (in.parkourDown || pDown) {
                state_      = MS::Fall;
                stateTime_  = 0.0f;
                speed_      = 0.0f;
                fallStartY_ = pos_.y;
            } else {
                if (up > 0.15f) {
                    pos_.y  += kClimbSpeed * up * dt;
                    climbT_ += dt;
                    LedgeInfo li;
                    if (world_->probeLedge(pos_, into, 0.1f, 1.3f, 0.9f, radius_, li) &&
                        li.found && goodV(li.grab)) {
                        grabLedge(li.grab, li.normal, n);
                    }
                } else if (up < -0.15f) {
                    pos_.y -= kClimbSpeed * dt;
                    const float gy = supportY(world_, pos_.x, pos_.z, pos_.y + 0.3f);
                    if (pos_.y <= gy + 0.05f) {
                        pos_.y    = gy;
                        grounded_ = true;
                        speed_    = 0.0f;
                        state_    = MS::Idle;
                        stateTime_ = 0.0f;
                    }
                }

                // Devor tugadimi?
                if (state_ == MS::Climb) {
                    RayHit wh;
                    const bool onWall = world_->probeWall(pos_, into, 1.1f, radius_ + 0.6f, wh) &&
                                        wh.hit && wh.climbable;
                    if (!onWall) {
                        LedgeInfo li;
                        if (world_->probeLedge(pos_, into, 0.0f, 1.7f, 1.0f, radius_, li) &&
                            li.found && goodV(li.grab)) {
                            grabLedge(li.grab, li.normal, n);
                        } else {
                            state_      = MS::Fall;
                            stateTime_  = 0.0f;
                            velY_       = 0.0f;
                            fallStartY_ = pos_.y;
                        }
                    }
                }
            }

            if (state_ == MS::Climb) {
                speed_     = 0.0f;
                targetYaw_ = yawFromDir(into);
                yaw_ = lerpAngleDeg(yaw_, targetYaw_, 1.0f - std::exp(-kTurnRate * dt));
            }
        }

    } else if (isActionState(st)) {
        // ---------------- Bir martalik harakatlar ----------------
        // Foydalanuvchi kiritishi harakat tugagunicha e'tiborsiz.
        stateTime_ += dt;
        const float t  = (actionDur_ > 1e-4f) ? saturate(stateTime_ / actionDur_) : 1.0f;
        const float s  = smoothstepf(t);
        // Mantle da Y ni oldinroq ko'taramiz — "target matching" effekti
        const float sy = (st == MS::Mantle) ? smoothstepf(saturate(t * 1.7f)) : s;

        Vec3 p;
        p.x = lerpf(actionFrom_.x, actionTo_.x, s);
        p.z = lerpf(actionFrom_.z, actionTo_.z, s);
        p.y = lerpf(actionFrom_.y, actionTo_.y, sy);
        if (climbT_ > 0.001f && (st == MS::Vault || st == MS::Dodge))
            p.y += std::sin(PI * t) * climbT_;          // sakrash yoyi
        pos_ = goodV(p) ? p : actionFrom_;

        yaw_      = lerpAngleDeg(actionFromYaw_, actionToYaw_, s);
        velY_     = 0.0f;
        grounded_ = (st == MS::Slide || st == MS::RollLand || st == MS::Dodge ||
                     st == MS::Land  || st == MS::Assassinate);

        if (stateTime_ >= actionDur_) {
            stateTime_ = 0.0f;
            const float jogGate = jogSpeed_ * 0.62f;
            switch (st) {
                case MS::Vault:
                    grounded_ = true;
                    state_ = (speed_ > jogGate) ? MS::Jog : MS::Idle;
                    break;
                case MS::Mantle:
                    grounded_ = true; speed_ = 0.0f; state_ = MS::Idle;
                    break;
                case MS::Slide:
                    grounded_ = true; crouched_ = true; speed_ = 1.0f;
                    state_ = MS::CrouchIdle;
                    break;
                case MS::RollLand:
                    grounded_ = true;
                    state_ = (speed_ > jogGate) ? MS::Jog : MS::Idle;
                    break;
                case MS::Dodge:
                    // Jangda chetlangandan keyin butunlay muzlab qolmaydi
                    grounded_ = true; speed_ = walkSpeed_ * 0.5f; state_ = MS::Idle;
                    break;
                case MS::Assassinate:
                case MS::Land:
                    grounded_ = true; speed_ = 0.0f; state_ = MS::Idle;
                    break;
                case MS::Eject:
                    grounded_ = false; speed_ = 0.0f; velY_ = 0.0f;
                    state_ = MS::Hang;
                    break;
                case MS::WallRun:
                default:
                    grounded_ = false; velY_ = 0.0f; fallStartY_ = pos_.y;
                    state_ = MS::Fall;
                    break;
            }
            // Tezlik vektorini yangi yaw ga moslaymiz (aks holda eski yo'nalish qoladi)
            vel_ = dirFromYaw(yaw_) * speed_;
            prevSpeed_ = speed_;
            yawRate_ = 0.0f;
            climbT_ = 0.0f;
            if (grounded_) {
                const float gy = supportY(world_, pos_.x, pos_.z, pos_.y + 0.5f);
                if (goodF(gy) && std::fabs(gy - pos_.y) < 1.5f) pos_.y = gy;
            }
        }

    } else if (st == MS::Fall || st == MS::JumpUp || st == MS::LeapOfFaith) {
        // ---------------- Havoda ----------------
        grounded_ = false;
        velY_ -= kGravity * dt;
        if (velY_ < -kMaxFallVel) velY_ = -kMaxFallVel;
        if (!goodF(velY_)) velY_ = 0.0f;

        // koyot vaqti — chekkadan chiqqach qisqa muddat sakrash mumkin
        if (pUp && coyote_ > 0.0f && velY_ < 1.0f) {
            velY_       = kJumpVel;
            coyote_     = 0.0f;
            fallStartY_ = pos_.y;
            state_      = MS::JumpUp;
            emitNoise(0.6f, 9.0f);
        } else if (pUp) {
            jumpBuffer_ = 0.15f;      // qo'nish paytida qayta sakrash uchun
        }

        // 0.35 x havo boshqaruvi
        if (wishLen > 0.05f) {
            targetYaw_ = yawFromDir(wish);
            yaw_ = lerpAngleDeg(yaw_, targetYaw_, 1.0f - std::exp(-kTurnRate * kAirControl * dt));
        }
        const float airTgt = (wishLen > 0.05f)
                           ? ((profile_ == Profile::High ? sprintSpeed_ : jogSpeed_) * wishLen)
                           : speed_;
        speed_ = damp(speed_, airTgt, kAccel * kAirControl, dt);
        if (!goodF(speed_) || speed_ < 0.0f) speed_ = 0.0f;

        const Vec3 mdir = dirFromYaw(yaw_);
        const Vec3 prevPos = pos_;
        pos_ += mdir * (speed_ * dt);
        pos_.y += velY_ * dt;
        if (!goodV(pos_)) { pos_ = prevPos; velY_ = 0.0f; }
        if (world_ != nullptr) world_->resolve(pos_, radius_, bodyH_);
        if (!goodV(pos_)) { pos_ = prevPos; velY_ = 0.0f; }

        if (pos_.y > fallStartY_) fallStartY_ = pos_.y;    // apeksni eslab qolamiz
        if (st == MS::JumpUp && velY_ <= 0.0f) state_ = MS::Fall;

        // Oldinda chekka bo'lsa va parkourUp ushlangan bo'lsa — ushlab olamiz
        bool grabbed = false;
        if (world_ != nullptr && velY_ < 1.0f && (in.parkourUp || pUp)) {
            const Vec3 d = (wishLen > 0.05f) ? wish : mdir;
            LedgeInfo li;
            if (world_->probeLedge(pos_, d, 0.6f, 2.4f, 1.1f, radius_, li) &&
                li.found && goodV(li.grab)) {
                grabLedge(li.grab, li.normal, -d);
                emitNoise(0.25f, 5.0f);
                grabbed = true;
            }
        }

        // Qo'nish
        if (!grabbed) {
            const float fromY = ((prevPos.y > pos_.y) ? prevPos.y : pos_.y) + 0.05f;
            const float gy    = supportY(world_, pos_.x, pos_.z, fromY);
            if (velY_ <= 0.0f && pos_.y <= gy) {
                pos_.y    = gy;
                velY_     = 0.0f;
                grounded_ = true;
                coyote_   = 0.12f;

                float h = fallStartY_ - gy;
                if (!goodF(h) || h < 0.0f) h = 0.0f;
                const float jogGate = jogSpeed_ * 0.62f;

                if (h > 12.0f) {
                    // og'ir qo'nish — butun mahalla eshitadi
                    beginAction(MS::Land, 0.9f, pos_, yaw_, 0.0f, false);
                    emitNoise(1.0f, 18.0f);
                } else if (h < 1.2f) {
                    state_     = (speed_ > jogGate) ? MS::Jog : MS::Idle;
                    stateTime_ = 0.0f;
                    emitNoise((h > 0.4f) ? 0.25f : 0.1f, 4.0f);
                    if (jumpBuffer_ > 0.0f) {          // sakrash buferi
                        velY_       = kJumpVel;
                        grounded_   = false;
                        fallStartY_ = pos_.y;
                        state_      = MS::JumpUp;
                        jumpBuffer_ = 0.0f;
                        emitNoise(0.6f, 9.0f);
                    }
                } else if (h < 4.5f) {
                    beginAction(MS::Land, 0.25f, pos_, yaw_, 0.0f, false);
                    emitNoise(0.6f, 8.0f);
                } else if (in.dodge || pDodge) {
                    // dumalab qo'nish — shovqin kam
                    beginAction(MS::RollLand, 0.6f, pos_ + mdir * 2.2f, yaw_, 0.0f, true);
                    emitNoise(0.25f, 5.0f);
                } else {
                    beginAction(MS::Land, 0.45f, pos_, yaw_, 0.0f, false);
                    emitNoise(0.7f, 12.0f);
                }
            }
        }

    } else {
        // ---------------- YERDA (Idle/Walk/Jog/Sprint/Crouch*) ----------------
        grounded_ = true;
        velY_     = 0.0f;
        bool consumed  = false;
        bool blockHold = false;

        // a) Cho'kkalash / yugurishda sirg'alish
        if (pCrouch && !crouched_ && speed_ > jogSpeed_ * 0.9f && wishLen > 0.3f) {
            const Vec3 d = (wishLen > 0.05f) ? wish : faceDir;
            beginAction(MS::Slide, 0.7f, pos_ + d * 4.0f, yawFromDir(d), 0.0f, true);
            emitNoise(0.5f, 9.0f);
            consumed = true;
        } else if (pCrouch) {
            crouched_ = !crouched_;
        }
        if (!consumed && in.highProfile && wishLen > 0.3f) crouched_ = false;

        // b) Chetlanish / dumalash
        if (!consumed && pDodge) {
            const Vec3  d  = (wishLen > 0.05f) ? wish : -faceDir;
            const float ty = (wishLen > 0.05f) ? yawFromDir(d) : yaw_;
            beginAction(MS::Dodge, 0.45f, pos_ + d * 2.6f, ty, 0.12f, true);
            emitNoise(0.3f, 5.0f);
            consumed = true;
        }

        // b2) Yashirin o'ldirish
        if (!consumed && pAss) {
            beginAction(MS::Assassinate, 1.1f, pos_ + faceDir * 0.9f, yaw_, 0.0f, true);
            pending_     = attackDef(DamageType::Assassinate);
            strikeFired_ = false;
            prev.restT   = 0.0f;
            emitNoise(0.35f, 4.0f);
            consumed = true;
        }

        // b2.5) KAMON — ushlab turiladi. Jang tugmalaridan USTUN:
        //       nishonlab turganda LMB ham otadi, RMB esa bekor qiladi.
        //       Kamon `blockHold` kabi MODIFIKATOR — consumed ni qo'ymaydi,
        //       shuning uchun "e) Oddiy harakat" ishlashda davom etadi (yurish saqlanadi).
        bool bowHold = false;
        // DIQQAT: shart ichiga `rBow` ham kiritilgan. Aks holda tugma qo'yib
        // yuborilgan kadrda in.bow ALLAQACHON false bo'lib, else tarmog'iga
        // tushardik va o'q hech qachon uchmasdi.
        const bool bowReady = (arrows_ > 0 && vitals.breath > 6.0f && !in.block);
        if (!consumed && bowReady && (in.bow || (rBow && drawT_ > 0.05f))) {
            if (in.bow) {
                bowHold = true;
                drawT_ = clampf(drawT_ + dt / kBowDraw, 0.0f, 1.0f);

                // Nafas — kamonning asosiy narxi. «Sukunat» uni yengillatadi.
                float cost = (drawT_ >= 1.0f) ? kBowHoldBreath : kBowDrawBreath;
                if (faith.silence()) cost *= kBowSilenceMul;
                vitals.breath -= cost * dt;
                if (!goodF(vitals.breath)) vitals.breath = 0.0f;
                vitals.breath = clampf(vitals.breath, 0.0f, vitals.breathMax);

                // Qo'l titrashi nafas tugab borgani sari kuchayadi
                shake_ = clampf((kBowShakeStart - vitals.breath) / kBowShakeStart, 0.0f, 1.0f);
            }

            // Nafas tugadi -> MAJBURIY reliz. Tarixiy: 60-70 kg yoyni uzoq
            // ushlab bo'lmaydi — o'q otuvchi tutila olmaydi.
            const bool force = in.bow && (vitals.breath <= 0.0f);
            if (rBow || (in.bow && pAtkL) || force) {
                const float ch = clampf(drawT_, 0.0f, 1.0f);
                pendingShot_.origin = aimOrigin();
                pendingShot_.dir    = aimDir();          // titrash kirgan
                pendingShot_.speed  = lerpf(kBowSpeedMin, kBowSpeedMax, ch);
                pendingShot_.charge = ch;
                pendingShot_.spread = aimSpread();
                // emitNoise dan OLDIN o'qiymiz — u noise_ ni ko'taradi
                pendingShot_.silent = (noise_ < 0.30f);
                shotFired_ = false;

                --arrows_;
                const Attack& ar = attackDef(DamageType::Arrow);
                vitals.hand = clampf(vitals.hand - ar.handCost, 0.0f, 100.0f);
                if (force) shake_ = 1.0f;

                drawT_     = 0.0f;
                bowHold    = false;
                state_     = MS::BowShoot;
                stateTime_ = 0.0f;
                actionDur_ = kBowRecover;
                actionFrom_    = actionTo_    = pos_;
                actionFromYaw_ = actionToYaw_ = yaw_;
                strikeFired_   = true;      // qilich kanali bu holatda jim tursin
                prev.queued    = false;
                prev.restT     = 0.0f;
                emitNoise(0.18f, 5.0f);     // kirish tovushi — 5 m
                consumed = true;
            }
        } else {
            // Bekor qilish / o'q tugashi — tortish tez so'nadi
            drawT_ = damp(drawT_, 0.0f, 15.0f, dt);
            if (drawT_ < 0.01f) drawT_ = 0.0f;
            shake_ = damp(shake_, 0.0f, 6.0f, dt);
        }

        // b3) JANG KIRISHLARI — parkurdan USTUN turadi.
        //     Tartib: parry -> og'ir zarba -> yengil zarba -> tepish.
        //     Hammasi faqat yerda va cho'kkalamagan holda.
        if (!consumed && !crouched_ && !bowHold) {
            if (pParry) {
                // Parry oynasi ochiladi (qo'l butunligi oynani qisqartiradi)
                float w = vitals.parryWindow();
                if (!goodF(w) || w < 0.0f) w = 0.0f;
                vitals.parryT  = w;
                state_         = MS::Parrying;
                stateTime_     = 0.0f;
                actionDur_     = 0.30f;
                speed_         = 0.0f;
                strikeFired_   = true;
                actionFrom_    = actionTo_ = pos_;
                actionFromYaw_ = actionToYaw_ = yaw_;
                prev.queued    = false;
                prev.restT     = 0.0f;
                consumed       = true;
            } else if (pAtkH) {
                // Nafas yetmasa zarba boshlanmaydi, lekin tugma baribir jangniki:
                // parkur (free-run) uni o'g'irlab ketmasin.
                (void)startAttack(DamageType::HeavyAttack, MS::AttackHeavy, 0);
                consumed = true;
            } else if (pAtkL) {
                // Kombo oynasi ochiq bo'lsa zanjir davom etadi, aks holda 0 dan
                const int nx = (comboWindow_ > 0.0f) ? ((combo_ + 1) % 3) : 0;
                if (!startAttack(DamageType::LightAttack, MS::AttackLight, nx)) {
                    combo_       = 0;
                    comboWindow_ = 0.0f;
                }
                consumed = true;
            } else if (pKick) {
                (void)startAttack(DamageType::Kick, MS::KickState, 0);
                consumed = true;
            }
        }

        // b4) Blok — ushlab turiladi, nafas sekin sarflanadi (-4/s).
        //     Nafas tugasa himoya ochiladi va zarba yegan holatga tushamiz.
        //     Nafas allaqachon nol bo'lsa qalqon umuman ko'tarilmaydi — aks holda
        //     "buzildi -> Hurt -> buzildi" halqasiga tushib qolardik.
        if (!consumed && !bowHold && in.block && !crouched_ && vitals.breath > 0.0f) {
            blockHold = true;
            vitals.breath -= 4.0f * dt;
            if (!goodF(vitals.breath)) vitals.breath = 0.0f;
            if (vitals.breath <= 0.0f) {
                vitals.breath  = 0.0f;
                blockHold      = false;
                vitals.posture = clampf(vitals.posture + 20.0f, 0.0f, vitals.postureMax);
                state_         = MS::Hurt;
                stateTime_     = 0.0f;
                actionDur_     = 0.35f;
                speed_         = 0.0f;
                strikeFired_   = true;
                actionFrom_    = pos_;
                actionTo_      = pos_ - faceDir * 0.30f;
                actionFromYaw_ = actionToYaw_ = yaw_;
                prev.queued    = false;
                prev.restT     = 0.0f;
                consumed       = true;
            }
        }

        // c) Parkur (tepaga). Yuqori profilda yugurayotganda avtomatik — AC free-run.
        const bool autoFree = (profile_ == Profile::High) && wishLen > 0.3f &&
                              speed_ > jogSpeed_ * 0.8f;
        const bool wantUp   = in.parkourUp || pUp;
        if (!consumed && (wantUp || autoFree)) {
            const Vec3 dir = (wishLen > 0.05f) ? wish : faceDir;
            bool did = false;

            if (world_ != nullptr) {
                // 1. Past to'siq — sakrab o'tamiz
                VaultInfo vi;
                if (world_->probeVault(pos_, dir, 1.25f, 1.15f, radius_, vi) &&
                    vi.found && goodV(vi.exit)) {
                    const float arc = clampf(vi.topY - pos_.y + 0.2f, 0.0f, 1.6f);
                    beginAction(MS::Vault, 0.55f, vi.exit, yawFromDir(dir), arc, true);
                    emitNoise(0.65f, 10.0f);
                    did = true;
                }
                // 2. Chekka — tepaga chiqamiz yoki osilib qolamiz
                if (!did) {
                    LedgeInfo li;
                    if (world_->probeLedge(pos_, dir, 0.9f, 2.6f, 1.15f, radius_, li) && li.found) {
                        if (li.mantleClear && goodV(li.top)) {
                            const Vec3 n = safeNormal(li.normal, -dir);
                            beginAction(MS::Mantle, 0.85f, li.top, yawFromDir(-n), 0.0f, false);
                            emitNoise(0.7f, 11.0f);
                            did = true;
                        } else if (goodV(li.grab)) {
                            grabLedge(li.grab, li.normal, -dir);
                            emitNoise(0.35f, 6.0f);
                            did = true;
                        }
                    }
                }
                // 3. Baland devor — yopishib chiqamiz
                if (!did) {
                    RayHit wh;
                    if (world_->probeWall(pos_, dir, 1.2f, 1.0f, wh) && wh.hit &&
                        wh.climbable && (wh.topY - pos_.y) > 2.6f) {
                        const Vec3 n = safeNormal(wh.normal, -dir);
                        hangNormal_ = n;
                        if (goodV(wh.point)) {
                            pos_.x = wh.point.x + n.x * (radius_ * 0.9f);
                            pos_.z = wh.point.z + n.z * (radius_ * 0.9f);
                        }
                        yaw_ = targetYaw_ = yawFromDir(-n);
                        state_     = MS::Climb;
                        stateTime_ = 0.0f;
                        climbT_    = 0.0f;
                        grounded_  = false;
                        speed_     = 0.0f;
                        emitNoise(0.3f, 6.0f);
                        did = true;
                    }
                }
            }

            // 4. Hech narsa topilmadi — oddiy sakrash (faqat tugma bosilganda)
            if (!did && wantUp) {
                velY_       = kJumpVel;
                grounded_   = false;
                fallStartY_ = pos_.y;
                state_      = MS::JumpUp;
                stateTime_  = 0.0f;
                emitNoise(0.6f, 9.0f);
                did = true;
            }
            consumed = did;
        }

        // d) Parkur (pastga) — chekkadan orqaga tushib osilish ("back eject")
        if (!consumed && (in.parkourDown || pDown) && world_ != nullptr) {
            const Vec3  dir   = (wishLen > 0.05f) ? wish : faceDir;
            const Vec3  front = pos_ + dir * (radius_ + 0.4f);
            const float gy    = supportY(world_, front.x, front.z, pos_.y + 0.2f);
            if (goodF(gy) && (pos_.y - gy) > 1.6f) {
                hangNormal_ = safeNormal(dir, Vec3{0.0f, 0.0f, 1.0f});
                const Vec3 to{pos_.x + hangNormal_.x * (radius_ * 0.7f),
                              pos_.y - kHangDrop,
                              pos_.z + hangNormal_.z * (radius_ * 0.7f)};
                beginAction(MS::Eject, 0.45f, to, yawFromDir(-hangNormal_), 0.0f, false);
                emitNoise(0.3f, 5.0f);
                consumed = true;
            }
        }

        // e) Oddiy harakat - AC lokomotsiyasi.
        //    Tartib MUHIM: yaw -> nishon tezlik -> tezlik -> siljish -> yer ->
        //    ds -> inersiya -> holat. Avval tana buriladi, keyin harakatlanadi;
        //    aks holda tana burilib ulgurmasdan yon tomonga sirg'aladi.
        if (!consumed) {
            // ==== 1) BURILISH - HARAKATDAN OLDIN ====
            bool faced = false;
            // Nishonlashda personaj DOIM kameraga qaraydi — lock-on dan ustun
            if (bowHold) { targetYaw_ = camYaw_; faced = true; }
            if (!faced && lock_ != nullptr && goodV(*lock_)) {
                const Vec3 dl{lock_->x - pos_.x, 0.0f, lock_->z - pos_.z};
                if (lengthSq(dl) > 0.09f) { targetYaw_ = yawFromDir(normalize(dl)); faced = true; }
            }
            if (!faced && wishLen > 0.05f) targetYaw_ = yawFromDir(wish);

            // 180 gradus PIVOT: keskin qaytishda avval TO'XTAYMIZ.
            // Eski kodda sprintda bir kadrda teskari uchib ketardi.
            if (wishLen > 0.5f && speed_ > jogSpeed_ * 0.70f && pivotT_ <= 0.0f && !faced) {
                if (std::fabs(wrapAngleDeg(targetYaw_ - yaw_)) > 135.0f) pivotT_ = kPivotWin;
            }
            if (pivotT_ > 0.0f) { pivotT_ -= dt; if (pivotT_ < 0.0f) pivotT_ = 0.0f; }

            // Burilish RADIUSI fizikadan: a_lat = v*omega <= kLatAccelMax.
            // Natija: walk ~509 deg/s (deyarli joyida), jog 208, sprint 110.
            const float yawErr = wrapAngleDeg(targetYaw_ - yaw_);
            float wCap = kOmegaStand;
            if (speed_ > 0.25f) {
                const float capPhys = rad2deg(kLatAccelMax / speed_);
                if (capPhys < wCap) wCap = capPhys;
            }
            if (wCap < kOmegaMin) wCap = kOmegaMin;
            if (crouched_ || blockHold || lock_ != nullptr) wCap *= 1.6f;  // jang o'tkir qoladi
            if (bowHold) wCap = kOmegaStand;    // nishon kameraga qattiq yopishadi
            if (pivotT_ > 0.0f) wCap *= 2.5f;
            const float wWant = clampf(yawErr * kTurnGain, -wCap, wCap);
            yawRate_ = damp(yawRate_, wWant, kYawAccelLam, dt);
            if (!goodF(yawRate_)) yawRate_ = 0.0f;
            yaw_ = wrapAngleDeg(yaw_ + yawRate_ * dt);
            targetYaw_ = wrapAngleDeg(targetYaw_);

            const Vec3 face = dirFromYaw(yaw_);      // YANGI yaw (443-qatordagi faceDir eski)

            // ==== 2) NISHON TEZLIK ====
            // Klaviaturada analog yo'q -> USHLASH DAVOMIYLIGI pog'ona beradi:
            // birinchi 0.28 s yurish, keyin yugurishga chiqadi.
            if (wishLen > 0.02f) walkRamp_ = clampf(walkRamp_ + dt / kWalkRampT, 0.0f, 1.0f);
            else                 walkRamp_ = 0.0f;
            const float tier = in.walk ? 0.0f
                             : (wishLen < 0.98f ? wishLen
                                                : lerpf(0.45f, 1.0f, smoothstepf(walkRamp_)));

            float tgt = 0.0f, accLam = kAccelJog;
            if (wishLen > 0.02f) {
                if (bowHold)        { tgt = kBowAimSpeed;  accLam = kAccelWalk; }
                else if (crouched_) { tgt = in.walk ? crouchSpeed_ * 0.62f : crouchSpeed_;
                                      accLam = kAccelWalk; }
                else if (blockHold) { tgt = 1.45f;         accLam = kAccelWalk; }
                else if (profile_ == Profile::High && !in.walk)
                                    { tgt = sprintSpeed_;  accLam = kAccelSprint; }
                else                { tgt = lerpf(walkSpeed_, jogSpeed_, tier);
                                      accLam = (tier > 0.6f) ? kAccelJog : kAccelWalk; }

                // Yo'nalish jarimasi: 0 grad -> x1.00, 90 -> x0.625, 180 -> x0.25.
                // Tezlik tushgani uchun wCap avtomatik ko'tariladi -> burilish
                // o'z-o'zidan qattiqlashadi. Aynan AC hissi.
                if (!faced) {
                    const float align = clampf(dot(face, wish), -1.0f, 1.0f);
                    tgt *= lerpf(0.25f, 1.0f, saturate(0.5f + 0.5f * align));
                }
            }
            if (pivotT_ > 0.0f) tgt = 0.0f;               // pivotda qattiq tormoz

            // ==== 3) TEZLIK ====
            prevSpeed_ = speed_;
            if (tgt > speed_) {
                speed_ = damp(speed_, tgt, accLam, dt);
            } else {
                // Sekinlanish MASOFA bo'yicha, CHIZIQLI -> aniq NOLGA keladi.
                // Eksponensial hech qachon nolga kelmaydi va "muz" hissini beradi.
                const float dStop = lerpf(kStopWalk, kStopSprint,
                                          saturate(speed_ / ((sprintSpeed_ > 0.1f) ? sprintSpeed_ : 6.4f)));
                const float aDec  = (speed_ * speed_) / (2.0f * ((dStop > 0.05f) ? dStop : 0.05f))
                                    + kDecelBase;
                speed_ -= aDec * dt;
                if (speed_ < tgt) speed_ = tgt;
            }
            if (!goodF(speed_) || speed_ < 0.02f) speed_ = 0.0f;

            // ==== 4) SILJISH - kiritish YO'Q bo'lganda HAM integratsiya ====
            // Lock-on da strafe saqlanadi: harakat wish bo'yicha, tana nishonda.
            const Vec3 moveDir = (faced && wishLen > 0.05f) ? wish : face;
            vel_ = moveDir * speed_;
            vel_.y = 0.0f;
            if (!goodV(vel_)) { vel_ = Vec3{0.0f, 0.0f, 0.0f}; speed_ = 0.0f; }

            const Vec3 prevPos = pos_;
            pos_ += vel_ * dt;
            if (world_ != nullptr) world_->resolve(pos_, radius_, bodyH_);
            if (!goodV(pos_)) { pos_ = prevPos; vel_ = Vec3{0.0f, 0.0f, 0.0f}; speed_ = 0.0f; }

            // ==== 5) YER / DEVOR BO'YLAB SIRG'ALISH ====
            float gy = supportY(world_, pos_.x, pos_.z, pos_.y + kStepUp + 0.02f);
            float d  = pos_.y - gy;
            if (d < -kStepUp) {
                // Baland to'siq. To'liq TO'XTATMAYMIZ - devor bo'ylab SIRG'ALAMIZ.
                // Devor normalini so'ramasdan X va Z o'qlarini alohida sinaymiz.
                Vec3 tryX = prevPos; tryX.x += vel_.x * dt;
                Vec3 tryZ = prevPos; tryZ.z += vel_.z * dt;
                if (world_ != nullptr) {
                    world_->resolve(tryX, radius_, bodyH_);
                    world_->resolve(tryZ, radius_, bodyH_);
                }
                const float gX = supportY(world_, tryX.x, tryX.z, tryX.y + kStepUp + 0.02f);
                const float gZ = supportY(world_, tryZ.x, tryZ.z, tryZ.y + kStepUp + 0.02f);
                const bool okX = goodV(tryX) && (tryX.y - gX) >= -kStepUp;
                const bool okZ = goodV(tryZ) && (tryZ.y - gZ) >= -kStepUp;
                const float impact = speed_;

                if (okX && !okZ)      { pos_ = tryX; vel_.z = 0.0f; }
                else if (okZ && !okX) { pos_ = tryZ; vel_.x = 0.0f; }
                else { pos_.x = prevPos.x; pos_.z = prevPos.z; vel_ = Vec3{0.0f, 0.0f, 0.0f}; }
                vel_ *= 0.92f;                         // ishqalanish
                speed_ = length(vel_);
                if (!goodF(speed_)) { speed_ = 0.0f; vel_ = Vec3{0.0f, 0.0f, 0.0f}; }
                if (impact > 3.0f) { visYOff_ -= 0.05f; visYVel_ -= 0.35f; }

                gy = supportY(world_, pos_.x, pos_.z, pos_.y + kStepUp + 0.02f);
                d  = pos_.y - gy;
                if (d >= -kStepUp && d <= kStepUp) pos_.y = gy;
            } else if (d <= kStepUp) {
                pos_.y = gy;                 // tekis yer yoki zina
            } else {
                grounded_   = false;         // chekkadan chiqib ketdik
                velY_       = 0.0f;
                fallStartY_ = pos_.y;
                state_      = MS::Fall;
                stateTime_  = 0.0f;
            }

            // ==== 6) HAQIQIY SILJISH - animatsiya SHUNDAN haydaladi ====
            // resolve() qisib qo'ygan yoki step-up qaytargan bo'lsa ds kichrayadi
            // va oyoqlar devor oldida joyida yugurmaydi.
            {
                const float ddx = pos_.x - prevPos.x, ddz = pos_.z - prevPos.z;
                const float ds  = std::sqrt(ddx * ddx + ddz * ddz);
                groundDs_ = goodF(ds) ? clampf(ds, 0.0f, 2.0f) : 0.0f;
            }

            // ==== 7) INERSIYA VIZUALIZATSIYASI ====
            const float aLat  = speed_ * deg2rad(yawRate_);
            const float bankT = clampf(rad2deg(std::atan(aLat / 9.81f)) * 0.30f, -14.0f, 14.0f)
                              * saturate(speed_ / ((jogSpeed_ > 0.1f) ? jogSpeed_ : 3.3f));
            bank_ = damp(bank_, bankT, 6.0f, dt);
            const float aLon  = (dt > 1e-4f) ? ((speed_ - prevSpeed_) / dt) : 0.0f;
            const float leanT = clampf(rad2deg(std::atan(aLon / 9.81f)) * 0.25f, -10.0f, 12.0f);
            lean_ = damp(lean_, leanT, 5.0f, dt);
            if (!goodF(bank_)) bank_ = 0.0f;
            if (!goodF(lean_)) lean_ = 0.0f;

            // Joyida burilish: oyoq mayda qadam tashlaydi, sirg'anmaydi
            turnStepHz_ = 0.0f;
            if (speed_ < 0.30f && std::fabs(yawRate_) > 60.0f)
                turnStepHz_ = clampf(std::fabs(yawRate_) / 200.0f, 0.0f, 1.40f);

            // Orientation warping: harakat yo'nalishi vs tana o'qi (strafe)
            {
                const float wy = (speed_ > 0.15f) ? yawFromDir(vel_) : yaw_;
                const float tw = clampf(wrapAngleDeg(wy - yaw_), -70.0f, 70.0f);
                strideYaw_ = damp(strideYaw_, goodF(tw) ? tw : 0.0f, 14.0f, dt);
            }

            // ==== 8) HOLAT - chegaralar qo'shni pog'onalar O'RTASI ====
            if (grounded_) {
                if (bowHold)        state_ = MS::BowAim;
                else if (blockHold) state_ = MS::Blocking;
                else if (crouched_) state_ = (speed_ > 0.2f) ? MS::CrouchWalk : MS::CrouchIdle;
                else if (speed_ > (jogSpeed_ + sprintSpeed_) * 0.5f) state_ = MS::Sprint;
                else if (speed_ > (walkSpeed_ + jogSpeed_) * 0.5f)   state_ = MS::Jog;
                else if (speed_ > 0.2f) state_ = MS::Walk;
                else state_ = MS::Idle;
            }
        }
    }

    // --- Koyot vaqti va sakrash buferi ---
    if (grounded_) {
        coyote_ = 0.12f;
    } else if (coyote_ > 0.0f) {
        coyote_ -= dt;
        if (coyote_ < 0.0f) coyote_ = 0.0f;
    }
    if (jumpBuffer_ > 0.0f) {
        jumpBuffer_ -= dt;
        if (jumpBuffer_ < 0.0f) jumpBuffer_ = 0.0f;
    }
    if (!goodV(pos_)) pos_ = actionFrom_;
    if (!goodF(yaw_)) yaw_ = 0.0f;

    // --- Vizual ildiz amortizatsiyasi (FAQAT chizish; fizikaga tegmaydi) ---
    // kStepUp = 0.45 m: zinaga chiqishda pos_.y bir kadrda 0.45 m sakraydi va
    // personaj "teleport" qilgandek ko'rinadi. Kamera va soya pos_ dan oladi.
    {
        const float dY = pos_.y - prevPosY_;
        if (grounded_ && std::fabs(dY) > 0.02f && std::fabs(dY) < kStepUp + 0.05f)
            visYOff_ -= dY;
        const float dd = (4.0f * 0.6931472f) / 0.11f;   // kritik damplangan, halflife 0.11 s
        const float y  = dd * 0.5f;
        const float j0 = visYOff_;
        const float j1 = visYVel_ + j0 * y;
        const float e  = std::exp(-y * dt);
        visYOff_ = e * (j0 + j1 * dt);
        visYVel_ = e * (visYVel_ - j1 * y * dt);
        visYOff_ = clampf(visYOff_, -0.22f, 0.10f);
        if (!goodF(visYOff_) || !goodF(visYVel_)) { visYOff_ = 0.0f; visYVel_ = 0.0f; }
        prevPosY_ = pos_.y;
    }

    // =======================================================================
    // 3) SHOVQIN — uzluksiz holatlar
    // =======================================================================
    switch (state_) {
        case MS::Sprint:     emitNoise(0.80f, 14.0f); break;
        case MS::Jog:        emitNoise(0.35f,  8.0f); break;
        case MS::Walk:       emitNoise(0.12f,  4.0f); break;
        case MS::CrouchWalk: emitNoise(0.04f,  2.0f); break;
        case MS::Slide:      emitNoise(0.50f,  9.0f); break;
        case MS::Climb:      emitNoise(0.15f,  4.0f); break;
        case MS::Shimmy:     emitNoise(0.08f,  3.0f); break;
        case MS::Idle:       emitNoise(0.02f,  1.5f); break;
        // jang shovqinli: qilich zarbasi va og'riq butun ko'chani uyg'otadi
        case MS::AttackLight: emitNoise(0.45f,  8.0f); break;
        case MS::AttackHeavy: emitNoise(0.60f, 11.0f); break;
        case MS::KickState:   emitNoise(0.40f,  7.0f); break;
        case MS::ParrySuccess:emitNoise(0.50f,  9.0f); break;
        case MS::Hurt:        emitNoise(0.55f, 10.0f); break;
        case MS::Staggered:   emitNoise(0.50f,  9.0f); break;
        case MS::Executing:   emitNoise(0.65f, 12.0f); break;
        case MS::BowAim:      emitNoise(0.03f,  1.5f); break;   // yoyni tortish deyarli jim
        case MS::BowShoot:    emitNoise(0.18f,  5.0f); break;   // kirish tovushi
        default: break;
    }
    if (noiseEv_.life <= 0.0f) noiseEv_.radius = 0.0f;

    // Kamera/HUD aralashuvi — 0.25 s
    {
        const float want = (state_ == MS::BowAim)   ? 1.0f
                         : (state_ == MS::BowShoot) ? 0.6f : 0.0f;
        aimBlend_ = damp(aimBlend_, want, 11.0f, dt);
        if (!goodF(aimBlend_)) aimBlend_ = 0.0f;
        aimBlend_ = clampf(aimBlend_, 0.0f, 1.0f);
    }

    // =======================================================================
    // 4) ANIMATSIYA — model_->update(dt) aniq bir marta
    // =======================================================================
    if (model_ != nullptr) {
        // klip tezligi harakat davomiyligiga moslanadi
        auto clipRate = [&](float nominal) {
            return (actionDur_ > 1e-3f) ? clampf(nominal / actionDur_, 0.25f, 3.0f) : 1.0f;
        };

        switch (state_) {
            case MS::Idle:
            case MS::Walk:
            case MS::Jog:
            case MS::Sprint: {
                // driveByLocomotion faqat Walk/Run/Idle orasida almashadi —
                // parkur klipidan qaytayotgan bo'lsak, avval Idle ga o'tamiz
                const AnimClip cur = model_->clip();
                if (cur != AnimClip::Idle && cur != AnimClip::Walk && cur != AnimClip::Run)
                    model_->setClip(AnimClip::Idle, 0.15f);
                // Bosh burilishni YETAKLAYDI - damp qilinmaydi (80-120 ms oldinda)
                const float headLead = clampf(wrapAngleDeg(targetYaw_ - yaw_) * 0.30f,
                                              -25.0f, 25.0f);
                model_->setBodyHeightMeters(bodyH_);
                model_->setLocomotionPose(bank_, lean_, headLead, strideYaw_, turnStepHz_);
                animState_ = state_;
                animTime_  = stateTime_;
                model_->driveByLocomotion(speed_, groundDs_, dt);  // ichida update(dt) bor
                break;
            }
            default: {
                AnimClip c  = AnimClip::Fall;
                float    sc = 1.0f;
                float    bl = 0.12f;
                switch (state_) {
                    case MS::CrouchIdle:  c = AnimClip::CrouchIdle; bl = 0.20f; break;
                    case MS::CrouchWalk:  c = AnimClip::CrouchWalk; bl = 0.20f;
                                          sc = clampf(speed_ / 1.1f, 0.5f, 1.8f); break;
                    case MS::Slide:       c = AnimClip::Slide;  bl = 0.08f; sc = clipRate(0.7f);  break;
                    case MS::Vault:       c = AnimClip::Vault;  bl = 0.08f; sc = clipRate(0.55f); break;
                    case MS::Mantle:      c = AnimClip::Mantle; bl = 0.08f; sc = clipRate(0.85f); break;
                    case MS::Climb:       c = AnimClip::ClimbUp; bl = 0.20f; break;
                    case MS::Hang:        c = AnimClip::Hang;   bl = 0.20f; break;
                    case MS::Shimmy:      c = AnimClip::Shimmy; bl = 0.15f; break;
                    case MS::Dodge:       c = AnimClip::Dodge;  bl = 0.06f; sc = clipRate(0.45f); break;
                    case MS::RollLand:    c = AnimClip::Roll;   bl = 0.06f; sc = clipRate(0.6f);  break;
                    case MS::Assassinate: c = AnimClip::Assassinate; bl = 0.08f; sc = clipRate(1.1f); break;
                    case MS::WallRun:     c = AnimClip::WallRun; bl = 0.10f; break;
                    // Qo'nishda tizzani bukish — alohida klip yo'q, cho'kkalashdan foydalanamiz
                    case MS::Land:        c = AnimClip::CrouchIdle; bl = 0.10f; break;

                    // --- Jang kliplari ---
                    case MS::AttackLight:
                        if (combo_ == 1)      { c = AnimClip::AttackLight2; sc = clipRate(0.50f); }
                        else if (combo_ == 2) { c = AnimClip::AttackLight3; sc = clipRate(0.70f); }
                        else                  { c = AnimClip::AttackLight1; sc = clipRate(0.55f); }
                        bl = 0.05f;
                        break;
                    case MS::AttackHeavy: c = AnimClip::AttackHeavy; bl = 0.06f; sc = clipRate(0.95f); break;
                    case MS::KickState:   c = AnimClip::KickClip;    bl = 0.06f; sc = clipRate(0.45f); break;
                    case MS::Blocking:    c = AnimClip::Block;       bl = 0.12f; break;
                    case MS::Parrying:    c = AnimClip::Block;       bl = 0.05f; break;
                    case MS::ParrySuccess:c = AnimClip::ParryHit;    bl = 0.04f; sc = clipRate(0.35f); break;
                    case MS::Hurt:        c = AnimClip::Hurt;        bl = 0.05f; sc = clipRate(0.35f); break;
                    case MS::Staggered:   c = AnimClip::Stagger;     bl = 0.08f; sc = clipRate(1.20f); break;
                    case MS::Executing:   c = AnimClip::Execute;     bl = 0.08f; sc = clipRate(1.30f); break;
                    case MS::BowAim:      c = AnimClip::BowAim;      bl = 0.14f; break;
                    case MS::BowShoot:    c = AnimClip::BowShoot;    bl = 0.04f; sc = clipRate(0.40f); break;
                    case MS::Dead:        c = AnimClip::Death;       bl = 0.10f; sc = clipRate(1.40f); break;

                    default:              c = AnimClip::Fall;   bl = 0.12f; break;
                }
                // CrouchWalk fazasi ham MASOFAdan haydalsin (klip tanlamasdan)
                model_->setBodyHeightMeters(bodyH_);
                model_->setLocomotionPose(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                model_->setLocomotion(speed_, groundDs_);
                // YANGI harakat boshlandimi? Holat o'zgargan bo'lsa yoki holat
                // taymeri orqaga qaytgan bo'lsa (kombo zanjirida shunday bo'ladi)
                // klip BOSHIDAN qo'yiladi; aks holda oddiy tanlash.
                const bool fresh = (state_ != animState_) || (stateTime_ < animTime_);
                animState_ = state_;
                animTime_  = stateTime_;
                if (fresh) model_->playClip(c, bl);
                else       model_->setClip(c, bl);
                model_->setSpeedScale(sc);
                model_->update(dt);
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Chizish
// ---------------------------------------------------------------------------
void Character::draw(float heightMeters, const float tint[3]) {
    // tint ni chaqiruvchi qo'llaydi (glColor / material) — bu yerda faqat model
    (void)tint;
    if (model_ == nullptr) return;
    float h = heightMeters;
    if (!goodF(h) || h <= 0.05f) h = bodyH_;
    // Vizual amortizatsiya faqat MODELga qo'llanadi (kamera/soya pos_ dan oladi)
    model_->draw(Vec3{pos_.x, pos_.y + visYOff_, pos_.z}, yaw_, h);
}

// ---------------------------------------------------------------------------
// Kamera
// ---------------------------------------------------------------------------
Vec3 Character::cameraFocus() const {
    float h = 1.45f;
    switch (state_) {
        case MoveState::Slide:
        case MoveState::RollLand:    h = 0.90f; break;
        case MoveState::CrouchIdle:
        case MoveState::CrouchWalk:  h = 1.00f; break;
        case MoveState::Hang:
        case MoveState::Shimmy:      h = 1.10f; break;
        case MoveState::Eject:       h = 1.15f; break;
        case MoveState::Climb:       h = 1.20f; break;
        case MoveState::Land:        h = 1.25f; break;
        case MoveState::Fall:
        case MoveState::JumpUp:
        case MoveState::LeapOfFaith: h = 1.50f; break;
        case MoveState::Sprint:      h = 1.60f; break;
        case MoveState::Staggered:   h = 1.20f; break;
        case MoveState::BowAim:
        case MoveState::BowShoot:    h = 1.55f; break;
        case MoveState::Dead:        h = 0.55f; break;
        case MoveState::Executing:
        case MoveState::Blocking:
        case MoveState::Parrying:
        case MoveState::ParrySuccess:
        case MoveState::Hurt:
        case MoveState::AttackLight:
        case MoveState::AttackHeavy:
        case MoveState::KickState:   h = 1.40f; break;
        default:                     h = crouched_ ? 1.00f : 1.45f; break;
    }
    h = clampf(h, (state_ == MoveState::Dead) ? 0.35f : 0.9f, 1.7f);
    return Vec3{pos_.x, pos_.y + h, pos_.z};
}

float Character::cameraDistanceHint() const {
    switch (state_) {
        case MoveState::Sprint:      return 6.8f;
        case MoveState::Hang:
        case MoveState::Shimmy:
        case MoveState::Climb:       return 3.4f;
        case MoveState::CrouchIdle:
        case MoveState::CrouchWalk:
        case MoveState::Slide:       return 4.2f;
        // Jangda kamera yaqinroq — zarbaning og'irligi sezilsin
        case MoveState::Executing:
        case MoveState::Assassinate: return 3.0f;
        // Nishonlashda yelka ustidan — juda yaqin
        case MoveState::BowAim:      return 2.6f;
        case MoveState::BowShoot:    return 3.0f;
        case MoveState::Dead:        return 3.6f;
        case MoveState::AttackLight:
        case MoveState::AttackHeavy:
        case MoveState::KickState:
        case MoveState::Blocking:
        case MoveState::Parrying:
        case MoveState::ParrySuccess:
        case MoveState::Hurt:
        case MoveState::Staggered:   return 4.6f;
        default:                     return 5.2f;
    }
}

const char* Character::stateLocKey() const {
    const int i = static_cast<int>(state_);
    if (i < 0 || i >= kStateCount) return kStateKeys[0];
    return kStateKeys[i];
}

// ---------------------------------------------------------------------------
// Jang
// ---------------------------------------------------------------------------
bool Character::busy() const {
    return isActionState(state_) || isCombatLock(state_);
}

bool Character::inCombatState() const {
    switch (state_) {
        case MoveState::AttackLight:
        case MoveState::AttackHeavy:
        case MoveState::KickState:
        case MoveState::Blocking:
        case MoveState::Parrying:
        case MoveState::ParrySuccess:
        case MoveState::Hurt:
        case MoveState::Staggered:
        case MoveState::Executing:
        case MoveState::Assassinate:
        case MoveState::Dead:
            return true;
        default:
            return false;
    }
}

void Character::setLockTarget(const Vec3* worldPos) {
    lock_ = worldPos;
}

// Zarbaning FAOL oynasi: windup dan keyin, recovery dan oldin — bir marta.
// ---------------------------------------------------------------------------
// Kamon
// ---------------------------------------------------------------------------
Vec3 Character::aimOrigin() const {
    // Chap qo'l balandligi (~ko'z sathi): oldinga 0.35 m, o'ngga 0.22 m
    const Vec3 f = dirFromYaw(yaw_);
    const Vec3 r{-f.z, 0.0f, f.x};
    const Vec3 o{pos_.x + f.x * 0.35f + r.x * 0.22f,
                 pos_.y + clampf(bodyH_ * 0.80f, 0.6f, 2.0f),
                 pos_.z + f.z * 0.35f + r.z * 0.22f};
    return goodV(o) ? o : pos_;
}

Vec3 Character::aimDir() const {
    // Titrash — ikki chastotali determinatsiyalangan tebranish (rand YO'Q):
    // o'yinchi noaniqlikni HUD da KO'RADI va tasodifdan aziyat chekmaydi.
    float amp = kBowShakeMaxDeg * shake_ * (0.55f + 0.45f * drawT_);
    if (faith.silence()) amp *= 0.45f;              // «Sukunat» qo'lni tinchlantiradi
    const float yawD   =  camYaw_   + amp * std::sin(shakeT_ * 5.7f) * 0.9f;
    const float pitchD = -camPitch_ + amp * std::sin(shakeT_ * 3.9f + 1.3f) * 0.7f;
    const float cp = std::cos(deg2rad(pitchD));
    const Vec3 d{std::sin(deg2rad(yawD)) * cp,
                 std::sin(deg2rad(pitchD)),
                 std::cos(deg2rad(yawD)) * cp};
    return (goodV(d) && lengthSq(d) > 1e-6f) ? normalize(d) : dirFromYaw(yaw_);
}

float Character::aimSpread() const {
    float s = lerpf(4.2f, 0.35f, smoothstepf(drawT_));       // tortish -> aniqlik
    s += shake_ * 3.0f;                                      // titrash
    s += clampf(speed_ / kBowAimSpeed, 0.0f, 1.0f) * 0.9f;   // harakatda yomonroq
    if (crouched_) s *= 0.70f;                               // cho'kkalash yordam beradi
    return goodF(s) ? clampf(s, 0.15f, 9.0f) : 4.0f;
}

bool Character::consumeShot(BowShot& out) {
    if (shotFired_) return false;
    shotFired_ = true;
    out = pendingShot_;
    edgesFor(this).restT = 0.0f;      // otdik — dam olish taymeri nolga
    return true;
}

void Character::addArrows(int n) {
    if (n <= 0) return;
    arrows_ = (arrows_ + n > arrowsMax_) ? arrowsMax_ : arrows_ + n;
}

void Character::setArrowCapacity(int n) {
    arrowsMax_ = (n < 1) ? 1 : ((n > 60) ? 60 : n);
    if (arrows_ > arrowsMax_) arrows_ = arrowsMax_;
}

float Character::cameraFovHint() const {
    // Nishonlashda ko'rish maydoni torayadi — masofa "yaqinlashadi"
    return lerpf(48.0f, 36.0f, clampf(aimBlend_, 0.0f, 1.0f));
}

float Character::cameraShoulderOffset() const {
    return 0.55f * clampf(aimBlend_, 0.0f, 1.0f);   // o'ngga siljish (m)
}

bool Character::consumeAttack(Attack& out) {
    if (strikeFired_ || !isStrikeState(state_)) return false;

    // Klip davomiyligi jadvaldagi davomiylikdan farq qilsa (Execute/Assassinate)
    // oynani mutanosib ravishda cho'zamiz.
    const float defDur = pending_.duration();
    float w = goodF(pending_.windup) ? pending_.windup : 0.0f;
    float a = goodF(pending_.active) ? pending_.active : 0.0f;
    if (defDur > 1e-4f && actionDur_ > 1e-4f) {
        const float k = clampf(actionDur_ / defDur, 0.1f, 10.0f);
        w *= k;
        a *= k;
    }
    if (w < 0.0f) w = 0.0f;
    if (a < 0.02f) a = 0.10f;            // jadval bo'sh bo'lsa ham zarba yo'qolmasin

    if (stateTime_ >= w && stateTime_ <= w + a) {
        strikeFired_ = true;
        out          = pending_;
        edgesFor(this).restT = 0.0f;     // jangdamiz — dam olish taymeri nolga
        return true;
    }
    return false;
}

// Zarba yo'nalishi va og'irligini animatsiyaga uzatadi.
// Og'irlik zarar + poza zararidan chiqadi: yengil zarba ~0.4, og'ir ~1.0.
void Character::setHitPose(const Vec3& toSrc, const HitResult& r) {
    if (model_ == nullptr) return;
    const float deg = (lengthSq(toSrc) > 0.01f)
                    ? wrapAngleDeg(yawFromDir(toSrc) - yaw_) : 0.0f;
    const float w = clampf((r.damage + 0.5f * r.posture - 8.0f) / 32.0f, 0.35f, 1.0f);
    model_->setHitDir(deg, w);
}

HitResult Character::receiveHit(const Attack& a, const Vec3& from) {
    using MS = MoveState;
    HitResult r;
    r.point = Vec3{pos_.x, pos_.y + clampf(bodyH_ * 0.55f, 0.2f, 2.0f), pos_.z};
    if (state_ == MS::Dead) { r.outcome = HitOutcome::Miss; return r; }

    EdgeState& es = edgesFor(this);

    // Zarba manbaiga qarab yo'nalish (XZ)
    Vec3 toSrc{0.0f, 0.0f, 0.0f};
    bool frontal = true;                     // manba noma'lum bo'lsa oldindan deb hisoblaymiz
    if (goodV(from)) {
        const Vec3 d{from.x - pos_.x, 0.0f, from.z - pos_.z};
        if (lengthSq(d) > 1.0e-4f) {
            toSrc = normalize(d);
            // oldingi 100 gradus sektor => yarim burchak 50 gradus
            frontal = dot(dirFromYaw(yaw_), toSrc) >= std::cos(deg2rad(50.0f));
        }
    }

    // 1) Qisqa daxlsizlik (stagger'dan chiqish va h.k.)
    if (vitals.invulnT > 0.0f) {
        es.restT  = 0.0f;
        r.outcome = HitOutcome::Dodged;
        return r;
    }

    // 2) Dumalash / chetlanishning boshlanishi — 0.25 s oyna.
    //    GDD: dodge o'zi i-frame emas, lekin dumalash paytida qisqa daxlsizlik
    //    mantiqiy (Ghost of Tsushima oqimi).
    if ((state_ == MS::Dodge || state_ == MS::RollLand) && stateTime_ <= 0.25f) {
        es.restT  = 0.0f;
        r.outcome = HitOutcome::Dodged;
        return r;
    }

    // Zarba yeyish reaksiyasi faqat yerda va yakunlovchi zarba paytida emas
    const bool onFoot = grounded_ && state_ != MS::Hang && state_ != MS::Shimmy &&
                        state_ != MS::Climb && state_ != MS::Executing &&
                        state_ != MS::Assassinate;

    // 3) Parry — oyna ochiq va zarba oldindan kelayapti
    if (parryOpen() && frontal) {
        r = vitals.receive(a, false, true, r.point);
        r.outcome = HitOutcome::Parried;
        r.posture = 35.0f;                   // raqibga KATTA poza zarari (chaqiruvchi qo'llaydi)
        r.point   = Vec3{pos_.x, pos_.y + clampf(bodyH_ * 0.55f, 0.2f, 2.0f), pos_.z};
        faith.add(1.0f);
        vitals.parryT  = 0.0f;
        // Mukammal parry — Iymonning asosiy manbai. Sukunat darajasida esa
        // vaqtni sekinlashtiradi (o'yinchi qarshi hujumga ulguradi).
        faith.event(FaithEvent::PerfectParry);
        faith.triggerSilence();
        state_         = MS::ParrySuccess;
        stateTime_     = 0.0f;
        actionDur_     = 0.35f;
        speed_         = 0.0f;
        strikeFired_   = true;
        actionFrom_    = actionTo_ = pos_;
        actionFromYaw_ = yaw_;
        setHitPose(toSrc, r);
        actionToYaw_   = (lengthSq(toSrc) > 0.01f) ? yawFromDir(toSrc) : yaw_;
        es.queued      = false;
        es.restT       = 0.0f;
        return r;
    }

    // 4) Blok — zarba oldindan kelsa qalqon ushlab qoladi
    if (state_ == MS::Blocking && frontal) {
        r = vitals.receive(a, true, false, r.point);
        r.outcome = HitOutcome::Blocked;
        es.restT  = 0.0f;
        if (vitals.health <= 0.0f) { kill(); r.outcome = HitOutcome::Killed; }
        return r;
    }

    // 5) To'liq zarba
    r = vitals.receive(a, false, false, r.point);
    r.outcome = HitOutcome::Hit;
    es.restT  = 0.0f;

    if (onFoot) {
        // Zarba YO'NALISHINI pozaga uzatamiz. Ilgari toSrc faqat actionToYaw_
        // uchun ishlatilardi va poza yo'nalishni bilmasdi: qayerdan zarba
        // yesangiz ham bosh bir xil tomonga qiyshayardi.
        setHitPose(toSrc, r);

        float back = goodF(a.knockback) ? (a.knockback * 0.18f) : 0.0f;
        back = clampf(back, 0.0f, 1.4f);
        const Vec3 push = (lengthSq(toSrc) > 0.01f) ? -toSrc : -dirFromYaw(yaw_);
        state_         = MS::Hurt;
        stateTime_     = 0.0f;
        actionDur_     = 0.35f;
        speed_         = 0.0f;
        strikeFired_   = true;
        combo_         = 0;
        comboWindow_   = 0.0f;
        actionFrom_    = pos_;
        actionTo_      = pos_ + push * back;
        actionFromYaw_ = yaw_;
        // Orqadan kelgan zarbada 180 gradusga pirillab ketmaslik uchun burilish
        // +-55 bilan cheklanadi. Aks holda kTurnRate = 12 bilan 0.35 s ichida
        // burilib bo'linardi va yo'nalishli poza HECH QACHON ko'rinmasdi.
        actionToYaw_   = yaw_ + clampf(wrapAngleDeg(
                             ((lengthSq(toSrc) > 0.01f) ? yawFromDir(toSrc) : yaw_) - yaw_),
                             -55.0f, 55.0f);
        es.queued      = false;
    }

    if (vitals.health <= 0.0f) { kill(); r.outcome = HitOutcome::Killed; }
    return r;
}

// Nishonga qarab 0.9 m masofada turib yashirin o'ldirish.
// playExecute shu tayyorgarlikni qayta ishlatadi (faqat holat va davomiylik boshqa).
void Character::playAssassinate(const Vec3& targetPos) {
    if (state_ == MoveState::Dead) return;

    execTarget_ = goodV(targetPos) ? targetPos : pos_;

    Vec3 d{execTarget_.x - pos_.x, 0.0f, execTarget_.z - pos_.z};
    Vec3 dir = dirFromYaw(yaw_);
    if (lengthSq(d) > 1.0e-4f) dir = normalize(d);

    Vec3 dst{execTarget_.x - dir.x * 0.9f, pos_.y, execTarget_.z - dir.z * 0.9f};
    if (!goodV(dst)) dst = pos_;
    if (world_ != nullptr) {
        world_->resolve(dst, radius_, bodyH_);
        if (!goodV(dst)) dst = pos_;
    }
    const float gy = supportY(world_, dst.x, dst.z, pos_.y + 0.6f);
    if (goodF(gy) && std::fabs(gy - pos_.y) < 2.0f) dst.y = gy;
    pos_ = goodV(dst) ? dst : pos_;

    yaw_ = targetYaw_ = yawFromDir(dir);
    actionFrom_    = actionTo_ = pos_;
    actionFromYaw_ = actionToYaw_ = yaw_;
    actionDur_     = 1.1f;
    stateTime_     = 0.0f;
    climbT_        = 0.0f;
    speed_         = 0.0f;
    velY_          = 0.0f;
    grounded_      = true;
    combo_         = 0;
    comboWindow_   = 0.0f;
    pending_       = attackDef(DamageType::Assassinate);
    strikeFired_   = false;
    state_         = MoveState::Assassinate;

    EdgeState& es = edgesFor(this);
    es.queued = false;
    es.restT  = 0.0f;
}

// Yakunlovchi zarba — tayyorgarlik bir xil, faqat holat va davomiylik boshqa
void Character::playExecute(const Vec3& targetPos) {
    playExecute(-1, targetPos);
}

void Character::playExecute(int victimIdx, const Vec3& targetPos) {
    if (state_ == MoveState::Dead) return;
    playAssassinate(targetPos);
    if (state_ == MoveState::Assassinate) {
        state_      = MoveState::Executing;
        actionDur_  = 1.3f;
        execVictim_ = victimIdx;
        // Yakunlovchi zarba jangchining sabri mukofoti
        faith.event(FaithEvent::Finisher);
        faith.triggerSilence();
    }
}

void Character::kill() {
    if (state_ == MoveState::Dead) return;
    state_         = MoveState::Dead;
    actionDur_     = 1.4f;
    stateTime_     = 0.0f;
    speed_         = 0.0f;
    velY_          = 0.0f;
    combo_         = 0;
    comboWindow_   = 0.0f;
    strikeFired_   = true;
    lock_          = nullptr;
    actionFrom_    = actionTo_ = pos_;
    actionFromYaw_ = actionToYaw_ = yaw_;
    vitals.health    = 0.0f;
    vitals.staggered = false;
    vitals.staggerT  = 0.0f;
    vitals.parryT    = 0.0f;
    vitals.invulnT   = 0.0f;

    EdgeState& es = edgesFor(this);
    es.queued = false;
    es.restT  = 0.0f;

    // model_->update(dt) faqat update() ichida chaqiriladi — bu yerda faqat klip
    if (model_ != nullptr) model_->setClip(AnimClip::Death, 0.10f);
}

void Character::revive(const Vec3& feetPos, float yawDeg) {
    reset(feetPos, yawDeg);          // iymon (faith) saqlanadi — reset unga tegmaydi
    vitals.reset(100.0f);
    state_ = MoveState::Idle;
    if (model_ != nullptr) model_->setClip(AnimClip::Idle, 0.20f);
}

} // namespace ert
