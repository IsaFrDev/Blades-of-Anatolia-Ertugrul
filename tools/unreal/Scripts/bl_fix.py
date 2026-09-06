import bpy, os
bpy.ops.wm.open_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_150k.blend")
o=[o for o in bpy.data.objects if o.type=='MESH'][0]
bpy.context.view_layer.objects.active=o; o.select_set(True); bpy.context.view_layer.update()
h=o.dimensions.z; sc=1.80/h; o.scale=(sc,sc,sc); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True); bpy.context.view_layer.update()
# oyoq tagini 0 ga
mn=min((o.matrix_world @ v.co).z for v in o.data.vertices); o.location.z -= mn; bpy.ops.object.transform_apply(location=True)
out="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_mixamo_150k.fbx"
bpy.ops.export_scene.fbx(filepath=out, use_selection=True, path_mode='COPY', embed_textures=True, mesh_smooth_type='FACE', add_leaf_bones=False)
bpy.ops.wm.save_mainfile()
print("CHK before", h, "dims", tuple(round(v,3) for v in o.dimensions), "size", os.path.getsize(out))
