import bpy, math, random, os, sys
from mathutils import Vector, Euler
random.seed(7)
PH = "D:/Unreal_projects/Ertugrul/art/polyhaven"
GEN = "D:/Unreal_projects/Ertugrul/Content/ErtAssets/Gen/src"
OUT = "D:/temp/claude/bl_ert"
FRAMES = int(os.environ.get("ERT_FRAMES", "240"))
RES = float(os.environ.get("ERT_RES", "1.0"))

bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
sc = bpy.context.scene
ert = bpy.data.objects["Ertugrul"]
front = Vector((0, -1, 0))   # personaj -Y ga qaraydi

# ---------- Yer: tepalikli dasht ----------
old = bpy.data.objects.get("Ground")
if old: bpy.data.objects.remove(old)
bpy.ops.mesh.primitive_grid_add(x_subdivisions=220, y_subdivisions=220, size=160, location=(0, 0, 0))
ground = bpy.context.active_object; ground.name = "Terrain"
import mathutils.noise as mn
for v in ground.data.vertices:
    p = v.co
    d = math.hypot(p.x, p.y)
    hill = 0.0
    if d > 22: hill = (d - 22) * 0.06 + 2.2 * mn.noise(Vector((p.x * 0.05, p.y * 0.05, 0)))
    v.co.z = 0.25 * mn.noise(Vector((p.x * 0.18, p.y * 0.18, 3))) * min(1.0, d / 8.0) + max(0.0, hill) + 1.0 * mn.noise(Vector((p.x * 0.02, p.y * 0.02, 9))) * min(1.0, max(0.0, (d - 12) / 20))
ground.data.update()
bpy.ops.object.shade_smooth()
gm = bpy.data.materials.new("SteppeGround"); gm.use_nodes = True; nt = gm.node_tree; bsdf = nt.nodes["Principled BSDF"]
tc = nt.nodes.new("ShaderNodeTexCoord")
n1 = nt.nodes.new("ShaderNodeTexNoise"); n1.inputs["Scale"].default_value = 0.35; n1.inputs["Detail"].default_value = 9; n1.inputs["Roughness"].default_value = 0.6
n2 = nt.nodes.new("ShaderNodeTexNoise"); n2.inputs["Scale"].default_value = 18; n2.inputs["Detail"].default_value = 6
ramp = nt.nodes.new("ShaderNodeValToRGB"); ramp.color_ramp.elements[0].position = 0.35; ramp.color_ramp.elements[0].color = (0.30, 0.22, 0.11, 1); ramp.color_ramp.elements[1].position = 0.6; ramp.color_ramp.elements[1].color = (0.11, 0.20, 0.05, 1)
mix = nt.nodes.new("ShaderNodeMix"); mix.data_type = 'RGBA'; mix.inputs[0].default_value = 0.25
nt.links.new(tc.outputs["Object"], n1.inputs["Vector"]); nt.links.new(tc.outputs["Object"], n2.inputs["Vector"])
nt.links.new(n1.outputs["Fac"], ramp.inputs["Fac"]); nt.links.new(ramp.outputs["Color"], mix.inputs[6]); nt.links.new(n2.outputs["Color"], mix.inputs[7]); nt.links.new(mix.outputs[2], bsdf.inputs["Base Color"])
bump = nt.nodes.new("ShaderNodeBump"); bump.inputs["Strength"].default_value = 0.3; nt.links.new(n2.outputs["Fac"], bump.inputs["Height"]); nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
bsdf.inputs["Roughness"].default_value = 0.95
ground.data.materials.append(gm)

def ground_z(x, y):
    dg = bpy.context.evaluated_depsgraph_get()
    hit, loc, nrm, idx, obj, mat = sc.ray_cast(dg, Vector((x, y, 60)), Vector((0, 0, -1)))
    return loc.z if hit else 0.0

# ---------- Import yordamchilari ----------
def import_model(path, name, height=None, width=None):
    before = set(bpy.data.objects)
    if path.lower().endswith(".glb"): bpy.ops.import_scene.gltf(filepath=path)
    else: bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == 'MESH']
    root = bpy.data.objects.new(name, None); sc.collection.objects.link(root)
    for o in new:
        if o.parent is None or o.parent not in new: o.parent = root
    # bbox
    bpy.context.view_layer.update()
    pts = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    mnz = min(p.z for p in pts); mxz = max(p.z for p in pts); mxx = max(p.x for p in pts) - min(p.x for p in pts)
    s = 1.0
    if height: s = height / max(mxz - mnz, 1e-3)
    elif width: s = width / max(mxx, 1e-3)
    root.scale = (s, s, s)
    root["base_z"] = mnz * s
    for o in meshes:
        for m in o.data.materials:
            if m and m.use_nodes:
                for n in m.node_tree.nodes:
                    if n.type == 'BSDF_PRINCIPLED': n.inputs["Metallic"].default_value = min(n.inputs["Metallic"].default_value, 0.0)
        o.hide_render = False
    root.hide_render = True
    return root

def place(root, x, y, yaw, scale=1.0):
    # nusxa (linked mesh data)
    def clone(o, parent):
        c = o.copy(); sc.collection.objects.link(c); c.parent = parent
        for ch in o.children: clone(ch, c)
        return c
    r = bpy.data.objects.new(root.name + "_i", None); sc.collection.objects.link(r)
    for ch in root.children: clone(ch, r)
    s = root.scale.x * scale
    r.scale = (s, s, s); r.rotation_euler = (0, 0, yaw)
    r.location = (x, y, ground_z(x, y) - root["base_z"] * scale)
    return r

yurt = import_model(GEN + "/SM_Yurt_Kayi.glb", "Yurt", height=3.6)
tent = import_model(GEN + "/SM_Tent_Mongol.glb", "Tent", height=3.0)
well = import_model(GEN + "/SM_Well_Stone.glb", "Well", height=1.6)
cart = import_model(GEN + "/SM_Cart_Hay.glb", "Cart", height=1.9)
firepit = import_model(PH + "/stone_fire_pit/stone_fire_pit.gltf", "FirePit", width=1.4)
tree1 = import_model(PH + "/island_tree_01/island_tree_01.gltf", "Tree1")
tree2 = import_model(PH + "/island_tree_02/island_tree_02.gltf", "Tree2")
fir = import_model(PH + "/fir_sapling_medium/fir_sapling_medium.gltf", "Fir")
rock = import_model(PH + "/rock_07/rock_07.gltf", "Rock", width=1.6)
boulder = import_model(PH + "/boulder_01/boulder_01.gltf", "Boulder", width=3.0)
shrub = import_model(PH + "/shrub_02/shrub_02.gltf", "Shrub")
grass = import_model(PH + "/grass_medium_01/grass_medium_01.gltf", "Grass")
barrel = import_model(PH + "/wooden_barrels_01/wooden_barrels_01.gltf", "Barrels")
crate = import_model(PH + "/wooden_crate_01/wooden_crate_01.gltf", "Crate")
lantern = import_model(PH + "/wooden_lantern_01/wooden_lantern_01.gltf", "Lantern")
stump = import_model(PH + "/tree_stump_01/tree_stump_01.gltf", "Stump")

# ---------- Oba joylashuvi ----------
# Personaj markazdan 3 m janubda, olovga qaraydi
fx, fy = 0.0, -2.0
place(firepit, fx, fy, 0.3)
ert.location = (0.0, 1.2, ground_z(0.0, 1.2))
# Olov: emissiya konusi + miltillovchi nur
bpy.ops.mesh.primitive_cone_add(vertices=16, radius1=0.35, radius2=0.03, depth=0.9, location=(fx, fy, ground_z(fx, fy) + 0.45))
flame = bpy.context.active_object; flame.name = "Flame"
fm = bpy.data.materials.new("Flame"); fm.use_nodes = True; fn = fm.node_tree
em = fn.nodes.new("ShaderNodeEmission"); em.inputs["Color"].default_value = (1.0, 0.45, 0.08, 1); em.inputs["Strength"].default_value = 25
fn.links.new(em.outputs[0], fn.nodes["Material Output"].inputs["Surface"]); flame.data.materials.append(fm)
bpy.ops.object.light_add(type='POINT', location=(fx, fy, ground_z(fx, fy) + 1.0)); fl = bpy.context.active_object; fl.name = "FireLight"
fl.data.color = (1.0, 0.55, 0.2); fl.data.shadow_soft_size = 0.4
for f in range(1, FRAMES + 1, 3):
    fl.data.energy = random.uniform(500, 900); fl.data.keyframe_insert("energy", frame=f)
    s = random.uniform(0.9, 1.15); flame.scale = (s, s, random.uniform(0.85, 1.2)); flame.keyframe_insert("scale", frame=f)
# O'tovlar halqasi (8 ta), 2 chodir
for i in range(8):
    a = i / 8 * 2 * math.pi + 0.2
    r = 11.0 + random.uniform(-1.0, 1.5)
    x, y = r * math.cos(a), r * math.sin(a)
    place(yurt, x, y, a + math.pi + random.uniform(-0.2, 0.2), random.uniform(0.95, 1.1))
for i in range(2):
    a = (i * 2 + 1) / 4 * 2 * math.pi + 0.6
    place(tent, 17 * math.cos(a), 17 * math.sin(a), a + math.pi)
place(well, -6.5, 5.5, 0.4)
place(cart, 7.5, -5.0, 1.2)
place(barrels, 5.0, 6.0, 0.7) if False else None
place(barrel, 5.0, 6.0, 0.7); place(crate, 5.9, 6.6, 0.2); place(lantern, -3.0, -4.0, 0.0); place(stump, 3.2, -3.5, 1.0)
# Daraxtlar: oba tashqarisida (18-60 m), tepaliklarda
for i in range(70):
    a = random.uniform(0, 2 * math.pi); r = random.uniform(19, 60)
    x, y = r * math.cos(a), r * math.sin(a)
    kind = random.choice([tree1, tree1, tree2, fir])
    place(kind, x, y, random.uniform(0, 6.28), random.uniform(0.8, 1.35))
for i in range(40):
    a = random.uniform(0, 2 * math.pi); r = random.uniform(8, 45)
    place(random.choice([rock, rock, boulder]), r * math.cos(a), r * math.sin(a), random.uniform(0, 6.28), random.uniform(0.5, 1.4))
for i in range(120):
    a = random.uniform(0, 2 * math.pi); r = random.uniform(4, 40)
    place(shrub, r * math.cos(a), r * math.sin(a), random.uniform(0, 6.28), random.uniform(0.6, 1.3))
for i in range(500):
    a = random.uniform(0, 2 * math.pi); r = random.uniform(1.5, 30)
    place(grass, r * math.cos(a), r * math.sin(a), random.uniform(0, 6.28), random.uniform(0.8, 1.4))

# ---------- Yorug'lik: kechki quyosh ----------
sun = next((o for o in bpy.data.objects if o.type == 'LIGHT' and o.data.type == 'SUN'), None)
if sun: sun.data.energy = 3.0; sun.rotation_euler = (math.radians(70), 0, math.radians(120)); sun.data.color = (1.0, 0.85, 0.7)
w = sc.world.node_tree
sky = next((n for n in w.nodes if n.type == 'TEX_SKY'), None)
if sky: sky.sun_elevation = math.radians(14); sky.sun_rotation = math.radians(120); sky.sun_intensity = 0.5; sky.dust_density = 5
sc.eevee.use_volumetric_lights = True if hasattr(sc.eevee, "use_volumetric_lights") else None

# ---------- Kamera animatsiyasi ----------
for o in [o for o in bpy.data.objects if o.type == 'CAMERA']: bpy.data.objects.remove(o)
cd = bpy.data.cameras.new("FlyCam"); cd.lens = 40; cam = bpy.data.objects.new("FlyCam", cd); sc.collection.objects.link(cam); sc.camera = cam
tgt = bpy.data.objects.new("CamTarget", None); sc.collection.objects.link(tgt)
con = cam.constraints.new('TRACK_TO'); con.target = tgt; con.track_axis = 'TRACK_NEGATIVE_Z'; con.up_axis = 'UP_Y'
hz = ert.location.z
keys = [  # (frame, cam xyz, target xyz, lens)
    (1,   (1.4, -1.0, hz + 1.65), (0, 0.9, hz + 1.58), 70),   # yuz
    (60,  (3.5, -4.5, hz + 1.5),  (0, 0.5, hz + 1.1), 50),    # tana + olov
    (120, (-6.0, -9.0, hz + 3.0), (0, 0, hz + 1.0), 40),      # orbit
    (180, (-12.0, 4.0, hz + 6.0), (2, 0, hz + 1.0), 35),      # oba
    (FRAMES, (-30.0, -34.0, hz + 22.0), (0, 0, hz + 2.0), 30), # tepadan butun oba
]
for f, c, t, lens in keys:
    cam.location = c; cam.keyframe_insert("location", frame=f)
    tgt.location = t; tgt.keyframe_insert("location", frame=f)
    cd.lens = lens; cd.keyframe_insert("lens", frame=f)
for fc in list(cam.animation_data.action.fcurves) + list(tgt.animation_data.action.fcurves):
    for kp in fc.keyframe_points: kp.interpolation = 'BEZIER'; kp.easing = 'EASE_IN_OUT'

# ---------- Render ----------
sc.frame_start = 1; sc.frame_end = FRAMES; sc.render.fps = 24
sc.render.engine = 'BLENDER_EEVEE_NEXT'
sc.eevee.taa_render_samples = 16
sc.render.resolution_x = int(1280 * RES); sc.render.resolution_y = int(720 * RES); sc.render.resolution_percentage = 100
sc.render.image_settings.file_format = 'FFMPEG'; sc.render.ffmpeg.format = 'MPEG4'; sc.render.ffmpeg.codec = 'H264'; sc.render.ffmpeg.constant_rate_factor = 'MEDIUM'
sc.render.filepath = OUT + "/oba_flythrough.mp4"
bpy.ops.wm.save_as_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/oba_scene.blend")
print("SCENE objects", len(bpy.data.objects), "frames", FRAMES); sys.stdout.flush()
# Avval bitta kadr (tekshiruv)
sc.frame_set(100); sc.render.image_settings.file_format = 'PNG'; sc.render.filepath = OUT + "/oba_preview.png"; bpy.ops.render.render(write_still=True)
sc.render.image_settings.file_format = 'FFMPEG'; sc.render.filepath = OUT + "/oba_flythrough.mp4"
if os.environ.get("ERT_VIDEO", "1") == "1":
    bpy.ops.render.render(animation=True)
print("VIDEO done")
