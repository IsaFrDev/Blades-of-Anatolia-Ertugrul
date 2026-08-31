#pragma once
#include <string>
#include <memory>
#include "ertugrul/core/Math.h"

namespace ert {

enum class AppState {
    Boot, Language, MainMenu, EpisodeSelect, Loading,
    Cutscene,          // epizod ochilish sahnasi
    Gameplay,          // jang / erkin yurish
    Paused,
    Failed,            // o'yinchi halok bo'ldi — qayta urinish ekrani
    EpisodeComplete,   // epizod bajarildi — cliffhanger ekrani
    Quitting
};

struct AppConfig {
    int   width = 1280, height = 760;
    bool  fullscreen = false;
    bool  vsync = true;
    std::string language = "";       // bo'sh = til tanlash ekrani ko'rsatiladi
    std::string startEpisode = "";   // bo'sh = menyu
    std::string startLevel   = "";   // bo'sh = epizoddan aniqlanadi
    bool  skipMenu = false;
    bool  showFps  = true;
    float masterVolume = 0.9f, musicVolume = 0.5f, sfxVolume = 0.8f, voiceVolume = 1.0f;
    int   modelQuality = 1;          // 0=past 1=o'rta 2=yuqori
    bool  subtitles = true;
    bool  showHud   = true;
    bool  showHints = true;
};

class App {
public:
    static App& get();

    bool init(void* hwnd, void* hdc, const AppConfig& cfg);
    void shutdown();

    void resize(int w, int h);
    void update(float dt);
    void render();

    AppState state() const;
    void setState(AppState s);

    bool wantsQuit() const;
    void requestQuit();

    void startEpisode(const std::string& episodeId);
    // Menyu/cutscene'siz to'g'ridan-to'g'ri darajaga tushish (sinov uchun)
    void enterLevel(const std::string& levelId);
    // Skrinshot sayohati uchun: kamerani burish (joyida burilishni sinash)
    void nudgeCamYaw(float deltaDeg);
    // Film uchun: kamera qiyaligi va masofasi (manfiy = o'zgarmasin)
    void filmCamera(float pitchDeg, float dist);
    // Joriy darajada spawn nuqtasiga qaytish
    void respawnHere();
    // Film rejimi: sog'liqni ushlab turish (faqat yozib olishda)
    void filmSustain(float minPct);
    void returnToMenu();

    const AppConfig& config() const;
    AppConfig& mutableConfig();
    void applyAudioConfig();
    void applyQualityConfig();       // model sifati -> skinning chastotasi
    void applyDisplayConfig();       // to'liq ekran / vsync
    void applyLanguageConfig();      // til -> Loc + ovoz banki + menyu
    // O'yinchi qayerda to'xtaganini saqlash (Davom ettirish uchun)
    const std::string& lastEpisode() const;
    // Kadr buferini PNG ga saqlaydi (GDI+ orqali). Avtomatlashtirish va nosozlik izlash uchun.
    bool captureScreenshot(const std::string& pngPath) const;
    // Video uchun: kadrni JPEG qilib saqlaydi (PNG dan ~12x kichik).
    // quality 1..100. Uzun yozuvda disk hajmi hal qiluvchi ahamiyatga ega.
    bool captureFrameJpeg(const std::string& jpgPath, int quality = 88) const;
    bool saveConfig(const std::string& path) const;
    bool loadConfig(const std::string& path);

    int  width()  const;
    int  height() const;
    float fps()   const;

private:
    App() = default;
};

} // namespace ert
