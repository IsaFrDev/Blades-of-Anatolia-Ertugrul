#pragma once
#include <string>

namespace ert {

class CutsceneDirector {
public:
    static CutsceneDirector& get();
    void playScene(const std::string& sceneId);
    void update(float dt);
    
    // UI (Subtitrlar)
    std::string getCurrentSubtitle() const;
    bool isPlaying() const { return playing_; }

private:
    bool playing_ = false;
    float timer_ = 0.0f;
    std::string currentScene_;
    std::string currentSubtitle_;
};

}
