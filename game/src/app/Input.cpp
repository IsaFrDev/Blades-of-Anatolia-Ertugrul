// Kiritish holati: klaviatura, sichqoncha, menyu navigatsiyasi.
#include "ertugrul/app/Input.h"
#include <windows.h>
#include <cstring>

namespace ert {

namespace {

struct State {
    bool  down[(int)Key::Count]     = {false};
    bool  pressed[(int)Key::Count]  = {false};
    bool  released[(int)Key::Count] = {false};
    bool  mDown[3] = {false, false, false};
    bool  mPressed[3] = {false, false, false};
    bool  mReleased[3] = {false, false, false};
    int   mx = 0, my = 0;
    float dx = 0.0f, dy = 0.0f;
    int   wheel = 0;
    bool  mouseLook = false;
    // Xom VK holati (klavish bog'lash uchun) — 0..255
    bool  vkDown[256]    = {false};
    bool  vkPressed[256] = {false};
    int   lastVk = 0;
    bool  frozen = false;
};

State& S() { static State s; return s; }

} // namespace

Key vkToKey(int vk) {
    switch (vk) {
        case 'W': return Key::W;  case 'A': return Key::A;  case 'S': return Key::S;  case 'D': return Key::D;
        case 'Q': return Key::Q;  case 'E': return Key::E;  case 'R': return Key::R;  case 'F': return Key::F;
        case 'G': return Key::G;  case 'H': return Key::H;  case 'J': return Key::J;  case 'K': return Key::K;
        case 'L': return Key::L;  case 'T': return Key::T;  case 'V': return Key::V;  case 'Z': return Key::Z;
        case 'C': return Key::C;  case 'P': return Key::P;  case 'M': return Key::M;  case 'I': return Key::I;
        case 'N': return Key::N;  case 'B': return Key::B;  case 'X': return Key::X;  case 'Y': return Key::Y;
        case 'U': return Key::U;  case 'O': return Key::O;
        case VK_SPACE:  return Key::Space;
        case VK_RETURN: return Key::Enter;
        case VK_ESCAPE: return Key::Escape;
        case VK_TAB:    return Key::Tab;
        case VK_BACK:   return Key::Backspace;
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:   return Key::Shift;
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return Key::Ctrl;
        case VK_MENU: case VK_LMENU: case VK_RMENU:      return Key::Alt;
        case VK_UP:    return Key::Up;
        case VK_DOWN:  return Key::Down;
        case VK_LEFT:  return Key::Left;
        case VK_RIGHT: return Key::Right;
        case VK_F1: return Key::F1;  case VK_F2: return Key::F2;  case VK_F3: return Key::F3;
        case VK_F4: return Key::F4;  case VK_F5: return Key::F5;  case VK_F9: return Key::F9;
        case VK_F11: return Key::F11; case VK_F12: return Key::F12;
        case '1': return Key::Num1;  case '2': return Key::Num2;  case '3': return Key::Num3;
        case '4': return Key::Num4;  case '5': return Key::Num5;  case '6': return Key::Num6;
        case '7': return Key::Num7;  case '8': return Key::Num8;  case '9': return Key::Num9;
        case '0': return Key::Num0;
        default: return Key::None;
    }
}

Input& Input::get() { static Input in; return in; }

void Input::beginFrame() {
    State& s = S();
    std::memset(s.pressed,   0, sizeof(s.pressed));
    std::memset(s.released,  0, sizeof(s.released));
    std::memset(s.mPressed,  0, sizeof(s.mPressed));
    std::memset(s.mReleased, 0, sizeof(s.mReleased));
    std::memset(s.vkPressed, 0, sizeof(s.vkPressed));
    s.lastVk = 0;
    s.wheel = 0;
    s.dx = s.dy = 0.0f;
}

void Input::onKey(int vk, bool isDown) {
    State& s = S();
    if (s.frozen) return;
    if (vk >= 0 && vk < 256) {
        if (isDown) {
            if (!s.vkDown[vk]) { s.vkPressed[vk] = true; if (s.lastVk == 0) s.lastVk = vk; }
            s.vkDown[vk] = true;
        } else {
            s.vkDown[vk] = false;
        }
    }
    Key k = vkToKey(vk);
    if (k == Key::None) return;
    int i = (int)k;
    if (isDown) {
        if (!s.down[i]) s.pressed[i] = true;   // avtomatik takrorlashni filtrlaymiz
        s.down[i] = true;
    } else {
        if (s.down[i]) s.released[i] = true;
        s.down[i] = false;
    }
}

void Input::onMouseButton(int button, bool isDown) {
    if (button < 0 || button > 2) return;
    State& s = S();
    if (s.frozen) return;
    // Sichqoncha tugmalarini ham xom VK sifatida ko'rsatamiz (VK_LBUTTON=1, RBUTTON=2, MBUTTON=4)
    const int vk = (button == 0) ? VK_LBUTTON : (button == 1 ? VK_RBUTTON : VK_MBUTTON);
    if (isDown) {
        if (!s.vkDown[vk]) { s.vkPressed[vk] = true; if (s.lastVk == 0) s.lastVk = vk; }
        s.vkDown[vk] = true;
    } else {
        s.vkDown[vk] = false;
    }
    if (isDown) { if (!s.mDown[button]) s.mPressed[button] = true;  s.mDown[button] = true; }
    else        { if (s.mDown[button])  s.mReleased[button] = true; s.mDown[button] = false; }
}

void Input::onMouseMove(int x, int y) { State& s = S(); if (s.frozen) return; s.mx = x; s.my = y; }
void Input::onMouseWheel(int delta)   { S().wheel += delta; }
void Input::onChar(unsigned int)      { /* hozircha matn kiritish yo'q */ }

bool Input::down(Key k) const     { return k != Key::None && S().down[(int)k]; }
bool Input::pressed(Key k) const  { return k != Key::None && S().pressed[(int)k]; }
bool Input::released(Key k) const { return k != Key::None && S().released[(int)k]; }

bool Input::mouseDown(int b) const     { return b >= 0 && b < 3 && S().mDown[b]; }
bool Input::mousePressed(int b) const  { return b >= 0 && b < 3 && S().mPressed[b]; }
bool Input::mouseReleased(int b) const { return b >= 0 && b < 3 && S().mReleased[b]; }

int   Input::mouseX() const      { return S().mx; }
int   Input::mouseY() const      { return S().my; }
float Input::mouseDeltaX() const { return S().dx; }
float Input::mouseDeltaY() const { return S().dy; }
int   Input::wheel() const       { return S().wheel; }

void Input::setMouseLook(bool on) { S().mouseLook = on; }
bool Input::mouseLook() const     { return S().mouseLook; }
void Input::setRawDelta(float dx, float dy) { State& s = S(); s.dx += dx; s.dy += dy; }
void Input::resetDeltas() { State& s = S(); s.dx = s.dy = 0.0f; }

bool Input::downVk(int vk) const    { return vk > 0 && vk < 256 && S().vkDown[vk]; }
bool Input::pressedVk(int vk) const { return vk > 0 && vk < 256 && S().vkPressed[vk]; }
int  Input::lastPressedVk() const   { return S().lastVk; }
void Input::clearLastPressed()      { S().lastVk = 0; }
void Input::setFrozen(bool on)      { State& s = S(); s.frozen = on;
                                      if (on) { std::memset(s.down, 0, sizeof(s.down));
                                                std::memset(s.vkDown, 0, sizeof(s.vkDown)); } }
bool Input::frozen() const          { return S().frozen; }

bool Input::navUp() const     { return pressed(Key::Up)    || pressed(Key::W); }
bool Input::navDown() const   { return pressed(Key::Down)  || pressed(Key::S); }
bool Input::navLeft() const   { return pressed(Key::Left)  || pressed(Key::A); }
bool Input::navRight() const  { return pressed(Key::Right) || pressed(Key::D); }
bool Input::navAccept() const { return pressed(Key::Enter) || pressed(Key::Space); }
bool Input::navCancel() const { return pressed(Key::Escape); }

} // namespace ert
