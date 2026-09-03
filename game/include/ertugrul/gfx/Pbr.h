#pragma once
// PBR-lite: GLSL (GL 2.0) orqali piksel darajasidagi yorug'lik — TASHQI KUTUBXONASIZ.
//
// Shader funksiyalari (glCreateShader, glUseProgram, ...) wglGetProcAddress bilan
// to'g'ridan-to'g'ri drayverdan olinadi; GLEW/GLAD kerak emas. Shader bog'langanda
// fixed-function yorug'lik va tekstura muhiti o'rniga GLSL ishlaydi, lekin
// HOLAT (gl_LightSource, gl_Fog, gl_Color, gl_MultiTexCoord0, gl_EyePlane*)
// o'sha-o'sha — shuning uchun display listlar, Level::applyLighting va
// ShadowMap texgen'i o'zgarishsiz qoladi.
//
// Nima "PBR": Cook-Torrance GGX spekulyar (D·G·F), Schlick Fresnel, energiya
// saqlovchi diffuz (1-F), piksel darajasida normal, soya xaritasi shader ichida
// (sampler2DShadow, PCF). Nima EMAS: metall/roughness xaritalari, normal-map,
// IBL — modellarda bunday tekstura yo'q (Tripo faqat basecolor beradi).
//
// ERT_NO_PBR=1 -> fixed-function (eski yo'l, GL 1.1 kartalar uchun zaxira).
namespace ert {

class Pbr {
public:
    static Pbr& get();

    bool init();                      // GL konteksti kerak; bir marta
    bool enabled();                   // mavjud va o'chirilmagan
    bool active() const { return active_; }

    // Sahna chizishdan oldin/keyin. shadowOn — ShadowMap::bindReceive chaqirilganmi.
    void begin(bool shadowOn, float shadowLevel);
    void end();
    // Yoritilmagan chizish (soya diski, marker) uchun vaqtincha to'xtatish
    void pause();
    void resume();

    // Material: pushTintMaterial (rang materialdan) / glColor (verteksdan)
    void setMaterial(float r, float g, float b, float a);
    void setUseVertexColor(bool on);
    void setRoughness(float r);       // 0.2 silliq metall .. 1.0 mot
    void setTexScale(float s);        // relyef detal teksturasi uchun 2.0

    // 1x1 oq tekstura: teksturasiz mesh shuni bog'laydi, shader doim sampler o'qiydi
    static unsigned whiteTexture();

private:
    bool inited_ = false, avail_ = false, active_ = false, paused_ = false;
    unsigned prog_ = 0;
    int uTex_ = -1, uShadow_ = -1, uShadowOn_ = -1, uShadowLevel_ = -1;
    int uTexScale_ = -1, uUseVCol_ = -1, uRough_ = -1, uMat_ = -1, uShadowEye_ = -1, uDbg_ = -1;
};

} // namespace ert
