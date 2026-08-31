#pragma once
// Ma'lumotga asoslangan cutscene: aktyorlar yuradi, kamera harakatlanadi,
// subtitr + ovoz sinxron chiqadi.  data/cutscenes/*.json
#include <string>
#include <vector>
#include "ertugrul/core/Math.h"
#include "ertugrul/gfx/Skin.h"

namespace ert {

struct CutActorKey {
    float       t    = 0.0f;      // sahna vaqti (soniya)
    Vec3        pos{0,0,0};
    float       yaw  = 0.0f;      // gradus
    std::string clip = "Idle";    // AnimClip nomi
};

struct CutActor {
    std::string id;               // sahna ichidagi nom, masalan "ertugrul"
    std::string charId;           // cast.json dagi id (ovoz profili uchun)
    std::string model;            // .obj yo'li
    std::string locName;          // ismi uchun lokalizatsiya kaliti
    float       scale  = 1.8f;    // metrdagi balandlik
    float       tint[3] = {1,1,1};
    std::vector<CutActorKey> keys;
};

struct CutCameraKey {
    float t = 0.0f;
    Vec3  pos{0,3,6};
    Vec3  look{0,1.6f,0};
    float fov = 45.0f;
};

struct CutLine {
    float       t = 0.0f;         // boshlanish vaqti
    float       dur = 0.0f;       // 0 = matn uzunligidan hisoblanadi
    std::string actorId;          // qaysi aktyor gapiradi
    std::string locKey;           // subtitr kaliti
    std::string voId;             // ovoz fayli id (bo'sh bo'lsa locKey ishlatiladi)
};

struct CutScene {
    std::string id;
    std::string episodeId;
    std::string levelId;
    float       duration = 0.0f;   // 0 = kalitlardan hisoblanadi
    std::string musicCue, ambienceTag;
    bool        letterbox = true;
    float       fadeIn = 1.0f, fadeOut = 1.0f;
    std::string timeOfDay = "day";      // dawn/day/dusk/night
    std::string weather   = "clear";
    std::vector<CutActor>     actors;
    std::vector<CutCameraKey> camera;
    std::vector<CutLine>      lines;
};

class CutsceneDirector {
public:
    static CutsceneDirector& get();

    bool loadDirectory(const std::string& dir);       // data/cutscenes
    bool loadFile(const std::string& path);
    // Sahna topilmasa, epizod ma'lumotidan avtomatik sahna quradi (hech qachon bo'sh qolmaydi)
    const CutScene* find(const std::string& sceneId) const;
    const CutScene* findForEpisode(const std::string& episodeId) const;
    void  registerGenerated(const CutScene& s);

    bool play(const std::string& sceneId);
    bool playScene(const CutScene& s);
    void stop();
    void skip();                       // butun sahnani tashlab ketish
    void advance();                    // joriy replikani tezlashtirish
    void update(float dt);

    bool  isPlaying() const;
    bool  finished() const;
    float time() const;
    float duration() const;
    const CutScene* current() const;

    // Render uchun holat
    struct ActorState {
        const CutActor* def = nullptr;
        Vec3   pos{0,0,0};
        float  yaw = 0.0f;
        AnimClip clip = AnimClip::Idle;
        float  speed = 0.0f;       // m/s (animatsiyani tanlash uchun)
        bool   speaking = false;
        SkinnedModel* model = nullptr;
    };
    const std::vector<ActorState>& actors() const;

    Vec3  cameraPos()  const;
    Vec3  cameraLook() const;
    float cameraFov()  const;

    const std::string& subtitle() const;      // lokalizatsiyalangan matn ("" = yo'q)
    const std::string& speakerName() const;   // lokalizatsiyalangan ism
    float letterbox() const;                  // 0..1
    float fade() const;                       // 0..1 (1 = to'liq qora)

private:
    CutsceneDirector() = default;
};

} // namespace ert
