import bpy, math
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/oba_scene.blend")
sc=bpy.context.scene
kill=[]
for o in bpy.data.objects:
    if o.parent is None:
        r=math.hypot(o.location.x,o.location.y)
        if (o.name.startswith("Shrub_i") and r<7.5) or (o.name.startswith("Grass_i") and r<3.0) or (o.name.startswith(("Rock_i","Boulder_i")) and r<9): kill.append(o)
names=set()
for o in kill: names.add(o.name); names.update(c.name for c in o.children_recursive)
for n in names:
    ob=bpy.data.objects.get(n)
    if ob: bpy.data.objects.remove(ob)
print("removed", len(names))
bpy.ops.wm.save_mainfile()
sc.render.image_settings.file_format='PNG'; sc.eevee.taa_render_samples=6
for f in (1,54):
    sc.frame_set(f); sc.render.filepath="D:/temp/claude/bl_ert/oba_f%03d.png"%f; bpy.ops.render.render(write_still=True)
print("PREVIEWS ok")
