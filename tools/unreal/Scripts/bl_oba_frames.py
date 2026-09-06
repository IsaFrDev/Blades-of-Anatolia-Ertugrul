import bpy, os
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/oba_scene.blend")
sc=bpy.context.scene
sc.eevee.taa_render_samples=6; sc.render.resolution_x=960; sc.render.resolution_y=540
sc.render.image_settings.file_format='PNG'
out="D:/temp/claude/bl_ert/frames"; os.makedirs(out, exist_ok=True)
for f in range(1,73):
    p="%s/f%03d.png"%(out,f)
    if os.path.exists(p) and os.path.getsize(p)>1000: continue
    sc.frame_set(f); sc.render.filepath=p; bpy.ops.render.render(write_still=True); print("FRAME",f, flush=True)
print("FRAMES done")
