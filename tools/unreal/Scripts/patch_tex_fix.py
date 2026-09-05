# -*- coding: utf-8 -*-
import io, json
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:80])
    return s.replace(a, b)

p = 'D:/temp/claude/ert_make_pbr.py'
s = io.open(p, encoding='utf-8').read()
s = rep(s, '''TEX_INPUTS = []
for _r in TEX_ROLES:
    TEX_INPUTS += ["T%sD" % _r.capitalize(), "T%sN" % _r.capitalize()]''', '''TEX_INPUTS = ["GrassD1", "GrassD2", "DirtD1", "DirtD2", "SandD", "RockD", "SnowD", "GrassN", "DirtN", "SandN", "RockN", "SnowN"]''')
s = rep(s, '''    texnodes = {}
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
            texnodes[key] = to''', '''    # Dunyo koordinatali UV (XY tekisligi): WorldPosition.xy * masshtab
    wpm = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -1500, 500)
    wpm.set_editor_property("r", True); wpm.set_editor_property("g", True); wpm.set_editor_property("b", False); wpm.set_editor_property("a", False)
    MEL.connect_material_expressions(wp, "", wpm, "")
    def uvnode(scale, y):
        k = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -1500, y)
        k.set_editor_property("r", scale)
        mu = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -1400, y)
        MEL.connect_material_expressions(wpm, "", mu, "A"); MEL.connect_material_expressions(k, "", mu, "B")
        return mu
    uv1 = uvnode(0.0033, 560)    # 3 m plitka
    uv2 = uvnode(0.0009, 620)    # 11 m plitka
    uvs = uvnode(0.005, 680)     # qum 2 m
    uvr = uvnode(0.0022, 740)    # qoya 4.5 m
    texnodes = {}
    tyy = [800]
    def sample(key, role, suffix, uvn):
        path = "/Game/ErtAssets/Tex/T_%s_%s" % (role, suffix)
        ts = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -1200, tyy[0]); tyy[0] += 70
        t = EAL.load_asset(path) if EAL.does_asset_exist(path) else None
        if t is None:
            t = EAL.load_asset("/Engine/EngineResources/DefaultTexture") if suffix == "D" else EAL.load_asset("/Engine/EngineMaterials/DefaultNormal")
            log("tekstura yo'q, standart: " + path)
        ts.set_editor_property("texture", t)
        ts.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if suffix == "N" else unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
        MEL.connect_material_expressions(uvn, "", ts, "UVs")
        texnodes[key] = ts
    sample("GrassD1", "grass", "D", uv1); sample("GrassD2", "grass", "D", uv2)
    sample("DirtD1", "dirt", "D", uv1); sample("DirtD2", "dirt", "D", uv2)
    sample("SandD", "sand", "D", uvs); sample("RockD", "rock", "D", uvr); sample("SnowD", "snow", "D", uv1)
    sample("GrassN", "grass", "N", uv1); sample("DirtN", "dirt", "N", uv1); sample("SandN", "sand", "N", uvs); sample("RockN", "rock", "N", uvr); sample("SnowN", "snow", "N", uv1)''')
s = rep(s, '''        for key, to in texnodes.items():
            MEL.connect_material_expressions(to, "", cu, key)''', '''        for key, ts in texnodes.items():
            MEL.connect_material_expressions(ts, "RGB", cu, key)''')
s = rep(s, '''	float2 u1 = uv * 0.33, u2 = uv * 0.09;   // 3 m va 11 m plitkalar (takrorlanish kamayadi)
	float3 gD = lerp(Texture2DSample(TGrassD, TGrassDSampler, u1).rgb, Texture2DSample(TGrassD, TGrassDSampler, u2 + 0.37).rgb, 0.45);
	float3 dD = lerp(Texture2DSample(TDirtD, TDirtDSampler, u1).rgb, Texture2DSample(TDirtD, TDirtDSampler, u2 + 0.11).rgb, 0.45);
	float3 sD = Texture2DSample(TSandD, TSandDSampler, u1 * 1.5).rgb;
	float3 rD = Texture2DSample(TRockD, TRockDSampler, uv * 0.22).rgb;
	float3 nD = Texture2DSample(TSnowD, TSnowDSampler, u1).rgb;''', '''	float3 gD = lerp(GrassD1, GrassD2, 0.45);
	float3 dD = lerp(DirtD1, DirtD2, 0.45);
	float3 sD = SandD;
	float3 rD = RockD;
	float3 nD = SnowD;''')
s = rep(s, '''	float2 u1 = uv * 0.33;
	float3 tg = Texture2DSample(TGrassN, TGrassNSampler, u1).rgb * 2.0 - 1.0;
	float3 td = Texture2DSample(TDirtN, TDirtNSampler, u1).rgb * 2.0 - 1.0;
	float3 ts = Texture2DSample(TSandN, TSandNSampler, u1 * 1.5).rgb * 2.0 - 1.0;
	float3 tr = Texture2DSample(TRockN, TRockNSampler, uv * 0.22).rgb * 2.0 - 1.0;
	float3 tn = Texture2DSample(TSnowN, TSnowNSampler, u1).rgb * 2.0 - 1.0;''', '''	float3 tg = GrassN, td = DirtN, ts = SandN, tr = RockN, tn = SnowN;   // Normal sampler: allaqachon -1..1''')
s = rep(s, '''	float3 n0;
	if (pl == 2) n0 = float3(t.x, t.y, t.z * sign(N.z));
	else if (pl == 0) n0 = float3(t.z * sign(N.x), t.x, t.y);
	else n0 = float3(t.x, t.z * sign(N.y), t.y);
	float puddle''', '''	// Relyef normali: XY tekislik bo'yicha, vertex normaliga aralashtiriladi (qiyaliklarda ham to'g'ri)
	float3 n0 = normalize(float3(t.x, t.y, t.z) + N * 1.2);
	float puddle''')
io.open(p, 'w', encoding='utf-8', newline='\n').write(s)
print('pbr ok')

p = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/character.json'
d = json.load(io.open(p, encoding='utf-8'))
for k in ('hero', 'enemy'):
    d[k]['anims'].pop('hurt', None)
    d[k]['_hurt_izoh'] = "Mannequin HitReact animatsiyalari additiv (SingleNode bilan ishlamaydi) - hurt yo'q; Blender'dan oddiy hurt qo'shilsa ishlaydi"
io.open(p, 'w', encoding='utf-8', newline='\n').write(json.dumps(d, ensure_ascii=False, indent=1))
print('json ok')

SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/ErtHeroBody.cpp'
s = io.open(SRC, encoding='utf-8').read()
s = rep(s, "\tif (Skel && OneShotT <= 0.15f)\n\t{\n\t\tSkelPlay(TEXT(\"hurt\"), false, 1.3f, -1, TEXT(\"idle\"));\n\t\tif (CurAnim) OneShotT = FMath::Min(0.6f, CurAnim->GetPlayLength() / 1.3f);\n\t}",
        "\tif (Skel && OneShotT <= 0.15f && SkelAnims.Contains(TEXT(\"hurt\")))\n\t{\n\t\tSkelPlay(TEXT(\"hurt\"), false, 1.3f, -1, TEXT(\"idle\"));\n\t\tif (CurAnim) OneShotT = FMath::Min(0.6f, CurAnim->GetPlayLength() / 1.3f);\n\t}")
s = rep(s, "\t\t\tauto Add = [&](const FString& P) { if (UAnimSequence* A = LoadObject<UAnimSequence>(nullptr, *ErtObjPath(P))) List.Add(A); else UE_LOG(LogErtugrul, Warning, TEXT(\"character.json: animatsiya topilmadi %s\"), *P); };",
        "\t\t\tauto Add = [&](const FString& P) { UAnimSequence* A = LoadObject<UAnimSequence>(nullptr, *ErtObjPath(P)); if (!A) UE_LOG(LogErtugrul, Warning, TEXT(\"character.json: animatsiya topilmadi %s\"), *P); else if (A->IsValidAdditive()) UE_LOG(LogErtugrul, Warning, TEXT(\"character.json: %s additiv - o'tkazib yuborildi\"), *P); else List.Add(A); };")
io.open(SRC, 'w', encoding='utf-8', newline='\n').write(s)
print('cpp ok')
