#pragma once
// Epizod jangi: maqsadlar, to'lqinlar, nazorat nuqtalari, muvaffaqiyatsizlik va qayta tug'ilish.
//
// TO'LIQ MANTIQ:
//   1. Epizod tanlanadi -> yuklash -> CUTSCENE (intro)
//   2. Cutscene tugagach ENCOUNTER boshlanadi: maqsadlar ko'rsatiladi,
//      birinchi to'lqin dushmanlari paydo bo'ladi
//   3. O'yinchi jang qiladi; maqsad bajarilsa keyingi to'lqin va NAZORAT NUQTASI
//   4a. Hamma maqsad bajarilsa -> EPIZOD TUGADI -> cliffhanger -> keyingi epizod ochiladi
//   4b. O'yinchi o'lsa -> MUVAFFAQIYATSIZ -> oxirgi nazorat nuqtasidan QAYTA BOSHLASH
//       (o'yinchi tiklanadi, joriy to'lqin dushmanlari yangidan, bajarilgan
//        to'lqinlar saqlanadi)
//   4c. Yashirinlik shart bo'lgan epizodda sezilsa -> shart buziladi (lekin jang davom etadi)
#include <string>
#include <vector>
#include "ertugrul/core/Math.h"
#include "ertugrul/game/Enemy.h"

namespace ert {

struct Episode;
class Level;
class PhysicsWorld;
class Character;

enum class ObjectiveKind {
    DefeatAll = 0,     // to'lqindagi barcha dushmanlarni yo'q qilish
    DefeatCount,       // N ta dushmanni yo'q qilish
    ReachPoint,        // belgilangan nuqtaga yetish
    SurviveTime,       // N sekund omon qolish
    StayUndetected,    // sezilmaslik (buzilsa "failed", lekin epizod davom etadi)
    Assassinate,       // nishonni yashirincha o'ldirish
    Count
};

struct Objective {
    ObjectiveKind kind = ObjectiveKind::DefeatAll;
    std::string   locKey;                 // "ui.obj.defeat_all"
    int           target   = 0;           // nechta / necha sekund
    int           progress = 0;
    bool          done     = false;
    bool          failed   = false;
    bool          optional = false;
    Vec3          point{0, 0, 0};
    float         radius = 4.0f;
    // Ekranda ko'rsatiladigan matn ("Dushmanlarni yo'q qiling  3/5")
    std::string   text() const;
};

struct Wave {
    std::vector<EnemySpawn> spawns;
    float                   delay = 0.0f;   // oldingi to'lqin tugagach kutish
    std::string             locKey;         // to'lqin nomi (ixtiyoriy)
};

enum class EncounterState {
    Inactive = 0,
    Briefing,      // maqsadlar ko'rsatilmoqda (qisqa)
    Fighting,
    WaveCleared,
    Cleared,       // epizod bajarildi
    Failed         // o'yinchi halok bo'ldi
};

// Epizod natijasi — statistikada va yakuniy ekranda ko'rsatiladi
struct EncounterResult {
    int   kills       = 0;
    int   deaths      = 0;
    float timeSec     = 0.0f;
    bool  undetected  = true;
    int   wavesDone   = 0;
    float bestHealth  = 100.0f;
};

class Encounter {
public:
    static Encounter& get();

    // Epizod ma'lumotidan jang rejasini quradi (arxetip, qiyinlik, max_simultaneous).
    // Cutscene TUGAGANDAN keyin chaqiriladi.
    bool begin(const Episode& ep, Level& level, PhysicsWorld& phys, Character& player);
    void stop();

    void update(float dt);
    // Zarba qaytarmasi taymerlarini HAQIQIY dt bilan yuritadi. App uni
    // update(gdt) dan OLDIN chaqiradi — aks holda muzlash o'zini cho'zadi.
    void  tickFeedback(float realDt);
    float timeScale() const;        // 1.0 normal, muzlash paytida 0.12
    // Kamera silkinishi 0..1. Muzlashdan MUSTAQIL — haqiqiy vaqtda so'nadi,
    // aks holda muzlash paytida kamera ham to'xtab qolardi.
    float cameraShake() const;
    // O'q yig'ib olinganda HUD hisoblagichi yonib o'chadi (0..1, haqiqiy vaqtda so'nadi)
    float pickupFlash() const;
    void draw();                       // dunyodagi belgilar (maqsad markeri, dushman ko'rsatkichlari)

    // O'yinchi halok bo'lganda chaqiriladi: oxirgi nazorat nuqtasidan tiklaydi
    void restartFromCheckpoint();
    // Butun epizodni boshidan
    void restartEpisode();

    EncounterState state() const;
    const std::vector<Objective>& objectives() const;
    const EncounterResult&        result() const;
    EnemyManager&                 enemies();
    const EnemyManager&           enemies() const;

    int   waveIndex()  const;
    int   waveCount()  const;
    int   aliveEnemies() const;
    float stateTime()  const;
    const std::string& episodeId() const;
    // Nazorat nuqtasi nomi (HUD da "Nazorat nuqtasi saqlandi")
    const std::string& checkpointName() const;
    float checkpointFlash() const;     // 0..1, yangi CP dan keyin so'nadi

    // Lock-on nishoni (kamera va HUD uchun); yo'q bo'lsa nullptr
    Enemy* lockTarget() const;
    void   cycleLockTarget();
    void   clearLockTarget();

private:
    Encounter() = default;
};

// ---------------------------------------------------------------------------
// O'yinchi taraqqiyoti — saves/progress.json
// ---------------------------------------------------------------------------
class Progress {
public:
    static Progress& get();

    bool load(const std::string& path);
    bool save(const std::string& path) const;

    bool completed(const std::string& episodeId) const;
    bool unlocked(const std::string& episodeId) const;   // birinchisi doim ochiq
    void markCompleted(const std::string& episodeId, const EncounterResult& r);
    void addDeath(const std::string& episodeId);

    // Iymon epizodlar orasida saqlanadi (saves/progress.json)
    float faith() const;
    void  setFaith(float v);

    int  totalKills() const;
    int  totalDeaths() const;
    int  completedCount() const;
    const std::string& lastEpisode() const;
    void setLastEpisode(const std::string& id);
    void reset();

private:
    Progress() = default;
};

} // namespace ert
