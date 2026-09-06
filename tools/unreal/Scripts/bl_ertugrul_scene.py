import bpy, math, os
from mathutils import Vector

# 1) Tozalash
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
for b in (bpy.data.meshes, bpy.data.materials, bpy.data.images):
    for x in list(b):
        if x.users == 0: b.remove(x)

# 2) Ertug'rul (150k, 1.8 m) ni blend faylidan olish
blend = "D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_150k.blend"
with bpy.data.libraries.load(blend, link=False) as (src, dst):
    dst.objects = [n for n in src.objects if n == "Ertugrul"]
ert = dst.objects[0]; bpy.context.scene.collection.objects.link(ert)
bpy.context.view_layer.objects.active = ert; ert.select_set(True)
bpy.context.view_layer.update()
# Balandlik tekshiruvi (1.8 m) va oyoq yerda
h = ert.dimensions.z
if abs(h - 1.8) > 0.05:
    s = 1.8 / h; ert.scale = (s, s, s); bpy.ops.object.transform_apply(scale=True)
mn = min((ert.matrix_world @ v.co).z for v in ert.data.vertices); ert.location.z -= mn; bpy.ops.object.transform_apply(location=True)
bpy.ops.object.shade_smooth()

# Oldini aniqlash: bosh balandligida (1.55-1.75) burun eng uzoq chiqqan tomon
head = [v.co for v in ert.data.vertices if 1.55 < v.co.z < 1.75]
ymin = min(v.y for v in head); ymax = max(v.y for v in head)
cy = sum(v.y for v in head) / len(head)
front_sign = -1.0 if (cy - ymin) > (ymax - cy) else 1.0   # burun uzoqroq chiqqan tomon = old
print("FRONT", front_sign, "ymin", round(ymin, 3), "ymax", round(ymax, 3), "cy", round(cy, 3))

# 3) Material: realistik (metall 0, g'adir-budirlik teksturadan, normal kuchli, teri uchun subsurface)
mat = ert.data.materials[0]; mat.use_nodes = True
nt = mat.node_tree; bsdf = next(n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED')
bsdf.inputs["Metallic"].default_value = 0.0
if bsdf.inputs["Roughness"].links: pass
else: bsdf.inputs["Roughness"].default_value = 0.75
bsdf.inputs["Specular IOR Level"].default_value = 0.35
for n in nt.nodes:
    if n.type == 'NORMAL_MAP': n.inputs["Strength"].default_value = 1.4
# Rangni biroz to'yintirish
img = next((n for n in nt.nodes if n.type == 'TEX_IMAGE' and n.outputs["Color"].links and n.outputs["Color"].links[0].to_socket.name == "Base Color"), None)
if img:
    hs = nt.nodes.new("ShaderNodeHueSaturation"); hs.inputs["Saturation"].default_value = 1.25; hs.inputs["Value"].default_value = 0.95
    nt.links.new(img.outputs["Color"], hs.inputs["Color"]); nt.links.new(hs.outputs["Color"], bsdf.inputs["Base Color"])

# 4) Soqol va soch: koordinata bo'yicha vertex guruhlar + hair particle
def vgroup(name, cond):
    g = ert.vertex_groups.new(name=name); idx = [v.index for v in ert.data.vertices if cond(v.co)]
    if idx: g.add(idx, 1.0, 'REPLACE')
    return g, len(idx)
# yuz markazi (y) bo'yicha: oldi = front_sign tomoni
def fwd(co): return co.y * front_sign
g_beard, nb = vgroup("beard", lambda c: 1.52 < c.z < 1.66 and fwd(c) > -(cy * front_sign) + 0.02 and abs(c.x) < 0.09)
g_hair, nh = vgroup("hair", lambda c: 1.62 < c.z < 1.74 and fwd(c) < -(cy * front_sign) - 0.01)
print("BEARD verts", nb, "HAIR verts", nh)
hair_mat = bpy.data.materials.new("HairDark"); hair_mat.use_nodes = True
hb = hair_mat.node_tree.nodes["Principled BSDF"]; hb.inputs["Base Color"].default_value = (0.05, 0.03, 0.02, 1); hb.inputs["Roughness"].default_value = 0.6
ert.data.materials.append(hair_mat)
def hair(name, group, length, count, child):
    ert.modifiers.new(name, 'PARTICLE_SYSTEM'); ps = ert.particle_systems[-1]; ps.name = name
    st = ps.settings; st.type = 'HAIR'; st.count = count; st.hair_length = length; st.child_type = 'INTERPOLATED'
    st.child_percent = child; st.rendered_child_count = child; st.root_radius = 0.02; st.tip_radius = 0.0
    st.use_advanced_hair = True; st.brownian_factor = 0.02; st.factor_random = 0.3
    st.material_slot = hair_mat.name; ps.vertex_group_density = group
    st.clump_factor = 0.4; st.roughness_1 = 0.02; st.roughness_endpoint = 0.01
hair("Beard", "beard", 0.035, 1500, 12)
hair("Hair", "hair", 0.12, 1200, 14)

# 5) Orqadagi qalqon (yog'och taxtalar + temir halqa + bo'ri bosh o'rniga temir bo'rtiq)
back = -front_sign
sy = cy + back * 0.20
bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=0.30, depth=0.035, location=(0.02, sy, 1.20), rotation=(math.radians(90), 0, 0))
shield = bpy.context.active_object; shield.name = "Shield"
bpy.ops.object.shade_smooth()
wood = bpy.data.materials.new("ShieldWood"); wood.use_nodes = True; wn = wood.node_tree; wb = wn.nodes["Principled BSDF"]
tc = wn.nodes.new("ShaderNodeTexCoord"); mp = wn.nodes.new("ShaderNodeMapping"); mp.inputs["Scale"].default_value = (1, 12, 1)
wave = wn.nodes.new("ShaderNodeTexWave"); wave.wave_type = 'BANDS'; wave.inputs["Scale"].default_value = 3.0; wave.inputs["Distortion"].default_value = 4.0; wave.inputs["Detail"].default_value = 6.0
ramp = wn.nodes.new("ShaderNodeValToRGB"); ramp.color_ramp.elements[0].color = (0.16, 0.08, 0.03, 1); ramp.color_ramp.elements[1].color = (0.38, 0.22, 0.10, 1)
wn.links.new(tc.outputs["Object"], mp.inputs["Vector"]); wn.links.new(mp.outputs["Vector"], wave.inputs["Vector"]); wn.links.new(wave.outputs["Fac"], ramp.inputs["Fac"]); wn.links.new(ramp.outputs["Color"], wb.inputs["Base Color"])
bump = wn.nodes.new("ShaderNodeBump"); bump.inputs["Strength"].default_value = 0.35; wn.links.new(wave.outputs["Fac"], bump.inputs["Height"]); wn.links.new(bump.outputs["Normal"], wb.inputs["Normal"])
wb.inputs["Roughness"].default_value = 0.7
shield.data.materials.append(wood)
iron = bpy.data.materials.new("Iron"); iron.use_nodes = True; ib = iron.node_tree.nodes["Principled BSDF"]; ib.inputs["Base Color"].default_value = (0.35, 0.34, 0.33, 1); ib.inputs["Metallic"].default_value = 1.0; ib.inputs["Roughness"].default_value = 0.45
bpy.ops.mesh.primitive_torus_add(major_radius=0.30, minor_radius=0.014, major_segments=64, minor_segments=12, location=(0.02, sy, 1.20), rotation=(math.radians(90), 0, 0))
rim = bpy.context.active_object; rim.name = "ShieldRim"; rim.data.materials.append(iron); bpy.ops.object.shade_smooth()
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.055, location=(0.02, sy + back * 0.02, 1.20))
boss = bpy.context.active_object; boss.name = "ShieldBoss"; boss.scale = (1, 0.5, 1); boss.data.materials.append(iron); bpy.ops.object.shade_smooth()
# Mixlar (rivets)
for k in range(12):
    a = k / 12 * 2 * math.pi
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.008, segments=12, ring_count=8, location=(0.02 + 0.27 * math.cos(a), sy + back * 0.018, 1.20 + 0.27 * math.sin(a)))
    r = bpy.context.active_object; r.name = "Rivet%d" % k; r.data.materials.append(iron)
# Qalqonni personajga bog'lash
for o in [shield, rim, boss] + [bpy.data.objects["Rivet%d" % k] for k in range(12)]:
    o.parent = ert

# 6) Sahna: yer, osmon, quyosh
bpy.ops.mesh.primitive_plane_add(size=40, location=(0, 0, 0)); ground = bpy.context.active_object; ground.name = "Ground"
gm = bpy.data.materials.new("Steppe"); gm.use_nodes = True; gn = gm.node_tree; gb = gn.nodes["Principled BSDF"]
nz = gn.nodes.new("ShaderNodeTexNoise"); nz.inputs["Scale"].default_value = 6.0; nz.inputs["Detail"].default_value = 8.0
gr = gn.nodes.new("ShaderNodeValToRGB"); gr.color_ramp.elements[0].color = (0.10, 0.16, 0.05, 1); gr.color_ramp.elements[1].color = (0.32, 0.26, 0.14, 1)
gn.links.new(nz.outputs["Fac"], gr.inputs["Fac"]); gn.links.new(gr.outputs["Color"], gb.inputs["Base Color"]); gb.inputs["Roughness"].default_value = 0.95
ground.data.materials.append(gm)
world = bpy.context.scene.world or bpy.data.worlds.new("World"); bpy.context.scene.world = world; world.use_nodes = True
wnt = world.node_tree
for n in list(wnt.nodes):
    if n.type == 'TEX_SKY': wnt.nodes.remove(n)
sky = wnt.nodes.new("ShaderNodeTexSky"); sky.sky_type = 'NISHITA'; sky.sun_elevation = math.radians(28); sky.sun_rotation = math.radians(140); sky.sun_intensity = 0.4; sky.altitude = 900; sky.dust_density = 3
bg = next(n for n in wnt.nodes if n.type == 'BACKGROUND'); wnt.links.new(sky.outputs["Color"], bg.inputs["Color"]); bg.inputs["Strength"].default_value = 0.35
bpy.ops.object.light_add(type='SUN', location=(4, -4, 8)); sun = bpy.context.active_object; sun.data.energy = 4.0; sun.data.angle = math.radians(2)
sun.rotation_euler = (math.radians(50), 0, math.radians(35 if front_sign < 0 else 215))
bpy.ops.object.light_add(type='AREA', location=(-3 * 1, front_sign * 3, 2.2)); fill = bpy.context.active_object; fill.data.energy = 250; fill.data.size = 3; fill.name = "Fill"
fill.rotation_euler = (math.radians(70), 0, math.radians(-45 if front_sign < 0 else 135))

# 7) Kameralar (old 3/4, orqa, yuz) va render
sc = bpy.context.scene
sc.render.engine = 'BLENDER_EEVEE_NEXT'
sc.render.resolution_x = 1280; sc.render.resolution_y = 720; sc.render.film_transparent = False
sc.view_settings.view_transform = 'AgX'; sc.view_settings.look = 'AgX - Medium High Contrast'
def cam(name, loc, target, lens=50):
    cd = bpy.data.cameras.new(name); cd.lens = lens; c = bpy.data.objects.new(name, cd); sc.collection.objects.link(c)
    c.location = loc; d = Vector(target) - Vector(loc); c.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler(); return c
f = front_sign
cams = [cam("Cam_Front34", (2.6, f * 3.4, 1.4), (0, 0, 1.0), 55), cam("Cam_Back", (-1.2, -f * 3.6, 1.5), (0, 0, 1.05), 55), cam("Cam_Face", (0.45, f * 1.1, 1.62), (0, 0, 1.58), 85)]
out = "D:/temp/claude/bl_ert"
os.makedirs(out, exist_ok=True)
for c in cams:
    sc.camera = c; sc.render.filepath = "%s/%s.png" % (out, c.name); bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
print("DONE renders", [c.name for c in cams])
