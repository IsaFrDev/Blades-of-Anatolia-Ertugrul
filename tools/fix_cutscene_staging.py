# -*- coding: utf-8 -*-
"""Cutscene kamera va aktyor yo'llarini daraja rekvizitlaridan chetlatadi.

Sabab: cutscene fayllari va daraja fayllari alohida yozilgani uchun kamera
lagerdagi katta chodir ichidan o'tib, ekranni mato bilan to'ldirib qo'yardi.

Ishlatish:  python tools/fix_cutscene_staging.py [--check]
  --check  -> hech narsa yozmaydi, faqat muammolarni sanaydi (CI uchun)
"""
import json, io, os, math, sys, glob

DEFAULT_LEVEL = 'oba_camp'
KNOWN_LEVELS = ('oba_camp', 'forest_pass', 'aleppo_road')
CAM_CLEAR   = 2.6       # kamera uchun qo'shimcha bo'shliq (m)
ACTOR_CLEAR = 0.7       # aktyor uchun (yelka kengligi)
SEG_SAMPLES = 8         # kalitlar orasidagi tekshiruv nuqtalari


def load_props(level_id):
    p = 'data/levels/%s.json' % level_id
    if not os.path.exists(p):
        return []
    j = json.load(io.open(p, encoding='utf-8'))
    out = []
    for pr in j.get('props', []):
        x, _, z = pr['pos']
        r = float(pr.get('radius', 0.5))
        # ko'rinish uchun radius kamida modelning yarim kengligi bo'lsin
        out.append((x, z, max(r, 0.5)))
    return out


def worst(props, x, z, extra):
    """Eng qattiq buzilish (musbat = buzilgan) va aybdor rekvizit."""
    bad, who = 0.0, None
    for px, pz, pr in props:
        need = pr + extra
        d = math.hypot(x - px, z - pz)
        if need - d > bad:
            bad, who = need - d, (px, pz, pr)
    return bad, who


def push_out(props, x, z, extra, max_iter=24):
    for _ in range(max_iter):
        bad, who = worst(props, x, z, extra)
        if bad <= 1e-3:
            break
        px, pz, _ = who
        dx, dz = x - px, z - pz
        d = math.hypot(dx, dz)
        if d < 1e-4:
            dx, dz, d = 1.0, 0.0, 1.0
        x += dx / d * (bad + 0.05)
        z += dz / d * (bad + 0.05)
    return x, z


def process(path, check_only):
    scene = json.load(io.open(path, encoding='utf-8'))
    # Sahnaning O'Z darajasidan foydalanamiz (avval qat'iy oba_camp edi)
    level = str(scene.get('level', '')).lower()
    if level not in KNOWN_LEVELS:
        level = DEFAULT_LEVEL
    props = load_props(level)
    if not props:
        return 0, 0
    fixed = issues = 0

    for key in scene.get('camera', []):
        x, y, z = key['pos']
        bad, _ = worst(props, x, z, CAM_CLEAR)
        if bad > 1e-3:
            issues += 1
            if not check_only:
                nx, nz = push_out(props, x, z, CAM_CLEAR)
                key['pos'] = [round(nx, 2), round(max(y, 1.7), 2), round(nz, 2)]
                fixed += 1

    for actor in scene.get('actors', []):
        for key in actor.get('keys', []):
            x, y, z = key['pos']
            bad, _ = worst(props, x, z, ACTOR_CLEAR)
            if bad > 1e-3:
                issues += 1
                if not check_only:
                    nx, nz = push_out(props, x, z, ACTOR_CLEAR)
                    key['pos'] = [round(nx, 2), round(y, 2), round(nz, 2)]
                    fixed += 1

    # kalitlar orasidagi kesmalar ham tekshiriladi (kamera yo'l ustida chodirni kesib o'tmasin)
    cam = scene.get('camera', [])
    for i in range(len(cam) - 1):
        a, b = cam[i]['pos'], cam[i + 1]['pos']
        for s in range(1, SEG_SAMPLES):
            t = s / float(SEG_SAMPLES)
            x = a[0] + (b[0] - a[0]) * t
            z = a[2] + (b[2] - a[2]) * t
            bad, who = worst(props, x, z, CAM_CLEAR * 0.75)
            if bad > 1e-3:
                issues += 1
                if not check_only:
                    # ikkala uchni ham aybdordan uzoqlashtiramiz
                    px, pz, _ = who
                    for key in (cam[i], cam[i + 1]):
                        kx, kz = key['pos'][0], key['pos'][2]
                        dx, dz = kx - px, kz - pz
                        d = math.hypot(dx, dz) or 1.0
                        key['pos'][0] = round(kx + dx / d * (bad * 0.6 + 0.1), 2)
                        key['pos'][2] = round(kz + dz / d * (bad * 0.6 + 0.1), 2)
                    fixed += 1
                break

    if fixed and not check_only:
        io.open(path, 'w', encoding='utf-8', newline='\n').write(
            json.dumps(scene, ensure_ascii=False, indent=2))
    return issues, fixed


def main():
    check_only = '--check' in sys.argv
    total_i = total_f = 0
    for path in sorted(glob.glob('data/cutscenes/*.json')):
        i, f = process(path, check_only)
        total_i += i
        total_f += f
        status = 'MUAMMO %d' % i if i else 'toza'
        extra = ' -> tuzatildi %d' % f if f else ''
        print('  %-34s %s%s' % (os.path.basename(path), status, extra))
    print('\nJami muammo: %d, tuzatildi: %d' % (total_i, total_f))
    return 1 if (check_only and total_i) else 0


if __name__ == '__main__':
    sys.exit(main())
