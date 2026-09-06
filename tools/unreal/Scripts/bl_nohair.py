import bpy
o=bpy.data.objects["Ertugrul"]; sc=bpy.context.scene
for m in [m for m in o.modifiers if m.type=='PARTICLE_SYSTEM']: o.modifiers.remove(m)
for name in ["Cam_Front34","Cam_Back","Cam_Face"]:
    sc.camera=bpy.data.objects[name]; sc.render.filepath="D:/temp/claude/bl_ert/%s.png"%name; bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_mainfile()
print("ok", len(o.particle_systems))
