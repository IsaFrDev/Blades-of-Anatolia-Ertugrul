# Ertug'rul kiyimi (kaftan etagi) mato simulyatsiyasi: yugurish + shamol. Chiqish: bl_ert/kaftan/k###.png
import bpy, os, math
from mathutils import Vector
OUT = "D:/temp/claude/bl_ert/kaftan"; os.makedirs(OUT, exist_ok=True)
FRAMES = int(os.environ.get("ERT_FRAMES", "48"))
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
sc = bpy.context.scene
ert = bpy.data.objects["Ertugrul"]
bpy.context.view_layer.objects.active = ert
# Oldingi plash (quti) va qalqonni olib tashlash - faqat kiyim ko'rinsin
for n in ["Cape"] + ["Rivet%d" % i for i in range(12)]:
    o = bpy.data.objects.get(n)
    if o: bpy.data.objects.remove(o)
H = ert.dimensions.z
print("HEIGHT", round(H, 3), flush=True)
# --- Etak vertex guruhi: bel (0.56H) va etak (0.33H) orasidagi tashqi qobiq
for g in list(ert.vertex_groups):
    if g.name in ("pin", "skirt"): ert.vertex_groups.remove(g)
pin = ert.vertex_groups.new(name="pin")
zlo, zhi = 0.33 * H, 0.57 * H
band = [v for v in ert.data.vertices if zlo < v.co.z < zhi]
rmax = max((math.hypot(v.co.x, v.co.y) for v in band), default=0.25)
skirt = set(v.index for v in band if math.hypot(v.co.x, v.co.y) > 0.62 * rmax)
free = skirt
pinned = [v.index for v in ert.data.vertices if v.index not in free]
pin.add(pinned, 1.0, 'REPLACE')
print("SKIRT verts", len(free), "of", len(ert.data.vertices), "rmax", round(rmax, 3), flush=True)
# --- Cloth
for m in [m for m in ert.modifiers if m.type in ('CLOTH', 'COLLISION')]:
    ert.modifiers.remove(m)
cl = ert.modifiers.new("Cloth", 'CLOTH'); st = cl.settings
st.quality = 5
st.mass = 0.35                 # charm kaftan: og'irroq
st.tension_stiffness = 20; st.compression_stiffness = 20; st.shear_stiffness = 10
st.bending_stiffness = 1.2     # charm - qattiqroq buklanadi
st.air_damping = 1.1
st.tension_damping = 8; st.bending_damping = 1.0
st.vertex_group_mass = "pin"
cl.collision_settings.use_self_collision = True
cl.collision_settings.self_distance_min = 0.006
cl.collision_settings.distance_min = 0.008
cl.point_cache.frame_start = 1; cl.point_cache.frame_end = FRAMES
# --- Yugurish: tanani oldinga surish + tebranish (mato inersiyadan hilpiraydi)
ert.animation_data_clear()
for f in range(1, FRAMES + 1):
    t = (f - 1) / 24.0
    ert.location = (t * 3.2, 0.0, 0.03 * abs(math.sin(t * math.pi * 3.2)))
    ert.rotation_euler = (0.0, math.radians(2.5 * math.sin(t * math.pi * 3.2)), 0.0)
    ert.keyframe_insert("location", frame=f); ert.keyframe_insert("rotation_euler", frame=f)
for fc in ert.animation_data.action.fcurves:
    for kp in fc.keyframe_points: kp.interpolation = 'LINEAR'
# --- Shamol (yon-orqadan, tebranuvchi)
for o in [o for o in bpy.data.objects if o.type == 'EMPTY' and o.field and o.field.type == 'WIND']:
    bpy.data.objects.remove(o)
bpy.ops.object.effector_add(type='WIND', location=(-3, 1.5, 1.0), rotation=(math.radians(-80), 0, math.radians(-25)))
wind = bpy.context.active_object; wind.field.strength = 700; wind.field.noise = 3.0; wind.field.flow = 0.5
for f, s in ((1, 450), (12, 1100), (24, 500), (36, 1200), (FRAMES, 600)):
    wind.field.strength = s; wind.field.keyframe_insert("strength", frame=f)
# --- Kamera: personajga ergashadi (orqa 3/4)
cam = bpy.data.objects.get("Cam_Front34") or bpy.data.objects.get("FlyCam")
cam.animation_data_clear()
for f in range(1, FRAMES + 1):
    t = (f - 1) / 24.0
    p = Vector((t * 3.2 - 2.6, 2.9, 1.45))
    cam.location = p
    d = Vector((t * 3.2, 0.0, 1.0)) - p
    cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    cam.keyframe_insert("location", frame=f); cam.keyframe_insert("rotation_euler", frame=f)
cam.data.lens = 55
sc.camera = cam
sc.frame_start = 1; sc.frame_end = FRAMES; sc.render.fps = 24
sc.render.engine = 'BLENDER_EEVEE_NEXT'; sc.eevee.taa_render_samples = 6
sc.render.resolution_x = 960; sc.render.resolution_y = 540
sc.render.image_settings.file_format = 'PNG'
bpy.ops.wm.save_as_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_kaftan.blend")
for f in range(1, FRAMES + 1):
    p = "%s/k%03d.png" % (OUT, f)
    if os.path.exists(p) and os.path.getsize(p) > 1000: continue
    sc.frame_set(f); sc.render.filepath = p; bpy.ops.render.render(write_still=True); print("KAFTAN", f, flush=True)
print("KAFTAN done", flush=True)
