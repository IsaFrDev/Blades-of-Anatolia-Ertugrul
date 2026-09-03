# Platforma auditi va ko'chirish rejasi

O'yin **faqat Windows** da ishlaydi — bu ataylab qilingan qaror (tashqi kutubxonasiz,
Win32 + WGL). Bu hujjat "Faqat Windows" cheklovini yopish uchun NIMA kerakligini
aniq ko'rsatadi: qaysi fayllar platformaga bog'liq, qaysilari toza, va ko'chirish
qanday bosqichlarda bo'ladi.

## 1. Bog'liqlik xaritasi (2026-09)

`windows.h` ni o'z ichiga olgan fayllar va ulardagi Win32 chaqiruvlari soni:

| Fayl | Win32 | Nima uchun |
|---|---|---|
| `game/src/app/Win32Main.cpp` | 20 | Oyna, WGL konteksti, xabar sikli, klaviatura/sichqoncha |
| `game/src/app/App.cpp` | 25 | `wglSwapIntervalEXT`, `wglGetCurrentContext`, GL sarlavhalari (`GL/gl.h` windows.h ni talab qiladi) |
| `game/src/gfx/Texture.cpp` | 14 | GDI+ orqali JPEG/PNG dekodlash |
| `game/src/audio/Audio.cpp` | 13 | `waveOut` mikser chiqishi |
| `game/src/gfx/Font.cpp` | 5 | GDI orqali shrift atlasi rasterlash |
| `game/src/gfx/Mesh.cpp` | 4 | `wglGetCurrentContext` (display list qurishdan oldin kontekst bormi) |
| `game/src/gfx/ShadowMap.cpp`, `Pbr.cpp` | 3 | `wglGetProcAddress` — GL kengaytma funksiyalari |
| `game/src/audio/Voice.cpp` | 2 | SAPI (faqat WAV topilmaganda zaxira) |
| `game/src/app/Bindings.cpp` | 2 | VK_* tugma kodlari |

**Toza (platformasiz) modullar:** `world/*` (Terrain, Level, Physics), `game/*`
(Character, Enemy, Encounter, Combat, Projectile, Cutscene, Episodes), `gfx/Skin.cpp`,
`ui/Menu.cpp`, `app/Input.cpp`, `loc/*`, `core/*`. Bu — kodning ~75% i.

## 2. Nima ko'chirish kerak (bosqichlar)

1. **GL yuklash qatlami** — `GL/gl.h` + `wglGetProcAddress` o'rniga bitta
   `gfx/GlLoader.h`: Windows da `wglGetProcAddress`, Linux da `glXGetProcAddress`,
   macOS da `dlsym`. ShadowMap/Pbr/App shu qatlamdan foydalanadi. (1 kun)
2. **Oyna + kiritish** — `Win32Main.cpp` ning aynan bir xil kontraktli
   `X11Main.cpp` (Xlib + GLX) yoki `CocoaMain.mm` nusxasi. Input.cpp allaqachon
   platformasiz (tugma kodlari Bindings orqali). (2–3 kun)
3. **Audio chiqishi** — `Audio.cpp` dagi `waveOut` o'rniga ALSA (`snd_pcm_writei`)
   yoki CoreAudio. Mikser (WAV yuklash, aralashtirish, qayta namunalash) toza C++. (1 kun)
4. **Rasm dekodlash** — GDI+ o'rniga o'zimizning PNG (zlib inflate ~400 qator) va
   baseline JPEG dekoderi (~900 qator), yoki barcha teksturalarni oldindan `.tga`/`.dds`
   ga aylantiruvchi `tools/convert_textures.py`. Ikkinchisi arzon va tezroq. (1 kun)
5. **Shrift** — GDI rasterlash o'rniga oldindan yaratilgan atlas (`tools/make_font_atlas.py`,
   PIL bilan) — bu Windows da ham foydali (bir xil ko'rinish). (0.5 kun)
6. **SAPI** — zaxira yo'l; boshqa platformada shunchaki o'chiriladi (barcha 311 replika
   allaqachon WAV). (0)

Jami baho: **~1 hafta**, kodning 25% i tegadi. O'yin mantig'i, fizika, animatsiya,
cutscene, lokalizatsiya — o'zgarmaydi.

## 3. Hozir qilingani

- Runtime statik bog'langan (`-static`): exe MinGW DLL siz, Git Bash/PowerShell/Explorer.
- GL kengaytmalari faqat `wglGetProcAddress` orqali ikki faylda (ShadowMap, Pbr) — 1-bosqich
  uchun almashtirish nuqtasi bitta.
- `CMakeLists.txt` boshqa platformada `FATAL_ERROR` beradi — yashirin buzilish emas, ochiq.
