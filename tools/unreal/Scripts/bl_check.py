import bpy
bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete()
bpy.ops.import_scene.fbx(filepath="D:/Yuklanadiganlar/ertugrul_fbx/ertugrul_mixamo_150k.fbx")
bpy.context.view_layer.update()
for o in bpy.data.objects:
    if o.type=='MESH':
        print("CHK", o.name, "dims", tuple(round(v,3) for v in o.dimensions), "scale", tuple(o.scale), "tris", sum(len(p.vertices)-2 for p in o.data.polygons), "uv", len(o.data.uv_layers), "mats", [m.name for m in o.data.materials], "imgs", [i.name for i in bpy.data.images])
