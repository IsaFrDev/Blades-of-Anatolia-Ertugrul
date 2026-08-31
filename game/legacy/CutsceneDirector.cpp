#include "ertugrul/subsystems/CutsceneDirector.h"
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace ert {

CutsceneDirector& CutsceneDirector::get() {
    static CutsceneDirector instance;
    return instance;
}

void CutsceneDirector::playScene(const std::string& sceneId) {
    playing_ = true;
    timer_ = 0.0f;
    currentScene_ = sceneId;
    currentSubtitle_ = "";
}

void CutsceneDirector::update(float dt) {
    if (!playing_) return;
    timer_ += dt;

    if (currentScene_ == "e1_intro") {
        if (timer_ < 3.0f) {
            currentSubtitle_ = "Ertug'rul: Biz Qayi edik, ammo yerimiz qolmadi...";
        } else if (timer_ < 6.0f) {
            currentSubtitle_ = "Turgut Alp: Beyim, ovga chiqib kiyik topmasak obamiz och qoladi!";
        } else if (timer_ < 9.0f) {
            currentSubtitle_ = "Ertug'rul: Unda ovni boshladik, Turgut! O'rmonga qarab yuramiz.";
        } else {
            playing_ = false;
            currentSubtitle_ = "";
        }
    }
}

std::string CutsceneDirector::getCurrentSubtitle() const {
    return currentSubtitle_;
}

}
