#pragma once
// Quyosh soyasi xaritasi — SHADERSIZ, fixed-function OpenGL bilan.
//
// Nima uchun bu mumkin: GL_ARB_depth_texture + GL_ARB_shadow (GL 1.4, 2002 yil)
// chuqurlik teksturasini apparat darajasida solishtiradi (COMPARE_R_TO_TEXTURE),
// GL_ARB_multitexture esa uni ikkinchi tekstura birligiga qo'yadi. Tekstura
// koordinatalari EYE_LINEAR texgen orqali quyosh matritsasidan olinadi —
// bitta shader satri ham kerak emas. Bugungi har qanday drayver buni beradi.
//
// O'tim (pass) tartibi:
//   1) begin()  -> quyosh nuqtai nazaridan faqat CHUQURLIK chiziladi
//      (Level::drawCasters + personajlar), end() uni teksturaga nusxalaydi.
//   2) Asosiy sahna: drawSky() dan KEYIN bindReceive(), personajlar chizilgach
//      unbindReceive(). Ikkinchi birlik har pikselni 1.0 (yorug') yoki
//      shadowLevel (soyada) bilan ko'paytiradi.
//
// Cheklovlar (halol): xarita o'lchami oyna balandligidan oshmaydi (orqa bufer
// orqali nusxalanadi, FBO yo'q); radius ~34 m — undan uzoqdagi narsalar soyasiz.
#include "ertugrul/core/Math.h"

namespace ert {

class ShadowMap {
public:
    static ShadowMap& get();

    // Kengaytmalarni tekshiradi va teksturani yaratadi. GL konteksti kerak.
    // Bir marta chaqiriladi (enabled() buni o'zi qiladi).
    bool init();
    bool available() const { return avail_; }
    // ERT_NO_SHADOWMAP=1 bilan o'chiriladi (eski yumshoq disklar qaytadi)
    bool enabled();

    // 1-o'tim. sunDir — quyoshga YO'NALGAN birlik vektor (Level::sky().sunDir).
    // Quyosh ufq ostida bo'lsa false qaytaradi (tunda soya yo'q).
    bool begin(const Vec3& sunDir, const Vec3& focus, float radius, int screenW, int screenH);
    void end();

    // 2-o'tim. Joriy MODELVIEW = kamera ko'rinishi bo'lishi SHART (texgen
    // EYE_LINEAR shu matritsa bilan hisoblanadi). shadowLevel: 0.35..0.7.
    void bindReceive(float shadowLevel);
    void unbindReceive();

    int  mapSize() const { return size_; }
    bool receiving() const { return receiving_; }
    bool ambientExt() const { return hasAmbient_; }

private:
    bool  inited_ = false, avail_ = false, hasAmbient_ = false, hasCombine_ = false;
    bool  active_ = false, receiving_ = false;
    unsigned tex_ = 0, white_ = 0;
    int   size_ = 0;
    int   savedVp_[4] = {0, 0, 0, 0};
    float lightMat_[16] = {0};      // bias * proj * view (ustun-major)
};

} // namespace ert
