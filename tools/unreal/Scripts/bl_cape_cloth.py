# Ertug'rul sahnasi: plash (cloth) + shamol + mo'yna tebranishi; 48 kadr PNG (uzilsa davom etadi)
import bpy, os, math
OUT = "D:/temp/claude/bl_ert/cape"; os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
sc = bpy.context.scene
ert = bpy.data.objects["Ertugrul"]
# --- Plash: yelkadan tushadigan to'rtburchak, yuqori chekkasi tanaga "pin" (vertex guruh)
bpy.ops.mesh.primitive_grid_add(x_subdivisions=24, y_subdivisions=36, size=1.0)
cape = bpy.context.active_object; cape.name = "Cape"
cape.scale = (0.62, 1.05, 1.0); bpy.ops.object.transform_apply(scale=True)
cape.rotation_euler = (math.radians(90), 0, 0)   # vertikal (XZ tekisligi)
cape.location = (0.0, 0.16, 1.05)                 # orqada (personaj -Y ga qaraydi -> orqa +Y), markazi 1.05 m
bpy.ops.object.transform_apply(rotation=True, location=True)
# pin: eng yuqori 2 qator
vg = cape.vertex_groups.new(name="pin")
zs = sorted({round(v.co.z, 4) for v in cape.data.vertices}, reverse=True)
top = set(zs[:2])
vg.add([v.index for v in cape.data.vertices if round(v.co.z, 4) in top], 1.0, 'REPLACE')
# yelka shaklida bukish: yuqori qatorni tana orqasiga yaqin, chetlarini oldinga
for v in cape.data.vertices:
    if round(v.co.z, 4) in top:
        v.co.y -= 0.10 * (1.0 - min(1.0, abs(v.co.x) / 0.31)); v.co.z = 1.56
        v.co.y += 0.0 if abs(v.co.x) < 0.2 else -0.06
# material: to'q qizil mato
m = bpy.data.materials.new("CapeCloth"); m.use_nodes = True; nt = m.node_tree; b = nt.nodes["Principled BSDF"]
b.inputs["Base Color"].default_value = (0.42, 0.06, 0.05, 1); b.inputs["Roughness"].default_value = 0.95; b.inputs["Sheen Weight"].default_value = 0.6
nz = nt.nodes.new("ShaderNodeTexNoise"); nz.inputs["Scale"].default_value = 300; bm = nt.nodes.new("ShaderNodeBump"); bm.inputs["Strength"].default_value = 0.25
nt.links.new(nz.outputs["Fac"], bm.inputs["Height"]); nt.links.new(bm.outputs["Normal"], b.inputs["Normal"])
cape.data.materials.append(m); m.use_backface_culling = False
# cloth
cl = cape.modifiers.new("Cloth", 'CLOTH'); st = cl.settings
st.quality = 8; st.mass = 0.4; st.tension_stiffness = 12; st.compression_stiffness = 12; st.shear_stiffness = 6; st.bending_stiffness = 0.15
st.air_damping = 1.2; st.vertex_group_mass = "pin"
cl.collision_settings.use_self_collision = True; cl.collision_settings.distance_min = 0.012; cl.collision_settings.self_distance_min = 0.008
cl.point_cache.frame_start = 1; cl.point_cache.frame_end = 48
sm = cape.modifiers.new("Sub", 'SUBSURF'); sm.levels = 1; sm.render_levels = 2
# tana bilan to'qnashuv (soddalashtirilgan: personaj meshi collision)
col = ert.modifiers.new("Col", 'COLLISION'); ert.collision.thickness_outer = 0.015
# qalqon/mixlar collision emas (tez ishlashi uchun)
# --- Shamol
bpy.ops.object.effector_add(type='WIND', location=(0, -3, 1.2), rotation=(math.radians(-90), 0, 0))
wind = bpy.context.active_object; wind.field.strength = 900; wind.field.noise = 2.5; wind.field.flow = 0.6
# kuch tebranishi
for f, s in ((1, 600), (14, 1300), (28, 500), (40, 1400), (48, 700)):
    wind.field.strength = s; wind.field.keyframe_insert("strength", frame=f)
# --- Kamera: orqadan 3/4, plash ko'rinadi
cam = bpy.data.objects.get("Cam_Front34")
from mathutils import Vector
cam.location = (2.4, 2.6, 1.5); d = Vector((0, 0, 1.1)) - Vector(cam.location); cam.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler(); cam.data.lens = 50
sc.camera = cam
sc.frame_start = 1; sc.frame_end = 48; sc.render.fps = 24
sc.render.engine = 'BLENDER_EEVEE_NEXT'; sc.eevee.taa_render_samples = 6
sc.render.resolution_x = 960; sc.render.resolution_y = 540; sc.render.image_settings.file_format = 'PNG'
# cloth bake (kadrma-kadr sim: frame_set ketma-ket)
for f in range(1, 49):
    sc.frame_set(f)
    p = "%s/c%03d.png" % (OUT, f)
    if os.path.exists(p) and os.path.getsize(p) > 1000: continue
    sc.render.filepath = p; bpy.ops.render.render(write_still=True); print("CAPE", f, flush=True)
bpy.ops.wm.save_as_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_cape.blend")
print("CAPE done", flush=True)
