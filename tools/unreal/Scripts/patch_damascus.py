# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtWorldBuilder.h')
h = rep(h, "\tconstexpr float DesertN = -700.f;   // shundan janubda cho'l", "\tconstexpr float DesertN = -700.f;   // shundan janubda cho'l\n\tconstexpr float DamE = 720.f, DamN = -850.f, DamHalfE = 140.f, DamHalfN = 110.f, DamZ = 11.f;   // Damashq")
h = rep(h, "\tvoid BuildDesert();", "\tvoid BuildDesert();\n\tvoid BuildDamascus();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildDesert();\n\t}", "\t\tBuildDesert();\n\t\tBuildDamascus();\n\t}")
# flatten terrain under Damascus
c = rep(c, "\t// Voha ko'li va oba yonidagi ko'l: qirg'oq tekis, o'rtasi chuqur", '''\t// Damashq: to'g'ri burchakli tekis maydon
\t{
\t\tconst float DX = FMath::Max(0.f, FMath::Abs(E - DamE) - DamHalfE), DY = FMath::Max(0.f, FMath::Abs(N - DamN) - DamHalfN);
\t\tconst float DD = FMath::Sqrt(DX * DX + DY * DY);
\t\tH = FMath::Lerp(H, DamZ, 1.f - Smooth01((DD - 15.f) / 45.f));
\t}
\t// Voha ko'li va oba yonidagi ko'l: qirg'oq tekis, o'rtasi chuqur''')
# color: paved inside
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DX = FMath::Max(0.f, FMath::Abs(E - DamE) - DamHalfE), DY = FMath::Max(0.f, FMath::Abs(N - DamN) - DamHalfN);
\t\tC = FMath::Lerp(C, FLinearColor(0.70f, 0.62f, 0.48f), 1.f - Smooth01((FMath::Sqrt(DX * DX + DY * DY) - 2.f) / 8.f));
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "\tif (FMath::Max(FMath::Abs(E - CaravanE), FMath::Abs(N - CaravanN)) < 40.f) return false;          // karvonsaroy",
        "\tif (FMath::Max(FMath::Abs(E - CaravanE), FMath::Abs(N - CaravanN)) < 40.f) return false;          // karvonsaroy\n\tif (FMath::Abs(E - DamE) < DamHalfE + 30.f && FMath::Abs(N - DamN) < DamHalfN + 30.f) return false;  // Damashq")
c += r'''
// ---------------- Damashq: devorli katta shahar ----------------

void AErtWorldBuilder::BuildDamascus()
{
	FRandomStream RS(Seed + 77);
	FErtMeshData M(100.f);
	int32 S = 1500;
	const float Z = DamZ;
	auto DE = [](float u) { return DamE + u; };
	auto DN = [](float v) { return DamN + v; };
	const FLinearColor Lime(0.86f, 0.82f, 0.70f), Basalt(0.30f, 0.30f, 0.32f), Wood(0.42f, 0.28f, 0.14f), Gold(0.9f, 0.75f, 0.25f), Teal(0.18f, 0.5f, 0.55f), Green(0.16f, 0.40f, 0.14f);
	// Devorlar (8 m baland, 2.5 m qalin), kungura, burjlar
	const float WallH = 8.f;
	for (int32 side = 0; side < 4; ++side)
	{
		const bool bEW = side < 2;                       // 0,1: shimol/janub devori (E bo'ylab), 2,3: g'arb/sharq
		const float Sign = (side & 1) ? 1.f : -1.f;
		const float Len = bEW ? DamHalfE : DamHalfN;
		for (float t = -Len; t < Len; t += 5.f)
		{
			const float u = bEW ? t + 2.5f : Sign * DamHalfE, v = bEW ? Sign * DamHalfN : t + 2.5f;
			const bool bGate = (side == 0 && FMath::Abs(u) < 6.f) || (side == 2 && FMath::Abs(v) < 6.f);
			if (bGate) { M.AddBox(W(DE(u), DN(v), Z + 7.5f), FVector(bEW ? 250 : 125, bEW ? 125 : 250, 100), Lime * 0.95f); continue; }
			M.AddBox(W(DE(u), DN(v), Z + WallH * 0.5f), FVector(bEW ? 250 : 125, bEW ? 125 : 250, WallH * 50.f), ErtCol::Vary(Lime, 0.05f, ++S));
			M.AddBox(W(DE(u), DN(v), Z + WallH + 0.5f), FVector(bEW ? 60 : 125, bEW ? 125 : 60, 50), ErtCol::Vary(Lime * 0.92f, 0.05f, ++S));
		}
	}
	for (int32 i = 0; i < 4; ++i)
	{
		const float u = (i & 1) ? DamHalfE : -DamHalfE, v = (i & 2) ? DamHalfN : -DamHalfN;
		M.AddCylinder(W(DE(u), DN(v), Z), 5.f, 4.6f, 13.f, 12, ErtCol::Vary(Lime, 0.04f, ++S), true);
		M.AddCylinder(W(DE(u), DN(v), Z + 13.f), 5.4f, 5.4f, 1.2f, 12, Lime * 0.9f, true);
	}
	// Darvoza minoralari
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(DE(k * 9.f), DN(-DamHalfN), Z + 6.f), FVector(300, 300, 600), ErtCol::Vary(Lime, 0.04f, ++S)); M.AddBox(W(DE(-DamHalfE), DN(k * 9.f), Z + 6.f), FVector(300, 300, 600), ErtCol::Vary(Lime, 0.04f, ++S)); }
	// Umaviylar masjidi: hovli devori 60x40, ibodatxona 60x18, katta gumbaz, baland kvadrat minora
	{
		const float MU = 20.f, MV = 25.f;
		M.AddBox(W(DE(MU), DN(MV), Z + 0.1f), FVector(3000, 2000, 10), FLinearColor(0.92f, 0.9f, 0.85f));                       // marmar hovli
		for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(DE(MU + k * 30.f), DN(MV), Z + 3.f), FVector(80, 2000, 300), Lime); M.AddBox(W(DE(MU), DN(MV + k * 20.f), Z + 3.f), FVector(3000, 80, 300), Lime); }
		M.AddBox(W(DE(MU), DN(MV - 12.f), Z + 5.f), FVector(3000, 900, 500), ErtCol::Vary(Lime, 0.03f, ++S));                    // ibodatxona
		M.AddBox(W(DE(MU), DN(MV - 12.f), Z + 10.5f), FVector(1200, 900, 60), Lime * 0.9f);
		M.AddSphere(W(DE(MU), DN(MV - 12.f), Z + 10.f), 8.5f, 14, Basalt, FVector(1, 1, 0.8f));                                   // gumbaz
		M.AddCylinder(W(DE(MU + 28.f), DN(MV + 18.f), Z), 2.4f, 2.2f, 34.f, 4, Lime, true);                                        // kvadrat minora
		M.AddCylinder(W(DE(MU + 28.f), DN(MV + 18.f), Z + 34.f), 2.8f, 2.8f, 1.5f, 8, Lime * 0.9f, true);
		M.AddCylinder(W(DE(MU + 28.f), DN(MV + 18.f), Z + 35.5f), 1.6f, 0.6f, 5.f, 8, Teal, true);
		M.AddSphere(W(DE(MU + 28.f), DN(MV + 18.f), Z + 41.f), 0.6f, 6, Gold);
		M.AddCylinder(W(DE(MU), DN(MV + 6.f), Z), 2.5f, 2.5f, 0.8f, 12, Basalt, true);                                             // hovli favvorasi
	}
	// Saroy (janubi-sharq): ikki qavatli, gumbazli, ayvonli
	{
		const float PU = 85.f, PV = -55.f;
		M.AddBox(W(DE(PU), DN(PV), Z + 4.f), FVector(2200, 1600, 400), ErtCol::Vary(Lime, 0.03f, ++S));
		M.AddBox(W(DE(PU), DN(PV), Z + 10.f), FVector(1400, 1000, 200), ErtCol::Vary(Lime * 0.97f, 0.03f, ++S));
		M.AddSphere(W(DE(PU), DN(PV), Z + 12.f), 6.f, 12, Teal, FVector(1, 1, 0.8f));
		for (int32 i = 0; i < 6; ++i) M.AddCylinder(W(DE(PU - 22.f), DN(PV - 12.f + i * 5.f), Z), 0.5f, 0.5f, 4.f, 8, Lime * 0.9f, true);
		M.AddBox(W(DE(PU - 22.f), DN(PV), Z + 4.3f), FVector(150, 1400, 30), Wood);
	}
	// Bozor ko'chasi (shimoliy darvozadan masjidgacha): arkadalar va rastalar
	for (float v = -DamHalfN + 12.f; v < 5.f; v += 8.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(DE(k * 9.f), DN(v), Z + 3.2f), FVector(150, 350, 20), ErtCol::Vary(Lime, 0.05f, ++S));                    // ark usti
			M.AddCylinder(W(DE(k * 9.f), DN(v - 3.f), Z), 0.4f, 0.4f, 3.2f, 8, Lime, true); M.AddCylinder(W(DE(k * 9.f), DN(v + 3.f), Z), 0.4f, 0.4f, 3.2f, 8, Lime, true);
			M.AddBox(W(DE(k * 9.f), DN(v), Z + 0.8f), FVector(70, 200, 15), Wood);
			M.AddBox(W(DE(k * 9.f), DN(v), Z + 1.1f), FVector(40, 150, 20), FLinearColor(RS.FRand() * 0.6f + 0.3f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.3f + 0.1f));
		}
	// Uylar: tekis tomli, ba'zilari gumbazli, 2 qavatli; ko'chalar grid
	for (float u = -DamHalfE + 14.f; u < DamHalfE - 10.f; u += 16.f)
		for (float v = -DamHalfN + 14.f; v < DamHalfN - 10.f; v += 16.f)
		{
			if (FMath::Abs(u) < 14.f && v < 8.f) continue;                                    // bozor ko'chasi
			if (FMath::Abs(u - 20.f) < 36.f && FMath::Abs(v - 25.f) < 26.f) continue;         // masjid
			if (FMath::Abs(u - 85.f) < 28.f && FMath::Abs(v + 55.f) < 22.f) continue;         // saroy
			if (RS.FRand() < 0.12f) continue;
			const float HW = RS.FRandRange(4.5f, 6.5f), HD = RS.FRandRange(4.5f, 6.5f), HH = RS.FRand() < 0.35f ? 6.5f : 3.8f;
			const float E = DE(u + RS.FRandRange(-2.f, 2.f)), N = DN(v + RS.FRandRange(-2.f, 2.f));
			M.AddBox(W(E, N, Z + HH * 0.5f), FVector(HW * 100.f, HD * 100.f, HH * 50.f), ErtCol::Vary(Lime, 0.08f, ++S));
			M.AddBox(W(E, N, Z + HH + 0.3f), FVector(HW * 100.f + 15.f, HD * 100.f + 15.f, 30.f), ErtCol::Vary(Lime * 0.9f, 0.06f, ++S));
			if (RS.FRand() < 0.3f) M.AddSphere(W(E, N, Z + HH), FMath::Min(HW, HD) * 0.7f, 8, ErtCol::Vary(Lime * 0.95f, 0.05f, ++S), FVector(1, 1, 0.6f));
			M.AddBox(W(E + HW * 0.5f, N, Z + 1.1f), FVector(12, 60, 110), Wood);
		}
	// Palmalar (hovlilar va darvoza oldi)
	for (int32 i = 0; i < 24; ++i)
	{
		const float u = RS.FRandRange(-DamHalfE + 8.f, DamHalfE - 8.f), v = RS.FRandRange(-DamHalfN + 8.f, DamHalfN - 8.f);
		if (FMath::Abs(u) < 14.f && v < 8.f) continue;
		AddPalm(M, DE(u), DN(v), Z, RS.FRandRange(6.f, 9.f), ++S);
	}
	for (int32 i = 0; i < 8; ++i) AddPalm(M, DE(-30.f + i * 9.f), DN(-DamHalfN - 12.f), HeightAt(DE(-30.f + i * 9.f), DN(-DamHalfN - 12.f)), RS.FRandRange(7.f, 10.f), ++S);
	// Bayroqlar va darvoza oldi olovi
	AddBanner(M, DE(-9.f), DN(-DamHalfN - 3.f), Z, 6.f, Green, false);
	AddBanner(M, DE(9.f), DN(-DamHalfN - 3.f), Z, 6.f, Green, false);
	AddFire(M, DE(18.f), DN(-DamHalfN + 10.f), Z, true);
	M.Commit(NewPart(TEXT("Damascus"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

# Mission anchors: Sham/Halab/Damashq regions -> Damascus north gate
c = load('ErtMission.cpp')
c = rep(c, "\t// Epizod indeksiga qarab biroz siljitamiz - har epizod boshqa joyda", "\t// Sham / Halab / Damashq epizodlari - Damashq shimoliy darvozasi oldi\n\tif (E.Region.Contains(TEXT(\"Damashq\")) || E.Region.Contains(TEXT(\"Sham\")) || E.Region.Contains(TEXT(\"Halab\"))) { PE = DamE; PN = DamN - DamHalfN - 40.f; }\n\t// Epizod indeksiga qarab biroz siljitamiz - har epizod boshqa joyda")
save('ErtMission.cpp', c)
# NPC place damascus; HUD map marker
c = load('ErtGameMode.cpp')
c = rep(c, "\t\telse if (Place == TEXT(\"forest\")) { E = -330.f; Nn = 700.f; }", "\t\telse if (Place == TEXT(\"forest\")) { E = -330.f; Nn = 700.f; }\n\t\telse if (Place == TEXT(\"damascus\")) { E = ErtMap::DamE; Nn = ErtMap::DamN; }")
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, "\tMark(-120.f, -860.f, 20.f, TEXT(\"Xarobalar\"), Ink);", "\tMark(-120.f, -860.f, 20.f, TEXT(\"Xarobalar\"), Ink);\n\tMark(DamE, DamN, 130.f, TEXT(\"Damashq\"), Ink);")
save('ErtHUD.cpp', c)
# move Al-Aziz / Ibn Arabi council NPCs? keep. Add Damascus merchant NPC
import json
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'sham_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'sham_savdogar', 'name': 'chr.sham_savdogar.name', 'place': 'damascus', 'u': -9, 'v': -60, 'yaw': 90, 'woman': False, 'kaftan': [0.5, 0.35, 0.15], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'ibn_arabi_dam', 'name': 'chr.ibn_arabi.name', 'place': 'damascus', 'u': 20, 'v': 31, 'yaw': 180, 'woman': False, 'kaftan': [0.85, 0.82, 0.75], 'dialog': 'ep008_arabi'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
r = '"chr.sham_savdogar.name","Damashq savdogari","Şam tüccarı","Damascus merchant"'
if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
