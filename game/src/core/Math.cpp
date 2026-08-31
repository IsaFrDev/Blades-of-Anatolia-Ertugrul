// Ertugrul :: core/Math.cpp
// Determinatsiyalangan qiymat shovqini (value noise) va fBm.
// Tashqi kutubxonasiz, faqat standart C++.
#include "ertugrul/core/Math.h"

namespace ert {

namespace {

// --- Butun koordinatalar uchun aralashtiruvchi (hash) funksiya ---
// Bir xil (x, y, seed) uchun HAR DOIM bir xil natija beradi.
// Bu FNV/Wang-ga o'xshash ko'p bosqichli aralashtirish: qo'shni katakchalar
// o'rtasida korrelyatsiya qolmasligi uchun 3 marta xor-shift + ko'paytirish.
inline uint32_t hashXY(int32_t x, int32_t y, uint32_t seed) noexcept {
    uint32_t h = seed * 374761393u + 2166136261u;
    h += static_cast<uint32_t>(x) * 3266489917u;
    h ^= h >> 15;
    h += static_cast<uint32_t>(y) * 668265263u;
    h ^= h >> 13;
    h *= 2246822519u;
    h ^= h >> 16;
    h *= 3266489917u;
    h ^= h >> 15;
    return h;
}

// 32-bitli hash -> [0,1] oralig'idagi float (yuqori 24 bit ishlatiladi)
inline float hashToUnit(uint32_t h) noexcept {
    return static_cast<float>(h >> 8) * (1.0f / 16777215.0f);
}

// std::floor ni int32 ga xavfsiz keltirish (juda katta qiymatlarda tashib
// ketmasligi uchun chegaralaymiz — o'yin hech qachon crash bo'lmasin).
inline int32_t safeFloorI(float v) noexcept {
    if (!(v > -1.0e9f)) return -1000000000;   // NaN ham shu yerga tushadi
    if (!(v <  1.0e9f)) return  1000000000;
    return static_cast<int32_t>(std::floor(v));
}

} // anonim namespace

// --- Qiymat shovqini (value noise) ---
// Butun to'r nuqtalarida hash-asosli tasodifiy qiymat olinadi,
// oraliq nuqtalarda smoothstep bilan bikubik-yaqin silliq interpolyatsiya.
// Natija har doim [0,1] oralig'ida.
float valueNoise2D(float x, float y, uint32_t seed) {
    const float fx = std::floor(x);
    const float fy = std::floor(y);
    const int32_t ix = safeFloorI(x);
    const int32_t iy = safeFloorI(y);

    // Katakcha ichidagi kasr qism (NaN/inf hollarida 0 ga tushadi)
    float tx = x - fx;
    float ty = y - fy;
    if (!(tx >= 0.0f && tx <= 1.0f)) tx = 0.0f;
    if (!(ty >= 0.0f && ty <= 1.0f)) ty = 0.0f;

    // Silliq (Hermite) interpolyatsiya vazni
    const float sx = smoothstepf(tx);
    const float sy = smoothstepf(ty);

    // To'rning 4 ta burchagi
    const float v00 = hashToUnit(hashXY(ix,     iy,     seed));
    const float v10 = hashToUnit(hashXY(ix + 1, iy,     seed));
    const float v01 = hashToUnit(hashXY(ix,     iy + 1, seed));
    const float v11 = hashToUnit(hashXY(ix + 1, iy + 1, seed));

    const float a = lerpf(v00, v10, sx);
    const float b = lerpf(v01, v11, sx);
    return saturate(lerpf(a, b, sy));
}

// --- fBm (fractal Brownian motion) ---
// Bir necha oktavaning yig'indisi: har bir keyingi oktava chastotasi
// `lacunarity` marta katta, amplitudasi `gain` marta kichik.
// Natija amplitudalar yig'indisiga bo'linib [0,1] ga normallashtiriladi.
float fbm2D(float x, float y, uint32_t seed, int octaves, float lacunarity, float gain) {
    // Parametrlarni xavfsiz chegaraga keltiramiz
    if (octaves < 1)  octaves = 1;
    if (octaves > 16) octaves = 16;
    if (!(lacunarity > 0.0f) || lacunarity > 16.0f) lacunarity = 2.0f;
    if (!(gain > 0.0f) || gain > 1.0f)              gain = 0.5f;

    float sum   = 0.0f;
    float norm  = 0.0f;
    float amp   = 1.0f;
    float freq  = 1.0f;

    for (int i = 0; i < octaves; ++i) {
        // Har bir oktava uchun boshqa urug' — oktavalar bir-birini takrorlamasin
        const uint32_t s = seed + static_cast<uint32_t>(i) * 0x9E3779B9u;
        sum  += amp * valueNoise2D(x * freq, y * freq, s);
        norm += amp;
        amp  *= gain;
        freq *= lacunarity;

        // Amplituda sezilmas darajaga tushsa — to'xtaymiz (tejash)
        if (amp < 1.0e-5f) break;
    }

    if (norm <= 0.0f) return 0.0f;
    return saturate(sum / norm);
}

} // namespace ert
