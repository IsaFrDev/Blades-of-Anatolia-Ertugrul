#pragma once
// GDI+ orqali JPG/PNG/BMP dekodlash -> OpenGL teksturasi. Tashqi kutubxona yo'q.
#include <string>
#include <vector>
#include <cstdint>

namespace ert {

class Texture {
public:
    // Butun dastur uchun bir marta (GDI+ ishga tushirish)
    static bool initImaging();
    static void shutdownImaging();

    Texture() = default;
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Fayldan yuklab, GL teksturasini yaratadi. maxSize dan katta rasmlar kichraytiriladi.
    bool loadFile(const std::string& path, int maxSize = 1024);
    // Tayyor RGBA piksel massividan
    bool createRGBA(const uint8_t* rgba, int w, int h, bool mipmap = true, bool repeat = true);
    void destroy();

    void bind() const;
    static void unbind();

    bool valid()  const { return id_ != 0; }
    int  width()  const { return w_; }
    int  height() const { return h_; }
    unsigned id() const { return id_; }
    const std::string& path() const { return path_; }

    // --- Kesh: bir xil yo'l ikkinchi marta yuklanmaydi ---
    // Yuklab bo'lmasa nullptr emas, balki protsedural o'rinbosar qaytaradi (hech qachon nullptr).
    static Texture* get(const std::string& path, int maxSize = 1024);
    static void     clearCache();

    // --- Protsedural teksturalar (fayl talab qilmaydi) ---
    static Texture* solid(uint8_t r, uint8_t g, uint8_t b);
    static Texture* checker(uint8_t r1,uint8_t g1,uint8_t b1, uint8_t r2,uint8_t g2,uint8_t b2, int cells = 8, int size = 128);
    static Texture* noise(uint8_t r,uint8_t g,uint8_t b, float amount, uint32_t seed, int size = 256);
    // O't / tuproq / tosh / mato / yog'och kabi uslublangan teksturalar
    static Texture* grass(uint32_t seed = 7);
    static Texture* dirt (uint32_t seed = 11);
    static Texture* rock (uint32_t seed = 23);
    static Texture* cloth(uint8_t r, uint8_t g, uint8_t b, uint32_t seed = 31);
    static Texture* wood (uint32_t seed = 41);

    // GL'siz dekodlash (CPU tahlili uchun). Muvaffaqiyatsizlikda false.
    static bool decodeFile(const std::string& path, std::vector<uint8_t>& outRGBA, int& w, int& h, int maxSize);

private:
    unsigned    id_ = 0;
    int         w_ = 0, h_ = 0;
    std::string path_;
};

} // namespace ert
