# -*- coding: utf-8 -*-
"""BLADES OF ANATOLIA: ERTUGRUL — brend belgisi.

Tuzilishi (tashqaridan ichkariga):
  1. Zarhal halqa + ichki nozik feruza chiziq
  2. Anadolu geometrik bezagi — halqa bo'ylab takrorlanuvchi tishlar
  3. Kesishgan ikki kilij (yalman bilan — uchi kengaygan turk qilichi)
  4. Markazda Qayi boyining IYI tamg'asi

Ranglar loyihaning "Temir va Firuza" dizayn tizimidan olingan.
Hamma narsa kod bilan chiziladi — istalgan o'lchamda qayta yaratiladi.
"""
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math, os

SS = 6                      # super-namuna (antialiasing uchun)
S  = 512 * SS
C  = S / 2.0

FON    = (12, 17, 20)
PANEL  = (24, 32, 36)
FERUZA = (72, 169, 181)
ZARHAL = (198, 156, 100)
ZAR_D  = (140, 105, 62)     # zarhalning soyasi
SUYAK  = (226, 218, 204)
POLAT  = (176, 186, 192)
POL_D  = (108, 118, 126)


def rot(p, a, o=None):
    o = o or (C, C)
    ca, sa = math.cos(a), math.sin(a)
    x, y = p[0] - o[0], p[1] - o[1]
    return (o[0] + x * ca - y * sa, o[1] + x * sa + y * ca)


# ---------------------------------------------------------------- kilij
def kilij(d, angle, light, dark, flip=False):
    """Turk kilichi: ozgina egri tig' va uchida KENGAYGAN yalman.
    Aynan yalman bu qilichni to'g'ri qilichdan ajratadi."""
    A, B = -0.44, 0.755                    # tig' boshi va uchi (nisbiy)
    top, bot = [], []
    N = 96
    for i in range(N + 1):
        t = i / float(N)
        u = A + (B - A) * t
        # egrilik: asta boshlanib uchga qarab kuchayadi
        # flip: ikkinchi qilich KO'ZGU aks bo'lsin — aks holda X simmetrik chiqmaydi
        bend = (-1.0 if flip else 1.0) * 0.095 * (t ** 1.8)
        # kenglik: o'rtada ingichkalashadi, yalmanda kengayadi, uchida nolga
        w = 0.0250 - 0.0075 * t
        if t > 0.70:                        # yalman — uchidagi kengayish, kichik
            w += 0.0072 * math.sin(math.pi * (t - 0.70) / 0.30)
        if t > 0.955:                       # uch
            w *= (1.0 - t) / 0.045
        x = C + u * S * 0.50
        y = C + bend * S
        top.append((x, y - w * S))
        bot.append((x, y + w * S))

    poly = [rot(p, angle) for p in top] + [rot(p, angle) for p in reversed(bot)]
    d.polygon(poly, fill=dark)
    # yorug' qirra — tig'ning tig'i
    edge = []
    for i in range(N + 1):
        t = i / float(N)
        x, y = top[i]
        yb = bot[i][1]
        edge.append((x, y + (yb - y) * 0.30))
    hi = [rot(p, angle) for p in top] + [rot(p, angle) for p in reversed(edge)]
    d.polygon(hi, fill=light)

    # qabza (gard) — ko'ndalang, uchlari ko'tarilgan
    gx = C + A * S * 0.50 + 0.020 * S
    gw, gh = 0.0125 * S, 0.062 * S
    d.polygon([rot(p, angle) for p in
               [(gx - gw, C - gh), (gx + gw, C - gh * 0.78),
                (gx + gw, C + gh * 0.78), (gx - gw, C + gh)]], fill=ZARHAL)
    d.polygon([rot(p, angle) for p in
               [(gx - gw, C + gh * 0.30), (gx + gw, C + gh * 0.40),
                (gx + gw, C + gh * 0.78), (gx - gw, C + gh)]], fill=ZAR_D)

    # dasta — ozgina egilgan
    hx = gx - 0.105 * S
    d.polygon([rot(p, angle) for p in
               [(hx, C - 0.0145 * S), (gx - gw, C - 0.0170 * S),
                (gx - gw, C + 0.0170 * S), (hx, C + 0.0145 * S)]],
              fill=(86, 60, 40))
    # dasta uchidagi qush boshi (kilijning an'anaviy elementi)
    p0 = rot((hx - 0.012 * S, C - 0.004 * S), angle)
    r = 0.023 * S
    d.ellipse([p0[0] - r, p0[1] - r, p0[0] + r, p0[1] + r], fill=ZARHAL)
    d.ellipse([p0[0] - r * 0.45, p0[1] - r * 0.45,
               p0[0] + r * 0.45, p0[1] + r * 0.45], fill=ZAR_D)


# ---------------------------------------------------------------- tamga
def tamga(d, cx, cy, h, col, shade=None):
    """Qayi boyining IYI tamg'asi.

    To'g'ri shakl: ikkita tik o'zak, har birining YUQORI uchida yuqoriga
    ochilgan juft tish, PASTKI uchida pastga ochilgan juft tish. Tishlar
    simmetrik — shuning uchun belgi "I Y I" bo'lib o'qiladi.
    Ilgari tishlar faqat bir tomonga qaragan edi va belgi qavsga o'xshardi.
    """
    t    = h * 0.098                  # chiziq qalinligi
    half = h * 0.5
    barb = h * 0.26                   # tish uzunligi
    spread = h * 0.19                 # tishning yon ochilishi

    def bar(p0, p1, w):
        """Ikki nuqta orasidagi qalin chiziq — to'ldirilgan to'rtburchak."""
        dx, dy = p1[0] - p0[0], p1[1] - p0[1]
        L = math.hypot(dx, dy) or 1.0
        nx, ny = -dy / L * w * 0.5, dx / L * w * 0.5
        return [(p0[0] + nx, p0[1] + ny), (p1[0] + nx, p1[1] + ny),
                (p1[0] - nx, p1[1] - ny), (p0[0] - nx, p0[1] - ny)]

    def glyph(x0):
        top, bot = (x0, cy - half), (x0, cy + half)
        return [
            bar(top, bot, t),                                            # o'zak
            bar(top, (x0 - spread, cy - half - barb), t * 0.92),         # yuqori chap
            bar(top, (x0 + spread, cy - half - barb), t * 0.92),         # yuqori o'ng
            bar(bot, (x0 - spread, cy + half + barb), t * 0.92),         # pastki chap
            bar(bot, (x0 + spread, cy + half + barb), t * 0.92),         # pastki o'ng
        ]

    gap = h * 0.30
    polys = glyph(cx - gap) + glyph(cx + gap)
    if shade:
        for p in polys:
            d.polygon([(x + t * 0.15, y + t * 0.15) for x, y in p], fill=shade)
    for p in polys:
        d.polygon(p, fill=col)


# ---------------------------------------------------------------- bezak
def ornament(d, r_out, r_in, n, col):
    """Halqa bo'ylab takrorlanuvchi uchburchak tishlar — Anadolu bezagi."""
    for i in range(n):
        a0 = (i / float(n)) * math.tau
        a1 = ((i + 0.42) / float(n)) * math.tau
        am = (a0 + a1) / 2.0
        p = [(C + math.cos(a0) * r_out, C + math.sin(a0) * r_out),
             (C + math.cos(a1) * r_out, C + math.sin(a1) * r_out),
             (C + math.cos(am) * r_in,  C + math.sin(am) * r_in)]
        d.polygon(p, fill=col)


def build_mark(path):
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    R = S * 0.470

    # asos
    d.ellipse([C - R, C - R, C + R, C + R], fill=FON)
    # tashqi zarhal halqa
    d.ellipse([C - R, C - R, C + R, C + R], outline=ZARHAL, width=int(S * 0.0165))
    # bezak tishlari
    ornament(d, R - S * 0.020, R - S * 0.052, 48, (30, 40, 45))
    # ichki feruza chiziq
    ri = R - S * 0.062
    d.ellipse([C - ri, C - ri, C + ri, C + ri], outline=FERUZA, width=int(S * 0.0042))
    ri2 = ri - S * 0.011
    d.ellipse([C - ri2, C - ri2, C + ri2, C + ri2], outline=(26, 35, 40),
              width=int(S * 0.0055))

    # kesishgan qilichlar
    kilij(d, math.radians(-36.0), POLAT, POL_D)
    kilij(d, math.radians(-144.0), (196, 205, 211), (128, 138, 146), flip=True)

    # markaziy medalyon
    rs = S * 0.170
    d.ellipse([C - rs, C - rs, C + rs, C + rs], fill=PANEL)
    d.ellipse([C - rs, C - rs, C + rs, C + rs], outline=ZARHAL, width=int(S * 0.0095))
    rs2 = rs - S * 0.020
    d.ellipse([C - rs2, C - rs2, C + rs2, C + rs2], outline=(38, 48, 54),
              width=int(S * 0.0035))

    # tamg'a
    tamga(d, C, C, S * 0.150, ZARHAL, ZAR_D)

    # to'rt yo'nalishdagi "mix boshi" kvadratlari
    q = S * 0.0155
    for a in (0, 90, 180, 270):
        px, py = rot((C, C - R + S * 0.0085), math.radians(a))
        d.rectangle([px - q, py - q, px + q, py + q], fill=FERUZA)
        d.rectangle([px - q * 0.42, py - q * 0.42, px + q * 0.42, py + q * 0.42],
                    fill=FON)

    img = img.resize((1024, 1024), Image.LANCZOS)
    img.save(path)
    return img


def font(sz, bold=True):
    for nm in (("georgiab.ttf" if bold else "georgia.ttf"),
               ("timesbd.ttf" if bold else "times.ttf"),
               ("arialbd.ttf" if bold else "arial.ttf")):
        try:
            return ImageFont.truetype("C:/Windows/Fonts/" + nm, sz)
        except Exception:
            continue
    return ImageFont.load_default()


def build_full(path, mark):
    """Vertikal qulf: belgi + so'z belgisi."""
    W, H = 1500, 1420
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    m = mark.resize((940, 940), Image.LANCZOS)
    out.paste(m, ((W - 940) // 2, 20), m)
    d = ImageDraw.Draw(out)

    def ctr(txt, f, y, col, sp=0):
        wtot = sum(d.textlength(ch, font=f) + sp for ch in txt) - sp
        x = (W - wtot) / 2.0
        for ch in txt:
            d.text((x, y), ch, font=f, fill=col)
            x += d.textlength(ch, font=f) + sp

    # nozik ajratuvchi
    d.rectangle([W / 2 - 250, 1002, W / 2 + 250, 1004], fill=(52, 64, 70))
    d.rectangle([W / 2 - 9, 995, W / 2 + 9, 1011], fill=FERUZA)

    ctr("BLADES OF ANATOLIA", font(74), 1052, SUYAK, sp=13)
    ctr("ERTUGRUL", font(158), 1150, ZARHAL, sp=18)
    out.save(path)
    return out


def build_wordmark(path, mark):
    """Gorizontal qulf — treyler, afisha va sayt sarlavhasi uchun."""
    W, H = 2200, 620
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    m = mark.resize((520, 520), Image.LANCZOS)
    out.paste(m, (40, 50), m)
    d = ImageDraw.Draw(out)
    x0 = 620
    d.rectangle([x0, 150, x0 + 2, 470], fill=(52, 64, 70))
    f1, f2 = font(64), font(140)
    x = x0 + 58
    xx = x
    for ch in "BLADES OF ANATOLIA":
        d.text((xx, 178), ch, font=f1, fill=SUYAK)
        xx += d.textlength(ch, font=f1) + 11
    xx = x
    for ch in "ERTUGRUL":
        d.text((xx, 272), ch, font=f2, fill=ZARHAL)
        xx += d.textlength(ch, font=f2) + 15
    out.save(path)
    return out


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    mk = build_mark(os.path.join(here, "logo_mark.png"))
    build_full(os.path.join(here, "logo_full.png"), mk)
    build_wordmark(os.path.join(here, "logo_wordmark.png"), mk)
    print("logo tayyor: belgi, vertikal qulf, gorizontal qulf")
