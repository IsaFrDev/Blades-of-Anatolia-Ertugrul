// Win32 oynasi + OpenGL konteksti + asosiy sikl.
// Buyruq qatori: --lang uz|tr|en  --episode EP001  --width N --height N
//                --fullscreen  --skip-menu  --no-console  --check
#include <windows.h>
#include <GL/gl.h>

#include "ertugrul/app/App.h"
#include "ertugrul/app/Input.h"
#include "ertugrul/core/Math.h"
#include "ertugrul/loc/Loc.h"
#include "ertugrul/game/Episodes.h"
#include "ertugrul/game/Cutscene.h"
#include "ertugrul/gfx/Mesh.h"
#include "ertugrul/ui/Menu.h"
#include "ertugrul/app/Bindings.h"
#include "ertugrul/audio/Voice.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

bool  g_running   = true;
bool  g_mouseLook = false;
bool  g_hasFocus  = true;
HWND  g_hwnd      = nullptr;

// Sichqonchani oyna markaziga qaytarib, kamera uchun delta olamiz
void centerCursor(HWND hwnd, POINT& outCenter) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    POINT c{ (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
    outCenter = c;
    ClientToScreen(hwnd, &c);
    SetCursorPos(c.x, c.y);
}

// Barcha tugmalarni bo'shatilgan deb belgilaymiz (fokus yo'qolganda).
void releaseAllKeys() {
    ert::Input& in = ert::Input::get();
    for (int vk = 0; vk < 256; ++vk) in.onKey(vk, false);
    for (int b = 0; b < 3; ++b)      in.onMouseButton(b, false);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    ert::Input& in = ert::Input::get();
    switch (msg) {
    case WM_CLOSE:
    case WM_DESTROY:
        g_running = false;
        std::printf("[chiqish] oyna yopildi (WM_CLOSE/WM_DESTROY)
");
        return 0;

    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) {
            int w = LOWORD(lp), h = HIWORD(lp);
            if (w > 0 && h > 0) ert::App::get().resize(w, h);
        }
        return 0;

    case WM_ACTIVATE:
        g_hasFocus = (LOWORD(wp) != WA_INACTIVE);
        if (!g_hasFocus) {
            if (g_mouseLook) { ShowCursor(TRUE); ClipCursor(nullptr); }
            releaseAllKeys();
        }
        return 0;

    case WM_SETFOCUS:  g_hasFocus = true;  return 0;
    case WM_KILLFOCUS:
        // Fokus yo'qolganda WM_KEYUP kelmaydi -> tugma "bosilgan" bo'lib qoladi
        // (masalan C ushlanib qolsa personaj abadiy cho'kkalaydi). Hammasini bo'shatamiz.
        g_hasFocus = false;
        releaseAllKeys();
        return 0;

    // Alt (VK_MENU) Windows'da WM_KEYDOWN emas, WM_SYSKEYDOWN yuboradi va
    // DefWindowProc uni oyna menyusi deb qabul qiladi (o'yin muzlaydi + biqillash).
    // Shu tuzatishsiz Action::HideHand ham, Action::Walk ham hech qachon ishlamaydi.
    case WM_SYSKEYDOWN:
        if (wp == VK_RETURN) return 0;              // Alt+Enter — yutamiz
        if (wp == VK_F4)     break;                 // Alt+F4 — tizimga qoldiramiz
        if (wp < 256) in.onKey((int)wp, true);
        return 0;

    case WM_SYSKEYUP:
        if (wp == VK_F4) break;
        if (wp < 256) in.onKey((int)wp, false);
        return 0;

    case WM_SYSCOMMAND:
        // Alt bosilganda oyna menyusi ochilib o'yin muzlashining oldini oladi
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;
        break;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE || wp < 256) in.onKey((int)wp, true);
        if (wp == 'Q' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            g_running = false; std::printf("[chiqish] Ctrl+Q
"); }
        return 0;

    case WM_KEYUP:
        if (wp < 256) in.onKey((int)wp, false);
        return 0;

    case WM_LBUTTONDOWN: SetCapture(hwnd); in.onMouseButton(0, true);  return 0;
    case WM_LBUTTONUP:   ReleaseCapture(); in.onMouseButton(0, false); return 0;
    case WM_RBUTTONDOWN: in.onMouseButton(1, true);  return 0;
    case WM_RBUTTONUP:   in.onMouseButton(1, false); return 0;
    case WM_MBUTTONDOWN: in.onMouseButton(2, true);  return 0;
    case WM_MBUTTONUP:   in.onMouseButton(2, false); return 0;

    case WM_MOUSEMOVE: {
        int mx = (short)LOWORD(lp), my = (short)HIWORD(lp);
        static const bool logMouse = (std::getenv("ERT_MOUSE_LOG") != nullptr);
        if (logMouse) {
            POINT sp2; GetCursorPos(&sp2);
            std::printf("[WM_MOUSEMOVE] client=(%d,%d) screen=(%ld,%ld)\n", mx, my, (long)sp2.x, (long)sp2.y);
        }
        in.onMouseMove(mx, my);
        return 0;
    }

    case WM_MOUSEWHEEL:
        in.onMouseWheel(GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA);
        return 0;

    case WM_ERASEBKGND:
        return 1;   // miltillashning oldini olish

    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

struct Args {
    ert::AppConfig cfg;
    bool console = true;
    bool check   = false;
    std::string shotsDir;      // --shots <papka>: skriptlangan sayohat + PNG saqlash
    bool locoTour = false;     // --loco: yurish mexanikasi sayohati
};

Args parseArgs(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        auto next = [&](const char* def) -> std::string {
            return (i + 1 < argc) ? std::string(argv[++i]) : std::string(def);
        };
        if      (s == "--lang"     || s == "-l") a.cfg.language     = next("uz");
        else if (s == "--episode"  || s == "-e") a.cfg.startEpisode = next("EP001");
        else if (s == "--width")                 a.cfg.width        = std::atoi(next("1280").c_str());
        else if (s == "--height")                a.cfg.height       = std::atoi(next("760").c_str());
        else if (s == "--fullscreen")            a.cfg.fullscreen   = true;
        else if (s == "--skip-menu")             a.cfg.skipMenu     = true;
        else if (s == "--quality")               a.cfg.modelQuality = std::atoi(next("1").c_str());
        else if (s == "--no-console")            a.console          = false;
        else if (s == "--check")                 a.check            = true;
        else if (s == "--shots")                 a.shotsDir         = next("shots");
        else if (s == "--loco")                  a.locoTour         = true;
        else if (s == "--level")                 a.cfg.startLevel   = next("sogut_village");
        else if (s == "--help" || s == "-h") {
            std::printf("Ertugrul 3D\n"
                        "  --lang uz|tr|en     tilni tanlash\n"
                        "  --episode EP001     to'g'ridan-to'g'ri epizoddan boshlash\n"
                        "  --width N --height N\n"
                        "  --fullscreen        to'liq ekran\n"
                        "  --skip-menu         menyuni o'tkazib yuborish\n"
                        "  --quality 0|1|2     model sifati\n"
                        "  --no-console        konsol oynasini yashirish\n"
                        "  --check             kontent diagnostikasi va chiqish\n");
            std::exit(0);
        }
    }
    return a;
}

// Kontent diagnostikasi (GPU siz ham foydali)
int runCheck() {
    std::printf("\n=================== KONTENT DIAGNOSTIKASI ===================\n");
    ert::Loc& loc = ert::Loc::get();
    struct { const char* p; } csvs[] = {
        {"localization/ertugrul_loc.csv"}, {"localization/ui_loc.csv"},
        {"localization/episodes_loc.csv"}, {"localization/cutscene_loc.csv"},
    };
    for (auto& c : csvs) {
        bool ok = loc.loadCsv(c.p);
        std::printf("  CSV %-38s %s\n", c.p, ok ? "OK" : "TOPILMADI");
    }
    std::printf("  Jami kalitlar: %d\n", (int)loc.keyCount());

    bool okEp = ert::EpisodeDb::get().load("data/episodes/episodes_v2.json");
    std::printf("  Epizodlar: %s (%d ta, %d mavsum)\n", okEp ? "OK" : "XATO",
                (int)ert::EpisodeDb::get().count(), (int)ert::EpisodeDb::get().seasons().size());
    if (!okEp) std::printf("    xato: %s\n", ert::EpisodeDb::get().lastError().c_str());

    ert::CutsceneDirector::get().loadDirectory("data/cutscenes");
    {
        int found = 0;
        for (int ep = 1; ep <= 48; ++ep) {
            char b[32]; std::snprintf(b, sizeof b, "ep%03d_intro", ep);
            if (ert::CutsceneDirector::get().find(b)) ++found;
        }
        std::printf("  Cutscene: %d / 48 epizod uchun sahna bor\n", found);
    }

    int missingTitles = 0;
    for (const char* lang : {"uz", "tr", "en"}) {
        loc.setLanguage(lang);
        int miss = 0;
        for (const auto& e : ert::EpisodeDb::get().all()) {
            if (!loc.has(e.locTitle)) ++miss;
        }
        std::printf("  [%s] tarjimasiz epizod sarlavhalari: %d / %d\n",
                    lang, miss, (int)ert::EpisodeDb::get().count());
        missingTitles += miss;
    }

    const char* models[] = {
        "assets/models/ottoman/ottoman.obj",
        "assets/models/crusader/crusader.obj",
    };
    for (const char* m : models) {
        FILE* f = std::fopen(m, "rb");
        std::printf("  Model %-44s %s\n", m, f ? "MAVJUD" : "YO'Q");
        if (f) std::fclose(f);
    }

    // --- Ovoz (VO) qamrovi: har til uchun nechta replika tayyor WAV ga ega ---
    {
        ert::VoiceBank& vb = ert::VoiceBank::get();
        for (const char* lang : {"uz", "tr", "en"}) {
            vb.setLanguage(lang);
            int have = 0, total = 0, subMissing = 0;
            loc.setLanguage(lang);
            std::vector<std::string> sceneIds;
            for (int ep = 1; ep <= 48; ++ep) {
                char b[32]; std::snprintf(b, sizeof b, "ep%03d_intro", ep);
                sceneIds.push_back(b);
            }
            sceneIds.push_back("generic_intro");
            for (const std::string& sid : sceneIds) {
                const ert::CutScene* s = ert::CutsceneDirector::get().find(sid);
                if (!s) continue;
                for (const ert::CutLine& l : s->lines) {
                    ++total;
                    const std::string id = l.voId.empty() ? l.locKey : l.voId;
                    if (vb.hasClip(id)) ++have;
                    if (!loc.has(l.locKey)) ++subMissing;
                }
            }
            std::printf("  [%s] ovoz: %d/%d WAV tayyor, subtitrsiz replika: %d\n",
                        lang, have, total, subMissing);
        }
    }

    loc.dumpMissing("saves/missing_loc_keys.txt");
    std::printf("  Topilmagan kalitlar ro'yxati: saves/missing_loc_keys.txt (%d ta)\n",
                (int)loc.missingKeys().size());
    std::printf("=============================================================\n\n");
    return missingTitles == 0 ? 0 : 1;
}


// --------------------------------------------------------------------------
// --shots: interfeys bo'ylab skriptlangan sayohat.
// Oynani old planga chiqarmasdan, o'yinning O'ZI kadr buferini PNG ga saqlaydi —
// shuning uchun boshqa dastur to'liq ekranda ishlayotgan bo'lsa ham ishlaydi.
// --------------------------------------------------------------------------
struct TourStep {
    float       at;          // sekund (dastur boshlanganidan)
    const char* file;        // PNG nomi
    int         action;      // 0=hech narsa 1=til 2=bosh 3=epizodlar 4=sozlamalar 5=boshqaruv
                             // 6=klavish kutish 7=epizod boshlash 8=pauza
};

const TourStep kTour[] = {
    {  2.0f, "01_splash.png",       0 },
    {  3.5f, "02_language.png",     1 },
    {  5.0f, "03_main.png",         2 },
    {  6.5f, "04_episodes.png",     3 },
    {  8.0f, "05_settings.png",     4 },
    {  9.5f, "06_controls.png",     5 },
    { 11.0f, "07_rebind.png",       6 },
    { 12.0f, "07b_scroll.png",     13 },
    { 13.0f, "08_loading.png",      7 },
    { 22.0f, "09_cutscene.png",     0 },
    { 32.0f, "10_cutscene2.png",    0 },
    { 52.0f, "11_gameplay.png",     0 },
    { 53.0f, "13_before_right.png",  9 },   // O'NGGA yurishni boshlaymiz
    { 55.5f, "14_after_right.png",  10 },   // to'xtatamiz
    { 56.5f, "15_before_left.png",  11 },   // CHAPGA yurishni boshlaymiz
    { 59.0f, "16_after_left.png",   12 },
    { 60.5f, "12_pause.png",         8 },
};

// --level bilan birga --shots berilsa FAQAT parkur qadamlari bajariladi
const TourStep kTourParkour[] = {
    {  2.0f, "20_village.png",       0 },   // qishloqda turibdi
    {  3.0f, "21_freerun.png",      15 },   // yuqori profil + parkur tugmasi
    {  6.0f, "22_parkour.png",       0 },
    {  9.0f, "23_parkour2.png",      0 },
    { 12.0f, "24_parkour3.png",      0 },
    { 14.0f, "25_eagle.png",        16 },   // Bilge Ko'z
    { 16.0f, "26_eagle2.png",        0 },
    { 17.0f, "27_stop.png",         17 },
};

// --loco bilan --shots berilsa LOKOMOTSIYA sayohati (yurish hissini tekshirish)
const TourStep kTourLoco[] = {
    {  1.5f, "60_idle.png",           0 },   // turgan holat - oyoq qo'yilgan
    {  2.0f, "61_walk_ramp.png",     30 },   // W ushlash (Shift yo'q) -> walk pog'onasi
    {  3.6f, "62_jog.png",            0 },   // 0.28 s dan keyin jogga chiqadi
    {  5.4f, "63_sprint.png",        31 },   // Shift -> sprint, og'ir ramp
    {  8.0f, "64_sprint2.png",        0 },
    {  8.3f, "65_runout_a.png",      32 },   // W qo'yib yuborildi -> run-out ~1 m
    {  8.6f, "66_runout_b.png",       0 },
    {  9.4f, "67_stopped.png",        0 },   // to'liq to'xtadi, oyoq qo'yilgan
    { 10.0f, "68_arc.png",           34 },   // W+D -> 90 gradus yoy + bank
    { 11.6f, "69_arc2.png",           0 },
    { 12.4f, "70_pivot_a.png",       33 },   // 180 gradus qaytish -> pivot
    { 12.8f, "71_pivot_b.png",        0 },
    { 13.6f, "72_pivot_c.png",        0 },
    { 14.4f, "73_turn_place.png",    35 },   // joyida burilish - oyoq qadam tashlaydi
    { 15.6f, "74_walk_alt.png",      36 },   // Alt -> past profil yurish
    { 17.4f, "75_crouch.png",        37 },   // cho'kkalab yurish
    { 19.0f, "76_end.png",           17 },
};

// --episode bilan birga --shots berilsa JANG sayohati bajariladi
const TourStep kTourCombat[] = {
    {  1.5f, "40_loading.png",       0 },
    {  3.0f, "41_cutscene.png",      0 },
    {  4.0f, "42_briefing.png",     18 },   // sahnani o'tkazib yuborish
    {  7.5f, "43_fight.png",        19 },   // hujum
    {  9.5f, "44_fight2.png",       19 },
    { 11.5f, "45_fight3.png",       19 },
    { 14.0f, "46_fight4.png",       20 },   // oldinga yurib hujum
    { 18.0f, "47_fight5.png",       19 },
    { 20.0f, "54_bow_draw.png",     38 },   // kamon tortilmoqda — nishon halqasi
    { 21.2f, "55_bow_full.png",      0 },   // to'la tortildi — zarhal markaz
    { 21.6f, "56_bow_shot.png",     39 },   // o'q uchdi
    { 22.0f, "57_arrow.png",         0 },   // o'q havoda
    { 24.0f, "48_state.png",         0 },
    { 40.0f, "49_result.png",        0 },   // o'lim yoki g'alaba
    { 42.0f, "50_retry.png",        21 },   // «nazorat nuqtasidan qayta urinish»
    { 45.0f, "51_respawn.png",       0 },   // qayta tug'ildi — jang davom etadi
    { 50.0f, "52_fight6.png",       19 },
    { 54.0f, "53_end.png",          17 },
};

void tourApply(int action) {
    ert::MenuSystem& menu = ert::MenuSystem::get();
    switch (action) {
    case 1: menu.setScreen(ert::MenuScreen::Language); break;
    case 2: menu.setScreen(ert::MenuScreen::Main);     break;
    case 3: menu.setScreen(ert::MenuScreen::EpisodeSelect); break;
    case 4: menu.setScreen(ert::MenuScreen::Settings);
            menu.setSettingsTab(ert::SettingsTab::Audio); break;
    case 5: menu.setScreen(ert::MenuScreen::Settings);
            menu.setSettingsTab(ert::SettingsTab::Controls); break;
    case 6: { // klavish kutish holatini ko'rsatish uchun qatorni "bosamiz"
            ert::Input& in = ert::Input::get();
            const bool wasFrozen = in.frozen();
            in.setFrozen(false);
            in.onKey(VK_DOWN, true);   in.onKey(VK_DOWN, false);
            in.onKey(VK_RETURN, true); in.onKey(VK_RETURN, false);
            in.setFrozen(wasFrozen);
            break;
        }
    case 7: ert::App::get().startEpisode("EP001"); break;
    case 8: ert::MenuSystem::get().setScreen(ert::MenuScreen::Pause);
            ert::App::get().setState(ert::AppState::Paused); break;
    case 14:     // parkur darajasiga kirish
        ert::App::get().enterLevel("sogut_village");
        break;
    case 15: {   // oldinga yugurish + parkur (free-run)
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            const ert::Bindings& kb = ert::Bindings::get();
            in.onKey(kb.key(ert::Action::MoveForward), true);
            in.onKey(kb.key(ert::Action::Run),         true);
            in.onKey(kb.key(ert::Action::ParkourUp),   true);
            break;   // muzlatishni OCHIQ qoldiramiz — tugmalar bosilgan turishi kerak
        }
    case 38: {   // kamonni tortish (G ushlash)
            ert::Input& in = ert::Input::get();
            const ert::Bindings& kb = ert::Bindings::get();
            in.setFrozen(false);
            for (int vk = 0; vk < 256; ++vk) in.onKey(vk, false);
            in.onKey(kb.key(ert::Action::Bow), true);
            break;
        }
    case 39: {   // o'qni qo'yib yuborish -> OTISH
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            in.onKey(ert::Bindings::get().key(ert::Action::Bow), false);
            break;
        }
    case 30: {   // oldinga YURISH (Shift YO'Q) - walkRamp_ pog'onasini ko'rsatadi
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            in.onKey(ert::Bindings::get().key(ert::Action::MoveForward), true);
            break;
        }
    case 31: {   // Shift ham ushlanadi -> sprint (kAccelSprint = 4.5, og'ir start)
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            in.onKey(ert::Bindings::get().key(ert::Action::Run), true);
            break;
        }
    case 32: {   // W qo'yib yuboriladi (Shift ushlangan qoladi) -> run-out
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            in.onKey(ert::Bindings::get().key(ert::Action::MoveForward), false);
            break;
        }
    case 33: {   // 180 gradus teskari -> PIVOT oynasi
            ert::Input& in = ert::Input::get();
            const ert::Bindings& kb = ert::Bindings::get();
            in.setFrozen(false);
            in.onKey(kb.key(ert::Action::MoveRight),   false);
            in.onKey(kb.key(ert::Action::MoveForward), false);
            in.onKey(kb.key(ert::Action::Run),         true);
            in.onKey(kb.key(ert::Action::MoveBack),    true);
            break;
        }
    case 34: {   // W + D -> 90 gradus yoy (burilish radiusi va bank)
            ert::Input& in = ert::Input::get();
            const ert::Bindings& kb = ert::Bindings::get();
            in.setFrozen(false);
            in.onKey(kb.key(ert::Action::MoveBack),    false);
            in.onKey(kb.key(ert::Action::MoveForward), true);
            in.onKey(kb.key(ert::Action::Run),         true);
            in.onKey(kb.key(ert::Action::MoveRight),   true);
            break;
        }
    case 35: {   // harakatsiz joyida burilish -> turnStepHz_
            ert::Input& in = ert::Input::get();
            for (int vk = 0; vk < 256; ++vk) in.onKey(vk, false);
            in.setFrozen(false);
            ert::App::get().nudgeCamYaw(150.0f);
            break;
        }
    case 36: {   // Alt -> past profil yurish
            ert::Input& in = ert::Input::get();
            const ert::Bindings& kb = ert::Bindings::get();
            for (int vk = 0; vk < 256; ++vk) in.onKey(vk, false);
            in.setFrozen(false);
            in.onKey(kb.key(ert::Action::Walk),        true);
            in.onKey(kb.key(ert::Action::MoveForward), true);
            break;
        }
    case 37: {   // cho'kkalab yurish
            ert::Input& in = ert::Input::get();
            const ert::Bindings& kb = ert::Bindings::get();
            in.setFrozen(false);
            in.onKey(kb.key(ert::Action::Walk), false);
            in.onKey(kb.key(ert::Action::Crouch), true);
            in.onKey(kb.key(ert::Action::Crouch), false);
            break;
        }
    case 16: {   // Bilge Ko'z
            ert::Input& in = ert::Input::get();
            const int vk = ert::Bindings::get().key(ert::Action::BilgeGoz);
            in.onKey(vk, true); in.onKey(vk, false);
            break;
        }
    case 17: {   // hamma tugmani qo'yib yuborish
            ert::Input& in = ert::Input::get();
            for (int vk = 0; vk < 256; ++vk) in.onKey(vk, false);
            in.setFrozen(true);
            break;
        }
    case 21: {   // yakuniy ekranda «Enter» — nazorat nuqtasidan qayta urinish
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            in.onKey(VK_RETURN, true); in.onKey(VK_RETURN, false);
            break;
        }
    case 18:     // ochilish sahnasini o'tkazib yuborish
        ert::CutsceneDirector::get().skip();
        break;
    case 19: {   // bitta zarba
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            const int vk = ert::Bindings::get().key(ert::Action::LightAttack);
            if (vk == VK_LBUTTON) { in.onMouseButton(0, true); in.onMouseButton(0, false); }
            else                  { in.onKey(vk, true); in.onKey(vk, false); }
            break;   // muzlatish ochiq qoladi — keyingi qadamlar ham tugma yubora oladi
        }
    case 20: {   // oldinga yurib hujum
            ert::Input& in = ert::Input::get();
            in.setFrozen(false);
            in.onKey(ert::Bindings::get().key(ert::Action::MoveForward), true);
            const int vk = ert::Bindings::get().key(ert::Action::LightAttack);
            if (vk == VK_LBUTTON) { in.onMouseButton(0, true); in.onMouseButton(0, false); }
            break;
        }
    case 13: {   // boshqaruv ro'yxatini pastga aylantirish (dunyo/tizim bog'lamalari)
            ert::Input& in = ert::Input::get();
            const bool wasFrozen = in.frozen();
            in.setFrozen(false);
            for (int i = 0; i < 14; ++i) {
                in.onKey(VK_DOWN, true); in.onKey(VK_DOWN, false);
                ert::App::get().update(1.0f / 60.0f);
            }
            in.setFrozen(wasFrozen);
            break;
        }
    case 9: case 10: case 11: case 12: {
            ert::Input& in = ert::Input::get();
            const bool wasFrozen = in.frozen();
            in.setFrozen(false);
            const int vkRight = ert::Bindings::get().key(ert::Action::MoveRight);
            const int vkLeft  = ert::Bindings::get().key(ert::Action::MoveLeft);
            if (action == 9)  in.onKey(vkRight, true);
            if (action == 10) in.onKey(vkRight, false);
            if (action == 11) in.onKey(vkLeft,  true);
            if (action == 12) in.onKey(vkLeft,  false);
            // muzlatish qayta yoqilsa down[] tozalanadi -> sinov davomida ochiq qoldiramiz
            if (action == 10 || action == 12) in.setFrozen(wasFrozen);
            break;
        }
    default: break;
    }
}

} // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);   // log darhol yozilsin
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    SetConsoleOutputCP(CP_UTF8);
    SetProcessDPIAware();

    Args args = parseArgs(argc, argv);
    if (!args.console) {
        HWND con = GetConsoleWindow();
        if (con) ShowWindow(con, SW_HIDE);
    }
    CreateDirectoryA("saves", nullptr);

    if (args.check) return runCheck();

    // ---------------- oyna ----------------
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ErtugrulWindow";
    if (!RegisterClassExW(&wc)) { std::printf("XATO: oyna klassi ro'yxatdan o'tmadi\n"); return 1; }

    DWORD style = args.cfg.fullscreen ? (WS_POPUP | WS_VISIBLE) : (WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);

    // Oyna ish stoli sohasidan chiqib ketmasin (masshtablangan ekranlarda muhim)
    RECT work{0, 0, sw, sh};
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0);
    int workW = work.right - work.left, workH = work.bottom - work.top;

    int winW = args.cfg.fullscreen ? sw : args.cfg.width;
    int winH = args.cfg.fullscreen ? sh : args.cfg.height;
    RECT rc{0, 0, winW, winH};
    if (!args.cfg.fullscreen) {
        AdjustWindowRect(&rc, style, FALSE);
        int frameW = (rc.right - rc.left) - winW;
        int frameH = (rc.bottom - rc.top) - winH;
        if (winW + frameW > workW) winW = workW - frameW - 8;
        if (winH + frameH > workH) winH = workH - frameH - 8;
        winW = winW < 640 ? 640 : winW;
        winH = winH < 400 ? 400 : winH;
        args.cfg.width = winW; args.cfg.height = winH;
        rc = RECT{0, 0, winW, winH};
        AdjustWindowRect(&rc, style, FALSE);
    }

    g_hwnd = CreateWindowExW(0, L"ErtugrulWindow", L"Dirilish: Ertug'rul - 3D",
                             style,
                             args.cfg.fullscreen ? 0 : work.left + (workW - (rc.right - rc.left)) / 2,
                             args.cfg.fullscreen ? 0 : work.top  + (workH - (rc.bottom - rc.top)) / 2,
                             rc.right - rc.left, rc.bottom - rc.top,
                             nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) { std::printf("XATO: oyna yaratilmadi\n"); return 1; }

    HDC hdc = GetDC(g_hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf || !SetPixelFormat(hdc, pf, &pfd)) { std::printf("XATO: piksel formati\n"); return 1; }

    HGLRC glrc = wglCreateContext(hdc);
    if (!glrc || !wglMakeCurrent(hdc, glrc)) { std::printf("XATO: OpenGL konteksti\n"); return 1; }

    std::printf("[GL] %s | %s | %s\n",
                (const char*)glGetString(GL_VENDOR),
                (const char*)glGetString(GL_RENDERER),
                (const char*)glGetString(GL_VERSION));

    // VSync (mavjud bo'lsa)
    typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);
    if (PROC p = wglGetProcAddress("wglSwapIntervalEXT")) {
        auto swapInterval = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(reinterpret_cast<void*>(p));
        swapInterval(args.cfg.vsync ? 1 : 0);
    }

    RECT cr; GetClientRect(g_hwnd, &cr);
    args.cfg.width  = cr.right - cr.left;
    args.cfg.height = cr.bottom - cr.top;

    // --shots rejimida haqiqiy klaviatura/sichqoncha butunlay e'tiborsiz qoldiriladi,
    // shunda sayohat determinatsiyalangan bo'ladi (fonda boshqa dastur ishlasa ham).
    if (!args.shotsDir.empty()) ert::Input::get().setFrozen(true);

    if (!ert::App::get().init(g_hwnd, hdc, args.cfg)) {
        std::printf("XATO: App::init muvaffaqiyatsiz\n");
        return 1;
    }
    ert::App::get().resize(args.cfg.width, args.cfg.height);

    // ---------------- asosiy sikl ----------------
    LARGE_INTEGER freq, prev;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);
    POINT center{0, 0};
    bool cursorHidden = false;
    float tourTime = 0.0f;
    size_t tourIdx = 0;
    const bool locoTour    = args.locoTour && !args.cfg.startLevel.empty();
    const bool parkourTour = !locoTour && !args.cfg.startLevel.empty();
    const bool combatTour  = !locoTour && !parkourTour && !args.cfg.startEpisode.empty();
    const TourStep* tour = locoTour ? kTourLoco
                         : (parkourTour ? kTourParkour : (combatTour ? kTourCombat : kTour));
    size_t tourCount = 0;
    if (!args.shotsDir.empty()) {
        if (locoTour)        tourCount = sizeof(kTourLoco)     / sizeof(kTourLoco[0]);
        else if (parkourTour) tourCount = sizeof(kTourParkour) / sizeof(kTourParkour[0]);
        else if (combatTour) tourCount = sizeof(kTourCombat)  / sizeof(kTourCombat[0]);
        else                 tourCount = sizeof(kTour)        / sizeof(kTour[0]);
        std::printf("[shot] sayohat: %s (%d qadam)  episode='%s' level='%s'\n",
                    locoTour ? "lokomotsiya"
                             : (parkourTour ? "parkur" : (combatTour ? "jang" : "menyu")),
                    (int)tourCount, args.cfg.startEpisode.c_str(), args.cfg.startLevel.c_str());
    }

    while (g_running && !ert::App::get().wantsQuit()) {
        ert::Input::get().beginFrame();

        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = false;
                std::printf("[chiqish] WM_QUIT
"); break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!g_running) break;

        // --- sichqoncha qulfi (faqat o'yin rejimida va fokus bo'lganda) ---
        bool wantLook = ert::Input::get().mouseLook() && g_hasFocus;
        if (wantLook) {
            if (!g_mouseLook) {              // endi yoqildi
                centerCursor(g_hwnd, center);
                if (!cursorHidden) { ShowCursor(FALSE); cursorHidden = true; }
                g_mouseLook = true;
            } else {
                POINT p; GetCursorPos(&p); ScreenToClient(g_hwnd, &p);
                float dx = (float)(p.x - center.x);
                float dy = (float)(p.y - center.y);
                if (dx != 0.0f || dy != 0.0f) {
                    ert::Input::get().setRawDelta(dx, dy);
                    centerCursor(g_hwnd, center);
                }
            }
        } else if (g_mouseLook) {
            g_mouseLook = false;
            if (cursorHidden) { ShowCursor(TRUE); cursorHidden = false; }
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = (float)((double)(now.QuadPart - prev.QuadPart) / (double)freq.QuadPart);
        prev = now;
        if (dt > 0.25f) dt = 1.0f / 60.0f;   // debugger/oyna sudralishi

        ert::App::get().update(dt);
        ert::App::get().render();
        SwapBuffers(hdc);

        // --shots: belgilangan lahzalarda kadrni saqlaymiz
        if (tourCount > 0) {
            tourTime += dt;
            while (tourIdx < tourCount && tourTime >= tour[tourIdx].at) {
                tourApply(tour[tourIdx].action);
                // Ekran o'tish effekti (fade) tugashi uchun bir necha kadr chizamiz
                for (int f = 0; f < 24; ++f) {
                    ert::App::get().update(1.0f / 60.0f);
                    ert::App::get().render();
                }
                SwapBuffers(hdc);
                const std::string path = args.shotsDir + "/" + tour[tourIdx].file;
                const bool ok = ert::App::get().captureScreenshot(path);
                std::printf("[shot] %-22s %s\n", tour[tourIdx].file, ok ? "OK" : "XATO");
                ++tourIdx;
            }
            if (tourIdx >= tourCount) { std::printf("[shot] sayohat tugadi\n"); g_running = false; }
        }

        if (!g_hasFocus) Sleep(8);           // fonda protsessorni bo'shatamiz
    }

    if (cursorHidden) ShowCursor(TRUE);