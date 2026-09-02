#pragma once
// Ma'lumotga asoslangan daraja: relyef + rekvizitlar + to'qnashuv + osmon/yorug'lik.
#include <string>
#include <vector>
#include "ertugrul/core/Math.h"
#include "ertugrul/world/Terrain.h"

namespace ert {

class Mesh;
class Texture;

struct Prop {
    std::string mesh;             // .obj yo'li ("cube","cylinder","cone","sphere" = protsedural)
    Vec3   pos{0,0,0};
    float  yaw = 0.0f;
    float  scale = 1.0f;
    float  tint[3] = {1,1,1};
    bool   collide = false;
    float  radius = 0.5f;
    bool   snapToGround = true;
    Mesh*  cached = nullptr;      // yuklanganda to'ldiriladi
};

struct SpawnPoint { std::string id; Vec3 pos{0,0,0}; float yaw = 0.0f; };

struct SkyPreset {
    float horizon[3] = {0.62f, 0.72f, 0.86f};
    float zenith[3]  = {0.22f, 0.42f, 0.78f};
    float sunDir[3]  = {0.4f, 0.8f, 0.35f};
    float sunColor[3]= {1.0f, 0.96f, 0.86f};
    float ambient[3] = {0.35f, 0.36f, 0.42f};
    float fogColor[3]= {0.66f, 0.74f, 0.84f};
    float fogStart = 60.0f, fogEnd = 240.0f;
};

class Level {
public:
    // data/levels/<id>.json dan yuklaydi; topilmasa protsedural oba quriladi.
    bool load(const std::string& levelId);
    void buildProceduralOba(uint32_t seed);
    void destroy();

    void applyTimeOfDay(const std::string& tod, const std::string& weather);   // dawn/day/dusk/night
    const SkyPreset& sky() const { return sky_; }

    Terrain& terrain() { return terrain_; }
    const Terrain& terrain() const { return terrain_; }

    // Yorug'lik + tuman + osmonni o'rnatadi va rekvizitlarni chizadi
    void drawSky(const Vec3& camPos) const;
    void applyLighting() const;
    void draw(const Vec3& camPos) const;
    // Soya xaritasi o'timi: relyef + fokus atrofidagi rekvizitlar, faqat geometriya
    // (rang/material/soya disklari yo'q). ShadowMap::begin() dan keyin chaqiriladi.
    void drawCasters(const Vec3& focus, float radius) const;

    // XZ da doiraviy to'qnashuv; pos ni tuzatadi, to'qnashuv bo'lsa true
    bool resolveCollision(Vec3& pos, float radius) const;
    float groundAt(float x, float z) const { return terrain_.heightAt(x, z); }

    const SpawnPoint* spawn(const std::string& id) const;
    const std::vector<Prop>& props() const { return props_; }
    const std::string& id() const { return id_; }
    const std::string& displayName() const { return locName_; }

private:
    std::string id_, locName_;
    Terrain     terrain_;
    SkyPreset   sky_;
    std::vector<Prop>       props_;
    std::vector<SpawnPoint> spawns_;
};

} // namespace ert
