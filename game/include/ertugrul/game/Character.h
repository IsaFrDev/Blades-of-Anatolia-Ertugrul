#pragma once
// Assassin's Creed uslubidagi personaj boshqaruvi.
//
// Asosiy g'oya — klassik AC ning "PAST PROFIL / YUQORI PROFIL" modeli:
//   past profil  : yurish, olomonga singish, jimgina tushish — shovqin yo'q
//   yuqori profil: yugurish, erkin yurish (free-run), devorga chiqish, sakrash —
//                  shovqin chiqadi va qorovullar sezadi
//
// Harakat holatlari avtomatik tanlanadi: oldinda past to'siq bo'lsa — sakrab o'tadi,
// baland devor bo'lsa — chekkani ushlab tepaga chiqadi, tomdan tushsa — dumalab qo'nadi.
#include <string>
#include "ertugrul/core/Math.h"
#include "ertugrul/gfx/Skin.h"
#include "ertugrul/game/Combat.h"
#include "ertugrul/game/Projectile.h"   // BowShot (bir tomonlama bog'liqlik)

namespace ert {

class PhysicsWorld;

enum class MoveState {
    Idle = 0,
    Walk,           // past profil
    Jog,
    Sprint,         // yuqori profil (free-run)
    CrouchIdle,
    CrouchWalk,
    Slide,          // yugurishda cho'kkalash
    Vault,          // past to'siqdan sakrab o'tish
    Mantle,         // chekkadan tepaga chiqish
    Climb,          // devorga yopishib ko'tarilish
    Hang,           // chekkada osilib turish
    Shimmy,         // osilgan holda yon siljish
    Eject,          // chekkadan orqaga sakrash
    JumpUp,         // bo'shliq ustidan sakrash
    Fall,
    Land,
    RollLand,       // dumalab qo'nish (shovqin kam)
    WallRun,
    Dodge,
    Assassinate,
    LeapOfFaith,

    // --- Jang holatlari ---
    AttackLight,    // yengil zarba (kombo indeksi ichkarida)
    AttackHeavy,
    KickState,
    Blocking,       // himoya ushlanmoqda
    Parrying,       // parry oynasi ochiq
    ParrySuccess,   // parry ishladi — raqib ochiq qoldi
    Hurt,           // zarba yedi
    Staggered,      // poza buzildi
    Executing,      // yakunlovchi zarba
    BowAim,         // kamon tortilmoqda — USHLANADI, sekin yurish mumkin
    BowShoot,       // o'q uchdi — qisqa tiklanish
    Dead,
    Count
};

enum class Profile { Low = 0, High };

const char* moveStateName(MoveState s);

struct CharacterInput {
    Vec2  move{0, 0};          // kameraga nisbatan (-1..1). MODULI ham ma'noli (analog)
    float camYaw   = 0.0f;     // kamera yo'nalishi (gradus)
    float camPitch = 0.0f;     // kamera qiyaligi (gradus; MUSBAT = pastga qaraydi)
    bool  highProfile = false; // Shift ushlangan -> yugurish / free-run
    bool  walk        = false; // Alt ushlangan  -> past profil yurish
    bool  parkourUp   = false; // Space -> sakrash / chiqish / ushlash
    bool  parkourDown = false; // Ctrl  -> tushish / qo'yib yuborish
    bool  dodge       = false; // Q     -> chetlanish / dumalash
    bool  crouch      = false; // C     -> cho'kkalash (bosilgan payt almashadi)
    bool  interact    = false;
    bool  assassinate = false;

    // --- Jang ---
    bool  attackLight = false;   // Mouse 1 (bosilgan payt)
    bool  attackHeavy = false;   // Shift + Mouse 1
    bool  block       = false;   // Mouse 2 ushlangan -> blok
    bool  parry       = false;   // Mouse 2 bosilgan payt -> parry oynasi
    bool  kick        = false;   // R
    bool  bow         = false;   // G ushlangan
    bool  lockOn      = false;   // o'rta tugma / T — nishonni qulflash
};

// O'yinchi chiqargan shovqin hodisasi (qorovullar uchun)
struct NoiseEvent {
    Vec3  pos{0, 0, 0};
    float radius = 0.0f;
    float life   = 0.0f;
};

class Character {
public:
    // model va dunyo egasi emas — faqat ko'rsatkich saqlaydi
    bool init(SkinnedModel* model, PhysicsWorld* world);
    void reset(const Vec3& feetPos, float yawDeg);

    void update(const CharacterInput& in, float dt);
    void draw(float heightMeters, const float tint[3]);

    // --- Holat ---
    Vec3      position() const { return pos_; }        // oyoq ostidagi nuqta
    float     yaw() const { return yaw_; }
    MoveState state() const { return state_; }
    Profile   profile() const { return profile_; }
    float     speed() const { return speed_; }         // m/s
    bool      grounded() const { return grounded_; }
    bool      hanging() const { return state_ == MoveState::Hang || state_ == MoveState::Shimmy; }
    bool      climbing() const { return state_ == MoveState::Climb; }

    // Kamera qaraydigan nuqta (holatga qarab balandligi o'zgaradi)
    Vec3  cameraFocus() const;
    // Kamera masofasi tavsiyasi (yugurishda uzoqroq, osilganda yaqinroq)
    float cameraDistanceHint() const;

    // --- Shovqin / sezilish ---
    float lastNoise() const { return noise_; }          // 0..1
    const NoiseEvent& noiseEvent() const { return noiseEv_; }
    // Ekranda ko'rsatiladigan qisqa holat matni (HUD uchun loc kaliti)
    const char* stateLocKey() const;

    // --- Jang ---
    Vitals vitals;
    Faith  faith;

    // Harakat tugagunicha boshqa buyruq qabul qilinmaydi
    bool  busy() const;
    bool  inCombatState() const;
    bool  blocking() const { return state_ == MoveState::Blocking; }
    bool  parryOpen() const { return vitals.parryT > 0.0f; }
    bool  dead() const { return state_ == MoveState::Dead; }

    // Zarbaning FAOL oynasi ochilganda bir marta true qaytaradi va
    // qaysi zarba ekanini beradi (EnemyManager::playerAttack ga uzatiladi).
    bool  consumeAttack(Attack& out);
    // Kamon otildimi? Bir kadrda BIR MARTA true qaytaradi va o'q buyurtmasini beradi.
    // Alohida kanal: o'q qilich konusidan emas, ballistik yo'ldan boradi.
    bool  consumeShot(BowShot& out);

    // --- Kamon holati (HUD va kamera uchun) ---
    bool  aiming()      const { return state_ == MoveState::BowAim; }
    float drawCharge()  const { return drawT_; }      // 0..1 tortish kuchi
    float aimBlend()    const { return aimBlend_; }   // 0..1 silliq kamera aralashuvi
    float aimShake()    const { return shake_; }      // 0..1 qo'l titrashi
    float aimSpread()   const;                        // gradus — HUD nishoni kengligi
    Vec3  aimOrigin()   const;                        // o'q chiqadigan nuqta (jahon)
    Vec3  aimDir()      const;                        // titrash QO'SHILGAN yo'nalish
    int   arrows()      const { return arrows_; }
    int   arrowsMax()   const { return arrowsMax_; }
    void  addArrows(int n);
    void  setArrowCapacity(int n);
    float cameraFovHint() const;
    float cameraShoulderOffset() const;
    // Dushman zarbasini qabul qilish (blok/parry/dodge shu yerda hal qilinadi)
    HitResult receiveHit(const Attack& a, const Vec3& from);
    // Nishonga qarab turish (lock-on). nullptr = qulf yo'q.
    void  setLockTarget(const Vec3* worldPos);
    bool  hasLock() const { return lock_ != nullptr; }
    // Yakunlovchi zarba / yashirin o'ldirish animatsiyasini boshlash
    void  playExecute(const Vec3& targetPos);                  // eski imzo (Cutscene moslik)
    // Zarba yo'nalishi va og'irligini animatsiyaga uzatadi (ichki)
    void  setHitPose(const Vec3& toSrc, const HitResult& r);
    // victimIdx: EnemyManager massividagi aniq qurbon. Konus qidiruvi
    // ishlatilmaydi, ya'ni zarba boshqa dushmanga "sakrab" ketmaydi.
    void  playExecute(int victimIdx, const Vec3& targetPos);
    int   execVictim() const { return execVictim_; }
    void  playAssassinate(const Vec3& targetPos);
    void  kill();
    void  revive(const Vec3& feetPos, float yawDeg);

    // --- Sozlash ---
    void setWalkSpeed(float v)   { walkSpeed_ = v; }
    void setJogSpeed(float v)    { jogSpeed_ = v; }
    void setSprintSpeed(float v) { sprintSpeed_ = v; }
    void setBodyRadius(float r)  { radius_ = r; }
    void setBodyHeight(float h)  { bodyH_ = h; }
    void setCrouchSpeed(float v) { crouchSpeed_ = v; }
    // Diagnostika: shu kadrda bosib o'tilgan masofa (oyoq sirg'alishini o'lchash)
    float debugDs() const { return groundDs_; }

private:
    SkinnedModel* model_ = nullptr;
    PhysicsWorld* world_ = nullptr;

    Vec3  pos_{0, 0, 0};
    float yaw_ = 0.0f, targetYaw_ = 0.0f;
    float velY_ = 0.0f, speed_ = 0.0f;
    // --- AC lokomotsiyasi: tezlik VEKTORI va burchak inersiyasi ---
    Vec3  vel_{0, 0, 0};       // gorizontal tezlik (m/s); speed_ = length(vel_)
    float yawRate_    = 0.0f;  // burchak tezligi (deg/s)
    float prevSpeed_  = 0.0f;
    float bank_       = 0.0f;  // yon egilish (burilish inersiyasi)
    float lean_       = 0.0f;  // uzunlamasiga egilish (tezlanish)
    float walkRamp_   = 0.0f;  // klaviaturada walk -> jog pog'onasi (0..1)
    float pivotT_     = 0.0f;  // 180 gradus pivot oynasi (s)
    float turnStepHz_ = 0.0f;  // joyida burilishda qadam chastotasi
    float strideYaw_  = 0.0f;  // harakat / tana burchagi (orientation warping)
    float groundDs_   = 0.0f;  // shu kadrda HAQIQATDA bosib o'tilgan masofa (m)
    float crouchSpeed_ = 1.20f;
    // Vizual ildiz amortizatsiyasi - FAQAT chizish uchun, fizikaga tegmaydi
    float visYOff_ = 0.0f, visYVel_ = 0.0f, prevPosY_ = 0.0f;
    MoveState state_ = MoveState::Idle;
    Profile   profile_ = Profile::Low;
    bool  grounded_ = true, crouched_ = false;

    // holat taymerlari
    float stateTime_ = 0.0f, actionDur_ = 0.0f;
    Vec3  actionFrom_{0, 0, 0}, actionTo_{0, 0, 0};
    float actionFromYaw_ = 0.0f, actionToYaw_ = 0.0f;
    float fallStartY_ = 0.0f, coyote_ = 0.0f, jumpBuffer_ = 0.0f;

    // osilib turish
    Vec3  hangNormal_{0, 0, 1};
    float climbT_ = 0.0f;

    float noise_ = 0.0f;
    NoiseEvent noiseEv_;

    float walkSpeed_ = 1.35f, jogSpeed_ = 3.30f, sprintSpeed_ = 6.40f;
    float radius_ = 0.42f, bodyH_ = 1.82f;

    // jang holati
    const Vec3* lock_ = nullptr;
    int   combo_ = 0;
    float comboWindow_ = 0.0f;
    bool  strikeFired_ = false;
    Attack pending_;
    Vec3  execTarget_{0, 0, 0};
    // Animatsiya: klipni qachon BOSHIDAN qo'yish kerakligini aniqlash uchun
    MoveState animState_ = MoveState::Idle;
    float     animTime_  = -1.0f;

    // --- Kamon ---
    float   drawT_     = 0.0f;   // 0..1 tortish
    float   aimBlend_  = 0.0f;   // 0..1 kamera/HUD aralashuvi
    float   shake_     = 0.0f;   // 0..1 titrash kuchi
    float   shakeT_    = 0.0f;   // titrash fazasi (determinatsiyalangan)
    float   camYaw_    = 0.0f;   // oxirgi kadrdagi kamera yo'nalishi
    float   camPitch_  = 0.0f;   // oxirgi kadrdagi kamera qiyaligi
    int     arrows_    = 12, arrowsMax_ = 12;
    float   slope_     = 0.0f;   // yer nishabi harakat yo'nalishida (tan), silliqlangan
    bool    shotFired_ = true;   // MUHIM: true — bo'sh pendingShot_ iste'mol qilinmasin
    BowShot pendingShot_;
    int   execVictim_ = -1;
};

} // namespace ert
