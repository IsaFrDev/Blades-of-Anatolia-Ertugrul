#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ert {

struct Episode {
    std::string id;
    std::string title;
    std::string historicalYear;
    std::string introText;
    std::string cliffhanger;
    
    // New V2 fields
    std::string archetype;
    int difficultyTier;
    int maxSimultaneousEnemies;
    std::string mihBeatKind;
};

class QuestManager {
public:
    static QuestManager& get();
    void loadEpisodes(const std::string& filepath);
    void printAllEpisodes() const;
    void startEpisode(const std::string& episodeId);
    const std::vector<Episode>& getEpisodes() const { return episodes_; }

private:
    std::vector<Episode> episodes_;
    std::string currentEpisodeId_;
};

} // namespace ert
