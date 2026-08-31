#pragma once
// GDI bilan chizilgan glif atlasi -> OpenGL. UTF-8 kirish, to'liq lotin+kirill+turkcha belgilar.
#include <string>
#include <cstdint>

namespace ert {

enum class TextAlign { Left, Center, Right };

class Font {
public:
    Font() = default;
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // faceName: "Segoe UI", "Georgia" ... pixelHeight: piksel balandligi
    bool create(const char* faceName, int pixelHeight, bool bold = false, bool italic = false);
    void destroy();
    bool valid() const { return atlasTex_ != 0; }

    float lineHeight() const { return lineHeight_; }
    float ascent()     const { return ascent_; }
    // UTF-8 satrning piksel kengligi
    float measure(const std::string& utf8) const;

    // begin2D() ichida chaqirilishi kerak. (x,y) = matn boshining chap-yuqori burchagi (Left uchun).
    void draw(const std::string& utf8, float x, float y,
              float r, float g, float b, float a = 1.0f,
              TextAlign align = TextAlign::Left) const;
    // Soyali variant (o'qilishi oson)
    void drawShadowed(const std::string& utf8, float x, float y,
                      float r, float g, float b, float a = 1.0f,
                      TextAlign align = TextAlign::Left, float shadowOfs = 2.0f) const;
    // Kenglikka sig'diradi, satrlarga bo'ladi; chizilgan satrlar sonini qaytaradi.
    int  drawWrapped(const std::string& utf8, float x, float y, float maxWidth,
                     float r, float g, float b, float a = 1.0f,
                     TextAlign align = TextAlign::Left) const;
    int  wrappedLineCount(const std::string& utf8, float maxWidth) const;

private:
    struct Glyph { float u0, v0, u1, v1; float w, h, advance, bearingX, bearingY; };
    // Amalga oshirish detallari .cpp ichida
    unsigned atlasTex_ = 0;
    int      atlasW_ = 0, atlasH_ = 0;
    float    lineHeight_ = 0.0f, ascent_ = 0.0f;
    void*    impl_ = nullptr;   // GdiState*
};

// --- 2D qatlam yordamchilari (menyu / HUD / subtitr) ---
void begin2D(int screenW, int screenH);
void end2D();
void drawRect(float x, float y, float w, float h, float r, float g, float b, float a);
void drawRectOutline(float x, float y, float w, float h, float thickness, float r, float g, float b, float a);
void drawGradientRect(float x, float y, float w, float h,
                      float r0,float g0,float b0,float a0,
                      float r1,float g1,float b1,float a1);
// Butun ekranni qoplovchi rang (fade uchun)
void drawFullscreenFade(int screenW, int screenH, float r, float g, float b, float a);
// Kino chiziqlari (letterbox). amount 0..1
void drawLetterbox(int screenW, int screenH, float amount);
// Teksturali to'rtburchak (fon rasmi uchun)
void drawTexturedRect(unsigned texId, float x, float y, float w, float h, float r, float g, float b, float a);

} // namespace ert
