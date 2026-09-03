// Ilova holat mashinasi: Splash -> Til -> Menyu -> Epizod -> Cutscene -> O'yin -> Pauza.
// 3D sahna chizish, uchinchi shaxs boshqaruvi, HUD va subtitrlar shu yerda.
#include "ertugrul/gfx/ShadowMap.h"
#include "ertugrul/gfx/Pbr.h"
#include "ertugrul/app/App.h"
#include "ertugrul/app/Input.h"
#include "ertugrul/app/Bindings.h"
#include "ertugrul/ui/Menu.h"
#include "ertugrul/gfx/Font.h"
#include "ertugrul/gfx/Texture.h"
#include "ertugrul/gfx/Mesh.h"
#include "ertugrul/gfx/Skin.h"
#include "ertugrul/loc/Loc.h"
#include "ertugrul/game/Episodes.h"
#include "ertugrul/game/Cutscene.h"
#include "ertugrul/world/Level.h"
#include "ertugrul/world/Physics.h"
#include "ertugrul/game/Character.h"
#include "ertugrul/game/Enemy.h"
#include "ertugrul/game/Encounter.h"
#include "ertugrul/audio/Audio.h"
#include "ertugrul/audio/Voice.h"
#include "ertugrul/audio/Sfx.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <objidl.h>
#include <gdiplus.h>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace ert {

namespace {

// «Temir va Firuza» palitrasi (menyu bilan bir xil)
const float FON[3]    = {0.055f, 0.075f, 0.086f};
const float MATN[3]   = {0.894f, 0.918f, 0.918f};
const float SONIK[3]  = {0.486f, 0.545f, 0.561f};
const float FERUZA[3] = {0.282f, 0.663f, 0.710f};
const float ZARHAL[3] = {0.753f, 0.588f, 0.314f};
const float SUYAK[3]  = {0.863f, 0.827f, 0.769f};
const float YARA[3]   = {0.737f, 0.353f, 0.267f};
const float CHIZIQ[3] = {0.165f, 0.208f, 0.227f};

struct Impl {
    HWND  hwnd = nullptr;
    HDC   hdc  = nullptr;
    AppConfig cfg;
    AppState  state = AppState::Boot;
    int   w = 1280, h = 760;
    bool  quit = false;
    float fps = 0.0f, fpsAccum = 0.0f;
    int   fpsFrames = 0;

    Font  fDisp, fItem, fSmall, fSub, fHud, fMono;
    Level level;
    std::string levelId;

    // O'yinchi — AC uslubidagi parkur boshqaruvi
    SkinnedModel playerModel;
    PhysicsWorld phys;
    Character    player;
    float camYaw = 0.0f, camPitch = 12.0f, camDist = 5.2f;
    float camDistTarget = 5.2f;
    Vec3  camSmooth{0, 2, 8};
    float lastDt = 1.0f / 60.0f;      // kamera damp uchun HAQIQIY dt
    float camFov = 48.0f;             // kamon nishonida torayadi
    float shakePhase = 0.0f;          // kamera silkinishi fazasi
    float inSmX = 0.0f, inSmY = 0.0f; // kiritish vektori silliqlagichi

    Vec3  spawnPos{0, 0, 0};
    float spawnYaw = 0.0f;

    // Bilge Ko'z (Eagle Vision)
    bool  eagle = false;
    float eagleT = 0.0f;

    // Jang
    Vec3  lockPos{0, 0, 0};        // setLockTarget uchun barqaror manzil
    int   endSel = 0;              // muvaffaqiyatsizlik / tugash ekranidagi tanlov
    float endT = 0.0f;

    // Menyu foni uchun aylanuvchi kamera
    float menuOrbit = 0.0f;

    // Epizod / cutscene
    std::string episodeId;
    std::string episodeTitle;
    std::string episodeMeta;      // "1227 Avgust · Anadolu chegarasi"
    std::string lastEpisode;      // «Davom ettirish» uchun
    float loadTimer = 0.0f;
    float stateFade = 0.0f;

    bool  initialized = false;
    int   frameCount = 0;
};

Impl& I() { static Impl i; return i; }

// --------------------------------------------------------------- 3D yordamchilar

void beginScene3D(int w, int h, float fovDeg, const Vec3& eye, const Vec3& target) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = h > 0 ? (float)w / (float)h : 1.6f;
    gluPerspective(fovDeg, aspect, 0.08, 900.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // Ko'rish yo'nalishi tik bo'lib qolsa gluLookAt buziladi -> "up" ni ehtiyot qilamiz
    Vec3 dir = normalize(target - eye);
    Vec3 up{0, 1, 0};
    if (std::fabs(dir.y) > 0.995f) up = Vec3{0, 0, 1};
    gluLookAt(eye.x, eye.y, eye.z, target.x, target.y, target.z, up.x, up.y, up.z);
}

// Personaj/rekvizit ostidagi yumshoq soya
void drawBlobShadow(const Level& lv, const Vec3& p, float radius, float alpha) {
    Pbr::get().pause();
    float gy = lv.groundAt(p.x, p.z) + 0.02f;
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(p.x, gy, p.z);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= 18; ++i) {
        float a = (float)i / 18.0f * TAU;
        glVertex3f(p.x + std::cos(a) * radius, gy, p.z + std::sin(a) * radius);
    }
    glEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    Pbr::get().resume();
}

// Bir xil model bilan chizilgan personajlarni farqlash uchun material rangi.
// GL_COLOR_MATERIAL o'chiriladi -> Mesh o'zining glColor4f'ini chaqirsa ham
// yorug'lik hisobida shu material ishlatiladi (tekstura x tint).
void pushTintMaterial(const float rgb[3]) {
    glDisable(GL_COLOR_MATERIAL);
    const GLfloat d[4] = { rgb[0], rgb[1], rgb[2], 1.0f };
    const GLfloat a[4] = { rgb[0] * 0.38f, rgb[1] * 0.38f, rgb[2] * 0.38f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, d);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, a);
    Pbr::get().setMaterial(rgb[0], rgb[1], rgb[2], 1.0f);
    Pbr::get().setUseVertexColor(false);
    Pbr::get().setRoughness(0.55f);            // mato/teri/zirh aralash
}
void popTintMaterial() {
    glEnable(GL_COLOR_MATERIAL);
    Pbr::get().setUseVertexColor(true);
    Pbr::get().setRoughness(0.70f);
}

// «Bilge Ko'z» (Eagle Vision) qoplamasi.
// AC dagi kabi dunyo rangi so'nadi va MUHIM geometriya yoritiladi:
// chiqib bo'ladigan yuzalar — feruza, sakrab o'tiladigan to'siqlar — zarhal.
void drawEagleOverlay(const PhysicsWorld& phys, float t) {
    if (t < 0.01f) return;
    // GL_COLOR_BUFFER_BIT ham push qilinadi — aks holda qo'shuvchi blend
    // keyingi kadrga oqib, ekranni oqartirib yuboradi.
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT |
                 GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);      // qo'shuvchi — "yorqin kontur"
    glLineWidth(1.6f);

    const std::vector<Box>& bs = phys.boxes();
    glBegin(GL_LINES);
    for (size_t i = 0; i < bs.size(); ++i) {
        const Box& b = bs[i];
        if (!b.climbable && !b.vaultable) continue;
        if (b.climbable) glColor4f(0.28f, 0.66f, 0.71f, 0.34f * t);
        else             glColor4f(0.75f, 0.59f, 0.31f, 0.26f * t);
        const float y0 = b.baseY, y1 = b.topY;
        const float x0 = b.minX, x1 = b.maxX, z0 = b.minZ, z1 = b.maxZ;
        // yuqori qirralar (chekkalar) — parkur uchun eng muhimi
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
        glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
        // tik qirralar (so'nikroq)
        if (b.climbable) {
            glColor4f(0.28f, 0.66f, 0.71f, 0.12f * t);
            glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0);
            glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
            glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
            glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1);
        }
    }
    glEnd();
    glPopAttrib();
}

// Ekran pastida subtitr paneli
void drawSubtitle(Impl& im, const std::string& speaker, const std::string& text) {
    if (text.empty() || !im.cfg.subtitles) return;
    Font& f = im.fSub.valid() ? im.fSub : im.fSmall;
    if (!f.valid()) return;

    const float maxW = im.w * 0.72f;
    const float cx   = im.w * 0.5f;
    int lines = f.wrappedLineCount(text, maxW);
    float lh  = f.lineHeight();
    const float nameH = speaker.empty() ? 0.0f
                      : (im.fMono.valid() ? im.fMono.lineHeight() : lh) + 6.0f;
    float boxH = lines * lh + 26.0f + nameH;
    float boxY = im.h - boxH - im.h * 0.10f;

    drawGradientRect(cx - maxW * 0.5f - 40.0f, boxY - 18.0f, maxW + 80.0f, boxH + 34.0f,
                     FON[0], FON[1], FON[2], 0.0f, FON[0], FON[1], FON[2], 0.78f);
    float ty = boxY;
    if (!speaker.empty()) {
        // ism: feruza, kichik; oldida «mix boshi» kvadrati
        Font& fs = im.fMono.valid() ? im.fMono : f;
        const float nw = fs.measure(speaker);
        drawRect(cx - nw * 0.5f - 16.0f, ty + 5.0f, 6.0f, 6.0f, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
        fs.draw(speaker, cx, ty, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f, TextAlign::Center);
        ty += fs.lineHeight() + 6.0f;
    }
    f.drawWrapped(text, cx, ty, maxW, MATN[0], MATN[1], MATN[2], 1.0f, TextAlign::Center);
}

void drawHud(Impl& im) {
    if (!im.fHud.valid() || !im.cfg.showHud) return;
    Font& fh = im.fHud;
    Font& fm = im.fMono.valid() ? im.fMono : im.fHud;

    // Yuqori chap: ▪ EPIZOD NOMI  +  yil / mintaqa (mono, so'nik)
    if (!im.episodeTitle.empty()) {
        drawRect(26.0f, 27.0f, 7.0f, 7.0f, FERUZA[0], FERUZA[1], FERUZA[2], 0.95f);
        fh.draw(im.episodeTitle, 44.0f, 20.0f, MATN[0], MATN[1], MATN[2], 0.92f);
        if (!im.episodeMeta.empty())
            fm.draw(im.episodeMeta, 44.0f, 20.0f + fh.lineHeight() + 2.0f,
                    SONIK[0], SONIK[1], SONIK[2], 0.7f);
    }
    // Yuqori o'ng: FPS (mono, juda so'nik)
    if (im.cfg.showFps) {
        char b[64];
        std::snprintf(b, sizeof b, "%.0f FPS", im.fps);
        fm.draw(b, (float)im.w - 26.0f, 22.0f, SONIK[0], SONIK[1], SONIK[2], 0.55f, TextAlign::Right);
    }
    // Pastki o'ng: profil + shovqin ko'rsatkichi (AC uslubi)
    {
        const bool high = (im.player.profile() == Profile::High);
        const float* pc = high ? SUYAK : SONIK;
        const std::string pl = T(high ? "ui.profile.high" : "ui.profile.low");
        const float rx = (float)im.w - 26.0f;
        const float ry = (float)im.h - 60.0f;
        fm.draw(pl, rx, ry, pc[0], pc[1], pc[2], high ? 0.95f : 0.6f, TextAlign::Right);

        // shovqin — kichik kvadratlar qatori
        const float n = clampf(im.player.lastNoise(), 0.0f, 1.0f);
        const int   lit = (int)(n * 6.0f + 0.5f);
        for (int i = 0; i < 6; ++i) {
            const float bx = rx - 6.0f - (5 - i) * 11.0f;
            const bool on = (i < lit);
            const float* c = (i >= 4) ? YARA : (i >= 2 ? ZARHAL : FERUZA);
            if (on) drawRect(bx, ry + fm.lineHeight() + 6.0f, 7.0f, 7.0f, c[0], c[1], c[2], 0.9f);
            else    drawRectOutline(bx, ry + fm.lineHeight() + 6.0f, 7.0f, 7.0f, 1.0f,
                                    SONIK[0], SONIK[1], SONIK[2], 0.35f);
        }
        // holat nomi (faqat parkur harakatlarida ko'rinadi)
        const MoveState st = im.player.state();
        if (st != MoveState::Idle && st != MoveState::Walk && st != MoveState::Jog) {
            fm.draw(T(im.player.stateLocKey()), rx, ry - fm.lineHeight() - 6.0f,
                    FERUZA[0], FERUZA[1], FERUZA[2], 0.85f, TextAlign::Right);
        }
    }
    // Bilge Ko'z yoqilganini ko'rsatamiz
    if (im.eagleT > 0.02f) {
        drawRect(26.0f, (float)im.h - 62.0f, 7.0f, 7.0f, FERUZA[0], FERUZA[1], FERUZA[2], im.eagleT);
        fm.draw(T("ui.hud.eagle"), 44.0f, (float)im.h - 66.0f,
                FERUZA[0], FERUZA[1], FERUZA[2], 0.9f * im.eagleT);
    }

    // Pastki chap: boshqaruv eslatmasi — HAQIQIY bog'lamalardan quriladi
    if (im.cfg.showHints) {
        const Bindings& kb = Bindings::get();
        std::string hint =
            Bindings::keyName(kb.key(Action::MoveForward)) + "/" +
            Bindings::keyName(kb.key(Action::MoveLeft))    + "/" +
            Bindings::keyName(kb.key(Action::MoveBack))    + "/" +
            Bindings::keyName(kb.key(Action::MoveRight)) + "  " +
            T("ui.controls.move") + "   ·   " +
            Bindings::keyName(kb.key(Action::Run)) + "  " + T("ui.controls.run") +
            "   ·   " + Bindings::keyName(kb.key(Action::ParkourUp)) + "  " + T("ui.act.parkour_up") +
            "   ·   " + Bindings::keyName(kb.key(Action::BilgeGoz)) + "  " + T("ui.hud.eagle") +
            "   ·   " + Bindings::keyName(kb.key(Action::Pause)) + "  " + T("ui.controls.pause");
        fm.draw(hint, 26.0f, (float)im.h - 34.0f, SONIK[0], SONIK[1], SONIK[2], 0.5f);
    }
}


// --------------------------------------------------------------- jang HUD

// Yupqa gorizontal ko'rsatkich (sog'liq / nafas / poza)
void drawBar(float x, float y, float w, float h, float t,
             const float* col, const float* bg, float alpha) {
    drawRect(x, y, w, h, bg[0], bg[1], bg[2], 0.75f * alpha);
    drawRect(x, y, w * clampf(t, 0.0f, 1.0f), h, col[0], col[1], col[2], alpha);
}

// Dushman yoki maqsad ustidagi belgini ekran koordinatasiga o'tkazadi.
// Kamera ortida bo'lsa false.
[[maybe_unused]] bool worldToScreen(const Vec3& p, int sw, int sh, float& outX, float& outY) {
    GLdouble mv[16], pr[16];
    GLint vp[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, mv);
    glGetDoublev(GL_PROJECTION_MATRIX, pr);
    glGetIntegerv(GL_VIEWPORT, vp);
    GLdouble wx = 0, wy = 0, wz = 0;
    if (gluProject(p.x, p.y, p.z, mv, pr, vp, &wx, &wy, &wz) != GL_TRUE) return false;
    if (wz < 0.0 || wz > 1.0) return false;
    outX = (float)wx;
    outY = (float)sh - (float)wy;          // GL pastdan, 2D qatlam yuqoridan
    (void)sw;
    return true;
}

void drawCombatHud(Impl& im) {
    if (!im.fHud.valid() || !im.cfg.showHud) return;
    Encounter& enc = Encounter::get();
    if (enc.state() == EncounterState::Inactive) return;

    Font& fh = im.fHud;
    Font& fm = im.fMono.valid() ? im.fMono : im.fHud;
    const Vitals& v = im.player.vitals;

    // --- Pastki chap: uch resurs ---
    const float bx = 26.0f, bw = 268.0f;
    float by = (float)im.h - 116.0f;
    const float dark[3] = { 0.10f, 0.13f, 0.15f };

    drawBar(bx, by, bw, 7.0f, v.healthPct(), YARA, dark, 1.0f);
    fm.draw(T("ui.hud.health"), bx, by - fm.lineHeight() - 2.0f, SONIK[0], SONIK[1], SONIK[2], 0.6f);
    by += 14.0f;
    drawBar(bx, by, bw * 0.78f, 5.0f, v.breathPct(), SUYAK, dark, 0.95f);
    by += 11.0f;
    // Poza — teskari: to'lsa muvozanat buziladi
    const float* postCol = v.posturePct() > 0.75f ? YARA : ZARHAL;
    drawBar(bx, by, bw * 0.78f, 5.0f, v.posturePct(), postCol, dark, 0.95f);
    if (v.staggered) {
        const float pulse = 0.5f + 0.5f * std::sin(im.endT * 12.0f);
        fm.draw(T("ui.state.staggered"), bx, by + 10.0f, YARA[0], YARA[1], YARA[2], 0.6f + 0.4f * pulse);
    }

    // --- Iymon: eng ingichka chiziq. Doim ko'rinadi, lekin so'nik;
    //     o'zgarganda porlaydi va daraja nomi chiqadi. ---
    {
        const Faith& fa = im.player.faith;
        by += 11.0f;
        const float* col = (fa.tier() == FaithTier::Sukunat)  ? FERUZA
                         : (fa.tier() == FaithTier::Adashgan) ? YARA : ZARHAL;
        const float glow = (fa.flashT > 0.0f) ? (0.45f + 0.55f * saturate(fa.flashT / 1.6f)) : 0.45f;
        drawBar(bx, by, bw * 0.78f, 4.0f, clampf(fa.value * 0.01f, 0.0f, 1.0f), col, dark, glow);

        // Sukunat faol bo'lsa - chapda kichik feruza kvadrat (dizayn tizimi motivi)
        if (fa.silenceT > 0.0f)
            drawRect(bx - 12.0f, by - 1.0f, 6.0f, 6.0f, FERUZA[0], FERUZA[1], FERUZA[2],
                     0.55f + 0.45f * saturate(fa.silenceT / 1.2f));

        if (fa.flashT > 0.0f) {
            char buf[96];
            const std::string tn = T(faithTierLocKey(fa.tier()));
            std::snprintf(buf, sizeof(buf), "%s  %+.1f", tn.c_str(), fa.flashDelta);
            const float a = saturate(fa.flashT / 1.6f);
            const float* fc = (fa.flashDelta >= 0.0f) ? col : YARA;
            fm.draw(buf, bx, by + 9.0f, fc[0], fc[1], fc[2], 0.85f * a);
        }
    }

    // --- Kamon: nishon belgisi, tortish kuchi va o'q soni ---
    if (im.player.aimBlend() > 0.01f) {
        const float ab = clampf(im.player.aimBlend(), 0.0f, 1.0f);
        const float cx = im.w * 0.5f + 40.0f * ab;   // yelka ustidagi kamera uchun siljitilgan
        const float cy = im.h * 0.5f;
        const float ch = clampf(im.player.drawCharge(), 0.0f, 1.0f);

        // Tarqalish halqasi: tortilgani sari torayadi. Bu HALOL ko'rsatkich —
        // o'q shu doiraning ichiga tushadi, tasodifiy emas (titrash sinusoidal).
        const float rad = 8.0f + im.player.aimSpread() * 7.0f;
        const int   seg = 28;
        for (int i = 0; i < seg; ++i) {
            const float a0 = (float)i / seg * TAU;
            const float px = cx + std::cos(a0) * rad;
            const float py2 = cy + std::sin(a0) * rad;
            drawRect(px - 1.0f, py2 - 1.0f, 2.0f, 2.0f,
                     SUYAK[0], SUYAK[1], SUYAK[2], 0.55f * ab);
        }
        // Markaz nuqtasi — to'la tortilganda zarhal
        const float* mc = (ch >= 0.999f) ? ZARHAL : SUYAK;
        drawRect(cx - 1.5f, cy - 1.5f, 3.0f, 3.0f, mc[0], mc[1], mc[2], 0.9f * ab);

        // Tortish chizig'i (markazdan pastda)
        drawRect(cx - 26.0f, cy + rad + 10.0f, 52.0f, 2.0f, 0.10f, 0.13f, 0.15f, 0.7f * ab);
        drawRect(cx - 26.0f, cy + rad + 10.0f, 52.0f * ch, 2.0f,
                 mc[0], mc[1], mc[2], 0.9f * ab);

        // Qo'l titrashi ogohlantirishi (nafas tugab bormoqda)
        if (im.player.aimShake() > 0.35f) {
            const float sa = clampf((im.player.aimShake() - 0.35f) / 0.65f, 0.0f, 1.0f);
            drawRect(cx - 30.0f, cy + rad + 16.0f, 60.0f, 1.0f,
                     YARA[0], YARA[1], YARA[2], 0.75f * sa * ab);
        }
    }

    // O'q soni — sog'liq ustunining o'ng tomonida, kichik kvadratlar bilan
    {
        const int have = im.player.arrows(), cap = im.player.arrowsMax();
        if (cap > 0) {
            char ab[48];
            std::snprintf(ab, sizeof(ab), "%d/%d", have, cap);
            const float ax = bx + bw + 16.0f;
            const float ay = (float)im.h - 116.0f;
            const float pf = enc.pickupFlash();          // o'q yig'ilganda yonadi
            const float* ac = (have == 0) ? YARA
                            : ((im.player.aimBlend() > 0.01f || pf > 0.0f) ? ZARHAL : SONIK);
            const float aa = (have == 0) ? 0.9f : (0.65f + 0.35f * pf);
            drawRect(ax - 1.0f * pf, ay + 1.0f - 1.0f * pf, 5.0f + 2.0f * pf, 5.0f + 2.0f * pf,
                     ac[0], ac[1], ac[2], aa);
            fm.draw(ab, ax + 11.0f, ay - 3.0f, ac[0], ac[1], ac[2],
                    (have == 0) ? 0.95f : (0.7f + 0.3f * pf));
        }
    }

    // --- Yuqori o'ng: maqsadlar ---
    const auto& objs = enc.objectives();
    if (!objs.empty()) {
        float oy = 62.0f;
        const float ox = (float)im.w - 26.0f;
        fm.draw(T("ui.obj.title"), ox, oy, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f, TextAlign::Right);
        oy += fm.lineHeight() + 8.0f;
        for (size_t i = 0; i < objs.size(); ++i) {
            const Objective& o = objs[i];
            const float* c = o.failed ? YARA : (o.done ? SONIK : MATN);
            const float a = o.done ? 0.55f : 1.0f;
            // ▪ belgisi
            drawRect(ox - fh.measure(o.text()) - 16.0f, oy + 5.0f, 6.0f, 6.0f,
                     c[0], c[1], c[2], a);
            fh.draw(o.text(), ox, oy, c[0], c[1], c[2], a, TextAlign::Right);
            oy += fh.lineHeight() + 5.0f;
        }
        // to'lqin ko'rsatkichi
        char wv[64];
        std::snprintf(wv, sizeof wv, "%s %d/%d", T("ui.hud.wave").c_str(),
                      enc.waveIndex() + 1, enc.waveCount());
        fm.draw(wv, ox, oy + 6.0f, SONIK[0], SONIK[1], SONIK[2], 0.7f, TextAlign::Right);
    }

    // --- Nazorat nuqtasi xabari ---
    const float cf = enc.checkpointFlash();
    if (cf > 0.01f) {
        const float cx = im.w * 0.5f;
        drawRect(cx - 6.0f, (float)im.h * 0.30f, 7.0f, 7.0f, FERUZA[0], FERUZA[1], FERUZA[2], cf);
        fm.draw(T("ui.hud.checkpoint"), cx + 14.0f, (float)im.h * 0.30f - 4.0f,
                FERUZA[0], FERUZA[1], FERUZA[2], cf);
    }

    // --- Brifing ---
    if (enc.state() == EncounterState::Briefing) {
        const float a = smoothstepf(clampf(enc.stateTime() * 2.0f, 0.0f, 1.0f));
        const float cx = im.w * 0.5f;
        im.fSmall.draw(T("ui.brief.title"), cx, im.h * 0.36f,
                       FERUZA[0], FERUZA[1], FERUZA[2], a, TextAlign::Center);
    }
}

void drawLoading(Impl& im) {
    begin2D(im.w, im.h);
    drawFullscreenFade(im.w, im.h, FON[0], FON[1], FON[2], 1.0f);
    const float x0 = std::max(64.0f, im.w * 0.105f);
    float y = im.h * 0.40f;

    if (im.fMono.valid())
        im.fMono.draw(im.episodeMeta.empty() ? T("ui.loading") : im.episodeMeta,
                      x0, y - 30.0f, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f);
    if (im.fDisp.valid() && !im.episodeTitle.empty()) {
        im.fDisp.draw(im.episodeTitle, x0, y, MATN[0], MATN[1], MATN[2], 1.0f);
        y += im.fDisp.lineHeight() + 22.0f;
    }
    // yupqa progress chizig'i + «mix boshi» yuguruvchi kvadrat
    const float t  = clampf(im.loadTimer / 1.2f, 0.0f, 1.0f);
    const float bw = std::min(560.0f, im.w - x0 * 2.0f);
    drawRect(x0, y, bw, 1.0f, 0.165f, 0.208f, 0.227f, 1.0f);
    drawRect(x0, y, bw * easeOutCubic(t), 1.0f, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
    drawRect(x0 + bw * easeOutCubic(t) - 3.0f, y - 3.0f, 7.0f, 7.0f, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);

    if (im.fSmall.valid())
        im.fSmall.draw(T("ui.loading.hint"), x0, im.h * 0.80f, SONIK[0], SONIK[1], SONIK[2], 0.8f);
    end2D();
}


// --------------------------------------------------------- yakuniy ekranlar

// Umumiy: to'q fon, chapga tekislangan sarlavha, tanlov ro'yxati
void drawEndScreen(Impl& im, const char* titleKey, const char* kicker,
                   const char* const* items, int itemCount, int sel,
                   const std::string& body, const std::string& stats) {
    begin2D(im.w, im.h);
    drawFullscreenFade(im.w, im.h, FON[0], FON[1], FON[2], 0.90f);
    const float x0 = std::max(64.0f, im.w * 0.105f);
    const float a = smoothstepf(clampf(im.endT * 1.6f, 0.0f, 1.0f));
    float y = im.h * 0.22f;

    if (im.fMono.valid() && kicker && *kicker)
        im.fMono.draw(kicker, x0, y - 30.0f, FERUZA[0], FERUZA[1], FERUZA[2], 0.85f * a);
    if (im.fDisp.valid()) {
        im.fDisp.draw(T(titleKey), x0, y, MATN[0], MATN[1], MATN[2], a);
        y += im.fDisp.lineHeight() + 18.0f;
    }
    drawRect(x0, y, 1.0f, 1.0f, 0, 0, 0, 0);       // joy ushlagich
    drawRect(x0, y, 220.0f * a, 1.0f, CHIZIQ[0], CHIZIQ[1], CHIZIQ[2], 1.0f);
    y += 22.0f;

    if (!body.empty() && im.fSmall.valid()) {
        y += im.fSmall.drawWrapped(body, x0, y, std::min(680.0f, im.w - x0 * 2.0f),
                                   SONIK[0], SONIK[1], SONIK[2], 0.95f * a) * im.fSmall.lineHeight();
        y += 16.0f;
    }
    if (!stats.empty() && im.fMono.valid()) {
        im.fMono.draw(stats, x0, y, ZARHAL[0], ZARHAL[1], ZARHAL[2], 0.9f * a);
        y += im.fMono.lineHeight() + 26.0f;
    }

    const float rowH = im.fItem.valid() ? im.fItem.lineHeight() + 18.0f : 42.0f;
    for (int i = 0; i < itemCount; ++i) {
        const float ry = y + i * rowH;
        const bool on = (i == sel);
        if (on) {
            drawRect(x0 - 26.0f, ry - 2.0f, 2.0f, rowH - 10.0f, FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
            drawRect(x0 - 14.0f, ry + (rowH - 10.0f) * 0.5f - 3.0f, 6.0f, 6.0f,
                     FERUZA[0], FERUZA[1], FERUZA[2], 1.0f);
        }
        const float* c = on ? MATN : SONIK;
        if (im.fItem.valid()) {
            std::string label = T(items[i]);
            for (char& ch : label) ch = (char)std::toupper((unsigned char)ch);
            im.fItem.draw(label, x0, ry, c[0], c[1], c[2], a);
        }
    }
    end2D();
}

const char* const kFailItems[3] = { "ui.fail.retry", "ui.fail.restart", "ui.fail.menu" };
const char* const kDoneItems[2] = { "ui.done.next", "ui.done.menu" };

bool levelFileExists(const std::string& id) {
    std::string p = "data/levels/" + id + ".json";
    DWORD a = GetFileAttributesA(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

// Epizod/cutscene ma'lumotidan mavjud daraja faylini tanlaydi
std::string pickLevel(const Episode* e, const CutScene* cs) {
    auto lower = [](std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; };
    if (cs && !cs->levelId.empty() && levelFileExists(lower(cs->levelId))) return lower(cs->levelId);
    if (e) {
        for (const std::string& l : e->levels)
            if (levelFileExists(lower(l))) return lower(l);
        std::string region = lower(e->region);
        if (region.find("halab") != std::string::npos || region.find("aleppo") != std::string::npos ||
            region.find("sham")  != std::string::npos || region.find("konya")  != std::string::npos) {
            if (levelFileExists("aleppo_road")) return "aleppo_road";
        }
        // Parkur ko'p bo'ladigan arxetiplar uchun qishloq (tomlar, minora)
        if (e->archetype == "INFILTRATION" || e->archetype == "CHASE" || e->archetype == "COURT") {
            if (levelFileExists("sogut_village")) return "sogut_village";
        }
        if (e->archetype == "SURVIVAL" || e->weather == "fog" || e->weather == "snow") {
            if (levelFileExists("forest_pass")) return "forest_pass";
        }
    }
    // Standart lager darajasi — yangi "vodiy" xaritasi (oba_camp dan chiroyliroq:
    // haqiqiy tepaliklar, qalin o'rmon, oltin soat yoritishi, kigiz o'tovlar).
    if (levelFileExists("oba_valley")) return "oba_valley";
    return "oba_camp";
}

// Soya xaritasi o'timi (shadersiz, ARB_shadow). casters() personaj/aktyorlarni
// chizadi; relyef va rekvizitlar Level::drawCasters dan. Quyosh past bo'lsa yoki
// kengaytma bo'lmasa false — o'shanda eski yumshoq disklar ishlatiladi.
template <class F>
bool shadowPass(Impl& im, const Vec3& focus, F casters) {
    ShadowMap& sm = ShadowMap::get();
    if (!sm.enabled()) return false;
    const SkyPreset& s = im.level.sky();
    if (!sm.begin(Vec3{s.sunDir[0], s.sunDir[1], s.sunDir[2]}, focus, 34.0f, im.w, im.h))
        return false;
    im.level.drawCasters(focus, 42.0f);
    casters();
    sm.end();
    return true;
}
// Soya qorong'iligi: kunduzi 0.42, tong/shomda yumshoqroq
float shadowLevelFor(const Impl& im) {
    const SkyPreset& s = im.level.sky();
    return (s.sunDir[1] > 0.5f) ? 0.42f : 0.55f;
}

// Menyu foni: lager ustida sekin aylanuvchi kamera
void renderMenuBackdrop(Impl& im, float dt) {
    im.menuOrbit += dt * 3.2f;
    float r = 34.0f;
    float a = deg2rad(im.menuOrbit);
    Vec3 center{0.0f, im.level.groundAt(0, 0) + 2.4f, 0.0f};
    Vec3 eye{ center.x + std::sin(a) * r, center.y + 9.0f + std::sin(a * 0.37f) * 2.2f, center.z + std::cos(a) * r };
    const bool shadowed = shadowPass(im, center, [] {});
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    beginScene3D(im.w, im.h, 44.0f, eye, center);
    im.level.applyLighting();          // ko'rinish matritsasidan KEYIN!
    im.level.drawSky(eye);
    if (shadowed) ShadowMap::get().bindReceive(shadowLevelFor(im));
    Pbr::get().begin(shadowed, shadowLevelFor(im));
    im.level.draw(eye);
    Pbr::get().end();
    if (shadowed) ShadowMap::get().unbindReceive();
}

} // namespace

// ================================================================= App

App& App::get() { static App a; return a; }

const AppConfig& App::config() const { return I().cfg; }
AppConfig& App::mutableConfig()      { return I().cfg; }
AppState App::state() const          { return I().state; }
int   App::width()  const            { return I().w; }
int   App::height() const            { return I().h; }
float App::fps()    const            { return I().fps; }
bool  App::wantsQuit() const         { return I().quit; }
void  App::requestQuit()             { I().quit = true; }

void App::setState(AppState s) {
    Impl& im = I();
    if (im.state == s) return;
    im.state = s;
    im.stateFade = 1.0f;
    Input::get().setMouseLook(s == AppState::Gameplay);
}

void App::applyAudioConfig() {
    const AppConfig& c = I().cfg;
    Audio& a = Audio::get();
    a.setBusVolume(BUS_MASTER,   c.masterVolume);
    a.setBusVolume(BUS_MUSIC,    c.musicVolume);
    a.setBusVolume(BUS_SFX,      c.sfxVolume);
    a.setBusVolume(BUS_VOICE,    c.voiceVolume);
    a.setBusVolume(BUS_AMBIENCE, c.musicVolume * 0.8f);
    VoiceBank::get().setEnabled(c.voiceVolume > 0.01f);
}

void App::applyQualityConfig() {
    const int q = I().cfg.modelQuality;
    SkinnedModel::setSkinRateHz(q >= 2 ? 60.0f : (q == 1 ? 30.0f : 20.0f));
}

void App::applyLanguageConfig() {
    Impl& im = I();
    const std::string& lang = im.cfg.language.empty() ? std::string("uz") : im.cfg.language;
    Loc::get().setLanguage(lang);
    VoiceBank::get().setLanguage(Loc::get().language());
    // Epizod sarlavhasi/metasi ham yangi tilda qayta quriladi
    if (const Episode* e = EpisodeDb::get().byId(im.episodeId)) {
        im.episodeTitle = EpisodeDb::get().title(*e);
        char meta[192];
        std::snprintf(meta, sizeof meta, "%s  ·  %s",
                      e->anchor.gregorian.empty() ? "-" : e->anchor.gregorian.c_str(),
                      e->region.empty() ? "-" : e->region.c_str());
        im.episodeMeta = meta;
    }
    MenuSystem::get().setContextTitle(im.episodeTitle);
    MenuSystem::get().rebuild();
}

void App::applyDisplayConfig() {
    Impl& im = I();
    // VSync
    typedef BOOL (WINAPI *PfnSwapInterval)(int);
    if (PROC pr = wglGetProcAddress("wglSwapIntervalEXT")) {
        auto fn = reinterpret_cast<PfnSwapInterval>(reinterpret_cast<void*>(pr));
        fn(im.cfg.vsync ? 1 : 0);
    }
    // To'liq ekran <-> oynali
    if (!im.hwnd) return;
    static RECT s_windowed = {0, 0, 0, 0};
    const LONG style = GetWindowLong(im.hwnd, GWL_STYLE);
    const bool isFull = ((style & WS_OVERLAPPEDWINDOW) == 0);
    if (im.cfg.fullscreen == isFull) return;

    if (im.cfg.fullscreen) {
        GetWindowRect(im.hwnd, &s_windowed);
        MONITORINFO mi = { sizeof(mi), {0,0,0,0}, {0,0,0,0}, 0 };
        if (GetMonitorInfo(MonitorFromWindow(im.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLong(im.hwnd, GWL_STYLE, (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowPos(im.hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(im.hwnd, GWL_STYLE, (style & ~WS_POPUP) | WS_OVERLAPPEDWINDOW);
        if (s_windowed.right > s_windowed.left) {
            SetWindowPos(im.hwnd, HWND_NOTOPMOST, s_windowed.left, s_windowed.top,
                         s_windowed.right - s_windowed.left,
                         s_windowed.bottom - s_windowed.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        } else {
            SetWindowPos(im.hwnd, HWND_NOTOPMOST, 80, 60, 1280, 800,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
}

const std::string& App::lastEpisode() const { return I().lastEpisode; }

// Kadr piksellarini BGRA holida o'qiydi (GL pastdan yuqoriga beradi -> ag'dariladi)
static bool readFrameBGRA(int w, int h, std::vector<unsigned char>& bgra) {
    if (w <= 0 || h <= 0) return false;
    std::vector<unsigned char> px((size_t)w * (size_t)h * 4u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    bgra.resize((size_t)w * (size_t)h * 4u);
    for (int y = 0; y < h; ++y) {
        const unsigned char* src = px.data() + (size_t)(h - 1 - y) * (size_t)w * 4u;
        unsigned char* dst = bgra.data() + (size_t)y * (size_t)w * 4u;
        for (int x = 0; x < w; ++x) {
            dst[x * 4 + 0] = src[x * 4 + 2];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 2] = src[x * 4 + 0];
            dst[x * 4 + 3] = 255;
        }
    }
    return true;
}

// GDI+ kodlovchi CLSID sini MIME turi bo'yicha topadi
static bool encoderClsid(const wchar_t* mime, CLSID* out) {
    UINT num = 0, sz = 0;
    Gdiplus::GetImageEncodersSize(&num, &sz);
    if (sz == 0) return false;
    std::vector<unsigned char> buf(sz);
    Gdiplus::ImageCodecInfo* info = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    Gdiplus::GetImageEncoders(num, sz, info);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(info[i].MimeType, mime) == 0) { *out = info[i].Clsid; return true; }
    }
    return false;
}

bool App::captureFrameJpeg(const std::string& jpgPath, int quality) const {
    const Impl& im = I();
    std::vector<unsigned char> bgra;
    if (!readFrameBGRA(im.w, im.h, bgra)) return false;

    static CLSID jpgClsid;
    static bool  haveClsid = encoderClsid(L"image/jpeg", &jpgClsid);
    if (!haveClsid) return false;

    Gdiplus::Bitmap bmp(im.w, im.h, im.w * 4, PixelFormat32bppARGB, bgra.data());
    if (bmp.GetLastStatus() != Gdiplus::Ok) return false;

    ULONG q = (ULONG)clampf((float)quality, 1.0f, 100.0f);
    Gdiplus::EncoderParameters ep;
    ep.Count = 1;
    ep.Parameter[0].Guid           = Gdiplus::EncoderQuality;
    ep.Parameter[0].Type           = Gdiplus::EncoderParameterValueTypeLong;
    ep.Parameter[0].NumberOfValues = 1;
    ep.Parameter[0].Value          = &q;

    wchar_t wpath[512] = {0};
    if (MultiByteToWideChar(CP_UTF8, 0, jpgPath.c_str(), -1, wpath, 511) <= 0) return false;
    std::string dir = jpgPath;
    const size_t slash = dir.find_last_of("/\\");
    if (slash != std::string::npos) { dir.resize(slash); CreateDirectoryA(dir.c_str(), nullptr); }

    return bmp.Save(wpath, &jpgClsid, &ep) == Gdiplus::Ok;
}

bool App::captureScreenshot(const std::string& pngPath) const {
    const Impl& im = I();
    const int w = im.w, h = im.h;
    if (w <= 0 || h <= 0) return false;

    std::vector<unsigned char> px((size_t)w * (size_t)h * 4u);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());

    // GL pastdan yuqoriga o'qiydi -> ag'daramiz va RGBA -> BGRA ga o'tkazamiz
    std::vector<unsigned char> bgra((size_t)w * (size_t)h * 4u);
    for (int y = 0; y < h; ++y) {
        const unsigned char* src = px.data() + (size_t)(h - 1 - y) * (size_t)w * 4u;
        unsigned char* dst = bgra.data() + (size_t)y * (size_t)w * 4u;
        for (int x = 0; x < w; ++x) {
            dst[x * 4 + 0] = src[x * 4 + 2];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 2] = src[x * 4 + 0];
            dst[x * 4 + 3] = 255;
        }
    }

    // PNG kodlovchi CLSID
    static const CLSID kPngClsid =
        { 0x557cf406, 0x1a04, 0x11d3, { 0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e } };

    Gdiplus::Bitmap bmp(w, h, w * 4, PixelFormat32bppARGB, bgra.data());
    if (bmp.GetLastStatus() != Gdiplus::Ok) return false;

    wchar_t wpath[512] = {0};
    if (MultiByteToWideChar(CP_UTF8, 0, pngPath.c_str(), -1, wpath, 511) <= 0) return false;
    // Papkani yaratib qo'yamiz
    std::string dir = pngPath;
    const size_t slash = dir.find_last_of("/\\");
    if (slash != std::string::npos) { dir.resize(slash); CreateDirectoryA(dir.c_str(), nullptr); }

    return bmp.Save(wpath, &kPngClsid, nullptr) == Gdiplus::Ok;
}

bool App::saveConfig(const std::string& path) const {
    const AppConfig& c = I().cfg;
    try {
        nlohmann::json j;
        j["language"]      = c.language;
        j["master_volume"] = c.masterVolume;
        j["music_volume"]  = c.musicVolume;
        j["sfx_volume"]    = c.sfxVolume;
        j["voice_volume"]  = c.voiceVolume;
        j["subtitles"]     = c.subtitles;
        j["model_quality"] = c.modelQuality;
        j["show_fps"]      = c.showFps;
        j["show_hud"]      = c.showHud;
        j["show_hints"]    = c.showHints;
        j["fullscreen"]    = c.fullscreen;
        j["vsync"]         = c.vsync;
        j["last_episode"]  = I().lastEpisode;
        j["width"]         = c.width;
        j["height"]        = c.height;
        CreateDirectoryA("saves", nullptr);
        std::ofstream f(path);
        if (!f) return false;
        f << j.dump(2);
        return true;
    } catch (...) { return false; }
}

bool App::loadConfig(const std::string& path) {
    AppConfig& c = I().cfg;
    try {
        std::ifstream f(path);
        if (!f) return false;
        nlohmann::json j;
        f >> j;
        c.language     = j.value("language", c.language);
        c.masterVolume = j.value("master_volume", c.masterVolume);
        c.musicVolume  = j.value("music_volume",  c.musicVolume);
        c.sfxVolume    = j.value("sfx_volume",    c.sfxVolume);
        c.voiceVolume  = j.value("voice_volume",  c.voiceVolume);
        c.subtitles    = j.value("subtitles",     c.subtitles);
        c.modelQuality = j.value("model_quality", c.modelQuality);
        c.showFps      = j.value("show_fps",      c.showFps);
        c.showHud      = j.value("show_hud",      c.showHud);
        c.showHints    = j.value("show_hints",    c.showHints);
        c.fullscreen   = j.value("fullscreen",    c.fullscreen);
        c.vsync        = j.value("vsync",         c.vsync);
        I().lastEpisode = j.value("last_episode", std::string());
        return true;
    } catch (...) { return false; }
}

bool App::init(void* hwnd, void* hdc, const AppConfig& cfg) {
    Impl& im = I();
    im.hwnd = (HWND)hwnd;
    im.hdc  = (HDC)hdc;
    im.cfg  = cfg;
    im.w    = cfg.width;
    im.h    = cfg.height;

    std::printf("[App] ishga tushirilmoqda...\n");

    // --- Sozlamalar (avval fayldan, keyin buyruq qatori ustun turadi) ---
    std::string cliLang = cfg.language;
    loadConfig("saves/settings.json");
    if (!cliLang.empty()) im.cfg.language = cliLang;
    Bindings::get().load("saves/bindings.json");
    Progress::get().load("saves/progress.json");

    // --- Lokalizatsiya ---
    Loc& loc = Loc::get();
    loc.loadCsv("localization/ertugrul_loc.csv");
    loc.loadCsv("localization/ui_loc.csv");
    loc.loadCsv("localization/episodes_loc.csv");
    loc.loadCsv("localization/cutscene_loc.csv");
    loc.setLanguage(im.cfg.language.empty() ? "uz" : im.cfg.language);
    std::printf("[Loc] %d kalit yuklandi, til = %s\n", (int)loc.keyCount(), loc.language().c_str());

    // --- Grafika ---
    Texture::initImaging();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // --- Shriftlar (DPI ga qarab kattaligi) ---
    float k = clampf(im.h / 760.0f, 0.75f, 2.0f);
    im.fDisp .create("Georgia",  (int)(48 * k), true);    // slab-serif o'rnida
    im.fItem .create("Segoe UI", (int)(23 * k), false);
    im.fSmall.create("Segoe UI", (int)(18 * k), false);
    im.fSub  .create("Segoe UI", (int)(23 * k), false);
    im.fHud  .create("Segoe UI", (int)(16 * k), false);
    im.fMono .create("Consolas", (int)(15 * k), false);
    if (!im.fItem.valid()) std::printf("[Font] OGOHLANTIRISH: shrift yaratilmadi\n");

    // --- Ovoz ---
    if (!Audio::get().init(44100))
        std::printf("[Audio] OGOHLANTIRISH: ovoz qurilmasi yo'q, o'yin jim ishlaydi\n");
    Sfx::get().build();          // jang tovushlari protsedural sintez qilinadi
    VoiceBank::get().init();
    VoiceBank::get().setLanguage(loc.language());
    applyAudioConfig();

    // --- Kontent ---
    if (!EpisodeDb::get().load("data/episodes/episodes_v2.json"))
        std::printf("[Episodes] XATO: %s\n", EpisodeDb::get().lastError().c_str());
    std::printf("[Episodes] %d epizod, %d mavsum\n",
                (int)EpisodeDb::get().count(), (int)EpisodeDb::get().seasons().size());
    CutsceneDirector::get().loadDirectory("data/cutscenes");

    // --- Daraja ---
    im.levelId = levelFileExists("oba_valley") ? "oba_valley" : "oba_camp";
    im.level.load(im.levelId);
    {
        Vec3 sp0{0, 0, 0};
        float syaw = 0.0f;
        if (const SpawnPoint* sp = im.level.spawn("player")) { sp0 = sp->pos; syaw = sp->yaw; }
        sp0.y = im.level.groundAt(sp0.x, sp0.z);
        im.spawnPos = sp0;
        im.spawnYaw = syaw;
        im.camYaw = syaw;
    }

    // --- O'yinchi modeli ---
    if (Mesh* m = Mesh::get("assets/models/ottoman/ottoman.obj")) {
        im.playerModel.init(m);
        std::printf("[Model] o'yinchi: %d uchburchak\n", (int)m->triangleCount());
    } else {
        im.playerModel.init(Mesh::unitCylinder(12));
        std::printf("[Model] OGOHLANTIRISH: ottoman.obj yo'q, zaxira shakl ishlatildi\n");
    }
    applyQualityConfig();

    // --- Parkur dunyosi va AC uslubidagi personaj boshqaruvi ---
    im.phys.build(im.level);
    im.player.init(&im.playerModel, &im.phys);
    im.player.reset(im.spawnPos, im.spawnYaw);
    std::printf("[Fizika] %d to'qnashuv qutisi\n", (int)im.phys.boxCount());

    // --- Menyu ---
    MenuSystem& menu = MenuSystem::get();
    menu.init(&im.fDisp, &im.fItem, &im.fSmall, &im.fMono);
    if (im.lastEpisode.empty()) im.lastEpisode = Progress::get().lastEpisode();
    menu.setContinueEpisode(im.lastEpisode);
    menu.onStartEpisode  = [this](const std::string& id) { startEpisode(id); };
    menu.onResume        = [this]() { MenuSystem::get().setScreen(MenuScreen::None); setState(AppState::Gameplay); };
    menu.onQuit          = [this]() { App::get().saveConfig("saves/settings.json"); requestQuit(); };
    menu.onReturnToMenu  = [this]() { returnToMenu(); };

    im.state = AppState::Boot;
    menu.setScreen(MenuScreen::Splash);
    im.initialized = true;

    if (!im.cfg.startLevel.empty()) {
        enterLevel(im.cfg.startLevel);
    } else if (!im.cfg.startEpisode.empty()) {
        startEpisode(im.cfg.startEpisode);
    } else if (im.cfg.skipMenu) {
        const auto& eps = EpisodeDb::get().all();
        if (!eps.empty()) startEpisode(eps[0].id);
    }
    return true;
}

void App::shutdown() {
    Impl& im = I();
    if (!im.initialized) return;
    saveConfig("saves/settings.json");
    Bindings::get().save("saves/bindings.json");
    Progress::get().save("saves/progress.json");
    CutsceneDirector::get().stop();
    VoiceBank::get().shutdown();
    Audio::get().shutdown();
    im.fDisp.destroy(); im.fItem.destroy(); im.fSmall.destroy();
    im.fSub.destroy(); im.fHud.destroy(); im.fMono.destroy();
    im.level.destroy();
    Mesh::clearCache();
    Texture::clearCache();
    Texture::shutdownImaging();
    im.initialized = false;
    std::printf("[App] yopildi\n");
}

void App::resize(int w, int h) {
    Impl& im = I();
    im.w = std::max(1, w);
    im.h = std::max(1, h);
    glViewport(0, 0, im.w, im.h);
}

void App::startEpisode(const std::string& episodeId) {
    Impl& im = I();
    const Episode* e = EpisodeDb::get().byId(episodeId);
    im.episodeId = episodeId;
    im.episodeTitle = e ? EpisodeDb::get().title(*e) : episodeId;
    im.episodeMeta.clear();
    if (e) {
        char meta[192];
        std::snprintf(meta, sizeof meta, "%s  ·  %s",
                      e->anchor.gregorian.empty() ? "-" : e->anchor.gregorian.c_str(),
                      e->region.empty() ? "-" : e->region.c_str());
        im.episodeMeta = meta;
    }
    im.lastEpisode = episodeId;
    MenuSystem::get().setContinueEpisode(episodeId);
    MenuSystem::get().setContextTitle(im.episodeTitle);
    MenuSystem::get().setScreen(MenuScreen::None);
    im.loadTimer = 0.0f;
    setState(AppState::Loading);
}

void App::enterLevel(const std::string& levelId) {
    Impl& im = I();
    im.levelId = levelId;
    im.level.load(levelId);
    im.phys.build(im.level);
    Vec3 sp0{0, 0, 0};
    float syaw = 0.0f;
    if (const SpawnPoint* sp = im.level.spawn("player")) { sp0 = sp->pos; syaw = sp->yaw; }
    sp0.y = im.level.groundAt(sp0.x, sp0.z);
    im.spawnPos = sp0; im.spawnYaw = syaw;
    im.player.reset(sp0, syaw);
    im.camYaw = syaw;
    im.episodeTitle = im.level.displayName().empty() ? levelId : im.level.displayName();
    im.episodeMeta.clear();
    MenuSystem::get().setScreen(MenuScreen::None);
    setState(AppState::Gameplay);
    std::printf("[Daraja] %s -> %d to'qnashuv qutisi\n", levelId.c_str(), (int)im.phys.boxCount());
}

void App::returnToMenu() {
    Impl& im = I();
    Encounter::get().stop();
    CutsceneDirector::get().stop();
    VoiceBank::get().stopAll();
    Audio::get().stopBus(BUS_VOICE);
    im.episodeId.clear();
    im.episodeTitle.clear();
    MenuSystem::get().setScreen(MenuScreen::Main);
    setState(AppState::MainMenu);
}

// --------------------------------------------------------------- yangilash

// Film uchun kamera boshqaruvi: qiyalik va masofa. Manfiy qiymat = tegilmasin.
// Joriy darajaga qayta kirish (film sayohatida segmentlar orasida).
// Ilgari sayohat "sogut_village" ni QATTIQ yozib qo'ygan edi va yangi
// xaritada yarim yo'lda eski qishloqqa o'tib ketardi.
// FILM REJIMI uchun: o'yinchining sog'lig'ini ushlab turadi.
// Trailer uchun uzun, uzluksiz jang kerak — aks holda o'yinchi 30 soniyada
// halok bo'lib, videoning yarmi yakuniy ekran bo'lib qoladi.
// Bu FAQAT --film bilan yozib olishda ishlatiladi, o'yin balansiga tegmaydi.
void App::filmSustain(float minPct) {
    Impl& im = I();
    if (!im.initialized || im.state != AppState::Gameplay) return;
    Vitals& v = im.player.vitals;
    const float floorHp = v.healthMax * clampf(minPct, 0.1f, 1.0f);
    if (v.health < floorHp) v.health = floorHp;
    if (v.breath < v.breathMax * 0.45f) v.breath = v.breathMax * 0.45f;
    if (v.posture > v.postureMax * 0.80f) v.posture = v.postureMax * 0.55f;
}

void App::respawnHere() {
    Impl& im = I();
    if (!im.initialized) return;
    enterLevel(im.levelId);
}

void App::filmCamera(float pitchDeg, float dist) {
    Impl& im = I();
    if (!im.initialized) return;
    if (pitchDeg > -900.0f) im.camPitch = clampf(pitchDeg, -35.0f, 70.0f);
    if (dist > 0.0f) { im.camDistTarget = clampf(dist, 2.0f, 30.0f); im.camDist = im.camDistTarget; }
}

void App::nudgeCamYaw(float deltaDeg) {
    Impl& im = I();
    if (!im.initialized) return;
    im.camYaw = wrapAngleDeg(im.camYaw + deltaDeg);
}

void App::update(float dt) {
    Impl& im = I();
    if (!im.initialized) return;
    dt = clampf(dt, 0.0f, 0.1f);          // sakrashlarni cheklaymiz
    im.lastDt = clampf(dt, 1.0f / 480.0f, 0.1f);
    ++im.frameCount;

    im.fpsAccum += dt; ++im.fpsFrames;
    if (im.fpsAccum >= 0.35f) {
        im.fps = im.fpsFrames / im.fpsAccum;
        im.fpsAccum = 0.0f; im.fpsFrames = 0;
    }
    im.stateFade = std::max(0.0f, im.stateFade - dt * 2.2f);

    Audio::get().update();
    Input& in = Input::get();
    MenuSystem& menu = MenuSystem::get();

    switch (im.state) {

    case AppState::Boot:
        menu.update(dt, im.w, im.h);
        if (menu.screen() == MenuScreen::Language)  setState(AppState::Language);
        else if (menu.screen() == MenuScreen::Main) setState(AppState::MainMenu);
        break;

    case AppState::Language:
        menu.update(dt, im.w, im.h);
        if (menu.screen() == MenuScreen::Main) {
            Loc::get().setLanguage(im.cfg.language);
            VoiceBank::get().setLanguage(im.cfg.language);
            menu.rebuild();
            setState(AppState::MainMenu);
        }
        break;

    case AppState::MainMenu:
    case AppState::EpisodeSelect:
        menu.update(dt, im.w, im.h);
        VoiceBank::get().setLanguage(Loc::get().language());
        break;

    case AppState::Loading: {
        im.loadTimer += dt;
        if (im.loadTimer > 0.9f) {
            const Episode* e = EpisodeDb::get().byId(im.episodeId);

            const CutScene* cs = nullptr;
            if (e) cs = CutsceneDirector::get().findForEpisode(e->id);
            if (!cs) cs = CutsceneDirector::get().find("generic_intro");

            // Epizodga mos daraja va kun vaqtini qo'llaymiz
            std::string want = pickLevel(e, cs);
            if (want != im.levelId) {
                im.levelId = want;
                im.level.load(im.levelId);
                im.phys.build(im.level);          // parkur qutilarini qayta quramiz
            }
            if (cs && !cs->timeOfDay.empty()) im.level.applyTimeOfDay(cs->timeOfDay, cs->weather);
            else if (e)                       im.level.applyTimeOfDay(e->timeOfDay, e->weather);

            if (cs && CutsceneDirector::get().playScene(*cs)) {
                setState(AppState::Cutscene);
            } else {
                Vec3 sp0 = im.spawnPos;
                float syaw = im.spawnYaw;
                if (const SpawnPoint* sp = im.level.spawn("player")) { sp0 = sp->pos; syaw = sp->yaw; }
                sp0.y = im.level.groundAt(sp0.x, sp0.z);
                im.player.reset(sp0, syaw);
                im.camYaw = syaw;
                if (const Episode* ep = EpisodeDb::get().byId(im.episodeId))
                    Encounter::get().begin(*ep, im.level, im.phys, im.player);
                setState(AppState::Gameplay);
            }
        }
        break;
    }

    case AppState::Cutscene: {
        CutsceneDirector& cd = CutsceneDirector::get();
        if (Bindings::get().pressed(Action::SkipScene)) cd.skip();
        if (Bindings::get().pressed(Action::Advance) || in.navAccept() || in.mousePressed(0)) cd.advance();
        cd.update(dt);
        if (!cd.isPlaying()) {
            VoiceBank::get().stopAll();
            // Cutscene o'z kun vaqtini (masalan "dawn") o'rnatgan bo'lishi mumkin —
            // o'yin qismida epizodning haqiqiy kun vaqtiga qaytamiz.
            if (const Episode* ep = EpisodeDb::get().byId(im.episodeId))
                im.level.applyTimeOfDay(ep->timeOfDay, ep->weather);
            Vec3 sp0 = im.spawnPos;
            float syaw = im.spawnYaw;
            if (const SpawnPoint* sp = im.level.spawn("player")) { sp0 = sp->pos; syaw = sp->yaw; }
            sp0.y = im.level.groundAt(sp0.x, sp0.z);
            im.player.reset(sp0, syaw);
            im.camYaw = syaw;
            // Ochilish sahnasi tugadi — endi EPIZOD JANGI boshlanadi
            if (const Episode* ep = EpisodeDb::get().byId(im.episodeId))
                Encounter::get().begin(*ep, im.level, im.phys, im.player);
            setState(AppState::Gameplay);
        }
        break;
    }

    case AppState::Gameplay: {
        const Bindings& kb = Bindings::get();
        if (kb.pressed(Action::Pause)) {
            MenuSystem::get().setContextTitle(im.episodeTitle);
            menu.setScreen(MenuScreen::Pause);
            setState(AppState::Paused);
            break;
        }
        // Bilge Ko'z (Eagle Vision) — bosilganda almashadi
        if (kb.pressed(Action::BilgeGoz)) im.eagle = !im.eagle;
        im.eagleT = damp(im.eagleT, im.eagle ? 1.0f : 0.0f, 9.0f, dt);

        // --- kamera ---
        im.camYaw   -= in.mouseDeltaX() * 0.16f;
        im.camPitch  = clampf(im.camPitch + in.mouseDeltaY() * 0.14f, -22.0f, 62.0f);
        if (in.wheel() != 0) im.camDistTarget = clampf(im.camDistTarget - in.wheel() * 0.6f, 2.2f, 11.0f);

        // --- AC uslubidagi personaj kirishi ---
        CharacterInput ci;
        {
            float rx = (kb.down(Action::MoveRight)   ? 1.0f : 0.0f) -
                       (kb.down(Action::MoveLeft)    ? 1.0f : 0.0f);
            float ry = (kb.down(Action::MoveForward) ? 1.0f : 0.0f) -
                       (kb.down(Action::MoveBack)    ? 1.0f : 0.0f);
            // Diagonalni NORMALLASHTIRAMIZ (clamp emas): aks holda diagonal ham,
            // to'g'ri ham wishLen = 1.0 beradi va walkSpeed_ hech qachon ishlamaydi.
            const float m = std::sqrt(rx * rx + ry * ry);
            if (m > 1.0f) { rx /= m; ry /= m; }
            im.inSmX = damp(im.inSmX, rx, 22.0f, dt);   // halflife ~31 ms
            im.inSmY = damp(im.inSmY, ry, 22.0f, dt);
            ci.move.x = (std::fabs(im.inSmX) < 0.02f) ? 0.0f : im.inSmX;
            ci.move.y = (std::fabs(im.inSmY) < 0.02f) ? 0.0f : im.inSmY;
        }
        ci.camYaw      = im.camYaw;
        ci.camPitch    = im.camPitch;
        ci.highProfile = kb.down(Action::Run);
        ci.walk        = kb.down(Action::Walk) && !kb.down(Action::Run);
        ci.parkourUp   = kb.down(Action::ParkourUp);
        ci.parkourDown = kb.down(Action::ParkourDown);
        ci.dodge       = kb.pressed(Action::Dodge);
        ci.crouch      = kb.pressed(Action::Crouch);
        ci.interact    = kb.pressed(Action::Interact);
        ci.assassinate = kb.pressed(Action::Takedown);

        // --- Jang kirishlari ---
        ci.attackHeavy = kb.pressed(Action::LightAttack) && kb.down(Action::Run);
        ci.attackLight = kb.pressed(Action::LightAttack) && !ci.attackHeavy;
        if (kb.pressed(Action::HeavyAttack)) { ci.attackHeavy = true; ci.attackLight = false; }
        ci.block  = kb.down(Action::Parry);
        ci.parry  = kb.pressed(Action::Parry);
        ci.kick   = kb.pressed(Action::Kick);
        ci.bow    = kb.down(Action::Bow);
        ci.lockOn = kb.pressed(Action::LockOn);

        Encounter& enc = Encounter::get();

        // Nishonni qulflash / almashtirish
        if (ci.lockOn) {
            if (enc.lockTarget()) enc.cycleLockTarget();
            else                  enc.cycleLockTarget();
        }
        if (Enemy* tgt = enc.lockTarget()) {
            if (tgt->alive()) {
                im.lockPos = tgt->position() + Vec3{0.0f, 1.1f, 0.0f};
                im.player.setLockTarget(&im.lockPos);
            } else {
                enc.clearLockTarget();
                im.player.setLockTarget(nullptr);
            }
        } else {
            im.player.setLockTarget(nullptr);
        }

        // Yashirin o'ldirish va yakunlovchi zarba — ikkalasi ham yaqin dushmanni talab qiladi
        // Nishonlab turganda LMB o'q otadi, yakunlovchi zarba emas
        if (!im.player.busy() && !im.player.aiming()
            && enc.state() == EncounterState::Fighting) {
            const Vec3 pp = im.player.position();
            if (ci.assassinate) {
                for (Enemy& e : enc.enemies().all()) {
                    if (e.assassinable(pp, im.player.yaw())) {
                        im.player.playAssassinate(e.position());
                        ci.assassinate = false;
                        break;
                    }
                }
            } else if (ci.attackLight) {
                // Massivdagi BIRINCHI staggered dushman emas — eng yaqin, oldinda
                // turgan va stagger oynasi yetarli qolgan nishon. Ilgari burchak
                // ham, staggerT ham tekshirilmasdi: o'yinchi ORQASIDAGI dushmanga
                // "yakunlovchi zarba" berishi mumkin edi, va stagger tugab qolsa
                // 999 zarar dushmanning invulnT oynasiga tushib jimgina yo'qolardi.
                const int vi = enc.enemies().findExecutable(pp, im.player.yaw(), 2.2f, 55.0f);
                if (vi >= 0) {
                    im.player.playExecute(vi, enc.enemies().all()[(size_t)vi].position());
                    ci.attackLight = false;
                }
            }
        }

        // SUKUNAT: Iymon 76+ bo'lsa mukammal parry yoki yakunlovchi zarba
        // vaqtni 1.2 s ga 0.45x sekinlashtiradi. Kiritish va kamera NORMAL
        // tezlikda qoladi - faqat dunyo sekinlashadi (Ghost of Tsushima usuli).
        // Muzlash HAQIQIY dt bilan so'nishi SHART — aks holda o'zi sekinlashtirgan
        // vaqt bilan kamayadi va cheksiz cho'ziladi.
        enc.tickFeedback(dt);
        Sfx::get().setListener(im.player.position());
        const float gdt = dt * clampf(im.player.faith.timeScale(), 0.2f, 1.0f)
                             * enc.timeScale();
        im.player.update(ci, gdt);
        enc.update(gdt);

        // Holat o'tishlari
        if (enc.state() == EncounterState::Failed) {
            Progress::get().addDeath(im.episodeId);
            Progress::get().save("saves/progress.json");
            im.endSel = 0; im.endT = 0.0f;
            setState(AppState::Failed);
            break;
        }
        if (enc.state() == EncounterState::Cleared) {
            Progress::get().markCompleted(im.episodeId, enc.result());
            Progress::get().setLastEpisode(im.episodeId);
            Progress::get().save("saves/progress.json");
            im.endSel = 0; im.endT = 0.0f;
            setState(AppState::EpisodeComplete);
            break;
        }

        // Holatga qarab kamera masofasi
        im.camDistTarget = clampf(im.camDistTarget, 2.2f, 11.0f);
        const float hint = im.player.cameraDistanceHint();
        im.camDist = damp(im.camDist, lerpf(im.camDistTarget, hint, 0.55f), 8.0f, dt);

        // Diagnostika
        {
            static const bool moveLog = (std::getenv("ERT_MOVE_LOG") != nullptr);
            if (moveLog) {
                // OYOQ SIRG'ALISHINING SONLI MEZONI.
                // S_kut = qadam geometriyasi kutgan sikl uzunligi (m)
                // S_haq = dunyoda bosib o'tilgan masofa / faza siljishi
                // SLIP  = ikkisining farqi. Qabul mezoni: |SLIP| < 3%.
                // Eski (sinusoidal + vaqt fazali) kodda bu ~ +185% edi.
                static float acc = 0.0f, lastPh = 0.0f, dsAcc = 0.0f, dphAcc = 0.0f;
                acc += dt;
                const float ph = im.playerModel.stridePhase();
                float dph = ph - lastPh;
                dph -= std::floor(dph);
                if (dph > 0.5f) dph -= 1.0f;
                lastPh = ph;
                dsAcc  += im.player.debugDs();
                dphAcc += std::fabs(dph);
                // O'lchov faqat YER lokomotsiyasida ma'noli. Mantle/Fall/JumpUp da
                // ildiz vertikal ko'chadi va nisbat bema'ni katta chiqadi.
                const MoveState ms = im.player.state();
                const bool locoState = (ms == MoveState::Walk || ms == MoveState::Jog ||
                                        ms == MoveState::Sprint || ms == MoveState::CrouchWalk);
                if (!locoState) { dsAcc = 0.0f; dphAcc = 0.0f; }
                if (acc > 0.5f && locoState && im.player.speed() > 0.05f) {
                    acc = 0.0f;
                    const float v    = im.player.speed();
                    float Sexp = clampf(0.97f + 0.35f * v, 0.75f, 3.40f);
                    // Cho'kkalab yurishda qadam 0.62x qisqaradi (Skin.cpp update())
                    if (im.player.state() == MoveState::CrouchWalk) Sexp *= 0.62f;
                    const float Sact = (dphAcc > 1e-4f) ? (dsAcc / dphAcc) : 0.0f;
                    const Vec3  p    = im.player.position();
                    std::printf("[loco] v=%.2f S_kut=%.3f S_haq=%.3f SLIP=%+6.1f%% "
                                "qadam/s=%.2f holat=%-10s pos=(%.1f,%.1f,%.1f) profil=%s\n",
                                v, Sexp, Sact,
                                (Sexp > 1e-4f && Sact > 1e-4f) ? 100.0f * (Sact / Sexp - 1.0f) : 0.0f,
                                (Sact > 1e-4f) ? 2.0f * v / Sact : 0.0f,
                                moveStateName(im.player.state()), p.x, p.y, p.z,
                                im.player.profile() == Profile::High ? "yuqori" : "past");
                    dsAcc = 0.0f; dphAcc = 0.0f;
                }
            }
        }
        break;
    }

    case AppState::Paused:
        menu.update(dt, im.w, im.h);
        if (menu.screen() == MenuScreen::None) setState(AppState::Gameplay);
        break;

    // ------------------------------------------------ HALOK BO'LDINGIZ
    case AppState::Failed: {
        im.endT += dt;
        if (im.endT < 0.9f) break;                 // qisqa pauza — tugma bosilib ketmasin
        if (in.navDown()) im.endSel = (im.endSel + 1) % 3;
        if (in.navUp())   im.endSel = (im.endSel + 2) % 3;
        if (in.navAccept()) {
            Encounter& enc = Encounter::get();
            if (im.endSel == 0) {                  // nazorat nuqtasidan
                enc.restartFromCheckpoint();
                setState(AppState::Gameplay);
            } else if (im.endSel == 1) {           // epizodni boshidan
                enc.restartEpisode();
                setState(AppState::Gameplay);
            } else {
                returnToMenu();
            }
        }
        if (in.navCancel()) returnToMenu();
        break;
    }

    // ------------------------------------------------ EPIZOD BAJARILDI
    case AppState::EpisodeComplete: {
        im.endT += dt;
        if (im.endT < 0.9f) break;
        if (in.navDown()) im.endSel = (im.endSel + 1) % 2;
        if (in.navUp())   im.endSel = (im.endSel + 1) % 2;
        if (in.navAccept()) {
            if (im.endSel == 0) {
                // Keyingi epizod
                const Episode* cur = EpisodeDb::get().byId(im.episodeId);
                std::string next;
                if (cur && !cur->unlocks.empty()) next = cur->unlocks[0];
                if (next.empty() && cur) {
                    const size_t i = (size_t)cur->globalIndex + 1;
                    if (const Episode* e2 = EpisodeDb::get().byIndex(i)) next = e2->id;
                }
                if (!next.empty()) startEpisode(next);
                else               returnToMenu();
            } else {
                returnToMenu();
            }
        }
        if (in.navCancel()) returnToMenu();
        break;
    }

    default:
        break;
    }

    if (menu.screen() == MenuScreen::EpisodeSelect && im.state == AppState::MainMenu) setState(AppState::EpisodeSelect);
    if (menu.screen() == MenuScreen::Main && im.state == AppState::EpisodeSelect)     setState(AppState::MainMenu);
}

// --------------------------------------------------------------- chizish

void App::render() {
    Impl& im = I();
    if (!im.initialized) return;

    switch (im.state) {

    case AppState::Loading:
        glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawLoading(im);
        return;

    case AppState::Cutscene: {
        CutsceneDirector& cd = CutsceneDirector::get();
        Vec3 eye = cd.cameraPos(), look = cd.cameraLook();
        // Kamera YER OSTIGA tushmasin. Cutscene kalitlari tekis maydon uchun
        // yozilgan (y ~ 2-3 m mutlaq); tepalikli darajalarda (forest_pass)
        // kamera tepalik ichiga kirib, ekranda faqat o't ko'rinardi (EP010).
        {
            const float ge = im.level.groundAt(eye.x,  eye.z);
            const float gl = im.level.groundAt(look.x, look.z);
            if (std::isfinite(ge) && eye.y  < ge + 1.2f) eye.y  = ge + 1.2f;
            if (std::isfinite(gl) && look.y < gl + 0.9f) look.y = gl + 0.9f;
        }
        // Kamera yer ostiga tushib ketmasin (relyef tepaliklarida muhim)
        const float minEyeY = im.level.groundAt(eye.x, eye.z) + 0.6f;
        if (eye.y < minEyeY) eye.y = minEyeY;
        im.level.terrain().clampToBounds(eye, 2.0f);
        const bool shadowed = shadowPass(im, look, [&] {
            for (const auto& a : cd.actors()) {
                if (!a.model || !a.def) continue;
                Vec3 p = a.pos;
                p.y = im.level.groundAt(p.x, p.z);
                a.model->draw(p, a.yaw, a.def->scale);
            }
        });
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        beginScene3D(im.w, im.h, cd.cameraFov(), eye, look);
        im.level.applyLighting();      // ko'rinish matritsasidan KEYIN!
        im.level.drawSky(eye);
        if (shadowed) ShadowMap::get().bindReceive(shadowLevelFor(im));
        Pbr::get().begin(shadowed, shadowLevelFor(im));
        im.level.draw(eye);

        for (const auto& a : cd.actors()) {
            if (!a.model || !a.def) continue;
            Vec3 p = a.pos;
            p.y = im.level.groundAt(p.x, p.z);
            if (!shadowed) drawBlobShadow(im.level, p, 0.75f, 0.42f);
            pushTintMaterial(a.def->tint);
            a.model->draw(p, a.yaw, a.def->scale);
            popTintMaterial();
        }
        Pbr::get().end();
        if (shadowed) ShadowMap::get().unbindReceive();

        begin2D(im.w, im.h);
        drawLetterbox(im.w, im.h, cd.letterbox());
        drawSubtitle(im, cd.speakerName(), cd.subtitle());
        if (im.fHud.valid()) {
            float blink = 0.45f + 0.35f * std::sin(cd.time() * 2.4f);
            im.fHud.draw(T("ui.cutscene.skip_hint"), (float)im.w - 26.0f, (float)im.h - 40.0f,
                         0.85f, 0.83f, 0.78f, blink, TextAlign::Right);
        }
        if (cd.fade() > 0.001f) drawFullscreenFade(im.w, im.h, 0, 0, 0, cd.fade());
        end2D();
        return;
    }

    case AppState::Gameplay:
    case AppState::Paused: {
        // --- uchinchi shaxs kamerasi (rekvizitlarga urilmasligi uchun oldinga suriladi) ---
        Vec3 focus = im.player.cameraFocus();
        float py = deg2rad(im.camPitch);
        Vec3 back = dirFromYaw(im.camYaw) * -1.0f;
        Vec3 desired = focus + Vec3{ back.x * std::cos(py), std::sin(py), back.z * std::cos(py) } * im.camDist;
        // Kamon: kamera o'ng yelka ustiga siljiydi (o'q yo'li ochiq qolsin)
        {
            const float sh = im.player.cameraShoulderOffset();
            if (sh > 0.001f) {
                const Vec3 f = dirFromYaw(im.camYaw);
                const Vec3 r{-f.z, 0.0f, f.x};
                desired.x += r.x * sh;  desired.z += r.z * sh;
                focus.x   += r.x * sh;  focus.z   += r.z * sh;
            }
        }
        float minY = im.level.groundAt(desired.x, desired.z) + 0.9f;
        if (desired.y < minY) desired.y = minY;
        // Ilgari bu yerda qattiq 1/60 turardi: 144 Gts da kamera 2.4x QATTIQ,
        // 30 Gts da 2x SUST ergashardi - bu ham "yurish yoqmayapti" hissiga qo'shilardi.
        im.camSmooth = dampV(im.camSmooth, desired, 16.0f, im.lastDt);

        // --- Zarba turtkisi: kamerani qisqa muddatga silkitadi ---
        // MUHIM: silkinish camSmooth ga YOZILMAYDI. Agar yozilsa, keyingi kadrda
        // dampV silkigan joydan boshlab tortadi va turtkilar bir-biriga qo'shilib
        // kamerani asta-sekin joyidan surib yuborardi. Shuning uchun u faqat
        // CHIZISH uchun hisoblanadigan alohida siljish.
        // Ikki o'q, to'rt chastota (34/19 va 27/43 Gts) — takrorlanuvchi tebranish
        // emas, tartibsiz turtki bo'lib sezilsin.
        Vec3 eye = im.camSmooth;
        {
            const float sk = Encounter::get().cameraShake();
            if (sk > 0.004f) {
                im.shakePhase += im.lastDt;
                if (im.shakePhase > 100.0f) im.shakePhase -= 100.0f;
                const float t = im.shakePhase;
                const float a = sk * sk * 0.13f;          // 0.13 m eng katta siljish
                const Vec3 f = dirFromYaw(im.camYaw);
                const Vec3 rt2{-f.z, 0.0f, f.x};
                const float sx = std::sin(TAU * 34.0f * t) * 0.85f
                               + std::sin(TAU * 19.0f * t + 1.7f) * 0.35f;
                const float sy = std::sin(TAU * 27.0f * t + 0.9f) * 0.80f
                               + std::sin(TAU * 43.0f * t + 2.3f) * 0.30f;
                eye.x += rt2.x * sx * a;
                eye.z += rt2.z * sx * a;
                eye.y += sy * a * 0.75f;
                // Nishon nuqtasini TESKARI tomonga surish — silkinish "burchak"
                // bo'lib sezilsin, oddiy siljish emas
                focus.x -= rt2.x * sx * a * 0.35f;
                focus.z -= rt2.z * sx * a * 0.35f;
                focus.y -= sy * a * 0.20f;
            }
        }

        const float playerTint[3] = {1.0f, 0.97f, 0.90f};
        const bool shadowed = shadowPass(im, im.player.position(), [&] {
            EnemyManager& em = Encounter::get().enemies();
            for (Enemy& e : em.all()) {
                if (!e.alive() && e.state() != EnemyState::Dead) continue;
                e.model().draw(e.position(), e.yaw(), enemyStats(e.kind()).scale);
            }
            im.player.draw(1.82f, playerTint);
        });
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Nishonlashda ko'rish maydoni torayadi — "yaqinlashish" hissi
        im.camFov = damp(im.camFov, im.player.cameraFovHint(), 10.0f, im.lastDt);
        beginScene3D(im.w, im.h, clampf(im.camFov, 30.0f, 60.0f), eye, focus);
        im.level.applyLighting();      // ko'rinish matritsasidan KEYIN!
        im.level.drawSky(eye);
        if (shadowed) ShadowMap::get().bindReceive(shadowLevelFor(im));
        Pbr::get().begin(shadowed, shadowLevelFor(im));
        im.level.draw(eye);

        // --- Dushmanlar ---
        {
            EnemyManager& em = Encounter::get().enemies();
            for (Enemy& e : em.all()) {
                if (!e.alive() && e.state() != EnemyState::Dead) continue;
                const EnemyStats& st = enemyStats(e.kind());
                if (!shadowed) drawBlobShadow(im.level, e.position(), 0.72f, e.alive() ? 0.42f : 0.22f);
                pushTintMaterial(st.tint);
                e.model().draw(e.position(), e.yaw(), st.scale);
                popTintMaterial();
            }
        }

        if (!shadowed) drawBlobShadow(im.level, im.player.position(), 0.7f, 0.45f);
        pushTintMaterial(playerTint);
        im.player.draw(1.82f, playerTint);
        popTintMaterial();
        Pbr::get().end();
        if (shadowed) ShadowMap::get().unbindReceive();

        // Maqsad markerlari va dushman ogohlik belgilari
        Encounter::get().draw();
        im.phys.debugDraw();

        // Bilge Ko'z: dunyo so'nadi, parkur geometriyasi yonadi
        if (im.eagleT > 0.01f) {
            begin2D(im.w, im.h);
            drawFullscreenFade(im.w, im.h, 0.04f, 0.07f, 0.09f, 0.72f * im.eagleT);
            end2D();
            drawEagleOverlay(im.phys, im.eagleT);
        }

        begin2D(im.w, im.h);
        drawHud(im);
        drawCombatHud(im);
        end2D();

        if (im.state == AppState::Paused) MenuSystem::get().draw(im.w, im.h);
        return;
    }

    case AppState::Failed:
    case AppState::EpisodeComplete: {
        // Orqada muzlatilgan sahna
        Vec3 focus = im.player.cameraFocus();
        const float pt[3] = {1.0f, 0.97f, 0.90f};
        const bool shadowed = shadowPass(im, im.player.position(), [&] {
            EnemyManager& em = Encounter::get().enemies();
            for (Enemy& e : em.all())
                e.model().draw(e.position(), e.yaw(), enemyStats(e.kind()).scale);
            im.player.draw(1.82f, pt);
        });
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        beginScene3D(im.w, im.h, 46.0f, im.camSmooth, focus);
        im.level.applyLighting();
        im.level.drawSky(im.camSmooth);
        if (shadowed) ShadowMap::get().bindReceive(shadowLevelFor(im));
        Pbr::get().begin(shadowed, shadowLevelFor(im));
        im.level.draw(im.camSmooth);
        {
            EnemyManager& em = Encounter::get().enemies();
            for (Enemy& e : em.all()) {
                const EnemyStats& st = enemyStats(e.kind());
                pushTintMaterial(st.tint);
                e.model().draw(e.position(), e.yaw(), st.scale);
                popTintMaterial();
            }
        }
        pushTintMaterial(pt);
        im.player.draw(1.82f, pt);
        popTintMaterial();
        Pbr::get().end();
        if (shadowed) ShadowMap::get().unbindReceive();

        const Encounter& enc = Encounter::get();
        const EncounterResult& r = enc.result();
        char stats[192];
        if (im.state == AppState::Failed) {
            std::snprintf(stats, sizeof stats, "%s: %d   ·   %s %d/%d",
                          T("ui.fail.deaths").c_str(), r.deaths,
                          T("ui.hud.wave").c_str(), enc.waveIndex() + 1, enc.waveCount());
            drawEndScreen(im, "ui.fail.title", im.episodeTitle.c_str(),
                          kFailItems, 3, im.endSel, T("ui.fail.line"), stats);
        } else {
            std::snprintf(stats, sizeof stats, "%s: %d   ·   %s: %d:%02d",
                          T("ui.done.kills").c_str(), r.kills,
                          T("ui.done.time").c_str(),
                          (int)(r.timeSec / 60.0f), ((int)r.timeSec) % 60);
            std::string body;
            if (const Episode* e = EpisodeDb::get().byId(im.episodeId))
                body = EpisodeDb::get().cliffhanger(*e);
            drawEndScreen(im, "ui.done.title", im.episodeTitle.c_str(),
                          kDoneItems, 2, im.endSel, body, stats);
        }
        return;
    }

    default: {
        // Menyu ekranlari — orqada tirik 3D dunyo
        renderMenuBackdrop(im, 1.0f / 60.0f);
        MenuSystem::get().draw(im.w, im.h);
        return;
    }
    }
}

} // namespace ert
