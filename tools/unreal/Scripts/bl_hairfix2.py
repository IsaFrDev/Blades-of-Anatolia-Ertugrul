import bpy, math
from mathutils import Vector
o=bpy.data.objects["Ertugrul"]; sc=bpy.context.scene
# 1) Haqiqiy tola uzunligini o'lchash
def measured(psname):
    dg=bpy.context.evaluated_depsgraph_get(); oe=o.evaluated_get(dg)
    ps=[p for p in oe.particle_systems if p.name==psname][0]
    ls=[]
    for p in list(ps.particles)[:50]:
        ks=p.hair_keys
        if len(ks)>1: ls.append((Vector(ks[-1].co)-Vector(ks[0].co)).length)
    return sum(ls)/max(len(ls),1)
L=measured("Beard"); want=0.035
k = want/L if L>1e-6 else 1.0
print("measured beard", round(L,4), "k", round(k,4))
# 2) Zonalarni qayta belgilash: soqol = yuz pastki qismi (iyak), soch = boshning orqasi telpak ostidan
front_sign=-1.0
head=[v.co for v in o.data.vertices if 1.55 < v.co.z < 1.75]; cy=sum(v.y for v in head)/len(head); cx=sum(v.x for v in head)/len(head)
for g in list(o.vertex_groups): o.vertex_groups.remove(g)
def vg(name, cond):
    g=o.vertex_groups.new(name=name); idx=[v.index for v in o.data.vertices if cond(v.co)]
    if idx: g.add(idx,1.0,'REPLACE')
    return len(idx)
nb=vg("beard", lambda c: 1.545 < c.z < 1.615 and (c.y-cy)*front_sign > 0.045 and abs(c.x-cx) < 0.075)
nh=vg("hair",  lambda c: 1.60 < c.z < 1.70 and (c.y-cy)*front_sign < -0.055 and abs(c.x-cx) < 0.09)
print("beard verts", nb, "hair verts", nh)
for ps in o.particle_systems:
    st=ps.settings; beard = ps.name=="Beard"
    ps.vertex_group_density = "beard" if beard else "hair"
    st.hair_length = (0.03 if beard else 0.10) * k
    st.count = 700 if beard else 600
    st.child_type='INTERPOLATED'; st.child_percent=6; st.rendered_child_count=6
    st.clump_factor=0.75; st.clump_shape=0.2; st.roughness_1=0.01; st.roughness_2=0.0; st.child_length=0.85
    st.root_radius=0.08; st.tip_radius=0.0; st.radius_scale=0.01
    st.kink='CURL' if beard else 'WAVE'; st.kink_amplitude=0.004; st.kink_frequency=3.0
    st.use_hair_bspline=True; st.hair_step=6
# 3) Qalqon orqaroq va past
back=-front_sign
for n in ["Shield","ShieldRim","ShieldBoss"]+["Rivet%d"%i for i in range(12)]:
    ob=bpy.data.objects[n]; ob.location.y += back*0.09; ob.location.z -= 0.06
# 4) Telpak/kiyim yaltiramasin: roughness >= 0.55
mat=o.data.materials[0]; nt=mat.node_tree; bsdf=next(n for n in nt.nodes if n.type=='BSDF_PRINCIPLED')
if bsdf.inputs["Roughness"].links:
    src=bsdf.inputs["Roughness"].links[0].from_socket
    mx=nt.nodes.new("ShaderNodeMath"); mx.operation='MAXIMUM'; mx.inputs[1].default_value=0.55
    nt.links.new(src, mx.inputs[0]); nt.links.new(mx.outputs[0], bsdf.inputs["Roughness"])
else: bsdf.inputs["Roughness"].default_value=0.7
bsdf.inputs["Specular IOR Level"].default_value=0.3
# 5) Yuz kamerasi biroz uzoqroq
c=bpy.data.objects["Cam_Face"]; c.location=(0.55, front_sign*1.6, 1.62); d=Vector((0,0,1.55))-Vector(c.location); c.rotation_euler=d.to_track_quat('-Z','Y').to_euler(); c.data.lens=70
for name in ["Cam_Front34","Cam_Back","Cam_Face"]:
    sc.camera=bpy.data.objects[name]; sc.render.filepath="D:/temp/claude/bl_ert/%s.png"%name; bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_mainfile()
print("beard now", round(measured("Beard"),4), "hair now", round(measured("Hair"),4))
