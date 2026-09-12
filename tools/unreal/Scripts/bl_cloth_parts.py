# Kiyim harakati: kaftan etagi + mo'yna yopinchiq + yeng uchlari (suyakli tebranish rigi)
# ERT_PROBE=1 -> faqat o'lchov. Chiqish: bl_ert/kaftan/k###.png, SK_Ertugrul_Cloth.fbx
import bpy, os, math
from mathutils import Vector, Quaternion
OUT = "D:/temp/claude/bl_ert/kaftan"; os.makedirs(OUT, exist_ok=True)
FRAMES = int(os.environ.get("ERT_FRAMES", "48"))
PROBE = os.environ.get("ERT_PROBE", "0") == "1"
RENDER = os.environ.get("ERT_RENDER", "1") == "1"
CUFF = os.environ.get("ERT_CUFF", "1") == "1"
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
sc = bpy.context.scene
ert = bpy.data.objects["Ertugrul"]
for n in ["Cape", "Shield", "ShieldRim", "ShieldBoss", "KaftanRig"] + ["Rivet%d" % i for i in range(12)]:
    o = bpy.data.objects.get(n)
    if o: bpy.data.objects.remove(o)
for m in [m for m in ert.modifiers if m.type in ('CLOTH', 'COLLISION', 'ARMATURE')]:
    ert.modifiers.remove(m)
ert.parent = None; ert.animation_data_clear(); ert.location = (0, 0, 0); ert.rotation_euler = (0, 0, 0)
bpy.context.view_layer.update()
H = ert.dimensions.z
V = ert.data.vertices
if PROBE:
    print("HEIGHT", round(H, 3), "verts", len(V), flush=True)
    for i in range(20):
        z0, z1 = H * i / 20.0, H * (i + 1) / 20.0
        band = [v.co for v in V if z0 <= v.co.z < z1]
        if not band: continue
        rmax = max(math.hypot(c.x, c.y) for c in band)
        xmax = max(abs(c.x) for c in band)
        print("Z %.2f-%.2f n=%5d rmax=%.3f xmax=%.3f" % (z0, z1, len(band), rmax, xmax), flush=True)
    raise SystemExit

def band_rmax(z0, z1):
    b = [v.co for v in V if z0 <= v.co.z < z1]
    return max((math.hypot(c.x, c.y) for c in b), default=0.3)
def smooth(x):
    x = min(1.0, max(0.0, x)); return x * x * (3 - 2 * x)

SKIRT_HI, SKIRT_LO = 0.500 * H, 0.325 * H   # kamar va xaltacha tepada qoladi
MANT_HI, MANT_LO = 0.845 * H, 0.700 * H          # mo'yna yopinchiq (yelka -> ko'krak)
CUFF_HI, CUFF_LO = 0.610 * H, 0.500 * H          # yeng uchi (bilakning sleeve qismi, kaftdan yuqori)
ARM_X, ARM_Y = 0.30, 0.12                        # qo'l zonasi: kaftan yon etagidan tashqarida
skirt_r = 0.60 * band_rmax(SKIRT_LO, SKIRT_HI)
mant_r = 0.62 * band_rmax(MANT_LO, MANT_HI)
def is_arm(x, y, z):
    return abs(x) > ARM_X and abs(y) < ARM_Y and 0.44 * H < z < 0.74 * H
xmax = max((abs(v.co.x) for v in V if CUFF_LO < v.co.z < CUFF_HI and abs(v.co.y) < ARM_Y), default=0.35)
cuff_x = ARM_X
print("H %.2f skirt_r %.3f mant_r %.3f cuff_x %.3f" % (H, skirt_r, mant_r, cuff_x), flush=True)

# ---------- Armatura ----------
SECT = [("f", 0.0), ("r", math.pi * 0.5), ("b", math.pi), ("l", math.pi * 1.5)]
bpy.ops.object.armature_add(enter_editmode=True, location=(0, 0, 0))
arm = bpy.context.active_object; arm.name = "ClothRig"; arm.data.name = "ClothRig"
eb = arm.data.edit_bones
for b in list(eb): eb.remove(b)
root = eb.new("root"); root.head = (0, 0, SKIRT_HI); root.tail = (0, 0, SKIRT_HI + 0.12)
SK_MID = (SKIRT_HI + SKIRT_LO) * 0.5
for name, ang in SECT:
    dx, dy = math.cos(ang), math.sin(ang); r0 = skirt_r * 0.85
    b1 = eb.new("skirt_%s_1" % name); b1.head = (dx * r0, dy * r0, SKIRT_HI); b1.tail = (dx * r0 * 1.05, dy * r0 * 1.05, SK_MID); b1.parent = root
    b2 = eb.new("skirt_%s_2" % name); b2.head = b1.tail; b2.tail = (dx * r0 * 1.1, dy * r0 * 1.1, SKIRT_LO - 0.02); b2.parent = b1; b2.use_connect = True
    m1 = eb.new("mantle_%s" % name); r1 = mant_r * 0.8
    m1.head = (dx * r1, dy * r1, MANT_HI); m1.tail = (dx * r1 * 1.12, dy * r1 * 1.12, MANT_LO - 0.01); m1.parent = root
for side, sx in (("l", -1.0), ("r", 1.0)):
    c = eb.new("cuff_%s" % side)
    c.head = (sx * cuff_x * 0.95, 0.0, CUFF_HI); c.tail = (sx * cuff_x * 1.12, 0.0, CUFF_LO - 0.02); c.parent = root
bpy.ops.object.mode_set(mode='OBJECT')

# ---------- Og'irliklar ----------
for g in list(ert.vertex_groups): ert.vertex_groups.remove(g)
G = {}
def vg(n):
    if n not in G: G[n] = ert.vertex_groups.new(name=n)
    return G[n]
n_sk = n_mn = n_cf = 0
for v in V:
    x, y, z = v.co.x, v.co.y, v.co.z
    r = math.hypot(x, y)
    # 1) Kaftan etagi
    if SKIRT_LO < z < SKIRT_HI and r > skirt_r and not is_arm(x, y, z):
        t = smooth((SKIRT_HI - z) / (SKIRT_HI - SKIRT_LO))
        if t > 0.001:
            a = math.atan2(y, x) % (2 * math.pi); n_sk += 1
            for name, ang in SECT:
                d = abs((a - ang + math.pi) % (2 * math.pi) - math.pi)
                w = max(0.0, 1.0 - d / (math.pi * 0.5))
                if w <= 0.001: continue
                u = (SKIRT_HI - z) / (SKIRT_HI - SKIRT_LO)
                w1 = t * w * (1.0 - smooth(u - 0.15)); w2 = t * w * smooth((u - 0.35) / 0.65)
                if w1 > 0.001: vg("skirt_%s_1" % name).add([v.index], min(1.0, w1), 'REPLACE')
                if w2 > 0.001: vg("skirt_%s_2" % name).add([v.index], min(1.0, w2), 'REPLACE')
    # 2) Mo'yna yopinchiq (faqat pastki chekkasi qimirlaydi)
    if MANT_LO < z < MANT_HI and r > mant_r and not is_arm(x, y, z):
        t = smooth((MANT_HI - z) / (MANT_HI - MANT_LO)) * 0.85
        if t > 0.001:
            a = math.atan2(y, x) % (2 * math.pi); n_mn += 1
            for name, ang in SECT:
                d = abs((a - ang + math.pi) % (2 * math.pi) - math.pi)
                w = max(0.0, 1.0 - d / (math.pi * 0.5))
                if w > 0.001: vg("mantle_%s" % name).add([v.index], min(1.0, t * w), 'REPLACE')
    # 3) Yeng uchlari
    if CUFF and CUFF_LO < z < CUFF_HI and abs(x) > cuff_x and abs(y) < ARM_Y:
        t = smooth((abs(x) - cuff_x) / max(1e-4, xmax - cuff_x)) * smooth((CUFF_HI - z) / (CUFF_HI - CUFF_LO))
        if t > 0.001:
            vg("cuff_%s" % ("r" if x > 0 else "l")).add([v.index], min(1.0, t), 'REPLACE'); n_cf += 1
print("WEIGHTS skirt %d mantle %d cuff %d" % (n_sk, n_mn, n_cf), flush=True)
ert.parent = arm
ert.modifiers.new("Armature", 'ARMATURE').object = arm

# ---------- Animatsiya ----------
arm.animation_data_clear()
for pb in arm.pose.bones: pb.rotation_mode = 'QUATERNION'
SPEED = 3.2
MOVE = Vector((0.0, -1.0, 0.0))
AXIS_W = Vector((0.0, 0.0, 1.0)).cross(MOVE).normalized()
SIDE_W = MOVE.copy()
rest = {b.name: b.matrix_local.to_3x3() for b in arm.data.bones}
def la(bn, ax): return (rest[bn].inverted() @ ax).normalized()
def key(bn, lift, side, f):
    pb = arm.pose.bones[bn]
    pb.rotation_quaternion = Quaternion(la(bn, AXIS_W), lift) @ Quaternion(la(bn, SIDE_W), side)
    pb.keyframe_insert("rotation_quaternion", frame=f)
for f in range(1, FRAMES + 1):
    t = (f - 1) / 24.0
    step = math.sin(t * math.pi * 3.2)
    arm.location = (0.0, -t * SPEED, 0.03 * abs(step))
    arm.rotation_euler = (math.radians(2.0 * step), 0.0, 0.0)
    arm.keyframe_insert("location", frame=f); arm.keyframe_insert("rotation_euler", frame=f)
    for name, ang in SECT:
        facing = Vector((math.cos(ang), math.sin(ang), 0.0)).dot(-MOVE)
        def sway(tt):
            return (math.radians(12.0 + 16.0 * facing) + math.radians(17.0) * math.sin(tt * math.pi * 3.2 + ang),
                    math.radians(11.0) * math.sin(tt * math.pi * 2.1 + ang * 1.3))
        l1, s1 = sway(t); l2, s2 = sway(t - 0.14)
        key("skirt_%s_1" % name, l1, s1, f)
        key("skirt_%s_2" % name, l2 * 1.1, s2 * 1.2, f)
        # mo'yna: qisqa, qattiqroq - kichik amplituda, tez tebranish
        ml = math.radians(4.0 + 6.0 * facing) + math.radians(7.0) * math.sin(t * math.pi * 3.2 + ang + 0.5)
        ms = math.radians(5.0) * math.sin(t * math.pi * 2.6 + ang)
        key("mantle_%s" % name, ml, ms, f)
    # yeng uchlari: qadam ritmida oldinga-orqaga (chap/o'ng qarama-qarshi fazada)
    for side, ph in (("l", 0.0), ("r", math.pi)):
        cl = math.radians(8.0) * math.sin(t * math.pi * 3.2 + ph) + math.radians(3.0)
        cs = math.radians(4.0) * math.sin(t * math.pi * 2.4 + ph)
        key("cuff_%s" % side, cl, cs, f)
for fc in arm.animation_data.action.fcurves:
    for kp in fc.keyframe_points: kp.interpolation = 'BEZIER'

# ---------- Kamera + render ----------
cam = bpy.data.objects.get("Cam_Front34") or bpy.data.objects.get("FlyCam")
cam.animation_data_clear()
for f in range(1, FRAMES + 1):
    t = (f - 1) / 24.0
    p = Vector((1.5, -t * SPEED + 3.4, 1.35)); cam.location = p
    d = Vector((0.0, -t * SPEED, 0.95)) - p
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    cam.keyframe_insert("location", frame=f); cam.keyframe_insert("rotation_euler", frame=f)
cam.data.lens = 42; sc.camera = cam
sc.frame_start = 1; sc.frame_end = FRAMES; sc.render.fps = 24
sc.render.engine = 'BLENDER_EEVEE_NEXT'; sc.eevee.taa_render_samples = 6
sc.render.resolution_x = 960; sc.render.resolution_y = 540; sc.render.image_settings.file_format = 'PNG'
bpy.ops.wm.save_as_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_cloth.blend")
bpy.ops.object.select_all(action='DESELECT'); ert.select_set(True); arm.select_set(True); bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.fbx(filepath="D:/Yuklanadiganlar/ertugrul_fbx/SK_Ertugrul_Cloth.fbx", use_selection=True,
                         object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False,
                         bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, path_mode='COPY', embed_textures=True,
                         mesh_smooth_type='FACE', use_armature_deform_only=True, apply_scale_options='FBX_SCALE_ALL')
print("FBX", os.path.getsize("D:/Yuklanadiganlar/ertugrul_fbx/SK_Ertugrul_Cloth.fbx"), flush=True)
if RENDER:
    for f in range(1, FRAMES + 1):
        sc.frame_set(f); sc.render.filepath = "%s/k%03d.png" % (OUT, f); bpy.ops.render.render(write_still=True)
        if f % 12 == 0: print("CLOTH", f, flush=True)
print("CLOTH done", flush=True)
