# Kaftan etagi uchun suyakli tebranish rigi (barqaror, UE ga ko'chadi):
# 4 sektor x 2 bo'g'in suyak, radial/vertikal og'irlik, yugurishda orqaga oqish + shamol tebranishi.
# Chiqish: bl_ert/kaftan/k###.png + D:/Yuklanadiganlar/ertugrul_fbx/SK_Ertugrul_Kaftan.fbx
import bpy, os, math
from mathutils import Vector
OUT = "D:/temp/claude/bl_ert/kaftan"; os.makedirs(OUT, exist_ok=True)
FRAMES = int(os.environ.get("ERT_FRAMES", "48"))
RENDER = os.environ.get("ERT_RENDER", "1") == "1"
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
sc = bpy.context.scene
ert = bpy.data.objects["Ertugrul"]
# Faqat kiyim ko'rinsin: qalqon, mixlar, plash olib tashlanadi
for n in ["Cape", "Shield", "ShieldRim", "ShieldBoss"] + ["Rivet%d" % i for i in range(12)]:
    o = bpy.data.objects.get(n)
    if o: bpy.data.objects.remove(o)
for m in [m for m in ert.modifiers if m.type in ('CLOTH', 'COLLISION', 'ARMATURE')]:
    ert.modifiers.remove(m)
ert.animation_data_clear(); ert.location = (0, 0, 0); ert.rotation_euler = (0, 0, 0)
bpy.context.view_layer.update()
H = ert.dimensions.z
ZW, ZH = 0.575 * H, 0.325 * H          # bel (yuqori) va etak (past)
band = [v for v in ert.data.vertices if ZH < v.co.z < ZW]
rmax = max((math.hypot(v.co.x, v.co.y) for v in band), default=0.3)
RMIN = 0.60 * rmax
print("HEIGHT", round(H, 3), "band", len(band), "rmax", round(rmax, 3), flush=True)

# ---------- Armatura: 4 sektor x 2 bo'g'in ----------
DIRS = [("f", 0.0), ("r", math.pi * 0.5), ("b", math.pi), ("l", math.pi * 1.5)]
bpy.ops.object.armature_add(enter_editmode=True, location=(0, 0, 0))
arm = bpy.context.active_object; arm.name = "KaftanRig"; arm.data.name = "KaftanRig"
eb = arm.data.edit_bones
for b in list(eb): eb.remove(b)
root = eb.new("root"); root.head = (0, 0, ZW); root.tail = (0, 0, ZW + 0.12)
MID = (ZW + ZH) * 0.5
for name, ang in DIRS:
    dx, dy = math.cos(ang), math.sin(ang)
    r0 = RMIN * 0.85
    b1 = eb.new("skirt_%s_1" % name); b1.head = (dx * r0, dy * r0, ZW); b1.tail = (dx * r0 * 1.05, dy * r0 * 1.05, MID); b1.parent = root
    b2 = eb.new("skirt_%s_2" % name); b2.head = b1.tail; b2.tail = (dx * r0 * 1.1, dy * r0 * 1.1, ZH - 0.02); b2.parent = b1; b2.use_connect = True
bpy.ops.object.mode_set(mode='OBJECT')

# ---------- Og'irliklar: sektor bo'yicha, bel 0 -> etak 1 ----------
for g in list(ert.vertex_groups): ert.vertex_groups.remove(g)
groups = {}
for name, _ in DIRS:
    groups[name + "1"] = ert.vertex_groups.new(name="skirt_%s_1" % name)
    groups[name + "2"] = ert.vertex_groups.new(name="skirt_%s_2" % name)
def smooth(x): x = min(1.0, max(0.0, x)); return x * x * (3 - 2 * x)
n_assigned = 0
for v in ert.data.vertices:
    z, x, y = v.co.z, v.co.x, v.co.y
    r = math.hypot(x, y)
    if not (ZH < z < ZW) or r < RMIN: continue
    t = smooth((ZW - z) / (ZW - ZH))                 # 0 belda, 1 etakda
    if t <= 0.001: continue
    a = math.atan2(y, x) % (2 * math.pi)
    # ikkita eng yaqin sektor orasida yumshoq aralashma
    for name, ang in DIRS:
        d = abs((a - ang + math.pi) % (2 * math.pi) - math.pi)
        w = max(0.0, 1.0 - d / (math.pi * 0.5))      # 90° ichida kamayadi
        if w <= 0.001: continue
        w1 = t * w * (1.0 - smooth((ZW - z) / (ZW - ZH) - 0.15))   # yuqori bo'g'in: bel yaqinida
        w2 = t * w * smooth(((ZW - z) / (ZW - ZH) - 0.35) / 0.65)  # pastki bo'g'in: etak yaqinida
        if w1 > 0.001: groups[name + "1"].add([v.index], min(1.0, w1), 'REPLACE')
        if w2 > 0.001: groups[name + "2"].add([v.index], min(1.0, w2), 'REPLACE')
    n_assigned += 1
print("WEIGHTED", n_assigned, flush=True)
ert.parent = arm
mod = ert.modifiers.new("Armature", 'ARMATURE'); mod.object = arm

# ---------- Animatsiya: yugurish (-Y yo'nalishi = personaj yuzi), etak orqaga oqadi ----------
arm.animation_data_clear()
for pb in arm.pose.bones: pb.rotation_mode = 'QUATERNION'
SPEED = 3.2
MOVE = Vector((0.0, -1.0, 0.0))                 # personaj -Y ga qaraydi
AXIS_W = Vector((0.0, 0.0, 1.0)).cross(MOVE).normalized()   # tebranish o'qi (dunyo)
SIDE_W = MOVE.copy()
rest = {b.name: b.matrix_local.to_3x3() for b in arm.data.bones}
def local_axis(bname, world_axis):
    return (rest[bname].inverted() @ world_axis).normalized()
from mathutils import Quaternion
for f in range(1, FRAMES + 1):
    t = (f - 1) / 24.0
    step = math.sin(t * math.pi * 3.2)
    arm.location = (0.0, -t * SPEED, 0.03 * abs(step))
    arm.rotation_euler = (math.radians(2.0 * step), 0.0, 0.0)
    arm.keyframe_insert("location", frame=f); arm.keyframe_insert("rotation_euler", frame=f)
    for name, ang in DIRS:
        d = Vector((math.cos(ang), math.sin(ang), 0.0))
        facing = d.dot(-MOVE)                    # +1: harakatga qarama-qarshi (orqa panel)
        def sway(tt):
            lf = math.radians(12.0 + 16.0 * facing) + math.radians(17.0) * math.sin(tt * math.pi * 3.2 + ang)
            sd = math.radians(11.0) * math.sin(tt * math.pi * 2.1 + ang * 1.3)
            return lf, sd
        lift, side = sway(t)
        lift2, side2 = sway(t - 0.14)            # pastki bo'g'in kechikib ergashadi
        for bn, lf, sd in (("skirt_%s_1" % name, lift, side), ("skirt_%s_2" % name, lift2 * 1.1, side2 * 1.2)):
            pb = arm.pose.bones[bn]
            q = Quaternion(local_axis(bn, AXIS_W), lf) @ Quaternion(local_axis(bn, SIDE_W), sd)
            pb.rotation_quaternion = q
            pb.keyframe_insert("rotation_quaternion", frame=f)
for fc in arm.animation_data.action.fcurves:
    for kp in fc.keyframe_points: kp.interpolation = 'BEZIER'

# ---------- Kamera ----------
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
bpy.ops.wm.save_as_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_kaftan.blend")
# FBX (UE uchun: etak suyaklari + animatsiya)
bpy.ops.object.select_all(action='DESELECT'); ert.select_set(True); arm.select_set(True); bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.fbx(filepath="D:/Yuklanadiganlar/ertugrul_fbx/SK_Ertugrul_Kaftan.fbx", use_selection=True,
                         object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=False,
                         bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, path_mode='COPY', embed_textures=True,
                         mesh_smooth_type='FACE', use_armature_deform_only=True, apply_scale_options='FBX_SCALE_ALL')
print("FBX", os.path.getsize("D:/Yuklanadiganlar/ertugrul_fbx/SK_Ertugrul_Kaftan.fbx"), flush=True)
if RENDER:
    for f in range(1, FRAMES + 1):
        p = "%s/k%03d.png" % (OUT, f)
        sc.frame_set(f); sc.render.filepath = p; bpy.ops.render.render(write_still=True)
        if f % 8 == 0: print("KAFTAN", f, flush=True)
print("KAFTAN done", flush=True)
