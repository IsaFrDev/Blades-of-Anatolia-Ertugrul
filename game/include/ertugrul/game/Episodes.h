#pragma once
// data/episodes/episodes_v2.json (48 epizod, 4 mavsum) yuklovchi.
#include <string>
#include <vector>

namespace ert {

struct EpisodeIntro {
    std::string archetype;        // WALK / RIDE / COUNCIL / ...
    int         durationSec = 120;
    std::string dateDisplay;      // OVERLAY / NONE
    std::string locDescription;   // ep.epXXX.intro
    bool        allowMovement = true;
    bool        allowCombat   = false;
    float       directorCameraWeight = 0.35f;
};

struct EpisodeAnchor {
    std::string hijri, gregorian, confidence, locNote;
    int         year = 0;
};

struct Episode {
    std::string id, seasonId;
    int         globalIndex = 0, seasonIndex = 0;
    std::string locTitle, locSynopsis, locCliffhanger;
    EpisodeAnchor anchor;
    EpisodeIntro  intro;
    std::string archetype;        // INVESTIGATION / SIEGE / ...
    int         difficultyTier = 1;
    int         estimatedMinutes = 0;
    int         maxSimultaneous = 0;
    std::string mihKind, mihLocKey;
    std::string season, timeOfDay, weather, region;
    std::string musicCue, ambienceTag;
    std::string retentionHook;
    std::vector<std::string> characters, levels, codexUnlocks, unlocks, prerequisites, mechanics, traversal;
    // Ushbu epizod uchun ochilish sahnasi id'si (data/cutscenes ichida)
    std::string cutsceneId;
};

struct Season {
    std::string id, locTitle;
    std::vector<size_t> episodes;   // EpisodeDb::all() indekslari
};

class EpisodeDb {
public:
    static EpisodeDb& get();

    bool load(const std::string& jsonPath);
    bool loaded() const;

    const std::vector<Episode>& all() const;
    const std::vector<Season>&  seasons() const;
    size_t count() const;

    const Episode* byId(const std::string& id) const;
    const Episode* byIndex(size_t i) const;
    const Season*  seasonById(const std::string& id) const;

    // Lokalizatsiyalangan sarlavha (kalit topilmasa o'qiladigan zaxira matn)
    std::string title(const Episode& e) const;
    std::string synopsis(const Episode& e) const;
    std::string cliffhanger(const Episode& e) const;
    std::string introText(const Episode& e) const;

    // Diagnostika
    const std::string& lastError() const;

private:
    EpisodeDb() = default;
};

} // namespace ert
