import bpy, math, random
random.seed(11)
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/oba_scene.blend")
sc=bpy.context.scene
# Toshlar o'tov halqasida (9.5-13.5 m) bo'lsa olib tashlash
names=set()
for o in bpy.data.objects:
    if o.parent is None and o.type=='EMPTY' and o.name.startswith(("Rock_i","Boulder_i")):
        r=math.hypot(o.location.x,o.location.y)
        if 9.0<r<14.0 or r<4.5: names.add(o.name); names.update(c.name for c in o.children_recursive)
for n in names:
    ob=bpy.data.objects.get(n)
    if ob: bpy.data.objects.remove(ob)
# Olov: kichik, to'q sariq, shovqinli shaffoflik
fl=bpy.data.objects["Flame"]; fl.scale=(0.55,0.55,0.6)
fl.animation_data_clear()
m=bpy.data.materials["Flame"]; nt=m.node_tree
for n in list(nt.nodes):
    if n.type!='OUTPUT_MATERIAL': nt.nodes.remove(n)
out=next(n for n in nt.nodes if n.type=='OUTPUT_MATERIAL')
em=nt.nodes.new("ShaderNodeEmission"); em.inputs["Strength"].default_value=12
ramp=nt.nodes.new("ShaderNodeValToRGB"); ramp.color_ramp.elements[0].color=(1.0,0.15,0.0,1); ramp.color_ramp.elements[1].color=(1.0,0.75,0.2,1)
nz=nt.nodes.new("ShaderNodeTexNoise"); nz.inputs["Scale"].default_value=6; nz.inputs["Detail"].default_value=4
tc=nt.nodes.new("ShaderNodeTexCoord")
tr=nt.nodes.new("ShaderNodeBsdfTransparent"); mix=nt.nodes.new("ShaderNodeMixShader")
nt.links.new(tc.outputs["Generated"], nz.inputs["Vector"]); nt.links.new(nz.outputs["Fac"], ramp.inputs["Fac"]); nt.links.new(ramp.outputs["Color"], em.inputs["Color"])
nt.links.new(nz.outputs["Fac"], mix.inputs["Fac"]); nt.links.new(tr.outputs[0], mix.inputs[1]); nt.links.new(em.outputs[0], mix.inputs[2]); nt.links.new(mix.outputs[0], out.inputs["Surface"])
m.blend_method='BLEND'
for f in range(1,73,2):
    s=random.uniform(0.5,0.65); fl.scale=(s,s,random.uniform(0.5,0.75)); fl.keyframe_insert("scale",frame=f)
    fl.rotation_euler=(0,0,random.uniform(0,6.28)); fl.keyframe_insert("rotation_euler",frame=f)
bpy.ops.wm.save_mainfile()
sc.frame_start=1; sc.frame_end=72; sc.eevee.taa_render_samples=6
sc.render.resolution_x=960; sc.render.resolution_y=540
sc.render.image_settings.file_format='FFMPEG'; sc.render.ffmpeg.format='MPEG4'; sc.render.ffmpeg.codec='H264'; sc.render.ffmpeg.constant_rate_factor='MEDIUM'
sc.render.filepath="D:/temp/claude/bl_ert/oba_flythrough.mp4"
bpy.ops.render.render(animation=True)
print("VIDEO done")
