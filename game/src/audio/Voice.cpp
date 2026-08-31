// Voice.cpp — personaj ovozi (VO).
//
// Ikki bosqichli strategiya:
//   1) assets/audio/vo/<til>/<lineId>.wav mavjud bo'lsa — uni Audio (BUS_VOICE)
//      orqali o'ynatamiz va WAV ning haqiqiy davomiyligini qaytaramiz.
//   2) Aks holda Windows SAPI (TTS) bilan jonli o'qiymiz.
//   3) Ikkalasi ham bo'lmasa 0 qaytaramiz — subtitr baribir ekranda chiqadi.
//
// MUHIM texnik nuqtalar:
//   * COM (CoInitializeEx + ISpVoice) ALOHIDA ishchi thread ichida yashaydi.
//     Apartment (STA) bo'lgani uchun ISpVoice metodlarini FAQAT o'sha thread
//     chaqirishi mumkin — shuning uchun barcha buyruqlar navbat (queue) orqali
//     uzatiladi, holat esa atomik bayroqlar orqali qaytariladi.
//   * MinGW da sapi.lib yo'q, shuning uchun CLSID/IID lar qo'lda e'lon qilingan
//     (sapi.h dagi DEFINE_GUID lar faqat `extern` e'lon beradi, ta'rif bermaydi).
//   * SAPI topilmasa hech narsa qulamaydi — jimgina o'chib qoladi.

#include <windows.h>
#include <objbase.h>
#include <sapi.h>

#include "ertugrul/audio/Voice.h"
#include "ertugrul/audio/Audio.h"

#include <atomic>
#include <cmath>
#include <cstring>
#include <deque>
#include <map>
#include <string>

namespace {

// ---------------------------------------------------------------------------
// SAPI GUID lari — sapi.lib siz ishlash uchun qo'lda ta'riflangan
// ---------------------------------------------------------------------------
static const CLSID CLSID_SpVoice_ =
    {0x96749377, 0x3391, 0x11D2, {0x9E, 0xE3, 0x00, 0xC0, 0x4F, 0x79, 0x73, 0x96}};
static const IID   IID_ISpVoice_ =
    {0x6C44DF74, 0x72B9, 0x4992, {0xA1, 0xEC, 0xEF, 0x99, 0x6E, 0x04, 0x22, 0xD4}};

// ---------------------------------------------------------------------------
// Yordamchilar
// ---------------------------------------------------------------------------
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

// UTF-8 -> UTF-16
std::wstring widen(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

bool fileExistsW(const std::wstring& wp) {
    if (wp.empty()) return false;
    DWORD a = GetFileAttributesW(wp.c_str());
    return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

// lineId ni fayl nomi sifatida xavfsiz qilish (yo'l bo'ylab chiqib ketmasin)
bool safeId(const std::string& id) {
    if (id.empty() || id.size() > 190) return false;
    if (id.find("..") != std::string::npos) return false;
    for (size_t i = 0; i < id.size(); ++i) {
        const char c = id[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok) return false;
    }
    return true;
}

// WAV davomiyligini FAQAT sarlavhadan o'qiydi (namunalarni yuklamaydi).
float wavDurationSec(const std::wstring& wp) {
    HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return 0.0f;

    float dur = 0.0f;
    unsigned char head[12];
    DWORD got = 0;

    if (ReadFile(h, head, 12, &got, nullptr) && got == 12 &&
        std::memcmp(head, "RIFF", 4) == 0 && std::memcmp(head + 8, "WAVE", 4) == 0) {

        unsigned int   srate = 0, blockAlign = 0;
        unsigned short chans = 0, bits = 0;

        for (int guard = 0; guard < 128; ++guard) {
            unsigned char ck[8];
            if (!ReadFile(h, ck, 8, &got, nullptr) || got != 8) break;
            unsigned int csz = (unsigned)ck[4] | ((unsigned)ck[5] << 8) |
                               ((unsigned)ck[6] << 16) | ((unsigned)ck[7] << 24);
            if (csz > 0x7FFFFFFEu) break;                 // ishonchsiz o'lcham

            if (std::memcmp(ck, "fmt ", 4) == 0 && csz >= 16) {
                unsigned char f[40];
                DWORD need = (csz > 40u) ? 40u : (DWORD)csz;
                if (!ReadFile(h, f, need, &got, nullptr) || got != need) break;
                chans      = (unsigned short)((unsigned)f[2] | ((unsigned)f[3] << 8));
                srate      = (unsigned)f[4] | ((unsigned)f[5] << 8) |
                             ((unsigned)f[6] << 16) | ((unsigned)f[7] << 24);
                blockAlign = (unsigned)f[12] | ((unsigned)f[13] << 8);
                bits       = (unsigned short)((unsigned)f[14] | ((unsigned)f[15] << 8));
                LONG rest = (LONG)(csz - need) + (LONG)(csz & 1u);
                if (rest > 0) SetFilePointer(h, rest, nullptr, FILE_CURRENT);
            } else if (std::memcmp(ck, "data", 4) == 0) {
                if (blockAlign == 0 && chans > 0 && bits > 0) blockAlign = chans * (bits / 8u);
                if (srate > 0 && blockAlign > 0)
                    dur = (float)((double)csz / ((double)srate * (double)blockAlign));
                break;
            } else {
                LONG skip = (LONG)csz + (LONG)(csz & 1u);
                if (SetFilePointer(h, skip, nullptr, FILE_CURRENT) == INVALID_SET_FILE_POINTER &&
                    GetLastError() != NO_ERROR) break;
            }
        }
    }
    CloseHandle(h);
    if (!(dur > 0.0f) || dur > 3600.0f) dur = 0.0f;
    return dur;
}

// ---------------------------------------------------------------------------
// Ichki holat
// ---------------------------------------------------------------------------
struct Profile {
    float pitch = 1.0f;      // hozircha saqlanadi, SAPI ga qo'llanilmaydi (XML kerak bo'lardi)
    float rate  = 1.0f;      // 1.0 = normal
};

struct Job {
    std::wstring text;
    long         rate = 0;   // SAPI -10..10
    unsigned short vol = 100;
};

const size_t kMaxQueue = 32;

struct VState {
    CRITICAL_SECTION cs;

    std::string lang = "uz";
    bool        inited = false;
    std::atomic<int> enabled{1};

    std::map<std::string, Profile>  profiles;
    std::map<std::string, float>    clipDur;    // yo'l -> davomiylik (0 = yo'q)

    // Ishchi thread
    HANDLE thread   = nullptr;
    HANDLE wakeEvt  = nullptr;
    HANDLE readyEvt = nullptr;
    std::deque<Job>  queue;

    std::atomic<int> quit{0};
    std::atomic<int> purgeReq{0};
    std::atomic<int> sapiOk{0};
    std::atomic<int> speaking{0};
    std::atomic<int> lastWavVoice{0};

    VState() { InitializeCriticalSection(&cs); }
    ~VState() { DeleteCriticalSection(&cs); }
    VState(const VState&) = delete;
    VState& operator=(const VState&) = delete;
};

VState& VS() {
    static VState s;
    return s;
}

// ---------------------------------------------------------------------------
// Ishchi thread: COM + SAPI shu yerda yashaydi
// ---------------------------------------------------------------------------
DWORD WINAPI voiceThreadProc(LPVOID) {
    VState& S = VS();

    // MUHIM: CoInitializeEx aynan shu thread ichida
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool coOk = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

    ISpVoice* sp = nullptr;
    if (coOk) {
        void* obj = nullptr;
        HRESULT ch = CoCreateInstance(CLSID_SpVoice_, nullptr, CLSCTX_ALL, IID_ISpVoice_, &obj);
        if (SUCCEEDED(ch) && obj) sp = (ISpVoice*)obj;
    }

    S.sapiOk.store(sp ? 1 : 0);
    if (S.readyEvt) SetEvent(S.readyEvt);

    while (S.quit.load() == 0) {
        // 25 ms — holatni yangilash uchun yetarlicha tez, CPU uchun arzon
        if (S.wakeEvt) WaitForSingleObject(S.wakeEvt, 25);
        else Sleep(25);

        if (S.quit.load() != 0) break;

        // 1) To'xtatish so'rovi
        if (S.purgeReq.exchange(0) != 0) {
            {
                Lock lk(&S.cs);
                S.queue.clear();
            }
            if (sp) sp->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
            S.speaking.store(0);
        }

        // 2) Navbatdagi replikalar
        for (;;) {
            Job j;
            {
                Lock lk(&S.cs);
                if (S.queue.empty()) break;
                j = S.queue.front();
                S.queue.pop_front();
            }
            if (!sp || j.text.empty()) continue;
            sp->SetRate(j.rate);
            sp->SetVolume(j.vol);
            if (SUCCEEDED(sp->Speak(j.text.c_str(), SPF_ASYNC | SPF_IS_NOT_XML, nullptr)))
                S.speaking.store(1);
        }

        // 3) Holatni e'lon qilish (isSpeaking() shu bayroqni o'qiydi)
        if (sp) {
            SPVOICESTATUS st;
            std::memset(&st, 0, sizeof(st));
            if (SUCCEEDED(sp->GetStatus(&st, nullptr)))
                S.speaking.store(st.dwRunningState == SPRS_IS_SPEAKING ? 1 : 0);
        }
    }

    if (sp) {
        sp->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
        sp->Release();
        sp = nullptr;
    }
    S.sapiOk.store(0);
    S.speaking.store(0);

    // CoUninitialize ham AYNAN shu thread ichida bo'lishi shart
    if (SUCCEEDED(hr)) CoUninitialize();
    return 0;
}

// Til kodini normallashtirish
std::string normLang(const std::string& code) {
    std::string s;
    for (size_t i = 0; i < code.size() && i < 8; ++i) {
        char c = code[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c == '-' || c == '_') break;
        s.push_back(c);
    }
    if (s != "uz" && s != "tr" && s != "en") return "uz";
    return s;
}

} // anonim namespace

namespace ert {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
VoiceBank& VoiceBank::get() {
    static VoiceBank inst;
    return inst;
}

// ---------------------------------------------------------------------------
// init / shutdown
// ---------------------------------------------------------------------------
bool VoiceBank::init() {
    VState& S = VS();
    {
        Lock lk(&S.cs);
        if (S.inited) return true;
    }

    S.quit.store(0);
    S.purgeReq.store(0);
    S.sapiOk.store(0);
    S.speaking.store(0);
    S.lastWavVoice.store(0);

    S.wakeEvt  = CreateEventW(nullptr, FALSE, FALSE, nullptr);   // auto-reset
    S.readyEvt = CreateEventW(nullptr, TRUE,  FALSE, nullptr);   // manual-reset

    S.thread = CreateThread(nullptr, 0, voiceThreadProc, nullptr, 0, nullptr);
    if (!S.thread) {
        // Thread ochilmadi — VO butunlay o'chadi, lekin o'yin ishlayveradi
        if (S.wakeEvt)  { CloseHandle(S.wakeEvt);  S.wakeEvt  = nullptr; }
        if (S.readyEvt) { CloseHandle(S.readyEvt); S.readyEvt = nullptr; }
        return false;
    }

    // COM/SAPI tayyor bo'lishini kutamiz (cheksiz kutmaymiz)
    if (S.readyEvt) WaitForSingleObject(S.readyEvt, 4000);

    {
        Lock lk(&S.cs);
        S.inited = true;
    }
    // WAV yo'li SAPI bo'lmasa ham ishlaydi, shuning uchun init muvaffaqiyatli
    return true;
}

void VoiceBank::shutdown() {
    VState& S = VS();

    HANDLE th = nullptr, wk = nullptr, rd = nullptr;
    {
        Lock lk(&S.cs);
        if (!S.inited && !S.thread) return;
        S.inited = false;
        S.queue.clear();
        th = S.thread;  S.thread   = nullptr;
        wk = S.wakeEvt; S.wakeEvt  = nullptr;
        rd = S.readyEvt;S.readyEvt = nullptr;
    }

    S.quit.store(1);
    if (wk) SetEvent(wk);

    if (th) {
        // Ishchi halqa 25 ms da bir uyg'onadi — tez chiqadi.
        // TerminateThread ishlatilmaydi (COM ni buzadi).
        WaitForSingleObject(th, 5000);
        CloseHandle(th);
    }
    if (wk) CloseHandle(wk);
    if (rd) CloseHandle(rd);

    S.sapiOk.store(0);
    S.speaking.store(0);
    S.lastWavVoice.store(0);
}

// ---------------------------------------------------------------------------
// Til / yoqish-o'chirish
// ---------------------------------------------------------------------------
void VoiceBank::setLanguage(const std::string& langCode) {
    VState& S = VS();
    Lock lk(&S.cs);
    std::string n = normLang(langCode);
    if (n != S.lang) {
        S.lang = n;
        S.clipDur.clear();          // til o'zgardi -> klip keshi eskirdi
    }
}

const std::string& VoiceBank::language() const {
    VState& S = VS();
    Lock lk(&S.cs);
    return S.lang;                  // static holatda saqlanadi -> havola xavfsiz
}

void VoiceBank::setEnabled(bool on) {
    VState& S = VS();
    if (!on) stopAll();
    S.enabled.store(on ? 1 : 0);
}

bool VoiceBank::enabled() const {
    return VS().enabled.load() != 0;
}

// ---------------------------------------------------------------------------
// Speaker profillari
// ---------------------------------------------------------------------------
void VoiceBank::setSpeakerProfile(const std::string& charId, float pitch, float rate) {
    if (charId.empty()) return;
    VState& S = VS();
    Lock lk(&S.cs);
    if (S.profiles.size() > 512) S.profiles.clear();     // cheksiz o'sishdan himoya
    Profile p;
    p.pitch = clampf(pitch, 0.25f, 4.0f);
    p.rate  = clampf(rate,  0.25f, 4.0f);
    S.profiles[charId] = p;
}

// ---------------------------------------------------------------------------
// Klip yo'li / mavjudligi
// ---------------------------------------------------------------------------
bool VoiceBank::hasClip(const std::string& lineId) const {
    if (!safeId(lineId)) return false;
    VState& S = VS();

    std::string path;
    {
        Lock lk(&S.cs);
        path = "assets/audio/vo/" + S.lang + "/" + lineId + ".wav";
        std::map<std::string, float>::const_iterator it = S.clipDur.find(path);
        if (it != S.clipDur.end()) return it->second > 0.0f;
    }

    const std::wstring wp = widen(path);
    float dur = 0.0f;
    if (fileExistsW(wp)) {
        dur = wavDurationSec(wp);
        if (dur <= 0.0f) dur = 0.0f;
    }
    {
        Lock lk(&S.cs);
        if (S.clipDur.size() > 4096) S.clipDur.clear();
        S.clipDur[path] = dur;
    }
    return dur > 0.0f;
}

// ---------------------------------------------------------------------------
// Davomiylikni taxmin qilish
// ---------------------------------------------------------------------------
float VoiceBank::estimateDuration(const std::string& utf8Text) {
    // Ko'p baytli UTF-8 belgilar bitta belgi deb sanaladi
    size_t chars = 0;
    for (size_t i = 0; i < utf8Text.size(); ++i) {
        const unsigned char c = (unsigned char)utf8Text[i];
        if ((c & 0xC0u) != 0x80u) ++chars;
    }
    double sec = (double)chars / 13.0;
    if (sec < 1.4) sec = 1.4;
    sec += 0.45;
    if (sec > 120.0) sec = 120.0;
    return (float)sec;
}

// ---------------------------------------------------------------------------
// Asosiy: gapirish
// ---------------------------------------------------------------------------
float VoiceBank::speak(const std::string& lineId, const std::string& utf8Text,
                       const std::string& charId) {
    VState& S = VS();
    if (S.enabled.load() == 0) return 0.0f;

    // --- 1) Oldindan tayyorlangan WAV --------------------------------------
    if (safeId(lineId)) {
        std::string path;
        float cached = -1.0f;
        {
            Lock lk(&S.cs);
            path = "assets/audio/vo/" + S.lang + "/" + lineId + ".wav";
            std::map<std::string, float>::const_iterator it = S.clipDur.find(path);
            if (it != S.clipDur.end()) cached = it->second;
        }

        float dur = cached;
        if (cached < 0.0f) {
            const std::wstring wp = widen(path);
            dur = fileExistsW(wp) ? wavDurationSec(wp) : 0.0f;
            if (dur < 0.0f) dur = 0.0f;
            Lock lk(&S.cs);
            if (S.clipDur.size() > 4096) S.clipDur.clear();
            S.clipDur[path] = dur;
        }

        if (dur > 0.0f) {
            const int vid = Audio::get().playFile(path, BUS_VOICE, 1.0f, false);
            if (vid > 0) {
                S.lastWavVoice.store(vid);
                return dur;
            }
            // Fayl buzuq bo'lib chiqdi — keshni tozalab, TTS ga o'tamiz
            Lock lk(&S.cs);
            S.clipDur[path] = 0.0f;
        }
    }

    // --- 2) SAPI (TTS) -----------------------------------------------------
    if (S.sapiOk.load() != 0) {
        std::wstring w = widen(utf8Text);
        if (!w.empty()) {
            Profile p;
            {
                Lock lk(&S.cs);
                if (!charId.empty()) {
                    std::map<std::string, Profile>::const_iterator it = S.profiles.find(charId);
                    if (it != S.profiles.end()) p = it->second;
                }
            }
            // rate ko'paytiruvchisi (1.0 = normal) -> SAPI -10..10
            long r = (long)std::lround((double)(p.rate - 1.0f) * 10.0);
            if (r < -10) r = -10;
            if (r >  10) r =  10;

            Job j;
            j.text = w;
            j.rate = r;
            j.vol  = 100;

            bool queued = false;
            {
                Lock lk(&S.cs);
                if (S.queue.size() < kMaxQueue) { S.queue.push_back(j); queued = true; }
            }
            if (queued) {
                S.speaking.store(1);          // Speak boshlangunicha ham "gapiryapti"
                if (S.wakeEvt) SetEvent(S.wakeEvt);
                return estimateDuration(utf8Text);
            }
        }
    }

    // --- 3) Ovoz yo'q — faqat subtitr --------------------------------------
    return 0.0f;
}

// ---------------------------------------------------------------------------
// To'xtatish / holat
// ---------------------------------------------------------------------------
void VoiceBank::stopAll() {
    VState& S = VS();

    Audio::get().stopBus(BUS_VOICE);
    S.lastWavVoice.store(0);

    {
        Lock lk(&S.cs);
        S.queue.clear();
    }
    // ISpVoice::Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr) ni STA thread bajaradi
    S.purgeReq.store(1);
    S.speaking.store(0);
    if (S.wakeEvt) SetEvent(S.wakeEvt);
}

bool VoiceBank::isSpeaking() const {
    VState& S = VS();

    const int vid = S.lastWavVoice.load();
    if (vid > 0) {
        if (Audio::get().isPlaying(vid)) return true;
        S.lastWavVoice.store(0);
    }
    return S.speaking.load() != 0;
}

} // namespace ert
