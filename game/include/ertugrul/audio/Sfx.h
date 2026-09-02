#pragma once
// Jang tovushlari — PROTSEDURAL sintez. Tashqi kutubxona ham, tovush fayli ham yo'q.
//
// Nima uchun sintez: loyihada faqat ovozli replikalar uchun WAV bor (912 ta),
// jang tovushlari yo'q edi. Audio::makeTone() sof sinus, makeNoise() oq shovqin
// beradi — ular yolg'iz o'zi "qilich zarbasi" bo'lib eshitilmaydi. Shuning uchun
// bu yerda konvert (envelope), garmonik bo'lmagan qismlar (inharmonic partials)
// va oddiy filtrlar bilan haqiqiy zarba tovushlari quriladi.
//
// Tovushlar BIR MARTA yaratiladi va keshda saqlanadi (har biri < 0.6 s).
#include "ertugrul/core/Math.h"

namespace ert {

enum class SfxId {
    Hit = 0,      // qilich tanaga tegdi — past "chuq" + qisqa shovqin
    Block,        // qalqonga tegdi — metall jaranglash
    Parry,        // mukammal parry — yorqinroq jarang + uzun ring
    Kill,         // o'ldiruvchi zarba — og'ir zarba + past pasayish
    Swing,        // havoda qilich yoyi (nishonga tegmasa ham eshitiladi)
    BowShot,      // kamon ipi
    ArrowHit,     // o'q tanaga
    ArrowWall,    // o'q devorga/yerga
    Death,        // tana yerga qulashi
    Pickup,       // o'q yig'ib olindi — yog'och "tak" + yengil jarang
    Count
};

class Sfx {
public:
    static Sfx& get();

    // Tovush bankini quradi (Audio::init() dan KEYIN bir marta chaqiriladi).
    // Ikkinchi marta chaqirilsa hech narsa qilmaydi.
    void build();
    void clear();
    bool ready() const;

    // Tovushni chalish. distM — tinglovchidan masofa (m); 0 dan katta bo'lsa
    // ovoz masofaga qarab so'nadi. varyN — variant indeksi (bir xil tovush
    // ketma-ket takrorlanganda quloqni charchatmasligi uchun ohangi biroz o'zgaradi).
    void play(SfxId id, float distM = 0.0f, float gain = 1.0f);

    // Tinglovchi joyi — masofa shundan hisoblanadi (odatda o'yinchi pozitsiyasi)
    void setListener(const Vec3& p);

private:
    Sfx() = default;
};

} // namespace ert
