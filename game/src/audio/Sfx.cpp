// Jang tovushlarining protsedural sintezi.
//
// Har bir tovush bir necha qatlamdan yig'iladi:
//   - PARTIAL: sinus qism, o'z chastotasi va o'z so'nish vaqti bilan.
//     Metall garmonik EMAS — qismlar 1 : 1.52 : 2.14 : 2.84 nisbatda olinadi
//     (yaqqol "jarang" shu nisbatdan chiqadi, butun sonli nisbat "musiqiy"
//     eshitiladi va zarbaga o'xshamaydi).
//   - NOISE: filtrlangan oq shovqin — zarbaning "shovqinli" boshlanishi.
//   - SWEEP: chastotasi pasayadigan sinus — og'irlik hissi.
//
// Determinatsiyalangan: shovqin uchun oddiy LCG ishlatiladi, seed qat'iy.
// Shuning uchun tovushlar har ishga tushirishda BIR XIL bo'ladi.
#include "ertugrul/audio/Sfx.h"
#include "ertugrul/audio/Audio.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace ert {
namespace {

constexpr int   kRate     = 44100;
constexpr float kFalloff  = 24.0f;   // m — shu masofada ovoz deyarli o'chadi
constexpr float kMinGain  = 0.05f;

// --- determinatsiyalangan shovqin ---
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1u) {}
    float next() {                            // -1 .. +1
        s = s * 1664525u + 1013904223u;
        return (float)((int32_t)(s >> 8) & 0xFFFF) * (1.0f / 32768.0f) - 1.0f;
    }
};

// Bir polyusli past chastota filtri (shovqinni "yumshatadi")
struct LowPass {
    float y = 0.0f, a = 0.5f;
    void setHz(float hz) {
        const float x = std::exp(-TAU * clampf(hz, 20.0f, 20000.0f) / (float)kRate);
        a = 1.0f - x;
    }
    float run(float in) { y += a * (in - y); return y; }
};

// Bir polyusli yuqori chastota filtri (past gumburlashni olib tashlaydi)
struct HighPass {
    float yPrev = 0.0f, xPrev = 0.0f, k = 0.9f;
    void setHz(float hz) {
        const float rc = 1.0f / (TAU * clampf(hz, 10.0f, 18000.0f));
        const float dt = 1.0f / (float)kRate;
        k = rc / (rc + dt);
    }
    float run(float in) {
        const float out = k * (yPrev + in - xPrev);
        xPrev = in; yPrev = out;
        return out;
    }
};

struct Buf {
    std::vector<float> v;
    explicit Buf(float sec) : v((size_t)(clampf(sec, 0.01f, 3.0f) * kRate), 0.0f) {}
    size_t n() const { return v.size(); }
    float  t(size_t i) const { return (float)i / (float)kRate; }
};

// Eksponensial so'nish konverti
inline float decay(float t, float tau) { return std::exp(-t / ((tau > 1e-4f) ? tau : 1e-4f)); }

// Qisqa "hujum" (attack) — nol dan cho'qqiga chiqish; klik tovushini yo'qotadi
inline float attack(float t, float ms) {
    const float a = ms * 0.001f;
    return (a <= 1e-5f) ? 1.0f : clampf(t / a, 0.0f, 1.0f);
}

void addPartial(Buf& b, float hz, float amp, float tau, float phase = 0.0f) {
    const float w = TAU * hz / (float)kRate;
    for (size_t i = 0; i < b.n(); ++i) {
        const float t = b.t(i);
        b.v[i] += amp * decay(t, tau) * attack(t, 1.2f) * std::sin(w * (float)i + phase);
    }
}

// Chastotasi hz0 dan hz1 ga pasayadigan sinus — "og'irlik" hissi
void addSweep(Buf& b, float hz0, float hz1, float amp, float tau) {
    float ph = 0.0f;
    for (size_t i = 0; i < b.n(); ++i) {
        const float t = b.t(i);
        const float u = (b.n() > 1) ? (float)i / (float)(b.n() - 1) : 0.0f;
        const float hz = lerpf(hz0, hz1, u * u);
        ph += TAU * hz / (float)kRate;
        b.v[i] += amp * decay(t, tau) * attack(t, 1.0f) * std::sin(ph);
    }
}

// Filtrlangan shovqin portlashi
void addNoise(Buf& b, float amp, float tau, float lpHz, float hpHz, uint32_t seed) {
    Rng r(seed);
    LowPass lp;  lp.setHz(lpHz);
    HighPass hp; hp.setHz(hpHz);
    for (size_t i = 0; i < b.n(); ++i) {
        const float t = b.t(i);
        float x = hp.run(lp.run(r.next()));
        b.v[i] += amp * decay(t, tau) * attack(t, 0.6f) * x;
    }
}

// Amplitudasi qo'ng'iroq shaklida (havodagi yoy tovushi)
void addWhoosh(Buf& b, float amp, float lpHz, uint32_t seed) {
    Rng r(seed);
    LowPass lp;
    for (size_t i = 0; i < b.n(); ++i) {
        const float u = (b.n() > 1) ? (float)i / (float)(b.n() - 1) : 0.0f;
        // filtr chastotasi o'rtada ochiladi — qilich yonimizdan o'tgandek
        lp.setHz(lerpf(320.0f, lpHz, std::sin(PI * u)));
        const float bell = std::sin(PI * u);
        b.v[i] += amp * bell * bell * lp.run(r.next());
    }
}

SoundRef finish(Buf& b, float peak) {
    // Normallashtirish — qirqilishning (clipping) oldini oladi
    float mx = 0.0f;
    for (size_t i = 0; i < b.n(); ++i) {
        const float a = std::fabs(b.v[i]);
        if (a > mx) mx = a;
    }
    const float g = (mx > 1e-6f) ? (clampf(peak, 0.05f, 0.98f) / mx) : 0.0f;

    SoundRef s = std::make_shared<SoundData>();
    s->channels = 1;
    s->rate     = kRate;
    s->samples.resize(b.n());
    for (size_t i = 0; i < b.n(); ++i) {
        float x = b.v[i] * g;
        // yumshoq chegaralash — keskin qirqilish o'rniga
        x = std::tanh(x * 1.15f);
        int v = (int)(x * 32000.0f);
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        s->samples[i] = (int16_t)v;
    }
    return s;
}

// --------------------------------------------------------------------------
//  Tovushlar
// --------------------------------------------------------------------------

// Qilich tanaga: past "chuq" + qisqa quruq shovqin. Metall jaranglamaydi.
SoundRef mkHit() {
    Buf b(0.26f);
    addSweep(b, 190.0f, 72.0f, 0.85f, 0.055f);
    addPartial(b, 118.0f, 0.40f, 0.070f);
    addNoise(b, 0.55f, 0.030f, 2600.0f, 180.0f, 0x51ADu);
    return finish(b, 0.86f);
}

// Qalqon/qilich to'qnashuvi: garmonik BO'LMAGAN qismlar — metall jarangi.
SoundRef mkBlock() {
    Buf b(0.42f);
    const float f0 = 1180.0f;
    addPartial(b, f0,          0.55f, 0.110f);
    addPartial(b, f0 * 1.52f,  0.42f, 0.085f, 0.7f);
    addPartial(b, f0 * 2.14f,  0.30f, 0.060f, 1.9f);
    addPartial(b, f0 * 2.84f,  0.20f, 0.045f, 2.6f);
    addPartial(b, f0 * 0.5f,   0.25f, 0.070f);
    addNoise(b, 0.45f, 0.018f, 7000.0f, 900.0f, 0x2C71u);
    addSweep(b, 260.0f, 140.0f, 0.22f, 0.045f);   // zarbaning "og'irligi"
    return finish(b, 0.80f);
}

// Parry: yorqinroq va uzunroq — muvaffaqiyat belgisi.
SoundRef mkParry() {
    Buf b(0.60f);
    const float f0 = 1720.0f;
    addPartial(b, f0,          0.55f, 0.220f);
    addPartial(b, f0 * 1.52f,  0.45f, 0.170f, 0.5f);
    addPartial(b, f0 * 2.14f,  0.32f, 0.120f, 1.4f);
    addPartial(b, f0 * 2.84f,  0.24f, 0.090f, 2.2f);
    addPartial(b, f0 * 3.76f,  0.14f, 0.060f, 3.0f);
    addNoise(b, 0.40f, 0.014f, 9000.0f, 1600.0f, 0x7B03u);
    return finish(b, 0.78f);
}

// O'ldiruvchi zarba: og'irroq, pastroq, uzunroq quyruq.
SoundRef mkKill() {
    Buf b(0.55f);
    addSweep(b, 230.0f, 48.0f, 0.95f, 0.130f);
    addPartial(b, 86.0f,  0.55f, 0.190f);
    addPartial(b, 131.0f, 0.30f, 0.110f, 1.1f);
    addNoise(b, 0.60f, 0.045f, 1900.0f, 120.0f, 0x19E4u);
    return finish(b, 0.92f);
}

// Havodagi yoy — nishonga tegmasa ham eshitiladi, zarbaga "og'irlik" beradi.
SoundRef mkSwing() {
    Buf b(0.30f);
    addWhoosh(b, 1.0f, 2400.0f, 0x3F19u);
    return finish(b, 0.42f);
}

// Kamon ipi: ipning chirt etishi + yog'ochning past aks-sadosi.
SoundRef mkBowShot() {
    Buf b(0.34f);
    addNoise(b, 0.75f, 0.012f, 6000.0f, 700.0f, 0x6A2Du);
    addSweep(b, 640.0f, 180.0f, 0.45f, 0.045f);
    addPartial(b, 148.0f, 0.30f, 0.090f);
    addPartial(b, 320.0f, 0.18f, 0.055f, 1.6f);
    return finish(b, 0.70f);
}

// O'q tanaga: qisqa, o'tkir "chirt".
SoundRef mkArrowHit() {
    Buf b(0.18f);
    addNoise(b, 0.70f, 0.014f, 4200.0f, 400.0f, 0x11C7u);
    addSweep(b, 420.0f, 130.0f, 0.50f, 0.030f);
    return finish(b, 0.72f);
}

// O'q devorga/yerga: quruqroq, past "tuq".
SoundRef mkArrowWall() {
    Buf b(0.22f);
    addNoise(b, 0.55f, 0.022f, 1500.0f, 150.0f, 0x4D80u);
    addPartial(b, 165.0f, 0.45f, 0.055f);
    addPartial(b, 240.0f, 0.20f, 0.035f, 0.9f);
    return finish(b, 0.66f);
}

// O'q yig'ib olindi: qisqa yog'och "tak" va yuqori, tez so'nuvchi jarang.
SoundRef mkPickup() {
    Buf b(0.16f);
    addNoise(b, 0.30f, 0.010f, 2600.0f, 400.0f, 0x7A11u);
    addPartial(b, 720.0f, 0.35f, 0.030f);
    addPartial(b, 1480.0f, 0.22f, 0.060f, 0.4f);
    return finish(b, 0.50f);
}

// Tana yerga qulashi: yumshoq, past, uzunroq.
SoundRef mkDeath() {
    Buf b(0.48f);
    addSweep(b, 130.0f, 42.0f, 0.80f, 0.150f);
    addNoise(b, 0.50f, 0.075f, 900.0f, 60.0f, 0x08B5u);
    addPartial(b, 62.0f, 0.40f, 0.200f);
    return finish(b, 0.80f);
}

// --------------------------------------------------------------------------

struct Bank {
    SoundRef s[(int)SfxId::Count];
    bool     built = false;
    Vec3     listener{0.0f, 0.0f, 0.0f};
    int      turn[(int)SfxId::Count] = {0};   // variant hisoblagichi
};

Bank& BK() { static Bank b; return b; }

// Har chaqirishda ohangni biroz o'zgartiramiz — bir xil tovush ketma-ket
// takrorlanganda quloqni charchatmasin. Tasodif YO'Q: aylanma jadval.
const float kPitchTable[6] = { 1.000f, 1.055f, 0.948f, 1.028f, 0.975f, 1.082f };

} // namespace

Sfx& Sfx::get() { static Sfx s; return s; }

bool Sfx::ready() const { return BK().built; }

void Sfx::setListener(const Vec3& p) {
    if (std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z)) BK().listener = p;
}

void Sfx::clear() {
    Bank& b = BK();
    for (int i = 0; i < (int)SfxId::Count; ++i) b.s[i] = nullptr;
    b.built = false;
}

void Sfx::build() {
    Bank& b = BK();
    if (b.built) return;
    b.s[(int)SfxId::Hit]       = mkHit();
    b.s[(int)SfxId::Block]     = mkBlock();
    b.s[(int)SfxId::Parry]     = mkParry();
    b.s[(int)SfxId::Kill]      = mkKill();
    b.s[(int)SfxId::Swing]     = mkSwing();
    b.s[(int)SfxId::BowShot]   = mkBowShot();
    b.s[(int)SfxId::ArrowHit]  = mkArrowHit();
    b.s[(int)SfxId::ArrowWall] = mkArrowWall();
    b.s[(int)SfxId::Death]     = mkDeath();
    b.s[(int)SfxId::Pickup]    = mkPickup();
    b.built = true;

    // ERT_SFX_DUMP=<papka> — tovushlarni WAV qilib yozadi (tinglash va tekshirish)
    if (const char* dir = std::getenv("ERT_SFX_DUMP")) {
        static const char* const nm[(int)SfxId::Count] = {
            "hit", "block", "parry", "kill", "swing", "bowshot",
            "arrow_hit", "arrow_wall", "death", "pickup"
        };
        for (int i = 0; i < (int)SfxId::Count; ++i) {
            if (!b.s[i]) continue;
            char path[512];
            std::snprintf(path, sizeof(path), "%s/%02d_%s.wav", dir, i, nm[i]);
            FILE* f = std::fopen(path, "wb");
            if (f == nullptr) continue;
            const uint32_t nb = (uint32_t)(b.s[i]->samples.size() * 2u);
            const uint32_t rate = (uint32_t)kRate;
            auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
            auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
            std::fwrite("RIFF", 1, 4, f); w32(36u + nb); std::fwrite("WAVE", 1, 4, f);
            std::fwrite("fmt ", 1, 4, f); w32(16u); w16(1); w16(1);
            w32(rate); w32(rate * 2u); w16(2); w16(16);
            std::fwrite("data", 1, 4, f); w32(nb);
            std::fwrite(b.s[i]->samples.data(), 1, nb, f);
            std::fclose(f);
        }
        std::printf("[sfx] WAV lar yozildi: %s\n", dir);
    }

    if (std::getenv("ERT_SFX_LOG") != nullptr) {
        size_t total = 0;
        for (int i = 0; i < (int)SfxId::Count; ++i)
            if (b.s[i]) total += b.s[i]->samples.size();
        std::printf("[sfx] %d ta jang tovushi sintez qilindi (%.0f KB)\n",
                    (int)SfxId::Count, (double)total * 2.0 / 1024.0);
    }
}

void Sfx::play(SfxId id, float distM, float gain) {
    Bank& b = BK();
    const int i = (int)id;
    if (!b.built || i < 0 || i >= (int)SfxId::Count || !b.s[i]) return;
    if (!Audio::get().ready()) return;

    float v = std::isfinite(gain) ? clampf(gain, 0.0f, 2.0f) : 1.0f;
    if (std::isfinite(distM) && distM > 0.0f) {
        // Chiziqli emas — yaqin masofada baland, uzoqda tez so'nadi
        const float f = clampf(1.0f - distM / kFalloff, 0.0f, 1.0f);
        v *= f * f;
    }
    if (v < kMinGain) return;                    // eshitilmaydi — ovoz kanalini band qilmaymiz

    const float pitch = kPitchTable[b.turn[i] % 6];
    b.turn[i] = (b.turn[i] + 1) % 6;
    Audio::get().play(b.s[i], BUS_SFX, v, false, pitch);
}

} // namespace ert
