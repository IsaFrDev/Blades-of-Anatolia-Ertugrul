// Ertugrul :: gfx/Font.cpp
// GDI bilan chizilgan glif atlasi -> OpenGL 1.1 teksturasi.
// UTF-8 kirish; lotin-1, lotin kengaytmasi (turkcha), kirill va o'zbekcha
// apostrof belgilari oldindan atlasga joylanadi.
// Tashqi kutubxona yo'q: faqat Win32 GDI + OpenGL.

#include "ertugrul/gfx/Font.h"

// DIQQAT: windows.h har doim GL sarlavhalaridan OLDIN
#include <windows.h>
#include <GL/gl.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <iostream>

namespace ert {

// ============================================================================
//  Ichki tuzilmalar va yordamchilar
// ============================================================================
namespace {

// GL 1.2 dagi CLAMP_TO_EDGE (GL 1.1 sarlavhasida yo'q)
constexpr GLenum kClampToEdge = 0x812F;

// Bitta glif haqidagi ma'lumot (atlasdagi UV + metrikalar).
// Font::Glyph header'da private, shuning uchun .cpp da o'z nusxamiz.
struct GlyphInfo {
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    float w = 0.0f, h = 0.0f;
    float advance = 0.0f;
    float bearingX = 0.0f;   // satr boshidan glif pikselining chap chekkasigacha
    float bearingY = 0.0f;   // satr yuqorisidan glif pikselining ustki chekkasigacha
};

// Font::impl_ ichida saqlanadigan holat
struct GdiState {
    std::unordered_map<uint32_t, GlyphInfo> glyphs;
    GlyphInfo fallback;            // topilmagan belgi uchun ('?')
    bool      hasFallback = false;
    float     spaceAdvance = 0.0f; // tab kengligini hisoblash uchun
};

// Atlas qurilishi paytidagi vaqtinchalik glif
struct PendingGlyph {
    uint32_t             cp = 0;
    int                  w = 0, h = 0;
    int                  bearingX = 0, bearingY = 0;
    float                advance = 0.0f;
    std::vector<uint8_t> alpha;    // w*h, 8-bit qoplama
};

// --- GL konteksti mavjudmi? ---
inline bool hasGLContext() {
    return wglGetCurrentContext() != nullptr;
}

// --- UTF-8 -> UTF-16 (Win32 uchun) ---
std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    const int need = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    if (need <= 0) return std::wstring();
    std::wstring w(static_cast<size_t>(need), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], need);
    return w;
}

// --- UTF-8 -> UTF-32 dekoderi ---
// Noto'g'ri baytlar xavfsiz o'tkazib yuboriladi (crash bo'lmasin).
void utf8ToCodepoints(const std::string& s, std::vector<uint32_t>& out) {
    out.clear();
    out.reserve(s.size());
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    const size_t n = s.size();
    size_t i = 0;

    while (i < n) {
        const unsigned char c = p[i];

        if (c < 0x80u) {                       // 1 bayt: ASCII
            out.push_back(c);
            ++i;
            continue;
        }
        if ((c & 0xE0u) == 0xC0u) {            // 2 bayt
            if (i + 1 < n && (p[i + 1] & 0xC0u) == 0x80u) {
                const uint32_t cp = ((c & 0x1Fu) << 6) | (p[i + 1] & 0x3Fu);
                if (cp >= 0x80u) out.push_back(cp);   // overlong emas
                i += 2;
                continue;
            }
            ++i;                               // buzuq — o'tkazib yuboramiz
            continue;
        }
        if ((c & 0xF0u) == 0xE0u) {            // 3 bayt
            if (i + 2 < n && (p[i + 1] & 0xC0u) == 0x80u && (p[i + 2] & 0xC0u) == 0x80u) {
                const uint32_t cp = ((c & 0x0Fu) << 12) | ((p[i + 1] & 0x3Fu) << 6) | (p[i + 2] & 0x3Fu);
                // Surrogat oralig'i va overlong'ni tashlaymiz
                if (cp >= 0x800u && !(cp >= 0xD800u && cp <= 0xDFFFu)) out.push_back(cp);
                i += 3;
                continue;
            }
            ++i;
            continue;
        }
        if ((c & 0xF8u) == 0xF0u) {            // 4 bayt
            if (i + 3 < n && (p[i + 1] & 0xC0u) == 0x80u && (p[i + 2] & 0xC0u) == 0x80u
                          && (p[i + 3] & 0xC0u) == 0x80u) {
                const uint32_t cp = ((c & 0x07u) << 18) | ((p[i + 1] & 0x3Fu) << 12)
                                  | ((p[i + 2] & 0x3Fu) << 6) | (p[i + 3] & 0x3Fu);
                if (cp >= 0x10000u && cp <= 0x10FFFFu) out.push_back(cp);
                i += 4;
                continue;
            }
            ++i;
            continue;
        }
        ++i;                                   // 0x80..0xBF yoki 0xF8+ — yaroqsiz
    }
}

// --- Glif izlash (topilmasa '?' ga tushamiz) ---
inline const GlyphInfo* findGlyph(const GdiState* st, uint32_t cp) {
    if (!st) return nullptr;
    std::unordered_map<uint32_t, GlyphInfo>::const_iterator it = st->glyphs.find(cp);
    if (it != st->glyphs.end()) return &it->second;
    if (st->hasFallback) return &st->fallback;
    return nullptr;
}

// Bitta belgining gorizontal siljishi (tab -> 4 ta bo'shliq)
inline float advanceOf(const GdiState* st, uint32_t cp) {
    if (cp == '\n' || cp == '\r') return 0.0f;
    if (cp == '\t') return st ? st->spaceAdvance * 4.0f : 0.0f;
    const GlyphInfo* g = findGlyph(st, cp);
    return g ? g->advance : 0.0f;
}

float measureRun(const GdiState* st, const uint32_t* cps, size_t n) {
    if (!st || !cps) return 0.0f;
    float w = 0.0f;
    for (size_t i = 0; i < n; ++i) w += advanceOf(st, cps[i]);
    return w;
}

// --- Bitta satrni chizish ---
void drawRun(const GdiState* st, unsigned tex, float lineHeight,
             const uint32_t* cps, size_t n,
             float x, float y, float r, float g, float b, float a, TextAlign align) {
    (void)lineHeight;
    if (!st || !cps || n == 0 || tex == 0 || !hasGLContext()) return;

    // Tekislash
    if (align == TextAlign::Center)      x -= measureRun(st, cps, n) * 0.5f;
    else if (align == TextAlign::Right)  x -= measureRun(st, cps, n);

    // Holatni saqlaymiz (chizishdan keyin to'liq tiklanadi)
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(tex));
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(r, g, b, a);

    glBegin(GL_QUADS);
    float pen = x;
    for (size_t i = 0; i < n; ++i) {
        const uint32_t cp = cps[i];
        if (cp == '\n' || cp == '\r') continue;
        if (cp == '\t') { pen += st->spaceAdvance * 4.0f; continue; }

        const GlyphInfo* gi = findGlyph(st, cp);
        if (!gi) continue;

        if (gi->w > 0.0f && gi->h > 0.0f) {
            const float gx0 = pen + gi->bearingX;
            const float gy0 = y   + gi->bearingY;
            const float gx1 = gx0 + gi->w;
            const float gy1 = gy0 + gi->h;

            glTexCoord2f(gi->u0, gi->v0); glVertex2f(gx0, gy0);
            glTexCoord2f(gi->u1, gi->v0); glVertex2f(gx1, gy0);
            glTexCoord2f(gi->u1, gi->v1); glVertex2f(gx1, gy1);
            glTexCoord2f(gi->u0, gi->v1); glVertex2f(gx0, gy1);
        }
        pen += gi->advance;
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glPopAttrib();
}

// --- So'z chegarasi bo'yicha o'rash ---
// So'z sig'masa — belgi bo'yicha majburiy uzish.
void wrapRun(const GdiState* st, const std::vector<uint32_t>& cps, float maxWidth,
             std::vector<std::vector<uint32_t> >& lines) {
    lines.clear();
    if (!st || cps.empty()) return;

    const bool noWrap = !(maxWidth > 0.0f);

    std::vector<uint32_t> cur;
    float curW = 0.0f;
    size_t i = 0;
    const size_t n = cps.size();

    while (i < n) {
        // Qattiq satr uzilishi
        if (cps[i] == '\n') {
            lines.push_back(cur);
            cur.clear();
            curW = 0.0f;
            ++i;
            continue;
        }
        if (cps[i] == '\r') { ++i; continue; }

        // Bo'shliqlar bo'lagi
        size_t ws = i;
        while (ws < n && (cps[ws] == ' ' || cps[ws] == '\t')) ++ws;
        // So'z bo'lagi
        size_t we = ws;
        while (we < n && cps[we] != ' ' && cps[we] != '\t' && cps[we] != '\n' && cps[we] != '\r') ++we;

        if (ws == we) {
            // Faqat bo'shliqlar qoldi (satr oxiri yoki '\n' oldidan)
            if (!cur.empty()) {
                for (size_t k = i; k < ws; ++k) { cur.push_back(cps[k]); curW += advanceOf(st, cps[k]); }
            }
            i = ws;
            continue;
        }

        float spW = 0.0f;
        for (size_t k = i; k < ws; ++k) spW += advanceOf(st, cps[k]);
        float wdW = 0.0f;
        for (size_t k = ws; k < we; ++k) wdW += advanceOf(st, cps[k]);

        if (cur.empty()) spW = 0.0f;           // satr boshidagi bo'shliqlar tashlanadi

        if (!noWrap && !cur.empty() && (curW + spW + wdW) > maxWidth) {
            lines.push_back(cur);
            cur.clear();
            curW = 0.0f;
            spW  = 0.0f;
        }

        // Satr o'rtasidagi bo'shliqlarni qo'shamiz (satr boshidagilar tashlangan)
        if (!cur.empty()) {
            for (size_t k = i; k < ws; ++k) { cur.push_back(cps[k]); curW += advanceOf(st, cps[k]); }
        }

        if (!noWrap && wdW > maxWidth) {
            // --- So'z bir satrga ham sig'maydi: belgi bo'yicha majburiy uzish ---
            for (size_t k = ws; k < we; ++k) {
                const float aw = advanceOf(st, cps[k]);
                if (!cur.empty() && (curW + aw) > maxWidth) {
                    lines.push_back(cur);
                    cur.clear();
                    curW = 0.0f;
                }
                cur.push_back(cps[k]);
                curW += aw;
            }
        } else {
            for (size_t k = ws; k < we; ++k) { cur.push_back(cps[k]); curW += advanceOf(st, cps[k]); }
        }

        i = we;
    }

    if (!cur.empty() || lines.empty()) lines.push_back(cur);
}

// --- Atlasga joylashtirish (javon / shelf usuli) ---
bool packShelf(const std::vector<PendingGlyph>& gs, const std::vector<int>& order, int size,
               std::vector<int>& outX, std::vector<int>& outY) {
    outX.assign(gs.size(), 0);
    outY.assign(gs.size(), 0);

    int penX = 1, penY = 1, rowH = 0;
    for (size_t k = 0; k < order.size(); ++k) {
        const int idx = order[k];
        const PendingGlyph& g = gs[static_cast<size_t>(idx)];
        if (g.w <= 0 || g.h <= 0) continue;                    // bo'sh glif (masalan, probel)
        if (g.w + 2 > size || g.h + 2 > size) return false;

        if (penX + g.w + 1 > size) { penX = 1; penY += rowH + 1; rowH = 0; }
        if (penY + g.h + 1 > size) return false;

        outX[static_cast<size_t>(idx)] = penX;
        outY[static_cast<size_t>(idx)] = penY;
        penX += g.w + 1;
        if (g.h > rowH) rowH = g.h;
    }
    return true;
}

// --- Atlasga kiritiladigan kod nuqtalari ro'yxati ---
void buildCodepointList(std::vector<uint32_t>& cps) {
    cps.clear();
    cps.reserve(768);

    // Lotin-1 (ASCII + G'arbiy Yevropa: « » ° é ü ...)
    for (uint32_t c = 0x0020u; c <= 0x00FFu; ++c) cps.push_back(c);
    // Lotin kengaytmasi A (turkcha: ç ğ ı İ ö ş ü Ğ Ş, shuningdek ā ē ...)
    for (uint32_t c = 0x0100u; c <= 0x017Fu; ++c) cps.push_back(c);
    // Kirill (sarlavha izohida talab qilingan)
    for (uint32_t c = 0x0400u; c <= 0x045Fu; ++c) cps.push_back(c);
    cps.push_back(0x0490u); cps.push_back(0x0491u);   // Ґ ґ

    // O'zbekcha o' / g' uchun modifikator apostroflar va tipografik tirnoqlar
    cps.push_back(0x02BBu);   // ʻ  (o'zbekcha to'g'ri belgi)
    cps.push_back(0x02BCu);   // ʼ
    cps.push_back(0x2018u);   // '
    cps.push_back(0x2019u);   // '
    cps.push_back(0x201Cu);   // "
    cps.push_back(0x201Du);   // "
    cps.push_back(0x201Eu);   // „
    // Tire va ellipsis
    cps.push_back(0x2013u);   // –
    cps.push_back(0x2014u);   // —
    cps.push_back(0x2026u);   // …
    // Qo'shtirnoq-burchaklar (00AB/00BB lotin-1 da bor, lekin kafolat uchun)
    cps.push_back(0x00ABu);
    cps.push_back(0x00BBu);
    // Foydali qo'shimchalar
    cps.push_back(0x2022u);   // •
    cps.push_back(0x2190u); cps.push_back(0x2192u);   // ← →
    cps.push_back(0x25B6u);   // ▶
    cps.push_back(0x2605u); cps.push_back(0x2606u);   // ★ ☆
}

// --- 2D qatlam holati (nesting hisoblagichi) ---
struct State2D {
    int nest = 0;
    int w = 0, h = 0;
};
State2D& s2d() {
    static State2D s;
    return s;
}

// 2D chizish uchun umumiy tayyorgarlik (begin2D dan tashqarida ham xavfsiz)
inline void begin2DShape() {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

} // anonim namespace

// ============================================================================
//  Font
// ============================================================================

Font::~Font() {
    destroy();
}

void Font::destroy() {
    if (atlasTex_ != 0) {
        if (hasGLContext()) {
            const GLuint t = static_cast<GLuint>(atlasTex_);
            glDeleteTextures(1, &t);
        }
        atlasTex_ = 0;
    }
    if (impl_) {
        delete static_cast<GdiState*>(impl_);
        impl_ = nullptr;
    }
    atlasW_ = 0;
    atlasH_ = 0;
    lineHeight_ = 0.0f;
    ascent_ = 0.0f;
}

bool Font::create(const char* faceName, int pixelHeight, bool bold, bool italic) {
    destroy();

    if (pixelHeight < 6)   pixelHeight = 6;
    if (pixelHeight > 200) pixelHeight = 200;

    if (!hasGLContext()) {
        std::cerr << "[Font] OGOHLANTIRISH: GL konteksti yo'q, shrift atlasi qurilmadi.\n";
        return false;
    }

    // --- GDI: xotira konteksti ---
    HDC screenDC = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screenDC);
    if (screenDC) ReleaseDC(nullptr, screenDC);
    if (!dc) {
        std::cerr << "[Font] OGOHLANTIRISH: CreateCompatibleDC muvaffaqiyatsiz.\n";
        return false;
    }

    // --- Shrift ---
    std::wstring wface = (faceName != nullptr) ? utf8ToWide(std::string(faceName)) : std::wstring();
    if (wface.empty()) wface = L"Segoe UI";
    if (wface.size() > 31) wface.resize(31);          // LOGFONT chegarasi

    HFONT hFont = CreateFontW(-pixelHeight, 0, 0, 0,
                              bold ? FW_BOLD : FW_NORMAL,
                              italic ? TRUE : FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                              ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              wface.c_str());
    if (!hFont) {
        // Zaxira yo'l: tizimda albatta bor shrift
        hFont = CreateFontW(-pixelHeight, 0, 0, 0,
                            bold ? FW_BOLD : FW_NORMAL,
                            italic ? TRUE : FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    }
    if (!hFont) {
        DeleteDC(dc);
        std::cerr << "[Font] OGOHLANTIRISH: CreateFontW muvaffaqiyatsiz.\n";
        return false;
    }

    HGDIOBJ oldFont = SelectObject(dc, hFont);

    TEXTMETRICW tm;
    std::memset(&tm, 0, sizeof(tm));
    if (!GetTextMetricsW(dc, &tm)) {
        tm.tmHeight = pixelHeight;
        tm.tmAscent = static_cast<LONG>(pixelHeight * 0.8f);
        tm.tmExternalLeading = 0;
    }
    lineHeight_ = static_cast<float>(tm.tmHeight + tm.tmExternalLeading);
    ascent_     = static_cast<float>(tm.tmAscent);
    if (lineHeight_ < 1.0f) lineHeight_ = static_cast<float>(pixelHeight);

    // --- Chizish uchun vaqtinchalik DIB (yuqoridan-pastga, 32-bit) ---
    const int pad   = (pixelHeight / 4 > 8) ? (pixelHeight / 4) : 8;
    const int cellW = pixelHeight * 4 + pad * 2 + 8;
    const int cellH = static_cast<int>(tm.tmHeight) + pad * 2 + 8;

    BITMAPINFO bi;
    std::memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = cellW;
    bi.bmiHeader.biHeight      = -cellH;      // manfiy = top-down
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* dibBits = nullptr;
    HBITMAP dib = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &dibBits, nullptr, 0);
    if (!dib || !dibBits) {
        SelectObject(dc, oldFont);
        DeleteObject(hFont);
        if (dib) DeleteObject(dib);
        DeleteDC(dc);
        std::cerr << "[Font] OGOHLANTIRISH: CreateDIBSection muvaffaqiyatsiz.\n";
        return false;
    }
    HGDIOBJ oldBmp = SelectObject(dc, dib);

    SetBkMode(dc, OPAQUE);
    SetBkColor(dc, RGB(0, 0, 0));
    SetTextColor(dc, RGB(255, 255, 255));
    SetTextAlign(dc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

    // --- Barcha kerakli kod nuqtalarini rasterlash ---
    std::vector<uint32_t> wanted;
    buildCodepointList(wanted);

    std::vector<PendingGlyph> pend;
    pend.reserve(wanted.size());

    const uint8_t* bits = static_cast<const uint8_t*>(dibBits);
    const size_t   rowStride = static_cast<size_t>(cellW) * 4u;
    RECT fullRect = { 0, 0, cellW, cellH };

    for (size_t wi = 0; wi < wanted.size(); ++wi) {
        const uint32_t cp = wanted[wi];

        wchar_t wc[2];
        int wlen = 1;
        if (cp <= 0xFFFFu) {
            wc[0] = static_cast<wchar_t>(cp);
        } else {
            const uint32_t v = cp - 0x10000u;
            wc[0] = static_cast<wchar_t>(0xD800u + (v >> 10));
            wc[1] = static_cast<wchar_t>(0xDC00u + (v & 0x3FFu));
            wlen = 2;
        }

        // --- Siljish (advance) ---
        float adv = 0.0f;
        ABCFLOAT abc;
        std::memset(&abc, 0, sizeof(abc));
        if (wlen == 1 && GetCharABCWidthsFloatW(dc, wc[0], wc[0], &abc)) {
            adv = abc.abcfA + abc.abcfB + abc.abcfC;
        }
        if (!(adv > 0.0f)) {
            SIZE sz;
            sz.cx = 0; sz.cy = 0;
            if (GetTextExtentPoint32W(dc, wc, wlen, &sz)) adv = static_cast<float>(sz.cx);
        }
        if (!(adv >= 0.0f)) adv = 0.0f;

        // --- Glifni chizish (oq matn qora fonda) ---
        ExtTextOutW(dc, pad, pad, ETO_OPAQUE, &fullRect, wc, static_cast<UINT>(wlen), nullptr);
        GdiFlush();

        // --- Chegaralarni topish ---
        int minX = cellW, minY = cellH, maxX = -1, maxY = -1;
        for (int y = 0; y < cellH; ++y) {
            const uint8_t* row = bits + rowStride * static_cast<size_t>(y);
            for (int x = 0; x < cellW; ++x) {
                // ANTIALIASED_QUALITY -> B=G=R=qoplama; ClearType bo'lsa maksimumini olamiz
                uint8_t c = row[x * 4 + 0];
                if (row[x * 4 + 1] > c) c = row[x * 4 + 1];
                if (row[x * 4 + 2] > c) c = row[x * 4 + 2];
                if (c != 0) {
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }

        PendingGlyph pg;
        pg.cp = cp;
        pg.advance = adv;

        if (maxX >= minX && maxY >= minY) {
            pg.w = maxX - minX + 1;
            pg.h = maxY - minY + 1;
            pg.bearingX = minX - pad;
            pg.bearingY = minY - pad;
            pg.alpha.assign(static_cast<size_t>(pg.w) * static_cast<size_t>(pg.h), 0);
            for (int y = 0; y < pg.h; ++y) {
                const uint8_t* row = bits + rowStride * static_cast<size_t>(minY + y);
                uint8_t* dst = &pg.alpha[static_cast<size_t>(y) * pg.w];
                for (int x = 0; x < pg.w; ++x) {
                    const int sx = minX + x;
                    uint8_t c = row[sx * 4 + 0];
                    if (row[sx * 4 + 1] > c) c = row[sx * 4 + 1];
                    if (row[sx * 4 + 2] > c) c = row[sx * 4 + 2];
                    dst[x] = c;
                }
            }
        }
        // Aks holda bo'sh glif (probel va h.k.) — faqat advance ishlatiladi

        pend.push_back(pg);
    }

    // --- GDI resurslarini bo'shatamiz (endi kerak emas) ---
    SelectObject(dc, oldBmp);
    SelectObject(dc, oldFont);
    DeleteObject(dib);
    DeleteObject(hFont);
    DeleteDC(dc);

    // --- Joylashtirish tartibi: balandligi bo'yicha kamayish (javon zich bo'ladi) ---
    std::vector<int> order;
    order.reserve(pend.size());
    for (size_t i = 0; i < pend.size(); ++i) order.push_back(static_cast<int>(i));
    std::sort(order.begin(), order.end(), [&pend](int a, int b) {
        const PendingGlyph& ga = pend[static_cast<size_t>(a)];
        const PendingGlyph& gb = pend[static_cast<size_t>(b)];
        if (ga.h != gb.h) return ga.h > gb.h;
        return ga.w > gb.w;
    });

    // --- Atlas o'lchamini avtomatik tanlash ---
    const int trySizes[] = { 256, 512, 1024, 2048, 4096 };
    int atlasSize = 0;
    std::vector<int> gx, gy;
    for (size_t k = 0; k < sizeof(trySizes) / sizeof(trySizes[0]); ++k) {
        if (packShelf(pend, order, trySizes[k], gx, gy)) {
            atlasSize = trySizes[k];
            break;
        }
    }
    if (atlasSize == 0) {
        std::cerr << "[Font] OGOHLANTIRISH: glif atlasi 4096x4096 ga sig'madi.\n";
        return false;
    }

    // --- Atlas piksellarini yig'ish: RGB = oq, A = qoplama ---
    std::vector<uint8_t> atlas;
    try {
        atlas.assign(static_cast<size_t>(atlasSize) * atlasSize * 4u, 0);
    } catch (...) {
        std::cerr << "[Font] OGOHLANTIRISH: atlas uchun xotira yetmadi.\n";
        return false;
    }
    for (size_t i = 0; i + 3 < atlas.size(); i += 4) {
        atlas[i + 0] = 255;
        atlas[i + 1] = 255;
        atlas[i + 2] = 255;
        atlas[i + 3] = 0;
    }

    GdiState* st = new GdiState();
    st->glyphs.reserve(pend.size() * 2u);

    const float invSize = 1.0f / static_cast<float>(atlasSize);

    for (size_t i = 0; i < pend.size(); ++i) {
        const PendingGlyph& pg = pend[i];
        GlyphInfo gi;
        gi.advance  = pg.advance;
        gi.bearingX = static_cast<float>(pg.bearingX);
        gi.bearingY = static_cast<float>(pg.bearingY);
        gi.w = static_cast<float>(pg.w);
        gi.h = static_cast<float>(pg.h);

        if (pg.w > 0 && pg.h > 0) {
            const int px = gx[i];
            const int py = gy[i];
            // Alfani atlasga ko'chiramiz
            for (int y = 0; y < pg.h; ++y) {
                const uint8_t* src = &pg.alpha[static_cast<size_t>(y) * pg.w];
                uint8_t* dst = &atlas[((static_cast<size_t>(py + y) * atlasSize) + px) * 4u];
                for (int x = 0; x < pg.w; ++x) {
                    dst[x * 4 + 0] = 255;
                    dst[x * 4 + 1] = 255;
                    dst[x * 4 + 2] = 255;
                    dst[x * 4 + 3] = src[x];
                }
            }
            gi.u0 = static_cast<float>(px)         * invSize;
            gi.v0 = static_cast<float>(py)         * invSize;
            gi.u1 = static_cast<float>(px + pg.w)  * invSize;
            gi.v1 = static_cast<float>(py + pg.h)  * invSize;
        }

        st->glyphs[pg.cp] = gi;
    }

    // Zaxira glif: '?' (topilmagan kod nuqtalari uchun)
    {
        std::unordered_map<uint32_t, GlyphInfo>::const_iterator it = st->glyphs.find('?');
        if (it != st->glyphs.end()) { st->fallback = it->second; st->hasFallback = true; }
        else {
            it = st->glyphs.find(' ');
            if (it != st->glyphs.end()) { st->fallback = it->second; st->hasFallback = true; }
        }
        std::unordered_map<uint32_t, GlyphInfo>::const_iterator sp = st->glyphs.find(' ');
        st->spaceAdvance = (sp != st->glyphs.end()) ? sp->second.advance
                                                    : static_cast<float>(pixelHeight) * 0.35f;
        if (!(st->spaceAdvance > 0.0f)) st->spaceAdvance = static_cast<float>(pixelHeight) * 0.35f;
    }

    // --- GL teksturasi ---
    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (tex == 0) {
        delete st;
        std::cerr << "[Font] OGOHLANTIRISH: glGenTextures muvaffaqiyatsiz.\n";
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(kClampToEdge));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(kClampToEdge));
    if (glGetError() != GL_NO_ERROR) {
        // GL 1.1: CLAMP_TO_EDGE yo'q
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasSize, atlasSize, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, atlas.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    atlasTex_ = tex;
    atlasW_   = atlasSize;
    atlasH_   = atlasSize;
    impl_     = st;
    return true;
}

float Font::measure(const std::string& utf8) const {
    const GdiState* st = static_cast<const GdiState*>(impl_);
    if (!st || utf8.empty()) return 0.0f;

    std::vector<uint32_t> cps;
    utf8ToCodepoints(utf8, cps);

    // '\n' bo'lsa — eng keng satrni qaytaramiz
    float best = 0.0f, cur = 0.0f;
    for (size_t i = 0; i < cps.size(); ++i) {
        if (cps[i] == '\n') { if (cur > best) best = cur; cur = 0.0f; continue; }
        cur += advanceOf(st, cps[i]);
    }
    if (cur > best) best = cur;
    return best;
}

void Font::draw(const std::string& utf8, float x, float y,
                float r, float g, float b, float a, TextAlign align) const {
    const GdiState* st = static_cast<const GdiState*>(impl_);
    if (!st || atlasTex_ == 0 || utf8.empty()) return;

    std::vector<uint32_t> cps;
    utf8ToCodepoints(utf8, cps);
    if (cps.empty()) return;

    // '\n' bo'lsa satrlarga bo'lib chizamiz
    size_t start = 0;
    float  lineY = y;
    for (size_t i = 0; i <= cps.size(); ++i) {
        if (i == cps.size() || cps[i] == '\n') {
            if (i > start) {
                drawRun(st, atlasTex_, lineHeight_, &cps[start], i - start,
                        x, lineY, r, g, b, a, align);
            }
            lineY += lineHeight_;
            start = i + 1;
        }
    }
}

void Font::drawShadowed(const std::string& utf8, float x, float y,
                        float r, float g, float b, float a,
                        TextAlign align, float shadowOfs) const {
    if (atlasTex_ == 0 || !impl_) return;
    if (!(shadowOfs > 0.0f)) shadowOfs = 1.0f;
    // Avval soya, keyin asosiy matn
    draw(utf8, x + shadowOfs, y + shadowOfs, 0.0f, 0.0f, 0.0f, a * 0.72f, align);
    draw(utf8, x, y, r, g, b, a, align);
}

int Font::drawWrapped(const std::string& utf8, float x, float y, float maxWidth,
                      float r, float g, float b, float a, TextAlign align) const {
    const GdiState* st = static_cast<const GdiState*>(impl_);
    if (!st || atlasTex_ == 0 || utf8.empty()) return 0;

    std::vector<uint32_t> cps;
    utf8ToCodepoints(utf8, cps);
    if (cps.empty()) return 0;

    std::vector<std::vector<uint32_t> > lines;
    wrapRun(st, cps, maxWidth, lines);

    float lineY = y;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (!lines[i].empty()) {
            drawRun(st, atlasTex_, lineHeight_, lines[i].data(), lines[i].size(),
                    x, lineY, r, g, b, a, align);
        }
        lineY += lineHeight_;
    }
    return static_cast<int>(lines.size());
}

int Font::wrappedLineCount(const std::string& utf8, float maxWidth) const {
    const GdiState* st = static_cast<const GdiState*>(impl_);
    if (!st || utf8.empty()) return 0;

    std::vector<uint32_t> cps;
    utf8ToCodepoints(utf8, cps);
    if (cps.empty()) return 0;

    std::vector<std::vector<uint32_t> > lines;
    wrapRun(st, cps, maxWidth, lines);
    return static_cast<int>(lines.size());
}

// ============================================================================
//  2D qatlam yordamchilari
// ============================================================================

void begin2D(int screenW, int screenH) {
    State2D& s = s2d();
    if (s.nest++ > 0) return;                 // ichma-ich chaqiruv — holat allaqachon tayyor

    if (screenW < 1) screenW = 1;
    if (screenH < 1) screenH = 1;
    s.w = screenW;
    s.h = screenH;

    if (!hasGLContext()) return;

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(screenW), static_cast<GLdouble>(screenH), 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void end2D() {
    State2D& s = s2d();
    if (s.nest <= 0) return;                  // muvozanatsiz chaqiruv — e'tiborsiz qoldiramiz
    if (--s.nest > 0) return;

    if (!hasGLContext()) return;

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}

void drawRect(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (!hasGLContext()) return;
    if (!(w > 0.0f) || !(h > 0.0f)) return;

    begin2DShape();
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

void drawRectOutline(float x, float y, float w, float h, float thickness,
                     float r, float g, float b, float a) {
    if (!hasGLContext()) return;
    if (!(w > 0.0f) || !(h > 0.0f)) return;
    if (!(thickness > 0.0f)) thickness = 1.0f;

    float t = thickness;
    if (t > w * 0.5f) t = w * 0.5f;
    if (t > h * 0.5f) t = h * 0.5f;
    if (!(t > 0.0f)) t = 1.0f;

    // 4 ta yupqa to'rtburchak (burchaklar ustma-ust tushmasin)
    drawRect(x,         y,         w,           t,           r, g, b, a);  // yuqori
    drawRect(x,         y + h - t, w,           t,           r, g, b, a);  // past
    drawRect(x,         y + t,     t,           h - 2.0f * t, r, g, b, a); // chap
    drawRect(x + w - t, y + t,     t,           h - 2.0f * t, r, g, b, a); // o'ng
}

void drawGradientRect(float x, float y, float w, float h,
                      float r0, float g0, float b0, float a0,
                      float r1, float g1, float b1, float a1) {
    if (!hasGLContext()) return;
    if (!(w > 0.0f) || !(h > 0.0f)) return;

    begin2DShape();
    glBegin(GL_QUADS);
        glColor4f(r0, g0, b0, a0); glVertex2f(x,     y);
        glColor4f(r0, g0, b0, a0); glVertex2f(x + w, y);
        glColor4f(r1, g1, b1, a1); glVertex2f(x + w, y + h);
        glColor4f(r1, g1, b1, a1); glVertex2f(x,     y + h);
    glEnd();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void drawFullscreenFade(int screenW, int screenH, float r, float g, float b, float a) {
    if (screenW < 1 || screenH < 1) return;
    if (!(a > 0.0f)) return;
    drawRect(0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH), r, g, b, a);
}

void drawLetterbox(int screenW, int screenH, float amount) {
    if (screenW < 1 || screenH < 1) return;
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    if (amount <= 0.0f) return;

    const float H = static_cast<float>(screenH);
    const float W = static_cast<float>(screenW);
    const float bar = amount * (H * 0.12f);
    if (!(bar > 0.0f)) return;

    drawRect(0.0f, 0.0f,     W, bar, 0.0f, 0.0f, 0.0f, 1.0f);   // yuqori chiziq
    drawRect(0.0f, H - bar,  W, bar, 0.0f, 0.0f, 0.0f, 1.0f);   // pastki chiziq
}

void drawTexturedRect(unsigned texId, float x, float y, float w, float h,
                      float r, float g, float b, float a) {
    if (!hasGLContext()) return;
    if (!(w > 0.0f) || !(h > 0.0f)) return;

    if (texId == 0) {                          // tekstura yo'q — oddiy rangli to'rtburchak
        drawRect(x, y, w, h, r, g, b, a);
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texId));
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f(r, g, b, a);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x,     y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x,     y + h);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

} // namespace ert
