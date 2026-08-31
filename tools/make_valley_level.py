# -*- coding: utf-8 -*-
"""Kinematik demo xarita: "Qayi obasi — vodiy".

Kompozitsiya g'oyasi:
  - Markazda TEKIS maydon: obaning halqa bo'lib tizilgan chodirlari
  - Atrofda ko'tarilgan tepaliklar — ufq bo'sh qolmaydi, ramka hosil bo'ladi
  - Tepaliklarda qalin qarag'ayzor, oldinda siyrak buta va o't
  - Oltin soat (dusk) yoritishi: quyosh past, soyalar uzun, tuman iliq

Bu fayl data/levels/oba_valley.json ni yaratadi.
"""
import json, math, os, random

random.seed(20261)

M = "assets/models/"
NAT = M + "nature/"
TOWN = M + "town/"

props = []


def P(mesh, x, z, yaw=0.0, scale=1.0, tint=(1, 1, 1), collide=False, r=0.5, y=0.0):
    props.append({
        "mesh": mesh, "pos": [round(x, 2), y, round(z, 2)],
        "yaw": round(yaw % 360.0, 1), "scale": round(scale, 2),
        "tint": [round(c, 3) for c in tint],
        "collide": bool(collide), "radius": round(r, 2),
    })


def jit(a, b):
    return random.uniform(a, b)


# ---------------------------------------------------------------- yo'lak
# Obaning JANUBIY tomonida kirish yo'lagi ochiq qoladi. Ikki sabab:
#   1. Haqiqiy obada kirish og'zi bo'ladi — chodirlar halqasi uzilgan
#   2. Cutscene sahnalashtirishi aynan shu sektorda kechadi (o'lchandi:
#      burchak +-40 gradus, radius 15-46). Bu yerda to'qnashuvchi rekvizit
#      qo'ysak, aktyorlar chodir ichidan o'tib ketadi.
CORR_HALF_DEG = 42.0
CORR_R0, CORR_R1 = 9.0, 48.0


def in_corridor(x, z):
    r = math.hypot(x, z)
    if r < CORR_R0 or r > CORR_R1:
        return False
    ang = abs(math.degrees(math.atan2(x, z)))     # +Z o'qidan burchak
    return ang <= CORR_HALF_DEG


FELT   = (0.70, 0.55, 0.40)     # kigiz — iliq oq
FELT_D = (0.58, 0.44, 0.32)
WOOD   = (0.50, 0.35, 0.24)
WOOD_L = (0.62, 0.46, 0.31)

# ---------------------------------------------------------------- chodirlar
# Oba an'anaviy ravishda HALQA bo'lib tiziladi: eshiklar markazga qaraydi,
# markazda bey chodiri va gulxan. Bu kompozitsiya kameradan chiroyli ko'rinadi.
# Kigiz rangli variantlar (tools/make_felt_tents.py yaratadi)
BIG = NAT + "yurt_felt_detailedClosed.obj"
OPEN = NAT + "yurt_felt_detailedOpen.obj"
SMALL_C = NAT + "yurt_felt_smallClosed.obj"
SMALL_O = NAT + "yurt_felt_smallOpen.obj"

# Bey chodiri — markazdan shimolda, eng katta
P(BIG, 0.0, -9.0, 180.0, 9.6, FELT, True, 3.9)
P(TOWN + "banner-red.obj", -3.6, -9.4, 176.0, 3.4, (0.62, 0.20, 0.18))
P(TOWN + "banner-red.obj",  3.6, -9.4, 184.0, 3.4, (0.62, 0.20, 0.18))

ring = [
    (23.0, 12),   # ichki halqa — katta chodirlar
    (34.0, 16),   # tashqi halqa — kichik chodirlar
]
for ri, (rad, cnt) in enumerate(ring):
    for i in range(cnt):
        a = (i / float(cnt)) * math.tau + (0.13 if ri else 0.0)
        x, z = math.cos(a) * rad + jit(-1.6, 1.6), math.sin(a) * rad + jit(-1.6, 1.6)
        # eshik markazga qaraydi
        yaw = math.degrees(math.atan2(-x, -z))
        if ri == 0:
            mesh = BIG if (i % 3) else OPEN
            sc = jit(6.6, 8.2)
        else:
            mesh = SMALL_C if (i % 2) else SMALL_O
            sc = jit(4.8, 6.2)
        if in_corridor(x, z):
            continue                      # kirish og'zi ochiq qoladi
        t = FELT if (i % 2) else FELT_D
        P(mesh, x, z, yaw + jit(-8, 8), sc,
          (t[0] * jit(0.92, 1.06), t[1] * jit(0.92, 1.06), t[2] * jit(0.92, 1.06)),
          True, sc * 0.40)

# ---------------------------------------------------------------- gulxanlar
fires = [(0.0, 6.0), (-16.0, 14.0), (17.0, 12.0), (-19.0, -12.0), (18.0, -14.0)]
for (fx, fz) in fires:
    if in_corridor(fx, fz):
        continue
    P(NAT + "campfire_stones.obj", fx, fz, jit(0, 360), 2.9, (0.46, 0.44, 0.42))
    P(NAT + "campfire_logs.obj",   fx, fz, jit(0, 360), 2.6, (0.42, 0.28, 0.18))
    for k in range(3):
        a = jit(0, math.tau)
        P(NAT + "log.obj", fx + math.cos(a) * 3.1, fz + math.sin(a) * 3.1,
          math.degrees(a) + 90, jit(1.6, 2.2), WOOD)
    P(TOWN + "lantern.obj", fx + jit(-4.5, 4.5), fz + jit(-4.5, 4.5), jit(0, 360),
      2.4, (0.78, 0.64, 0.40))

# ---------------------------------------------------------------- darvoza va devor
# Janubda kirish darvozasi — video shu yerdan boshlanadi
# Darvoza YO'LAKDAN TASHQARIDA — ikki ustun keng ochiq turadi, o'rtasi bo'sh.
P(TOWN + "pillar-wood.obj", -12.0, 49.5, 0.0, 5.4, WOOD, True, 0.8)
P(TOWN + "pillar-wood.obj",  12.0, 49.5, 0.0, 5.4, WOOD, True, 0.8)
P(TOWN + "banner-green.obj", -12.0, 48.6, 0.0, 4.0, (0.26, 0.46, 0.34))
P(TOWN + "banner-green.obj",  12.0, 48.6, 0.0, 4.0, (0.26, 0.46, 0.34))

# Yog'och panjara — darvozadan ikki tomonga yoy bo'lib ketadi
for side in (-1, 1):
    for i in range(11):
        a = math.radians(90 - side * (10 + i * 7.0))
        x, z = math.cos(a) * 44.5, math.sin(a) * 44.5
        yaw = math.degrees(a) + 90
        if in_corridor(x, z):
            continue
        P(TOWN + "fence.obj", x, z, yaw, 4.2, WOOD, True, 1.5)

# ---------------------------------------------------------------- turmush
carts = [(-11.0, 26.0, 40), (13.0, 27.0, -35), (-26.0, 4.0, 95), (27.0, -3.0, -80)]
for (x, z, yw) in carts:
    if in_corridor(x, z):
        continue
    P(TOWN + "cart.obj", x, z, yw, 3.4, WOOD_L, True, 1.4)
P(TOWN + "cart-high.obj", -21.0, 20.0, 20, 3.2, WOOD, True, 1.4)

for i, (x, z) in enumerate([(-22.0, 18.0), (23.0, 19.0), (-14.0, -20.0), (15.0, -19.0)]):
    P(NAT + "log_stack.obj", x, z, jit(0, 360), 2.4, WOOD)
    P(NAT + "log_stackLarge.obj", x + jit(-3, 3), z + jit(-3, 3), jit(0, 360), 2.0, WOOD_L)

for i, (x, z) in enumerate([(-24.0, 30.0), (25.0, 31.0)]):
    P(TOWN + "stall-red.obj" if i % 2 else TOWN + "stall-green.obj",
      x, z, 180 + jit(-20, 20), 3.0, (0.55, 0.34, 0.28) if i % 2 else (0.34, 0.48, 0.34),
      True, 1.4)
    P(TOWN + "stall-bench.obj", x + 2.2, z + 1.4, jit(0, 360), 2.4, WOOD)
    P(TOWN + "stall-stool.obj", x - 1.8, z + 1.6, jit(0, 360), 2.0, WOOD)

# obelisk — markaziy nishon, uzoqdan ko'rinadi
P(NAT + "statue_obelisk.obj", -6.5, -4.0, 0.0, 4.6, (0.52, 0.50, 0.46), True, 1.2)

# ---------------------------------------------------------------- katta toshlar
for i in range(26):
    a = jit(0, math.tau)
    rad = jit(46.0, 92.0)
    mesh = random.choice([NAT + "rock_largeA.obj", NAT + "rock_largeB.obj",
                          NAT + "rock_largeC.obj"])
    rx, rz = math.cos(a) * rad, math.sin(a) * rad
    if in_corridor(rx, rz):
        continue
    P(mesh, rx, rz, jit(0, 360), jit(3.2, 6.4),
      (jit(0.44, 0.56), jit(0.42, 0.52), jit(0.40, 0.48)), True, 2.2)

# ---------------------------------------------------------------- scatter
scatter = [
    {   # tepaliklardagi qalin qarag'ayzor — ufqni to'ldiradi
        "meshes": [NAT + "tree_pineTallA.obj", NAT + "tree_pineDefaultA.obj",
                   NAT + "tree_pineRoundA.obj", NAT + "tree_pineTallA_detailed.obj"],
        "count": 420, "min_radius": 58, "max_radius": 176,
        "scale_min": 3.8, "scale_max": 7.6,
        "tint": [0.30, 0.44, 0.28], "tint_jitter": 0.20,
        "collide": True, "radius": 1.4, "min_gap": 3.0, "min_slope_y": 0.55,
    },
    {   # ikkinchi qatlam — bargli daraxtlar, iliq rang (oltin soat)
        "meshes": [NAT + "tree_oak.obj", NAT + "tree_default.obj",
                   NAT + "tree_fat.obj", NAT + "tree_detailed.obj"],
        "count": 150, "min_radius": 50, "max_radius": 140,
        "scale_min": 3.4, "scale_max": 6.2,
        "tint": [0.52, 0.46, 0.24], "tint_jitter": 0.22,
        "collide": True, "radius": 1.5, "min_gap": 4.0, "min_slope_y": 0.55,
    },
    {   # oba chekkasidagi butalar
        "meshes": [NAT + "plant_bushLarge.obj", NAT + "plant_bushDetailed.obj",
                   NAT + "plant_bush.obj", NAT + "plant_bushSmall.obj"],
        "count": 240, "min_radius": 14, "max_radius": 64,
        "scale_min": 1.1, "scale_max": 2.3,
        "tint": [0.38, 0.46, 0.26], "tint_jitter": 0.18,
        "collide": False, "radius": 0.6, "min_gap": 1.6, "min_slope_y": 0.4,
    },
    {   # o't — eng zich qatlam, yerni jonlantiradi
        "meshes": [NAT + "grass.obj", NAT + "grass_large.obj",
                   NAT + "grass_leafs.obj", NAT + "grass_leafsLarge.obj"],
        "count": 700, "min_radius": 8, "max_radius": 78,
        "scale_min": 0.7, "scale_max": 1.5,
        "tint": [0.46, 0.50, 0.26], "tint_jitter": 0.22,
        "collide": False, "radius": 0.3, "min_gap": 0.9, "min_slope_y": 0.35,
    },
    {   # gullar — kichik rang urg'usi
        "meshes": [NAT + "flower_yellowA.obj", NAT + "flower_redA.obj",
                   NAT + "flower_purpleA.obj"],
        "count": 200, "min_radius": 10, "max_radius": 58,
        "scale_min": 0.8, "scale_max": 1.4,
        "tint": [0.92, 0.82, 0.44], "tint_jitter": 0.30,
        "collide": False, "radius": 0.25, "min_gap": 1.1, "min_slope_y": 0.35,
    },
    {   # kichik toshlar va kunda — yaqin plandagi detal
        "meshes": [NAT + "rock_smallA.obj", NAT + "rock_smallB.obj",
                   NAT + "stump_round.obj", NAT + "stump_old.obj"],
        "count": 150, "min_radius": 12, "max_radius": 86,
        "scale_min": 1.6, "scale_max": 3.2,
        "tint": [0.48, 0.45, 0.41], "tint_jitter": 0.16,
        "collide": False, "radius": 0.5, "min_gap": 2.0, "min_slope_y": 0.4,
    },
]

level = {
    "id": "oba_valley",
    "loc_name": "Qayi obasi — vodiy",
    "terrain": {
        "grid": 160,          # zichroq to'r — tepaliklar silliq
        "size": 400,
        "seed": 90210,
        "hill_height": 15,    # haqiqiy tepaliklar (ilgari 6-7 edi)
        "flat_radius": 46,    # oba tekis maydonda turadi
    },
    "sky": {"time_of_day": "golden", "weather": "clear"},
    "spawns": [
        {"id": "player",  "pos": [0.0, 0.0, 38.0], "yaw": 180.0},
        {"id": "enemy_a", "pos": [-9.0, 0.0, 14.0], "yaw": 200.0},
        {"id": "enemy_b", "pos": [10.0, 0.0, 15.0], "yaw": 160.0},
        {"id": "enemy_c", "pos": [0.0, 0.0, 22.0], "yaw": 180.0},
    ],
    "props": props,
    "scatter": scatter,
}

out = os.path.join("data", "levels", "oba_valley.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(level, f, ensure_ascii=False, indent=1)
print("yozildi: %s  |  %d prop, %d scatter turi, jami ~%d obyekt"
      % (out, len(props), len(scatter),
         len(props) + sum(s["count"] for s in scatter)))
