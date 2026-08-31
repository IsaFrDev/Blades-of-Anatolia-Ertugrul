#include "ertugrul/subsystems/QuestManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace ert {

QuestManager& QuestManager::get() {
    static QuestManager instance;
    return instance;
}

// Full JSON parser using nlohmann::json
void QuestManager::loadEpisodes(const std::string& filepath) {
    episodes_.clear();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "XATO: JSON fayl topilmadi: " << filepath << "\n";
        return;
    }
    
    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON xatosi: " << e.what() << '\n';
        return;
    }

    if (!j.contains("episodes")) {
        std::cerr << "XATO: 'episodes' massivi topilmadi.\n";
        return;
    }

    for (const auto& epJson : j["episodes"]) {
        Episode ep;
        ep.id = epJson.value("id", "Noma'lum");
        ep.title = epJson.value("loc_key_title", "Noma'lum");
        
        if (epJson.contains("anchor") && epJson["anchor"].contains("year_gregorian")) {
            ep.historicalYear = std::to_string(epJson["anchor"]["year_gregorian"].get<int>());
        }
        
        if (epJson.contains("intro") && epJson["intro"].contains("loc_key_description")) {
            ep.introText = epJson["intro"]["loc_key_description"].get<std::string>();
        }
        
        ep.cliffhanger = epJson.value("loc_key_cliffhanger", "");
        
        ep.archetype = epJson.value("archetype", "");
        ep.difficultyTier = epJson.value("difficulty_tier", 1);
        
        if (epJson.contains("enemy_composition") && epJson["enemy_composition"].contains("max_simultaneous")) {
            ep.maxSimultaneousEnemies = epJson["enemy_composition"]["max_simultaneous"].get<int>();
        }
        
        if (epJson.contains("mih_beat") && epJson["mih_beat"].contains("kind")) {
            ep.mihBeatKind = epJson["mih_beat"]["kind"].get<std::string>();
        }
        
        episodes_.push_back(ep);
    }
    
    std::cout << "Muvaffaqiyatli: " << episodes_.size() << " ta epizod yuklandi (V2 tizimi)!\n";
}

void QuestManager::printAllEpisodes() const {
    std::cout << "--- BARCHA 23 TA EPIZODLAR ---\n";
    for (const auto& ep : episodes_) {
        std::cout << "[" << ep.historicalYear << "] " << ep.id << ": " << ep.title << "\n";
    }
}

void QuestManager::startEpisode(const std::string& episodeId) {
    currentEpisodeId_ = episodeId;
    std::cout << "\n>>> " << episodeId << " - EPIZOD BOSHLANDI <<<\n";
}

} // namespace ert
