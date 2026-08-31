#pragma once
#include <cstdint>

namespace ert {

enum class Key : int {
    None = 0,
    W, A, S, D, Q, E, R, F, G, H, J, K, L, T, V, Z, C, P, M, I, N, B, X, Y, U, O,
    Space, Enter, Escape, Tab, Backspace, Shift, Ctrl, Alt,
    Up, Down, Left, Right,
    F1, F2, F3, F4, F5, F9, F11, F12,
    Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Count
};

class Input {
public:
    static Input& get();

    void beginFrame();                       // har kadr boshida (pressed/released ni tozalaydi)
    void onKey(int vk, bool down);
    void onMouseButton(int button, bool down);   // 0=chap 1=o'ng 2=o'rta
    void onMouseMove(int x, int y);
    void onMouseWheel(int delta);
    void onChar(unsigned int codepoint);

    bool down(Key k) const;
    bool pressed(Key k) const;               // shu kadrda bosildi
    bool released(Key k) const;

    bool mouseDown(int b) const;
    bool mousePressed(int b) const;
    bool mouseReleased(int b) const;

    int  mouseX() const;
    int  mouseY() const;
    float mouseDeltaX() const;
    float mouseDeltaY() const;
    int  wheel() const;

    void setMouseLook(bool on);              // kursorni markazga qulflash
    bool mouseLook() const;
    void setRawDelta(float dx, float dy);    // oyna kodi tomonidan
    void resetDeltas();

    // Menyu navigatsiyasi uchun umumlashgan signal (klaviatura + strelka)
    bool navUp() const;  bool navDown() const; bool navLeft() const; bool navRight() const;
    bool navAccept() const; bool navCancel() const;

    // --- Xom VK kodlari (klavish bog'lash uchun) ---
    bool downVk(int vk) const;
    bool pressedVk(int vk) const;
    // Shu kadrda bosilgan birinchi VK (0 = yo'q). Sichqoncha tugmalari
    // VK_LBUTTON / VK_RBUTTON / VK_MBUTTON sifatida qaytadi.
    int  lastPressedVk() const;
    void clearLastPressed();

    // Avtomatik sayohat (--shots) uchun: haqiqiy kiritishni butunlay e'tiborsiz qoldiradi
    void setFrozen(bool on);
    bool frozen() const;

private:
    Input() = default;
};

Key vkToKey(int vk);

} // namespace ert
