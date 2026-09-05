# -*- coding: utf-8 -*-
# Yer materialiga haqiqiy teksturalar (Poly Haven): Custom tugunlarga TextureObject kirishlari
import io
p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# 1) Custom tugun kirishlari: WP, N, VC + teksturalar
m = rep(m, '''def custom(m, code, name, x, y, out_type):
    cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    cu.set_editor_property("code", code)
    cu.set_editor_property("description", name)
    cu.set_editor_property("output_type", out_type)
    ins = []
    for nm in ["WP", "N", "VC"]:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        ins.append(ci)
    cu.set_editor_property("inputs", ins)
    return cu''', '''TEX_ROLES = ["grass", "dirt", "rock", "sand", "snow"]
TEX_INPUTS = []
for _r in TEX_ROLES:
    TEX_INPUTS += ["T%sD" % _r.capitalize(), "T%sN" % _r.capitalize()]


def tex_exists(role, suffix):
    return EAL.does_asset_exist("/Game/ErtAssets/Tex/T_%s_%s" % (role, suffix))


def custom(m, code, name, x, y, out_type):
    cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    cu.set_editor_property("code", code)
    cu.set_editor_property("description", name)
    cu.set_editor_property("output_type", out_type)
    ins = []
    for nm in ["WP", "N", "VC"] + TEX_INPUTS:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        ins.append(ci)
    cu.set_editor_property("inputs", ins)
    return cu''')

# 2) Tekstura ob'ektlarini ulash
m = rep(m, '''    for cu in (ccol, cnrm):
        MEL.connect_material_expressions(wp, "", cu, "WP")
        MEL.connect_material_expressions(nw, "", cu, "N")
        MEL.connect_material_expressions(vc4, "", cu, "VC")''', '''    texnodes = {}
    ty = 500
    for role in TEX_ROLES:
        for suffix in ("D", "N"):
            path = "/Game/ErtAssets/Tex/T_%s_%s" % (role, suffix)
            key = "T%s%s" % (role.capitalize(), suffix)
            to = MEL.create_material_expression(m, unreal.MaterialExpressionTextureObject, -1200, ty); ty += 60
            t = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
            if t is None:
                t = EAL.load_asset("/Engine/EngineResources/DefaultTexture") if suffix == "D" else EAL.load_asset("/Engine/EngineMaterials/DefaultNormal")
                log("tekstura yo'q, standart: " + path)
            to.set_editor_property("texture", t)
            texnodes[key] = to
    for cu in (ccol, cnrm):
        MEL.connect_material_expressions(wp, "", cu, "WP")
        MEL.connect_material_expressions(nw, "", cu, "N")
        MEL.connect_material_expressions(vc4, "", cu, "VC")
        for key, to in texnodes.items():
            MEL.connect_material_expressions(to, "", cu, key)''')

# 3) HLSL: yer qatlamlari teksturadan (rang)
m = rep(m, '''if (st == 0)
{
	// Auto-qatlamlar: qiyalikda qoya, 75 m dan yuqorida qor, tekis pastlikda ko'lmak
	float slope = 1.0 - saturate((N.z - 0.62) / 0.28);
	float rockH = HROCK(uv * 0.5);
	float3 rockC = float3(0.42, 0.40, 0.37) * (0.62 + 0.55 * saturate(rockH + 0.25));
	float3 grassC = col * (0.72 + 0.55 * h);
	float snow = saturate((Pm.z - 75.0) / 18.0) * saturate((N.z - 0.55) / 0.3);
	float puddle = (N.z > 0.996 && Pm.z < 20.0) ? smoothstep(0.86, 0.90, VN(uv * 0.35 + 7.3)) * smoothstep(0.62, 0.74, VN(uv * 0.06 + 3.1)) : 0.0;
	col = lerp(grassC, rockC, slope);
	col = lerp(col, float3(0.92, 0.94, 0.98) * (0.85 + 0.15 * h), snow);
	col = lerp(col, float3(0.20, 0.24, 0.27), puddle * 0.75);
	h = lerp(h, rockH, slope);
	if (puddle > 0.5) h = 1.5;   // roughness tuguni uchun ko'lmak belgisi
}''', '''if (st == 0)
{
	// Auto-qatlamlar teksturadan: o't/tuproq (relyef rangi bo'yicha), qum (cho'l), qiyalikda qoya, 75 m dan yuqorida qor, ko'lmak
	float slope = 1.0 - saturate((N.z - 0.62) / 0.28);
	float lum = dot(VC.rgb, float3(0.3, 0.5, 0.2));
	float dry = saturate((VC.r / max(VC.g, 0.01) - 0.75) * 3.0);
	float sandM = saturate((lum - 0.42) * 6.0) * dry;
	float2 u1 = uv * 0.33, u2 = uv * 0.09;   // 3 m va 11 m plitkalar (takrorlanish kamayadi)
	float3 gD = lerp(Texture2DSample(TGrassD, TGrassDSampler, u1).rgb, Texture2DSample(TGrassD, TGrassDSampler, u2 + 0.37).rgb, 0.45);
	float3 dD = lerp(Texture2DSample(TDirtD, TDirtDSampler, u1).rgb, Texture2DSample(TDirtD, TDirtDSampler, u2 + 0.11).rgb, 0.45);
	float3 sD = Texture2DSample(TSandD, TSandDSampler, u1 * 1.5).rgb;
	float3 rD = Texture2DSample(TRockD, TRockDSampler, uv * 0.22).rgb;
	float3 nD = Texture2DSample(TSnowD, TSnowDSampler, u1).rgb;
	float3 base = lerp(gD, dD, dry);
	base = lerp(base, sD, sandM);
	float3 tint = VC.rgb / max(lum, 0.05);
	base *= lerp(float3(1, 1, 1), tint, 0.35) * (0.95 + 0.1 * VN(uv * 0.5));
	float snow = saturate((Pm.z - 75.0) / 18.0) * saturate((N.z - 0.55) / 0.3);
	float puddle = (N.z > 0.996 && Pm.z < 20.0) ? smoothstep(0.86, 0.90, VN(uv * 0.35 + 7.3)) * smoothstep(0.62, 0.74, VN(uv * 0.06 + 3.1)) : 0.0;
	col = lerp(base, rD, slope);
	col = lerp(col, nD, snow);
	col = lerp(col, float3(0.20, 0.24, 0.27), puddle * 0.75);
	h = lerp(h, HROCK(uv * 0.5), slope);
	if (puddle > 0.5) h = 1.5;   // roughness tuguni uchun ko'lmak belgisi
}''')

# 4) HLSL: yer normali teksturadan (normal xarita, tekislikka moslab)
m = rep(m, '''float e = (st == 1 || st == 3 || st == 5 || st == 7 || st == 9 || st == 13 || st == 14) ? 0.002 : (st == 11) ? 0.03 : 0.015;''',
        '''if (st == 0)
{
	float slope = 1.0 - saturate((N.z - 0.62) / 0.28);
	float lum = dot(VC.rgb, float3(0.3, 0.5, 0.2));
	float dry = saturate((VC.r / max(VC.g, 0.01) - 0.75) * 3.0);
	float sandM = saturate((lum - 0.42) * 6.0) * dry;
	float snow = saturate((Pm.z - 75.0) / 18.0) * saturate((N.z - 0.55) / 0.3);
	float2 u1 = uv * 0.33;
	float3 tg = Texture2DSample(TGrassN, TGrassNSampler, u1).rgb * 2.0 - 1.0;
	float3 td = Texture2DSample(TDirtN, TDirtNSampler, u1).rgb * 2.0 - 1.0;
	float3 ts = Texture2DSample(TSandN, TSandNSampler, u1 * 1.5).rgb * 2.0 - 1.0;
	float3 tr = Texture2DSample(TRockN, TRockNSampler, uv * 0.22).rgb * 2.0 - 1.0;
	float3 tn = Texture2DSample(TSnowN, TSnowNSampler, u1).rgb * 2.0 - 1.0;
	float3 t = lerp(lerp(lerp(tg, td, dry), ts, sandM), tr, slope);
	t = lerp(t, tn, snow);
	t.xy *= 0.8;
	t = normalize(t);
	float3 n0;
	if (pl == 2) n0 = float3(t.x, t.y, t.z * sign(N.z));
	else if (pl == 0) n0 = float3(t.z * sign(N.x), t.x, t.y);
	else n0 = float3(t.x, t.z * sign(N.y), t.y);
	float puddle = (N.z > 0.996 && Pm.z < 20.0) ? smoothstep(0.86, 0.90, VN(uv * 0.35 + 7.3)) * smoothstep(0.62, 0.74, VN(uv * 0.06 + 3.1)) : 0.0;
	return normalize(lerp(n0, N, puddle));
}
float e = (st == 1 || st == 3 || st == 5 || st == 7 || st == 9 || st == 13 || st == 14) ? 0.002 : (st == 11) ? 0.03 : 0.015;''')
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('material script patched')
