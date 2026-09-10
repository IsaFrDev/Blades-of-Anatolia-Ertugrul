# Ot: GLB/FBX mesh -> avtomatik skelet (bo'yin, bosh, dum, 4 oyoq x 3 bo'g'in) + avto og'irlik + protsedural yurish/yo'rtish/chopish/tinch/sakrash animatsiyalari -> FBX (UE skelet mesh)
# Kirish: ERT_HORSE=path.glb (bo'lmasa placeholder ot yasaladi). Chiqish: D:/Yuklanadiganlar/horse_rig/SK_Horse.fbx
import bpy, os, math, sys
from mathutils import Vector
SRC = os.environ.get("ERT_HORSE", "")
OUT = "D:/Yuklanadiganlar/horse_rig"; os.makedirs(OUT, exist_ok=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()

def placeholder_horse():
    """Sinov uchun oddiy ot: tana + bo'yin + bosh + 4 oyoq (birlashtirilgan)."""
    parts = []
    def add(op, **kw):
        op(**kw); o = bpy.context.active_object; parts.append(o); return o
    b = add(bpy.ops.mesh.primitive_uv_sphere_add, radius=0.5, location=(0, 0, 1.15)); b.scale = (0.55, 1.0, 0.5)
    n = add(bpy.ops.mesh.primitive_cylinder_add, radius=0.18, depth=0.9, location=(0, -0.95, 1.55), rotation=(math.radians(-55), 0, 0))
    h = add(bpy.ops.mesh.primitive_cube_add, size=0.3, location=(0, -1.35, 1.95)); h.scale = (0.7, 1.6, 0.9)
    t = add(bpy.ops.mesh.primitive_cylinder_add, radius=0.05, depth=0.8, location=(0, 1.05, 1.0), rotation=(math.radians(-60), 0, 0))
    for x in (-0.22, 0.22):
        for y in (-0.62, 0.62):
            add(bpy.ops.mesh.primitive_cylinder_add, radius=0.075, depth=0.55, location=(x, y, 0.83))
            add(bpy.ops.mesh.primitive_cylinder_add, radius=0.06, depth=0.55, location=(x, y, 0.28))
    for o in parts: o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]; bpy.ops.object.transform_apply(scale=True, rotation=True); bpy.ops.object.join()
    o = bpy.context.active_object; o.name = "Horse"
    bpy.ops.object.modifier_add(type='REMESH'); o.modifiers[-1].voxel_size = 0.03; bpy.ops.object.modifier_apply(modifier=o.modifiers[-1].name)
    m = bpy.data.materials.new("HorseBay"); m.use_nodes = True; m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.30, 0.16, 0.08, 1); o.data.materials.append(m)
    return o

if SRC:
    if SRC.lower().endswith((".glb", ".gltf")): bpy.ops.import_scene.gltf(filepath=SRC)
    else: bpy.ops.import_scene.fbx(filepath=SRC)
    meshes = [o for o in bpy.data.objects if o.type == 'MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes: o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1: bpy.ops.object.join()
    horse = bpy.context.active_object; horse.name = "Horse"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    # o'lcham: yelka balandligi ~1.55 m; ot uzun o'qi Y bo'ylab, boshi -Y da deb faraz (bo'lmasa ERT_HORSE_YAW bilan burang)
    yaw = float(os.environ.get("ERT_HORSE_YAW", "0"))
    if yaw: horse.rotation_euler.z = math.radians(yaw); bpy.ops.object.transform_apply(rotation=True)
    d = horse.dimensions; s = 1.6 / d.z; horse.scale = (s, s, s); bpy.ops.object.transform_apply(scale=True)
    mn = min((horse.matrix_world @ v.co).z for v in horse.data.vertices); horse.location.z -= mn; bpy.ops.object.transform_apply(location=True)
    # markazlash (x, y)
    cx = sum(v.co.x for v in horse.data.vertices) / len(horse.data.vertices); cy = sum(v.co.y for v in horse.data.vertices) / len(horse.data.vertices)
    horse.location = (-cx, -cy, 0); bpy.ops.object.transform_apply(location=True)
else:
    horse = placeholder_horse()
bpy.context.view_layer.update()
D = horse.dimensions; L = D.y; H = D.z; Wd = D.x
print("HORSE dims", tuple(round(v, 2) for v in D), flush=True)
# Bo'yin/bosh qaysi tomonda: -Y tomondagi eng baland vertexlar (bosh yuqorida)
vs = [v.co for v in horse.data.vertices]
front_neg = sum(1 for v in vs if v.y < -L * 0.3 and v.z > H * 0.7); front_pos = sum(1 for v in vs if v.y > L * 0.3 and v.z > H * 0.7)
F = -1.0 if front_neg >= front_pos else 1.0   # old tomon belgisi
print("HORSE front sign", F, front_neg, front_pos, flush=True)

# ---------- Skelet ----------
bpy.ops.object.armature_add(enter_editmode=True, location=(0, 0, 0))
arm = bpy.context.active_object; arm.name = "HorseRig"; arm.data.name = "HorseRig"
eb = arm.data.edit_bones
for b in list(eb): eb.remove(b)
def bone(name, head, tail, parent=None, connect=False):
    b = eb.new(name); b.head = Vector(head); b.tail = Vector(tail)
    if parent: b.parent = eb[parent]; b.use_connect = connect
    return b
bodyZ = H * 0.68; hipY = -F * (-L * 0.28); shoY = -F * (L * 0.28)   # hip orqada, shoulder oldinda
hipY = F * (-L * 0.30) * -1; shoY = F * (L * 0.30) * -1   # F=-1 (bosh -Y): shoY=-0.3L (old), hipY=+0.3L
shoY = F * L * 0.30; hipY = -F * L * 0.30
bone("root", (0, 0, 0), (0, 0, 0.3))
bone("pelvis", (0, hipY, bodyZ), (0, hipY * 0.4, bodyZ + 0.02), "root")
bone("spine", (0, hipY * 0.4, bodyZ + 0.02), (0, shoY * 0.6, bodyZ + 0.05), "pelvis", True)
bone("chest", (0, shoY * 0.6, bodyZ + 0.05), (0, shoY, bodyZ + 0.04), "spine", True)
neckBase = (0, shoY * 1.05, bodyZ + 0.10); neckMid = (0, shoY * 1.05 + F * L * 0.12, H * 0.86); headB = (0, shoY * 1.05 + F * L * 0.22, H * 0.98); headT = (0, shoY * 1.05 + F * L * 0.36, H * 0.90)
bone("neck1", neckBase, neckMid, "chest"); bone("neck2", neckMid, headB, "neck1", True); bone("head", headB, headT, "neck2", True)
bone("tail1", (0, hipY * 1.05, bodyZ + 0.05), (0, hipY * 1.05 - F * L * 0.12, bodyZ - 0.15), "pelvis"); bone("tail2", (0, hipY * 1.05 - F * L * 0.12, bodyZ - 0.15), (0, hipY * 1.05 - F * L * 0.2, bodyZ - 0.45), "tail1", True)
legX = Wd * 0.28
for side, sx in (("l", -1), ("r", 1)):
    for part, py, par in (("f", shoY, "chest"), ("b", hipY, "pelvis")):
        x = sx * legX
        bone("%s_%s_upper" % (part, side), (x, py, bodyZ - 0.05), (x, py, H * 0.36), par)
        bone("%s_%s_lower" % (part, side), (x, py, H * 0.36), (x, py, H * 0.10), "%s_%s_upper" % (part, side), True)
        bone("%s_%s_foot" % (part, side), (x, py, H * 0.10), (x, py + F * 0.08, 0.0), "%s_%s_lower" % (part, side), True)
bpy.ops.object.mode_set(mode='OBJECT')
# Avto og'irlik
bpy.ops.object.select_all(action='DESELECT'); horse.select_set(True); arm.select_set(True); bpy.context.view_layer.objects.active = arm
bpy.ops.object.parent_set(type='ARMATURE_AUTO')
print("RIG bones", len(arm.data.bones), flush=True)

# ---------- Animatsiyalar (protsedural kalitlar) ----------
def clip(name, frames, fn):
    act = bpy.data.actions.new(name); arm.animation_data_create(); arm.animation_data.action = act
    for pb in arm.pose.bones: pb.rotation_mode = 'XYZ'; pb.rotation_euler = (0, 0, 0); pb.location = (0, 0, 0)
    for f in range(1, frames + 1):
        t = (f - 1) / frames
        fn(t)
        for pb in arm.pose.bones: pb.keyframe_insert("rotation_euler", frame=f); pb.keyframe_insert("location", frame=f)
    act.use_fake_user = True; act.frame_range = (1, frames)
    return act
P = arm.pose.bones
def rot(name, x=0.0, y=0.0, z=0.0): P[name].rotation_euler = (math.radians(x), math.radians(y), math.radians(z))
def leg(part, side, ph, amp, bend):
    # oyoq: yuqori bo'g'in oldinga/orqaga (X o'qi atrofida), pastki bo'g'in bukiladi, tuyoq tekislaydi
    sw = math.sin(ph) * amp; bd = max(0.0, math.sin(ph + math.pi * 0.5)) * bend
    rot("%s_%s_upper" % (part, side), x=F * sw); rot("%s_%s_lower" % (part, side), x=F * (-bd if part == "f" else bd * 0.8)); rot("%s_%s_foot" % (part, side), x=F * (bd * 0.4 if part == "f" else -bd * 0.3))
def gait(t, amp, bend, bob, diag):
    ph = t * 2 * math.pi
    # diag=True: yo'rtish (FL+BR, FR+BL); False: chopish (old juft / orqa juft siljigan)
    phs = {"f_l": 0, "b_r": 0, "f_r": math.pi, "b_l": math.pi} if diag else {"f_l": 0, "f_r": 0.35, "b_l": math.pi * 0.6, "b_r": math.pi * 0.6 + 0.35}
    for k, p0 in phs.items(): leg(k[0], k[2], ph + p0, amp, bend)
    P["root"].location = (0, 0, math.sin(ph * 2) * bob)
    rot("spine", x=math.sin(ph * 2) * (2 if diag else 6)); rot("neck1", x=math.sin(ph * 2) * (3 if diag else 8) - 5); rot("head", x=math.sin(ph * 2) * 2 + 4)
    rot("tail1", x=-20 + math.sin(ph) * 6, z=math.sin(ph * 0.5) * 8); rot("tail2", z=math.sin(ph + 1) * 10)
clip("idle", 48, lambda t: (rot("chest", x=math.sin(t * 2 * math.pi) * 0.8), rot("neck1", x=-6 + math.sin(t * 2 * math.pi) * 2), rot("head", x=6 + math.sin(t * 4 * math.pi) * 3, z=math.sin(t * 2 * math.pi) * 4), rot("tail1", z=math.sin(t * 2 * math.pi) * 12), rot("tail2", z=math.sin(t * 2 * math.pi + 1) * 15)))
clip("walk", 32, lambda t: gait(t, 22, 30, 0.012, True))
clip("trot", 20, lambda t: gait(t, 30, 45, 0.03, True))
clip("gallop", 14, lambda t: gait(t, 42, 60, 0.06, False))
def jump(t):
    a = math.sin(t * math.pi)
    for s in ("l", "r"): rot("f_%s_upper" % s, x=F * -40 * a); rot("f_%s_lower" % s, x=F * -50 * a); rot("b_%s_upper" % s, x=F * 30 * a); rot("b_%s_lower" % s, x=F * 25 * a)
    rot("neck1", x=-12 * a); P["root"].location = (0, 0, 0.15 * a)
clip("jump", 24, jump)
def death(t):
    a = min(1.0, t * 1.4)
    rot("root", y=80 * a); P["root"].location = (0, 0, -0.55 * a)
    for s in ("l", "r"):
        for p in ("f", "b"): rot("%s_%s_upper" % (p, s), x=F * 30 * a); rot("%s_%s_lower" % (p, s), x=F * (-40 if p == "f" else 35) * a)
    rot("neck1", x=25 * a); rot("head", x=20 * a)
clip("death", 30, death)
arm.animation_data.action = bpy.data.actions["idle"]
# ---------- Eksport ----------
bpy.ops.object.select_all(action='DESELECT'); horse.select_set(True); arm.select_set(True); bpy.context.view_layer.objects.active = arm
out = OUT + "/SK_Horse.fbx"
bpy.ops.export_scene.fbx(filepath=out, use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=True, bake_anim_use_nla_strips=False, bake_anim_use_all_bones=True, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0, path_mode='COPY', embed_textures=True, mesh_smooth_type='FACE', armature_nodetype='NULL', use_armature_deform_only=True, apply_scale_options='FBX_SCALE_ALL')
bpy.ops.wm.save_as_mainfile(filepath=OUT + "/horse_rig.blend")
print("EXPORT", out, os.path.getsize(out), "actions", [a.name for a in bpy.data.actions], flush=True)
