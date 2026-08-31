// Ertugrul :: Terrain.cpp
// Balandlik xaritasiga asoslangan relyef.
// Markazda tekis oba maydoni, atrofida silliq ko'tariluvchi tepaliklar,
// chetlarda tabiiy chegara (baland qirg'oq). Verteks ranglari o't/tuproq/tosh
// aralashmasidan hisoblanadi — shuning uchun xarita hech qachon "rangsiz" bo'lmaydi.

#include "ertugrul/world/Terrain.h"
#include "ertugrul/gfx/Texture.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace ert {

namespace {

// ---------------------------------------------------------------------------
// Yordamchi holat.
// Terrain.h — o'zgarmas kontrakt, unga yangi maydon qo'sha olmaymiz. Shu sababli
// verteks ranglari/normallari .cpp ichidagi anonim namespace'dagi jadvalda
// (obyekt manzili bo'yicha) saqlanadi. Bu ma'lumot faqat display list qurishda
// va list yaratilmagan holatdagi zaxira (immediate mode) chizishda kerak.
// ---------------------------------------------------------------------------
struct VertAux {
    float nx = 0.0f, ny = 1.0f, nz = 0.0f;   // sirt normali
    float r = 0.45f, g = 0.52f, b = 0.30f;   // aralashtirilgan asosiy rang
    float rockA = 0.0f;                       // tosh qoplamasi shaffofligi
    float dirtA = 0.0f;                       // tuproq qoplamasi shaffofligi
};

struct TerrainAux {
    std::vector<VertAux> v;
};

std::unordered_map<const Terrain*, TerrainAux>& auxStore() {
    static std::unordered_map<const Terrain*, TerrainAux> s;
    return s;
}

const TerrainAux* auxFind(const Terrain* t) {
    std::unordered_map<const Terrain*, TerrainAux>& s = auxStore();
    std::unordered_map<const Terrain*, TerrainAux>::const_iterator it = s.find(t);
    return (it == s.end()) ? nullptr : &it->second;
}

// fbm2D ning haqiqiy qiymat diapazoni ([0,1] yoki [-1,1]) kafolatlanmagan.
// Shuning uchun butun massivni min/max bo'yicha [0,1] ga keltiramiz — bu
// har qanday amalga oshirilishda bir xil, ishonchli natija beradi.
void normalize01(std::vector<float>& a) {
    if (a.empty()) return;
    float mn = a[0], mx = a[0];
    for (size_t k = 0; k < a.size(); ++k) {
        const float v = a[k];
        if (!(v == v)) { a[k] = 0.0f; continue; }   // NaN himoyasi
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }
    const float d = mx - mn;
    if (d < 1e-6f) {
        for (size_t k = 0; k < a.size(); ++k) a[k] = 0.5f;
        return;
    }
    const float inv = 1.0f / d;
    for (size_t k = 0; k < a.size(); ++k) a[k] = (a[k] - mn) * inv;
}

// Uch komponentli rangni aralashtirish
inline void mix3(float* dst, const float* a, const float* b, float t) {
    dst[0] = lerpf(a[0], b[0], t);
    dst[1] = lerpf(a[1], b[1], t);
    dst[2] = lerpf(a[2], b[2], t);
}

// Bitta verteksni yuborish
inline void emitVert(int i, int j, int V, float cell, float halfSz,
                     const std::vector<float>& h, const VertAux& a, float alpha) {
    const size_t k = (size_t)j * (size_t)V + (size_t)i;
    const float wx = -halfSz + (float)i * cell;
    const float wz = -halfSz + (float)j * cell;
    glNormal3f(a.nx, a.ny, a.nz);
    glColor4f(a.r, a.g, a.b, alpha);
    glTexCoord2f(wx * 0.125f, wz * 0.125f);        // UV = (x,z)/8 — tekstura takrorlanadi
    glVertex3f(wx, h[k], wz);
}

// Butun to'rni GL_TRIANGLES bilan yuborish.
// pass 0 = asosiy (to'liq shaffofmas), 1 = tosh qoplamasi, 2 = tuproq qoplamasi.
void emitTerrain(int gridN, float cell, float halfSz,
                 const std::vector<float>& h, const TerrainAux& aux, int pass) {
    const int V = gridN + 1;
    if ((size_t)V * (size_t)V > h.size() || aux.v.size() < h.size()) return;

    glBegin(GL_TRIANGLES);
    for (int j = 0; j < gridN; ++j) {
        for (int i = 0; i < gridN; ++i) {
            const size_t ia = (size_t)j * V + i;            // (i,   j)
            const size_t ib = (size_t)(j + 1) * V + i;      // (i,   j+1)
            const size_t ic = (size_t)(j + 1) * V + i + 1;  // (i+1, j+1)
            const size_t id = (size_t)j * V + i + 1;        // (i+1, j)

            float aA = 1.0f, aB = 1.0f, aC = 1.0f, aD = 1.0f;
            if (pass == 1) {
                aA = aux.v[ia].rockA; aB = aux.v[ib].rockA;
                aC = aux.v[ic].rockA; aD = aux.v[id].rockA;
            } else if (pass == 2) {
                aA = aux.v[ia].dirtA; aB = aux.v[ib].dirtA;
                aC = aux.v[ic].dirtA; aD = aux.v[id].dirtA;
            }
            if (pass != 0) {
                // Butunlay ko'rinmas hujayralarni o'tkazib yuboramiz (tezlik uchun)
                const float mx = std::max(std::max(aA, aB), std::max(aC, aD));
                if (mx < 0.02f) continue;
            }

            // Yuqoridan qaralganda soat strelkasiga teskari (CCW) — normal +Y
            emitVert(i,     j,     V, cell, halfSz, h, aux.v[ia], aA);
            emitVert(i,     j + 1, V, cell, halfSz, h, aux.v[ib], aB);
            emitVert(i + 1, j + 1, V, cell, halfSz, h, aux.v[ic], aC);

            emitVert(i,     j,     V, cell, halfSz, h, aux.v[ia], aA);
            emitVert(i + 1, j + 1, V, cell, halfSz, h, aux.v[ic], aC);
            emitVert(i + 1, j,     V, cell, halfSz, h, aux.v[id], aD);
        }
    }
    glEnd();
}

// GL 1.3 tokenlari (MinGW ning gl.h si GL 1.1 da to'xtagan)
#ifndef GL_COMBINE
#define GL_COMBINE        0x8570
#define GL_COMBINE_RGB    0x8571
#define GL_RGB_SCALE      0x8573
#define GL_PRIMARY_COLOR  0x8577
#define GL_SRC0_RGB       0x8580
#define GL_SRC1_RGB       0x8581
#endif

// Relyef teksturasi ham, verteks rangi ham "albedo" ni ifodalaydi — ularni oddiy
// ko'paytirsak yer IKKI MARTA qorayadi (protsedural tekstura o'rtachasi ~0.5).
// GL_COMBINE + RGB_SCALE=2 bilan natijani ikkilantirib, teksturani "detal"
// modulyatsiyasiga (o'rtachasi ~1.0) aylantiramiz.
void setBrightModulate() {
    static int supported = -1;
    if (supported < 0) {
        const char* ver = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        supported = (ver && (ver[0] > '1' || (ver[0] == '1' && ver[1] == '.' && ver[2] >= '3'))) ? 1 : 0;
    }
    if (!supported) {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        return;
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
}

} // anonim namespace

// ---------------------------------------------------------------------------
// build
// ---------------------------------------------------------------------------
bool Terrain::build(int gridN, float worldSize, uint32_t seed, float hillHeight, float flatRadius) {
    destroy();

    // Kirish qiymatlarini xavfsiz chegaralarga keltiramiz
    if (gridN < 4)   gridN = 4;
    if (gridN > 512) gridN = 512;
    if (!(worldSize > 4.0f) || !(worldSize < 100000.0f)) worldSize = 320.0f;
    if (!(hillHeight >= 0.0f)) hillHeight = 6.0f;
    if (hillHeight > 200.0f)   hillHeight = 200.0f;
    if (!(flatRadius >= 0.0f)) flatRadius = 24.0f;

    gridN_     = gridN;
    worldSize_ = worldSize;
    cell_      = worldSize_ / (float)gridN_;

    const int    V  = gridN_ + 1;
    const size_t NV = (size_t)V * (size_t)V;
    const float  hs = worldSize_ * 0.5f;
    if (flatRadius > hs * 0.75f) flatRadius = hs * 0.75f;

    h_.assign(NV, 0.0f);

    // --- 1) Xom shovqin qatlamlari -----------------------------------------
    std::vector<float> nBase(NV), nDet(NV), nCol(NV);
    for (int j = 0; j < V; ++j) {
        const float wz = -hs + (float)j * cell_;
        for (int i = 0; i < V; ++i) {
            const float wx = -hs + (float)i * cell_;
            const size_t k = (size_t)j * V + i;
            nBase[k] = fbm2D(wx * 0.010f, wz * 0.010f, seed, 5, 2.0f, 0.5f);
            nDet[k]  = fbm2D(wx * 0.048f, wz * 0.048f, seed ^ 0x9E3779B9u, 3, 2.0f, 0.5f);
            nCol[k]  = fbm2D(wx * 0.260f, wz * 0.260f, seed + 1315423911u, 2, 2.0f, 0.55f);
        }
    }
    normalize01(nBase);
    normalize01(nDet);
    normalize01(nCol);

    // --- 2) Balandliklar ----------------------------------------------------
    const float inner = flatRadius;
    const float outer = flatRadius + std::max(14.0f, flatRadius * 0.9f);
    const float span  = std::max(1.0f, outer - inner);

    for (int j = 0; j < V; ++j) {
        const float wz = -hs + (float)j * cell_;
        for (int i = 0; i < V; ++i) {
            const float wx = -hs + (float)i * cell_;
            const size_t k = (size_t)j * V + i;

            // Asosiy relyef shakli
            float t = saturate(nBase[k] * 0.78f + nDet[k] * 0.22f);
            t = t * t * (3.0f - 2.0f * t);            // platolar/vodiylar kuchayadi
            float hh = hillHeight * t;

            // Markazda TEKIS maydon -> silliq o'tish (smoothstep) bilan tepaliklarga
            const float d     = std::sqrt(wx * wx + wz * wz);
            const float blend = smoothstepf((d - inner) / span);
            hh *= blend;

            // Chetlarda tabiiy chegara (baland qirg'oq)
            const float e    = std::max(std::fabs(wx), std::fabs(wz)) / hs;
            const float edge = smoothstepf((e - 0.62f) / 0.38f);
            hh += edge * hillHeight * 2.0f * (0.55f + 0.45f * nBase[k]);

            // Tekis maydonda ham juda kichik notekislik (mutlaqo tekis "plastik" ko'rinmasin)
            hh += (nDet[k] - 0.5f) * 0.22f * (1.0f - blend);

            h_[k] = hh;
        }
    }

    // --- 3) Normallar va verteks ranglari ----------------------------------
    TerrainAux aux;
    aux.v.assign(NV, VertAux());

    const float hMax = std::max(1.0f, hillHeight * 1.6f);

    for (int j = 0; j < V; ++j) {
        const float wz = -hs + (float)j * cell_;
        for (int i = 0; i < V; ++i) {
            const float wx = -hs + (float)i * cell_;
            const size_t k = (size_t)j * V + i;

            // Markaziy farq bilan normal
            const int im = (i > 0) ? i - 1 : 0;
            const int ip = (i < gridN_) ? i + 1 : gridN_;
            const int jm = (j > 0) ? j - 1 : 0;
            const int jp = (j < gridN_) ? j + 1 : gridN_;
            const float dx = (float)(ip - im) * cell_;
            const float dz = (float)(jp - jm) * cell_;
            const float dhx = (dx > 1e-6f) ? (h_[(size_t)j * V + ip] - h_[(size_t)j * V + im]) / dx : 0.0f;
            const float dhz = (dz > 1e-6f) ? (h_[(size_t)jp * V + i] - h_[(size_t)jm * V + i]) / dz : 0.0f;
            Vec3 n = normalize(Vec3(-dhx, 1.0f, -dhz));
            if (n.y <= 0.0f) n = Vec3(0.0f, 1.0f, 0.0f);

            VertAux& a = aux.v[k];
            a.nx = n.x; a.ny = n.y; a.nz = n.z;

            // --- material og'irliklari ---
            // Qiya joylar -> tosh (normal.y ~0.82 atrofida o'tish)
            const float rockSlope = 1.0f - smoothstepf((n.y - 0.72f) / 0.18f);
            // Baland joylar ham toshloq
            const float hNorm     = saturate(h_[k] / hMax);
            const float rockHigh  = smoothstepf((hNorm - 0.58f) / 0.42f) * 0.85f;
            const float rockW     = saturate(std::max(rockSlope, rockHigh));

            // Markaz (oba maydoni) va yo'lakcha -> tuproq
            const float d = std::sqrt(wx * wx + wz * wz);
            const float dirtCenter = 1.0f - smoothstepf((d - inner * 0.82f) / std::max(4.0f, inner * 0.55f));
            const float dirtPatch  = smoothstepf((nCol[k] - 0.70f) / 0.20f) * 0.45f;
            const float dirtW      = saturate(std::max(dirtCenter, dirtPatch) * (1.0f - rockW));
            const float grassW     = saturate(1.0f - rockW - dirtW);

            // --- ranglar (fbm shovqini bilan xilma-xillik) ---
            const float var = nCol[k];
            static const float grassLo[3] = { 0.22f, 0.36f, 0.15f };
            static const float grassHi[3] = { 0.44f, 0.57f, 0.25f };
            static const float dirtLo [3] = { 0.34f, 0.26f, 0.16f };
            static const float dirtHi [3] = { 0.54f, 0.42f, 0.27f };
            static const float rockLo [3] = { 0.36f, 0.36f, 0.34f };
            static const float rockHi [3] = { 0.62f, 0.60f, 0.56f };

            float cg[3], cd[3], cr[3];
            mix3(cg, grassLo, grassHi, var);
            mix3(cd, dirtLo,  dirtHi,  var);
            mix3(cr, rockLo,  rockHi,  var);

            float col[3];
            for (int c = 0; c < 3; ++c)
                col[c] = cg[c] * grassW + cd[c] * dirtW + cr[c] * rockW;

            // Past joylar biroz to'qroq (soyalik hissi) + mayda shovqin
            const float shade = (0.88f + 0.16f * hNorm) * (0.94f + 0.12f * nDet[k]);
            a.r = saturate(col[0] * shade);
            a.g = saturate(col[1] * shade);
            a.b = saturate(col[2] * shade);

            a.rockA = rockW;
            a.dirtA = dirtW;
        }
    }

    auxStore()[this] = aux;

    // --- 4) Display list'lar (3 ta: asosiy + tosh + tuproq qoplamasi) -------
    const GLuint base = glGenLists(3);
    if (base != 0) {
        list_ = (unsigned)base;
        const TerrainAux& ax = auxStore()[this];
        for (int p = 0; p < 3; ++p) {
            glNewList(base + (GLuint)p, GL_COMPILE);
            emitTerrain(gridN_, cell_, hs, h_, ax, p);
            glEndList();
        }
    } else {
        // GL konteksti tayyor emas — zaxira yo'l: har kadrda immediate mode.
        list_ = 0;
    }

    return true;
}

// ---------------------------------------------------------------------------
void Terrain::destroy() {
    if (list_ != 0) {
        glDeleteLists((GLuint)list_, 3);
        list_ = 0;
    }
    auxStore().erase(this);
    h_.clear();
    h_.shrink_to_fit();
    gridN_ = 0;
    worldSize_ = 0.0f;
    cell_ = 0.0f;
    texGrass_ = texDirt_ = texRock_ = nullptr;
}

// ---------------------------------------------------------------------------
void Terrain::setTextures(Texture* grass, Texture* dirt, Texture* rock) {
    texGrass_ = grass;
    texDirt_  = dirt;
    texRock_  = rock;
}

// ---------------------------------------------------------------------------
float Terrain::heightAt(float x, float z) const {
    if (gridN_ <= 0 || h_.empty() || cell_ <= 1e-6f) return 0.0f;
    // NaN himoyasi — buzuq kirish qiymati butun o'yinni "zaharlab" yubormasin
    if (!(x == x)) x = 0.0f;
    if (!(z == z)) z = 0.0f;
    const int V = gridN_ + 1;
    const float hs = worldSize_ * 0.5f;

    // To'r koordinatalari; chegaradan tashqarida eng yaqin qiymat (clamp)
    float fx = (x + hs) / cell_;
    float fz = (z + hs) / cell_;
    fx = clampf(fx, 0.0f, (float)gridN_);
    fz = clampf(fz, 0.0f, (float)gridN_);

    int i0 = (int)fx; if (i0 > gridN_ - 1) i0 = gridN_ - 1; if (i0 < 0) i0 = 0;
    int j0 = (int)fz; if (j0 > gridN_ - 1) j0 = gridN_ - 1; if (j0 < 0) j0 = 0;
    const int i1 = i0 + 1, j1 = j0 + 1;

    const float tx = clampf(fx - (float)i0, 0.0f, 1.0f);
    const float tz = clampf(fz - (float)j0, 0.0f, 1.0f);

    const size_t k00 = (size_t)j0 * V + i0;
    const size_t k10 = (size_t)j0 * V + i1;
    const size_t k01 = (size_t)j1 * V + i0;
    const size_t k11 = (size_t)j1 * V + i1;
    if (k11 >= h_.size()) return 0.0f;

    // Bilinear interpolyatsiya
    const float a = lerpf(h_[k00], h_[k10], tx);
    const float b = lerpf(h_[k01], h_[k11], tx);
    return lerpf(a, b, tz);
}

// ---------------------------------------------------------------------------
Vec3 Terrain::normalAt(float x, float z) const {
    if (gridN_ <= 0 || h_.empty()) return Vec3(0.0f, 1.0f, 0.0f);
    const float e = (cell_ > 0.05f) ? cell_ : 0.5f;
    // Markaziy farq
    const float hl = heightAt(x - e, z);
    const float hr = heightAt(x + e, z);
    const float hd = heightAt(x, z - e);
    const float hu = heightAt(x, z + e);
    Vec3 n = normalize(Vec3(-(hr - hl) / (2.0f * e), 1.0f, -(hu - hd) / (2.0f * e)));
    if (n.y <= 0.0f) return Vec3(0.0f, 1.0f, 0.0f);
    return n;
}

// ---------------------------------------------------------------------------
bool Terrain::inBounds(float x, float z) const {
    if (gridN_ <= 0) return false;
    if (!(x == x) || !(z == z)) return false;          // NaN -> chegaradan tashqarida
    const float hs = worldSize_ * 0.5f;
    return (x >= -hs && x <= hs && z >= -hs && z <= hs);
}

void Terrain::clampToBounds(Vec3& p, float margin) const {
    if (gridN_ <= 0) return;
    if (!(p.x == p.x)) p.x = 0.0f;                     // NaN himoyasi
    if (!(p.y == p.y)) p.y = 0.0f;
    if (!(p.z == p.z)) p.z = 0.0f;
    float hs = worldSize_ * 0.5f;
    if (margin < 0.0f) margin = 0.0f;
    if (margin > hs * 0.9f) margin = hs * 0.9f;
    const float lim = hs - margin;
    p.x = clampf(p.x, -lim, lim);
    p.z = clampf(p.z, -lim, lim);
}

// ---------------------------------------------------------------------------
void Terrain::draw() const {
    if (gridN_ <= 0 || h_.empty()) return;

    // Aux ma'lumot topilmasa (masalan obyekt nusxalangan bo'lsa) — zaxira
    TerrainAux fallback;
    const TerrainAux* aux = auxFind(this);
    if (!aux || aux->v.size() < h_.size()) {
        fallback.v.assign(h_.size(), VertAux());
        aux = &fallback;
    }

    const float hs = worldSize_ * 0.5f;

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    // --- Asosiy o'tim: o't teksturasi verteks ranglariga KO'PAYTIRILADI ---
    const bool useGrass = (texGrass_ != nullptr && texGrass_->valid());
    if (useGrass) {
        glEnable(GL_TEXTURE_2D);
        setBrightModulate();
        texGrass_->bind();
    } else {
        glDisable(GL_TEXTURE_2D);
    }

    if (list_ != 0) glCallList((GLuint)list_);
    else            emitTerrain(gridN_, cell_, hs, h_, *aux, 0);

    // --- Qoplama o'timlari: tosh va tuproq teksturalari (alpha = og'irlik) ---
    const Texture* overlay[2] = { texRock_, texDirt_ };
    bool anyOverlay = false;
    for (int p = 0; p < 2; ++p)
        if (overlay[p] != nullptr && overlay[p]->valid()) anyOverlay = true;

    if (anyOverlay) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        for (int p = 0; p < 2; ++p) {
            if (overlay[p] == nullptr || !overlay[p]->valid()) continue;
            overlay[p]->bind();
            if (list_ != 0) glCallList((GLuint)list_ + 1 + (GLuint)p);
            else            emitTerrain(gridN_, cell_, hs, h_, *aux, 1 + p);
        }
    }

    glPopAttrib();
}

} // namespace ert
