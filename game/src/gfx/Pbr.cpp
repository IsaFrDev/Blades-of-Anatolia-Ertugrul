#include "ertugrul/gfx/Pbr.h"
#include <windows.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// GL 2.0 turlari/konstantalari — MinGW gl.h 1.1 da yo'q
typedef char GLchar;
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

namespace ert {
namespace {

typedef GLuint (APIENTRY* PFN_CreateShader)(GLenum);
typedef void   (APIENTRY* PFN_ShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void   (APIENTRY* PFN_CompileShader)(GLuint);
typedef void   (APIENTRY* PFN_GetShaderiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY* PFN_GetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint (APIENTRY* PFN_CreateProgram)(void);
typedef void   (APIENTRY* PFN_AttachShader)(GLuint, GLuint);
typedef void   (APIENTRY* PFN_LinkProgram)(GLuint);
typedef void   (APIENTRY* PFN_GetProgramiv)(GLuint, GLenum, GLint*);
typedef void   (APIENTRY* PFN_GetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void   (APIENTRY* PFN_UseProgram)(GLuint);
typedef GLint  (APIENTRY* PFN_GetUniformLocation)(GLuint, const GLchar*);
typedef void   (APIENTRY* PFN_Uniform1i)(GLint, GLint);
typedef void   (APIENTRY* PFN_Uniform1f)(GLint, GLfloat);
typedef void   (APIENTRY* PFN_Uniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);

PFN_CreateShader        pCreateShader;
PFN_ShaderSource        pShaderSource;
PFN_CompileShader       pCompileShader;
PFN_GetShaderiv         pGetShaderiv;
PFN_GetShaderInfoLog    pGetShaderInfoLog;
PFN_CreateProgram       pCreateProgram;
PFN_AttachShader        pAttachShader;
PFN_LinkProgram         pLinkProgram;
PFN_GetProgramiv        pGetProgramiv;
PFN_GetProgramInfoLog   pGetProgramInfoLog;
PFN_UseProgram          pUseProgram;
PFN_GetUniformLocation  pGetUniformLocation;
PFN_Uniform1i           pUniform1i;
PFN_Uniform1f           pUniform1f;
PFN_Uniform4f           pUniform4f;

template <class T> bool load(T& fn, const char* name) {
    fn = (T)wglGetProcAddress(name);
    return fn != nullptr;
}

// --- GLSL 1.20 (compatibility): fixed-function holatiga kirish uchun ---
const char* kVert = R"GLSL(
#version 120
varying vec3 vN;
varying vec3 vE;
varying vec4 vCol;
varying vec2 vUV;
varying vec4 vSh;
varying float vFog;
void main() {
    vec4 e = gl_ModelViewMatrix * gl_Vertex;
    vE   = e.xyz;
    vN   = gl_NormalMatrix * gl_Normal;
    vCol = gl_Color;
    vUV  = gl_MultiTexCoord0.xy;
    // ShadowMap 1-birlikka EYE_LINEAR texgen qo'ygan — o'sha tekisliklar
    vSh  = vec4(dot(gl_EyePlaneS[1], e), dot(gl_EyePlaneT[1], e),
                dot(gl_EyePlaneR[1], e), dot(gl_EyePlaneQ[1], e));
    // chiziqli tuman: (end - dist) * scale, dist = -e.z
    vFog = clamp((gl_Fog.end + e.z) * gl_Fog.scale, 0.0, 1.0);
    gl_Position = ftransform();
}
)GLSL";

const char* kFrag = R"GLSL(
#version 120
uniform sampler2D       uTex;
uniform sampler2DShadow uShadow;
uniform float uShadowOn;
uniform float uShadowLevel;
uniform float uTexScale;
uniform float uUseVCol;
uniform float uRough;
uniform vec4  uMat;
varying vec3 vN;
varying vec3 vE;
varying vec4 vCol;
varying vec2 vUV;
varying vec4 vSh;
varying float vFog;

const float PI = 3.14159265;

void main() {
    vec4 tex  = texture2D(uTex, vUV);
    vec4 base = mix(uMat, vCol, uUseVCol);
    vec3 albedo = base.rgb * tex.rgb * uTexScale;
    float alpha = base.a * tex.a;

    vec3 N = normalize(vN);
    if (!gl_FrontFacing) N = -N;
    vec3 V = normalize(-vE);
    float nv = max(dot(N, V), 0.001);

    // Soya: apparat solishtiruvi (PCF), xaritadan tashqarisi yorug' (border=1)
    float sh = 1.0;
    if (uShadowOn > 0.5) {
        float s = shadow2DProj(uShadow, vSh).r;
        sh = mix(uShadowLevel, 1.0, s);
    }

    // --- Quyosh (GL_LIGHT0, yo'naltirilgan): GGX ---
    vec3  L0  = normalize(gl_LightSource[0].position.xyz);
    float nl0 = max(dot(N, L0), 0.0);
    vec3  H   = normalize(L0 + V);
    float nh  = max(dot(N, H), 0.0);
    float hv  = max(dot(H, V), 0.0);
    float a   = max(uRough * uRough, 0.02);
    float a2  = a * a;
    float dd  = nh * nh * (a2 - 1.0) + 1.0;
    float D   = a2 / (PI * dd * dd);
    float k   = (uRough + 1.0) * (uRough + 1.0) / 8.0;
    float G   = (nv / (nv * (1.0 - k) + k)) * (nl0 / (nl0 * (1.0 - k) + k + 1e-4));
    float F0  = 0.04;
    float F   = F0 + (1.0 - F0) * pow(1.0 - hv, 5.0);
    vec3  sunC = gl_LightSource[0].diffuse.rgb;
    vec3  spec = vec3(D * G * F / (4.0 * nv * nl0 + 1e-4)) * sunC * nl0 * sh;
    vec3  diff0 = albedo * (1.0 - F) * sunC * nl0 * sh;

    // --- To'ldiruvchi (GL_LIGHT1): osmon gumbazi, yumshoq ---
    vec3  L1  = normalize(gl_LightSource[1].position.xyz);
    float nl1 = max(dot(N, L1), 0.0);
    vec3  diff1 = albedo * gl_LightSource[1].diffuse.rgb * nl1;

    // --- Ambient + gorizontdan yengil hemisferik qo'shimcha ---
    vec3 amb = albedo * gl_LightModel.ambient.rgb * (0.85 + 0.15 * N.y);

    vec3 col = amb + diff0 + diff1 + spec;
    col = mix(gl_Fog.color.rgb, col, vFog);
    gl_FragColor = vec4(col, alpha);
}
)GLSL";

GLuint compile(GLenum type, const char* src, const char* label) {
    GLuint s = pCreateShader(type);
    pShaderSource(s, 1, &src, nullptr);
    pCompileShader(s);
    GLint ok = 0;
    pGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; GLsizei n = 0;
        pGetShaderInfoLog(s, sizeof log, &n, log);
        std::printf("[PBR] %s shader XATO:\n%.*s\n", label, (int)n, log);
        return 0;
    }
    return s;
}

} // namespace

Pbr& Pbr::get() { static Pbr p; return p; }

unsigned Pbr::whiteTexture() {
    static GLuint id = 0;
    if (id == 0 && wglGetCurrentContext() != nullptr) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        const unsigned char px[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return id;
}

bool Pbr::init() {
    if (inited_) return avail_;
    if (wglGetCurrentContext() == nullptr) return false;
    inited_ = true;

    const bool ok =
        load(pCreateShader, "glCreateShader") && load(pShaderSource, "glShaderSource") &&
        load(pCompileShader, "glCompileShader") && load(pGetShaderiv, "glGetShaderiv") &&
        load(pGetShaderInfoLog, "glGetShaderInfoLog") && load(pCreateProgram, "glCreateProgram") &&
        load(pAttachShader, "glAttachShader") && load(pLinkProgram, "glLinkProgram") &&
        load(pGetProgramiv, "glGetProgramiv") && load(pGetProgramInfoLog, "glGetProgramInfoLog") &&
        load(pUseProgram, "glUseProgram") && load(pGetUniformLocation, "glGetUniformLocation") &&
        load(pUniform1i, "glUniform1i") && load(pUniform1f, "glUniform1f") &&
        load(pUniform4f, "glUniform4f");
    if (!ok) {
        std::printf("[PBR] GL 2.0 shader funksiyalari yo'q -> fixed-function\n");
        return false;
    }
    const GLuint vs = compile(GL_VERTEX_SHADER, kVert, "vertex");
    const GLuint fs = compile(GL_FRAGMENT_SHADER, kFrag, "fragment");
    if (!vs || !fs) return false;
    prog_ = pCreateProgram();
    pAttachShader(prog_, vs);
    pAttachShader(prog_, fs);
    pLinkProgram(prog_);
    GLint linked = 0;
    pGetProgramiv(prog_, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[2048]; GLsizei n = 0;
        pGetProgramInfoLog(prog_, sizeof log, &n, log);
        std::printf("[PBR] link XATO:\n%.*s\n", (int)n, log);
        prog_ = 0;
        return false;
    }
    uTex_         = pGetUniformLocation(prog_, "uTex");
    uShadow_      = pGetUniformLocation(prog_, "uShadow");
    uShadowOn_    = pGetUniformLocation(prog_, "uShadowOn");
    uShadowLevel_ = pGetUniformLocation(prog_, "uShadowLevel");
    uTexScale_    = pGetUniformLocation(prog_, "uTexScale");
    uUseVCol_     = pGetUniformLocation(prog_, "uUseVCol");
    uRough_       = pGetUniformLocation(prog_, "uRough");
    uMat_         = pGetUniformLocation(prog_, "uMat");
    avail_ = true;
    std::printf("[PBR] GLSL 1.20 dasturi tayyor (GGX + soya + tuman)\n");
    whiteTexture();
    return true;
}

bool Pbr::enabled() {
    static const bool off = (std::getenv("ERT_NO_PBR") != nullptr);
    if (off) return false;
    if (!inited_) init();
    return avail_;
}

void Pbr::begin(bool shadowOn, float shadowLevel) {
    if (!enabled() || active_) return;
    pUseProgram(prog_);
    pUniform1i(uTex_, 0);
    pUniform1i(uShadow_, 1);
    pUniform1f(uShadowOn_, shadowOn ? 1.0f : 0.0f);
    pUniform1f(uShadowLevel_, shadowLevel);
    pUniform1f(uTexScale_, 1.0f);
    pUniform1f(uUseVCol_, 1.0f);
    pUniform1f(uRough_, 0.70f);
    pUniform4f(uMat_, 1.0f, 1.0f, 1.0f, 1.0f);
    active_ = true;
    paused_ = false;
}

void Pbr::end() {
    if (!active_) return;
    pUseProgram(0);
    active_ = false;
    paused_ = false;
}

void Pbr::pause()  { if (active_ && !paused_) { pUseProgram(0);     paused_ = true;  } }
void Pbr::resume() { if (active_ &&  paused_) { pUseProgram(prog_); paused_ = false; } }

void Pbr::setMaterial(float r, float g, float b, float a) {
    if (active_) pUniform4f(uMat_, r, g, b, a);
}
void Pbr::setUseVertexColor(bool on) { if (active_) pUniform1f(uUseVCol_, on ? 1.0f : 0.0f); }
void Pbr::setRoughness(float r)      { if (active_) pUniform1f(uRough_, r < 0.05f ? 0.05f : (r > 1.0f ? 1.0f : r)); }
void Pbr::setTexScale(float s)       { if (active_) pUniform1f(uTexScale_, s); }

} // namespace ert
