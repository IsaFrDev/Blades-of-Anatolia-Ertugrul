# Pishirilgan teksturalarni uzluksiz (tileable) qilish: yarim siljitish + chekka cross-fade. Kirish/chiqish: art/blender_tex -> art/blender_tex/seamless
import os, sys
from PIL import Image
import numpy as np
SRC = "D:/Unreal_projects/Ertugrul/art/blender_tex"
DST = SRC + "/seamless"
os.makedirs(DST, exist_ok=True)
FEATHER = 0.14

def seamless(img):
    a = np.asarray(img).astype(np.float32)
    h, w = a.shape[:2]
    # yarmiga siljitish: choklar markazga keladi
    b = np.roll(np.roll(a, h // 2, axis=0), w // 2, axis=1)
    fh, fw = int(h * FEATHER), int(w * FEATHER)
    # markaziy gorizontal chok (qator h/2) va vertikal chok (ustun w/2) atrofida asl (siljitilmagan) rasm bilan aralashtirish
    wy = np.ones(h, np.float32); wx = np.ones(w, np.float32)
    y = np.arange(h); x = np.arange(w)
    wy = np.clip((np.abs(y - h // 2) - 0) / fh, 0, 1); wx = np.clip((np.abs(x - w // 2) - 0) / fw, 0, 1)
    wy = wy * wy * (3 - 2 * wy); wx = wx * wx * (3 - 2 * wx)
    m = np.minimum(wy[:, None], wx[None, :])[:, :, None]
    out = b * m + a * (1 - m)
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8))

n = 0
for f in sorted(os.listdir(SRC)):
    if not f.lower().endswith(".png"): continue
    img = Image.open(os.path.join(SRC, f)).convert("RGB")
    seamless(img).save(os.path.join(DST, f)); n += 1
    print("seamless", f)
print("done", n)
