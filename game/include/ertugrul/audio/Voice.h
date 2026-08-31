#pragma once
// Personaj ovozi: oldindan tayyorlangan WAV (assets/audio/vo/<til>/<id>.wav),
// topilmasa Windows SAPI (TTS) orqali jonli o'qish.
#include <string>

namespace ert {

class VoiceBank {
public:
    static VoiceBank& get();

    bool init();                      // SAPI/COM ni tayyorlaydi (muvaffaqiyatsiz bo'lsa ham o'yin ishlaydi)
    void shutdown();

    void setLanguage(const std::string& langCode);   // "uz" | "tr" | "en"
    const std::string& language() const;
    void setEnabled(bool on);
    bool enabled() const;

    // Personaj ovozi profili (balandlik/tezlik) — id: "ertugrul", "turgut", ...
    void setSpeakerProfile(const std::string& charId, float pitch, float rate);

    // lineId: "e1_intro_01" kabi. utf8Text: subtitr matni (TTS zaxirasi uchun).
    // Qaytaradi: taxminiy davomiylik (soniya). 0 = ovoz chiqmadi.
    float speak(const std::string& lineId, const std::string& utf8Text, const std::string& charId = "");
    void  stopAll();
    bool  isSpeaking() const;

    // Ushbu satr uchun tayyor WAV bormi?
    bool  hasClip(const std::string& lineId) const;
    // Matn uzunligiga qarab taxminiy davomiylik (WAV yo'q bo'lganda subtitr vaqti uchun)
    static float estimateDuration(const std::string& utf8Text);

private:
    VoiceBank() = default;
};

} // namespace ert
