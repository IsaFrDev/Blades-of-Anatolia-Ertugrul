# -*- coding: utf-8 -*-
"""Cutscene replikalari uchun ovoz fayllarini yaratadi.

ESKI TIZIM NIMA UCHUN YOMON EDI (scripts/gen_voice.ps1):
  Windows SAPI da bu kompyuterda faqat ikki ovoz bor — Zira (en) va Irina (ru).
  O'zbek va turk matnlari KIRILLGA o'girilib, RUS ovozi bilan o'qilardi.
  Natijada: talaffuz noto'g'ri, va HAMMA personaj — hatto Ertug'rul ham —
  ayol ovozida gapirardi.

YANGI TIZIM:
  * edge-tts (Microsoft neural ovozlari, BEPUL, kalit kerak emas):
      uz  -> Sardor (erkak) / Madina (ayol)
      tr  -> Ahmet  (erkak) / Emel   (ayol)
      en  -> Andrew (erkak) / Ava    (ayol) / Ana (bola)
  * AISHA (aisha.group) — o'zbekcha uchun muqobil, faqat AYOL ovozi (Gulnoza).
    Kalitlar --aisha-keys <fayl> orqali beriladi (repoga YOZILMAYDI).

  Har personajning ROLI va OHANGI data/voice_cast.json da. Ro'yxatda yo'q
  personaj uchun ohang uning id sidan determinatsiyalangan tarzda hisoblanadi —
  shunda 40 ta erkak personaj bir xil ovozda gapirmaydi.

ISHLATISH:
  python tools/gen_voice.py --lang uz,tr,en
  python tools/gen_voice.py --lang uz --force
  python tools/gen_voice.py --lang uz --engine aisha --aisha-keys D:/tmp/keys.txt
  python tools/gen_voice.py --list          # kim qaysi ovozda gapirishini ko'rsatadi
"""
import argparse, asyncio, csv, glob, io, json, os, subprocess, sys, wave, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CUT  = os.path.join(ROOT, "data", "cutscenes")
OUT  = os.path.join(ROOT, "assets", "audio", "vo")
CAST = os.path.join(ROOT, "data", "voice_cast.json")

# --------------------------------------------------------------- ovoz jadvali
VOICES = {
    "uz": {"male": "uz-UZ-SardorNeural", "female": "uz-UZ-MadinaNeural",
           "boy":  "uz-UZ-MadinaNeural"},
    "tr": {"male": "tr-TR-AhmetNeural",  "female": "tr-TR-EmelNeural",
           "boy":  "tr-TR-EmelNeural"},
    "en": {"male": "en-US-AndrewNeural", "female": "en-US-AvaNeural",
           "boy":  "en-US-AnaNeural"},          # Ana — haqiqiy bola ovozi
}
# Bola uchun alohida ovoz yo'q bo'lgan tillarda ayol ovozi ko'tariladi.
BOY_LIFT = {"pitch_hz": 22, "rate_pct": 6}


def load_cast():
    d = json.load(io.open(CAST, encoding="utf-8"))
    return d.get("default_role", "male"), d.get("actors", {})


def auto_tone(actor_id):
    """Ro'yxatda yo'q personaj uchun barqaror (determinatsiyalangan) ohang."""
    h = zlib.crc32(actor_id.encode("utf-8"))
    return {"pitch_hz": (h % 21) - 10,          # -10 .. +10
            "rate_pct": ((h >> 8) % 13) - 6}    #  -6 .. +6


def collect_lines():
    """(voId, actor, locKey) — barcha cutscene lardan."""
    out, seen = [], set()
    for f in sorted(glob.glob(os.path.join(CUT, "*.json"))):
        d = json.load(io.open(f, encoding="utf-8"))
        for ln in d.get("lines", []):
            vo = ln.get("vo") or ln.get("voId")
            lk = ln.get("loc") or ln.get("locKey")
            ac = ln.get("actor", "")
            if not vo or not lk or vo in seen:
                continue
            seen.add(vo)
            out.append((vo, ac, lk))
    return out


def load_text():
    """locKey -> {uz, tr, en}"""
    t = {}
    for fn in ("cutscene_loc.csv", "ertugrul_loc.csv", "ui_loc.csv", "episodes_loc.csv"):
        p = os.path.join(ROOT, "localization", fn)
        if not os.path.exists(p):
            continue
        for row in csv.DictReader(io.open(p, encoding="utf-8")):
            k = row.get("keys")
            if k and k not in t:
                t[k] = {L: (row.get(L) or "").strip() for L in ("uz", "tr", "en")}
    return t


def ffmpeg():
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except Exception:
        return "ffmpeg"


def to_wav(src, dst, rate=24000):
    """MP3 -> 16-bit mono WAV. O'yin mikseri istalgan chastotani qayta namunalaydi."""
    cmd = [ffmpeg(), "-y", "-loglevel", "error", "-i", src,
           "-ar", str(rate), "-ac", "1", "-c:a", "pcm_s16le", dst]
    return subprocess.call(cmd) == 0


# --------------------------------------------------------------- edge-tts
async def edge_one(text, voice, pitch_hz, rate_pct, out_mp3):
    import edge_tts
    c = edge_tts.Communicate(
        text, voice,
        rate=("%+d%%" % int(rate_pct)),
        pitch=("%+dHz" % int(pitch_hz)))
    await c.save(out_mp3)


# --------------------------------------------------------------- AISHA
class Aisha:
    """aisha.group TTS. Faqat o'zbekcha va faqat AYOL ovozi (Gulnoza).
    Kalit tugasa/xato bersa keyingisiga o'tadi."""
    URL = "https://back.aisha.group/api/v1/tts/post/"
    UA  = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
           "(KHTML, like Gecko) Chrome/124.0 Safari/537.36")

    def __init__(self, keys):
        self.keys = list(keys)
        self.i = 0
        self.dead = set()

    def _post(self, key, text):
        import urllib.request
        b = "----ertbnd"
        def f(n, v):
            return ("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n"
                    % (b, n, v))
        body = (f("transcript", text) + f("language", "uz") + f("model", "Gulnoza")
                + f("mood", "Neutral") + f("speed", "1.0")
                + "--%s--\r\n" % b).encode("utf-8")
        r = urllib.request.Request(self.URL, data=body, method="POST")
        r.add_header("X-Api-Key", key)
        r.add_header("Content-Type", "multipart/form-data; boundary=%s" % b)
        with urllib.request.urlopen(r, timeout=120) as resp:
            return json.loads(resp.read().decode("utf-8")).get("audio_path")

    def synth(self, text, out_wav):
        import urllib.request
        for _ in range(len(self.keys)):
            if self.i in self.dead:
                self.i = (self.i + 1) % len(self.keys)
                continue
            key = self.keys[self.i]
            try:
                ap = self._post(key, text[:1000])
                if not ap:
                    raise RuntimeError("audio_path yo'q")
                q = urllib.request.Request(ap)
                q.add_header("User-Agent", self.UA)
                q.add_header("Referer", "https://aisha.group/")
                with urllib.request.urlopen(q, timeout=120) as resp:
                    io.open(out_wav, "wb").write(resp.read())
                return True
            except Exception as e:
                msg = str(e)
                print("      kalit #%d ishlamadi (%s) -> keyingisi" % (self.i + 1, msg[:60]))
                self.dead.add(self.i)
                self.i = (self.i + 1) % len(self.keys)
        return False


# --------------------------------------------------------------- asosiy
def resolve(actor, default_role, cast):
    e = cast.get(actor)
    if e:
        role = e.get("role", default_role)
        tone = {"pitch_hz": e.get("pitch_hz", 0), "rate_pct": e.get("rate_pct", 0)}
    else:
        role, tone = default_role, auto_tone(actor)
    return role, tone


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lang", default="uz,tr,en")
    ap.add_argument("--engine", default="edge", choices=["edge", "aisha"])
    ap.add_argument("--aisha-keys", default="")
    ap.add_argument("--force", action="store_true")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--limit", type=int, default=0)
    a = ap.parse_args()

    default_role, cast = load_cast()
    lines = collect_lines()
    texts = load_text()

    if a.list:
        seen = {}
        for vo, actor, lk in lines:
            seen.setdefault(actor, 0)
            seen[actor] += 1
        print("%-18s %5s  %-8s %8s %8s" % ("actor", "repl.", "rol", "pitch", "rate"))
        for actor in sorted(seen, key=lambda k: -seen[k]):
            role, tone = resolve(actor, default_role, cast)
            print("%-18s %5d  %-8s %+8d %+7d%%"
                  % (actor, seen[actor], role, tone["pitch_hz"], tone["rate_pct"]))
        return 0

    langs = [x.strip() for x in a.lang.replace(",", " ").split() if x.strip()]
    aisha = None
    if a.engine == "aisha":
        if not a.aisha_keys or not os.path.exists(a.aisha_keys):
            print("XATO: --aisha-keys <fayl> kerak (har qatorda bitta kalit).")
            return 2
        keys = [l.strip() for l in io.open(a.aisha_keys, encoding="utf-8")
                if l.strip() and not l.startswith("#")]
        aisha = Aisha(keys)
        print("AISHA: %d ta kalit yuklandi" % len(keys))

    tmp = os.path.join(ROOT, "saves", "_tts_tmp")
    os.makedirs(tmp, exist_ok=True)

    for lang in langs:
        if lang not in VOICES:
            print("noma'lum til:", lang); continue
        d = os.path.join(OUT, lang)
        os.makedirs(d, exist_ok=True)
        done = skip = fail = 0
        todo = lines[:a.limit] if a.limit else lines
        print("\n=== %s — %d replika ===" % (lang, len(todo)))
        for i, (vo, actor, lk) in enumerate(todo, 1):
            dst = os.path.join(d, vo + ".wav")
            if os.path.exists(dst) and not a.force:
                skip += 1; continue
            text = (texts.get(lk) or {}).get(lang, "").strip()
            if not text:
                fail += 1
                print("  [%3d] %-22s MATN YO'Q (%s)" % (i, vo, lk)); continue

            role, tone = resolve(actor, default_role, cast)
            pitch, rate = tone["pitch_hz"], tone["rate_pct"]

            # AISHA faqat o'zbek AYOL ovozini beradi (erkak ovozi yo'q)
            use_aisha = (aisha is not None and lang == "uz" and role == "female")
            try:
                if use_aisha:
                    raw = os.path.join(tmp, vo + "_a.wav")
                    if not aisha.synth(text, raw):
                        raise RuntimeError("AISHA kalitlari tugadi")
                    ok = to_wav(raw, dst)
                else:
                    voice = VOICES[lang][role]
                    p, r = pitch, rate
                    if role == "boy" and VOICES[lang]["boy"] == VOICES[lang]["female"]:
                        p += BOY_LIFT["pitch_hz"]; r += BOY_LIFT["rate_pct"]
                    mp3 = os.path.join(tmp, vo + ".mp3")
                    asyncio.run(edge_one(text, voice, p, r, mp3))
                    ok = to_wav(mp3, dst)
                if ok:
                    done += 1
                    if done % 25 == 0:
                        print("  %d/%d ..." % (i, len(todo)))
                else:
                    fail += 1
                    print("  [%3d] %-22s WAV ga o'girish xatosi" % (i, vo))
            except Exception as e:
                fail += 1
                print("  [%3d] %-22s XATO: %s" % (i, vo, str(e)[:70]))
        print("  yaratildi %d, o'tkazildi %d, xato %d" % (done, skip, fail))

    # tozalash
    for f in glob.glob(os.path.join(tmp, "*")):
        try: os.remove(f)
        except Exception: pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
