// Klavish bog'lamalari: standart qiymatlar, ziddiyat yechimi, saqlash/yuklash,
// va Windows'dan olinadigan klavish nomlari.
#include "ertugrul/app/Bindings.h"
#include "ertugrul/app/Input.h"

#include <windows.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <cstring>
#include <cstdio>

namespace ert {

namespace {

struct Def {
    Action      act;
    const char* id;
    const char* group;      // ui.actgrp.*
    int         vk;
};

// Standart bog'lamalar. Jang klavishlari GDD (07_SETTINGS_HOTKEYS) bo'yicha.
const Def kDefs[] = {
    { Action::MoveForward, "move_forward", "movement", 'W'          },
    { Action::MoveBack,    "move_back",    "movement", 'S'          },
    { Action::MoveLeft,    "move_left",    "movement", 'A'          },
    { Action::MoveRight,   "move_right",   "movement", 'D'          },
    { Action::Walk,        "walk",         "movement", VK_MENU      },
    { Action::Run,         "run",          "movement", VK_SHIFT     },
    { Action::Crouch,      "crouch",       "movement", 'C'          },
    { Action::Dodge,       "dodge",        "movement", 'Q'          },
    { Action::ParkourUp,   "parkour_up",   "movement", VK_SPACE     },
    { Action::ParkourDown, "parkour_down", "movement", VK_CONTROL   },

    { Action::LightAttack, "light_attack", "combat",   VK_LBUTTON   },
    { Action::HeavyAttack, "heavy_attack", "combat",   0            },   // Shift + LMB (bog'lanmagan)
    { Action::Parry,       "parry",        "combat",   VK_RBUTTON   },
    { Action::Kick,        "kick",         "combat",   'R'          },
    { Action::LockOn,      "lock_on",      "combat",   VK_MBUTTON   },
    { Action::Bow,         "bow",          "combat",   'G'          },
    { Action::Takedown,    "takedown",     "combat",   'V'          },

    { Action::Interact,    "interact",     "world",    'E'          },
    { Action::Horse,       "horse",        "world",    'F'          },
    { Action::BilgeGoz,    "bilge_goz",    "world",    'H'          },
    { Action::Journal,     "journal",      "world",    'J'          },
    { Action::Timeline,    "timeline",     "world",    'N'          },
    { Action::YearCard,    "year_card",    "world",    'Y'          },
    { Action::BindHand,    "bind_hand",    "world",    'Z'          },
    { Action::HideHand,    "hide_hand",    "world",    VK_TAB       },
    { Action::Zikr,        "zikr",         "world",    'X'          },
    { Action::Falcon,      "falcon",       "world",    'L'          },

    { Action::Pause,       "pause",        "system",   VK_ESCAPE    },
    { Action::SkipScene,   "skip_scene",   "system",   VK_ESCAPE    },
    { Action::Advance,     "advance",      "system",   VK_SPACE     },
};
static_assert(sizeof(kDefs) / sizeof(kDefs[0]) == Bindings::kCount,
              "kDefs va Action::Count mos kelmaydi");

int idx(Action a) {
    const int i = static_cast<int>(a);
    return (i >= 0 && i < Bindings::kCount) ? i : 0;
}

// Tizim uchun zarur, qayta bog'lab bo'lmaydigan klavishlar
bool systemKey(int vk) {
    switch (vk) {
        case VK_LWIN: case VK_RWIN: case VK_APPS:
        case VK_SNAPSHOT: case VK_PAUSE: case VK_NUMLOCK:
        case VK_CAPITAL: case VK_SCROLL:
        case VK_F4:                 // Alt+F4
            return true;
        default: return false;
    }
}

const char* loc(const char* prefix, const char* id) {
    static char buf[8][64];
    static int  n = 0;
    n = (n + 1) % 8;
    std::snprintf(buf[n], sizeof buf[n], "%s%s", prefix, id);
    return buf[n];
}

} // namespace

Bindings::Bindings() { resetDefaults(); }

Bindings& Bindings::get() { static Bindings b; return b; }

void Bindings::resetDefaults() {
    for (int i = 0; i < kCount; ++i) vk_[i] = 0;
    for (const Def& d : kDefs) vk_[idx(d.act)] = d.vk;
}

int  Bindings::key(Action a) const { return vk_[idx(a)]; }

// Amallar ikki KONTEKSTGA bo'linadi: o'yin ichidagi (harakat/jang/dunyo) va
// tizim (pauza, sahnani o'tkazish, replikani tezlatish). Ular hech qachon bir
// vaqtda faol bo'lmaydi, shuning uchun bir xil klavishni baham ko'rishlari mumkin
// (masalan Space: o'yinda parkur, cutscene da replikani tezlatish).
bool isSystemAction(Action a) {
    return a == Action::Pause || a == Action::SkipScene || a == Action::Advance;
}

Action Bindings::conflict(Action a, int vk) const {
    if (vk == 0) return Action::Count;
    const bool aSys = isSystemAction(a);
    for (int i = 0; i < kCount; ++i) {
        if (i == idx(a) || vk_[i] != vk) continue;
        const Action other = static_cast<Action>(i);
        if (isSystemAction(other) != aSys) continue;   // boshqa kontekst — ziddiyat emas
        return other;
    }
    return Action::Count;
}

void Bindings::setKey(Action a, int vk) {
    if (vk != 0 && !bindable(vk)) return;
    const Action c = conflict(a, vk);
    if (c != Action::Count) vk_[idx(c)] = 0;    // eskisini bo'shatamiz
    vk_[idx(a)] = vk;
}

void Bindings::clearKey(Action a) { vk_[idx(a)] = 0; }

const char* Bindings::id(Action a) const { return kDefs[idx(a)].id; }
const char* Bindings::locKey(Action a) const { return loc("ui.act.", kDefs[idx(a)].id); }
const char* Bindings::groupLocKey(Action a) const { return loc("ui.actgrp.", kDefs[idx(a)].group); }

bool Bindings::bindable(int vk) {
    if (vk <= 0 || vk >= 256) return false;
    return !systemKey(vk);
}

std::string Bindings::keyName(int vk) {
    if (vk == 0) return "—";                 // em dash
    switch (vk) {
        case VK_LBUTTON: return "Mouse 1";
        case VK_RBUTTON: return "Mouse 2";
        case VK_MBUTTON: return "Mouse 3";
        case VK_SHIFT:   return "Shift";
        case VK_CONTROL: return "Ctrl";
        case VK_MENU:    return "Alt";
        case VK_ESCAPE:  return "Esc";
        case VK_SPACE:   return "Space";
        case VK_RETURN:  return "Enter";
        case VK_TAB:     return "Tab";
        case VK_BACK:    return "Backspace";
        default: break;
    }
    // Windows'ning o'z nomlari (klaviatura tiliga mos)
    UINT sc = MapVirtualKeyW((UINT)vk, MAPVK_VK_TO_VSC);
    switch (vk) {   // kengaytirilgan klavishlar
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            sc |= 0x100u; break;
        default: break;
    }
    wchar_t w[64] = {0};
    if (sc != 0 && GetKeyNameTextW((LONG)(sc << 16), w, 63) > 0) {
        char utf8[128] = {0};
        if (WideCharToMultiByte(CP_UTF8, 0, w, -1, utf8, sizeof utf8 - 1, nullptr, nullptr) > 0)
            return utf8;
    }
    char b[32];
    std::snprintf(b, sizeof b, "0x%02X", vk);
    return b;
}

bool Bindings::down(Action a) const {
    const int vk = key(a);
    return vk != 0 && Input::get().downVk(vk);
}

bool Bindings::pressed(Action a) const {
    const int vk = key(a);
    return vk != 0 && Input::get().pressedVk(vk);
}

bool Bindings::save(const std::string& path) const {
    try {
        nlohmann::json j;
        for (const Def& d : kDefs) j[d.id] = vk_[idx(d.act)];
        CreateDirectoryA("saves", nullptr);
        std::ofstream f(path);
        if (!f) return false;
        f << j.dump(2);
        return true;
    } catch (...) { return false; }
}

bool Bindings::load(const std::string& path) {
    try {
        std::ifstream f(path);
        if (!f) return false;
        nlohmann::json j;
        f >> j;
        for (const Def& d : kDefs) {
            if (!j.contains(d.id)) continue;
            const auto& v = j[d.id];
            if (!v.is_number_integer()) continue;
            const int vk = v.get<int>();
            if (vk == 0 || bindable(vk)) vk_[idx(d.act)] = vk;
        }
        // Bir martalik migratsiya: Alt endi Action::Walk ga tegishli.
        // Eski saqlangan faylda HideHand hamon VK_MENU bo'lsa — Tab ga ko'chiramiz.
        if (vk_[idx(Action::HideHand)] == VK_MENU) vk_[idx(Action::HideHand)] = VK_TAB;
        if (vk_[idx(Action::Walk)] == 0)           vk_[idx(Action::Walk)]     = VK_MENU;
        return true;
    } catch (...) { return false; }
}

} // namespace ert
