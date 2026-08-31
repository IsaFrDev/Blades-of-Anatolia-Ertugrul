// Cutscene.cpp — ma'lumotga asoslangan kino sahna rejissyori.
// Aktyorlar haqiqatan yuradi (driveByLocomotion), kamera silliq harakatlanadi,
// subtitr + ovoz sinxron chiqadi.  Ma'lumot: data/cutscenes/*.json
//
// Muhim: ushbu fayl faqat ertugrul/game/Cutscene.h kontraktini amalga oshiradi.
// Header'da a'zo o'zgaruvchilar yo'q (faqat interfeys), shuning uchun butun holat
// shu .cpp ichidagi anonim namespace + funksiya-lokal static'da saqlanadi.

#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>   // FindFirstFileA / FindNextFileA (GL header'lardan oldin)

#include "ertugrul/game/Cutscene.h"
#include "ertugrul/gfx/Mesh.h"
#include "ertugrul/gfx/Skin.h"
#include "ertugrul/audio/Audio.h"
#include "ertugrul/audio/Voice.h"
#include "ertugrul/loc/Loc.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace ert {
namespace {

using nlohmann::json;

// ---------------------------------------------------------------------------
// Doimiylar
// ---------------------------------------------------------------------------

// "yaw berilmagan" sentinel qiymati. Haqiqiy yaw hech qachon -900 dan kichik bo'lmaydi.
constexpr float kAutoYaw        = -1000.0f;
constexpr float kLetterboxTime  = 0.35f;   // letterbox ochilish/yopilish vaqti
constexpr float kMoveThreshold  = 0.40f;   // shu tezlikdan past = "turibdi" (m/s)
constexpr float kLocoOverride   = 1.10f;   // shu tezlikdan yuqori = yurish klipi ustun
constexpr float kMaxSpeed       = 9.0f;    // tezlik chegarasi (sakrashlarga qarshi)
constexpr float kDefaultLineDur = 2.6f;    // ovoz ham, dur ham yo'q bo'lsa

inline bool isAutoYaw(float y) { return y < -900.0f; }

// ---------------------------------------------------------------------------
// Yordamchi: yo'lni bir nechta ildizdan izlash (exe build/ ichidan ishga tushishi mumkin)
// ---------------------------------------------------------------------------

bool fileExists(const std::string& p) {
    if (p.empty()) return false;
    DWORD a = ::GetFileAttributesA(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool dirExists(const std::string& p) {
    if (p.empty()) return false;
    DWORD a = ::GetFileAttributesA(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

const char* const kRoots[] = { "", "./", "../", "../../", "../../../", "../../../../" };

// Fayl uchun mavjud yo'lni topadi; topilmasa asl matnni qaytaradi.
std::string resolveFile(const std::string& rel) {
    if (rel.empty()) return rel;
    for (const char* r : kRoots) {
        std::string cand = std::string(r) + rel;
        if (fileExists(cand)) return cand;
    }
    return rel;
}

std::string resolveDir(const std::string& rel) {
    if (rel.empty()) return rel;
    for (const char* r : kRoots) {
        std::string cand = std::string(r) + rel;
        if (dirExists(cand)) return cand;
    }
    return rel;
}

// "a\\b" -> "a/b"
std::string normSlashes(std::string s) {
    for (char& c : s) if (c == '\\') c = '/';
    return s;
}

std::string lower(std::string s) {
    for (char& c : s) if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    return s;
}

// ---------------------------------------------------------------------------
// JSON o'qish yordamchilari — HECH QACHON istisno tashlamaydi
// ---------------------------------------------------------------------------

std::string jstr(const json& j, const char* key, const std::string& def) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return def;
    return it->get<std::string>();
}

float jnum(const json& j, const char* key, float def) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || !it->is_number()) return def;
    return it->get<float>();
}

bool jbool(const json& j, const char* key, bool def) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || !it->is_boolean()) return def;
    return it->get<bool>();
}

Vec3 jvec3(const json& j, const char* key, const Vec3& def) {
    if (!j.is_object()) return def;
    auto it = j.find(key);
    if (it == j.end() || !it->is_array() || it->size() < 3) return def;
    Vec3 v = def;
    const json& a = *it;
    if (a[0].is_number()) v.x = a[0].get<float>();
    if (a[1].is_number()) v.y = a[1].get<float>();
    if (a[2].is_number()) v.z = a[2].get<float>();
    return v;
}

// ---------------------------------------------------------------------------
// Rejissyor holati (butun holat shu yerda — header'da a'zolar yo'q)
// ---------------------------------------------------------------------------

struct Runtime {
    // Kutubxona: ko'rsatkichlar barqaror bo'lishi uchun unique_ptr
    std::vector<std::unique_ptr<CutScene>> library;

    // Joriy ijro
    CutScene         playing;           // sahnaning o'z nusxasi (tashqi havolaga bog'lanmaymiz)
    const CutScene*  cur      = nullptr;
    bool             isPlaying = false;
    bool             finished  = true;
    float            time      = 0.0f;
    float            duration  = 0.0f;

    // Aktyorlar
    std::vector<CutsceneDirector::ActorState>   actors;
    std::vector<std::unique_ptr<SkinnedModel>>  models;
    std::vector<Vec3>  prevPos;
    std::vector<float> smoothSpeed;
    std::vector<float> talkVal;
    bool  warpFrame = false;            // advance()/skip() dan keyin tezlikni nolga tushirish

    // Kamera
    Vec3  camPos{0.0f, 3.0f, 6.0f};
    Vec3  camLook{0.0f, 1.6f, 0.0f};
    float camFov  = 45.0f;
    bool  camInit = false;
    float autoOrbit = 0.0f;

    // Replikalar
    std::string subtitle;
    std::string speakerName;
    std::string speakerActorId;
    int    lineIdx  = -1;
    float  lineEnd  = 0.0f;
    size_t nextLine = 0;

    // Post
    float letterbox = 0.0f;
    float fade      = 0.0f;

    // Bo'sh satr — const& qaytarish uchun barqaror obyekt
    std::string emptyStr;

    // Audio ovoz id'lari (to'xtatish uchun)
    int ambienceVoice = 0;
};

Runtime& RT() {
    static Runtime rt;
    return rt;
}

// ---------------------------------------------------------------------------
// Sahna yordamchilari
// ---------------------------------------------------------------------------

void sortScene(CutScene& s) {
    for (CutActor& a : s.actors) {
        std::stable_sort(a.keys.begin(), a.keys.end(),
                         [](const CutActorKey& x, const CutActorKey& y) { return x.t < y.t; });
    }
    std::stable_sort(s.camera.begin(), s.camera.end(),
                     [](const CutCameraKey& x, const CutCameraKey& y) { return x.t < y.t; });
    std::stable_sort(s.lines.begin(), s.lines.end(),
                     [](const CutLine& x, const CutLine& y) { return x.t < y.t; });
}

// duration 0 bo'lsa: barcha kalitlar va replikalarning eng kech tugash vaqti + 1 sek
float computeDuration(const CutScene& s) {
    float last = 0.0f;
    for (const CutActor& a : s.actors)
        for (const CutActorKey& k : a.keys) last = std::max(last, k.t);
    for (const CutCameraKey& c : s.camera) last = std::max(last, c.t);
    for (const CutLine& l : s.lines)
        last = std::max(last, l.t + (l.dur > 0.0f ? l.dur : kDefaultLineDur));
    return last + 1.0f;
}

// ---------------------------------------------------------------------------
// JSON -> CutScene
// ---------------------------------------------------------------------------

bool parseActor(const json& j, CutActor& a) {
    if (!j.is_object()) return false;
    a.id      = jstr(j, "id", "");
    a.charId  = jstr(j, "char", jstr(j, "char_id", a.id));
    a.model   = normSlashes(jstr(j, "model", ""));
    a.locName = jstr(j, "loc_name", jstr(j, "locName", ""));
    a.scale   = jnum(j, "scale", 1.8f);
    if (a.scale < 0.2f || a.scale > 6.0f) a.scale = 1.8f;   // aqlga sig'adigan chegaralar

    Vec3 tint = jvec3(j, "tint", Vec3(1.0f, 1.0f, 1.0f));
    a.tint[0] = clampf(tint.x, 0.0f, 4.0f);
    a.tint[1] = clampf(tint.y, 0.0f, 4.0f);
    a.tint[2] = clampf(tint.z, 0.0f, 4.0f);

    if (a.id.empty()) return false;

    auto itk = j.find("keys");
    if (itk != j.end() && itk->is_array()) {
        a.keys.reserve(itk->size());
        for (const json& jk : *itk) {
            if (!jk.is_object()) continue;
            CutActorKey k;
            k.t    = jnum(jk, "t", 0.0f);
            k.pos  = jvec3(jk, "pos", Vec3(0.0f, 0.0f, 0.0f));
            k.yaw  = jnum(jk, "yaw", kAutoYaw);      // berilmasa — avtomatik yo'nalish
            k.clip = jstr(jk, "clip", "Idle");
            a.keys.push_back(k);
        }
    }
    return true;
}

bool parseScene(const json& j, CutScene& s) {
    if (!j.is_object()) return false;

    s.id = jstr(j, "id", jstr(j, "scene_id", ""));
    if (s.id.empty()) return false;

    s.episodeId   = jstr(j, "episode", jstr(j, "episode_id", jstr(j, "episodeId", "")));
    s.levelId     = jstr(j, "level",   jstr(j, "level_id",   jstr(j, "levelId",   "")));
    s.duration    = jnum(j, "duration", 0.0f);
    s.musicCue    = jstr(j, "music",    jstr(j, "music_cue", ""));
    s.ambienceTag = jstr(j, "ambience", jstr(j, "ambience_tag", ""));
    s.letterbox   = jbool(j, "letterbox", true);
    s.fadeIn      = std::max(0.0f, jnum(j, "fade_in",  1.0f));
    s.fadeOut     = std::max(0.0f, jnum(j, "fade_out", 1.0f));
    s.timeOfDay   = jstr(j, "time_of_day", jstr(j, "timeOfDay", "day"));
    s.weather     = jstr(j, "weather", "clear");

    auto ita = j.find("actors");
    if (ita != j.end() && ita->is_array()) {
        for (const json& ja : *ita) {
            CutActor a;
            if (parseActor(ja, a)) s.actors.push_back(std::move(a));
        }
    }

    auto itc = j.find("camera");
    if (itc != j.end() && itc->is_array()) {
        for (const json& jc : *itc) {
            if (!jc.is_object()) continue;
            CutCameraKey c;
            c.t    = jnum(jc, "t", 0.0f);
            c.pos  = jvec3(jc, "pos",  Vec3(0.0f, 3.0f, 6.0f));
            c.look = jvec3(jc, "look", Vec3(0.0f, 1.6f, 0.0f));
            c.fov  = clampf(jnum(jc, "fov", 45.0f), 12.0f, 110.0f);
            s.camera.push_back(c);
        }
    }

    auto itl = j.find("lines");
    if (itl != j.end() && itl->is_array()) {
        for (const json& jl : *itl) {
            if (!jl.is_object()) continue;
            CutLine l;
            l.t       = jnum(jl, "t", 0.0f);
            l.dur     = std::max(0.0f, jnum(jl, "dur", 0.0f));
            l.actorId = jstr(jl, "actor", jstr(jl, "actor_id", ""));
            l.locKey  = jstr(jl, "loc",   jstr(jl, "loc_key", ""));
            l.voId    = jstr(jl, "vo",    jstr(jl, "vo_id", ""));
            s.lines.push_back(l);
        }
    }

    sortScene(s);
    if (s.duration <= 0.0f) s.duration = computeDuration(s);
    return true;
}

// Kutubxonaga qo'shish (bir xil id bo'lsa — almashtirish, ko'rsatkich barqaror qoladi)
CutScene* addToLibrary(const CutScene& s) {
    Runtime& rt = RT();
    for (std::unique_ptr<CutScene>& p : rt.library) {
        if (p && p->id == s.id) { *p = s; return p.get(); }
    }
    rt.library.push_back(std::unique_ptr<CutScene>(new CutScene(s)));
    return rt.library.back().get();
}

// ---------------------------------------------------------------------------
// Zaxira (fallback) sahna — hech qachon bo'sh qolmaslik uchun protsedural quriladi
// ---------------------------------------------------------------------------

CutActorKey mkKey(float t, float x, float z, float yaw, const char* clip) {
    CutActorKey k;
    k.t   = t;
    k.pos = Vec3(x, 0.0f, z);
    k.yaw = yaw;
    k.clip = clip;
    return k;
}

CutScene buildProceduralScene(const std::string& sceneId, const std::string& episodeId) {
    CutScene s;
    s.id          = sceneId;
    s.episodeId   = episodeId;
    s.levelId     = "oba_camp";
    s.musicCue    = "MUS_THEME";
    s.ambienceTag = "Ambience.Autumn.Clear";
    s.letterbox   = true;
    s.fadeIn      = 1.2f;
    s.fadeOut     = 1.0f;
    s.timeOfDay   = "dawn";
    s.weather     = "clear";

    CutActor a;
    a.id      = "ertugrul";
    a.charId  = "ertugrul";
    a.model   = "assets/models/ottoman/ottoman.obj";
    a.locName = "chr.ertugrul.name";
    a.scale   = 1.82f;
    a.tint[0] = 1.0f; a.tint[1] = 0.96f; a.tint[2] = 0.88f;
    a.keys.push_back(mkKey(0.0f,  -6.0f,  4.0f, kAutoYaw, "Idle"));
    a.keys.push_back(mkKey(5.0f,  -2.0f,  2.6f, kAutoYaw, "Walk"));
    a.keys.push_back(mkKey(9.0f,   0.6f,  1.4f, kAutoYaw, "Walk"));
    a.keys.push_back(mkKey(13.0f,  1.4f,  0.6f, kAutoYaw, "Talk"));
    a.keys.push_back(mkKey(20.0f,  1.4f,  0.6f, kAutoYaw, "Idle"));
    s.actors.push_back(a);

    CutActor b;
    b.id      = "turgut";
    b.charId  = "turgut";
    b.model   = "assets/models/ottoman/ottoman.obj";
    b.locName = "chr.turgut.name";
    b.scale   = 1.86f;
    b.tint[0] = 0.88f; b.tint[1] = 0.86f; b.tint[2] = 0.82f;
    b.keys.push_back(mkKey(0.0f,   6.0f, -1.0f, kAutoYaw, "Idle"));
    b.keys.push_back(mkKey(5.0f,   3.0f,  0.0f, kAutoYaw, "Walk"));
    b.keys.push_back(mkKey(9.0f,   0.8f, -0.8f, kAutoYaw, "Walk"));
    b.keys.push_back(mkKey(13.0f,  0.2f, -1.2f, kAutoYaw, "Listen"));
    b.keys.push_back(mkKey(20.0f,  0.2f, -1.2f, kAutoYaw, "Idle"));
    s.actors.push_back(b);

    CutLine l1; l1.t = 2.0f;  l1.actorId = "ertugrul"; l1.locKey = "cut.generic.01";
    CutLine l2; l2.t = 7.0f;  l2.actorId = "turgut";   l2.locKey = "cut.generic.02";
    CutLine l3; l3.t = 12.0f; l3.actorId = "ertugrul"; l3.locKey = "cut.generic.03";
    CutLine l4; l4.t = 16.5f; l4.actorId = "turgut";   l4.locKey = "cut.generic.04";
    s.lines.push_back(l1);
    s.lines.push_back(l2);
    s.lines.push_back(l3);
    s.lines.push_back(l4);

    // Kamera kalitlari ataylab bo'sh — avtomatik "gapiruvchiga qarash" kamerasi ishlaydi.
    sortScene(s);
    s.duration = computeDuration(s);
    return s;
}

// generic_intro dan yoki protsedural tarzda zaxira sahna yaratib, kutubxonaga qo'shadi
const CutScene* makeFallback(const std::string& sceneId, const std::string& episodeId) {
    Runtime& rt = RT();
    const CutScene* tmpl = nullptr;
    for (const std::unique_ptr<CutScene>& p : rt.library) {
        if (p && p->id == "generic_intro") { tmpl = p.get(); break; }
    }
    CutScene s = tmpl ? *tmpl : buildProceduralScene(sceneId, episodeId);
    s.id        = sceneId.empty() ? std::string("generated_intro") : sceneId;
    s.episodeId = episodeId;
    if (s.duration <= 0.0f) s.duration = computeDuration(s);
    return addToLibrary(s);
}

// ---------------------------------------------------------------------------
// Interpolyatsiya yordamchilari
// ---------------------------------------------------------------------------

// t bo'yicha segmentni topadi; u — [0,1] normallashgan.
// Qaytadi: segment boshining indeksi (oxirgi kalitda size-1).
template <class KeyT>
size_t findSegment(const std::vector<KeyT>& keys, float t, float& u) {
    u = 0.0f;
    if (keys.empty()) return 0;
    if (keys.size() == 1 || t <= keys.front().t) return 0;
    if (t >= keys.back().t) { u = 0.0f; return keys.size() - 1; }
    size_t k = 0;
    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (t >= keys[i].t && t < keys[i + 1].t) { k = i; break; }
    }
    float span = keys[k + 1].t - keys[k].t;
    u = (span > 1e-5f) ? saturate((t - keys[k].t) / span) : 0.0f;
    return k;
}

// Catmull-Rom (3+ kalit) yoki chiziqli interpolyatsiya
template <class KeyT, class Getter>
Vec3 splinePos(const std::vector<KeyT>& keys, size_t k, float u, Getter get) {
    if (keys.empty()) return Vec3(0.0f, 0.0f, 0.0f);
    if (keys.size() == 1 || k + 1 >= keys.size()) return get(keys[keys.size() - 1]);
    if (keys.size() < 3) return lerp(get(keys[k]), get(keys[k + 1]), u);

    const size_t last = keys.size() - 1;
    const size_t i1 = k;
    const size_t i2 = k + 1;
    const size_t i0 = (k == 0) ? 0 : k - 1;
    const size_t i3 = (i2 + 1 > last) ? last : i2 + 1;
    return catmullRom(get(keys[i0]), get(keys[i1]), get(keys[i2]), get(keys[i3]), u);
}

// ---------------------------------------------------------------------------
// Aktyor namunasi (sample)
// ---------------------------------------------------------------------------

struct ActorSample {
    Vec3   pos{0, 0, 0};
    float  yaw     = kAutoYaw;
    bool   autoYaw = true;
    const std::string* clip = nullptr;   // kalitdagi klip nomi (nullptr = yo'q)
};

ActorSample sampleActor(const CutActor& a, float t) {
    ActorSample out;
    if (a.keys.empty()) return out;

    float u = 0.0f;
    size_t k = findSegment(a.keys, t, u);

    out.pos = splinePos(a.keys, k, u, [](const CutActorKey& kk) { return kk.pos; });

    const CutActorKey& k0 = a.keys[k];
    const CutActorKey& k1 = a.keys[(k + 1 < a.keys.size()) ? k + 1 : k];

    const bool a0 = isAutoYaw(k0.yaw), a1 = isAutoYaw(k1.yaw);
    if (a0 && a1)        { out.yaw = kAutoYaw;  out.autoYaw = true;  }
    else if (a0)         { out.yaw = k1.yaw;    out.autoYaw = false; }
    else if (a1)         { out.yaw = k0.yaw;    out.autoYaw = false; }
    else                 { out.yaw = lerpAngleDeg(k0.yaw, k1.yaw, u); out.autoYaw = false; }

    out.clip = &k0.clip;
    return out;
}

// Klip nomi "yurish bilan boshqariladigan"mi? (Idle/Walk/Run/bo'sh)
bool isLocomotionClip(const std::string& c) {
    if (c.empty()) return true;
    const std::string l = lower(c);
    return l == "idle" || l == "walk" || l == "run" || l == "auto" || l == "locomotion";
}

// ---------------------------------------------------------------------------
// Audio yordamchilari (fayl topilmasa — jimgina o'tib ketadi)
// ---------------------------------------------------------------------------

SoundRef tryLoadWav(const std::string& relNoExt) {
    if (relNoExt.empty()) return nullptr;
    std::string p = resolveFile(relNoExt + ".wav");
    if (!fileExists(p)) return nullptr;
    return Audio::loadWav(p);
}

// "Ambience.Autumn.Clear" -> nomzod yo'llar
void startSceneAudio(const CutScene& s) {
    Audio& au = Audio::get();
    if (!au.ready()) return;

    if (!s.musicCue.empty()) {
        SoundRef m = tryLoadWav("assets/audio/music/" + s.musicCue);
        if (!m) m = tryLoadWav("assets/audio/music/" + lower(s.musicCue));
        if (m) au.crossfadeMusic(m, 1.5f, 0.55f);
    }

    if (!s.ambienceTag.empty()) {
        std::string tag = s.ambienceTag;
        std::string underscored = tag;
        for (char& c : underscored) if (c == '.') c = '_';

        SoundRef amb = tryLoadWav("assets/audio/ambience/" + tag);
        if (!amb) amb = tryLoadWav("assets/audio/ambience/" + underscored);
        if (!amb) amb = tryLoadWav("assets/audio/ambience/" + lower(underscored));
        if (amb) {
            Runtime& rt = RT();
            if (rt.ambienceVoice) au.stop(rt.ambienceVoice);
            rt.ambienceVoice = au.play(amb, BUS_AMBIENCE, 0.45f, true, 1.0f);
        }
    }
}

// ---------------------------------------------------------------------------
// Aktyor modellarini tayyorlash
// ---------------------------------------------------------------------------

void buildActors(const CutScene& s) {
    Runtime& rt = RT();
    rt.actors.clear();
    rt.models.clear();
    rt.prevPos.clear();
    rt.smoothSpeed.clear();
    rt.talkVal.clear();

    const size_t n = s.actors.size();
    rt.actors.reserve(n);
    rt.models.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        const CutActor& a = s.actors[i];

        // 1) Modelni yuklash; topilmasa — protsedural zaxira (CRASH YO'Q)
        Mesh* mesh = nullptr;
        if (!a.model.empty()) {
            mesh = Mesh::get(a.model);
            if (!mesh) {
                std::string alt = resolveFile(a.model);
                if (alt != a.model) mesh = Mesh::get(alt);
            }
        }
        if (!mesh) mesh = Mesh::unitCylinder(12);   // zaxira gavda

        std::unique_ptr<SkinnedModel> sm(new SkinnedModel());
        bool ok = false;
        if (mesh) ok = sm->init(mesh);

        CutsceneDirector::ActorState st;
        st.def   = &s.actors[i];        // s == rt.playing, vektor endi o'zgarmaydi
        st.model = ok ? sm.get() : nullptr;

        // 2) Boshlang'ich holat — birinchi kalitdan
        ActorSample smp = sampleActor(a, 0.0f);
        st.pos   = smp.pos;
        st.yaw   = smp.autoYaw ? 0.0f : smp.yaw;
        st.clip  = AnimClip::Idle;
        st.speed = 0.0f;
        st.speaking = false;

        if (st.model) {
            const std::string c = smp.clip ? *smp.clip : std::string("Idle");
            st.clip = isLocomotionClip(c) ? AnimClip::Idle : animClipFromName(c);
            st.model->setClip(st.clip, 0.0f);
            st.model->setTalkIntensity(0.0f);
        }

        rt.models.push_back(std::move(sm));
        rt.actors.push_back(st);
        rt.prevPos.push_back(st.pos);
        rt.smoothSpeed.push_back(0.0f);
        rt.talkVal.push_back(0.0f);
    }

    // Aktyorlar bir-biriga qarab tursin (auto yaw bo'lsa)
    if (rt.actors.size() >= 2) {
        for (size_t i = 0; i < rt.actors.size(); ++i) {
            const CutActor& a = s.actors[i];
            if (a.keys.empty() || !isAutoYaw(a.keys.front().yaw)) continue;
            size_t other = (i == 0) ? 1 : 0;
            Vec3 d = rt.actors[other].pos - rt.actors[i].pos;
            if (lengthSq(d) > 1e-4f) rt.actors[i].yaw = yawFromDir(normalize(d));
        }
    }
}

// ---------------------------------------------------------------------------
// Replikalar
// ---------------------------------------------------------------------------

const CutActor* actorDefById(const CutScene& s, const std::string& id) {
    for (const CutActor& a : s.actors) if (a.id == id) return &a;
    return nullptr;
}

int actorIndexById(const CutScene& s, const std::string& id) {
    for (size_t i = 0; i < s.actors.size(); ++i)
        if (s.actors[i].id == id) return (int)i;
    return -1;
}

void beginLine(size_t idx) {
    Runtime& rt = RT();
    if (!rt.cur || idx >= rt.cur->lines.size()) return;
    const CutLine& l = rt.cur->lines[idx];

    rt.subtitle.clear();
    rt.speakerName.clear();
    rt.speakerActorId.clear();
    rt.lineIdx = -1;

    if (l.locKey.empty()) return;                       // matn yo'q — aktyor gapirmasin

    rt.subtitle = Loc::get().tr(l.locKey);
    if (rt.subtitle.empty()) return;

    const CutActor* def = actorDefById(*rt.cur, l.actorId);
    if (def && !def->locName.empty()) rt.speakerName = Loc::get().tr(def->locName);

    const std::string voId  = l.voId.empty() ? l.locKey : l.voId;
    const std::string chrId = def ? def->charId : std::string();

    float spoken = 0.0f;
    spoken = VoiceBank::get().speak(voId, rt.subtitle, chrId);

    float dur = l.dur;
    if (dur <= 0.0f) dur = (spoken > 0.05f) ? spoken : VoiceBank::estimateDuration(rt.subtitle);
    if (dur <= 0.05f) dur = kDefaultLineDur;

    rt.lineIdx        = (int)idx;
    rt.lineEnd        = rt.time + dur;
    rt.speakerActorId = l.actorId;
}

void endLine() {
    Runtime& rt = RT();
    rt.lineIdx = -1;
    rt.subtitle.clear();
    rt.speakerName.clear();
    rt.speakerActorId.clear();
}

// ---------------------------------------------------------------------------
// Kamera
// ---------------------------------------------------------------------------

float actorEyeHeight(const CutsceneDirector::ActorState& st) {
    const float h = st.def ? st.def->scale : 1.8f;
    return clampf(h, 0.6f, 3.0f) * 0.92f;
}

// Kamera kalitlari yo'q bo'lganda: "gapiruvchiga qarash" kinematik kamerasi
void updateAutoCamera(float dt) {
    Runtime& rt = RT();
    if (rt.actors.empty()) return;

    // 1) Nishon: gapirayotgan aktyor -> eng tez harakatlanayotgan -> birinchi
    int speaker = -1;
    if (!rt.speakerActorId.empty() && rt.cur)
        speaker = actorIndexById(*rt.cur, rt.speakerActorId);
    if (speaker < 0) {
        float best = 0.5f;
        for (size_t i = 0; i < rt.actors.size(); ++i) {
            if (rt.actors[i].speed > best) { best = rt.actors[i].speed; speaker = (int)i; }
        }
    }
    if (speaker < 0 || (size_t)speaker >= rt.actors.size()) speaker = 0;

    const CutsceneDirector::ActorState& sp = rt.actors[(size_t)speaker];
    const Vec3 spHead(sp.pos.x, sp.pos.y + actorEyeHeight(sp), sp.pos.z);

    // 2) Yelka egasi: gapiruvchiga eng yaqin boshqa aktyor
    int shoulder = -1;
    float bestD = 1e9f;
    for (size_t i = 0; i < rt.actors.size(); ++i) {
        if ((int)i == speaker) continue;
        float d = distanceXZ(rt.actors[i].pos, sp.pos);
        if (d < bestD && d > 0.35f) { bestD = d; shoulder = (int)i; }
    }

    rt.autoOrbit += dt;

    Vec3  wantPos;
    Vec3  wantLook = spHead;
    float wantFov  = 40.0f;

    // Sekin dolly (nafas oluvchi harakat)
    const float breathe = std::sin(rt.autoOrbit * 0.35f);

    if (shoulder >= 0 && bestD < 12.0f) {
        // --- Yelka ustidan (over-the-shoulder) ---
        const CutsceneDirector::ActorState& li = rt.actors[(size_t)shoulder];
        const Vec3 liHead(li.pos.x, li.pos.y + actorEyeHeight(li), li.pos.z);

        Vec3 fwd = spHead - liHead;
        fwd.y = 0.0f;
        fwd = normalize(fwd);
        if (lengthSq(fwd) < 1e-6f) fwd = Vec3(0.0f, 0.0f, 1.0f);
        const Vec3 right = normalize(cross(Vec3(0.0f, 1.0f, 0.0f), fwd));

        const float back = 1.35f + 0.22f * breathe;
        const float side = 0.72f;
        wantPos = liHead - fwd * back + right * side + Vec3(0.0f, 0.14f, 0.0f);
        wantFov = 38.0f;
    } else {
        // --- Uch choraklik ko'rinish + sekin orbit ---
        const float baseYaw = sp.yaw + 148.0f + 6.0f * breathe;
        const Vec3  dir     = dirFromYaw(baseYaw);
        const float radius  = 4.1f + 0.45f * breathe;
        wantPos = Vec3(sp.pos.x, 0.0f, sp.pos.z) + dir * radius
                + Vec3(0.0f, actorEyeHeight(sp) + 0.32f + 0.12f * breathe, 0.0f);
        wantFov = 42.0f;
    }

    // Yerdan pastga tushmasin
    if (wantPos.y < 0.35f) wantPos.y = 0.35f;

    if (!rt.camInit) {
        rt.camPos = wantPos; rt.camLook = wantLook; rt.camFov = wantFov;
        rt.camInit = true;
    } else {
        rt.camPos  = dampV(rt.camPos,  wantPos,  3.2f, dt);
        rt.camLook = dampV(rt.camLook, wantLook, 5.0f, dt);
        rt.camFov  = damp (rt.camFov,  wantFov,  3.0f, dt);
    }
}

void updateKeyedCamera(const CutScene& s, float t, float dt) {
    Runtime& rt = RT();
    if (s.camera.empty()) { updateAutoCamera(dt); return; }

    if (s.camera.size() == 1) {
        rt.camPos  = s.camera[0].pos;
        rt.camLook = s.camera[0].look;
        rt.camFov  = s.camera[0].fov;
        rt.camInit = true;
        return;
    }

    float u = 0.0f;
    const size_t k = findSegment(s.camera, t, u);

    rt.camPos  = splinePos(s.camera, k, u, [](const CutCameraKey& c) { return c.pos;  });
    rt.camLook = splinePos(s.camera, k, u, [](const CutCameraKey& c) { return c.look; });

    const CutCameraKey& c0 = s.camera[k];
    const CutCameraKey& c1 = s.camera[(k + 1 < s.camera.size()) ? k + 1 : k];
    rt.camFov  = lerpf(c0.fov, c1.fov, u);
    rt.camInit = true;
}

// ---------------------------------------------------------------------------
// Aktyorlarni yangilash
// ---------------------------------------------------------------------------

void updateActors(const CutScene& s, float t, float dt) {
    Runtime& rt = RT();
    const size_t n = std::min(rt.actors.size(), s.actors.size());
    if (n == 0) return;

    const bool lineActive = (rt.lineIdx >= 0) && !rt.subtitle.empty();
    int speakerIdx = -1;
    if (lineActive && !rt.speakerActorId.empty()) speakerIdx = actorIndexById(s, rt.speakerActorId);

    // 1-bosqich: pozitsiya, tezlik
    for (size_t i = 0; i < n; ++i) {
        CutsceneDirector::ActorState& st = rt.actors[i];
        const CutActor& def = s.actors[i];

        ActorSample smp = sampleActor(def, t);
        if (def.keys.empty()) smp.pos = st.pos;      // kalitsiz aktyor joyida qoladi

        // Tezlik — oldingi kadr pozitsiyasidan
        float raw = 0.0f;
        if (dt > 1e-5f && !rt.warpFrame) {
            raw = distanceXZ(smp.pos, rt.prevPos[i]) / dt;
            if (raw > kMaxSpeed) raw = kMaxSpeed;
            if (!(raw == raw)) raw = 0.0f;           // NaN himoyasi
        }
        rt.smoothSpeed[i] = damp(rt.smoothSpeed[i], raw, 12.0f, std::max(dt, 1e-4f));
        if (rt.smoothSpeed[i] < 0.02f) rt.smoothSpeed[i] = 0.0f;

        const Vec3 delta = smp.pos - rt.prevPos[i];
        rt.prevPos[i] = smp.pos;
        st.pos   = smp.pos;
        st.speed = rt.smoothSpeed[i];

        // --- YAW ---
        const bool moving = st.speed > kMoveThreshold;
        if (!smp.autoYaw) {
            // Kalitda aniq yaw berilgan — u ustun
            st.yaw = lerpAngleDeg(st.yaw, smp.yaw, 1.0f - std::exp(-10.0f * dt));
        } else if (moving && lengthSq(Vec3(delta.x, 0.0f, delta.z)) > 1e-6f) {
            // Harakat yo'nalishiga qarab yursin
            const float target = yawFromDir(normalize(Vec3(delta.x, 0.0f, delta.z)));
            st.yaw = lerpAngleDeg(st.yaw, target, 1.0f - std::exp(-8.0f * dt));
        }
        st.speaking = (lineActive && (int)i == speakerIdx);
    }

    // 2-bosqich: gapiruvchiga qarash (turgan aktyorlar boshini buradi)
    if (speakerIdx >= 0 && (size_t)speakerIdx < n) {
        const Vec3 spPos = rt.actors[(size_t)speakerIdx].pos;
        for (size_t i = 0; i < n; ++i) {
            if ((int)i == speakerIdx) continue;
            CutsceneDirector::ActorState& st = rt.actors[i];
            if (st.speed > kMoveThreshold) continue;                 // yurayotganini buzmaymiz
            const CutActor& def = s.actors[i];
            ActorSample smp = sampleActor(def, t);
            if (!smp.autoYaw) continue;                              // aniq yaw — tegmaymiz
            Vec3 d = spPos - st.pos; d.y = 0.0f;
            if (lengthSq(d) < 1e-4f) continue;
            const float target = yawFromDir(normalize(d));
            st.yaw = lerpAngleDeg(st.yaw, target, 1.0f - std::exp(-2.6f * dt));
        }
        // Gapiruvchi ham tinglovchiga qarasin (agar yurmayotgan va auto yaw bo'lsa)
        CutsceneDirector::ActorState& sp = rt.actors[(size_t)speakerIdx];
        ActorSample sSmp = sampleActor(s.actors[(size_t)speakerIdx], t);
        if (sSmp.autoYaw && sp.speed <= kMoveThreshold) {
            int nearest = -1; float bd = 1e9f;
            for (size_t i = 0; i < n; ++i) {
                if ((int)i == speakerIdx) continue;
                float d = distanceXZ(rt.actors[i].pos, sp.pos);
                if (d < bd) { bd = d; nearest = (int)i; }
            }
            if (nearest >= 0) {
                Vec3 d = rt.actors[(size_t)nearest].pos - sp.pos; d.y = 0.0f;
                if (lengthSq(d) > 1e-4f)
                    sp.yaw = lerpAngleDeg(sp.yaw, yawFromDir(normalize(d)), 1.0f - std::exp(-2.6f * dt));
            }
        }
    }

    // 3-bosqich: animatsiya klipi + gapirish intensivligi
    for (size_t i = 0; i < n; ++i) {
        CutsceneDirector::ActorState& st = rt.actors[i];
        const CutActor& def = s.actors[i];
        ActorSample smp = sampleActor(def, t);

        const std::string clipName = smp.clip ? *smp.clip : std::string("Idle");
        const bool locoManaged = isLocomotionClip(clipName);
        const bool fastMoving  = st.speed > kLocoOverride;

        AnimClip explicitClip = AnimClip::Idle;
        bool haveExplicit = false;
        if (!locoManaged) { explicitClip = animClipFromName(clipName); haveExplicit = true; }

        // Gapirayotgan aktyor "Listen" pozasida qolib ketmasin — u gapiryapti, tinglamayapti.
        if (haveExplicit && st.speaking && explicitClip == AnimClip::Listen) explicitClip = AnimClip::Talk;

        // Gapirish/tinglash klipi (kalitda aniq klip yo'q bo'lsa)
        if (!haveExplicit && st.speed <= kMoveThreshold) {
            if (st.speaking)            { explicitClip = AnimClip::Talk;   haveExplicit = true; }
            else if (speakerIdx >= 0)   { explicitClip = AnimClip::Listen; haveExplicit = true; }
        }

        SkinnedModel* m = st.model;
        if (!m) {
            // Model yo'q — faqat mantiqiy klipni yozib qo'yamiz
            st.clip = haveExplicit ? explicitClip
                                   : (st.speed > 2.7f ? AnimClip::Run
                                      : (st.speed > kMoveThreshold ? AnimClip::Walk : AnimClip::Idle));
        } else if (haveExplicit && !fastMoving) {
            // Aniq klip ustun (lekin tez yurganda oyoq sirg'anmasligi uchun locomotion g'olib)
            m->setClip(explicitClip, 0.22f);
            m->update(dt);
            st.clip = explicitClip;
        } else {
            // HAQIQIY YURISH: tezlikdan Walk/Run/Idle avtomatik tanlanadi
            const float before = m->animTime();
            m->driveByLocomotion(st.speed, dt);
            if (m->animTime() == before && dt > 0.0f) m->update(dt);  // faqat klip tanlagan bo'lsa
            // Kafolat: gavda siljiyotgan bo'lsa oyoq ham harakatlansin (oyoq sirg'anishiga qarshi).
            if (st.speed > kMoveThreshold) {
                if (m->clip() == AnimClip::Idle) {
                    m->setClip(st.speed > 2.7f ? AnimClip::Run : AnimClip::Walk, 0.20f);
                    m->setSpeedScale(clampf(st.speed / 1.35f, 0.55f, 1.8f));
                }
            } else {
                m->setSpeedScale(1.0f);
            }
            st.clip = m->clip();
        }

        // Gapirish intensivligi: gapiruvchi 1.0, tinglovchilar 0.15, jim sahnada 0
        float target = 0.0f;
        if (st.speaking)                 target = 1.0f;
        else if (speakerIdx >= 0)        target = 0.15f;
        rt.talkVal[i] = damp(rt.talkVal[i], target, 9.0f, std::max(dt, 1e-4f));
        if (m) m->setTalkIntensity(rt.talkVal[i]);
    }

    rt.warpFrame = false;
}

// ---------------------------------------------------------------------------
// Post effektlar
// ---------------------------------------------------------------------------

void updatePost(const CutScene& s, float t) {
    Runtime& rt = RT();

    // Letterbox: boshida 0.35 sek ichida 1 ga, oxirida 0 ga
    if (!s.letterbox) {
        rt.letterbox = 0.0f;
    } else {
        float up = saturate(t / kLetterboxTime);
        float dn = 1.0f;
        const float closeStart = rt.duration - kLetterboxTime;
        if (t > closeStart) dn = 1.0f - saturate((t - closeStart) / kLetterboxTime);
        rt.letterbox = smoothstepf(std::min(up, dn));
    }

    // Fade: boshida fadeIn davomida 1->0, oxirida fadeOut davomida 0->1
    float f = 0.0f;
    if (s.fadeIn > 0.001f)  f = std::max(f, 1.0f - smoothstepf(saturate(t / s.fadeIn)));
    if (s.fadeOut > 0.001f) {
        const float fs = rt.duration - s.fadeOut;
        if (t > fs) f = std::max(f, smoothstepf(saturate((t - fs) / s.fadeOut)));
    }
    rt.fade = saturate(f);
}

} // anonim namespace

// ===========================================================================
// CutsceneDirector — ommaviy interfeys
// ===========================================================================

CutsceneDirector& CutsceneDirector::get() {
    static CutsceneDirector inst;   // shaxsiy konstruktor a'zo funksiyada ochiq
    return inst;
}

// --- Yuklash ---------------------------------------------------------------

bool CutsceneDirector::loadFile(const std::string& path) {
    if (path.empty()) return false;

    const std::string full = resolveFile(normSlashes(path));
    std::ifstream in(full.c_str(), std::ios::binary);
    if (!in.good()) return false;

    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();
    if (text.empty()) return false;

    json root = json::parse(text, nullptr, false, true);   // istisnosiz + izohlarga ruxsat
    if (root.is_discarded()) return false;

    size_t added = 0;
    try {
        if (root.is_array()) {
            for (const json& j : root) {
                CutScene s;
                if (parseScene(j, s)) { addToLibrary(s); ++added; }
            }
        } else if (root.is_object() && root.contains("scenes") && root["scenes"].is_array()) {
            for (const json& j : root["scenes"]) {
                CutScene s;
                if (parseScene(j, s)) { addToLibrary(s); ++added; }
            }
        } else {
            CutScene s;
            if (parseScene(root, s)) { addToLibrary(s); ++added; }
        }
    } catch (...) {
        return added > 0;      // buzuq fayl o'yinni yiqitmasin
    }
    return added > 0;
}

bool CutsceneDirector::loadDirectory(const std::string& dir) {
    if (dir.empty()) return false;

    std::string base = normSlashes(dir);
    while (!base.empty() && (base.back() == '/' )) base.pop_back();
    base = resolveDir(base);
    if (!dirExists(base)) return false;

    const std::string pattern = base + "/*.json";

    WIN32_FIND_DATAA fd;
    ZeroMemory(&fd, sizeof(fd));
    HANDLE h = ::FindFirstFileA(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;

    size_t ok = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        const std::string name = fd.cFileName;
        if (name.empty() || name[0] == '.') continue;
        if (loadFile(base + "/" + name)) ++ok;
    } while (::FindNextFileA(h, &fd) != 0);

    ::FindClose(h);
    return ok > 0;
}

// --- Qidiruv ---------------------------------------------------------------

const CutScene* CutsceneDirector::find(const std::string& sceneId) const {
    Runtime& rt = RT();
    if (sceneId.empty()) return makeFallback("generated_intro", "");

    for (const std::unique_ptr<CutScene>& p : rt.library)
        if (p && p->id == sceneId) return p.get();

    // Kichik/katta harf farqi
    const std::string want = lower(sceneId);
    for (const std::unique_ptr<CutScene>& p : rt.library)
        if (p && lower(p->id) == want) return p.get();

    // Topilmasa — hech qachon bo'sh qolmaydi: zaxira sahna quriladi
    return makeFallback(sceneId, "");
}

const CutScene* CutsceneDirector::findForEpisode(const std::string& episodeId) const {
    Runtime& rt = RT();

    if (!episodeId.empty()) {
        for (const std::unique_ptr<CutScene>& p : rt.library)
            if (p && p->episodeId == episodeId) return p.get();

        const std::string want = lower(episodeId);
        for (const std::unique_ptr<CutScene>& p : rt.library)
            if (p && lower(p->episodeId) == want) return p.get();

        // Nom qoidasi bo'yicha: EP001 -> ep001_intro
        const std::string byName = want + "_intro";
        for (const std::unique_ptr<CutScene>& p : rt.library)
            if (p && lower(p->id) == byName) return p.get();
    }

    // generic_intro (yoki protsedural) asosida yangi sahna
    const std::string genId = episodeId.empty() ? std::string("generated_intro")
                                                : (lower(episodeId) + "_auto_intro");
    return makeFallback(genId, episodeId);
}

void CutsceneDirector::registerGenerated(const CutScene& s) {
    if (s.id.empty()) return;
    CutScene copy = s;
    sortScene(copy);
    if (copy.duration <= 0.0f) copy.duration = computeDuration(copy);
    addToLibrary(copy);
}

// --- Ijro ------------------------------------------------------------------

bool CutsceneDirector::play(const std::string& sceneId) {
    const CutScene* s = find(sceneId);
    if (!s) return false;
    return playScene(*s);
}

bool CutsceneDirector::playScene(const CutScene& s) {
    Runtime& rt = RT();

    VoiceBank::get().stopAll();

    // O'z nusxamiz — tashqi obyekt o'chsa ham xavfsiz
    rt.playing = s;
    sortScene(rt.playing);
    rt.duration = (rt.playing.duration > 0.0f) ? rt.playing.duration : computeDuration(rt.playing);
    if (rt.duration < 0.5f) rt.duration = 0.5f;
    rt.playing.duration = rt.duration;
    rt.cur = &rt.playing;

    rt.time      = 0.0f;
    rt.isPlaying = true;
    rt.finished  = false;
    rt.warpFrame = true;
    rt.nextLine  = 0;
    rt.lineIdx   = -1;
    rt.lineEnd   = 0.0f;
    rt.subtitle.clear();
    rt.speakerName.clear();
    rt.speakerActorId.clear();
    rt.letterbox = 0.0f;
    rt.fade      = (rt.playing.fadeIn > 0.001f) ? 1.0f : 0.0f;
    rt.camInit   = false;
    rt.autoOrbit = 0.0f;

    buildActors(rt.playing);

    // Boshlang'ich kamera
    if (!rt.playing.camera.empty()) {
        rt.camPos  = rt.playing.camera.front().pos;
        rt.camLook = rt.playing.camera.front().look;
        rt.camFov  = rt.playing.camera.front().fov;
        rt.camInit = true;
    } else {
        updateAutoCamera(0.0f);
    }

    startSceneAudio(rt.playing);
    return true;
}

void CutsceneDirector::stop() {
    Runtime& rt = RT();
    if (rt.isPlaying) VoiceBank::get().stopAll();
    rt.isPlaying = false;
    rt.finished  = true;
    rt.subtitle.clear();
    rt.speakerName.clear();
    rt.speakerActorId.clear();
    rt.lineIdx   = -1;
    rt.letterbox = 0.0f;

    Audio& au = Audio::get();
    if (rt.ambienceVoice && au.ready()) au.stop(rt.ambienceVoice);
    rt.ambienceVoice = 0;
    // Modellar keyingi playScene() gacha saqlanadi — oxirgi kadr xavfsiz chiziladi.
}

void CutsceneDirector::skip() {
    Runtime& rt = RT();
    if (!rt.isPlaying || !rt.cur) return;

    VoiceBank::get().stopAll();
    endLine();
    rt.nextLine = rt.cur->lines.size();

    const float fo = rt.cur->fadeOut;
    if (fo <= 0.01f) {
        rt.time     = rt.duration;
        rt.fade     = 1.0f;
        rt.letterbox = 0.0f;
        rt.isPlaying = false;
        rt.finished  = true;
        return;
    }
    // Fade bilan tugatish: oxirgi fadeOut oynasiga sakraymiz
    rt.time      = std::max(rt.time, rt.duration - fo);
    rt.warpFrame = true;
}

void CutsceneDirector::advance() {
    Runtime& rt = RT();
    if (!rt.isPlaying || !rt.cur) return;

    VoiceBank::get().stopAll();
    endLine();

    // Keyingi replika boshiga sakraymiz (faqat oldinga)
    if (rt.nextLine < rt.cur->lines.size()) {
        const float nt = rt.cur->lines[rt.nextLine].t;
        if (nt > rt.time) rt.time = nt;
        rt.warpFrame = true;
        return;
    }
    // Replika qolmadi — sahnani yakunlash (fade bilan)
    skip();
}

void CutsceneDirector::update(float dt) {
    Runtime& rt = RT();
    if (!rt.isPlaying || !rt.cur) return;

    if (!(dt == dt)) dt = 0.0f;                 // NaN
    dt = clampf(dt, 0.0f, 0.25f);               // ilib qolishdan keyin sakramaslik uchun
    rt.time += dt;

    const CutScene& s = *rt.cur;

    // --- Replikalar ---
    while (rt.nextLine < s.lines.size() && rt.time >= s.lines[rt.nextLine].t) {
        beginLine(rt.nextLine);
        ++rt.nextLine;
    }
    if (rt.lineIdx >= 0 && rt.time >= rt.lineEnd) endLine();

    // --- Aktyorlar ---
    updateActors(s, rt.time, dt);

    // --- Kamera ---
    updateKeyedCamera(s, rt.time, dt);

    // --- Letterbox / fade ---
    updatePost(s, rt.time);

    // --- Tugash ---
    if (rt.time >= rt.duration) {
        rt.time      = rt.duration;
        rt.isPlaying = false;
        rt.finished  = true;
        rt.fade      = 1.0f;
        rt.letterbox = 0.0f;
        endLine();
        VoiceBank::get().stopAll();

        Audio& au = Audio::get();
        if (rt.ambienceVoice && au.ready()) au.stop(rt.ambienceVoice);
        rt.ambienceVoice = 0;
    }
}

// --- Holat -----------------------------------------------------------------

bool  CutsceneDirector::isPlaying() const { return RT().isPlaying; }
bool  CutsceneDirector::finished()  const { return RT().finished;  }
float CutsceneDirector::time()      const { return RT().time;      }
float CutsceneDirector::duration()  const { return RT().duration;  }

const CutScene* CutsceneDirector::current() const { return RT().cur; }

const std::vector<CutsceneDirector::ActorState>& CutsceneDirector::actors() const {
    return RT().actors;
}

Vec3  CutsceneDirector::cameraPos()  const { return RT().camPos;  }
Vec3  CutsceneDirector::cameraLook() const { return RT().camLook; }
float CutsceneDirector::cameraFov()  const { return RT().camFov;  }

const std::string& CutsceneDirector::subtitle() const {
    Runtime& rt = RT();
    return (rt.lineIdx >= 0) ? rt.subtitle : rt.emptyStr;
}

const std::string& CutsceneDirector::speakerName() const {
    Runtime& rt = RT();
    return (rt.lineIdx >= 0) ? rt.speakerName : rt.emptyStr;
}

float CutsceneDirector::letterbox() const { return RT().letterbox; }
float CutsceneDirector::fade()      const { return RT().fade;      }

} // namespace ert
