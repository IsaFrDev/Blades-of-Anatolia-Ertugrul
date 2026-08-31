// Ertugrul :: gfx/Texture.cpp
// GDI+ orqali JPG/PNG/BMP dekodlash -> OpenGL 1.1 teksturasi.
// Protsedural teksturalar (o't, tuproq, tosh, mato, yog'och) fbm2D asosida.
// Tashqi kutubxona yo'q: faqat Win32 + GDI+ + OpenGL/GLU.

#include "ertugrul/gfx/Texture.h"

// DIQQAT: include tartibi muhim — windows.h -> objidl.h -> gdiplus.h,
// va windows.h har doim GL sarlavhalaridan oldin.
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "ertugrul/core/Math.h"

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <iostream>

namespace ert {

// ============================================================================
//  Ichki yordamchilar (faqat shu fayl uchun)
// ============================================================================
namespace {

// --- Anizotrop filtrlash uchun kengaytma konstantalari (GL 1.1 da yo'q) ---
constexpr GLenum kTexMaxAnisotropyExt    = 0x84FE;
constexpr GLenum kMaxTexMaxAnisotropyExt = 0x84FF;
// GL 1.2 dagi CLAMP_TO_EDGE; mavjud bo'lmasa GL_CLAMP ga qaytamiz
constexpr GLenum kClampToEdge            = 0x812F;

// --- GDI+ holati (funksiya-lokal static orqali, header'da global yo'q) ---
struct ImagingState {
    ULONG_PTR token   = 0;
    int       refCount = 0;   // initImaging() ni ikki marta chaqirishga chidamli
};
ImagingState& imaging() {
    static ImagingState s;
    return s;
}

// --- Tekstura keshi ---
using TexMap = std::map<std::string, Texture*>;
TexMap& cache() {
    static TexMap m;
    return m;
}

// GL konteksti mavjudmi? (kontekstsiz GL chaqiruvlari qilinmasin)
inline bool hasGLContext() {
    return wglGetCurrentContext() != nullptr;
}

// GL kengaytmasi mavjudligini tekshirish (butun so'z bo'yicha)
bool hasGLExtension(const char* name) {
    if (!name || !hasGLContext()) return false;
    const GLubyte* ext = glGetString(GL_EXTENSIONS);
    if (!ext) return false;
    const char* s = reinterpret_cast<const char*>(ext);
    const size_t n = std::strlen(name);
    const char* p = s;
    while ((p = std::strstr(p, name)) != nullptr) {
        const bool leftOk  = (p == s) || (p[-1] == ' ');
        const char after   = p[n];
        const bool rightOk = (after == ' ' || after == '\0');
        if (leftOk && rightOk) return true;
        p += n;
    }
    return false;
}

// GL xatolar navbatini tozalash
inline void flushGLErrors() {
    if (!hasGLContext()) return;
    for (int i = 0; i < 8 && glGetError() != GL_NO_ERROR; ++i) { /* bo'sh */ }
}

// UTF-8 -> UTF-16 (Windows keng satri). Bo'sh bo'lsa bo'sh qaytaradi.
std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    const int need = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (need <= 0) {
        // UTF-8 emas ekan — tizim kod sahifasi bilan urinib ko'ramiz
        const int need2 = MultiByteToWideChar(CP_ACP, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
        if (need2 <= 0) return std::wstring();
        std::wstring w2(static_cast<size_t>(need2), L'\0');
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), static_cast<int>(s.size()), &w2[0], need2);
        return w2;
    }
    std::wstring w(static_cast<size_t>(need), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], need);
    return w;
}

// --- QUTI (box) filtri bilan kichraytirish ---
// Alfa bilan vaznlangan o'rtacha: shaffof piksellar rangni "yuvib" yubormasin.
void boxDownscale(const std::vector<uint8_t>& src, int sw, int sh,
                  std::vector<uint8_t>& dst, int dw, int dh) {
    dst.assign(static_cast<size_t>(dw) * static_cast<size_t>(dh) * 4u, 0);
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;

    for (int y = 0; y < dh; ++y) {
        int sy0 = static_cast<int>((static_cast<int64_t>(y)     * sh) / dh);
        int sy1 = static_cast<int>((static_cast<int64_t>(y + 1) * sh) / dh);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        if (sy1 > sh)   sy1 = sh;
        if (sy0 >= sh)  sy0 = sh - 1;

        for (int x = 0; x < dw; ++x) {
            int sx0 = static_cast<int>((static_cast<int64_t>(x)     * sw) / dw);
            int sx1 = static_cast<int>((static_cast<int64_t>(x + 1) * sw) / dw);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            if (sx1 > sw)   sx1 = sw;
            if (sx0 >= sw)  sx0 = sw - 1;

            double accR = 0.0, accG = 0.0, accB = 0.0, accA = 0.0;
            double plainR = 0.0, plainG = 0.0, plainB = 0.0;
            int    count = 0;

            for (int sy = sy0; sy < sy1; ++sy) {
                const uint8_t* row = &src[(static_cast<size_t>(sy) * sw + sx0) * 4u];
                for (int sx = sx0; sx < sx1; ++sx, row += 4) {
                    const double a = row[3];
                    accR += row[0] * a; accG += row[1] * a; accB += row[2] * a;
                    plainR += row[0];   plainG += row[1];   plainB += row[2];
                    accA += a;
                    ++count;
                }
            }
            if (count <= 0) count = 1;

            uint8_t* out = &dst[(static_cast<size_t>(y) * dw + x) * 4u];
            if (accA > 0.5) {
                out[0] = static_cast<uint8_t>(clampf(static_cast<float>(accR / accA), 0.0f, 255.0f) + 0.5f);
                out[1] = static_cast<uint8_t>(clampf(static_cast<float>(accG / accA), 0.0f, 255.0f) + 0.5f);
                out[2] = static_cast<uint8_t>(clampf(static_cast<float>(accB / accA), 0.0f, 255.0f) + 0.5f);
            } else {
                out[0] = static_cast<uint8_t>(clampf(static_cast<float>(plainR / count), 0.0f, 255.0f) + 0.5f);
                out[1] = static_cast<uint8_t>(clampf(static_cast<float>(plainG / count), 0.0f, 255.0f) + 0.5f);
                out[2] = static_cast<uint8_t>(clampf(static_cast<float>(plainB / count), 0.0f, 255.0f) + 0.5f);
            }
            out[3] = static_cast<uint8_t>(clampf(static_cast<float>(accA / count), 0.0f, 255.0f) + 0.5f);
        }
    }
}

// ============================================================================
//  Protsedural shovqin yordamchilari
// ============================================================================

// Cheksiz takrorlanuvchi (seamless) fBm.
// u,v in [0,1). fx,fy — butun chastotalar (tekstura chegarasida uzilish bo'lmaydi).
// 4 ta siljitilgan namunani bilinear aralashtirish orqali chekkalar mos keladi.
float tileFbm(float u, float v, int fx, int fy, uint32_t seed, int oct) {
    if (fx < 1) fx = 1;
    if (fy < 1) fy = 1;
    const float X = static_cast<float>(fx);
    const float Y = static_cast<float>(fy);

    const float a = fbm2D( u        * X,  v        * Y, seed, oct, 2.0f, 0.5f);
    const float b = fbm2D((u - 1.f) * X,  v        * Y, seed, oct, 2.0f, 0.5f);
    const float c = fbm2D( u        * X, (v - 1.f) * Y, seed, oct, 2.0f, 0.5f);
    const float d = fbm2D((u - 1.f) * X, (v - 1.f) * Y, seed, oct, 2.0f, 0.5f);

    const float iu = 1.0f - u, iv = 1.0f - v;
    return saturate(a * iu * iv + b * u * iv + c * iu * v + d * u * v);
}

// "Ridged" shovqin — yoriq/tomir effekti uchun (1 ga yaqin qiymatlar = chiziq)
inline float ridged(float n) {
    return 1.0f - std::fabs(n * 2.0f - 1.0f);
}

inline uint8_t toByte(float v) {
    return static_cast<uint8_t>(clampf(v, 0.0f, 255.0f) + 0.5f);
}

// Piksel yozish (chegara tekshiruvi bilan, o'ralish/wrap bilan)
inline void putPx(std::vector<uint8_t>& px, int size, int x, int y,
                  float r, float g, float b, float a) {
    if (size <= 0) return;
    x = ((x % size) + size) % size;
    y = ((y % size) + size) % size;
    uint8_t* p = &px[(static_cast<size_t>(y) * size + x) * 4u];
    p[0] = toByte(r); p[1] = toByte(g); p[2] = toByte(b); p[3] = toByte(a);
}

inline void blendPx(std::vector<uint8_t>& px, int size, int x, int y,
                    float r, float g, float b, float t) {
    if (size <= 0) return;
    t = saturate(t);
    x = ((x % size) + size) % size;
    y = ((y % size) + size) % size;
    uint8_t* p = &px[(static_cast<size_t>(y) * size + x) * 4u];
    p[0] = toByte(lerpf(static_cast<float>(p[0]), r, t));
    p[1] = toByte(lerpf(static_cast<float>(p[1]), g, t));
    p[2] = toByte(lerpf(static_cast<float>(p[2]), b, t));
}

// Keshdan olish yoki yangi protsedural teksturani yaratib keshga qo'yish.
// gen() piksellarni to'ldiradi. HECH QACHON nullptr qaytarmaydi.
template <typename Gen>
Texture* cachedProcedural(const std::string& key, int size, Gen gen) {
    TexMap& c = cache();
    TexMap::iterator it = c.find(key);
    if (it != c.end() && it->second) return it->second;

    if (size < 1)    size = 1;
    if (size > 2048) size = 2048;

    std::vector<uint8_t> px(static_cast<size_t>(size) * size * 4u, 255);
    gen(px, size);

    Texture* t = new Texture();
    t->createRGBA(px.data(), size, size, true, true);   // muvaffaqiyatsiz bo'lsa ham obyekt qoladi
    c[key] = t;
    return t;
}

} // anonim namespace

// ============================================================================
//  GDI+ ishga tushirish / to'xtatish
// ============================================================================

bool Texture::initImaging() {
    ImagingState& st = imaging();
    if (st.refCount > 0) { ++st.refCount; return true; }

    Gdiplus::GdiplusStartupInput input;
    const Gdiplus::Status s = Gdiplus::GdiplusStartup(&st.token, &input, nullptr);
    if (s != Gdiplus::Ok) {
        std::cerr << "[Texture] OGOHLANTIRISH: GDI+ ishga tushmadi (status="
                  << static_cast<int>(s) << "). Rasm fayllari yuklanmaydi.\n";
        st.token = 0;
        return false;
    }
    st.refCount = 1;
    return true;
}

void Texture::shutdownImaging() {
    ImagingState& st = imaging();
    if (st.refCount <= 0) return;          // ikki marta chaqirishga chidamli
    if (--st.refCount > 0) return;

    // Keshdagi teksturalarni GDI+ yopilishidan oldin bo'shatamiz
    clearCache();

    if (st.token != 0) {
        Gdiplus::GdiplusShutdown(st.token);
        st.token = 0;
    }
}

// ============================================================================
//  Texture: hayot sikli
// ============================================================================

Texture::~Texture() {
    destroy();
}

void Texture::destroy() {
    // GL konteksti bo'lmasa ham xavfsiz: shunchaki id ni tashlab yuboramiz
    if (id_ != 0) {
        if (hasGLContext()) {
            const GLuint t = static_cast<GLuint>(id_);
            glDeleteTextures(1, &t);
        }
        id_ = 0;
    }
    w_ = 0;
    h_ = 0;
    path_.clear();
}

void Texture::bind() const {
    if (id_ != 0 && hasGLContext()) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(id_));
    }
}

void Texture::unbind() {
    if (hasGLContext()) glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
//  Fayldan dekodlash (GDI+)
// ============================================================================

bool Texture::decodeFile(const std::string& path, std::vector<uint8_t>& outRGBA,
                         int& w, int& h, int maxSize) {
    outRGBA.clear();
    w = 0;
    h = 0;
    if (path.empty()) return false;

    // GDI+ hali ishga tushmagan bo'lsa — o'zimiz yoqamiz (kutubxona qarzda qolmaydi)
    if (imaging().refCount <= 0) {
        if (!initImaging()) return false;
    }

    const std::wstring wpath = utf8ToWide(path);
    if (wpath.empty()) return false;

    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(wpath.c_str(), FALSE);
    if (!bmp) return false;
    if (bmp->GetLastStatus() != Gdiplus::Ok) { delete bmp; return false; }

    const int sw = static_cast<int>(bmp->GetWidth());
    const int sh = static_cast<int>(bmp->GetHeight());
    if (sw <= 0 || sh <= 0) { delete bmp; return false; }

    // Juda katta rasm — vaqtinchalik bufer ham katta bo'ladi; 16384 dan oshmasin
    if (sw > 16384 || sh > 16384) {
        std::cerr << "[Texture] OGOHLANTIRISH: rasm juda katta (" << sw << "x" << sh
                  << "): " << path << "\n";
        delete bmp;
        return false;
    }

    Gdiplus::Rect rect(0, 0, sw, sh);
    Gdiplus::BitmapData bd;
    std::memset(&bd, 0, sizeof(bd));

    const Gdiplus::Status lockSt =
        bmp->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd);
    if (lockSt != Gdiplus::Ok || bd.Scan0 == nullptr) {
        delete bmp;
        return false;
    }

    // BGRA -> RGBA. Stride manfiy bo'lishi mumkin (pastdan-yuqoriga tartib).
    std::vector<uint8_t> full;
    bool ok = true;
    try {
        full.assign(static_cast<size_t>(sw) * static_cast<size_t>(sh) * 4u, 0);
    } catch (...) {
        ok = false;
    }

    if (ok) {
        const uint8_t* base = static_cast<const uint8_t*>(bd.Scan0);
        const int stride = static_cast<int>(bd.Stride);
        for (int y = 0; y < sh; ++y) {
            const uint8_t* srcRow = base + static_cast<ptrdiff_t>(stride) * y;
            uint8_t* dstRow = &full[static_cast<size_t>(y) * sw * 4u];
            for (int x = 0; x < sw; ++x) {
                // GDI+ 32bppARGB xotirada: B, G, R, A
                dstRow[x * 4 + 0] = srcRow[x * 4 + 2];
                dstRow[x * 4 + 1] = srcRow[x * 4 + 1];
                dstRow[x * 4 + 2] = srcRow[x * 4 + 0];
                dstRow[x * 4 + 3] = srcRow[x * 4 + 3];
            }
        }
    }

    bmp->UnlockBits(&bd);
    delete bmp;
    if (!ok) return false;

    // --- Kerak bo'lsa quti (box) filtri bilan kichraytirish ---
    if (maxSize < 1) maxSize = 1;
    // OBJ/MTL UV konventsiyasi: v = 0 rasmning PASTKI qatori.
    // GDI+ esa birinchi qator sifatida YUQORI qatorni beradi. Ag'darmasak
    // barcha UV lar noto'g'ri joyga tushadi — Kenney colormap.png ning yuqori
    // qatori qop-qora bo'lgani uchun butun qishloq qop-qora chiqardi.
    {
        const size_t rowBytes = static_cast<size_t>(sw) * 4u;
        if (rowBytes > 0 && full.size() >= rowBytes * static_cast<size_t>(sh)) {
            std::vector<uint8_t> tmp(rowBytes);
            for (int y = 0; y < sh / 2; ++y) {
                uint8_t* a = full.data() + rowBytes * static_cast<size_t>(y);
                uint8_t* b = full.data() + rowBytes * static_cast<size_t>(sh - 1 - y);
                std::memcpy(tmp.data(), a, rowBytes);
                std::memcpy(a, b, rowBytes);
                std::memcpy(b, tmp.data(), rowBytes);
            }
        }
    }

    if (sw > maxSize || sh > maxSize) {
        const float scale = (sw >= sh) ? (static_cast<float>(maxSize) / sw)
                                       : (static_cast<float>(maxSize) / sh);
        int dw = static_cast<int>(sw * scale + 0.5f);
        int dh = static_cast<int>(sh * scale + 0.5f);
        if (dw < 1) dw = 1;
        if (dh < 1) dh = 1;
        if (dw > maxSize) dw = maxSize;
        if (dh > maxSize) dh = maxSize;

        boxDownscale(full, sw, sh, outRGBA, dw, dh);
        w = dw;
        h = dh;
    } else {
        outRGBA.swap(full);
        w = sw;
        h = sh;
    }
    return !outRGBA.empty();
}

bool Texture::loadFile(const std::string& path, int maxSize) {
    std::vector<uint8_t> px;
    int w = 0, h = 0;
    if (!decodeFile(path, px, w, h, maxSize)) return false;
    if (!createRGBA(px.data(), w, h, true, true)) return false;
    path_ = path;
    return true;
}

// ============================================================================
//  GL teksturasini yaratish
// ============================================================================

bool Texture::createRGBA(const uint8_t* rgba, int w, int h, bool mipmap, bool repeat) {
    destroy();

    if (rgba == nullptr || w <= 0 || h <= 0) return false;
    if (!hasGLContext()) {
        // Kontekst yo'q — halokatsiz muvaffaqiyatsizlik
        return false;
    }

    // Qurilma chegarasidan oshmasin
    GLint maxTex = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    if (maxTex < 64) maxTex = 64;

    std::vector<uint8_t> resized;
    const uint8_t* data = rgba;
    if (w > maxTex || h > maxTex) {
        std::vector<uint8_t> src(rgba, rgba + static_cast<size_t>(w) * h * 4u);
        int dw = w, dh = h;
        const float s = (w >= h) ? (static_cast<float>(maxTex) / w)
                                 : (static_cast<float>(maxTex) / h);
        dw = static_cast<int>(w * s); if (dw < 1) dw = 1;
        dh = static_cast<int>(h * s); if (dh < 1) dh = 1;
        boxDownscale(src, w, h, resized, dw, dh);
        data = resized.data();
        w = dw;
        h = dh;
    }

    flushGLErrors();

    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (tex == 0) return false;

    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // O'rash rejimi
    const GLint wrapMode = repeat ? static_cast<GLint>(GL_REPEAT)
                                  : static_cast<GLint>(kClampToEdge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    if (!repeat && glGetError() != GL_NO_ERROR) {
        // CLAMP_TO_EDGE (GL 1.2) qo'llab-quvvatlanmasa — GL 1.1 dagi CLAMP
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    bool built = false;
    if (mipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        // GLU mavjud: mipmap zanjirini o'zi quradi (2 darajasi bo'lmagan o'lchamlarni ham)
        const GLint r = gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w, h,
                                          GL_RGBA, GL_UNSIGNED_BYTE, data);
        built = (r == 0);
    }
    if (!built) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

    // --- Anizotropiya: faqat kengaytma mavjud bo'lsa ---
    {
        // Har safar tekshiramiz: GL konteksti qayta yaratilgan bo'lishi mumkin
        const bool anisoOk = hasGLExtension("GL_EXT_texture_filter_anisotropic");
        if (anisoOk && mipmap && built) {
            GLfloat maxAniso = 1.0f;
            glGetFloatv(kMaxTexMaxAnisotropyExt, &maxAniso);
            if (maxAniso > 1.0f) {
                if (maxAniso > 8.0f) maxAniso = 8.0f;
                glTexParameterf(GL_TEXTURE_2D, kTexMaxAnisotropyExt, maxAniso);
            }
        }
    }

    // Xatolar bo'lsa teksturani tashlab yuboramiz
    if (glGetError() != GL_NO_ERROR) {
        // Yaratilgan bo'lishi mumkin — baribir ishlatamiz, lekin bog'lanishni tozalaymiz
        flushGLErrors();
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    id_ = tex;
    w_  = w;
    h_  = h;
    return true;
}

// ============================================================================
//  Kesh
// ============================================================================

void Texture::clearCache() {
    TexMap& c = cache();
    for (TexMap::iterator it = c.begin(); it != c.end(); ++it) {
        delete it->second;      // ~Texture() GL kontekstsiz ham xavfsiz
    }
    c.clear();
}

Texture* Texture::get(const std::string& path, int maxSize) {
    TexMap& c = cache();
    // Kalitga maxSize ham kiradi: bir xil rasm turli o'lchamda kerak bo'lishi mumkin
    char sizeBuf[32];
    std::snprintf(sizeBuf, sizeof(sizeBuf), "|%d", maxSize);
    const std::string key = path + sizeBuf;

    TexMap::iterator it = c.find(key);
    if (it != c.end() && it->second) return it->second;

    Texture* t = new Texture();
    if (!t->loadFile(path, maxSize)) {
        // --- O'RINBOSAR: nullptr EMAS, ko'zga tashlanadigan magenta/kulrang shaxmat ---
        std::cerr << "[Texture] OGOHLANTIRISH: tekstura yuklanmadi -> o'rinbosar ishlatildi: "
                  << path << "\n";

        const int size = 64;
        const int cell = size / 8;
        std::vector<uint8_t> px(static_cast<size_t>(size) * size * 4u, 255);
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const bool on = (((x / cell) + (y / cell)) & 1) != 0;
                uint8_t* p = &px[(static_cast<size_t>(y) * size + x) * 4u];
                p[0] = on ? 255 : 110;
                p[1] = on ?   0 : 110;
                p[2] = on ? 255 : 110;
                p[3] = 255;
            }
        }
        t->createRGBA(px.data(), size, size, true, true);
        // createRGBA muvaffaqiyatsiz bo'lsa ham (GL kontekst yo'q) obyektni qaytaramiz
    }
    c[key] = t;
    return t;
}

// ============================================================================
//  Protsedural teksturalar
// ============================================================================

Texture* Texture::solid(uint8_t r, uint8_t g, uint8_t b) {
    char key[64];
    std::snprintf(key, sizeof(key), "#solid:%u,%u,%u",
                  static_cast<unsigned>(r), static_cast<unsigned>(g), static_cast<unsigned>(b));
    return cachedProcedural(key, 4, [r, g, b](std::vector<uint8_t>& px, int size) {
        for (int i = 0; i < size * size; ++i) {
            px[static_cast<size_t>(i) * 4u + 0] = r;
            px[static_cast<size_t>(i) * 4u + 1] = g;
            px[static_cast<size_t>(i) * 4u + 2] = b;
            px[static_cast<size_t>(i) * 4u + 3] = 255;
        }
    });
}

Texture* Texture::checker(uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2, int cells, int size) {
    if (cells < 1) cells = 1;
    if (size  < 2) size  = 2;
    if (size  > 1024) size = 1024;

    char key[128];
    std::snprintf(key, sizeof(key), "#checker:%u,%u,%u,%u,%u,%u,%d,%d",
                  static_cast<unsigned>(r1), static_cast<unsigned>(g1), static_cast<unsigned>(b1),
                  static_cast<unsigned>(r2), static_cast<unsigned>(g2), static_cast<unsigned>(b2),
                  cells, size);

    return cachedProcedural(key, size, [=](std::vector<uint8_t>& px, int s) {
        const int cell = (s / cells) > 0 ? (s / cells) : 1;
        for (int y = 0; y < s; ++y) {
            for (int x = 0; x < s; ++x) {
                const bool on = (((x / cell) + (y / cell)) & 1) != 0;
                uint8_t* p = &px[(static_cast<size_t>(y) * s + x) * 4u];
                p[0] = on ? r1 : r2;
                p[1] = on ? g1 : g2;
                p[2] = on ? b1 : b2;
                p[3] = 255;
            }
        }
    });
}

Texture* Texture::noise(uint8_t r, uint8_t g, uint8_t b, float amount, uint32_t seed, int size) {
    if (size < 2) size = 2;
    if (size > 1024) size = 1024;
    amount = clampf(amount, 0.0f, 1.0f);

    char key[160];
    std::snprintf(key, sizeof(key), "#noise:%u,%u,%u,%.4f,%u,%d",
                  static_cast<unsigned>(r), static_cast<unsigned>(g), static_cast<unsigned>(b),
                  static_cast<double>(amount), static_cast<unsigned>(seed), size);

    return cachedProcedural(key, size, [=](std::vector<uint8_t>& px, int s) {
        const float inv = 1.0f / static_cast<float>(s);
        for (int y = 0; y < s; ++y) {
            const float v = (y + 0.5f) * inv;
            for (int x = 0; x < s; ++x) {
                const float u = (x + 0.5f) * inv;
                // Ikki qatlam: yirik dog'lar + mayda don
                const float big  = tileFbm(u, v, 4,  4,  seed,        4);
                const float fine = tileFbm(u, v, 32, 32, seed + 777u, 2);
                const float n = (big * 0.6f + fine * 0.4f - 0.5f) * 2.0f;   // [-1,1]
                const float k = 1.0f + n * amount;
                uint8_t* p = &px[(static_cast<size_t>(y) * s + x) * 4u];
                p[0] = toByte(r * k);
                p[1] = toByte(g * k);
                p[2] = toByte(b * k);
                p[3] = 255;
            }
        }
    });
}

// --- O't: yashil tuslar + tasodifiy o't tolalari ---
Texture* Texture::grass(uint32_t seed) {
    char key[64];
    std::snprintf(key, sizeof(key), "#grass:%u", static_cast<unsigned>(seed));

    return cachedProcedural(key, 256, [seed](std::vector<uint8_t>& px, int s) {
        const float inv = 1.0f / static_cast<float>(s);

        for (int y = 0; y < s; ++y) {
            const float v = (y + 0.5f) * inv;
            for (int x = 0; x < s; ++x) {
                const float u = (x + 0.5f) * inv;

                const float patch = tileFbm(u, v, 3,  3,  seed,          4);  // yirik dog'lar
                const float mid   = tileFbm(u, v, 10, 10, seed + 101u,   3);
                const float fine  = tileFbm(u, v, 40, 40, seed + 2029u,  2);  // mayda don
                float t = saturate(patch * 0.45f + mid * 0.35f + fine * 0.20f);

                // To'q -> och yashil oralig'i
                float rr = lerpf( 34.0f,  98.0f, t);
                float gg = lerpf( 74.0f, 156.0f, t);
                float bb = lerpf( 28.0f,  60.0f, t);

                // Quruq (sarg'ish) joylar
                const float dry = tileFbm(u, v, 2, 2, seed + 555u, 4);
                if (dry > 0.60f) {
                    const float k = saturate((dry - 0.60f) / 0.40f) * 0.55f;
                    rr = lerpf(rr, 150.0f, k);
                    gg = lerpf(gg, 134.0f, k);
                    bb = lerpf(bb,  74.0f, k);
                }

                uint8_t* p = &px[(static_cast<size_t>(y) * s + x) * 4u];
                p[0] = toByte(rr); p[1] = toByte(gg); p[2] = toByte(bb); p[3] = 255;
            }
        }

        // --- Tasodifiy o't tolalari (ingichka, biroz egri chiziqlar) ---
        Rng rng(seed * 2654435761u + 17u);
        const int blades = s * 6;
        for (int i = 0; i < blades; ++i) {
            const int   bx  = rng.rangeI(0, s - 1);
            const int   by  = rng.rangeI(0, s - 1);
            const int   len = rng.rangeI(3, 8);
            const float lean = rng.range(-0.55f, 0.55f);
            const bool  light = rng.nextFloat() > 0.35f;

            const float cr = light ? rng.range(110.0f, 168.0f) : rng.range(24.0f, 52.0f);
            const float cg = light ? rng.range(168.0f, 226.0f) : rng.range(56.0f, 92.0f);
            const float cb = light ? rng.range( 60.0f,  96.0f) : rng.range(20.0f, 44.0f);

            for (int k = 0; k < len; ++k) {
                const float tt = static_cast<float>(k) / static_cast<float>(len);
                const int   ox = static_cast<int>(lean * k);
                // Uchi ingichkalashadi
                blendPx(px, s, bx + ox, by - k, cr, cg, cb, 0.85f * (1.0f - tt * 0.6f));
            }
        }
    });
}

// --- Tuproq: jigarrang donador + mayda toshchalar ---
Texture* Texture::dirt(uint32_t seed) {
    char key[64];
    std::snprintf(key, sizeof(key), "#dirt:%u", static_cast<unsigned>(seed));

    return cachedProcedural(key, 256, [seed](std::vector<uint8_t>& px, int s) {
        const float inv = 1.0f / static_cast<float>(s);

        for (int y = 0; y < s; ++y) {
            const float v = (y + 0.5f) * inv;
            for (int x = 0; x < s; ++x) {
                const float u = (x + 0.5f) * inv;

                const float big  = tileFbm(u, v, 3,  3,  seed,         4);
                const float mid  = tileFbm(u, v, 12, 12, seed + 313u,  3);
                const float gran = tileFbm(u, v, 64, 64, seed + 9091u, 1);   // don
                float t = saturate(big * 0.45f + mid * 0.33f + gran * 0.22f);

                // To'q jigarrangdan och qumrangga
                float rr = lerpf( 74.0f, 162.0f, t);
                float gg = lerpf( 54.0f, 124.0f, t);
                float bb = lerpf( 36.0f,  84.0f, t);

                // Nam (to'qroq) joylar
                const float wet = tileFbm(u, v, 5, 5, seed + 4441u, 3);
                if (wet < 0.34f) {
                    const float k = saturate((0.34f - wet) / 0.34f) * 0.45f;
                    rr = lerpf(rr, 48.0f, k);
                    gg = lerpf(gg, 34.0f, k);
                    bb = lerpf(bb, 24.0f, k);
                }

                uint8_t* p = &px[(static_cast<size_t>(y) * s + x) * 4u];
                p[0] = toByte(rr); p[1] = toByte(gg); p[2] = toByte(bb); p[3] = 255;
            }
        }

        // --- Mayda toshchalar (yumshoq chekkali, tuproqqa singib ketgan) ---
        Rng rng(seed * 40503u + 7u);
        const int pebbles = s / 2;
        for (int i = 0; i < pebbles; ++i) {
            const int   cx  = rng.rangeI(0, s - 1);
            const int   cy  = rng.rangeI(0, s - 1);
            const float rad = rng.range(1.6f, 3.6f);
            const int   ir  = static_cast<int>(rad) + 1;
            const bool  light = rng.nextFloat() > 0.45f;
            // Toshcha rangi: tuproqdan biroz och yoki to'q, lekin keskin emas
            const float cr = light ? rng.range(150.0f, 188.0f) : rng.range(58.0f, 84.0f);
            const float cg = light ? rng.range(136.0f, 172.0f) : rng.range(46.0f, 68.0f);
            const float cb = light ? rng.range(112.0f, 148.0f) : rng.range(34.0f, 52.0f);

            for (int dy = -ir; dy <= ir; ++dy) {
                for (int dx = -ir; dx <= ir; ++dx) {
                    const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (d > rad) continue;
                    // Markazda kuchli, chetga borib so'nadi
                    const float k = saturate(1.0f - d / rad) * 0.45f;
                    blendPx(px, s, cx + dx, cy + dy, cr, cg, cb, k);
                }
            }
        }
    });
}

// --- Tosh: kulrang, yoriqlar bilan ---
Texture* Texture::rock(uint32_t seed) {
    char key[64];
    std::snprintf(key, sizeof(key), "#rock:%u", static_cast<unsigned>(seed));

    return cachedProcedural(key, 256, [seed](std::vector<uint8_t>& px, int s) {
        const float inv = 1.0f / static_cast<float>(s);

        for (int y = 0; y < s; ++y) {
            const float v = (y + 0.5f) * inv;
            for (int x = 0; x < s; ++x) {
                const float u = (x + 0.5f) * inv;

                const float big  = tileFbm(u, v, 4,  4,  seed,         4);
                const float mid  = tileFbm(u, v, 13, 13, seed + 191u,  3);
                const float spec = tileFbm(u, v, 80, 80, seed + 6151u, 1);  // mineral donalar
                float t = saturate(big * 0.42f + mid * 0.30f + spec * 0.28f);

                // Kulrang, biroz iliq tusda
                float rr = lerpf( 84.0f, 180.0f, t);
                float gg = lerpf( 82.0f, 175.0f, t);
                float bb = lerpf( 78.0f, 167.0f, t);

                // Granitga xos yorug'/to'q mineral dog'lar
                if (spec > 0.74f) {
                    const float k = saturate((spec - 0.74f) / 0.26f) * 0.55f;
                    rr = lerpf(rr, 214.0f, k);
                    gg = lerpf(gg, 210.0f, k);
                    bb = lerpf(bb, 202.0f, k);
                } else if (spec < 0.26f) {
                    const float k = saturate((0.26f - spec) / 0.26f) * 0.45f;
                    rr = lerpf(rr, 56.0f, k);
                    gg = lerpf(gg, 55.0f, k);
                    bb = lerpf(bb, 53.0f, k);
                }

                // --- Yoriqlar: ridged shovqin cho'qqilari, halosiz ingichka chiziq ---
                const float cr1 = ridged(tileFbm(u, v, 10, 10, seed + 8081u, 2));
                const float cr2 = ridged(tileFbm(u, v, 25, 25, seed + 8623u, 2)) * 0.94f;
                const float crack = (cr1 > cr2) ? cr1 : cr2;
                const float e = saturate((crack - 0.80f) / 0.20f);
                const float sharp = e * e * e;              // faqat eng cho'qqi qoladi
                if (sharp > 0.004f) {
                    rr = lerpf(rr, 38.0f, sharp * 0.85f);
                    gg = lerpf(gg, 37.0f, sharp * 0.85f);
                    bb = lerpf(bb, 35.0f, sharp * 0.85f);
                }

                uint8_t* p = &px[(static_cast<size_t>(y) * s + x) * 4u];
                p[0] = toByte(rr); p[1] = toByte(gg); p[2] = toByte(bb); p[3] = 255;
            }
        }
    });
}

// --- Mato: to'quv chizig'i (o'ru-arqoq) + tola shovqini ---
Texture* Texture::cloth(uint8_t r, uint8_t g, uint8_t b, uint32_t seed) {
    char key[96];
    std::snprintf(key, sizeof(key), "#cloth:%u,%u,%u,%u",
                  static_cast<unsigned>(r), static_cast<unsigned>(g),
                  static_cast<unsigned>(b), static_cast<unsigned>(seed));

    return cachedProcedural(key, 256, [r, g, b, seed](std::vector<uint8_t>& px, int s) {
        const float inv = 1.0f / static_cast<float>(s);
        const int   thread = 4;              // ip qalinligi (piksel)

        for (int y = 0; y < s; ++y) {
            const float v = (y + 0.5f) * inv;
            for (int x = 0; x < s; ++x) {
                const float u = (x + 0.5f) * inv;

                // To'quv: o'ru (vertikal) va arqoq (gorizontal) navbatlashadi
                const int cellX = x / thread;
                const int cellY = y / thread;
                const bool warpOnTop = ((cellX + cellY) & 1) != 0;

                // Ip ichidagi silindrik yorug'lik (chetlari to'qroq)
                const int inX = x % thread;
                const int inY = y % thread;
                const float bumpX = std::sin((inX + 0.5f) / thread * PI);
                const float bumpY = std::sin((inY + 0.5f) / thread * PI);
                const float bump  = warpOnTop ? bumpX : bumpY;

                // Tola/rang notekisligi
                const float fiber = tileFbm(u, v, 26, 26, seed + 71u, 2);
                const float stain = tileFbm(u, v, 3,  3,  seed,       3);

                float k = 0.66f + 0.42f * bump;          // to'quv soyasi
                k *= 0.90f + 0.20f * fiber;              // tola
                k *= 0.88f + 0.24f * stain;              // yirik dog'lar
                if (warpOnTop) k *= 1.05f;               // ustki ip biroz yorug'roq

                uint8_t* p = &px[(static_cast<size_t>(y) * s + x) * 4u];
                p[0] = toByte(r * k);
                p[1] = toByte(g * k);
                p[2] = toByte(b * k);
                p[3] = 255;
            }
        }
    });
}

// --- Yog'och: tola + yillik halqalar ---
Texture* Texture::wood(uint32_t seed) {
    char key[64];
    std::snprintf(key, sizeof(key), "#wood:%u", static_cast<unsigned>(seed));

    return cachedProcedural(key, 256, [seed](std::vector<uint8_t>& px, int s) {
        const float inv = 1.0f / static_cast<float>(s);

        for (int y = 0; y < s; ++y) {
            const float v = (y + 0.5f) * inv;
            for (int x = 0; x < s; ++x) {
                const float u = (x + 0.5f) * inv;

                // Halqalarni burab yuboruvchi shovqin (tekis chiziq bo'lib qolmasin)
                const float warp = tileFbm(u, v, 2, 2, seed + 13u, 3) - 0.5f;
                // Halqalar u o'qi bo'ylab (butun chastota -> takrorlanadi)
                const float rings = 0.5f + 0.5f * std::sin((u * 7.0f + warp * 0.9f) * TAU);
                // Uzun tolalar: v bo'ylab cho'zilgan shovqin
                const float fiber = tileFbm(u, v, 3, 96, seed + 977u, 2);

                float t = saturate(rings * 0.62f + fiber * 0.38f);
                t = t * t * (3.0f - 2.0f * t);           // kontrastni oshirish

                // To'q jigarrangdan och asal rangigacha
                float rr = lerpf( 88.0f, 186.0f, t);
                float gg = lerpf( 56.0f, 138.0f, t);
                float bb = lerpf( 30.0f,  84.0f, t);

                // Ingichka to'q tola chiziqlari
                const float streak = tileFbm(u, v, 6, 160, seed + 4231u, 1);
                if (streak > 0.72f) {
                    const float k = saturate((streak - 0.72f) / 0.28f) * 0.42f;
                    rr = lerpf(rr, 58.0f, k);
                    gg = lerpf(gg, 36.0f, k);
                    bb = lerpf(bb, 20.0f, k);
                }

                uint8_t* p = &px[(static_cast<size_t>(y) * s + x) * 4u];
                p[0] = toByte(rr); p[1] = toByte(gg); p[2] = toByte(bb); p[3] = 255;
            }
        }

        // --- Kichik shoxchalar (tugunlar) ---
        Rng rng(seed * 22695477u + 3u);
        const int knots = 3;
        for (int i = 0; i < knots; ++i) {
            const int   cx = rng.rangeI(0, s - 1);
            const int   cy = rng.rangeI(0, s - 1);
            const float rad = rng.range(4.0f, 9.0f);
            const int   ir = static_cast<int>(rad) + 2;
            for (int dy = -ir; dy <= ir; ++dy) {
                for (int dx = -ir; dx <= ir; ++dx) {
                    const float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (d > rad) continue;
                    const float k = (1.0f - d / rad) * 0.75f;
                    blendPx(px, s, cx + dx, cy + dy, 52.0f, 32.0f, 18.0f, k);
                }
            }
        }
    });
}

} // namespace ert
