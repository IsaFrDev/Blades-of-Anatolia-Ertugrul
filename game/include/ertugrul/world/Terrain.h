#pragma once
// Balandlik xaritasiga asoslangan relyef: tepaliklar, o't/tuproq/tosh aralashmasi.
#include <vector>
#include "ertugrul/core/Math.h"

namespace ert {

class Texture;

class Terrain {
public:
    // gridN: to'r hujayralari soni (masalan 128), worldSize: metrda kvadrat tomoni
    bool build(int gridN, float worldSize, uint32_t seed, float hillHeight = 6.0f, float flatRadius = 24.0f);
    void destroy();
    bool valid() const { return gridN_ > 0; }

    float size()  const { return worldSize_; }
    float half()  const { return worldSize_ * 0.5f; }

    float heightAt(float x, float z) const;
    Vec3  normalAt(float x, float z) const;
    // Chegaradan chiqmaslik
    bool  inBounds(float x, float z) const;
    void  clampToBounds(Vec3& p, float margin = 2.0f) const;

    void setTextures(Texture* grass, Texture* dirt, Texture* rock);
    void draw() const;             // display list

private:
    int    gridN_ = 0;
    float  worldSize_ = 0.0f, cell_ = 0.0f;
    std::vector<float> h_;
    unsigned list_ = 0;
    Texture *texGrass_ = nullptr, *texDirt_ = nullptr, *texRock_ = nullptr;
};

} // namespace ert
