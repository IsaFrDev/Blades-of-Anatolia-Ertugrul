// Audio.cpp — waveOut asosidagi dasturiy aralashtirgich (software mixer).
// Tashqi kutubxona yo'q: faqat Win32 + winmm (waveOut).
//
// Ishlash tamoyili:
//   * init() da 16-bit stereo waveOut qurilmasi ochiladi va N ta halqa buferi
//     (har biri ~40 ms) tayyorlanadi (waveOutPrepareHeader).
//   * CALLBACK_NULL ishlatiladi — drayver callback'i YO'Q. Bufer bo'shaganini
//     WHDR_INQUEUE bayrog'i orqali update() ichida tekshiramiz. Bu deadlock
//     xavfini butunlay yo'q qiladi.
//   * Butun aralashtirish update() ichida (o'yin thread'ida) bajariladi.
//     Ovoz qo'shish/o'chirish boshqa thread'dan kelishi mumkin (masalan Voice.cpp),
//     shuning uchun barcha umumiy holat CRITICAL_SECTION bilan himoyalangan.
//   * Qurilma ochilmasa ready_=false bo'ladi, lekin API'lar jimgina ishlaydi:
//     ovozlar real vaqt bo'yicha "quruq" surilib boradi, shuning uchun
//     isPlaying() va davomiyliklar to'g'ri qoladi — o'yin ovozsiz ishlayveradi.

#include <windows.h>
#include <mmsystem.h>

#include "ertugrul/audio/Audio.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Kichik yordamchilar
// ---------------------------------------------------------------------------

// CRITICAL_SECTION uchun RAII qulf. Win32 CS rekursiv — bir thread ichida
// qayta kirish xavfsiz (crossfadeMusic ichidan play() chaqirish uchun kerak).
struct Lock {
    CRITICAL_SECTION* cs;
    explicit Lock(CRITICAL_SECTION* c) : cs(c) { if (cs) EnterCriticalSection(cs); }
    ~Lock() { if (cs) LeaveCriticalSection(cs); }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
};

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
inline int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// UTF-8 -> UTF-16 (fayl yo'llari uchun; kirill/lotin nomlarni ham qo'llab-quvvatlaydi)
std::wstring widen(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

// Faylni to'liq o'qish. Juda katta fayllarni rad etamiz (xotira himoyasi).
const unsigned long kMaxFileBytes = 192u * 1024u * 1024u;

bool readWholeFile(const std::string& path, std::vector<unsigned char>& out) {
    out.clear();
    std::wstring wp = widen(path);
    if (wp.empty()) return false;
    HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER sz;
    sz.QuadPart = 0;
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 ||
        (unsigned long long)sz.QuadPart > (unsigned long long)kMaxFileBytes) {
        CloseHandle(h);
        return false;
    }

    out.resize((size_t)sz.QuadPart);
    size_t done = 0;
    while (done < out.size()) {
        DWORD want = (DWORD)std::min<size_t>(out.size() - done, 1u << 20);
        DWORD got = 0;
        if (!ReadFile(h, out.data() + done, want, &got, nullptr) || got == 0) break;
        done += got;
    }
    CloseHandle(h);
    if (done != out.size()) { out.clear(); return false; }
    return true;
}

inline unsigned short rd16(const unsigned char* p) {
    return (unsigned short)((unsigned)p[0] | ((unsigned)p[1] << 8));
}
inline unsigned int rd32(const unsigned char* p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

// ---------------------------------------------------------------------------
// Faol ovoz (voice)
// ---------------------------------------------------------------------------
struct Voice {
    ert::SoundRef snd;
    double pos      = 0.0;      // manba ichidagi kadr pozitsiyasi (kasrli)
    double step     = 1.0;      // bitta chiqish kadriga qancha manba kadri
    float  vol      = 1.0f;     // buyurilgan hajm (fade shuni o'zgartiradi)
    float  volTarget= 1.0f;     // fade maqsadi
    float  volRate  = 0.0f;     // hajm birligi / soniya (0 = fade yo'q)
    float  gain     = 0.0f;     // amalda qo'llanayotgan silliq kuchaytirish
    bool   gainInit = false;    // birinchi blokda sakrashsiz boshlash uchun
    int    bus      = ert::BUS_SFX;
    bool   loop     = false;
    bool   killAtZero = false;  // hajm 0 ga yetganda o'chirilsin
    bool   killing  = false;    // stop() chaqirilgan (isPlaying=false)
    bool   dead     = false;
    int    id       = 0;
};

const int   kBufCount     = 4;        // halqa buferlari soni
const int   kBufMs        = 40;       // har bir bufer ~40 ms
const int   kMaxVoices    = 64;       // bir vaqtda maksimal ovoz
const int   kMaxCache     = 256;      // playFile keshidagi maksimal yozuv
const float kStopFadeSec  = 0.012f;   // stop() dagi mikro-so'nish (shiqillashga qarshi)

struct AudioState {
    CRITICAL_SECTION cs;

    HWAVEOUT hwo = nullptr;
    int  rate         = 44100;
    int  framesPerBuf = 1764;
    bool opened       = false;

    WAVEHDR            hdr[kBufCount];
    std::vector<short> buf[kBufCount];
    bool               prepared[kBufCount];

    std::vector<int>   acc;       // int32 yig'indi akkumulyatori (interleaved L,R)
    std::vector<Voice> voices;
    int   nextId = 1;
    float busVol[ert::BUS_COUNT];

    // Qurilmasiz ("silent") rejim uchun soat
    LARGE_INTEGER lastTick;
    LONGLONG      qpcFreq = 0;
    bool          tickValid = false;

    // playFile keshi (nullptr = fayl buzuq/yo'q, qayta urinmaymiz)
    std::map<std::string, ert::SoundRef> cache;

    AudioState() {
        InitializeCriticalSection(&cs);
        std::memset(hdr, 0, sizeof(hdr));
        for (int i = 0; i < kBufCount; ++i) prepared[i] = false;
        for (int i = 0; i < ert::BUS_COUNT; ++i) busVol[i] = 1.0f;
        lastTick.QuadPart = 0;
        LARGE_INTEGER f;
        if (QueryPerformanceFrequency(&f) && f.QuadPart > 0) qpcFreq = f.QuadPart;
    }
    ~AudioState() {
        DeleteCriticalSection(&cs);
    }
    AudioState(const AudioState&) = delete;
    AudioState& operator=(const AudioState&) = delete;
};

AudioState& S() {
    static AudioState s;
    return s;
}

// Bus kuchaytirishi (master har doim qo'shiladi, lekin ikki marta emas)
float busGain(const AudioState& st, int bus) {
    const float m = st.busVol[ert::BUS_MASTER];
    if (bus <= ert::BUS_MASTER || bus >= ert::BUS_COUNT) return m;
    return st.busVol[bus] * m;
}

// Bitta blok uchun fade holatini yangilaydi.
// Qaytaradi: shu blokdan keyin ovoz o'chirilishi kerakmi.
bool advanceFade(Voice& v, float dt) {
    if (v.volRate > 0.0f) {
        if (v.vol < v.volTarget) {
            v.vol += v.volRate * dt;
            if (v.vol >= v.volTarget) { v.vol = v.volTarget; v.volRate = 0.0f; }
        } else if (v.vol > v.volTarget) {
            v.vol -= v.volRate * dt;
            if (v.vol <= v.volTarget) { v.vol = v.volTarget; v.volRate = 0.0f; }
        } else {
            v.volRate = 0.0f;
        }
    }
    if (v.vol < 0.0f) v.vol = 0.0f;
    if (v.killAtZero && v.vol <= 0.0001f) {
        v.vol = 0.0f;
        return true;      // blok baribir aralashtiriladi (gain 0 ga ramp qiladi)
    }
    return false;
}

// Manbadan chiziqli interpolyatsiya bilan bitta kadr o'qish (mono->stereo kengaytirish).
inline void fetchFrame(const ert::SoundData& sd, long long srcFrames,
                       double pos, bool loop, float& outL, float& outR) {
    long long i0 = (long long)pos;
    if (i0 < 0) i0 = 0;
    if (i0 >= srcFrames) i0 = srcFrames - 1;
    double frac = pos - (double)i0;
    if (frac < 0.0) frac = 0.0;

    long long i1 = i0 + 1;
    if (i1 >= srcFrames) i1 = loop ? 0 : i0;

    const short* p = sd.samples.data();
    if (sd.channels <= 1) {
        float a = (float)p[i0];
        float b = (float)p[i1];
        float s = a + (b - a) * (float)frac;
        outL = s;
        outR = s;
    } else {
        const size_t c = (size_t)sd.channels;
        float a0 = (float)p[(size_t)i0 * c + 0];
        float b0 = (float)p[(size_t)i1 * c + 0];
        float a1 = (float)p[(size_t)i0 * c + 1];
        float b1 = (float)p[(size_t)i1 * c + 1];
        outL = a0 + (b0 - a0) * (float)frac;
        outR = a1 + (b1 - a1) * (float)frac;
    }
}

// O'lgan ovozlarni ro'yxatdan olib tashlash
void reapDead(AudioState& st) {
    st.voices.erase(std::remove_if(st.voices.begin(), st.voices.end(),
                                   [](const Voice& v) { return v.dead; }),
                    st.voices.end());
}

// Bitta blokni aralashtirib `out` ga yozadi (interleaved stereo int16).
// Chaqirilishidan oldin qulf olingan bo'lishi shart.
void mixBlock(AudioState& st, short* out, int frames) {
    if (!out || frames <= 0) return;

    const size_t n = (size_t)frames * 2;
    if (st.acc.size() < n) st.acc.assign(n, 0);
    else std::fill(st.acc.begin(), st.acc.begin() + (ptrdiff_t)n, 0);

    const float dt    = (float)frames / (float)st.rate;
    const int   rampN = std::min(frames, std::max(1, st.rate / 200));   // ~5 ms silliqlash

    for (size_t vi = 0; vi < st.voices.size(); ++vi) {
        Voice& v = st.voices[vi];
        if (v.dead) continue;

        const ert::SoundData* sd = v.snd.get();
        if (!sd || sd->samples.empty() || sd->channels < 1) { v.dead = true; continue; }

        const long long srcFrames = (long long)(sd->samples.size() / (size_t)sd->channels);
        if (srcFrames <= 0) { v.dead = true; continue; }

        const bool dieAfter = advanceFade(v, dt);

        const float gTarget = clampf(v.vol, 0.0f, 4.0f) * busGain(st, v.bus);
        if (!v.gainInit) { v.gain = gTarget; v.gainInit = true; }

        float g    = v.gain;
        const float gInc = (gTarget - g) / (float)rampN;

        int* acc = st.acc.data();
        bool ended = false;

        for (int i = 0; i < frames; ++i) {
            if (v.pos >= (double)srcFrames) {
                if (v.loop) {
                    v.pos = std::fmod(v.pos, (double)srcFrames);
                    if (v.pos < 0.0) v.pos = 0.0;
                } else {
                    ended = true;
                    break;
                }
            }
            float l = 0.0f, r = 0.0f;
            fetchFrame(*sd, srcFrames, v.pos, v.loop, l, r);

            acc[i * 2 + 0] += (int)(l * g);
            acc[i * 2 + 1] += (int)(r * g);

            v.pos += v.step;
            if (i < rampN) g += gInc; else g = gTarget;
        }

        v.gain = gTarget;
        if (ended || dieAfter) v.dead = true;
    }

    reapDead(st);

    // int32 -> int16, kesish (clipping)
    const int* acc = st.acc.data();
    for (size_t i = 0; i < n; ++i) {
        int s = acc[i];
        if (s > 32767) s = 32767;
        else if (s < -32768) s = -32768;
        out[i] = (short)s;
    }
}

// Qurilma yo'q bo'lganda: ovozlarni real vaqt bo'yicha oldinga suradi.
// Chaqirilishidan oldin qulf olingan bo'lishi shart.
void advanceSilent(AudioState& st, float dt) {
    if (dt <= 0.0f) return;
    const double outFrames = (double)dt * (double)st.rate;

    for (size_t vi = 0; vi < st.voices.size(); ++vi) {
        Voice& v = st.voices[vi];
        if (v.dead) continue;

        const ert::SoundData* sd = v.snd.get();
        if (!sd || sd->samples.empty() || sd->channels < 1) { v.dead = true; continue; }

        const long long srcFrames = (long long)(sd->samples.size() / (size_t)sd->channels);
        if (srcFrames <= 0) { v.dead = true; continue; }

        const bool dieAfter = advanceFade(v, dt);
        v.pos += outFrames * v.step;

        if (v.pos >= (double)srcFrames) {
            if (v.loop) {
                v.pos = std::fmod(v.pos, (double)srcFrames);
                if (v.pos < 0.0 || v.pos != v.pos) v.pos = 0.0;   // NaN himoyasi
            } else {
                v.dead = true;
            }
        }
        if (dieAfter) v.dead = true;
    }
    reapDead(st);
}

// O'tgan vaqtni o'lchash (silent rejim uchun)
float tickDelta(AudioState& st) {
    if (st.qpcFreq <= 0) return 1.0f / 60.0f;
    LARGE_INTEGER now;
    if (!QueryPerformanceCounter(&now)) return 1.0f / 60.0f;
    if (!st.tickValid) {
        st.lastTick = now;
        st.tickValid = true;
        return 0.0f;
    }
    double d = (double)(now.QuadPart - st.lastTick.QuadPart) / (double)st.qpcFreq;
    st.lastTick = now;
    if (d < 0.0) d = 0.0;
    if (d > 0.25) d = 0.25;      // uzoq to'xtashdan keyin sakrab ketmasin
    return (float)d;
}

// Ichki: yangi ovoz qo'shish. Qulf olingan bo'lishi shart. 0 = muvaffaqiyatsiz.
int addVoice(AudioState& st, ert::SoundRef s, int bus, float volume, bool loop,
             float pitch, float fadeInSec) {
    if (!s || s->samples.empty() || s->channels < 1 || s->rate <= 0) return 0;

    if ((int)st.voices.size() >= kMaxVoices) {
        // Eng eski takrorlanmaydigan ovozni qurbon qilamiz
        bool freed = false;
        for (size_t i = 0; i < st.voices.size(); ++i) {
            if (!st.voices[i].loop) { st.voices.erase(st.voices.begin() + (ptrdiff_t)i); freed = true; break; }
        }
        if (!freed) return 0;
    }

    Voice v;
    v.snd  = s;
    v.pos  = 0.0;
    v.bus  = clampi(bus, 0, ert::BUS_COUNT - 1);
    v.loop = loop;
    v.step = ((double)s->rate / (double)st.rate) * (double)clampf(pitch, 0.05f, 16.0f);
    if (v.step <= 0.0) v.step = 1.0;

    const float target = clampf(volume, 0.0f, 4.0f);
    if (fadeInSec > 0.0001f) {
        v.vol       = 0.0f;
        v.volTarget = target;
        v.volRate   = target / fadeInSec;
        if (v.volRate <= 0.0f) { v.vol = target; v.volRate = 0.0f; }
    } else {
        v.vol       = target;
        v.volTarget = target;
        v.volRate   = 0.0f;
    }

    if (st.nextId <= 0) st.nextId = 1;      // o'ralib ketishdan himoya
    v.id = st.nextId++;
    st.voices.push_back(v);
    return v.id;
}

// Ichki: ovozni mikro-so'nish bilan to'xtatish. Qulf olingan bo'lishi shart.
void killVoice(Voice& v) {
    v.killing    = true;
    v.killAtZero = true;
    v.volTarget  = 0.0f;
    float from = v.vol > 0.0f ? v.vol : 0.0001f;
    v.volRate  = from / kStopFadeSec;
}

} // anonim namespace

namespace ert {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
Audio& Audio::get() {
    static Audio inst;
    return inst;
}

// ---------------------------------------------------------------------------
// init / shutdown
// ---------------------------------------------------------------------------
bool Audio::init(int sampleRate) {
    AudioState& st = S();
    Lock lk(&st.cs);

    if (st.opened) { ready_ = true; return true; }

    st.rate = clampi(sampleRate, 8000, 192000);
    st.framesPerBuf = (st.rate * kBufMs) / 1000;
    if (st.framesPerBuf < 64) st.framesPerBuf = 64;

    WAVEFORMATEX wf;
    std::memset(&wf, 0, sizeof(wf));
    wf.wFormatTag      = WAVE_FORMAT_PCM;
    wf.nChannels       = 2;
    wf.nSamplesPerSec  = (DWORD)st.rate;
    wf.wBitsPerSample  = 16;
    wf.nBlockAlign     = (WORD)(wf.nChannels * wf.wBitsPerSample / 8);
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    wf.cbSize          = 0;

    HWAVEOUT h = nullptr;
    // CALLBACK_NULL — drayver bizni chaqirmaydi, hammasini update() qiladi.
    MMRESULT mr = waveOutOpen(&h, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL);
    if (mr != MMSYSERR_NOERROR || !h) {
        st.hwo = nullptr;
        st.opened = false;
        ready_ = false;
        st.acc.assign((size_t)st.framesPerBuf * 2, 0);
        return false;               // ovozsiz rejim: API'lar baribir ishlaydi
    }
    st.hwo = h;

    bool ok = true;
    for (int i = 0; i < kBufCount; ++i) {
        st.buf[i].assign((size_t)st.framesPerBuf * 2, 0);
        std::memset(&st.hdr[i], 0, sizeof(WAVEHDR));
        st.hdr[i].lpData         = (LPSTR)st.buf[i].data();
        st.hdr[i].dwBufferLength = (DWORD)(st.buf[i].size() * sizeof(short));
        if (waveOutPrepareHeader(st.hwo, &st.hdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
            ok = false;
            break;
        }
        st.prepared[i] = true;
    }

    if (!ok) {
        for (int i = 0; i < kBufCount; ++i) {
            if (st.prepared[i]) { waveOutUnprepareHeader(st.hwo, &st.hdr[i], sizeof(WAVEHDR)); st.prepared[i] = false; }
        }
        waveOutClose(st.hwo);
        st.hwo = nullptr;
        st.opened = false;
        ready_ = false;
        st.acc.assign((size_t)st.framesPerBuf * 2, 0);
        return false;
    }

    st.acc.assign((size_t)st.framesPerBuf * 2, 0);
    st.opened = true;
    ready_    = true;
    return true;
}

void Audio::shutdown() {
    AudioState& st = S();
    Lock lk(&st.cs);

    ready_ = false;
    st.voices.clear();

    if (st.hwo) {
        waveOutReset(st.hwo);
        for (int i = 0; i < kBufCount; ++i) {
            if (st.prepared[i]) {
                waveOutUnprepareHeader(st.hwo, &st.hdr[i], sizeof(WAVEHDR));
                st.prepared[i] = false;
            }
        }
        waveOutClose(st.hwo);
        st.hwo = nullptr;
    }
    st.opened = false;
    st.tickValid = false;
    st.cache.clear();
}

// ---------------------------------------------------------------------------
// Har kadrda chaqiriladi
// ---------------------------------------------------------------------------
void Audio::update() {
    AudioState& st = S();
    Lock lk(&st.cs);

    if (!ready_ || !st.hwo || !st.opened) {
        // Qurilmasiz rejim: ovozlar baribir vaqt bo'yicha tugaydi
        advanceSilent(st, tickDelta(st));
        return;
    }
    st.tickValid = false;   // qurilma qaytsa soat qaytadan boshlanadi

    for (int i = 0; i < kBufCount; ++i) {
        if (!st.prepared[i]) continue;
        if (st.hdr[i].dwFlags & WHDR_INQUEUE) continue;      // hali o'ynalmoqda

        mixBlock(st, st.buf[i].data(), st.framesPerBuf);

        st.hdr[i].dwBufferLength = (DWORD)(st.buf[i].size() * sizeof(short));
        st.hdr[i].dwFlags &= ~(DWORD)WHDR_DONE;              // qayta ishlatishga tayyorlash
        st.hdr[i].dwLoops = 0;

        if (waveOutWrite(st.hwo, &st.hdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
            // Qurilma yo'qoldi — jimgina ovozsiz rejimga o'tamiz
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// WAV yuklash
// ---------------------------------------------------------------------------
SoundRef Audio::loadWav(const std::string& path) {
    if (path.empty()) return nullptr;

    std::vector<unsigned char> f;
    if (!readWholeFile(path, f)) return nullptr;
    if (f.size() < 12) return nullptr;

    const unsigned char* d = f.data();
    const size_t total = f.size();
    if (std::memcmp(d, "RIFF", 4) != 0 || std::memcmp(d + 8, "WAVE", 4) != 0) return nullptr;

    unsigned short fmtTag = 0, chans = 0, bits = 0;
    unsigned int   srate = 0;
    const unsigned char* data = nullptr;
    size_t dataLen = 0;
    bool haveFmt = false;

    size_t off = 12;
    int guard = 0;
    while (off + 8 <= total && guard++ < 4096) {
        const unsigned char* id = d + off;
        unsigned int csz = rd32(d + off + 4);
        const size_t body = off + 8;
        if (body > total) break;
        const size_t avail = total - body;
        if ((size_t)csz > avail) csz = (unsigned int)avail;   // buzuq/kesilgan fayl

        if (std::memcmp(id, "fmt ", 4) == 0 && csz >= 16) {
            fmtTag = rd16(d + body + 0);
            chans  = rd16(d + body + 2);
            srate  = rd32(d + body + 4);
            bits   = rd16(d + body + 14);
            // WAVE_FORMAT_EXTENSIBLE -> haqiqiy format SubFormat GUID boshida
            if (fmtTag == 0xFFFE && csz >= 40) fmtTag = rd16(d + body + 24);
            haveFmt = true;
        } else if (std::memcmp(id, "data", 4) == 0) {
            data    = d + body;
            dataLen = (size_t)csz;
        }

        size_t next = body + (size_t)csz + ((size_t)csz & 1u);
        if (next <= off) break;          // oldinga siljimasak — cheksiz halqa
        off = next;
    }

    if (!haveFmt || !data || dataLen == 0) return nullptr;
    if (chans < 1 || chans > 8) return nullptr;
    if (srate < 1000 || srate > 384000) return nullptr;
    if (fmtTag != 1 && fmtTag != 3) return nullptr;                 // PCM yoki IEEE float
    if (fmtTag == 3 && bits != 32) return nullptr;
    if (bits != 8 && bits != 16 && bits != 24 && bits != 32) return nullptr;

    const size_t bps       = (size_t)bits / 8;
    const size_t srcCh     = (size_t)chans;
    const size_t frameSize = bps * srcCh;
    if (frameSize == 0) return nullptr;

    const size_t frames = dataLen / frameSize;
    if (frames == 0) return nullptr;

    const int outCh = (chans >= 2) ? 2 : 1;

    SoundRef out = std::make_shared<SoundData>();
    out->channels = outCh;
    out->rate     = (int)srate;
    out->samples.resize(frames * (size_t)outCh);

    for (size_t fr = 0; fr < frames; ++fr) {
        const unsigned char* base = data + fr * frameSize;
        for (int c = 0; c < outCh; ++c) {
            const unsigned char* p = base + (size_t)c * bps;
            int v = 0;
            if (bits == 8) {
                v = ((int)p[0] - 128) << 8;                          // unsigned 8-bit
            } else if (bits == 16) {
                v = (int)(short)rd16(p);
            } else if (bits == 24) {
                int raw = (int)((unsigned int)p[0] << 8 | (unsigned int)p[1] << 16 | (unsigned int)p[2] << 24);
                v = raw >> 16;
            } else { // 32
                if (fmtTag == 3) {
                    float fv = 0.0f;
                    std::memcpy(&fv, p, 4);
                    if (!(fv == fv)) fv = 0.0f;                      // NaN
                    fv = clampf(fv, -1.0f, 1.0f);
                    v = (int)(fv * 32767.0f);
                } else {
                    int raw = (int)rd32(p);
                    v = raw >> 16;
                }
            }
            if (v > 32767) v = 32767;
            else if (v < -32768) v = -32768;
            out->samples[fr * (size_t)outCh + (size_t)c] = (short)v;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Protsedural ovozlar
// ---------------------------------------------------------------------------
SoundRef Audio::makeTone(float freqHz, float seconds, float amplitude, int rate) {
    rate    = clampi(rate, 8000, 192000);
    seconds = clampf(seconds, 0.005f, 60.0f);
    freqHz  = clampf(freqHz, 1.0f, (float)rate * 0.45f);
    amplitude = clampf(amplitude, 0.0f, 1.0f);

    const size_t n = (size_t)((double)seconds * (double)rate);
    if (n == 0) return nullptr;

    SoundRef s = std::make_shared<SoundData>();
    s->channels = 1;
    s->rate     = rate;
    s->samples.resize(n);

    // Yumshoq ADSR o'ram — boshi va oxirida chertish (click) bo'lmasin
    const float atk  = std::min(0.008f, seconds * 0.20f);
    const float dec  = std::min(0.030f, seconds * 0.20f);
    const float rel  = std::min(0.060f, seconds * 0.30f);
    const float sus  = 0.80f;
    const double w   = 6.283185307179586 * (double)freqHz / (double)rate;

    for (size_t i = 0; i < n; ++i) {
        const float t = (float)i / (float)rate;
        float env;
        if (atk > 0.0f && t < atk) {
            env = t / atk;
        } else if (dec > 0.0f && t < atk + dec) {
            env = 1.0f - (1.0f - sus) * ((t - atk) / dec);
        } else if (rel > 0.0f && t > seconds - rel) {
            float k = (seconds - t) / rel;
            env = sus * clampf(k, 0.0f, 1.0f);
        } else {
            env = sus;
        }
        const double v = std::sin(w * (double)i) * (double)(amplitude * env);
        int iv = (int)(v * 32767.0);
        if (iv > 32767) iv = 32767;
        else if (iv < -32768) iv = -32768;
        s->samples[i] = (short)iv;
    }
    return s;
}

SoundRef Audio::makeNoise(float seconds, float amplitude, int rate) {
    rate      = clampi(rate, 8000, 192000);
    seconds   = clampf(seconds, 0.005f, 60.0f);
    amplitude = clampf(amplitude, 0.0f, 1.0f);

    const size_t n = (size_t)((double)seconds * (double)rate);
    if (n == 0) return nullptr;

    SoundRef s = std::make_shared<SoundData>();
    s->channels = 1;
    s->rate     = rate;
    s->samples.resize(n);

    unsigned int seed = 0x1234567u;                 // determinlashtirilgan LCG
    const float atk = std::min(0.004f, seconds * 0.10f);

    for (size_t i = 0; i < n; ++i) {
        seed = seed * 1664525u + 1013904223u;
        const float r = (float)((int)(seed >> 8) - 8388608) / 8388608.0f;   // -1..1

        const float t   = (float)i / (float)rate;
        float env = 1.0f - (t / seconds);            // chiziqli pasayish
        if (env < 0.0f) env = 0.0f;
        env *= env;                                  // tabiiyroq so'nish
        if (atk > 0.0f && t < atk) env *= (t / atk); // boshida chertish bo'lmasin

        int iv = (int)(r * amplitude * env * 32767.0f);
        if (iv > 32767) iv = 32767;
        else if (iv < -32768) iv = -32768;
        s->samples[i] = (short)iv;
    }
    return s;
}

// ---------------------------------------------------------------------------
// Ijro boshqaruvi
// ---------------------------------------------------------------------------
int Audio::play(SoundRef s, int bus, float volume, bool loop, float pitch) {
    if (!s) return 0;
    AudioState& st = S();
    Lock lk(&st.cs);
    return addVoice(st, s, bus, volume, loop, pitch, 0.0f);
}

int Audio::playFile(const std::string& wavPath, int bus, float volume, bool loop) {
    if (wavPath.empty()) return 0;

    AudioState& st = S();
    SoundRef s;
    {
        Lock lk(&st.cs);
        std::map<std::string, SoundRef>::iterator it = st.cache.find(wavPath);
        if (it != st.cache.end()) {
            s = it->second;
            if (!s) return 0;                         // avval buzuq deb belgilangan
        }
    }

    if (!s) {
        s = loadWav(wavPath);                          // qulfsiz — disk I/O uzoq
        Lock lk(&st.cs);
        if (st.cache.size() >= (size_t)kMaxCache) st.cache.clear();
        st.cache[wavPath] = s;
        if (!s) return 0;
        return addVoice(st, s, bus, volume, loop, 1.0f, 0.0f);
    }

    Lock lk(&st.cs);
    return addVoice(st, s, bus, volume, loop, 1.0f, 0.0f);
}

void Audio::stop(int voiceId) {
    if (voiceId <= 0) return;
    AudioState& st = S();
    Lock lk(&st.cs);
    for (size_t i = 0; i < st.voices.size(); ++i) {
        if (st.voices[i].id == voiceId) { killVoice(st.voices[i]); return; }
    }
}

void Audio::stopBus(int bus) {
    AudioState& st = S();
    Lock lk(&st.cs);
    const bool all = (bus <= BUS_MASTER || bus >= BUS_COUNT);
    for (size_t i = 0; i < st.voices.size(); ++i) {
        if (all || st.voices[i].bus == bus) killVoice(st.voices[i]);
    }
}

void Audio::stopAll() {
    AudioState& st = S();
    Lock lk(&st.cs);
    for (size_t i = 0; i < st.voices.size(); ++i) killVoice(st.voices[i]);
}

bool Audio::isPlaying(int voiceId) const {
    if (voiceId <= 0) return false;
    AudioState& st = S();
    Lock lk(&st.cs);
    for (size_t i = 0; i < st.voices.size(); ++i) {
        if (st.voices[i].id == voiceId) return !st.voices[i].dead && !st.voices[i].killing;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Bus hajmi
// ---------------------------------------------------------------------------
void Audio::setBusVolume(int bus, float v) {
    if (bus < 0 || bus >= BUS_COUNT) return;
    AudioState& st = S();
    Lock lk(&st.cs);
    st.busVol[bus] = clampf(v, 0.0f, 2.0f);
}

float Audio::busVolume(int bus) const {
    if (bus < 0 || bus >= BUS_COUNT) return 0.0f;
    AudioState& st = S();
    Lock lk(&st.cs);
    return st.busVol[bus];
}

// ---------------------------------------------------------------------------
// Musiqa kross-fade
// ---------------------------------------------------------------------------
void Audio::crossfadeMusic(SoundRef s, float seconds, float volume) {
    AudioState& st = S();
    Lock lk(&st.cs);

    const float sec = clampf(seconds, 0.01f, 30.0f);

    // Eski musiqani so'ndiramiz
    for (size_t i = 0; i < st.voices.size(); ++i) {
        Voice& v = st.voices[i];
        if (v.dead || v.bus != BUS_MUSIC || v.killAtZero) continue;
        v.volTarget  = 0.0f;
        v.volRate    = (v.vol > 0.0f ? v.vol : 0.0001f) / sec;
        v.killAtZero = true;
        // killing = false: hali eshitiladi, shuning uchun isPlaying() true qoladi
    }

    // Yangisini ko'taramiz (nullptr bo'lsa faqat so'ndirish bo'ladi)
    if (s) addVoice(st, s, BUS_MUSIC, volume, true, 1.0f, sec);
}

} // namespace ert
