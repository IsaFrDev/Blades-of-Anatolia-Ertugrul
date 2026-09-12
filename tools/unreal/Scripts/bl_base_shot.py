import bpy, math
from mathutils import Vector
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
for n in ["Cape","Shield","ShieldRim","ShieldBoss","KaftanRig","ClothRig"]+["Rivet%d"%i for i in range(12)]:
    o=bpy.data.objects.get(n)
    if o: bpy.data.objects.remove(o)
sc=bpy.context.scene; ert=bpy.data.objects["Ertugrul"]
for m in [m for m in ert.modifiers if m.type in ('CLOTH','COLLISION','ARMATURE')]: ert.modifiers.remove(m)
ert.parent=None; ert.animation_data_clear(); ert.location=(0,0,0); ert.rotation_euler=(0,0,0)
cam=bpy.data.objects.get("Cam_Front34") or bpy.data.objects.get("FlyCam")
cam.animation_data_clear(); p=Vector((1.5,3.4,1.35)); cam.location=p
d=Vector((0,0,0.95))-p; cam.rotation_euler=d.to_track_quat('-Z','Y').to_euler(); cam.data.lens=42; sc.camera=cam
sc.render.engine='BLENDER_EEVEE_NEXT'; sc.eevee.taa_render_samples=6
sc.render.resolution_x=960; sc.render.resolution_y=540; sc.render.image_settings.file_format='PNG'
sc.frame_set(1); sc.render.filepath="D:/temp/claude/bl_ert/base_back.png"; bpy.ops.render.render(write_still=True)
print("BASE ok", flush=True)
