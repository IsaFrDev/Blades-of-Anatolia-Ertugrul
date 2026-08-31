# AI video to'plami

Bu papka **AI bilan kinematik treyler** yaratish uchun kerak bo'lgan hamma
materialni jamlaydi.

## Nima bor

| Fayl | Nima |
|---|---|
| `prompts.md` | 16 ta kadr uchun to'liq promptlar (inglizcha) + o'zbekcha izohlar |
| `style_guide.md` | Palitra, kayfiyat, kiyim va qurol tavsifi, nima qilmaslik kerak |
| `reference/` | O'yin ichidan olingan ma'lumotnoma rasmlari va logo |
| `reference/_contact_sheet.png` | Hammasi bitta varaqda |
| `Blades_of_Anatolia_demo.mp4` | O'yin ichidan yozilgan demo video (2:55) — AI kadrlari uchun asosiy ma'lumotnoma |

## Qanday ishlatiladi

1. **Vositani tanlang.** Runway Gen-3, Kling, Luma Dream Machine, Veo yoki Sora —
   hammasi matn→video va rasm→video qo'llab-quvvatlaydi.
2. **Rasm→video afzalroq.** Har kadr uchun `prompts.md` da ko'rsatilgan
   ma'lumotnoma rasmini boshlang'ich kadr sifatida bering — natija o'yinning
   uslubiga ancha yaqin chiqadi.
3. **Uslub blokini o'zgartirmang.** `prompts.md` boshidagi STYLE matnini har
   promptning oxiriga qo'shing, aks holda kadrlar bir-biriga o'xshamaydi.
4. **Negative prompt** ham har safar bir xil bo'lsin.
5. **Har kadrni 3–4 marta generatsiya qiling** va eng yaxshisini tanlang.
   Bu AI video bilan ishlashning odatiy narxi.
6. **Logo kadrini AI da qilmang** — `reference/11_logo_full.png` ni montajda
   qo'ying. AI logoni deyarli har doim buzadi.

## Muhim

Treylerni **faqat** AI dan yig'ish tavsiya etilmaydi. Eng ishonchli natija —
aralash montaj:

- AI kadrlari — "o'yin qanday bo'lishi mumkin" (atmosfera, miqyos, kayfiyat)
- O'yin kadrlari — "hozir nima bor" (`docs/Blades_of_Anatolia_demo.mp4`)

O'yin videosi shu papkada ham turibdi: `Blades_of_Anatolia_demo.mp4`
(2:55, 1280x720, 30 k/s). Undan istalgan bo'lakni olib ishlatish mumkin —
`../brand/video_generate.py` da kadr oraliqlarini o'zgartirsangiz boshqa
montaj chiqaradi.

**Rasm->video o'rniga video->video.** Ko'p AI vositalari boshlang'ich kadr
o'rniga qisqa videoni ham qabul qiladi. Shunda harakat xarakteri
(qadam ritmi, kamera og'irligi, jang tezligi) ancha aniq ko'chadi.
Mos bo'laklar:

| Kadr | Videodagi vaqt | Nima ko'rinadi |
|---|---|---|
| 01-02 (vodiy, oba) | 0:07 - 0:20 | Kran plani, oltin soat, o'tovlar halqasi |
| 05 (yugurish) | 0:23 - 0:42 | Yurish -> yugurish -> sprint, burilish yoyi, tana egilishi |
| 06 (parkur) | 0:44 - 1:04 | To'siqdan o'tish, devorga chiqish, dumalab qo'nish |
| 03 (qahramon) | 1:04 - 1:15 | To'xtash masofasi, cho'kkalab yurish |
| 07 (tepalikda) | 1:17 - 1:25 | Bilge Ko'z — parkur geometriyasi yonadi |
| 09 (kamon) | 1:27 - 1:46 | Tortish, qo'l titrashi, otish, o'q uchishi |
| 08, 10-13 (jang) | 1:48 - 2:25 | Ko'p dushman, parry, uchqun, muzlash, kamera turtkisi |
| 14 (bahosi) | 2:28 - 2:49 | Halokat, yakuniy ekran, nazorat nuqtasidan qayta tug'ilish |

Vaqtlar montaj tuzilishidan aniq hisoblangan (`../brand/video_generate.py`).

## Logo

`../brand/` papkasida uch ko'rinishda:

- `logo_mark.png` — kvadrat belgi (ilova ikonkasi, kadr burchagi)
- `logo_full.png` — vertikal qulf (yakuniy kadr)
- `logo_wordmark.png` — gorizontal qulf (afisha, sayt sarlavhasi)

Hammasi `logo_generate.py` bilan istalgan o'lchamda qayta yaratiladi.
