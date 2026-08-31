#pragma once
// Menyu tizimi — «Temir va Firuza» dizayni.
// Splash -> til tanlash -> bosh menyu -> epizod tanlash -> sozlamalar (yon panelli).
#include <string>
#include <vector>
#include <functional>

namespace ert {

class Font;

enum class MenuScreen {
    Splash, Language, Main, EpisodeSelect, Settings, Credits, Pause, None
};

// Sozlamalar bo'limlari (dizayndagi yon panel)
enum class SettingsTab {
    Game = 0, Graphics, Audio, Controls, Interface, Language, Count
};

enum class ItemKind {
    Button,     // oddiy amal
    Slider,     // 0..1 qiymat
    Choice,     // < qiymat >
    Binding,    // klavish bog'lami (bosilganda yangi klavish kutiladi)
    Header      // bo'lim sarlavhasi (tanlanmaydi)
};

struct MenuItem {
    std::string locKey;
    std::string valueText;
    ItemKind    kind = ItemKind::Button;
    bool        enabled = true;
    float       sliderValue = 0.0f;
    int         bindingAction = -1;          // Action indeksi (Binding uchun)
    std::function<void()>    onActivate;
    std::function<void(int)> onAdjust;       // -1 / +1
};

class MenuSystem {
public:
    static MenuSystem& get();

    void init(Font* display, Font* item, Font* small, Font* mono);
    void setScreen(MenuScreen s);
    MenuScreen screen() const;
    bool active() const { return screen() != MenuScreen::None; }

    void setSettingsTab(SettingsTab t);
    SettingsTab settingsTab() const;

    void rebuild();                              // til/qiymat o'zgarganda
    void update(float dt, int screenW, int screenH);
    void draw(int screenW, int screenH);

    // Tashqi ulanishlar
    std::function<void(const std::string& episodeId)> onStartEpisode;
    std::function<void()> onResume;
    std::function<void()> onQuit;
    std::function<void()> onReturnToMenu;

    void setSelectedEpisode(const std::string& id);
    const std::string& selectedEpisode() const;
    void setContextTitle(const std::string& t);

    // Eng so'nggi o'ynalgan epizod (Davom ettirish uchun); bo'sh = yo'q
    void setContinueEpisode(const std::string& id);
    const std::string& continueEpisode() const;

    // Klavish kutish rejimi (sozlamalarda)
    bool capturingKey() const;

private:
    MenuSystem() = default;
};

} // namespace ert
