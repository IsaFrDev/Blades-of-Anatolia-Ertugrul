# -*- coding: utf-8 -*-
"""Cutscene "bibliyasi": har sahna uchun to'g'ri JOY, VAQT, OB-HAVO va
KANONIK AKTYOR MODELLARI. EP006 uchun yo'q bo'lgan cutscene'ni ham yaratadi.

NIMA ARALASHGAN EDI:
  * Söğüt epizodlari (39, 42, 44, 48) Qayi obasida kechardi; Mo'g'ul lageri
    (32, 36) ham o'sha obada; Yassıçemen jang maydoni shahar yo'lida.
  * 14 aktyor sahnadan sahnaga MODEL almashtirardi (No'yon goh salibchi,
    goh usmonli; Hayma Ona uch xil model).
  * Ko'p aktyor Kenney'ning ZAMONAVIY kubik odamchalari (character-a..r:
    gamepad futbolkasi, politsiya formasi) bilan chizilardi.
  * EP006 (Sultan Han) uchun cutscene umuman yo'q edi -> generic_intro.

Ishlatish:  python tools/apply_scene_bible.py
"""
import json, io, csv, glob, os, copy

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

M = "assets/models/"
OTT = M + "ottoman/ottoman.obj";              CRU = M + "crusader/crusader.obj"
BARB = M + "barbarian+warrior+3d+model.obj";  CARD = M + "cardinal+robe+3d+model.obj"
CKN = M + "crusader+knight+3d+model.obj";     FAN = M + "fantasy+warrior+3d+model.obj"
ARM = M + "medieval+armored+warrior+3d+model.obj"; MKN = M + "medieval+knight+3d+model.obj"
MWAR = M + "medieval+warrior+3d+model.obj";   QUEEN = M + "ornate+queen+costume+3d+model.obj"
OWAR = M + "ottoman+warrior+3d+model.obj";    ELD = M + "turbaned+elder+3d+model.obj"
W = [1.0, 1.0, 1.0]

# ---- KANONIK AKTYORLAR: (model, bo'y m, tint) ----
CAST = {
    "ertugrul": (OTT, 1.82, [1.0, 0.95, 0.85]),
    # alplar — charm sovutli jangchi
    "turgut": (FAN, 1.92, [0.86, 0.80, 0.74]),   # boltali pahlavon
    "bamsi": (OWAR, 1.88, [0.78, 0.70, 0.62]),
    "dogan": (MWAR, 1.82, [0.92, 0.88, 0.82]),   # kamonchi
    "deli_demir": (FAN, 1.90, [0.70, 0.66, 0.62]),
    # beklar va saroy — usmonli jangchi (qizil kaftan)
    "dundar": (OWAR, 1.78, [0.95, 0.90, 0.85]), "gunduz": (OWAR, 1.82, [0.90, 0.85, 0.80]),
    "gundogdu": (OWAR, 1.86, [0.85, 0.80, 0.78]), "savci": (OWAR, 1.80, W),
    "tekin": (OWAR, 1.82, [0.80, 0.80, 0.85]), "sungur": (OWAR, 1.80, W),
    "husam": (OWAR, 1.82, [0.90, 0.90, 0.95]), "arslan_bek": (OWAR, 1.86, [0.85, 0.85, 0.90]),
    "abdurrahman": (OWAR, 1.84, W), "kurdoglu": (OWAR, 1.84, [0.75, 0.72, 0.72]),
    "kopek": (OWAR, 1.84, [0.60, 0.60, 0.70]), "kir_khan": (OWAR, 1.86, [0.90, 0.80, 0.70]),
    "emir_sharaf": (OWAR, 1.84, [0.95, 0.90, 0.75]), "yakub": (OWAR, 1.76, [1.0, 0.90, 0.70]),
    "togrul": (OWAR, 1.84, [0.80, 0.85, 0.95]), "hamza": (OWAR, 1.80, [0.60, 0.60, 0.60]),
    "al_aziz": (OWAR, 1.56, [1.0, 0.95, 0.80]),           # o'n besh yoshli amir
    # oqsoqollar, shayxlar — sallali keksa
    "suleyman_shah": (ELD, 1.84, W), "ibn_arabi": (ELD, 1.80, [0.95, 0.95, 1.0]),
    "rumi": (ELD, 1.80, [1.0, 0.97, 0.90]), "bektas": (ELD, 1.78, [0.90, 0.95, 0.90]),
    "ahi_evren": (ELD, 1.80, [1.0, 0.90, 0.80]), "darvesh": (ELD, 1.76, [0.80, 0.80, 0.80]),
    "hanchi": (ELD, 1.78, [0.90, 0.85, 0.75]), "xizmatkor": (ELD, 1.72, [0.85, 0.85, 0.85]),
    "uygur_kotib": (ELD, 1.76, [0.90, 0.90, 1.0]),
    # ayollar — yagona ayol model, tint bilan farqlanadi
    "halime": (QUEEN, 1.64, W), "aykiz": (QUEEN, 1.60, [0.95, 0.85, 0.95]),
    "hayme_ana": (QUEEN, 1.62, [0.75, 0.72, 0.70]), "gulbahor": (QUEEN, 1.60, [0.72, 0.70, 0.68]),
    # mo'g'ullar — mo'ynali jangchi
    "noyan": (BARB, 1.94, W), "mongol_chavandoz": (MWAR, 1.84, [0.75, 0.75, 0.80]),   # otliq kamonchi
    "mongol_katib": (OWAR, 1.76, [0.60, 0.60, 0.68]), "darugachi": (BARB, 1.86, [0.85, 0.80, 0.80]),
    "jallod": (FAN, 1.94, [0.55, 0.55, 0.55]),
    # salibchilar va vizantiyaliklar
    "titus": (CKN, 1.90, W), "josus": (MKN, 1.80, [0.80, 0.80, 0.80]),
    "theodoros": (CARD, 1.80, [0.80, 0.80, 0.90]), "kosta": (CARD, 1.72, [0.70, 0.70, 0.70]),
    # sultonlar — zirhli, tojli
    "kayqubad": (ARM, 1.88, W), "alauddin": (ARM, 1.88, W),
    "keyhusrev": (ARM, 1.84, [0.95, 0.95, 1.0]), "kaykhusraw": (ARM, 1.84, [0.95, 0.95, 1.0]),
    # bolalar — kattalar modeli kichik bo'yda
    "orphan_boy": (OWAR, 1.18, [0.85, 0.80, 0.75]), "bola": (OWAR, 1.12, [0.90, 0.85, 0.80]),
    "tashchi_bola": (OWAR, 1.22, [0.80, 0.78, 0.75]), "osman": (OWAR, 1.02, [1.0, 0.95, 0.85]),
}
json.dump({"_izoh": "Har aktyor uchun kanonik model, bo'y va tint. tools/apply_scene_bible.py qo'llaydi.",
           "cast": {k: {"model": v[0], "scale": v[1], "tint": v[2]} for k, v in CAST.items()}},
          io.open("data/cast_models.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)


def level_for(region):
    r = region.lower()
    if "söğüt" in r or "sogut" in r or "karacahisar" in r:
        return "sogut_village"
    if "nikeya" in r:
        return "aleppo_road"                    # monastir
    if "qayi obasi" in r or "karacadağ" in r or "chegara" in r:
        return "oba_valley"
    if "lager" in r:
        return "oba_camp"                       # begona / harbiy lager
    for k in ("o'rmon", "qarag", "bagras", "torus", "dovon", "qorli", "g'or", "dasht",
              "köse dağ", "domaniç", "yassıçemen", "karvonsaroy xarobasi"):
        if k in r:
            return "forest_pass"
    return "aleppo_road"                        # shahar, saroy, yo'l, han


def apply_cast(scene):
    for a in scene.get("actors", []):
        k = CAST.get(a["id"])
        if k:
            a["model"], a["scale"], a["tint"] = k[0], k[1], list(k[2])
        elif "/characters/" in a.get("model", ""):
            a["model"], a["scale"] = OWAR, 1.80     # zamonaviy odamcha -> tarixiy


eps = json.load(io.open("data/episodes/episodes_v2.json", encoding="utf-8"))["episodes"]
env = {e["id"]: e.get("environment", {}) for e in eps}
changed = []
for f in sorted(glob.glob("data/cutscenes/ep*_intro.json")):
    c = json.load(io.open(f, encoding="utf-8"))
    eid = c.get("episode") or os.path.basename(f)[:5].upper()
    en = env.get(eid, {})
    before = (c.get("level"), c.get("time_of_day"), c.get("weather"))
    c["level"] = level_for(str(en.get("region", "")))
    et, ct = str(en.get("time_of_day", "")), c.get("time_of_day", "day")
    if et in ("night", "dusk", "dawn"):
        c["time_of_day"] = et
    elif et == "day" and ct == "night":
        c["time_of_day"] = "day"
    if c["time_of_day"] == "dusk" and c["level"] in ("oba_valley", "sogut_village"):
        c["time_of_day"] = "golden"             # oltin soat preseti
    ew = str(en.get("weather", ""))
    if ew in ("snow", "rain", "fog"):
        c["weather"] = ew
    elif ew == "clear" and c.get("weather") in ("snow", "rain"):
        c["weather"] = "clear"
    elif ew == "dust":
        c["weather"] = "fog"
    apply_cast(c)
    after = (c["level"], c["time_of_day"], c["weather"])
    if before != after:
        changed.append((eid, before, after))
    io.open(f, "w", encoding="utf-8", newline="\n").write(json.dumps(c, ensure_ascii=False, indent=2))
print("joy/vaqt/ob-havo o'zgargan sahnalar: %d" % len(changed))
for eid, b, a in changed:
    print("  %s %s -> %s" % (eid, b, a))

g = "data/cutscenes/generic_intro.json"
c = json.load(io.open(g, encoding="utf-8"))
apply_cast(c)
c["level"] = "oba_valley"
io.open(g, "w", encoding="utf-8", newline="\n").write(json.dumps(c, ensure_ascii=False, indent=2))

# ---- EP006: Sultan Han karvonsaroyi ----
t = json.load(io.open("data/cutscenes/ep007_intro.json", encoding="utf-8"))   # aleppo_road shabloni
ids = ["ertugrul", "turgut", "hanchi", "josus"]
acts = []
for i, a in enumerate(t["actors"][:4]):
    a = copy.deepcopy(a)
    a["id"] = a["char"] = ids[i]
    a["loc_name"] = "chr.%s.name" % ids[i]
    k = CAST[ids[i]]
    a["model"], a["scale"], a["tint"] = k[0], k[1], list(k[2])
    if ids[i] == "josus":
        for kk in a["keys"]:
            kk["clip"] = "Idle"                  # josus jim turadi, faqat kuzatadi
    acts.append(a)
lines = [(2.0, "ertugrul"), (7.5, "hanchi"), (12.5, "turgut"), (17.0, "hanchi"),
         (22.0, "ertugrul"), (26.5, "turgut"), (31.0, "ertugrul")]
c6 = {"id": "ep006_intro", "episode": "EP006", "level": "aleppo_road",
      "music": "MUS_EP006", "ambience": "Ambience.Winter.Clear",
      "letterbox": True, "fade_in": 1.8, "fade_out": 1.4,
      "time_of_day": "day", "weather": "clear", "duration": 0,
      "actors": acts, "camera": t["camera"],
      "lines": [{"t": tt, "actor": ac, "loc": "cut.ep006_intro.%02d" % (i + 1),
                 "vo": "ep006_intro_%02d" % (i + 1), "dur": 0}
                for i, (tt, ac) in enumerate(lines)]}
io.open("data/cutscenes/ep006_intro.json", "w", encoding="utf-8", newline="\n").write(
    json.dumps(c6, ensure_ascii=False, indent=2))
print("EP006 cutscene yozildi: %d aktyor, %d replika" % (len(acts), len(lines)))

rows = [
    ("cut.ep006_intro.01",
     "Sulton Han. Uch kun, ikki yuz jon — va hech kimdan bir tanga so'ralmaydi.",
     "Sultan Han. Üç gün, iki yüz can — ve kimseden tek bir akçe istenmez.",
     "Sultan Han. Three days, two hundred souls — and not one coin asked of anyone."),
    ("cut.ep006_intro.02",
     "Bu sultonning hukmi, bek. Karvonsaroy — yo'lovchining uyi, savdogarning emas.",
     "Bu sultanın buyruğu, bey. Kervansaray yolcunun evidir, tüccarın değil.",
     "That is the Sultan's decree, Bey. The caravanserai is the traveller's home, not the merchant's."),
    ("cut.ep006_intro.03",
     "Bek, ikki yuz odam ichida bittasi bizni kuzatyapti. Kim — hali bilmayman.",
     "Bey, iki yüz kişinin içinde biri bizi izliyor. Kim — henüz bilmiyorum.",
     "Bey, among two hundred people one is watching us. Who — I do not know yet."),
    ("cut.ep006_intro.04",
     "Karvonsaroyda hamma birovni kuzatadi, alp. Bu yerda ko'z — pul.",
     "Kervansarayda herkes birini izler, alp. Burada göz paradır.",
     "In a caravanserai everyone watches someone, alp. Here, eyes are money."),
    ("cut.ep006_intro.05",
     "Qon to'kmaymiz. Sulton Han sultonniki — mehmonniki emas.",
     "Kan dökmeyiz. Sultan Han sultanındır — misafirin değil.",
     "We spill no blood. Sultan Han belongs to the Sultan — not to the guest."),
    ("cut.ep006_intro.06",
     "Unda uni qilichsiz topamiz. Ko'z bilan, quloq bilan.",
     "Öyleyse onu kılıçsız buluruz. Gözle, kulakla.",
     "Then we find him without a sword. With eyes, with ears."),
    ("cut.ep006_intro.07",
     "Uch kun bor. Uchinchi kuni karvon ketadi — u ham ketadi. Ungacha topamiz.",
     "Üç günümüz var. Üçüncü gün kervan gider — o da gider. O zamana kadar buluruz.",
     "We have three days. On the third the caravan leaves — and so does he. We find him before then."),
]
p = "localization/cutscene_loc.csv"
have = {r["keys"] for r in csv.DictReader(io.open(p, encoding="utf-8"))}
with io.open(p, "a", encoding="utf-8", newline="") as f:
    w = csv.writer(f, quoting=csv.QUOTE_ALL); n = 0
    for r in rows:
        if r[0] not in have:
            w.writerow(r); n += 1
print("cutscene_loc.csv: +%d" % n)
p = "localization/ertugrul_loc.csv"
have = {r["keys"] for r in csv.DictReader(io.open(p, encoding="utf-8"))}
with io.open(p, "a", encoding="utf-8", newline="") as f:
    w = csv.writer(f, quoting=csv.QUOTE_ALL); n = 0
    for r in [("chr.hanchi.name", "Han egasi", "Han sahibi", "Innkeeper"),
              ("chr.josus.name", "Josus", "Casus", "Spy")]:
        if r[0] not in have:
            w.writerow(r); n += 1
print("ertugrul_loc.csv: +%d" % n)

vc = json.load(io.open("data/voice_cast.json", encoding="utf-8"))
vc["actors"].setdefault("hanchi", {"role": "male", "pitch_hz": -6, "rate_pct": -8, "izoh": "Han egasi — keksa"})
vc["actors"].setdefault("xizmatkor", {"role": "male", "pitch_hz": -2, "rate_pct": -4})
io.open("data/voice_cast.json", "w", encoding="utf-8", newline="\n").write(json.dumps(vc, ensure_ascii=False, indent=2))
for f in ("data/levels/chars_lineup.json", "data/levels/cast_lineup.json"):
    if os.path.exists(f):
        os.remove(f)
print("tayyor")
