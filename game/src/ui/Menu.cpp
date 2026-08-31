// Menyu tizimi — «TEMIR VA FIRUZA» dizayn tili.
//   fon #0E1316 · panel #161D21 · chiziq #2A353A · matn #E4EAEA · so'nik #7C8B8F
//   feruza #48A9B5 (tanlov) · zarhal #C09660 (qiymat) · yara #BC5A44 (ogohlantirish)
// Motiv — kichik kvadrat («mix boshi»): yagona takrorlanuvchi belgi.
// Joylashuv chapga tekislangan, tahririy uslubda.
#include "ertugrul/ui/Menu.h"
#include "ertugrul/gfx/Font.h"
#include "ertugrul/loc/Loc.h"
#include "ertugrul/app/Input.h"
#include "ertugrul/app/Bindings.h"
#include "ertugrul/app/App.h"
#include "ertugrul/core/Math.h"
#include "ertugrul/game/Episodes.h"
#include "ertugrul/game/Encounter.h"
#include "ertugrul/audio/Audio.h"

#include <windows.h>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <algorithm>

namespace ert {

namespace {

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};

// --- «Temir va Firuza» palitrasi ---
const float FON[3]      = {0.055f, 0.075f, 0.086f};   // #0E1316
const float PANEL[3]    = {0.086f, 0.114f, 0.129f};   // #161D21
const float CHIZIQ[3]   = {0.165f, 0.208f, 0.227f};   // #2A353A
const float MATN[3]     = {0.894f, 0.918f, 0.918f};   // #E4EAEA
const float SONIK[3]    = {0.486f, 0.545f, 0.561f};   // #7C8B8F
const float OCHIQ[3]    = {0.369f, 0.424f, 0.439f};   // #5E6C70 (o'chirilgan)
const float FERUZA[3]   = {0.282f, 0.663f, 0.710f};   // #48A9B5
const float ZARHAL[3]   = {0.753f, 0.588f, 0.314f};   // #C09660
const float YARA[3]     = {0.737f, 0.353f, 0.267f};   // #BC5A44
const float SUYAK[3]    = {0.863f, 0.827f, 0.769f};   // #DCD3C4

struct MenuState {
    MenuScreen  screen = MenuScreen::Splash;
    SettingsTab tab    = SettingsTab::Game;
    std::vector<MenuItem> items;
    std::vector<Rect>     itemRects;
    std::vector<Rect>     tabRects;
    int    sel = 0;
    float  t = 0.0f, selAnim = 0.0f, fade = 1.0f;
    Font  *fDisp = nullptr, *fItem = nullptr, *fSmall = nullptr, *fMono = nullptr;
    std::string contextTitle, selectedEpisode = "EP001", continueEpisode;

    // epizod tanlash
    int    seasonTab = 0, epSel = 0;
    float  epScroll = 0.0f, epScrollTarget = 0.0f;
    std::vector<Rect> epRects, seasonRects;
    std::vector<Rect> langRects;

    // klavish kutish
    int    captureAction = -1;
    float  captureBlink = 0.0f;
    int    conflictAction = -1;
    float  conflictTimer = 0.0f;

    float  setScroll = 0.0f, setScrollTarget = 0.0f;   // sozlamalar ro'yxati aylanishi
    float  inputLock = 0.0f;     // ekran ochilgandan keyin tasodifiy bosishni bloklaydi
    int    lastMouseX = -1, lastMouseY = -1;
    bool   mouseActive = false;
    int    sw = 1280, sh = 760;
};

MenuState& M() { static MenuState s; return s; }

void beep(float freq, float dur, float vol) {
    if (!Audio::get().ready()) return;
    Audio::get().play(Audio::makeTone(freq, dur, vol), BUS_SFX, 1.0f, false);
}
void sfxMove()   { beep(440.0f, 0.035f, 0.14f); }
void sfxAccept() { beep(660.0f, 0.07f,  0.20f); }
void sfxBack()   { beep(280.0f, 0.07f,  0.16f); }
void sfxDeny()   { beep(150.0f, 0.14f,  0.22f); }

std::string fmtPct(float v) { char b[32]; std::snprintf(b, sizeof b, "%d%%", (int)(v * 100.0f + 0.5f)); return b; }
std::string onOff(bool v)   { return v ? T("ui.common.on") : T("ui.common.off"); }
std::string qualityName(int q) {
    if (q <= 0) return T("ui.common.low");
    if (q == 1) return T("ui.common.medium");
    return T("ui.common.high");
}

// ---------------------------------------------------------------- primitivlar

// Motiv: «mix boshi» — kichik kvadrat
void nailMark(float x, float y, float s, const float c[3], float a) {
    drawRect(x, y, s, s, c[0], c[1], c[2], a);
}
void nailMarkOutline(float x, float y, float s, const float c[3], float a) {
    drawRectOutline(x, y, s, s, 1.0f, c[0], c[1], c[2], a);
}

void hairline(float x, float y, float w, const float c[3], float a) {
    drawRect(x, y, w, 1.0f, c[0], c[1], c[2], a);
}

// Bo'lim sarlavhasi: ▫ NOMI ────────────────  o'ngda meta
void sectionHeader(Font* mono, const std::string& label, float x, float y, float w,
                   const std::string& meta = std::string()) {
    if (!mono) return;
    nailMark(x, y + 5.0f, 7.0f, FERUZA, 1.0f);
    mono->draw(label, x + 18.0f, y, MATN[0], MATN[1], MATN[2], 0.95f);
    const float lw = mono->measure(label) + 30.0f;
    float rightPad = 0.0f;
    if (!meta.empty()) {
        rightPad = mono->measure(meta) + 16.0f;
        mono->draw(meta, x + w, y, SONIK[0], SONIK[1], SONIK[2], 0.75f, TextAlign::Right);
    }
    const float lineX = x + lw;
    const float lineW = std::max(0.0f, w - lw - rightPad);
    hairline(lineX, y + mono->lineHeight() * 0.5f, lineW, CHIZIQ, 1.0f);
}

// Fon: to'q, tepadan pastga engil gradient + diagonal «rumiy» to'r
void backdropWash(int w, int h, float alpha) {
    const float W = (float)w, H = (float)h;
    drawGradientRect(0, 0, W, H, FON[0], FON[1], FON[2], alpha,
                     FON[0] * 0.55f, FON[1] * 0.55f, FON[2] * 0.55f, alpha * 0.94f);
    // diagonal to'r — juda past kontrast
    const float step = 78.0f;
    for (float d = -H; d < W + H; d += step) {
        drawGradientRect(d, 0, 1.0f, H, MATN[0], MATN[1], MATN[2], 0.014f,
                                          MATN[0], MATN[1], MATN[2], 0.004f);
    }
}

std::string upperAscii(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

} // namespace

// ==================================================================== API

MenuSystem& MenuSystem::get() { static MenuSystem m; return m; }

void MenuSystem::init(Font* display, Font* item, Font* small, Font* mono) {
    MenuState& s = M();
    s.fDisp = display; s.fItem = item; s.fSmall = small; s.fMono = mono;
    rebuild();
}

MenuScreen  MenuSystem::screen() const      { return M().screen; }
SettingsTab MenuSystem::settingsTab() const { return M().tab; }
bool        MenuSystem::capturingKey() const { return M().captureAction >= 0; }

void MenuSystem::setScreen(MenuScreen sc) {
    MenuState& s = M();
    if (s.screen == sc) return;
    s.screen = sc;
    s.t = 0.0f; s.fade = 1.0f; s.sel = 0;
    s.captureAction = -1;
    s.conflictAction = -1;
    s.setScroll = s.setScrollTarget = 0.0f;
    // Eski kursor pozitsiyasi tanlovni o'g'irlab ketmasin
    s.mouseActive = false; s.lastMouseX = -1; s.lastMouseY = -1;
    // Ekran endi ochildi — 0.28 s davomida "tasdiqlash" bosishlarini e'tiborsiz qoldiramiz,
    // aks holda oldingi ekrandagi tasodifiy bosish darhol amalga oshib ketadi
    // (masalan til tanlash ekrani ochilishi bilan noto'g'ri til tanlanib qolardi).
    s.inputLock = 0.28f;
    rebuild();
}

void MenuSystem::setSettingsTab(SettingsTab t) {
    MenuState& s = M();
    if (s.tab == t) return;
    s.tab = t; s.sel = 0; s.captureAction = -1;
    s.setScroll = s.setScrollTarget = 0.0f;
    rebuild();
}

void MenuSystem::setSelectedEpisode(const std::string& id) { M().selectedEpisode = id; }
const std::string& MenuSystem::selectedEpisode() const { return M().selectedEpisode; }
void MenuSystem::setContextTitle(const std::string& t) { M().contextTitle = t; }
void MenuSystem::setContinueEpisode(const std::string& id) { M().continueEpisode = id; }
const std::string& MenuSystem::continueEpisode() const { return M().continueEpisode; }

// ---------------------------------------------------------------- rebuild

void MenuSystem::rebuild() {
    MenuState& s = M();
    s.items.clear();
    AppConfig& cfg = App::get().mutableConfig();

    auto add = [&](const char* key, std::function<void()> act, bool enabled = true) {
        MenuItem it; it.locKey = key; it.onActivate = std::move(act); it.enabled = enabled;
        s.items.push_back(std::move(it));
    };
    auto addHeader = [&](const char* key) {
        MenuItem it; it.locKey = key; it.kind = ItemKind::Header; it.enabled = false;
        s.items.push_back(std::move(it));
    };
    auto addSlider = [&](const char* key, float value, std::function<void(int)> adj) {
        MenuItem it;
        it.locKey = key; it.kind = ItemKind::Slider; it.sliderValue = value;
        it.valueText = fmtPct(value); it.onAdjust = std::move(adj);
        s.items.push_back(std::move(it));
    };
    auto addChoice = [&](const char* key, const std::string& val, std::function<void(int)> adj) {
        MenuItem it;
        it.locKey = key; it.kind = ItemKind::Choice; it.valueText = val; it.onAdjust = adj;
        it.onActivate = [adj]() { adj(+1); };
        s.items.push_back(std::move(it));
    };
    auto addBinding = [&](Action a) {
        MenuItem it;
        Bindings& kb = Bindings::get();
        it.locKey = kb.locKey(a);
        it.kind = ItemKind::Binding;
        it.bindingAction = static_cast<int>(a);
        it.valueText = Bindings::keyName(kb.key(a));
        s.items.push_back(std::move(it));
    };

    switch (s.screen) {

    case MenuScreen::Main: {
        const bool canContinue = !s.continueEpisode.empty();
        add("ui.menu.continue", [this]() {
            if (!M().continueEpisode.empty() && onStartEpisode) onStartEpisode(M().continueEpisode);
        }, canContinue);
        add("ui.menu.new_game", [this]() {
            const auto& eps = EpisodeDb::get().all();
            if (!eps.empty()) M().selectedEpisode = eps[0].id;
            if (onStartEpisode) onStartEpisode(M().selectedEpisode);
        });
        add("ui.menu.episodes", [this]() { setScreen(MenuScreen::EpisodeSelect); });
        add("ui.menu.settings", [this]() { setScreen(MenuScreen::Settings); });
        add("ui.menu.credits",  [this]() { setScreen(MenuScreen::Credits); });
        add("ui.menu.quit",     [this]() { if (onQuit) onQuit(); });
        break;
    }

    case MenuScreen::Pause:
        add("ui.menu.resume",       [this]() { if (onResume) onResume(); });
        add("ui.menu.settings",     [this]() { setScreen(MenuScreen::Settings); });
        add("ui.menu.back_to_menu", [this]() { if (onReturnToMenu) onReturnToMenu(); });
        add("ui.menu.quit",         [this]() { if (onQuit) onQuit(); });
        break;

    case MenuScreen::Settings:
        switch (s.tab) {
        case SettingsTab::Game:
            addChoice("ui.settings.subtitles", onOff(cfg.subtitles), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.subtitles = !c.subtitles; MenuSystem::get().rebuild();
            });
            addChoice("ui.settings.fps", onOff(cfg.showFps), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.showFps = !c.showFps; MenuSystem::get().rebuild();
            });
            break;

        case SettingsTab::Graphics:
            addChoice("ui.settings.quality", qualityName(cfg.modelQuality), [](int d) {
                AppConfig& c = App::get().mutableConfig();
                c.modelQuality = (c.modelQuality + (d >= 0 ? 1 : 2)) % 3;
                App::get().applyQualityConfig();
                MenuSystem::get().rebuild();
            });
            addChoice("ui.settings.fullscreen", onOff(cfg.fullscreen), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.fullscreen = !c.fullscreen;
                App::get().applyDisplayConfig();
                MenuSystem::get().rebuild();
            });
            addChoice("ui.settings.vsync", onOff(cfg.vsync), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.vsync = !c.vsync;
                App::get().applyDisplayConfig();
                MenuSystem::get().rebuild();
            });
            break;

        case SettingsTab::Audio:
            addSlider("ui.settings.master", cfg.masterVolume, [](int d) {
                AppConfig& c = App::get().mutableConfig();
                c.masterVolume = clampf(c.masterVolume + d * 0.05f, 0.0f, 1.0f);
                App::get().applyAudioConfig(); MenuSystem::get().rebuild();
            });
            addSlider("ui.settings.music", cfg.musicVolume, [](int d) {
                AppConfig& c = App::get().mutableConfig();
                c.musicVolume = clampf(c.musicVolume + d * 0.05f, 0.0f, 1.0f);
                App::get().applyAudioConfig(); MenuSystem::get().rebuild();
            });
            addSlider("ui.settings.sfx", cfg.sfxVolume, [](int d) {
                AppConfig& c = App::get().mutableConfig();
                c.sfxVolume = clampf(c.sfxVolume + d * 0.05f, 0.0f, 1.0f);
                App::get().applyAudioConfig(); MenuSystem::get().rebuild();
            });
            addSlider("ui.settings.voice", cfg.voiceVolume, [](int d) {
                AppConfig& c = App::get().mutableConfig();
                c.voiceVolume = clampf(c.voiceVolume + d * 0.05f, 0.0f, 1.0f);
                App::get().applyAudioConfig(); MenuSystem::get().rebuild();
            });
            break;

        case SettingsTab::Controls: {
            // DIQQAT: groupLocKey() aylanma statik buferga ko'rsatkich qaytaradi,
            // shuning uchun taqqoslashdan oldin NUSXA olamiz.
            std::string lastGroup;
            Bindings& kb = Bindings::get();
            for (int i = 0; i < Bindings::kCount; ++i) {
                const Action a = static_cast<Action>(i);
                const std::string g = kb.groupLocKey(a);
                if (g != lastGroup) {
                    addHeader(g.c_str());
                    lastGroup = g;
                }
                addBinding(a);
            }
            add("ui.settings.reset_keys", []() {
                Bindings::get().resetDefaults();
                Bindings::get().save("saves/bindings.json");
                MenuSystem::get().rebuild();
            });
            break;
        }

        case SettingsTab::Interface:
            addChoice("ui.settings.subtitles", onOff(cfg.subtitles), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.subtitles = !c.subtitles; MenuSystem::get().rebuild();
            });
            addChoice("ui.settings.hud", onOff(cfg.showHud), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.showHud = !c.showHud; MenuSystem::get().rebuild();
            });
            addChoice("ui.settings.hints", onOff(cfg.showHints), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.showHints = !c.showHints; MenuSystem::get().rebuild();
            });
            break;

        case SettingsTab::Language: {
            MenuItem it;
            it.locKey = "ui.settings.language";
            it.kind = ItemKind::Choice;
            it.valueText = Loc::languageNativeName(Loc::get().language());
            it.onAdjust = [](int d) {
                const char* const* L = Loc::kLanguages;
                int n = 0; while (L[n]) ++n;
                int cur = 0;
                for (int i = 0; i < n; ++i) if (Loc::get().language() == L[i]) cur = i;
                cur = (cur + d + n) % n;
                Loc::get().setLanguage(L[cur]);
                App::get().mutableConfig().language = L[cur];
                App::get().applyLanguageConfig();
                MenuSystem::get().rebuild();
            };
            auto adj = it.onAdjust;
            it.onActivate = [adj]() { adj(+1); };
            s.items.push_back(std::move(it));
            addChoice("ui.settings.subtitles", onOff(cfg.subtitles), [](int) {
                AppConfig& c = App::get().mutableConfig();
                c.subtitles = !c.subtitles; MenuSystem::get().rebuild();
            });
            break;
        }
        default: break;
        }
        break;

    case MenuScreen::Credits:
        add("ui.common.back", [this]() {
            setScreen(App::get().state() == AppState::Paused ? MenuScreen::Pause : MenuScreen::Main);
        });
        break;

    default: break;
    }

    if (s.sel >= (int)s.items.size()) s.sel = 0;
    // Header'lar tanlanmaydi — birinchi tanlanadigan elementga suramiz
    const int n = (int)s.items.size();
    for (int i = 0; i < n && s.sel < n && !s.items[(size_t)s.sel].enabled; ++i)
        s.sel = (s.sel + 1) % n;
}

// ---------------------------------------------------------------- update

namespace {

// Tanlanadigan keyingi/oldingi element (Header'larni o'tkazib yuboradi)
int stepSelection(const std::vector<MenuItem>& items, int cur, int dir) {
    const int n = (int)items.size();
    if (n == 0) return 0;
    for (int k = 1; k <= n; ++k) {
        const int i = ((cur + dir * k) % n + n) % n;
        if (items[(size_t)i].enabled && items[(size_t)i].kind != ItemKind::Header) return i;
    }
    return cur;
}

} // namespace

void MenuSystem::update(float dt, int screenW, int screenH) {
    MenuState& s = M();
    s.sw = screenW; s.sh = screenH;
    s.t += dt;
    s.fade = std::max(0.0f, s.fade - dt * 3.2f);
    s.captureBlink += dt;
    if (s.inputLock > 0.0f) s.inputLock = std::max(0.0f, s.inputLock - dt);
    const bool locked = (s.inputLock > 0.0f);
    if (s.conflictTimer > 0.0f) s.conflictTimer = std::max(0.0f, s.conflictTimer - dt);
    Input& in = Input::get();

    if (s.lastMouseX < 0) {
        s.lastMouseX = in.mouseX(); s.lastMouseY = in.mouseY();
    } else if (std::abs(in.mouseX() - s.lastMouseX) > 3 || std::abs(in.mouseY() - s.lastMouseY) > 3) {
        s.mouseActive = true;
        s.lastMouseX = in.mouseX(); s.lastMouseY = in.mouseY();
    }
    const float mx = (float)in.mouseX(), my = (float)in.mouseY();

    // --- Klavish kutish rejimi hamma narsadan ustun ---
    if (s.captureAction >= 0) {
        const int vk = in.lastPressedVk();
        if (vk != 0) {
            in.clearLastPressed();
            if (vk == VK_ESCAPE) {
                sfxBack();
                s.captureAction = -1;
            } else if (!Bindings::bindable(vk)) {
                sfxDeny();
            } else {
                Bindings& kb = Bindings::get();
                const Action a = static_cast<Action>(s.captureAction);
                const Action c = kb.conflict(a, vk);
                kb.setKey(a, vk);
                kb.save("saves/bindings.json");
                s.conflictAction = (c != Action::Count) ? static_cast<int>(c) : -1;
                s.conflictTimer  = (c != Action::Count) ? 3.0f : 0.0f;
                s.captureAction = -1;
                sfxAccept();
                const int keep = s.sel;
                rebuild();
                s.sel = std::min(keep, (int)s.items.size() - 1);
            }
        }
        s.selAnim = damp(s.selAnim, (float)s.sel, 18.0f, dt);
        return;
    }

    switch (s.screen) {

    case MenuScreen::Splash:
        if (s.t > 0.9f && !locked && (in.navAccept() || in.navCancel() || in.mousePressed(0) || s.t > 8.0f)) {
            sfxAccept();
            setScreen(App::get().config().language.empty() ? MenuScreen::Language : MenuScreen::Main);
        }
        break;

    case MenuScreen::Language: {
        int n = 0; while (Loc::kLanguages[n]) ++n;
        if (in.navLeft()  || in.navUp())   { s.sel = (s.sel - 1 + n) % n; sfxMove(); }
        if (in.navRight() || in.navDown()) { s.sel = (s.sel + 1) % n;     sfxMove(); }
        for (int i = 0; i < (int)s.langRects.size() && i < n; ++i) {
            if (!s.langRects[(size_t)i].contains(mx, my)) continue;
            if (s.sel != i && s.mouseActive) { s.sel = i; sfxMove(); }
            if (!locked && in.mousePressed(0)) { s.sel = i; goto acceptLang; }
        }
        if (!locked && in.navAccept()) {
        acceptLang:
            Loc::get().setLanguage(Loc::kLanguages[s.sel]);
            App::get().mutableConfig().language = Loc::kLanguages[s.sel];
            App::get().applyLanguageConfig();
            App::get().saveConfig("saves/settings.json");
            sfxAccept();
            setScreen(MenuScreen::Main);
            return;
        }
        break;
    }

    case MenuScreen::EpisodeSelect: {
        const auto& seasons = EpisodeDb::get().seasons();
        const auto& eps     = EpisodeDb::get().all();
        const int nSeason = (int)seasons.size();
        if (nSeason > 0) {
            for (int i = 0; i < (int)s.seasonRects.size() && i < nSeason; ++i)
                if (s.seasonRects[(size_t)i].contains(mx, my) && in.mousePressed(0)) {
                    s.seasonTab = i; s.epSel = 0; s.epScrollTarget = 0; sfxMove();
                }
            if (in.navLeft())  { s.seasonTab = (s.seasonTab - 1 + nSeason) % nSeason; s.epSel = 0; s.epScrollTarget = 0; sfxMove(); }
            if (in.navRight()) { s.seasonTab = (s.seasonTab + 1) % nSeason;           s.epSel = 0; s.epScrollTarget = 0; sfxMove(); }
            if (s.seasonTab >= nSeason) s.seasonTab = nSeason - 1;

            const Season& se = seasons[(size_t)s.seasonTab];
            const int nEp = (int)se.episodes.size();
            if (nEp > 0) {
                if (in.navDown()) { s.epSel = (s.epSel + 1) % nEp; sfxMove(); }
                if (in.navUp())   { s.epSel = (s.epSel - 1 + nEp) % nEp; sfxMove(); }
                if (in.wheel() != 0) s.epScrollTarget -= in.wheel() * 0.8f;

                bool start = false;
                for (int i = 0; i < (int)s.epRects.size() && i < nEp; ++i) {
                    const Rect& r = s.epRects[(size_t)i];
                    if (r.w <= 0.0f || !r.contains(mx, my)) continue;
                    if (s.epSel != i && s.mouseActive) { s.epSel = i; sfxMove(); }
                    if (!locked && in.mousePressed(0)) { s.epSel = i; start = true; }
                }
                if (!locked && in.navAccept()) start = true;
                if (start) {
                    const size_t gi = se.episodes[(size_t)s.epSel];
                    if (gi < eps.size()) {
                        if (!Progress::get().unlocked(eps[gi].id)) {
                            sfxDeny();          // hali ochilmagan
                        } else {
                            s.selectedEpisode = eps[gi].id;
                            sfxAccept();
                            if (onStartEpisode) onStartEpisode(s.selectedEpisode);
                            return;
                        }
                    }
                }
                const float rowH  = 32.0f;
                const float viewH = std::max(rowH * 3.0f, (float)s.sh - 268.0f);
                const float rows  = viewH / rowH;
                if ((float)s.epSel < s.epScrollTarget)            s.epScrollTarget = (float)s.epSel;
                if ((float)s.epSel > s.epScrollTarget + rows - 1) s.epScrollTarget = (float)s.epSel - rows + 1;
                s.epScrollTarget = clampf(s.epScrollTarget, 0.0f, std::max(0.0f, (float)nEp - rows));
            }
        }
        if (in.navCancel()) { sfxBack(); setScreen(MenuScreen::Main); return; }
        s.epScroll = damp(s.epScroll, s.epScrollTarget, 16.0f, dt);
        break;
    }

    case MenuScreen::Settings: {
        // Yon paneldagi bo'limlar
        const int nTabs = (int)SettingsTab::Count;
        for (int i = 0; i < (int)s.tabRects.size() && i < nTabs; ++i)
            if (s.tabRects[(size_t)i].contains(mx, my) && in.mousePressed(0)) {
                setSettingsTab(static_cast<SettingsTab>(i)); sfxMove(); return;
            }
        if (in.pressed(Key::Q)) { setSettingsTab(static_cast<SettingsTab>(((int)s.tab - 1 + nTabs) % nTabs)); sfxMove(); return; }
        if (in.pressed(Key::E)) { setSettingsTab(static_cast<SettingsTab>(((int)s.tab + 1) % nTabs));         sfxMove(); return; }
        if (in.pressed(Key::Tab)) { setSettingsTab(static_cast<SettingsTab>(((int)s.tab + 1) % nTabs));       sfxMove(); return; }
        if (in.wheel() != 0) s.setScrollTarget -= in.wheel() * 64.0f;
        goto listScreen;
    }

    default:
    listScreen: {
        const int n = (int)s.items.size();
        if (n > 0) {
            if (in.navDown()) { s.sel = stepSelection(s.items, s.sel, +1); sfxMove(); }
            if (in.navUp())   { s.sel = stepSelection(s.items, s.sel, -1); sfxMove(); }

            for (int i = 0; i < (int)s.itemRects.size() && i < n; ++i) {
                const Rect& r = s.itemRects[(size_t)i];
                const MenuItem& it = s.items[(size_t)i];
                if (r.w <= 0.0f || !r.contains(mx, my) || !it.enabled || it.kind == ItemKind::Header) continue;
                if (s.sel != i && s.mouseActive) { s.sel = i; sfxMove(); }
                if (!locked && in.mousePressed(0)) {
                    s.sel = i;
                    if (it.kind == ItemKind::Binding) {
                        s.captureAction = it.bindingAction; s.captureBlink = 0.0f;
                        Input::get().clearLastPressed();
                        sfxAccept();
                    } else {
                        auto act = it.onActivate; auto adj = it.onAdjust;
                        sfxAccept();
                        if (act) act(); else if (adj) adj(+1);
                    }
                    return;
                }
            }

            const int cur = std::min(s.sel, n - 1);
            const MenuItem& it = s.items[(size_t)cur];
            if (it.onAdjust) {
                if (in.navLeft())  { auto f = it.onAdjust; sfxMove(); f(-1); return; }
                if (in.navRight()) { auto f = it.onAdjust; sfxMove(); f(+1); return; }
            }
            if (!locked && in.navAccept()) {
                if (it.kind == ItemKind::Binding) {
                    s.captureAction = it.bindingAction; s.captureBlink = 0.0f;
                    Input::get().clearLastPressed();
                    sfxAccept();
                    return;
                }
                auto act = it.onActivate;
                sfxAccept();
                if (act) act();
                return;
            }
            // Binding qatorida Delete -> bog'lamani bo'shatish
            if (it.kind == ItemKind::Binding && in.pressedVk(VK_DELETE)) {
                Bindings::get().clearKey(static_cast<Action>(it.bindingAction));
                Bindings::get().save("saves/bindings.json");
                sfxBack();
                const int keep = s.sel; rebuild(); s.sel = keep;
                return;
            }
        }
        if (in.navCancel()) {
            if (s.screen == MenuScreen::Settings || s.screen == MenuScreen::Credits) {
                App::get().saveConfig("saves/settings.json");
                sfxBack();
                setScreen(App::get().state() == AppState::Paused ? MenuScreen::Pause : MenuScreen::Main);
                return;
            }
            if (s.screen == MenuScreen::Pause) { sfxBack(); if (onResume) onResume(); return; }
        }
        break;
    }
    }

    s.selAnim = damp(s.selAnim, (float)s.sel, 18.0f, dt);
    s.setScroll = damp(s.setScroll, s.setScrollTarget, 16.0f, dt);
}

// ---------------------------------------------------------------- draw

namespace {

// Ro'yxatli ekran (Main / Pause): chapga tekislangan, katta serif
void drawListScreen(MenuState& s, int W, int H, const char* titleKey, const char* kicker) {
    Font* fD = s.fDisp, *fI = s.fItem, *fM = s.fMono ? s.fMono : s.fSmall;
    const float x0 = std::max(64.0f, W * 0.105f);
    float y = H * 0.20f;

    if (fM && kicker && *kicker) {
        fM->draw(kicker, x0, y - 34.0f, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f);
    }
    if (fD) {
        fD->draw(T(titleKey), x0, y, MATN[0], MATN[1], MATN[2], 1.0f);
        y += fD->lineHeight();
    }
    if (fM) {
        fM->draw(T("ui.game_subtitle"), x0 + 3.0f, y + 4.0f, SONIK[0], SONIK[1], SONIK[2], 0.85f);
        y += fM->lineHeight() + 22.0f;
    }
    nailMarkOutline(x0, y, 6.0f, SONIK, 0.7f);
    hairline(x0 + 16.0f, y + 3.0f, 250.0f, CHIZIQ, 1.0f);
    y += 48.0f;

    const float rowH = fI ? fI->lineHeight() + 22.0f : 44.0f;
    s.itemRects.assign(s.items.size(), Rect{});
    for (size_t i = 0; i < s.items.size(); ++i) {
        const MenuItem& it = s.items[i];
        const float ry = y + i * rowH;
        s.itemRects[i] = Rect{ x0 - 26.0f, ry - 4.0f, 460.0f, rowH - 8.0f };
        const bool sel = ((int)i == s.sel);
        const float* col = !it.enabled ? OCHIQ : (sel ? MATN : SONIK);

        if (sel) {
            // feruza chap chiziq + kvadrat
            drawRect(x0 - 26.0f, ry - 2.0f, 2.0f, rowH - 12.0f, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
            nailMark(x0 - 14.0f, ry + (rowH - 12.0f) * 0.5f - 3.0f, 6.0f, FERUZA, 1.0f);
        }
        if (fI) fI->draw(upperAscii(T(it.locKey)), x0, ry, col[0], col[1], col[2], it.enabled ? 1.0f : 0.55f);
    }
}

} // namespace

void MenuSystem::draw(int W, int H) {
    MenuState& s = M();
    s.sw = W; s.sh = H;
    if (!s.fItem) return;

    begin2D(W, H);

    Font* fD = s.fDisp  ? s.fDisp  : s.fItem;
    Font* fS = s.fSmall ? s.fSmall : s.fItem;
    Font* fM = s.fMono  ? s.fMono  : fS;
    const float x0 = std::max(64.0f, W * 0.105f);

    switch (s.screen) {

    // ------------------------------------------------------------ SPLASH
    case MenuScreen::Splash: {
        backdropWash(W, H, 0.90f);
        const float a = smoothstepf(clampf(s.t / 1.1f, 0.0f, 1.0f));
        float y = H * 0.34f;
        fD->draw(T("ui.game_title"), x0, y, MATN[0], MATN[1], MATN[2], a);
        y += fD->lineHeight();
        fM->draw(T("ui.game_subtitle"), x0 + 3.0f, y + 4.0f, SONIK[0], SONIK[1], SONIK[2], a * 0.9f);
        y += fM->lineHeight() + 26.0f;
        nailMarkOutline(x0, y, 6.0f, SONIK, a * 0.7f);
        hairline(x0 + 16.0f, y + 3.0f, 250.0f * a, CHIZIQ, 1.0f);
        if (s.t > 0.9f) {
            const float blink = 0.45f + 0.4f * std::sin(s.t * 2.6f);
            fM->draw(T("ui.splash.press_any"), x0, H * 0.80f, FERUZA[0], FERUZA[1], FERUZA[2], blink * a);
        }
        break;
    }

    // ------------------------------------------------------------ TIL
    case MenuScreen::Language: {
        backdropWash(W, H, 0.90f);
        float y = H * 0.24f;
        fM->draw("TIL / DIL / LANGUAGE", x0, y - 32.0f, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f);
        fD->draw(T("ui.lang.select_title"), x0, y, MATN[0], MATN[1], MATN[2], 1.0f);
        y += fD->lineHeight() + 44.0f;

        int n = 0; while (Loc::kLanguages[n]) ++n;
        const float cardW = std::min(300.0f, (W - x0 * 2.0f - 24.0f * (n - 1)) / n);
        const float cardH = 132.0f;
        s.langRects.assign((size_t)n, Rect{});
        for (int i = 0; i < n; ++i) {
            Rect r{ x0 + i * (cardW + 24.0f), y, cardW, cardH };
            s.langRects[(size_t)i] = r;
            const bool sel = (i == s.sel);
            drawRect(r.x, r.y, r.w, r.h, PANEL[0], PANEL[1], PANEL[2], sel ? 1.0f : 0.75f);
            drawRectOutline(r.x, r.y, r.w, r.h, 1.0f,
                            sel ? FERUZA[0] : CHIZIQ[0], sel ? FERUZA[1] : CHIZIQ[1],
                            sel ? FERUZA[2] : CHIZIQ[2], 1.0f);
            if (sel) {
                drawRect(r.x, r.y, 2.0f, r.h, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
                nailMark(r.x + r.w - 18.0f, r.y + 12.0f, 6.0f, FERUZA, 1.0f);
            }
            const char* code = Loc::kLanguages[i];
            const float* col = sel ? MATN : SONIK;
            s.fItem->draw(Loc::languageNativeName(code), r.x + 22.0f, r.y + 34.0f, col[0], col[1], col[2], 1.0f);
            fM->draw(upperAscii(code), r.x + 22.0f, r.y + cardH - 34.0f, SONIK[0], SONIK[1], SONIK[2], 0.8f);
        }
        fM->draw(T("ui.lang.hint"), x0, H - 56.0f, SONIK[0], SONIK[1], SONIK[2], 0.75f);
        break;
    }

    // ------------------------------------------------------------ EPIZODLAR
    case MenuScreen::EpisodeSelect: {
        backdropWash(W, H, 0.94f);
        const auto& seasons = EpisodeDb::get().seasons();
        const auto& eps     = EpisodeDb::get().all();
        const int nSeason = (int)seasons.size();

        fM->draw("EPIZODLAR", x0, 40.0f, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f);
        fD->draw(T("ui.episodes.title"), x0, 62.0f, MATN[0], MATN[1], MATN[2], 1.0f);

        const float topY = 62.0f + fD->lineHeight() + 26.0f;

        // --- Mavsum yorliqlari (chapga tekislangan, mono) ---
        s.seasonRects.assign((size_t)std::max(0, nSeason), Rect{});
        float tx = x0;
        for (int i = 0; i < nSeason; ++i) {
            char key[32]; std::snprintf(key, sizeof key, "ui.season.s%d", i + 1);
            const std::string nm = upperAscii(Loc::get().trOr(seasons[(size_t)i].locTitle,
                                              Loc::get().trOr(key, seasons[(size_t)i].id)));
            const float w = fM->measure(nm) + 34.0f;
            Rect r{ tx, topY, w, 30.0f };
            s.seasonRects[(size_t)i] = r;
            const bool act = (i == s.seasonTab);
            if (act) {
                drawRect(r.x, r.y, r.w, r.h, PANEL[0], PANEL[1], PANEL[2], 1.0f);
                drawRect(r.x, r.y + r.h - 2.0f, r.w, 2.0f, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
                nailMark(r.x + 12.0f, r.y + 12.0f, 6.0f, FERUZA, 1.0f);
            }
            fM->draw(nm, r.x + (act ? 24.0f : 12.0f), r.y + 6.0f,
                     act ? MATN[0] : SONIK[0], act ? MATN[1] : SONIK[1], act ? MATN[2] : SONIK[2], 1.0f);
            tx += w + 6.0f;
        }
        hairline(x0, topY + 30.0f, W - x0 * 2.0f, CHIZIQ, 1.0f);

        // --- Ro'yxat (chap) va tafsilot (o'ng) ---
        const float listY = topY + 52.0f;
        const float listW = (W - x0 * 2.0f) * 0.40f;
        const float infoX = x0 + listW + 44.0f;
        const float infoW = W - x0 - infoX;
        const float rowH  = 32.0f;
        const float viewH = (float)H - listY - 62.0f;

        if (nSeason > 0 && s.seasonTab < nSeason) {
            const Season& se = seasons[(size_t)s.seasonTab];
            const int nEp = (int)se.episodes.size();
            s.epRects.assign((size_t)std::max(0, nEp), Rect{});

            for (int i = 0; i < nEp; ++i) {
                const float ry = listY + (i - s.epScroll) * rowH;
                if (ry < listY - rowH || ry > listY + viewH) { s.epRects[(size_t)i] = Rect{}; continue; }
                Rect r{ x0 - 12.0f, ry, listW + 12.0f, rowH - 3.0f };
                s.epRects[(size_t)i] = r;
                const bool sel = (i == s.epSel);
                if (sel) {
                    drawRect(r.x, r.y, r.w, r.h, PANEL[0], PANEL[1], PANEL[2], 1.0f);
                    drawRect(r.x, r.y, 2.0f, r.h, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
                }
                const size_t gi = se.episodes[(size_t)i];
                if (gi >= eps.size()) continue;
                const Episode& e = eps[gi];
                const bool done   = Progress::get().completed(e.id);
                const bool locked = !Progress::get().unlocked(e.id);
                char num[8]; std::snprintf(num, sizeof num, "%02d", e.seasonIndex + 1);
                fM->draw(num, x0 + 4.0f, ry + 5.0f,
                         sel ? FERUZA[0] : SONIK[0], sel ? FERUZA[1] : SONIK[1], sel ? FERUZA[2] : SONIK[2],
                         sel ? 1.0f : (locked ? 0.35f : 0.7f));
                // bajarilgan epizod — feruza kvadrat; yopiq — bo'sh kontur
                if (done)        nailMark(x0 + 32.0f, ry + 8.0f, 6.0f, FERUZA, 0.95f);
                else if (locked) nailMarkOutline(x0 + 32.0f, ry + 8.0f, 6.0f, SONIK, 0.4f);
                const float* col = locked ? OCHIQ : (sel ? MATN : SONIK);
                fS->draw(EpisodeDb::get().title(e), x0 + 44.0f, ry + 3.0f, col[0], col[1], col[2],
                         locked ? 0.5f : (sel ? 1.0f : 0.85f));
                char yr[12]; std::snprintf(yr, sizeof yr, "%d", e.anchor.year);
                fM->draw(yr, x0 + listW - 4.0f, ry + 5.0f, SONIK[0], SONIK[1], SONIK[2], 0.6f, TextAlign::Right);
            }

            // --- tafsilot ---
            if (s.epSel >= 0 && s.epSel < nEp) {
                const size_t gi = se.episodes[(size_t)s.epSel];
                if (gi < eps.size()) {
                    const Episode& e = eps[gi];
                    float iy = listY - 4.0f;
                    char meta[128];
                    std::snprintf(meta, sizeof meta, "%s  ·  %s",
                                  e.anchor.gregorian.empty() ? "-" : e.anchor.gregorian.c_str(),
                                  e.region.empty() ? "-" : e.region.c_str());
                    fM->draw(meta, infoX, iy, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f);
                    iy += fM->lineHeight() + 8.0f;

                    fD->draw(EpisodeDb::get().title(e), infoX, iy, MATN[0], MATN[1], MATN[2], 1.0f);
                    iy += fD->lineHeight() + 16.0f;
                    hairline(infoX, iy, infoW, CHIZIQ, 1.0f);
                    iy += 18.0f;

                    // arxetip · daraja · davomiylik
                    const std::string arch = upperAscii(Loc::get().trOr("ui.arch." + e.archetype, e.archetype));
                    fM->draw(arch, infoX, iy, SUYAK[0], SUYAK[1], SUYAK[2], 0.9f);
                    float bx = infoX + fM->measure(arch) + 26.0f;
                    for (int k = 0; k < 5; ++k) {
                        const bool on = (k < e.difficultyTier);
                        if (on) nailMark(bx + k * 12.0f, iy + 5.0f, 7.0f, YARA, 1.0f);
                        else    nailMarkOutline(bx + k * 12.0f, iy + 5.0f, 7.0f, CHIZIQ, 1.0f);
                    }
                    char mins[32]; std::snprintf(mins, sizeof mins, "%d %s", e.estimatedMinutes, T("ui.episodes.minutes").c_str());
                    fM->draw(mins, infoX + infoW, iy, SONIK[0], SONIK[1], SONIK[2], 0.75f, TextAlign::Right);
                    iy += fM->lineHeight() + 22.0f;

                    iy += fS->drawWrapped(EpisodeDb::get().synopsis(e), infoX, iy, infoW,
                                          MATN[0], MATN[1], MATN[2], 0.88f) * fS->lineHeight();
                    iy += 20.0f;
                    hairline(infoX, iy, infoW * 0.45f, CHIZIQ, 1.0f);
                    iy += 18.0f;
                    fS->drawWrapped(EpisodeDb::get().cliffhanger(e), infoX, iy, infoW,
                                    ZARHAL[0], ZARHAL[1], ZARHAL[2], 0.9f);
                }
            }
        }
        fM->draw(T("ui.episodes.hint"), x0, (float)H - 44.0f, SONIK[0], SONIK[1], SONIK[2], 0.7f);
        break;
    }

    // ------------------------------------------------------------ SOZLAMALAR
    case MenuScreen::Settings: {
        backdropWash(W, H, 0.95f);

        // Sarlavha satri
        nailMark(x0, 46.0f, 8.0f, YARA, 1.0f);
        fS->draw(T("ui.game_title"), x0 + 22.0f, 36.0f, MATN[0], MATN[1], MATN[2], 1.0f);
        const float th = fS->measure(T("ui.game_title"));
        hairline(x0 + 40.0f + th, 46.0f, 1.0f, CHIZIQ, 1.0f);
        fM->draw(upperAscii(T("ui.settings.title")), x0 + 52.0f + th, 40.0f, SONIK[0], SONIK[1], SONIK[2], 0.8f);
        hairline(x0, 78.0f, W - x0 * 2.0f, CHIZIQ, 1.0f);

        // --- Yon panel ---
        const float sideW = 250.0f;
        const float sideY = 112.0f;
        fM->draw(T("ui.settings.sections"), x0, sideY - 26.0f, SONIK[0], SONIK[1], SONIK[2], 0.6f);
        static const char* kTabKeys[(int)SettingsTab::Count] = {
            "ui.settings.tab.game", "ui.settings.tab.graphics", "ui.settings.tab.audio",
            "ui.settings.tab.controls", "ui.settings.tab.interface", "ui.settings.tab.language"
        };
        s.tabRects.assign((size_t)SettingsTab::Count, Rect{});
        for (int i = 0; i < (int)SettingsTab::Count; ++i) {
            Rect r{ x0 - 16.0f, sideY + i * 46.0f, sideW, 42.0f };
            s.tabRects[(size_t)i] = r;
            const bool act = (i == (int)s.tab);
            if (act) {
                drawRect(r.x, r.y, r.w, r.h, PANEL[0], PANEL[1], PANEL[2], 1.0f);
                drawRect(r.x, r.y, 2.0f, r.h, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
                nailMark(r.x + 16.0f, r.y + 18.0f, 7.0f, FERUZA, 1.0f);
            }
            const float* col = act ? MATN : SONIK;
            fS->draw(upperAscii(T(kTabKeys[i])), r.x + (act ? 34.0f : 20.0f), r.y + 9.0f,
                     col[0], col[1], col[2], act ? 1.0f : 0.8f);
        }
        drawRect(x0 + sideW - 16.0f, sideY - 8.0f, 1.0f, H - sideY - 60.0f, CHIZIQ[0], CHIZIQ[1], CHIZIQ[2], 1.0f);

        // --- O'ng panel ---
        const float px = x0 + sideW + 32.0f;
        const float pw = W - px - x0;
        float py = sideY - 8.0f;
        sectionHeader(fM, upperAscii(T(kTabKeys[(int)s.tab])), px, py, pw, T("ui.settings.hint"));
        py += fM->lineHeight() + 26.0f;

        const float rowH   = 44.0f;
        const float viewTop = py;
        const float viewBot = (float)H - 84.0f;
        py -= s.setScroll;                       // aylantirish
        float selTop = 0.0f, selBot = 0.0f;
        s.itemRects.assign(s.items.size(), Rect{});
        for (size_t i = 0; i < s.items.size(); ++i) {
            const MenuItem& it = s.items[i];
            const bool sel = ((int)i == s.sel);
            const bool visible = (py >= viewTop && py <= viewBot);
            if (sel) { selTop = py + s.setScroll; selBot = selTop + rowH; }

            if (it.kind == ItemKind::Header) {
                py += 12.0f;
                if (!visible) { py += fM->lineHeight() + 8.0f; continue; }
                fM->draw(upperAscii(T(it.locKey)), px, py, FERUZA[0], FERUZA[1], FERUZA[2], 0.75f);
                py += fM->lineHeight() + 8.0f;
                continue;
            }

            if (!visible) { py += rowH; continue; }
            Rect r{ px - 14.0f, py - 6.0f, pw + 14.0f, rowH - 6.0f };
            s.itemRects[i] = r;
            if (sel) {
                drawRect(r.x, r.y, r.w, r.h, PANEL[0], PANEL[1], PANEL[2], 1.0f);
                drawRect(r.x, r.y, 2.0f, r.h, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
            }
            const float* col = !it.enabled ? OCHIQ : (sel ? MATN : SONIK);
            fS->draw(T(it.locKey), px + 8.0f, py, col[0], col[1], col[2], 1.0f);

            const float vRight = px + pw - 8.0f;
            switch (it.kind) {
            case ItemKind::Slider: {
                const float bw = 210.0f, bx = vRight - bw - 66.0f, by = py + 11.0f;
                drawRect(bx, by, bw, 3.0f, CHIZIQ[0], CHIZIQ[1], CHIZIQ[2], 1.0f);
                drawRect(bx, by, bw * it.sliderValue, 3.0f, FERUZA[0], FERUZA[1], FERUZA[2], sel ? 1.0f : 0.7f);
                nailMark(bx + bw * it.sliderValue - 4.0f, by - 4.0f, 8.0f, FERUZA, sel ? 1.0f : 0.8f);
                fM->draw(it.valueText, vRight, py + 2.0f, ZARHAL[0], ZARHAL[1], ZARHAL[2], 0.95f, TextAlign::Right);
                break;
            }
            case ItemKind::Binding: {
                const bool capturing = (s.captureAction == it.bindingAction);
                const float boxW = 208.0f, boxH = 28.0f;
                const float bx = vRight - boxW, by = py - 2.0f;
                if (capturing) {
                    const float pulse = 0.5f + 0.5f * std::sin(s.captureBlink * 7.0f);
                    drawRect(bx, by, boxW, boxH, FERUZA[0] * 0.22f, FERUZA[1] * 0.22f, FERUZA[2] * 0.22f, 1.0f);
                    drawRectOutline(bx, by, boxW, boxH, 1.5f, FERUZA[0], FERUZA[1], FERUZA[2], 0.5f + 0.5f * pulse);
                    fM->draw(T("ui.settings.press_key"), bx + boxW * 0.5f, by + 5.0f,
                             FERUZA[0], FERUZA[1], FERUZA[2], 1.0f, TextAlign::Center);
                } else {
                    const bool conflicted = (s.conflictAction == it.bindingAction && s.conflictTimer > 0.0f);
                    const float* bc = conflicted ? YARA : (sel ? FERUZA : CHIZIQ);
                    drawRect(bx, by, boxW, boxH, FON[0], FON[1], FON[2], 0.85f);
                    drawRectOutline(bx, by, boxW, boxH, 1.0f, bc[0], bc[1], bc[2], sel ? 1.0f : 0.8f);
                    const float* tc = conflicted ? YARA : (it.valueText == "—" ? OCHIQ : ZARHAL);
                    fM->draw(it.valueText, bx + boxW * 0.5f, by + 5.0f, tc[0], tc[1], tc[2], 1.0f, TextAlign::Center);
                }
                break;
            }
            case ItemKind::Choice:
                if (sel) {
                    fM->draw("‹", vRight - fM->measure(it.valueText) - 34.0f, py + 2.0f,
                             FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
                    fM->draw("›", vRight + 4.0f, py + 2.0f, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
                }
                fM->draw(it.valueText, vRight - 8.0f, py + 2.0f, ZARHAL[0], ZARHAL[1], ZARHAL[2], 0.95f, TextAlign::Right);
                break;
            default:
                break;
            }
            py += rowH;
        }

        // Tanlangan qator ko'rinish sohasidan chiqib ketmasin
        if (selBot > 0.0f) {
            const float top = selTop - viewTop;
            const float bot = selBot - viewTop;
            const float viewH = viewBot - viewTop;
            if (top < s.setScrollTarget)             s.setScrollTarget = top - 8.0f;
            else if (bot > s.setScrollTarget + viewH) s.setScrollTarget = bot - viewH + 8.0f;
            const float contentH = py + s.setScroll - viewTop;
            s.setScrollTarget = clampf(s.setScrollTarget, 0.0f, std::max(0.0f, contentH - viewH));
        }

        // Aylantirish ko'rsatkichi
        {
            const float viewH = viewBot - viewTop;
            const float contentH = py + s.setScroll - viewTop;
            if (contentH > viewH + 1.0f) {
                const float trackX = px + pw + 10.0f;
                drawRect(trackX, viewTop, 2.0f, viewH, CHIZIQ[0], CHIZIQ[1], CHIZIQ[2], 1.0f);
                const float th = std::max(28.0f, viewH * (viewH / contentH));
                const float ty = viewTop + (viewH - th) * clampf(s.setScroll / std::max(1.0f, contentH - viewH), 0.0f, 1.0f);
                drawRect(trackX, ty, 2.0f, th, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
            }
        }

        // Ziddiyat ogohlantirishi
        if (s.conflictTimer > 0.0f && s.conflictAction >= 0) {
            const std::string msg = T("ui.settings.conflict") + ": " +
                                    T(Bindings::get().locKey(static_cast<Action>(s.conflictAction)));
            fM->draw(msg, px, (float)H - 74.0f, YARA[0], YARA[1], YARA[2], std::min(1.0f, s.conflictTimer));
        }
        fM->draw(T("ui.settings.nav_hint"), x0, (float)H - 44.0f, SONIK[0], SONIK[1], SONIK[2], 0.7f);
        break;
    }

    // ------------------------------------------------------------ MUALLIFLAR
    case MenuScreen::Credits: {
        backdropWash(W, H, 0.95f);
        float y = H * 0.20f;
        fM->draw("MUALLIFLAR", x0, y - 32.0f, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f);
        fD->draw(T("ui.credits.title"), x0, y, MATN[0], MATN[1], MATN[2], 1.0f);
        y += fD->lineHeight() + 30.0f;
        y += fS->drawWrapped(T("ui.credits.body"), x0, y, std::min(760.0f, W - x0 * 2.0f),
                             SONIK[0], SONIK[1], SONIK[2], 0.95f) * fS->lineHeight();
        y += 40.0f;
        s.itemRects.assign(s.items.size(), Rect{});
        if (!s.items.empty()) {
            Rect r{ x0 - 26.0f, y - 6.0f, 300.0f, 40.0f };
            s.itemRects[0] = r;
            const bool sel = (s.sel == 0);
            if (sel) {
                drawRect(r.x, r.y, 2.0f, r.h, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
                nailMark(x0 - 14.0f, y + 8.0f, 6.0f, FERUZA, 1.0f);
            }
            s.fItem->draw(upperAscii(T("ui.common.back")), x0, y,
                          sel ? MATN[0] : SONIK[0], sel ? MATN[1] : SONIK[1], sel ? MATN[2] : SONIK[2], 1.0f);
        }
        break;
    }

    // ------------------------------------------------------------ BOSH / PAUZA
    default: {
        const bool pause = (s.screen == MenuScreen::Pause);
        backdropWash(W, H, pause ? 0.88f : 0.78f);
        if (pause) {
            drawListScreen(s, W, H, "ui.pause.title", s.contextTitle.empty() ? "" : s.contextTitle.c_str());
        } else {
            drawListScreen(s, W, H, "ui.game_title", "XIII ASR ANADOLU  ·  1227" "–" "1261");
        }
        break;
    }
    }

    if (s.fade > 0.001f) drawFullscreenFade(W, H, 0, 0, 0, s.fade * 0.9f);
    end2D();
}

} // namespace ert
