import bpy, os
bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete()
bpy.ops.import_scene.gltf(filepath="D:/Yuklanadiganlar/ertugrul.glb")
objs=[o for o in bpy.data.objects if o.type=='MESH']
o=objs[0]; bpy.context.view_layer.objects.active=o; o.select_set(True)
before=sum(len(p.vertices)-2 for p in o.data.polygons)
m=o.modifiers.new("dec",'DECIMATE'); m.ratio=150000/max(before,1); m.use_collapse_triangulate=True
bpy.ops.object.modifier_apply(modifier="dec")
after=sum(len(p.vertices)-2 for p in o.data.polygons)
o.name="Ertugrul"; o.data.name="Ertugrul"
# masshtab: 1.16 m -> 1.80 m
dims=o.dimensions; sc=1.80/dims.z; o.scale=(sc,sc,sc); bpy.ops.object.transform_apply(scale=True)
out="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_mixamo_150k.fbx"
bpy.ops.export_scene.fbx(filepath=out, use_selection=True, path_mode='COPY', embed_textures=True, mesh_smooth_type='FACE', add_leaf_bones=False)
bpy.ops.wm.save_as_mainfile(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_150k.blend")
print("tris", before, "->", after, "height", o.dimensions.z, "size", os.path.getsize(out))
