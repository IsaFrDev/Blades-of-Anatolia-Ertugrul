import bpy, math
from mathutils import Vector
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/oba_scene.blend")
sc=bpy.context.scene
terrain=bpy.data.objects["Terrain"]
# 1) Shablon (template) ildizlarini o'chirish: nomi "_i" bilan tugamaydigan bo'sh (EMPTY) ildizlar
tmpl=["Yurt","Tent","Well","Cart","FirePit","Tree1","Tree2","Fir","Rock","Boulder","Shrub","Grass","Barrels","Crate","Lantern","Stump"]
names=set()
for o in bpy.data.objects:
    if o.type=='EMPTY' and o.parent is None and o.name in tmpl:
        names.add(o.name); names.update(c.name for c in o.children_recursive)
for n in names:
    ob=bpy.data.objects.get(n)
    if ob: bpy.data.objects.remove(ob)
print("templates removed", len(names))
# 2) Faqat Terrain ga nisbatan yerga qo'yish
inv=terrain.matrix_world.inverted()
def tz(x,y):
    hit,loc,nrm,idx=terrain.ray_cast(inv @ Vector((x,y,80)), (inv.to_3x3() @ Vector((0,0,-1))).normalized())
    return (terrain.matrix_world @ loc).z if hit else 0.0
fixed=0
for o in bpy.data.objects:
    if o.parent is None and o.type=='EMPTY' and o.name.endswith("_i") or (o.parent is None and o.type=='EMPTY' and "_i." in o.name):
        z=tz(o.location.x,o.location.y)
        # base_z: bola meshlarning eng past nuqtasi
        bpy.context.view_layer.update()
        pts=[o.matrix_world @ Vector(c) for ch in o.children_recursive if ch.type=='MESH' for c in ch.bound_box]
        if not pts: continue
        mnz=min(p.z for p in pts)
        d=z-mnz
        if abs(d)>0.02: o.location.z += d; fixed+=1
print("regrounded", fixed)
# 3) Yer rangi: yumshoq yashil-jigarrang
gm=bpy.data.materials["SteppeGround"]; nt=gm.node_tree
ramp=next(n for n in nt.nodes if n.type=='VALTORGB')
ramp.color_ramp.elements[0].position=0.3; ramp.color_ramp.elements[0].color=(0.13,0.11,0.05,1)
ramp.color_ramp.elements[1].position=0.7; ramp.color_ramp.elements[1].color=(0.08,0.14,0.04,1)
for n in nt.nodes:
    if n.type=='TEX_NOISE' and n.inputs["Scale"].default_value<1: n.inputs["Scale"].default_value=0.12; n.inputs["Detail"].default_value=12; n.inputs["Roughness"].default_value=0.75
bpy.ops.wm.save_mainfile()
sc.render.image_settings.file_format='PNG'; sc.eevee.taa_render_samples=6
for f in (1,54):
    sc.frame_set(f); sc.render.filepath="D:/temp/claude/bl_ert/oba_f%03d.png"%f; bpy.ops.render.render(write_still=True)
print("PREVIEWS ok")
