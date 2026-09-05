# -*- coding: utf-8 -*-
import io
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:80])
    return s.replace(a, b)

# Relyef: yo'l ustidagi vertexlar alfa 0.03 (hali ground diapazoni), shader tosh yotqizma qo'yadi
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/ErtWorldBuilder.cpp'
s = io.open(SRC, encoding='utf-8').read()
s = rep(s, "\t\t\t\t\tC.Add(ErtCol::Sty(TerrainColor(E, N, H, 1.f - Nm.Z), ErtCol::StyleGround));",
        "\t\t\t\t\t{\n\t\t\t\t\t\tfloat RW = 0.f; const float RD = RoadDist(E, N, &RW);\n\t\t\t\t\t\tconst bool bRoad = RD < RW * 0.5f + 0.5f && N > DesertN;\n\t\t\t\t\t\tC.Add(ErtCol::Sty(TerrainColor(E, N, H, 1.f - Nm.Z), bRoad ? 0.03f : ErtCol::StyleGround));   // 0.03 = yo'l (tosh yotqizma)\n\t\t\t\t\t}")
io.open(SRC, 'w', encoding='utf-8', newline='\n').write(s)

p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
m = rep(m, 'TEX_INPUTS = ["GrassD1", "GrassD2", "DirtD1", "DirtD2", "SandD", "RockD", "SnowD", "GrassN", "DirtN", "SandN", "RockN", "SnowN"]',
        'TEX_INPUTS = ["GrassD1", "GrassD2", "DirtD1", "DirtD2", "SandD", "RockD", "SnowD", "CobbleD", "GrassN", "DirtN", "SandN", "RockN", "SnowN", "CobbleN"]')
m = rep(m, '    sample("GrassN", "grass", "N", uv1); sample("DirtN", "dirt", "N", uv1); sample("SandN", "sand", "N", uvs); sample("RockN", "rock", "N", uvr); sample("SnowN", "snow", "N", uv1)',
        '    sample("GrassN", "grass", "N", uv1); sample("DirtN", "dirt", "N", uv1); sample("SandN", "sand", "N", uvs); sample("RockN", "rock", "N", uvr); sample("SnowN", "snow", "N", uv1)\n    uvc = uvnode(0.004, 1700)   # tosh yotqizma 2.5 m\n    sample("CobbleD", "cobble", "D", uvc); sample("CobbleN", "cobble", "N", uvc)')
# Rang: yashil tus + yo'l
m = rep(m, "\tfloat3 gD = lerp(GrassD1, GrassD2, 0.45);", "\tfloat3 gD = lerp(GrassD1, GrassD2, 0.45) * float3(0.78, 1.02, 0.62);   // yashilroq o't")
m = rep(m, "\tfloat3 base = lerp(gD, dD, dry);\n\tbase = lerp(base, sD, sandM);",
        "\tfloat3 base = lerp(gD, dD, dry);\n\tbase = lerp(base, sD, sandM);\n\tfloat road = (s > 0.015) ? 1.0 : 0.0;   // relyef alfa 0.03 = yo'l\n\tbase = lerp(base, CobbleD * 0.9, road * (0.55 + 0.35 * VN(uv * 0.4)));")
m = rep(m, "\tbase *= lerp(float3(1, 1, 1), tint, 0.6) * (0.82 + 0.1 * VN(uv * 0.5));",
        "\tbase *= lerp(lerp(float3(1, 1, 1), tint, 0.6), float3(1, 1, 1), road) * (0.82 + 0.1 * VN(uv * 0.5));")
# Normal: yo'lda tosh yotqizma
m = rep(m, "\tfloat3 t = lerp(lerp(lerp(tg, td, dry), ts, sandM), tr, slope);\n\tt = lerp(t, tn, snow);",
        "\tfloat3 t = lerp(lerp(lerp(tg, td, dry), ts, sandM), tr, slope);\n\tt = lerp(t, tn, snow);\n\tt = lerp(t, CobbleN, (s > 0.015) ? 0.8 : 0.0);")
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('patched')
