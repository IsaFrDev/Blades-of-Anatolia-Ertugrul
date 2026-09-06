# Blender asset fabrikasi: (1) yer PBR teksturalari (bake, 2K) -> art/blender_tex, (2) oba elementlari -> Content/ErtAssets/Gen/src/*.glb
import bpy, math, os, random
random.seed(5)
TEX = "D:/Unreal_projects/Ertugrul/art/blender_tex"
GLB = "D:/Unreal_projects/Ertugrul/Content/ErtAssets/Gen/src"
os.makedirs(TEX, exist_ok=True); os.makedirs(GLB, exist_ok=True)
SIZE = int(os.environ.get("ERT_TEXSIZE", "2048"))
ONLY = os.environ.get("ERT_ONLY")

def clear():
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
    for b in (bpy.data.meshes, bpy.data.materials, bpy.data.images):
        for x in list(b):
            if x.users == 0: b.remove(x)

sc = bpy.context.scene
sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 8; sc.cycles.use_denoising = False
sc.render.bake.margin = 16; sc.render.bake.use_pass_direct = False; sc.render.bake.use_pass_indirect = False; sc.render.bake.use_pass_color = True

# ---------- Protsedural materiallar ----------
def new_mat(name):
    m = bpy.data.materials.new(name); m.use_nodes = True; nt = m.node_tree
    for n in list(nt.nodes): nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial"); bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); nt.links.new(bsdf.outputs[0], out.inputs[0])
    tc = nt.nodes.new("ShaderNodeTexCoord")
    return m, nt, bsdf, tc

def ramp(nt, stops):
    r = nt.nodes.new("ShaderNodeValToRGB")
    el = r.color_ramp.elements
    while len(el) < len(stops): el.new(0.5)
    for i, (p, c) in enumerate(stops): el[i].position = p; el[i].color = (*c, 1)
    return r

def noise(nt, tc, scale, detail=6, rough=0.5, dist=0.0):
    n = nt.nodes.new("ShaderNodeTexNoise"); n.inputs["Scale"].default_value = scale; n.inputs["Detail"].default_value = detail; n.inputs["Roughness"].default_value = rough; n.inputs["Distortion"].default_value = dist
    nt.links.new(tc.outputs["UV"], n.inputs["Vector"]); return n

def voronoi(nt, tc, scale, feature='F1', rnd=1.0):
    v = nt.nodes.new("ShaderNodeTexVoronoi"); v.feature = feature; v.inputs["Scale"].default_value = scale; v.inputs["Randomness"].default_value = rnd
    nt.links.new(tc.outputs["UV"], v.inputs["Vector"]); return v

def bump(nt, bsdf, height_socket, strength, distance=0.02):
    b = nt.nodes.new("ShaderNodeBump"); b.inputs["Strength"].default_value = strength; b.inputs["Distance"].default_value = distance
    nt.links.new(height_socket, b.inputs["Height"]); nt.links.new(b.outputs["Normal"], bsdf.inputs["Normal"]); return b

def mat_grass():
    m, nt, bsdf, tc = new_mat("grass")
    n1 = noise(nt, tc, 14, 8, 0.6); n2 = noise(nt, tc, 90, 10, 0.7); n3 = noise(nt, tc, 4, 4, 0.5)
    r = ramp(nt, [(0.30, (0.06, 0.12, 0.03)), (0.5, (0.12, 0.22, 0.05)), (0.7, (0.24, 0.32, 0.08)), (0.85, (0.40, 0.42, 0.14))])
    mix = nt.nodes.new("ShaderNodeMix"); mix.data_type = 'FLOAT'; mix.inputs[0].default_value = 0.45
    nt.links.new(n1.outputs["Fac"], mix.inputs[2]); nt.links.new(n2.outputs["Fac"], mix.inputs[3]); nt.links.new(mix.outputs[0], r.inputs["Fac"])
    # katta dog'lar (sarg'ish quruq joylar)
    r2 = ramp(nt, [(0.45, (0.0, 0.0, 0.0)), (0.6, (1.0, 1.0, 1.0))]); nt.links.new(n3.outputs["Fac"], r2.inputs["Fac"])
    mc = nt.nodes.new("ShaderNodeMix"); mc.data_type = 'RGBA'; mc.inputs[7].default_value = (0.36, 0.33, 0.12, 1)
    nt.links.new(r2.outputs["Color"], mc.inputs[0]); nt.links.new(r.outputs["Color"], mc.inputs[6]); nt.links.new(mc.outputs[2], bsdf.inputs["Base Color"])
    # Barg tolalari: cho'zilgan mayda shovqin -> to'q/ochiq chiziqlar
    mp = nt.nodes.new("ShaderNodeMapping"); mp.inputs["Scale"].default_value = (60, 6, 1); mp.inputs["Rotation"].default_value = (0, 0, 0.6); nt.links.new(tc.outputs["UV"], mp.inputs["Vector"])
    bl = nt.nodes.new("ShaderNodeTexNoise"); bl.inputs["Scale"].default_value = 3.0; bl.inputs["Detail"].default_value = 6; bl.inputs["Roughness"].default_value = 0.7; nt.links.new(mp.outputs["Vector"], bl.inputs["Vector"])
    mp2 = nt.nodes.new("ShaderNodeMapping"); mp2.inputs["Scale"].default_value = (7, 55, 1); mp2.inputs["Rotation"].default_value = (0, 0, -0.9); nt.links.new(tc.outputs["UV"], mp2.inputs["Vector"])
    bl2 = nt.nodes.new("ShaderNodeTexNoise"); bl2.inputs["Scale"].default_value = 3.0; bl2.inputs["Detail"].default_value = 6; nt.links.new(mp2.outputs["Vector"], bl2.inputs["Vector"])
    bm = nt.nodes.new("ShaderNodeMath"); bm.operation = 'MULTIPLY'; nt.links.new(bl.outputs["Fac"], bm.inputs[0]); nt.links.new(bl2.outputs["Fac"], bm.inputs[1])
    br = ramp(nt, [(0.15, (0.55, 0.6, 0.45)), (0.35, (1.0, 1.0, 1.0)), (0.6, (1.25, 1.3, 1.1))]); nt.links.new(bm.outputs[0], br.inputs["Fac"])
    mb = nt.nodes.new("ShaderNodeMix"); mb.data_type = 'RGBA'; mb.blend_type = 'MULTIPLY'; mb.inputs[0].default_value = 1.0
    nt.links.new(mc.outputs[2], mb.inputs[6]); nt.links.new(br.outputs["Color"], mb.inputs[7]); nt.links.new(mb.outputs[2], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.9
    ba = nt.nodes.new("ShaderNodeMath"); ba.operation = 'ADD'; nt.links.new(n2.outputs["Fac"], ba.inputs[0]); nt.links.new(bm.outputs[0], ba.inputs[1])
    bump(nt, bsdf, ba.outputs[0], 0.6, 0.012)
    return m

def mat_dirt():
    m, nt, bsdf, tc = new_mat("dirt")
    n1 = noise(nt, tc, 10, 8, 0.6); v = voronoi(nt, tc, 60, 'DISTANCE_TO_EDGE'); n2 = noise(nt, tc, 120, 8, 0.8)
    r = ramp(nt, [(0.3, (0.16, 0.10, 0.05)), (0.55, (0.30, 0.20, 0.10)), (0.8, (0.42, 0.31, 0.17))]); nt.links.new(n1.outputs["Fac"], r.inputs["Fac"])
    # toshchalar: voronoi F1 kichik tepaliklar
    vf = voronoi(nt, tc, 40, 'F1'); rp = ramp(nt, [(0.15, (1, 1, 1)), (0.35, (0, 0, 0))]); nt.links.new(vf.outputs["Distance"], rp.inputs["Fac"])
    mc = nt.nodes.new("ShaderNodeMix"); mc.data_type = 'RGBA'; mc.inputs[7].default_value = (0.36, 0.33, 0.28, 1)
    mul = nt.nodes.new("ShaderNodeMath"); mul.operation = 'MULTIPLY'; mul.inputs[1].default_value = 0.35
    nt.links.new(rp.outputs["Color"], mul.inputs[0]); nt.links.new(mul.outputs[0], mc.inputs[0]); nt.links.new(r.outputs["Color"], mc.inputs[6]); nt.links.new(mc.outputs[2], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.95
    add = nt.nodes.new("ShaderNodeMath"); add.operation = 'ADD'; nt.links.new(n2.outputs["Fac"], add.inputs[0]); nt.links.new(rp.outputs["Color"], add.inputs[1])
    bump(nt, bsdf, add.outputs[0], 0.6, 0.02)
    return m

def mat_rock():
    m, nt, bsdf, tc = new_mat("rock")
    v = voronoi(nt, tc, 7, 'DISTANCE_TO_EDGE'); n1 = noise(nt, tc, 20, 9, 0.7, 0.4); n2 = noise(nt, tc, 3, 5, 0.5)
    cr = ramp(nt, [(0.0, (0, 0, 0)), (0.06, (1, 1, 1))]); nt.links.new(v.outputs["Distance"], cr.inputs["Fac"])   # yoriqlar
    col = ramp(nt, [(0.3, (0.22, 0.20, 0.18)), (0.6, (0.38, 0.35, 0.31)), (0.85, (0.52, 0.48, 0.42))]); nt.links.new(n1.outputs["Fac"], col.inputs["Fac"])
    tint = ramp(nt, [(0.3, (0.9, 0.85, 0.8)), (0.7, (1.0, 1.0, 1.0))]); nt.links.new(n2.outputs["Fac"], tint.inputs["Fac"])
    mm = nt.nodes.new("ShaderNodeMix"); mm.data_type = 'RGBA'; mm.blend_type = 'MULTIPLY'; mm.inputs[0].default_value = 1.0
    nt.links.new(col.outputs["Color"], mm.inputs[6]); nt.links.new(tint.outputs["Color"], mm.inputs[7])
    mc = nt.nodes.new("ShaderNodeMix"); mc.data_type = 'RGBA'; mc.blend_type = 'MULTIPLY'; mc.inputs[0].default_value = 0.8
    nt.links.new(mm.outputs[2], mc.inputs[6]); nt.links.new(cr.outputs["Color"], mc.inputs[7]); nt.links.new(mc.outputs[2], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.85
    add = nt.nodes.new("ShaderNodeMath"); add.operation = 'ADD'; sc1 = nt.nodes.new("ShaderNodeMath"); sc1.operation = 'MULTIPLY'; sc1.inputs[1].default_value = 0.4
    nt.links.new(n1.outputs["Fac"], sc1.inputs[0]); nt.links.new(cr.outputs["Color"], add.inputs[0]); nt.links.new(sc1.outputs[0], add.inputs[1])
    bump(nt, bsdf, add.outputs[0], 0.9, 0.05)
    return m

def mat_sand():
    m, nt, bsdf, tc = new_mat("sand")
    w = nt.nodes.new("ShaderNodeTexWave"); w.wave_type = 'BANDS'; w.inputs["Scale"].default_value = 12; w.inputs["Distortion"].default_value = 6; w.inputs["Detail"].default_value = 4; nt.links.new(tc.outputs["UV"], w.inputs["Vector"])
    n = noise(nt, tc, 200, 6, 0.6)
    col = ramp(nt, [(0.2, (0.62, 0.50, 0.32)), (0.8, (0.80, 0.68, 0.46))]); nt.links.new(n.outputs["Fac"], col.inputs["Fac"]); nt.links.new(col.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.95
    add = nt.nodes.new("ShaderNodeMath"); add.operation = 'ADD'; s = nt.nodes.new("ShaderNodeMath"); s.operation = 'MULTIPLY'; s.inputs[1].default_value = 0.3
    nt.links.new(w.outputs["Fac"], add.inputs[0]); nt.links.new(n.outputs["Fac"], s.inputs[0]); nt.links.new(s.outputs[0], add.inputs[1])
    bump(nt, bsdf, add.outputs[0], 0.4, 0.02)
    return m

def mat_snow():
    m, nt, bsdf, tc = new_mat("snow")
    n = noise(nt, tc, 30, 6, 0.5); n2 = noise(nt, tc, 5, 3)
    col = ramp(nt, [(0.3, (0.80, 0.85, 0.92)), (0.8, (0.96, 0.97, 1.0))]); nt.links.new(n2.outputs["Fac"], col.inputs["Fac"]); nt.links.new(col.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.6
    bump(nt, bsdf, n.outputs["Fac"], 0.3, 0.02)
    return m

def mat_cobble():
    m, nt, bsdf, tc = new_mat("cobble")
    v = voronoi(nt, tc, 9, 'DISTANCE_TO_EDGE', 0.8); vc = voronoi(nt, tc, 9, 'F1', 0.8); n = noise(nt, tc, 60, 8, 0.7)
    mortar = ramp(nt, [(0.0, (0, 0, 0)), (0.05, (0, 0, 0)), (0.10, (1, 1, 1))]); nt.links.new(v.outputs["Distance"], mortar.inputs["Fac"])
    stone = ramp(nt, [(0.0, (0.30, 0.28, 0.26)), (0.5, (0.42, 0.40, 0.36)), (1.0, (0.55, 0.50, 0.44))]); nt.links.new(vc.outputs["Color"], stone.inputs["Fac"])
    mc = nt.nodes.new("ShaderNodeMix"); mc.data_type = 'RGBA'; mc.inputs[6].default_value = (0.14, 0.12, 0.10, 1)
    nt.links.new(mortar.outputs["Color"], mc.inputs[0]); nt.links.new(stone.outputs["Color"], mc.inputs[7]); nt.links.new(mc.outputs[2], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.8
    add = nt.nodes.new("ShaderNodeMath"); add.operation = 'ADD'; s = nt.nodes.new("ShaderNodeMath"); s.operation = 'MULTIPLY'; s.inputs[1].default_value = 0.15
    nt.links.new(mortar.outputs["Color"], add.inputs[0]); nt.links.new(n.outputs["Fac"], s.inputs[0]); nt.links.new(s.outputs[0], add.inputs[1])
    bump(nt, bsdf, add.outputs[0], 1.0, 0.06)
    return m

def mat_wood():
    m, nt, bsdf, tc = new_mat("wood")
    mp = nt.nodes.new("ShaderNodeMapping"); mp.inputs["Scale"].default_value = (1, 14, 1); nt.links.new(tc.outputs["UV"], mp.inputs["Vector"])
    w = nt.nodes.new("ShaderNodeTexWave"); w.wave_type = 'BANDS'; w.bands_direction = 'X'; w.inputs["Scale"].default_value = 4; w.inputs["Distortion"].default_value = 3; w.inputs["Detail"].default_value = 8; nt.links.new(mp.outputs["Vector"], w.inputs["Vector"])
    col = ramp(nt, [(0.2, (0.18, 0.10, 0.04)), (0.6, (0.36, 0.22, 0.10)), (0.9, (0.48, 0.32, 0.16))]); nt.links.new(w.outputs["Fac"], col.inputs["Fac"]); nt.links.new(col.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.75
    bump(nt, bsdf, w.outputs["Fac"], 0.35, 0.01)
    return m

def mat_hay():
    m, nt, bsdf, tc = new_mat("hay")
    mp = nt.nodes.new("ShaderNodeMapping"); mp.inputs["Scale"].default_value = (1, 40, 1); nt.links.new(tc.outputs["UV"], mp.inputs["Vector"])
    n = nt.nodes.new("ShaderNodeTexNoise"); n.inputs["Scale"].default_value = 8; n.inputs["Detail"].default_value = 10; n.inputs["Distortion"].default_value = 1.0; nt.links.new(mp.outputs["Vector"], n.inputs["Vector"])
    col = ramp(nt, [(0.3, (0.45, 0.32, 0.10)), (0.6, (0.75, 0.60, 0.25)), (0.85, (0.90, 0.80, 0.45))]); nt.links.new(n.outputs["Fac"], col.inputs["Fac"]); nt.links.new(col.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.9
    bump(nt, bsdf, n.outputs["Fac"], 0.7, 0.02)
    return m

def mat_cloth(rgb):
    m, nt, bsdf, tc = new_mat("cloth")
    n = noise(nt, tc, 300, 4, 0.5); col = ramp(nt, [(0.3, tuple(c * 0.75 for c in rgb)), (0.8, rgb)]); nt.links.new(n.outputs["Fac"], col.inputs["Fac"]); nt.links.new(col.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.9; bump(nt, bsdf, n.outputs["Fac"], 0.2, 0.005); return m

# ---------- Bake ----------
def bake_material(m, name, size=SIZE):
    """Tekis kvadrat plitkaga bake: D (rang) va N (tangent normal) -> PNG. Baked image texture nodes qaytaradi."""
    clear_scene_keep = None
    bpy.ops.mesh.primitive_plane_add(size=2); pl = bpy.context.active_object; pl.name = "BakePlane"
    pl.data.materials.append(m); nt = m.node_tree
    out = {}
    for suffix, btype in (("D", 'DIFFUSE'), ("N", 'NORMAL')):
        img = bpy.data.images.new("T_%s_%s" % (name, suffix), size, size, alpha=False, float_buffer=False)
        img.colorspace_settings.name = 'sRGB' if suffix == "D" else 'Non-Color'
        node = nt.nodes.new("ShaderNodeTexImage"); node.image = img; nt.nodes.active = node
        bpy.ops.object.select_all(action='DESELECT'); pl.select_set(True); bpy.context.view_layer.objects.active = pl
        bpy.ops.object.bake(type=btype, margin=16, use_clear=True)
        path = "%s/T_%s_%s.png" % (TEX, name, suffix); img.filepath_raw = path; img.file_format = 'PNG'; img.save()
        nt.nodes.remove(node); out[suffix] = path
        print("BAKED", path, flush=True)
    bpy.data.objects.remove(pl)
    return out

def baked_mat(name, paths, rough=0.8):
    """GLB eksport uchun: faqat rasm teksturali oddiy material."""
    m = bpy.data.materials.new("M_" + name); m.use_nodes = True; nt = m.node_tree; bsdf = nt.nodes["Principled BSDF"]
    d = nt.nodes.new("ShaderNodeTexImage"); d.image = bpy.data.images.load(paths["D"]); d.image.colorspace_settings.name = 'sRGB'; nt.links.new(d.outputs["Color"], bsdf.inputs["Base Color"])
    n = nt.nodes.new("ShaderNodeTexImage"); n.image = bpy.data.images.load(paths["N"]); n.image.colorspace_settings.name = 'Non-Color'
    nm = nt.nodes.new("ShaderNodeNormalMap"); nt.links.new(n.outputs["Color"], nm.inputs["Color"]); nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
    bsdf.inputs["Roughness"].default_value = rough; bsdf.inputs["Metallic"].default_value = 0.0
    return m

clear()
paths = {}
for name, fn in (("grass", mat_grass), ("dirt", mat_dirt), ("rock", mat_rock), ("sand", mat_sand), ("snow", mat_snow), ("cobble", mat_cobble), ("wood", mat_wood), ("hay", mat_hay)):
    if ONLY and name != ONLY: continue
    paths[name] = bake_material(fn(), name)
if ONLY:
    print("FACTORY done (only %s)" % ONLY, flush=True); import sys; sys.exit(0)
paths["cloth"] = bake_material(mat_cloth((0.55, 0.08, 0.06)), "cloth", 1024)

# ---------- Oba elementlari ----------
M_WOOD = baked_mat("wood", paths["wood"], 0.75); M_HAY = baked_mat("hay", paths["hay"], 0.9); M_ROCK = baked_mat("rock", paths["rock"], 0.85); M_CLOTH = baked_mat("cloth", paths["cloth"], 0.9)

def uv_all(objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02); bpy.ops.object.mode_set(mode='OBJECT')

def cyl(r, h, loc, rot=(0, 0, 0), v=16, mat=None, name="c"):
    bpy.ops.mesh.primitive_cylinder_add(vertices=v, radius=r, depth=h, location=loc, rotation=rot); o = bpy.context.active_object; o.name = name
    if mat: o.data.materials.append(mat)
    return o

def box(sz, loc, rot=(0, 0, 0), mat=None, name="b"):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=rot); o = bpy.context.active_object; o.scale = sz; o.name = name
    bpy.ops.object.transform_apply(scale=True)
    if mat: o.data.materials.append(mat)
    return o

def finish(name, objs):
    uv_all(objs)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join(); o = bpy.context.active_object; o.name = name; o.data.name = name
    bpy.ops.object.shade_smooth_by_angle(angle=math.radians(35)) if hasattr(bpy.ops.object, "shade_smooth_by_angle") else bpy.ops.object.shade_smooth()
    path = "%s/%s.glb" % (GLB, name)
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format='GLB', export_apply=True, export_yup=True)
    print("GLB", path, os.path.getsize(path), flush=True)
    bpy.data.objects.remove(o)

# 1) Ot bog'lash ustuni (tether post): 2 ustun + ko'ndalang yog'och
o = [cyl(0.07, 1.5, (0, -0.9, 0.75), mat=M_WOOD, name="p1"), cyl(0.07, 1.5, (0, 0.9, 0.75), mat=M_WOOD, name="p2"), cyl(0.05, 2.0, (0, 0, 1.3), rot=(math.radians(90), 0, 0), mat=M_WOOD, name="bar")]
finish("SM_TetherPost_Wood", o)
# 2) Oxur (trough)
o = [box((1.6, 0.5, 0.08), (0, 0, 0.30), mat=M_WOOD, name="base"), box((1.6, 0.05, 0.4), (0, 0.25, 0.5), mat=M_WOOD), box((1.6, 0.05, 0.4), (0, -0.25, 0.5), mat=M_WOOD), box((0.05, 0.5, 0.4), (0.8, 0, 0.5), mat=M_WOOD), box((0.05, 0.5, 0.4), (-0.8, 0, 0.5), mat=M_WOOD),
     box((0.08, 0.08, 0.3), (0.6, 0.18, 0.15), mat=M_WOOD), box((0.08, 0.08, 0.3), (-0.6, -0.18, 0.15), mat=M_WOOD), box((0.08, 0.08, 0.3), (0.6, -0.18, 0.15), mat=M_WOOD), box((0.08, 0.08, 0.3), (-0.6, 0.18, 0.15), mat=M_WOOD)]
finish("SM_Trough_Wood", o)
# 3) Pichan uyumi (hay bale): silindr + shovqin displacement
b = cyl(0.45, 1.0, (0, 0, 0.45), rot=(0, math.radians(90), 0), v=24, mat=M_HAY, name="bale")
bpy.ops.object.modifier_add(type='SUBSURF'); b.modifiers[-1].levels = 2
tex = bpy.data.textures.new("hn", 'CLOUDS'); tex.noise_scale = 0.15
d = b.modifiers.new("disp", 'DISPLACE'); d.texture = tex; d.strength = 0.08
bpy.ops.object.modifier_apply(modifier=b.modifiers[0].name); bpy.ops.object.modifier_apply(modifier="disp")
finish("SM_HayBale_01", [b])
# 4) O'tin uyumi (woodpile)
o = []
for row in range(3):
    for i in range(5 - row):
        o.append(cyl(0.09, 0.9, (-0.36 + i * 0.18 + row * 0.09, 0, 0.09 + row * 0.16), rot=(math.radians(90), 0, 0), v=10, mat=M_WOOD, name="log%d_%d" % (row, i)))
finish("SM_Woodpile_01", o)
# 5) Qayi bayrog'i: ustun + mato
o = [cyl(0.03, 3.2, (0, 0, 1.6), mat=M_WOOD, name="pole"), box((0.02, 0.6, 0.9), (0, 0.32, 2.6), mat=M_CLOTH, name="flag"), cyl(0.05, 0.12, (0, 0, 3.25), v=8, mat=M_WOOD, name="tip")]
finish("SM_Banner_Kayi", o)
# 6) Qurol tokchasi (weapon rack): rom + 3 nayza
o = [box((1.2, 0.06, 0.06), (0, 0, 1.2), mat=M_WOOD), box((1.2, 0.06, 0.06), (0, 0, 0.3), mat=M_WOOD), box((0.06, 0.06, 1.3), (0.6, 0, 0.65), mat=M_WOOD), box((0.06, 0.06, 1.3), (-0.6, 0, 0.65), mat=M_WOOD)]
for i in range(3): o.append(cyl(0.015, 2.2, (-0.3 + i * 0.3, 0.05, 1.1), rot=(math.radians(8), 0, 0), v=8, mat=M_WOOD, name="spear%d" % i))
finish("SM_WeaponRack_Wood", o)
# 7) Tosh gulxan halqasi (fire_pit): 12 tosh
o = []
for i in range(12):
    a = i / 12 * 2 * math.pi
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.16, location=(0.55 * math.cos(a), 0.55 * math.sin(a), 0.10)); s = bpy.context.active_object
    s.scale = (random.uniform(0.8, 1.2), random.uniform(0.8, 1.2), random.uniform(0.5, 0.7)); s.rotation_euler = (random.uniform(0, 1), random.uniform(0, 1), random.uniform(0, 3)); bpy.ops.object.transform_apply(scale=True, rotation=True)
    s.data.materials.append(M_ROCK); o.append(s)
o.append(cyl(0.45, 0.06, (0, 0, 0.03), v=16, mat=M_ROCK, name="ash"))
finish("SM_FirePit_Stone", o)
print("FACTORY done", flush=True)
