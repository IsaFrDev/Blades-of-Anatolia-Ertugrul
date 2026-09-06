import bpy, os
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_scene.blend")
keep=["Ertugrul","Shield","ShieldRim","ShieldBoss"]+["Rivet%d"%i for i in range(12)]
bpy.ops.object.select_all(action='DESELECT')
for n in keep:
    o=bpy.data.objects.get(n)
    if o: o.select_set(True)
out="D:/Yuklanadiganlar/ertugrul_fbx/SM_ErtugrulHero.fbx"
bpy.ops.export_scene.fbx(filepath=out, use_selection=True, path_mode='COPY', embed_textures=True, mesh_smooth_type='FACE', add_leaf_bones=False, apply_scale_options='FBX_SCALE_ALL', bake_anim=False)
print("EXPORT", os.path.getsize(out))
