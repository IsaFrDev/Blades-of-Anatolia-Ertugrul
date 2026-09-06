import bpy, math, os
from mathutils import Vector
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/oba_scene.blend")
sc=bpy.context.scene
# 1) Yer materiali: to'q yashil-jigarrang, rangli noise aralashmasini olib tashlash
gm=bpy.data.materials["SteppeGround"]; nt=gm.node_tree; bsdf=nt.nodes["Principled BSDF"]
ramp=next(n for n in nt.nodes if n.type=='VALTORGB')
ramp.color_ramp.elements[0].color=(0.16,0.11,0.05,1); ramp.color_ramp.elements[1].color=(0.07,0.14,0.03,1)
nt.links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
# 2) Daraxtlarni 26 m dan yaqin bo'lsa olib tashlash
rm=0
import random; random.seed(3)
roots=[o for o in bpy.data.objects if o.parent is None]
kill=[]
for o in roots:
    r=math.hypot(o.location.x,o.location.y)
    if o.name.startswith(("Tree1_i","Tree2_i","Fir_i")) and r<26: kill.append(o)
    elif o.name.startswith("Grass_i") and random.random()<0.7: kill.append(o)
    elif o.name.startswith("Shrub_i") and random.random()<0.4: kill.append(o)
names=set()
for o in kill:
    names.add(o.name); names.update(c.name for c in o.children_recursive)
for n in names:
    ob=bpy.data.objects.get(n)
    if ob: bpy.data.objects.remove(ob); rm+=1
# kamera 240 -> 72 kadr
for ob in (bpy.data.objects["FlyCam"], bpy.data.objects["CamTarget"]):
    for fc in ob.animation_data.action.fcurves:
        for kp in fc.keyframe_points: kp.co.x = 1 + (kp.co.x-1)*71/239; kp.handle_left.x = kp.co.x-3; kp.handle_right.x = kp.co.x+3
cd=bpy.data.cameras["FlyCam"]
for fc in cd.animation_data.action.fcurves:
    for kp in fc.keyframe_points: kp.co.x = 1 + (kp.co.x-1)*71/239
sc.frame_end=72
sc.render.resolution_x=960; sc.render.resolution_y=540
# 3) Personaj joyi va kamera
ert=bpy.data.objects["Ertugrul"]; print("ERT", tuple(round(v,2) for v in ert.location), "hide", ert.hide_render)
sun=next(o for o in bpy.data.objects if o.type=='LIGHT' and o.data.type=='SUN'); sun.data.energy=4.0
w=sc.world.node_tree; bg=next(n for n in w.nodes if n.type=='BACKGROUND'); bg.inputs["Strength"].default_value=0.6
sc.view_settings.look='AgX - Medium High Contrast'
bpy.ops.wm.save_mainfile()
print("removed trees", rm)
sc.render.image_settings.file_format='PNG'; sc.eevee.taa_render_samples=6
for f in (1,18,54,72):
    sc.frame_set(f); sc.render.filepath="D:/temp/claude/bl_ert/oba_f%03d.png"%f; bpy.ops.render.render(write_still=True)
print("PREVIEWS ok")
