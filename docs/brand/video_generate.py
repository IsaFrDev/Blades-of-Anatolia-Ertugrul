# -*- coding: utf-8 -*-
"""Loyiha videosini yig'adi: yozib olingan kadrlar + sarlavha kartalari."""
import os, numpy as np
from PIL import Image, ImageDraw, ImageFont
import imageio.v2 as iio

W, H, FPS = 1280, 720, 30
FILM = r"D:\temp\claude\film"
BRAND = r"D:\My_apps\Ertugrul\docs\brand"
OUT = r"D:\My_apps\Ertugrul\docs\Blades_of_Anatolia_demo.mp4"

FON    = (14, 19, 22)
MATN   = (228, 234, 234)
SONIK  = (124, 139, 143)
FERUZA = (72, 169, 181)
ZARHAL = (198, 156, 100)
SUYAK  = (226, 218, 204)


def fnt(sz, bold=True):
    for nm in (("georgiab.ttf" if bold else "georgia.ttf"), "arialbd.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype("C:/Windows/Fonts/" + nm, sz)
        except Exception:
            continue
    return ImageFont.load_default()


def ctr(d, txt, f, y, col, sp=0):
    if sp:
        wt = sum(d.textlength(c, font=f) + sp for c in txt) - sp
        x = (W - wt) / 2.0
        for c in txt:
            d.text((x, y), c, font=f, fill=col)
            x += d.textlength(c, font=f) + sp
    else:
        d.text(((W - d.textlength(txt, font=f)) / 2, y), txt, font=f, fill=col)


def card(title, sub, secs, logo=None, big=False):
    """Sarlavha kartasi — kirish va chiqishda qorayadi."""
    base = Image.new("RGB", (W, H), FON)
    d = ImageDraw.Draw(base)
    if logo is not None:
        lw = 300 if big else 150
        lg = logo.resize((lw, lw), Image.LANCZOS)
        base.paste(lg, ((W - lw) // 2, (150 if big else 210)), lg)
        ty = (150 if big else 210) + lw + (34 if big else 24)
    else:
        ty = 300
    if big:
        ctr(d, "BLADES OF ANATOLIA", fnt(34), ty, SUYAK, sp=9)
        ctr(d, "ERTUGRUL", fnt(74), ty + 52, ZARHAL, sp=12)
        d.rectangle([W / 2 - 150, ty + 152, W / 2 + 150, ty + 153], fill=(46, 58, 64))
        ctr(d, sub, fnt(20, False), ty + 172, SONIK)
    else:
        ctr(d, title, fnt(52), ty, SUYAK, sp=6)
        d.rectangle([W / 2 - 9, ty + 76, W / 2 + 9, ty + 94], fill=FERUZA)
        ctr(d, sub, fnt(22, False), ty + 116, SONIK)
    n = int(secs * FPS)
    fade = max(4, int(0.28 * FPS))
    arr = np.asarray(base, dtype=np.float32)
    for i in range(n):
        k = 1.0
        if i < fade:            k = i / float(fade)
        elif i > n - fade - 1:  k = max(0.0, (n - 1 - i) / float(fade))
        yield (arr * k).astype(np.uint8)


def clip(part, a, b, step=1):
    d = os.path.join(FILM, part)
    for i in range(a, b, step):
        p = os.path.join(d, "f%05d.jpg" % i)
        if not os.path.exists(p):
            continue
        im = Image.open(p).convert("RGB")
        if im.size != (W, H):
            im = im.resize((W, H), Image.LANCZOS)
        yield np.asarray(im, dtype=np.uint8)


def main():
    logo = Image.open(os.path.join(BRAND, "logo_mark.png")).convert("RGBA")
    wr = iio.get_writer(OUT, fps=FPS, codec="libx264",
                        macro_block_size=8, output_params=["-crf", "23"],
                        ffmpeg_params=["-pix_fmt", "yuv420p", "-preset", "slow",
                                       "-profile:v", "high", "-level", "4.0",
                                       "-movflags", "+faststart"])
    total = 0

    def push(gen):
        nonlocal total
        for fr in gen:
            wr.append_data(fr)
            total += 1

    # 1. Ochilish
    push(card(None, "So'nggi yurish  ·  1227-1261", 5.0, logo, big=True))

    # 2. Xarita — kran plani, sekin aylanish
    push(card("XARITA", "Qayi obasi — vodiy  ·  oltin soat", 2.4))
    push(clip("p1", 20, 420))

    # 3. Harakat
    push(card("HARAKAT", "Yurish · yugurish · sprint · burilish yoyi", 2.4))
    push(clip("p1", 440, 1010))

    # 4. Parkur
    push(card("PARKUR", "Sakrash · to'siqdan o'tish · devorga chiqish", 2.4))
    push(clip("p1", 1090, 1700))

    # 5. To'xtash va cho'kkalash
    push(clip("p1", 1760, 2080))

    # 6. Bilge Ko'z
    push(card("BILGE KO'Z", "Parkur geometriyasi yonadi", 2.2))
    push(clip("p1", 2100, 2330))

    # 7. Kamon
    push(card("KAMON", "Ballistik o'q · tana zonalari · nafas", 2.4))
    push(clip("p2", 230, 790))

    # 8. To'liq jang
    push(card("JANG", "Uch resurs · parry · muzlash · uchqun", 2.4))
    push(clip("p2", 790, 1900))

    # 9. Halokat va qayta tug'ilish
    push(card("HALOKAT VA QAYTA TUG'ILISH", "Nazorat nuqtasidan davom etadi", 2.4))
    push(clip("p3", 960, 1140))
    push(clip("p3", 1340, 1500))
    push(clip("p3", 1500, 1810))

    # 10. Yakun
    push(card(None, "Sof C++  ·  o'z dvigateli  ·  tashqi kutubxonasiz", 6.0, logo, big=True))

    wr.close()
    print("video: %s" % OUT)
    print("kadrlar: %d  davomiyligi: %d:%02d" % (total, total // FPS // 60, (total // FPS) % 60))
    print("hajm MB: %.1f" % (os.path.getsize(OUT) / 1024.0 / 1024.0))


if __name__ == "__main__":
    main()
