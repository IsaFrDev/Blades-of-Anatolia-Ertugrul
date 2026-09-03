// Ertugrul :: Level.cpp
// Ma'lumotga asoslangan daraja: data/levels/<id>.json dan relyef, osmon,
// spawn nuqtalari, rekvizitlar va "scatter" (determinatsiyalangan tarqatish).
// Fayl topilmasa yoki buzuq bo'lsa — protsedural Qayi obasi quriladi (crash yo'q).

#include "ertugrul/world/Level.h"
#include "ertugrul/gfx/ShadowMap.h"
#include "ertugrul/gfx/Pbr.h"
#include "ertugrul/gfx/Mesh.h"
#include "ertugrul/gfx/Texture.h"

#include <nlohmann/json.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using nlohmann::json;

namespace ert {

namespace {

// ===========================================================================
// Fayl yo'llarini topish
// ===========================================================================

bool fileExists(const std::string& p) {
    if (p.empty()) return false;
    const DWORD a = GetFileAttributesA(p.c_str());
    return (a != INVALID_FILE_ATTRIBUTES) && ((a & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

// Bajariluvchi fayl joylashgan katalog (oxirida '/' bilan)
const std::string& exeDir() {
    static std::string dir = []() -> std::string {
        char buf[MAX_PATH];
        const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (n == 0 || n >= MAX_PATH) return std::string();
        std::string s(buf, buf + n);
        const size_t cut = s.find_last_of("\\/");
        if (cut == std::string::npos) return std::string();
        return s.substr(0, cut + 1);
    }();
    return dir;
}

// Nisbiy yo'lni bir necha ehtimoliy ildizdan qidiradi. Topilmasa bo'sh satr.
std::string resolvePath(const std::string& rel) {
    if (rel.empty()) return std::string();
    if (fileExists(rel)) return rel;

    static const char* kUp[] = { "", "../", "../../", "../../../", "../../../../" };
    for (int i = 0; i < 5; ++i) {
        const std::string c = std::string(kUp[i]) + rel;
        if (fileExists(c)) return c;
    }
    const std::string& ed = exeDir();
    if (!ed.empty()) {
        for (int i = 0; i < 5; ++i) {
            const std::string c = ed + kUp[i] + rel;
            if (fileExists(c)) return c;
        }
    }
    return std::string();
}

std::string toLower(std::string s) {
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = (char)std::tolower((unsigned char)s[i]);
    return s;
}

bool contains(const std::string& hay, const char* needle) {
    return hay.find(needle) != std::string::npos;
}

// Satrdan determinatsiyalangan urug' (seed) hosil qilish — FNV-1a
uint32_t hashSeed(const std::string& s) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < s.size(); ++i) {
        h ^= (uint32_t)(unsigned char)s[i];
        h *= 16777619u;
    }
    return h ? h : 1u;
}

// ===========================================================================
// Mesh yechish (protsedural nomlar + fayl yo'llari + zaxira)
// ===========================================================================

Mesh* proceduralMesh(const std::string& lower) {
    if (lower == "cube"   || lower == "box")      return Mesh::unitCube();
    if (lower == "cylinder")                      return Mesh::unitCylinder(14);
    if (lower == "cone"   || lower == "tent")     return Mesh::unitCone(14);
    if (lower == "sphere" || lower == "ball")     return Mesh::unitSphere(12, 16);
    if (lower == "quad"   || lower == "plane")    return Mesh::quadXZ();
    return nullptr;
}

// Fayl topilmaganda mazmunan yaqin protsedural shakl (dunyo bo'sh qolmasin)
Mesh* fallbackMesh(const std::string& lower) {
    if (contains(lower, "tree") || contains(lower, "pine") || contains(lower, "tent"))
        return Mesh::unitCone(12);
    if (contains(lower, "rock") || contains(lower, "stone") || contains(lower, "bush") ||
        contains(lower, "mushroom"))
        return Mesh::unitSphere(8, 12);
    if (contains(lower, "grass") || contains(lower, "flower") || contains(lower, "plank"))
        return Mesh::quadXZ();
    return Mesh::unitCube();
}

Mesh* resolveMeshFor(const std::string& name) {
    if (name.empty()) return nullptr;
    const std::string lower = toLower(name);

    Mesh* m = proceduralMesh(lower);
    if (!m) {
        const std::string p = resolvePath(name);
        if (!p.empty()) m = Mesh::get(p);
    }
    if (!m) m = fallbackMesh(lower);
    if (m && !m->hasList()) m->buildDisplayList();   // statik geometriya -> display list

    // Diagnostika: ERT_MESH_DUMP=1 -> har mesh uchun material/tekstura ma'lumoti
    static const bool dump = (std::getenv("ERT_MESH_DUMP") != nullptr);
    if (dump && m) {
        static std::vector<const Mesh*> seen;
        bool isNew = true;
        for (size_t k = 0; k < seen.size(); ++k) if (seen[k] == m) { isNew = false; break; }
        if (isNew) {
            seen.push_back(m);
            std::printf("[mesh] %-46s tri=%6d subs=%d", name.c_str(),
                        (int)m->triangleCount(), (int)m->subs().size());
            for (size_t si = 0; si < m->subs().size() && si < 3; ++si) {
                const SubMesh& sm = m->subs()[si];
                std::printf("  | kd=(%.2f,%.2f,%.2f) a=%.2f tex=%s", sm.kd[0], sm.kd[1], sm.kd[2],
                            sm.alpha, sm.tex ? "bor" : "yo'q");
            }
            std::printf("\n");
        }
    }
    return m;
}

// ===========================================================================
// JSON yordamchilari (hech qachon istisno tashlamaydi)
// ===========================================================================

float jNum(const json& j, const char* key, float def) {
    if (!j.is_object()) return def;
    const json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_number()) return def;
    const double v = it->get<double>();
    if (!(v == v)) return def;
    return (float)v;
}

int jInt(const json& j, const char* key, int def) {
    if (!j.is_object()) return def;
    const json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_number()) return def;
    return (int)it->get<double>();
}

bool jBool(const json& j, const char* key, bool def) {
    if (!j.is_object()) return def;
    const json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_boolean()) return def;
    return it->get<bool>();
}

std::string jStr(const json& j, const char* key, const std::string& def) {
    if (!j.is_object()) return def;
    const json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_string()) return def;
    return it->get<std::string>();
}

void jVec3(const json& j, const char* key, Vec3& out) {
    if (!j.is_object()) return;
    const json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_array() || it->size() < 3) return;
    const json& a = *it;
    if (a[0].is_number()) out.x = (float)a[0].get<double>();
    if (a[1].is_number()) out.y = (float)a[1].get<double>();
    if (a[2].is_number()) out.z = (float)a[2].get<double>();
}

void jRgb(const json& j, const char* key, float* out) {
    if (!j.is_object() || !out) return;
    const json::const_iterator it = j.find(key);
    if (it == j.end() || !it->is_array() || it->size() < 3) return;
    const json& a = *it;
    for (int c = 0; c < 3; ++c)
        if (a[c].is_number()) out[c] = saturate((float)a[c].get<double>());
}

// ===========================================================================
// Scatter — determinatsiyalangan tasodifiy joylashtirish
// ===========================================================================

struct ScatterSpec {
    std::vector<std::string> meshes;
    int      count      = 0;
    float    minR       = 20.0f, maxR = 100.0f;
    float    cx         = 0.0f,  cz   = 0.0f;
    float    scaleMin   = 1.0f,  scaleMax = 1.0f;
    float    tint[3]    = { 1.0f, 1.0f, 1.0f };
    float    tintJitter = 0.14f;
    bool     collide    = false;
    float    radius     = 0.8f;
    bool     snap       = true;
    float    minSlopeY  = 0.0f;    // 0 = e'tiborsiz; aks holda normal.y >= shu qiymat
    float    minGap     = 0.0f;    // to'qnashuvchi rekvizitlar orasidagi eng kichik masofa
    uint32_t seed       = 1u;
};

void doScatter(std::vector<Prop>& out, const Terrain& terr, const ScatterSpec& sp) {
    if (sp.count <= 0 || sp.meshes.empty()) return;

    const int    total = (sp.count > 4000) ? 4000 : sp.count;
    const float  hs    = terr.half();
    const float  edge  = std::max(2.0f, hs * 0.02f);
    float minR = std::max(0.0f, sp.minR);
    float maxR = std::max(minR + 0.5f, sp.maxR);

    Rng rng(sp.seed ? sp.seed : 1u);
    const size_t firstNew = out.size();

    for (int n = 0; n < total; ++n) {
        // Halqa ichida bir tekis taqsimot
        const float ang = rng.range(0.0f, TAU);
        const float u   = rng.nextFloat();
        const float rr  = std::sqrt(lerpf(minR * minR, maxR * maxR, u));
        const float x   = sp.cx + std::cos(ang) * rr;
        const float z   = sp.cz + std::sin(ang) * rr;

        if (x < -hs + edge || x > hs - edge || z < -hs + edge || z > hs - edge) continue;

        if (sp.minSlopeY > 0.0f) {
            if (terr.normalAt(x, z).y < sp.minSlopeY) continue;
        }

        // Yaqin turgan (to'qnashuvchi) rekvizitlardan qochish
        if (sp.minGap > 0.0f) {
            bool tooClose = false;
            for (size_t k = 0; k < out.size(); ++k) {
                if (!out[k].collide && k < firstNew) continue;
                const float dx = out[k].pos.x - x;
                const float dz = out[k].pos.z - z;
                const float need = sp.minGap + out[k].radius;
                if (dx * dx + dz * dz < need * need) { tooClose = true; break; }
            }
            if (tooClose) continue;
        }

        Prop p;
        p.mesh  = sp.meshes[(size_t)(rng.next() % (uint32_t)sp.meshes.size())];
        p.pos   = Vec3(x, terr.heightAt(x, z), z);
        p.yaw   = rng.range(0.0f, 360.0f);
        p.scale = rng.range(sp.scaleMin, sp.scaleMax);
        for (int c = 0; c < 3; ++c) {
            const float jitter = 1.0f + rng.range(-sp.tintJitter, sp.tintJitter);
            p.tint[c] = saturate(sp.tint[c] * jitter);
        }
        p.collide      = sp.collide;
        p.radius       = std::max(0.05f, sp.radius);
        p.snapToGround = sp.snap;
        out.push_back(p);
    }
}

// ===========================================================================
// Soya diski (yerga yopishgan yumshoq qora doira) — bir marta quriladigan list
// ===========================================================================

unsigned shadowDiskList() {
    static unsigned id = 0;
    if (id != 0) return id;
    const GLuint g = glGenLists(1);
    if (g == 0) return 0;                 // GL tayyor emas — keyingi kadrda urinamiz
    id = (unsigned)g;
    const int SEGS = 14;
    glNewList(g, GL_COMPILE);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glColor4f(0.03f, 0.04f, 0.03f, 0.42f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor4f(0.03f, 0.04f, 0.03f, 0.0f);
    for (int i = 0; i <= SEGS; ++i) {
        const float a = TAU * (float)i / (float)SEGS;
        glVertex3f(std::cos(a), 0.0f, std::sin(a));
    }
    glEnd();
    glEndList();
    return id;
}

// ===========================================================================
// Yakuniy tayyorlash: mesh'larni yechish, yerga yopishtirish, spawn kafolati.
// (Level.h — o'zgarmas kontrakt, unga yangi a'zo funksiya qo'sha olmaymiz,
//  shuning uchun bu erkin funksiya sifatida yozilgan.)
// ===========================================================================
void finalizeLevel(std::vector<Prop>& props, std::vector<SpawnPoint>& spawns,
                   const Terrain& terr) {
    for (size_t i = 0; i < props.size(); ++i) {
        Prop& p = props[i];
        p.cached = resolveMeshFor(p.mesh);
        if (p.snapToGround) p.pos.y = terr.heightAt(p.pos.x, p.pos.z);
        if (!(p.scale > 0.0001f)) p.scale = 1.0f;
        if (p.radius < 0.05f)     p.radius = 0.05f;
    }

    // "player" spawn har doim mavjud bo'lsin
    size_t playerIdx = (size_t)-1;
    bool   hasCut    = false;
    for (size_t i = 0; i < spawns.size(); ++i) {
        if (spawns[i].id == "player")   playerIdx = i;
        if (spawns[i].id == "cutscene") hasCut = true;
    }
    if (playerIdx == (size_t)-1) {
        SpawnPoint p;
        p.id  = "player";
        p.pos = Vec3(0.0f, terr.heightAt(0.0f, 12.0f), 12.0f);
        p.yaw = 180.0f;
        playerIdx = spawns.size();
        spawns.push_back(p);
    }
    if (!hasCut) {
        SpawnPoint c = spawns[playerIdx];
        c.id = "cutscene";
        c.pos.z += 14.0f;
        c.pos.y = terr.heightAt(c.pos.x, c.pos.z);
        spawns.push_back(c);
    }
}

// Osmon rangi: gorizontdan zenitgacha
void skyColorAt(const SkyPreset& s, float elevSin, float* out) {
    const float t = smoothstepf(saturate(elevSin * 1.30f));
    for (int c = 0; c < 3; ++c) out[c] = lerpf(s.horizon[c], s.zenith[c], t);
    if (elevSin < 0.0f) {
        // Gorizontdan pastda biroz to'qroq (yer tomon o'tish)
        const float k = lerpf(1.0f, 0.55f, saturate(-elevSin * 3.2f));
        for (int c = 0; c < 3; ++c) out[c] *= k;
    }
}

} // anonim namespace

// ===========================================================================
// applyTimeOfDay
// ===========================================================================
void Level::applyTimeOfDay(const std::string& tod, const std::string& weather) {
    const std::string t = toLower(tod);
    const std::string w = toLower(weather);

    SkyPreset s;   // default = kunduzgi (header'dagi qiymatlar)

    if (t == "dawn" || t == "tong" || t == "sahar") {
        const float horizon[3] = { 0.97f, 0.68f, 0.46f };
        const float zenith [3] = { 0.28f, 0.40f, 0.68f };
        const float sun    [3] = { 0.62f, 0.20f, 0.76f };
        const float sunC   [3] = { 1.00f, 0.78f, 0.56f };
        const float amb    [3] = { 0.26f, 0.25f, 0.31f };
        const float fog    [3] = { 0.85f, 0.72f, 0.62f };
        for (int c = 0; c < 3; ++c) {
            s.horizon[c] = horizon[c]; s.zenith[c] = zenith[c];
            s.sunDir[c]  = sun[c];     s.sunColor[c] = sunC[c];
            s.ambient[c] = amb[c];     s.fogColor[c] = fog[c];
        }
        s.fogStart = 40.0f; s.fogEnd = 210.0f;
    } else if (t == "dusk" || t == "shom" || t == "evening") {
        const float horizon[3] = { 0.96f, 0.52f, 0.28f };
        const float zenith [3] = { 0.18f, 0.20f, 0.44f };
        const float sun    [3] = { -0.72f, 0.16f, -0.52f };
        const float sunC   [3] = { 1.00f, 0.64f, 0.38f };
        const float amb    [3] = { 0.24f, 0.21f, 0.27f };
        const float fog    [3] = { 0.70f, 0.50f, 0.40f };
        for (int c = 0; c < 3; ++c) {
            s.horizon[c] = horizon[c]; s.zenith[c] = zenith[c];
            s.sunDir[c]  = sun[c];     s.sunColor[c] = sunC[c];
            s.ambient[c] = amb[c];     s.fogColor[c] = fog[c];
        }
        s.fogStart = 45.0f; s.fogEnd = 205.0f;
    } else if (t == "golden" || t == "oltin") {
        // OLTIN SOAT — demo xarita uchun. dusk dan farqi: quyosh ufqdan biroz
        // yuqoriroq (soyalar uzun, lekin yer qorayib ketmaydi) va MUHIT
        // yorug'ligi ancha kuchli — aks holda oba maydonining tuprog'i qop-qora
        // bo'lib qolardi.
        const float horizon[3] = { 1.00f, 0.72f, 0.44f };
        const float zenith [3] = { 0.30f, 0.44f, 0.70f };
        const float sun    [3] = { -0.55f, 0.34f, -0.46f };
        const float sunC   [3] = { 1.00f, 0.86f, 0.64f };
        const float amb    [3] = { 0.46f, 0.43f, 0.44f };
        const float fog    [3] = { 0.88f, 0.70f, 0.54f };
        for (int c = 0; c < 3; ++c) {
            s.horizon[c] = horizon[c]; s.zenith[c] = zenith[c];
            s.sunDir[c]  = sun[c];     s.sunColor[c] = sunC[c];
            s.ambient[c] = amb[c];     s.fogColor[c] = fog[c];
        }
        s.fogStart = 72.0f; s.fogEnd = 300.0f;
    } else if (t == "night" || t == "tun") {
        // Tunda ancha qorong'i va ko'kish; "quyosh" o'rnida oy
        const float horizon[3] = { 0.09f, 0.12f, 0.22f };
        const float zenith [3] = { 0.015f, 0.035f, 0.11f };
        const float sun    [3] = { -0.32f, 0.62f, 0.42f };
        const float sunC   [3] = { 0.30f, 0.37f, 0.56f };
        const float amb    [3] = { 0.10f, 0.13f, 0.21f };
        const float fog    [3] = { 0.055f, 0.075f, 0.145f };
        for (int c = 0; c < 3; ++c) {
            s.horizon[c] = horizon[c]; s.zenith[c] = zenith[c];
            s.sunDir[c]  = sun[c];     s.sunColor[c] = sunC[c];
            s.ambient[c] = amb[c];     s.fogColor[c] = fog[c];
        }
        s.fogStart = 26.0f; s.fogEnd = 150.0f;
    }
    // aks holda "day" — default qiymatlar qoladi

    // --- Ob-havo tuzatmalari ---
    if (w == "cloudy" || w == "bulutli") {
        for (int c = 0; c < 3; ++c) {
            const float grey = (s.horizon[0] + s.horizon[1] + s.horizon[2]) / 3.0f;
            s.horizon[c] = lerpf(s.horizon[c], grey, 0.55f);
            s.zenith[c]  = lerpf(s.zenith[c], grey * 0.75f, 0.45f);
            s.sunColor[c] *= 0.74f;
            s.ambient[c]  = saturate(s.ambient[c] + 0.05f);
            s.fogColor[c] = lerpf(s.fogColor[c], grey, 0.45f);
        }
        s.fogEnd *= 0.78f;
    } else if (w == "rain" || w == "yomgir" || w == "yomg'ir") {
        for (int c = 0; c < 3; ++c) {
            s.horizon[c]  *= 0.55f;
            s.zenith[c]   *= 0.50f;
            s.sunColor[c] *= 0.50f;
            s.ambient[c]   = saturate(s.ambient[c] * 0.85f + 0.03f);
            s.fogColor[c]  = lerpf(s.fogColor[c] * 0.60f, 0.38f, 0.25f);
        }
        s.fogStart *= 0.45f;
        s.fogEnd   *= 0.42f;
    } else if (w == "fog" || w == "tuman" || w == "mist") {
        for (int c = 0; c < 3; ++c) {
            s.sunColor[c] *= 0.68f;
            s.ambient[c]   = saturate(s.ambient[c] + 0.08f);
            s.horizon[c]   = lerpf(s.horizon[c], s.fogColor[c], 0.70f);
            s.zenith[c]    = lerpf(s.zenith[c],  s.fogColor[c], 0.45f);
        }
        s.fogStart = 6.0f;
        s.fogEnd   = std::max(40.0f, s.fogEnd * 0.30f);
    }

    if (s.fogEnd < s.fogStart + 5.0f) s.fogEnd = s.fogStart + 5.0f;
    sky_ = s;
}

// ===========================================================================
// load
// ===========================================================================
bool Level::load(const std::string& levelId) {
    destroy();

    id_      = levelId.empty() ? std::string("oba_camp") : levelId;
    locName_ = id_;

    const std::string path = resolvePath("data/levels/" + id_ + ".json");
    if (path.empty()) {
        const std::string want = id_;
        buildProceduralOba(hashSeed(want));
        id_ = want;
        return false;
    }

    json j;
    {
        std::ifstream f(path.c_str(), std::ios::binary);
        if (!f.is_open()) {
            const std::string want = id_;
            buildProceduralOba(hashSeed(want));
            id_ = want;
            return false;
        }
        // Istisnosiz tahlil: xato bo'lsa "discarded" qaytadi
        j = json::parse(f, nullptr, false, true);
    }
    if (j.is_discarded() || !j.is_object()) {
        const std::string want = id_;
        buildProceduralOba(hashSeed(want));
        id_ = want;
        return false;
    }

    try {
        id_      = jStr(j, "id", id_);
        locName_ = jStr(j, "loc_name", id_);

        // --- relyef ---
        int      grid   = 128;
        float    wsize  = 320.0f;
        uint32_t tseed  = hashSeed(id_);
        float    hillH  = 7.0f;
        float    flatR  = 26.0f;
        {
            const json::const_iterator it = j.find("terrain");
            if (it != j.end() && it->is_object()) {
                const json& t = *it;
                grid  = jInt(t, "grid", grid);
                wsize = jNum(t, "size", wsize);
                tseed = (uint32_t)jInt(t, "seed", (int)tseed);
                hillH = jNum(t, "hill_height", hillH);
                flatR = jNum(t, "flat_radius", flatR);
            }
        }
        terrain_.build(grid, wsize, tseed ? tseed : 1u, hillH, flatR);
        terrain_.setTextures(Texture::grass(tseed + 7u),
                             Texture::dirt(tseed + 11u),
                             Texture::rock(tseed + 23u));

        // --- osmon ---
        std::string tod = "day", weather = "clear";
        {
            const json::const_iterator it = j.find("sky");
            if (it != j.end() && it->is_object()) {
                tod     = jStr(*it, "time_of_day", tod);
                weather = jStr(*it, "weather", weather);
            }
            applyTimeOfDay(tod, weather);
            // Ixtiyoriy aniq qiymatlar bilan ustiga yozish
            if (it != j.end() && it->is_object()) {
                jRgb(*it, "horizon",   sky_.horizon);
                jRgb(*it, "zenith",    sky_.zenith);
                jRgb(*it, "sun_color", sky_.sunColor);
                jRgb(*it, "ambient",   sky_.ambient);
                jRgb(*it, "fog_color", sky_.fogColor);
                Vec3 sd(sky_.sunDir[0], sky_.sunDir[1], sky_.sunDir[2]);
                jVec3(*it, "sun_dir", sd);
                sky_.sunDir[0] = sd.x; sky_.sunDir[1] = sd.y; sky_.sunDir[2] = sd.z;
                sky_.fogStart = jNum(*it, "fog_start", sky_.fogStart);
                sky_.fogEnd   = jNum(*it, "fog_end",   sky_.fogEnd);
                if (sky_.fogEnd < sky_.fogStart + 5.0f) sky_.fogEnd = sky_.fogStart + 5.0f;
            }
        }

        // --- spawn nuqtalari ---
        {
            const json::const_iterator it = j.find("spawns");
            if (it != j.end() && it->is_array()) {
                for (json::const_iterator e = it->begin(); e != it->end(); ++e) {
                    if (!e->is_object()) continue;
                    SpawnPoint sp;
                    sp.id = jStr(*e, "id", "");
                    if (sp.id.empty()) continue;
                    jVec3(*e, "pos", sp.pos);
                    sp.yaw = jNum(*e, "yaw", 0.0f);
                    sp.pos.y = terrain_.heightAt(sp.pos.x, sp.pos.z) + jNum(*e, "y_offset", 0.0f);
                    spawns_.push_back(sp);
                }
            }
        }

        // --- rekvizitlar ---
        {
            const json::const_iterator it = j.find("props");
            if (it != j.end() && it->is_array()) {
                for (json::const_iterator e = it->begin(); e != it->end(); ++e) {
                    if (!e->is_object()) continue;
                    Prop p;
                    p.mesh = jStr(*e, "mesh", "");
                    if (p.mesh.empty()) continue;
                    jVec3(*e, "pos", p.pos);
                    p.yaw          = jNum(*e, "yaw", 0.0f);
                    p.scale        = jNum(*e, "scale", 1.0f);
                    if (!(p.scale > 0.0001f)) p.scale = 1.0f;
                    jRgb(*e, "tint", p.tint);
                    p.collide      = jBool(*e, "collide", false);
                    p.radius       = std::max(0.05f, jNum(*e, "radius", 0.5f));
                    p.snapToGround = jBool(*e, "snap", true);
                    props_.push_back(p);
                }
            }
        }

        // --- scatter ---
        {
            const json::const_iterator it = j.find("scatter");
            if (it != j.end() && it->is_array()) {
                for (json::const_iterator e = it->begin(); e != it->end(); ++e) {
                    if (!e->is_object()) continue;
                    ScatterSpec sp;
                    const json::const_iterator ml = e->find("meshes");
                    if (ml != e->end() && ml->is_array()) {
                        for (json::const_iterator m = ml->begin(); m != ml->end(); ++m)
                            if (m->is_string()) sp.meshes.push_back(m->get<std::string>());
                    }
                    const std::string single = jStr(*e, "mesh", "");
                    if (!single.empty()) sp.meshes.push_back(single);
                    if (sp.meshes.empty()) continue;

                    sp.count     = jInt(*e, "count", 0);
                    sp.minR      = jNum(*e, "min_radius", 20.0f);
                    sp.maxR      = jNum(*e, "max_radius", 120.0f);
                    sp.cx        = jNum(*e, "center_x", 0.0f);
                    sp.cz        = jNum(*e, "center_z", 0.0f);
                    sp.scaleMin  = jNum(*e, "scale_min", 1.0f);
                    sp.scaleMax  = jNum(*e, "scale_max", std::max(1.0f, sp.scaleMin));
                    jRgb(*e, "tint", sp.tint);
                    sp.tintJitter= saturate(jNum(*e, "tint_jitter", 0.14f));
                    sp.collide   = jBool(*e, "collide", false);
                    sp.radius    = std::max(0.05f, jNum(*e, "radius", 0.8f));
                    sp.snap      = jBool(*e, "snap", true);
                    sp.minSlopeY = jNum(*e, "min_slope_y", 0.0f);
                    sp.minGap    = jNum(*e, "min_gap", 0.0f);
                    sp.seed      = (uint32_t)jInt(*e, "seed", 7);
                    doScatter(props_, terrain_, sp);
                }
            }
        }
    } catch (...) {
        // JSON kutilmagan shaklda bo'lsa ham o'yin davom etsin
        if (props_.empty() && !terrain_.valid()) {
            const std::string want = id_;
            buildProceduralOba(hashSeed(want));
            id_ = want;
            return false;
        }
    }

    if (!terrain_.valid())
        terrain_.build(128, 320.0f, hashSeed(id_), 7.0f, 26.0f);

    finalizeLevel(props_, spawns_, terrain_);
    return true;
}

// ===========================================================================
// buildProceduralOba — JSON bo'lmaganda quriladigan Qayi obasi
// ===========================================================================
void Level::buildProceduralOba(uint32_t seed) {
    destroy();
    if (seed == 0u) seed = 1337u;

    if (id_.empty())      id_ = "oba_camp";
    locName_ = "Qayi obasi";

    terrain_.build(128, 320.0f, seed, 7.0f, 26.0f);
    terrain_.setTextures(Texture::grass(seed + 7u),
                         Texture::dirt(seed + 11u),
                         Texture::rock(seed + 23u));
    applyTimeOfDay("day", "clear");

    // Kichik yordamchi: rekvizit qo'shish
    auto add = [&](const char* mesh, float x, float z, float yaw, float scale,
                   float r, float g, float b, bool collide, float radius) {
        Prop p;
        p.mesh = mesh;
        p.pos  = Vec3(x, terrain_.heightAt(x, z), z);
        p.yaw  = yaw;
        p.scale = scale;
        p.tint[0] = r; p.tint[1] = g; p.tint[2] = b;
        p.collide = collide;
        p.radius  = std::max(0.05f, radius);
        p.snapToGround = true;
        props_.push_back(p);
    };

    const char* kNat  = "assets/models/nature/";
    const char* kTown = "assets/models/town/";
    auto nat  = [&](const char* n) { return std::string(kNat)  + n; };
    auto town = [&](const char* n) { return std::string(kTown) + n; };

    Rng rng(seed);

    // --- Markazda katta bek chodiri ---
    add((nat("tent_detailedClosed.obj")).c_str(), 0.0f, -3.0f, 18.0f, 15.0f,
        0.66f, 0.47f, 0.33f, true, 6.0f);
    // Yon qanotlar (bek chodiri atrofi)
    add((nat("tent_detailedOpen.obj")).c_str(), -9.5f, -6.0f, 55.0f, 9.0f,
        0.60f, 0.42f, 0.30f, true, 3.4f);
    add((nat("tent_detailedOpen.obj")).c_str(),  9.5f, -6.0f, -55.0f, 9.0f,
        0.58f, 0.40f, 0.29f, true, 3.4f);

    // --- Doira bo'ylab 12 ta chodir ---
    {
        const char* tents[4] = { "tent_smallClosed.obj", "tent_detailedClosed.obj",
                                 "tent_smallOpen.obj",   "tent_detailedOpen.obj" };
        const int   N = 12;
        for (int i = 0; i < N; ++i) {
            const float a = TAU * (float)i / (float)N + 0.18f;
            const float rr = 19.5f + rng.range(-1.4f, 1.4f);
            const float x = std::cos(a) * rr;
            const float z = std::sin(a) * rr;
            const float yaw = rad2deg(std::atan2(-x, -z));    // markazga qaraydi
            const float sc  = rng.range(7.0f, 9.0f);
            const float tr  = rng.range(0.48f, 0.72f);
            add((nat(tents[i % 4])).c_str(), x, z, yaw, sc,
                tr, tr * rng.range(0.68f, 0.80f), tr * rng.range(0.48f, 0.62f),
                true, sc * 0.30f);
        }
    }

    // --- Gulxan (markaz oldida) ---
    add((nat("campfire_stones.obj")).c_str(), 0.0f, 9.0f, 0.0f, 6.0f,
        0.60f, 0.58f, 0.55f, false, 1.2f);
    add((nat("campfire_logs.obj")).c_str(),   0.0f, 9.0f, 24.0f, 4.5f,
        0.50f, 0.34f, 0.20f, true, 1.0f);

    // --- Gulxan atrofidagi fonarlar ---
    for (int i = 0; i < 6; ++i) {
        const float a = TAU * (float)i / 6.0f;
        add((town("lantern.obj")).c_str(), std::cos(a) * 13.0f, 9.0f + std::sin(a) * 13.0f,
            rng.range(0.0f, 360.0f), 2.2f, 0.85f, 0.72f, 0.45f, false, 0.4f);
    }

    // --- Otxona panjarasi (to'g'ri to'rtburchak og'il) ---
    {
        const float cx = 36.0f, cz = -24.0f;
        const float hw = 11.0f, hd = 8.0f;   // yarim o'lcham
        const float step = 3.6f;
        const float fs = 4.0f;               // panjara masshtabi (uzunlik ~1 birlik)
        for (float t = -hw; t <= hw + 0.01f; t += step) {
            add((town("fence.obj")).c_str(), cx + t, cz - hd, 90.0f, fs,
                0.52f, 0.40f, 0.27f, true, 1.6f);
            if (t < hw * 0.35f || t > hw * 0.62f)     // darvoza uchun bo'shliq
                add((town("fence.obj")).c_str(), cx + t, cz + hd, 90.0f, fs,
                    0.52f, 0.40f, 0.27f, true, 1.6f);
        }
        for (float t = -hd + step; t <= hd - step + 0.01f; t += step) {
            add((town("fence.obj")).c_str(), cx - hw, cz + t, 0.0f, fs,
                0.52f, 0.40f, 0.27f, true, 1.6f);
            add((town("fence.obj")).c_str(), cx + hw, cz + t, 0.0f, fs,
                0.52f, 0.40f, 0.27f, true, 1.6f);
        }
        add((town("fence-gate.obj")).c_str(), cx + hw * 0.5f, cz + hd, 90.0f, fs,
            0.56f, 0.43f, 0.29f, true, 1.6f);
    }

    // --- Temirchilik va bozor rastalari ---
    add((town("stall-red.obj")).c_str(),   -25.0f, -15.0f,  38.0f, 5.0f, 0.72f, 0.32f, 0.26f, true, 2.6f);
    add((town("stall-green.obj")).c_str(), -31.0f,  -6.0f,  62.0f, 5.0f, 0.36f, 0.55f, 0.34f, true, 2.6f);
    add((town("stall.obj")).c_str(),       -24.0f,   3.0f, 100.0f, 5.0f, 0.62f, 0.50f, 0.36f, true, 2.6f);
    add((town("stall-bench.obj")).c_str(), -21.0f,  -9.0f,  20.0f, 3.4f, 0.55f, 0.41f, 0.27f, false, 1.0f);
    add((town("stall-stool.obj")).c_str(), -19.5f,  -5.0f,   0.0f, 2.6f, 0.55f, 0.41f, 0.27f, false, 0.6f);
    add((town("blade.obj")).c_str(),       -23.0f, -12.0f, 210.0f, 3.0f, 0.72f, 0.74f, 0.78f, false, 0.5f);
    add((nat("log_stack.obj")).c_str(),    -29.0f, -18.0f,  15.0f, 4.0f, 0.48f, 0.34f, 0.21f, true, 1.6f);
    add((nat("log.obj")).c_str(),          -33.0f, -12.0f,  70.0f, 4.0f, 0.46f, 0.33f, 0.20f, true, 1.0f);

    // --- Aravalar ---
    add((town("cart.obj")).c_str(),       21.0f, 24.0f,  35.0f, 5.5f, 0.55f, 0.40f, 0.26f, true, 2.2f);
    add((town("cart-high.obj")).c_str(), -18.0f, 26.0f, 145.0f, 5.5f, 0.52f, 0.38f, 0.25f, true, 2.2f);
    add((town("cart.obj")).c_str(),       30.0f,  9.0f, 250.0f, 5.0f, 0.57f, 0.42f, 0.28f, true, 2.2f);

    // --- Bayroqlar (oba chegarasi bo'ylab) ---
    for (int i = 0; i < 8; ++i) {
        const float a = TAU * (float)i / 8.0f + 0.4f;
        const bool  red = (i % 2) == 0;
        add((town(red ? "banner-red.obj" : "banner-green.obj")).c_str(),
            std::cos(a) * 27.0f, std::sin(a) * 27.0f, rad2deg(a) + 90.0f, 6.5f,
            red ? 0.78f : 0.30f, red ? 0.20f : 0.52f, red ? 0.16f : 0.28f, false, 0.5f);
    }
    // Yog'och ustunlar (chodir arqonlari uchun)
    for (int i = 0; i < 6; ++i) {
        const float a = TAU * (float)i / 6.0f + 0.9f;
        add((town("pillar-wood.obj")).c_str(), std::cos(a) * 24.0f, std::sin(a) * 24.0f,
            0.0f, 4.0f, 0.50f, 0.37f, 0.24f, true, 0.6f);
    }

    // --- Tashqaridagi qarag'ay o'rmoni halqasi ---
    {
        ScatterSpec sp;
        sp.meshes.push_back(nat("tree_pineDefaultA.obj"));
        sp.meshes.push_back(nat("tree_pineTallA.obj"));
        sp.meshes.push_back(nat("tree_pineRoundA.obj"));
        sp.meshes.push_back(nat("tree_pineTallA_detailed.obj"));
        sp.count = 240; sp.minR = 52.0f; sp.maxR = 148.0f;
        sp.scaleMin = 3.4f; sp.scaleMax = 6.2f;
        sp.tint[0] = 0.42f; sp.tint[1] = 0.58f; sp.tint[2] = 0.34f;
        sp.tintJitter = 0.18f;
        sp.collide = true; sp.radius = 1.3f; sp.minGap = 3.2f;
        sp.minSlopeY = 0.60f;
        sp.seed = seed + 101u;
        doScatter(props_, terrain_, sp);
    }
    // Ichkariroqda siyrak daraxtlar
    {
        ScatterSpec sp;
        sp.meshes.push_back(nat("tree_pineDefaultA.obj"));
        sp.meshes.push_back(nat("tree_oak.obj"));
        sp.count = 26; sp.minR = 34.0f; sp.maxR = 52.0f;
        sp.scaleMin = 3.0f; sp.scaleMax = 4.6f;
        sp.tint[0] = 0.46f; sp.tint[1] = 0.60f; sp.tint[2] = 0.36f;
        sp.collide = true; sp.radius = 1.2f; sp.minGap = 5.0f;
        sp.seed = seed + 202u;
        doScatter(props_, terrain_, sp);
    }
    // --- Toshlar ---
    {
        ScatterSpec sp;
        sp.meshes.push_back(nat("rock_largeA.obj"));
        sp.meshes.push_back(nat("rock_largeB.obj"));
        sp.meshes.push_back(nat("rock_largeC.obj"));
        sp.meshes.push_back(nat("rock_smallA.obj"));
        sp.meshes.push_back(nat("rock_smallB.obj"));
        sp.count = 90; sp.minR = 32.0f; sp.maxR = 152.0f;
        sp.scaleMin = 2.0f; sp.scaleMax = 5.5f;
        sp.tint[0] = 0.60f; sp.tint[1] = 0.58f; sp.tint[2] = 0.54f;
        sp.collide = true; sp.radius = 1.4f; sp.minGap = 2.5f;
        sp.seed = seed + 303u;
        doScatter(props_, terrain_, sp);
    }
    // --- Butalar, o't tutamlari, gullar (to'qnashuvsiz bezak) ---
    {
        ScatterSpec sp;
        sp.meshes.push_back(nat("plant_bush.obj"));
        sp.meshes.push_back(nat("plant_bushLarge.obj"));
        sp.meshes.push_back(nat("plant_bushSmall.obj"));
        sp.meshes.push_back(nat("grass_large.obj"));
        sp.meshes.push_back(nat("grass.obj"));
        sp.count = 190; sp.minR = 28.0f; sp.maxR = 150.0f;
        sp.scaleMin = 1.6f; sp.scaleMax = 3.8f;
        sp.tint[0] = 0.44f; sp.tint[1] = 0.60f; sp.tint[2] = 0.30f;
        sp.tintJitter = 0.20f;
        sp.collide = false; sp.radius = 0.5f;
        sp.minSlopeY = 0.55f;
        sp.seed = seed + 404u;
        doScatter(props_, terrain_, sp);
    }
    {
        ScatterSpec sp;
        sp.meshes.push_back(nat("flower_redA.obj"));
        sp.meshes.push_back(nat("flower_yellowA.obj"));
        sp.meshes.push_back(nat("flower_purpleA.obj"));
        sp.meshes.push_back(nat("mushroom_redGroup.obj"));
        sp.meshes.push_back(nat("stump_old.obj"));
        sp.count = 70; sp.minR = 30.0f; sp.maxR = 120.0f;
        sp.scaleMin = 1.8f; sp.scaleMax = 3.2f;
        sp.tint[0] = 0.90f; sp.tint[1] = 0.85f; sp.tint[2] = 0.70f;
        sp.tintJitter = 0.22f;
        sp.collide = false; sp.radius = 0.4f;
        sp.minSlopeY = 0.60f;
        sp.seed = seed + 505u;
        doScatter(props_, terrain_, sp);
    }

    // --- Spawn nuqtalari ---
    {
        SpawnPoint p;
        p.id = "player";
        p.pos = Vec3(0.0f, terrain_.heightAt(0.0f, 16.0f), 16.0f);
        p.yaw = 180.0f;
        spawns_.push_back(p);

        SpawnPoint c;
        c.id = "cutscene";
        c.pos = Vec3(-14.0f, terrain_.heightAt(-14.0f, 34.0f), 34.0f);
        c.yaw = 155.0f;
        spawns_.push_back(c);

        SpawnPoint e;
        e.id = "enemy";
        e.pos = Vec3(46.0f, terrain_.heightAt(46.0f, 46.0f), 46.0f);
        e.yaw = 225.0f;
        spawns_.push_back(e);
    }

    finalizeLevel(props_, spawns_, terrain_);
}

// ===========================================================================
void Level::destroy() {
    terrain_.destroy();
    props_.clear();
    spawns_.clear();
    id_.clear();
    locName_.clear();
    sky_ = SkyPreset();
}

// ===========================================================================
const SpawnPoint* Level::spawn(const std::string& id) const {
    for (size_t i = 0; i < spawns_.size(); ++i)
        if (spawns_[i].id == id) return &spawns_[i];
    return nullptr;
}

// ===========================================================================
// resolveCollision — XZ da doiraviy itarish
// ===========================================================================
bool Level::resolveCollision(Vec3& pos, float radius) const {
    if (!(radius == radius) || radius < 0.0f) radius = 0.0f;
    // NaN himoyasi: buzuq pozitsiya butun harakat tizimini buzmasin
    if (!(pos.x == pos.x)) pos.x = 0.0f;
    if (!(pos.y == pos.y)) pos.y = 0.0f;
    if (!(pos.z == pos.z)) pos.z = 0.0f;
    bool hit = false;

    for (size_t i = 0; i < props_.size(); ++i) {
        const Prop& p = props_[i];
        if (!p.collide) continue;
        const float minD = radius + p.radius;
        if (minD <= 0.0f) continue;

        float dx = pos.x - p.pos.x;
        float dz = pos.z - p.pos.z;
        const float d2 = dx * dx + dz * dz;
        if (d2 >= minD * minD) continue;

        float d = std::sqrt(d2);
        if (d < 1e-4f) { dx = 1.0f; dz = 0.0f; d = 1.0f; }   // aynan markazda bo'lsa
        const float push = (minD - d) / d;
        pos.x += dx * push;
        pos.z += dz * push;
        hit = true;
    }

    const float bx = pos.x, bz = pos.z;
    terrain_.clampToBounds(pos, 2.5f);
    if (pos.x != bx || pos.z != bz) hit = true;
    return hit;
}

// ===========================================================================
// applyLighting
// ===========================================================================
void Level::applyLighting() const {
    Vec3 d = normalize(Vec3(sky_.sunDir[0], sky_.sunDir[1], sky_.sunDir[2]));
    if (lengthSq(d) < 1e-6f) d = Vec3(0.0f, 1.0f, 0.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    // w = 0 -> yo'naltirilgan yorug'lik. DIQQAT: joriy MODELVIEW (kamera) matritsasi
    // bo'yicha o'zgartiriladi — shuning uchun bu funksiya ko'rinish matritsasi
    // o'rnatilgandan KEYIN chaqirilishi kerak.
    const GLfloat lpos[4] = { d.x, d.y, d.z, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    // To'ldiruvchi yorug'lik (pastda GL_LIGHT1) qo'shilgani uchun quyoshni biroz
    // pasaytiramiz: aks holda quyoshga tik qaragan yuzalarda amb+sun+fill > 1 bo'lib,
    // rang kanallari to'yinib ("kuyib") ketadi — chodirlar neon-qizil chiqardi.
    const float kSun = 0.82f;
    const GLfloat dif[4] = { sky_.sunColor[0] * kSun, sky_.sunColor[1] * kSun,
                             sky_.sunColor[2] * kSun, 1.0f };
    const GLfloat amb0[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat spc[4] = { sky_.sunColor[0] * 0.16f, sky_.sunColor[1] * 0.16f,
                             sky_.sunColor[2] * 0.16f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spc);

    const GLfloat amb[4] = { sky_.ambient[0], sky_.ambient[1], sky_.ambient[2], 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

    // --- To'ldiruvchi (fill) yorug'lik: osmon gumbazidan yumshoq sochilgan nur ---
    // Bittagina yo'naltirilgan quyosh bilan quyoshga teskari yuzalar qop-qora chiqadi
    // (ayniqsa tong/shomda personajlar siluetga aylanadi). GL_LIGHT1 quyoshga qarama-qarshi
    // tomondan, zenit rangi bilan, quvvati past — bu "hemispheric ambient" ning arzon taqlidi.
    Vec3 fill = normalize(Vec3(-d.x * 0.7f, 0.85f, -d.z * 0.7f));
    if (lengthSq(fill) < 1e-6f) fill = Vec3(0.0f, 1.0f, 0.0f);
    const GLfloat fpos[4] = { fill.x, fill.y, fill.z, 0.0f };
    const float   fk      = 0.42f;
    const GLfloat fdif[4] = { (sky_.zenith[0] * 0.55f + 0.45f) * fk,
                              (sky_.zenith[1] * 0.55f + 0.45f) * fk,
                              (sky_.zenith[2] * 0.55f + 0.50f) * fk, 1.0f };
    const GLfloat zero4[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, fpos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  fdif);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  zero4);
    glLightfv(GL_LIGHT1, GL_SPECULAR, zero4);

    // --- Tuman ---
    const GLfloat fc[4] = { sky_.fogColor[0], sky_.fogColor[1], sky_.fogColor[2], 1.0f };
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fc);
    glFogf(GL_FOG_START, sky_.fogStart);
    glFogf(GL_FOG_END,   sky_.fogEnd);
    glHint(GL_FOG_HINT, GL_NICEST);

    glClearColor(sky_.fogColor[0], sky_.fogColor[1], sky_.fogColor[2], 1.0f);
}

// ===========================================================================
// drawSky — kamera atrofidagi gradient gumbaz + quyosh diski (+ tunda yulduzlar)
// ===========================================================================
void Level::drawSky(const Vec3& camPos) const {
    const SkyPreset& s = sky_;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT |
                 GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT | GL_POINT_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(camPos.x, camPos.y, camPos.z);

    const float R     = 30.0f;      // kichik radius — yaqin tekislikdan ichkarida
    const int   RINGS = 14;
    const int   SEGS  = 28;
    const float eLo   = -0.34f;     // gorizontdan pastga ham cho'ziladi
    const float eHi   = PI * 0.5f;

    for (int r = 0; r < RINGS; ++r) {
        const float e0 = lerpf(eLo, eHi, (float)r / (float)RINGS);
        const float e1 = lerpf(eLo, eHi, (float)(r + 1) / (float)RINGS);
        const float s0 = std::sin(e0), c0 = std::cos(e0);
        const float s1 = std::sin(e1), c1 = std::cos(e1);
        float col0[3], col1[3];
        skyColorAt(s, s0, col0);
        skyColorAt(s, s1, col1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int k = 0; k <= SEGS; ++k) {
            const float a = TAU * (float)k / (float)SEGS;
            const float ca = std::cos(a), sa = std::sin(a);
            glColor3f(col1[0], col1[1], col1[2]);
            glVertex3f(ca * c1 * R, s1 * R, sa * c1 * R);
            glColor3f(col0[0], col0[1], col0[2]);
            glVertex3f(ca * c0 * R, s0 * R, sa * c0 * R);
        }
        glEnd();
    }

    // --- Tunda yulduzlar ---
    const float zenLum = (s.zenith[0] + s.zenith[1] + s.zenith[2]) / 3.0f;
    if (zenLum < 0.16f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glPointSize(2.0f);
        Rng rng(20250830u);
        glBegin(GL_POINTS);
        for (int i = 0; i < 260; ++i) {
            const float a  = rng.range(0.0f, TAU);
            const float el = std::asin(rng.range(0.04f, 1.0f));
            const float br = rng.range(0.35f, 1.0f);
            glColor4f(0.86f * br, 0.90f * br, 1.0f * br, br);
            glVertex3f(std::cos(a) * std::cos(el) * R * 0.97f,
                       std::sin(el) * R * 0.97f,
                       std::sin(a) * std::cos(el) * R * 0.97f);
        }
        glEnd();
        glDisable(GL_BLEND);
    }

    // --- Quyosh (yoki oy) diski ---
    Vec3 sd = normalize(Vec3(s.sunDir[0], s.sunDir[1], s.sunDir[2]));
    if (lengthSq(sd) > 1e-6f && sd.y > -0.14f) {
        // Bazis vektorlar
        Vec3 hint = (std::fabs(sd.y) > 0.95f) ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f, 1.0f, 0.0f);
        Vec3 rt = normalize(cross(hint, sd));
        if (lengthSq(rt) < 1e-6f) rt = Vec3(1.0f, 0.0f, 0.0f);
        const Vec3 up = normalize(cross(sd, rt));
        const Vec3 ctr = sd * (R * 0.90f);

        // Past turgan quyosh kattaroq va qizg'ish (dawn/dusk)
        const float lowT   = 1.0f - saturate(sd.y * 2.4f);
        const float sunR   = R * lerpf(0.030f, 0.085f, lowT);
        const float warm[3] = { 1.00f, 0.55f, 0.28f };
        float core[3];
        for (int c = 0; c < 3; ++c)
            core[c] = saturate(lerpf(s.sunColor[c], warm[c], lowT * 0.75f) + 0.18f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);      // additiv nur

        // Halo (yumshoq nur)
        const float haloR = sunR * lerpf(2.6f, 4.6f, lowT);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(core[0], core[1], core[2], 0.42f);
        glVertex3f(ctr.x, ctr.y, ctr.z);
        glColor4f(core[0], core[1], core[2], 0.0f);
        for (int i = 0; i <= 24; ++i) {
            const float a = TAU * (float)i / 24.0f;
            const Vec3 p = ctr + rt * (std::cos(a) * haloR) + up * (std::sin(a) * haloR);
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();

        // Yadro
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(core[0], core[1], core[2], 0.95f);
        glVertex3f(ctr.x, ctr.y, ctr.z);
        glColor4f(core[0], core[1], core[2], 0.55f);
        for (int i = 0; i <= 24; ++i) {
            const float a = TAU * (float)i / 24.0f;
            const Vec3 p = ctr + rt * (std::cos(a) * sunR) + up * (std::sin(a) * sunR);
            glVertex3f(p.x, p.y, p.z);
        }
        glEnd();
    }

    glPopMatrix();
    glPopAttrib();
}

// ===========================================================================
// draw — relyef + soyalar + rekvizitlar
// ===========================================================================
void Level::drawCasters(const Vec3& focus, float radius) const {
    terrain_.draw();                       // tepaliklar vodiyga soya tashlaydi
    static const bool noProps = (std::getenv("ERT_NO_PROPS") != nullptr);
    if (noProps) return;
    const float r2 = radius * radius;
    for (size_t i = 0; i < props_.size(); ++i) {
        const Prop& p = props_[i];
        if (p.cached == nullptr) continue;
        const float dx = p.pos.x - focus.x, dz = p.pos.z - focus.z;
        if (dx * dx + dz * dz > r2) continue;
        const float gy = p.snapToGround ? terrain_.heightAt(p.pos.x, p.pos.z) : p.pos.y;
        glPushMatrix();
        glTranslatef(p.pos.x, gy, p.pos.z);
        if (p.yaw != 0.0f)   glRotatef(p.yaw, 0.0f, 1.0f, 0.0f);
        if (p.scale != 1.0f) glScalef(p.scale, p.scale, p.scale);
        if (p.cached->hasList()) p.cached->drawList();
        else                     p.cached->draw();
        glPopMatrix();
    }
}

void Level::draw(const Vec3& camPos) const {
    terrain_.draw();
    if (props_.empty()) return;

    const float kCullDist   = 200.0f;    // rekvizit ko'rinish masofasi
    const float kCull2      = kCullDist * kCullDist;
    const float kShadowDist = 95.0f;
    const float kShadow2    = kShadowDist * kShadowDist;

    // --- 1) Yumshoq soyalar (yerga yopishgan shaffof disk) ---
    // Zich darajada (qishloq, 371 rekvizit) disklar bir-birining ustiga tushib,
    // polygon-offset tufayli devorlarni ham qop-qora bo'yab yuborardi —
    // shuning uchun har ikkinchi rekvizitni o'tkazib yuboramiz.
    static const bool noShadows = (std::getenv("ERT_NO_SHADOWS") != nullptr);
    static const bool noProps   = (std::getenv("ERT_NO_PROPS")   != nullptr);
    static const bool flatProps = (std::getenv("ERT_FLAT_PROPS") != nullptr);
    static const bool noLists   = (std::getenv("ERT_NO_LISTS")   != nullptr);
    const bool thinShadows = (props_.size() > 150);
    // Haqiqiy soya xaritasi ishlayotganda disklar ortiqcha (ikki soya bo'lardi)
    const unsigned disk = (noShadows || ShadowMap::get().receiving()) ? 0u : shadowDiskList();
    if (disk != 0) {
        Pbr::get().pause();
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT |
                     GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glDisable(GL_FOG);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        // DIQQAT: polygon-offset ISHLATILMAYDI. Manfiy ofset diskni kameraga
        // tortib, orqadagi devorlarni ham qora bo'yab yuborardi. Disk yerdan
        // 4 sm balandda chiziladi — bu z-fighting uchun yetarli.

        for (size_t i = 0; i < props_.size(); ++i) {
            const Prop& p = props_[i];
            if (thinShadows && (i % 2) == 1) continue;

            const float dx = p.pos.x - camPos.x;
            const float dz = p.pos.z - camPos.z;
            if (dx * dx + dz * dz > kShadow2) continue;

            // FAQAT yerda turgan rekvizitlar soya beradi.
            // Ko'tarilgan qismlar (tom taxtalari, ikkinchi qavat devorlari) soya bermaydi:
            // aks holda zich qishloqda yuzlab disk bir-birining ustiga tushib,
            // polygon-offset tufayli devorlarni ham qop-qora bo'yab yuboradi.
            const float terrY = terrain_.heightAt(p.pos.x, p.pos.z);
            if (!p.snapToGround && p.pos.y > terrY + 0.6f) continue;

            const float gy = p.snapToGround ? terrY : p.pos.y;
            // Soya diski faqat KICHIK rekvizitlar uchun. Katta devor/tom bo'laklari
            // uchun disk ma'nosiz va ekranni qoraytiradi.
            float sr = std::max(p.radius, p.scale * 0.22f) * 0.6f;
            if (sr > 2.2f) continue;          // katta bo'lak — soya chizmaymiz
            if (sr < 0.25f) sr = 0.25f;

            glPushMatrix();
            glTranslatef(p.pos.x, gy + 0.04f, p.pos.z);
            glScalef(sr, 1.0f, sr);
            glCallList((GLuint)disk);
            glPopMatrix();
        }
        glPopAttrib();
        Pbr::get().resume();
    }

    // --- 2) Rekvizitlar ---
    if (noProps) return;
    Pbr::get().setRoughness(0.72f);
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    // MUHIM: soya o'timi glColor4f(..., 0) bilan tugaydi va glColor3fv ALFANI
    // o'zgartirmaydi -> rekvizitlar alfa=0 bilan chizilib ko'rinmay qolardi.
    // Shuning uchun aralashtirishni o'chirib, alfani aniq 1 ga qo'yamiz.
    glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (flatProps) { glDisable(GL_LIGHTING); glDisable(GL_FOG); }

    for (size_t i = 0; i < props_.size(); ++i) {
        const Prop& p = props_[i];
        if (p.cached == nullptr) continue;

        const float dx = p.pos.x - camPos.x;
        const float dz = p.pos.z - camPos.z;
        if (dx * dx + dz * dz > kCull2) continue;

        const float gy = p.snapToGround ? terrain_.heightAt(p.pos.x, p.pos.z) : p.pos.y;

        // Kamera rekvizit ICHIDA qolsa uni chizmaymiz — aks holda mato/devor
        // butun ekranni to'ldirib, sahna ko'rinmay qoladi (cutscene kamerasi
        // katta chodir ichidan o'tganda shu holat yuz beradi).
        {
            const float nearR = (p.radius > 0.6f ? p.radius : 0.9f) * 0.95f;
            const float top   = gy + p.cached->height() * p.scale;
            if (camPos.y < top + 0.3f && dx * dx + dz * dz < nearR * nearR) continue;
        }

        glPushMatrix();
        glTranslatef(p.pos.x, gy, p.pos.z);
        if (p.yaw != 0.0f)   glRotatef(p.yaw, 0.0f, 1.0f, 0.0f);
        if (p.scale != 1.0f) glScalef(p.scale, p.scale, p.scale);
        glColor4f(p.tint[0], p.tint[1], p.tint[2], 1.0f);
        if (p.cached->hasList() && !noLists) p.cached->drawList();
        else                                p.cached->draw();
        glPopMatrix();
    }

    glPopAttrib();
}

} // namespace ert
