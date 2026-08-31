#pragma once
// Klaviatura/sichqoncha bog'lamalari: har amal uchun bitta klavish,
// foydalanuvchi sozlamalar menyusida o'zgartira oladi, saves/bindings.json ga saqlanadi.
#include <string>

namespace ert {

enum class Action : int {
    MoveForward = 0, MoveBack, MoveLeft, MoveRight,
    Walk,                      // Alt — past profil yurish (AC "blend in")
    Run, Crouch, Dodge,
    ParkourUp, ParkourDown,
    LightAttack, HeavyAttack, Parry, Kick, LockOn,
    Bow, Takedown, Interact, Horse,
    BilgeGoz, Journal, Timeline, YearCard,
    BindHand, HideHand, Zikr, Falcon,
    Pause, SkipScene, Advance,
    Count
};

class Bindings {
public:
    static Bindings& get();

    static constexpr int kCount = static_cast<int>(Action::Count);

    void resetDefaults();

    int  key(Action a) const;                       // VK kodi (0 = bog'lanmagan)
    void setKey(Action a, int vk);                  // ziddiyat bo'lsa eskisini bo'shatadi
    void clearKey(Action a);
    // Shu VK band bo'lgan boshqa amal (yo'q bo'lsa Action::Count)
    Action conflict(Action a, int vk) const;

    const char* id(Action a) const;                 // "move_forward"
    const char* locKey(Action a) const;             // "ui.act.move_forward"
    const char* groupLocKey(Action a) const;        // "ui.actgrp.movement"

    // Klavish nomi: "A", "Space", "Left Shift", "Sichqoncha 1", "—"
    static std::string keyName(int vk);
    // Ushbu VK ni bog'lash mumkinmi (masalan Win/Alt+Tab kabi tizim klavishlari mumkin emas)
    static bool bindable(int vk);

    bool down(Action a) const;
    bool pressed(Action a) const;

    bool load(const std::string& path);
    bool save(const std::string& path) const;

private:
    Bindings();
    int vk_[kCount];
};

} // namespace ert
