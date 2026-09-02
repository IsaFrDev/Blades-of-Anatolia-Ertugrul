#include "ertugrul/gfx/ShadowMap.h"
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// MinGW ning GL/gl.h faqat 1.1 — kerakli konstantalarni o'zimiz e'lon qilamiz.
#ifndef GL_TEXTURE0_ARB
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2_ARB                   0x84C2
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24              0x81A6
#endif
#ifndef GL_TEXTURE_COMPARE_MODE_ARB
#define GL_TEXTURE_COMPARE_MODE_ARB       0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB       0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB       0x884E
#endif
#ifndef GL_DEPTH_TEXTURE_MODE_ARB
#define GL_DEPTH_TEXTURE_MODE_ARB         0x884B
#endif
#ifndef GL_TEXTURE_COMPARE_FAIL_VALUE_ARB
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB 0x80BF
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER                0x812D
#endif
#ifndef GL_COMBINE_ARB
#define GL_COMBINE_ARB                    0x8570
#define GL_COMBINE_RGB_ARB                0x8571
#define GL_COMBINE_ALPHA_ARB              0x8572
#define GL_SOURCE0_RGB_ARB                0x8580
#define GL_SOURCE1_RGB_ARB                0x8581
#define GL_SOURCE2_RGB_ARB                0x8582
#define GL_OPERAND0_RGB_ARB               0x8590
#define GL_OPERAND1_RGB_ARB               0x8591
#define GL_OPERAND2_RGB_ARB               0x8592
#define GL_SOURCE0_ALPHA_ARB              0x8588
#define GL_SOURCE1_ALPHA_ARB              0x8589
#define GL_SOURCE2_ALPHA_ARB              0x858A
#define GL_OPERAND0_ALPHA_ARB             0x8598
#define GL_OPERAND1_ALPHA_ARB             0x8599
#define GL_OPERAND2_ALPHA_ARB             0x859A
#define GL_CONSTANT_ARB                   0x8576
#define GL_PRIMARY_COLOR_ARB              0x8577
#define GL_PREVIOUS_ARB                   0x8578
#define GL_INTERPOLATE_ARB                0x8575
#endif

namespace ert {
namespace {

typedef void (APIENTRY* PFN_ActiveTexture)(GLenum);
PFN_ActiveTexture pActiveTexture = nullptr;

bool hasExt(const char* name) {
    const GLubyte* ext = glGetString(GL_EXTENSIONS);
    if (!ext) return false;
    const char* s = reinterpret_cast<const char*>(ext);
    const size_t n = std::strlen(name);
    const char* p = s;
    while ((p = std::strstr(p, name)) != nullptr) {
        const bool l = (p == s) || (p[-1] == ' ');
        const char a = p[n];
        if (l && (a == ' ' || a == '\0')) return true;
        p += n;
    }
    return false;
}

// ustun-major 4x4 ko'paytma: out = a * b
void mul44(const float* a, const float* b, float* out) {
    float r[16];
    for (int c = 0; c < 4; ++c)
        for (int rI = 0; rI < 4; ++rI) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a[k * 4 + rI] * b[c * 4 + k];
            r[c * 4 + rI] = s;
        }
    std::memcpy(out, r, sizeof r);
}

} // namespace

ShadowMap& ShadowMap::get() { static ShadowMap s; return s; }

bool ShadowMap::init() {
    if (inited_) return avail_;
    inited_ = true;
    if (wglGetCurrentContext() == nullptr) { inited_ = false; return false; }

    const bool depthTex = hasExt("GL_ARB_depth_texture");
    const bool shadow   = hasExt("GL_ARB_shadow");
    const bool multi    = hasExt("GL_ARB_multitexture");
    hasAmbient_ = hasExt("GL_ARB_shadow_ambient");
    hasCombine_ = hasExt("GL_ARB_texture_env_combine");
    pActiveTexture = (PFN_ActiveTexture)wglGetProcAddress("glActiveTextureARB");
    if (!pActiveTexture) pActiveTexture = (PFN_ActiveTexture)wglGetProcAddress("glActiveTexture");

    avail_ = depthTex && shadow && multi && pActiveTexture != nullptr
             && (hasAmbient_ || hasCombine_);
    std::printf("[Soya] depth_texture=%d shadow=%d multitexture=%d shadow_ambient=%d "
                "env_combine=%d -> %s\n",
                (int)depthTex, (int)shadow, (int)multi, (int)hasAmbient_, (int)hasCombine_,
                avail_ ? "soya xaritasi YOQILDI" : "mavjud emas, yumshoq disklar");
    if (!avail_) return false;

    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // PCF 2x2 (apparat)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const GLfloat border[4] = {1.0f, 1.0f, 1.0f, 1.0f};   // xaritadan tashqarisi YORUG'
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
    glBindTexture(GL_TEXTURE_2D, 0);
    while (glGetError() != GL_NO_ERROR) {}
    return true;
}

bool ShadowMap::enabled() {
    static const bool off = (std::getenv("ERT_NO_SHADOWMAP") != nullptr);
    if (off) return false;
    if (!inited_) init();
    return avail_;
}

bool ShadowMap::begin(const Vec3& sunDirIn, const Vec3& focus, float radius, int screenW, int screenH) {
    if (!avail_ || active_) return false;
    Vec3 sd = sunDirIn;
    const float L = std::sqrt(sd.x * sd.x + sd.y * sd.y + sd.z * sd.z);
    if (!(L > 1.0e-4f)) return false;
    sd = sd * (1.0f / L);
    if (sd.y < 0.12f) return false;                   // quyosh ufqda/ostida — soya yo'q

    // Xarita o'lchami: orqa buferdan nusxalanadi, shuning uchun oynadan katta bo'lolmaydi
    int s = (screenW < screenH) ? screenW : screenH;
    if (s > 2048) s = 2048;
    if (s < 64) return false;
    if (s != size_) {
        size_ = s;
        glBindTexture(GL_TEXTURE_2D, tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size_, size_, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGetIntegerv(GL_VIEWPORT, savedVp_);
    glViewport(0, 0, size_, size_);

    // Quyosh kamerasi: fokusdan 90 m nariga, ortografik
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-radius, radius, -radius, radius, 1.0, 260.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    const Vec3 eye = focus + sd * 90.0f;
    Vec3 up{0.0f, 1.0f, 0.0f};
    if (std::fabs(sd.y) > 0.98f) up = Vec3{0.0f, 0.0f, 1.0f};
    gluLookAt(eye.x, eye.y, eye.z, focus.x, focus.y, focus.z, up.x, up.y, up.z);

    // Texgen uchun matritsa: bias * P * V
    float P[16], V[16], PV[16];
    glGetFloatv(GL_PROJECTION_MATRIX, P);
    glGetFloatv(GL_MODELVIEW_MATRIX, V);
    mul44(P, V, PV);
    static const float bias[16] = {0.5f, 0, 0, 0,  0, 0.5f, 0, 0,  0, 0, 0.5f, 0,  0.5f, 0.5f, 0.5f, 1.0f};
    mul44(bias, PV, lightMat_);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.4f, 6.0f);                      // "soya husnbuzari"ga qarshi
    glClear(GL_DEPTH_BUFFER_BIT);
    active_ = true;
    return true;
}

void ShadowMap::end() {
    if (!active_) return;
    glBindTexture(GL_TEXTURE_2D, tex_);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, size_, size_);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glViewport(savedVp_[0], savedVp_[1], savedVp_[2], savedVp_[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    active_ = false;
}

void ShadowMap::bindReceive(float shadowLevel) {
    if (!avail_ || size_ == 0 || receiving_) return;
    if (shadowLevel < 0.0f) shadowLevel = 0.0f;
    if (shadowLevel > 1.0f) shadowLevel = 1.0f;

    // Sahna 0-birlikda o'zgarishsiz qoladi (tekstura koordinatalari, relyefning
    // COMBINE x2 rejimi — hammasi avvalgidek). Soya IKKI qo'shimcha birlikda:
    //   1-birlik: soya teksturasi (INTENSITY: rgb=a=t, t = 0 soyada, 1 yorug'da)
    //       RGB   = PREVIOUS                       (rang o'tkaziladi)
    //       ALPHA = t*t + L*(1-t) = L + (1-L)*t    (soya koeffitsiyenti alfada)
    //   2-birlik: 1x1 oq tekstura
    //       RGB   = PREVIOUS.rgb * PREVIOUS.alpha  (rang x koeffitsiyent)
    //       ALPHA = PRIMARY.alpha                  (shaffoflik saqlanadi)
    // Birinchi urinish sahnani 1-birlikka ko'chirgan edi — glTexCoordPointer va
    // glTexCoord2f faqat 0-birlikka boradi, teksturalar (0,0) tekselga yopishib
    // hamma narsa tekis rangga aylangan edi.

    // --- 1: soya ---
    pActiveTexture(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
    // EYE_LINEAR: GL berilgan tekislikni joriy MODELVIEW ning teskarisi bilan
    // ko'paytirib saqlaydi — shuning uchun matritsa qatorlari DUNYO fazosida
    // beriladi va modelview kamera ko'rinishiga teng bo'lishi shart.
    const float* M = lightMat_;
    const GLfloat sP[4] = {M[0], M[4], M[8],  M[12]};
    const GLfloat tP[4] = {M[1], M[5], M[9],  M[13]};
    const GLfloat rP[4] = {M[2], M[6], M[10], M[14]};
    const GLfloat qP[4] = {M[3], M[7], M[11], M[15]};
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, sP);
    glTexGenfv(GL_T, GL_EYE_PLANE, tP);
    glTexGenfv(GL_R, GL_EYE_PLANE, rP);
    glTexGenfv(GL_Q, GL_EYE_PLANE, qP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_GEN_Q);
    {
        const GLfloat c[4] = {1.0f, 1.0f, 1.0f, shadowLevel};
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, c);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,  GL_REPLACE);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,  GL_PREVIOUS_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
        // A = Arg0*Arg2 + Arg1*(1-Arg2) = t*t + L*(1-t)
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,  GL_INTERPOLATE_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,  GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB,  GL_CONSTANT_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB,  GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, GL_SRC_ALPHA);
    }

    // --- 2: rang x koeffitsiyent ---
    pActiveTexture(GL_TEXTURE2_ARB);
    if (white_ == 0) {
        glGenTextures(1, &white_);
        glBindTexture(GL_TEXTURE_2D, white_);
        const unsigned char px[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, white_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,  GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,  GL_PREVIOUS_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,  GL_PREVIOUS_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,  GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,  GL_PRIMARY_COLOR_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

    pActiveTexture(GL_TEXTURE0_ARB);          // sahna avvalgidek 0-birlikda
    receiving_ = true;
}

void ShadowMap::unbindReceive() {
    if (!receiving_) return;
    pActiveTexture(GL_TEXTURE2_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    pActiveTexture(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    pActiveTexture(GL_TEXTURE0_ARB);
    receiving_ = false;
}

} // namespace ert
