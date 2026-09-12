# Boshqa personajlar — rasm → 3D promptlari (AssetHub Image to Mesh)

Ertug'rul bilan bir xil quvur: 3 rasm (Front / Back / Left), A-poza, qo'lda qurol yo'q, qalqon yo'q, plash yo'q.
Front rasmni birinchi yarating, qolgan ikkitasini **front rasmni reference qilib** so'rang (Midjourney `--cref`, Nano Banana / Flux Kontext — rasmni yuklab).

AssetHub: **High poly, Hunyuan3D 3.1, 4 slot emas — faqat Front/Back/Left (3/8), Polygon Count 500 000.**
Fayl nomi: `D:\Yuklanadiganlar\<id>.glb` (jadvaldagi ID).

## Umumiy blok (hamma personajda bir xil, oxiriga ko'rinish qatori qo'shiladi)

```
13th century Anatolian Turkic setting, historically plausible clothing, no modern items. Exact A-pose: arms straight 35 degrees from the body, palms facing the body, legs straight shoulder width apart, feet flat. Neutral face, mouth closed, looking straight ahead. Nothing in the hands, no shield, no cape, no bow. Full body from head to toe, centered, nothing cropped, orthographic camera at chest height, no perspective distortion. Plain flat light grey background, soft even studio lighting, no ground shadow, no text, no watermark, photorealistic PBR game character, 8k.
```

Ko'rinish qatorlari (umumiy blokdan keyin):
- Front: `Front view, facing the camera directly, both eyes visible.`
- Back: `Back view, seen exactly from behind at 180 degrees. Same character, same outfit, same A-pose, same scale as the front view.`
- Left: `Left side profile view at 90 degrees, only one arm and one leg visible in profile. Same character, same outfit, same A-pose, same scale as the front view.`

## Personajlar (umumiy blokdan oldin qo'yiladigan tavsif)

| ID | Kim | Tavsif (prompt boshiga) |
|---|---|---|
| `turgut` | Turg'ut Alp — bolta ustasi, yirik jussali | `A broad heavy-set Turkic alp warrior, age 30, thick dark beard, short hair, brown leather kaftan with iron studs over a grey wool tunic, thick leather shoulder guards, wide belt, forearm bracers, dark wool trousers, tall leather boots, bare head (no cap).` |
| `bamsi` | Bamsi Beyrek — quvnoq, mo'ynali telpak | `A stocky cheerful Turkic alp warrior, age 32, bushy black beard and moustache, tall pointed felt cap with fur trim, dark red padded kaftan with yellow trim, leather chest straps, wide sash belt, wool trousers, worn leather boots.` |
| `halime` | Halima Sulton — shahzoda ayol | `A young Seljuk noblewoman, age 22, fair skin, long dark braided hair under a light blue silk headscarf, ankle-length deep blue embroidered silk dress with gold trim at the collar and cuffs, fitted bodice, wide sleeves, thin gold belt, soft leather shoes, modest and fully covered.` |
| `selcan` | Selcan Xotun — kuchli xarakterli ayol | `A Turkic tribal woman, age 30, dark hair braided under a dark red headscarf with coin ornaments, long burgundy wool dress with black geometric embroidery, sleeveless felt vest, beaded belt, leather boots, modest and fully covered.` |
| `suleyman` | Sulaymon Shoh — qabila boshlig'i, keksa | `An elderly Turkic tribal chief, age 60, long grey beard, weathered face, tall fur-trimmed leather cap, floor-length dark green wool kaftan with heavy gold-red embroidered borders, thick fur collar, wide leather belt with a large brass buckle, boots.` |
| `kurdoglu` | Kurdo'g'li — xoin amaki | `A middle-aged Turkic tribal man, age 45, trimmed dark beard with grey streaks, dark brown fur-lined kaftan over a black tunic, fur shoulder mantle, wide studded belt, scowling arrogant expression, boots.` |
| `demir` | Deli Demir — temirchi | `A muscular blacksmith, age 45, bald with a thick grey beard, soot-stained sleeveless leather apron over a rough linen shirt with rolled sleeves, thick leather gloves at the belt, heavy leather trousers, worn boots, strong forearms.` |
| `enemy_mongol` | Mo'g'ul jangchisi (dushman) | `A Mongol warrior, age 30, thin moustache, shaved head with a long braid, conical iron helmet with fur trim, dark brown lamellar leather armour in horizontal rows, padded sleeves, thick belt, baggy trousers, riding boots, menacing expression.` |
| `enemy_templar` | Tampliyer ritsari (dushman) | `A crusader Templar knight, age 35, clean-shaven, chainmail hauberk with white surcoat bearing a red cross, mail coif on the head, leather gloves, sword belt without a sword, mail leggings, leather boots.` |
| `meryem` | Meryem — kamonchi ayol jangchi | `A young Turkic archer woman, age 24, dark hair in a tight braid, dark teal fitted kaftan over trousers, leather chest harness, empty quiver on the belt (no bow), forearm bracers, hooded shawl down on the shoulders, light boots.` |

## Keyingi qadamlar (fayl kelgach men qilaman)
1. Blender: 1.5 mln → 150 ming (`bl_decimate.py`), balandlik 1.80 m (ayollar 1.68), oyoq Z=0, teksturali FBX.
2. Mixamo: sizning auto-rig (har personaj uchun bir marta) → animatsiyalar.
3. UE: skelet import → `character.json` profillari (`hero`, `enemy`, `npc`) → o'yinda.

**Eslatma:** dushmanlar va NPClar hozir protsedural tanada, ular darhol ishlaydi. Haqiqiy modellar kelgach `character.json` da profil yo'lini yozsam, avtomatik almashadi.
