# Ertug'rul personaji — rasm → 3D (AssetHub Image to Mesh, Hunyuan3D 3.1)

## 1. Qanday ishlaydi
Image-to-Mesh tuguni 8 ta ko'rinish qabul qiladi, lekin sifatli natija uchun **4 ta asosiy** (Front, Back, Left, Right) yetarli; Top/Bottom/Left Front/Right Front ixtiyoriy. Muhim shart: **hamma rasm bir xil personaj, bir xil poza, bir xil kiyim, bir xil yorug'lik, bo'sh fon**.

Eng ishonchli yo'l: avval **bitta "character turnaround sheet"** (bir rasmda front/side/back) yaratib, keyin uni 3-4 bo'lakka kesib har bir slotga qo'yish. Rasm generatorlar bitta rasm ichida personajni bir xil saqlashda ancha yaxshi.

## 2. Poza va kiyim qoidalari (mesh buzilmasligi uchun)
- **A-poza** (qo'llar tanadan 30-40° ochilgan, oyoqlar yelka kengligida) — rigging (Mixamo) uchun eng yaxshisi. T-poza ham bo'ladi.
- Qilich **qinda, belda** (qo'lda emas), kamon **orqada** — tanadan uzoq chiqib turgan narsalar mesh bo'lib chiqmaydi yoki singan bo'ladi.
- Plash/yopinchiq **tanaga yopishgan**, shamolda hilpiramaydi (uchib turgan mato = teshik mesh).
- Sochlar to'plangan, soqol kalta-o'rtacha. Bosh kiyim: kiyik terisi telpak (bo'rk).
- Fon oq yoki och kulrang, soyasiz, **studio yorug'lik**, hech qanday matn/logotip yo'q.
- Oyoq ostida yer ko'rinmasin (butun tana, boshdan oyoqqacha, kadrga to'liq sig'sin).

## 3. Master prompt (rasm generator uchun: Midjourney / SDXL / Flux / Nano Banana / Ideogram)
Ingliz tilida (generatorlar shunda aniq ishlaydi). Avval turnaround:

```
Character turnaround sheet of Ertugrul Bey, 13th century Turkic Oghuz (Kayi tribe) warrior chief, age 35, strong build, tanned skin, short dark beard, long dark hair tied back under a deer-fur cap (borok) with a small feather. Outfit: knee-length dark brown leather kaftan with red and gold Turkic geometric embroidery on chest and cuffs, thick wool inner tunic, wide leather belt with bronze buckle, curved sword (kilij) sheathed at the left hip, small round wooden shield on the back, leather bracers, wool trousers, tall dark leather boots, short wolf-fur mantle on shoulders tight to the body. A-pose, arms 35 degrees from the body, legs shoulder width, neutral expression, mouth closed. Three views side by side in one image: front view, left side view, back view, exactly the same character, same scale, same outfit, same pose. Full body head to toe, nothing cropped. Plain flat light grey background, soft even studio lighting, no shadows on the ground, no text, no watermark, photorealistic game character concept, 8k, sharp.
```

Negative prompt (SDXL/Flux uchun):
```
text, watermark, logo, multiple different characters, weapon in hand, flowing cape, wind, motion blur, dramatic shadows, cropped, missing feet, extra limbs, low quality, cartoon, anime
```

Midjourney parametrlari: `--ar 16:9 --style raw --v 6.1 --s 150` (turnaround), keyin `--cref <URL>` bilan bir xillik.

## 4. Har bir ko'rinish uchun alohida prompt (turnaround yetarli bo'lmasa)
Master tavsifni (kiyim qismi) o'zgartirmasdan, faqat oxirini almashtiring:

| Slot | Qo'shimcha (prompt oxiri) |
|---|---|
| Front | `front view, facing the camera directly, orthographic, A-pose, full body head to toe, plain light grey background` |
| Back | `back view, seen exactly from behind, showing the round shield on the back and the fur mantle, orthographic, same A-pose, full body, plain light grey background` |
| Left | `left side profile view, seen exactly from the left, orthographic, same A-pose, sheathed sword visible at the hip, full body, plain light grey background` |
| Right | `right side profile view, seen exactly from the right, orthographic, same A-pose, full body, plain light grey background` |
| Top (ixtiyoriy) | `top-down view from directly above, showing the fur cap and shoulders, orthographic, same A-pose` |
| Left Front / Right Front (ixtiyoriy) | `three-quarter front view from the left (45 degrees)` / `... from the right (45 degrees)` |

Bir xillik uchun: Midjourney `--cref` + `--cw 100`, SDXL da IP-Adapter/FaceID yoki Flux Kontext/Nano Banana ga birinchi (front) rasmni yuklab "same character, now show the back view, same outfit, same pose, same background" deb so'rang.

## 5. AssetHub tugun sozlamalari
- Tab: **High poly**, Model: **Hunyuan3D 3.1**.
- Input Images: Front, Back, Left, Right (4/8). Rasmlar kvadrat yoki portret, personaj markazda, 1024-2048 px.
- **Polygon Count: 400 000 - 600 000** (1 500 000 shart emas: bu skelet mesh, Nanite ishlatilmaydi; katta son = og'ir fayl, sekin rigging). Keyin Low poly (30-60k) variantini ham olib qo'ying — o'yin uchun aynan shu kerak.
- Private storage / Priority: ixtiyoriy. Narx ~85 kredit.
- Tekstura: agar tugunda "Texture / PBR" bosqichi bo'lsa yoqing (Base color + normal + roughness). Bo'lmasa AssetHub "Texture" tugunini ulang.

## 6. Keyingi zanjir (rasm → o'yin)
1. GLB/FBX ni yuklab oling → `D:\Yuklanadiganlar\SK_Ertugrul.glb`.
2. Rigging: **Mixamo** (mixamo.com, bepul): FBX yuklang → Auto-Rigger (chin, wrists, elbows, knees, groin belgilang) → animatsiyalar: Idle, Walking, Running, Jump, Sword And Shield Slash (x3), Sword And Shield Block, Hit Reaction, Death, **Horse Riding** (`ride` uchun) — hammasi "Without skin" (birinchisi "With skin").
3. UE ga import (Interchange, Skeleton = Ertugrul skeleti) → `Content/Ertugrul/Data/character.json` da `hero.mesh` va `hero.anims` yo'llarini yangilayman, `yaw`/`z`/`scale` kalibr qilaman.
4. Yuz: Hunyuan meshida morph-target bo'lmaydi. Variantlar: (a) boshni MetaHuman Creator dan olib almashtirish (morphlar tayyor, bizning `FaceAnimate` darhol ishlaydi); (b) hozircha yuzsiz qoldirish.

## 7. Nima berasiz
Tayyor bo'lgach faylni `D:\Yuklanadiganlar` ga tashlang va yozing: "Ertugrul GLB tayyor" — men import, rigging tekshiruvi, character.json va testni qilaman.
