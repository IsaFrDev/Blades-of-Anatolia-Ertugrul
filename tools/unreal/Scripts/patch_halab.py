# -*- coding: utf-8 -*-
import io, json
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtWorldBuilder.h')
h = rep(h, "// Damashq", "// Damashq\n\tconstexpr float HalabE = 800.f, HalabN = 120.f, HalabR = 130.f, HalabZ = 14.f, HalabMoundR = 42.f, HalabMoundH = 24.f;   // Halab")
h = rep(h, "\tvoid BuildDamascus();", "\tvoid BuildDamascus();\n\tvoid BuildHalab();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildDamascus();\n\t}", "\t\tBuildDamascus();\n\t\tBuildHalab();\n\t}")
c = rep(c, "\t// Damashq: to'g'ri burchakli tekis maydon", '''\t// Halab: tekis aylana + markazda qal'a tepaligi (glasis)
\t{
\t\tconst float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(HalabE, HalabN));
\t\tH = FMath::Lerp(H, HalabZ, 1.f - Smooth01((DD - HalabR - 10.f) / 45.f));
\t\tH += HalabMoundH * Smooth01(1.f - (DD - 6.f) / (HalabMoundR - 6.f));
\t}
\t// Damashq: to'g'ri burchakli tekis maydon''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(HalabE, HalabN));
\t\tC = FMath::Lerp(C, FLinearColor(0.62f, 0.56f, 0.46f), 1.f - Smooth01((DD - HalabR) / 8.f));
\t\tC = FMath::Lerp(C, FLinearColor(0.55f, 0.50f, 0.42f), 1.f - Smooth01((DD - 8.f) / (HalabMoundR - 8.f)));   // glasis tosh
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Damashq\n", "// Damashq\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(HalabE, HalabN)) < HalabR + 30.f) return false;   // Halab\n")
c += r'''
// ---------------- Halab: tepalikdagi qal'a va aylana shahar ----------------

void AErtWorldBuilder::BuildHalab()
{
	FRandomStream RS(Seed + 91);
	FErtMeshData M(100.f);
	int32 S = 2500;
	const float Z = HalabZ, TopZ = HalabZ + HalabMoundH;
	auto HE = [](float u) { return HalabE + u; };
	auto HN = [](float v) { return HalabN + v; };
	const FLinearColor Stone(0.72f, 0.68f, 0.60f), Dark(0.45f, 0.42f, 0.38f), HWood(0.40f, 0.27f, 0.13f), Copper(0.35f, 0.55f, 0.45f), Red(0.6f, 0.12f, 0.1f);
	// Tashqi devor: 16 burchakli halqa, har ikkinchi burchakda burj, sharqda darvoza
	const int32 Sides = 16;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * HalabR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * HalabR, cv = FMath::Sin(Am) * HalabR;
		const bool bGate = (i == 0 || i == Sides - 1);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) { M.AddBox(W(HE(cu), HN(cv), Z + 7.5f), FVector(Len * 50.f, 130, 90), Stone, FRotator(0, Yaw, 0)); }
		else
		{
			M.AddBox(W(HE(cu), HN(cv), Z + 4.f), FVector(Len * 50.f + 20.f, 130, 400), ErtCol::Vary(Stone, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
			{
				const float du = -FMath::Sin(Am) * t, dv = FMath::Cos(Am) * t;
				M.AddBox(W(HE(cu + du), HN(cv + dv), Z + 8.6f), FVector(60, 130, 60), ErtCol::Vary(Stone * 0.92f, 0.05f, ++S), FRotator(0, Yaw, 0));
			}
		}
		if (i % 2 == 0)
		{
			const float tu = FMath::Cos(A0) * HalabR, tv = FMath::Sin(A0) * HalabR;
			M.AddCylinder(W(HE(tu), HN(tv), Z), 4.2f, 3.8f, 11.f, 10, ErtCol::Vary(Stone, 0.04f, ++S), true);
			M.AddCylinder(W(HE(tu), HN(tv), Z + 11.f), 4.4f, 4.4f, 1.f, 10, Stone * 0.9f, true);
		}
	}
	// Sharqiy darvoza minoralari
	for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(HE(HalabR), HN(k * 9.f), Z + 6.5f), FVector(400, 300, 650), ErtCol::Vary(Stone, 0.04f, ++S));
	// Qal'a (tepalik ustida): halqa devor, burjlar, ichida saroy, hammom, minora
	{
		const float CR = 30.f;
		for (int32 i = 0; i < 12; ++i)
		{
			const float A0 = i * 2.f * PI / 12, A1 = (i + 1) * 2.f * PI / 12, Am = (A0 + A1) * 0.5f;
			const float Len = 2.f * CR * FMath::Sin(PI / 12);
			M.AddBox(W(HE(FMath::Cos(Am) * CR), HN(FMath::Sin(Am) * CR), TopZ + 5.f), FVector(Len * 50.f + 20.f, 150, 500), ErtCol::Vary(Dark, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
			if (i % 3 == 0) { M.AddCylinder(W(HE(FMath::Cos(A0) * CR), HN(FMath::Sin(A0) * CR), TopZ), 4.5f, 4.f, 14.f, 10, ErtCol::Vary(Dark, 0.04f, ++S), true); M.AddCylinder(W(HE(FMath::Cos(A0) * CR), HN(FMath::Sin(A0) * CR), TopZ + 14.f), 4.8f, 4.8f, 1.f, 10, Dark * 0.9f, true); }
		}
		// Qal'a darvozasi (janubda) va ko'prik: tepalikdan shahar tekisligiga qiya yo'lak, arklar ustida
		M.AddBox(W(HE(0.f), HN(-CR), TopZ + 8.f), FVector(1000, 500, 800), ErtCol::Vary(Dark, 0.03f, ++S));
		M.AddBox(W(HE(0.f), HN(-CR + 2.f), TopZ + 4.f), FVector(250, 400, 100), Stone * 0.3f);
		const int32 Steps = 14;
		const float Run = HalabMoundR + 26.f - CR;
		const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(HalabMoundH, Run));
		for (int32 i = 0; i < Steps; ++i)
		{
			const float t = (i + 0.5f) / Steps;
			const float v = -CR - 2.f - t * Run, Zc = FMath::Lerp(TopZ + 0.5f, Z + 0.5f, t);
			M.AddBox(W(HE(0.f), HN(v), Zc), FVector(300, 260, 30), ErtCol::Vary(Stone, 0.03f, ++S), FRotator(-Pitch, 0, 0));
			const float G = HeightAt(HE(0.f), HN(v));
			if (i % 2 == 1 && Zc - G > 2.f) M.AddBox(W(HE(0.f), HN(v), (Zc + G) * 0.5f), FVector(240, 120, (Zc - G) * 50.f), ErtCol::Vary(Stone * 0.9f, 0.03f, ++S));
			for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(HE(k * 3.2f), HN(v), Zc + 0.9f), FVector(30, 260, 90), Stone * 0.85f, FRotator(-Pitch, 0, 0));
		}
		M.AddBox(W(HE(-8.f), HN(6.f), TopZ + 4.f), FVector(1400, 1000, 400), ErtCol::Vary(Stone, 0.03f, ++S));
		M.AddBox(W(HE(-8.f), HN(6.f), TopZ + 9.f), FVector(800, 600, 100), ErtCol::Vary(Stone, 0.03f, ++S));
		M.AddSphere(W(HE(-8.f), HN(6.f), TopZ + 10.f), 4.f, 10, Copper, FVector(1, 1, 0.7f));
		M.AddBox(W(HE(12.f), HN(12.f), TopZ + 1.5f), FVector(900, 400, 150), ErtCol::Vary(Stone, 0.03f, ++S));
		for (int32 i = 0; i < 3; ++i) M.AddSphere(W(HE(7.f + i * 5.f), HN(12.f), TopZ + 3.f), 2.4f, 8, ErtCol::Vary(Stone, 0.05f, ++S), FVector(1, 1, 0.8f));
		M.AddCylinder(W(HE(14.f), HN(-8.f), TopZ), 1.6f, 1.4f, 22.f, 10, Stone, true);
		M.AddCylinder(W(HE(14.f), HN(-8.f), TopZ + 22.f), 2.f, 2.f, 1.2f, 10, Stone * 0.9f, true);
		M.AddCylinder(W(HE(14.f), HN(-8.f), TopZ + 23.2f), 1.2f, 0.4f, 3.5f, 8, Copper, true);
		AddBanner(M, HE(-7.f), HN(-CR + 4.f), TopZ, 7.f, Red, false);
		AddBanner(M, HE(7.f), HN(-CR + 4.f), TopZ, 7.f, Red, false);
		AddFire(M, HE(0.f), HN(0.f), TopZ, true);
	}
	// Shahar: tepalik atrofida halqa-halqa uylar
	for (float R = HalabMoundR + 12.f; R < HalabR - 10.f; R += 14.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 13.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(u) < 6.f && v < -HalabMoundR + 4.f) continue;
			if (FMath::Abs(v) < 7.f && u > 0.f) continue;
			if (FMath::Abs(u + 60.f) < 14.f && FMath::Abs(v - 55.f) < 12.f) continue;
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.45f ? 6.5f : 3.8f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(HE(u), HN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(Stone, 0.08f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(HE(u), HN(v), Z + HH + 0.25f), FVector(HW * 50.f + 12.f, HD * 50.f + 12.f, 25.f), ErtCol::Vary(Stone * 0.85f, 0.06f, ++S), FRotator(0, Yaw, 0));
			if (RS.FRand() < 0.25f) M.AddSphere(W(HE(u), HN(v), Z + HH), FMath::Min(HW, HD) * 0.35f, 8, ErtCol::Vary(Stone * 0.95f, 0.05f, ++S), FVector(1, 1, 0.6f));
		}
	}
	// Yopiq bozor: sharqiy darvozadan tepalikkacha gumbazli tomli ko'cha
	for (float u = HalabMoundR + 14.f; u < HalabR - 8.f; u += 7.f)
	{
		M.AddBox(W(HE(u), HN(0.f), Z + 4.f), FVector(330, 1100, 25), ErtCol::Vary(Stone * 0.9f, 0.05f, ++S));
		M.AddSphere(W(HE(u), HN(0.f), Z + 4.f), 2.f, 8, ErtCol::Vary(Stone, 0.05f, ++S), FVector(1, 1, 0.5f));
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddCylinder(W(HE(u - 3.f), HN(k * 5.4f), Z), 0.35f, 0.35f, 4.f, 8, Stone, true); M.AddCylinder(W(HE(u + 3.f), HN(k * 5.4f), Z), 0.35f, 0.35f, 4.f, 8, Stone, true);
			M.AddBox(W(HE(u), HN(k * 4.f), Z + 0.8f), FVector(220, 70, 15), HWood);
			M.AddBox(W(HE(u), HN(k * 4.f), Z + 1.1f), FVector(160, 40, 20), FLinearColor(RS.FRand() * 0.6f + 0.3f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.3f + 0.1f));
		}
	}
	// Katta masjid (shimoli-g'arb): hovli, gumbaz, dumaloq minora
	{
		const float MU = -60.f, MV = 55.f;
		M.AddBox(W(HE(MU), HN(MV), Z + 0.1f), FVector(2200, 1600, 10), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(HE(MU), HN(MV - 9.f), Z + 4.5f), FVector(2200, 700, 450), ErtCol::Vary(Stone, 0.03f, ++S));
		M.AddSphere(W(HE(MU), HN(MV - 9.f), Z + 9.f), 6.5f, 12, Copper, FVector(1, 1, 0.75f));
		for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(HE(MU + k * 22.f), HN(MV + 3.f), Z + 2.5f), FVector(80, 1000, 250), Stone);
		M.AddBox(W(HE(MU), HN(MV + 16.f), Z + 2.5f), FVector(2200, 80, 250), Stone);
		M.AddCylinder(W(HE(MU + 20.f), HN(MV + 14.f), Z), 1.8f, 1.4f, 28.f, 12, Stone, true);
		M.AddCylinder(W(HE(MU + 20.f), HN(MV + 14.f), Z + 28.f), 2.2f, 2.2f, 1.2f, 12, Stone * 0.9f, true);
		M.AddCylinder(W(HE(MU + 20.f), HN(MV + 14.f), Z + 29.2f), 1.3f, 0.4f, 4.f, 8, Copper, true);
	}
	for (int32 i = 0; i < 18; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(HalabMoundR + 10.f, HalabR - 12.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(v) < 8.f && u > 0.f) continue;
		AddTree(M, HE(u), HN(v), Z, RS.FRandRange(0.8f, 1.2f), true, ++S);
	}
	AddBanner(M, HE(HalabR + 3.f), HN(-9.f), Z, 6.f, Red, false);
	AddBanner(M, HE(HalabR + 3.f), HN(9.f), Z, 6.f, Red, false);
	M.Commit(NewPart(TEXT("Halab"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\tif (E.Region.Contains(TEXT("Damashq")) || E.Region.Contains(TEXT("Sham")) || E.Region.Contains(TEXT("Halab"))) { PE = DamE; PN = DamN - DamHalfN - 40.f; }',
        '\tif (E.Region.Contains(TEXT("Damashq")) || E.Region.Contains(TEXT("Sham"))) { PE = DamE; PN = DamN - DamHalfN - 40.f; }\n\telse if (E.Region.Contains(TEXT("Halab"))) { PE = HalabE + HalabR + 45.f; PN = HalabN; }   // Halab sharqiy darvozasi oldi')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("damascus")) { E = ErtMap::DamE; Nn = ErtMap::DamN; }', '\t\telse if (Place == TEXT("damascus")) { E = ErtMap::DamE; Nn = ErtMap::DamN; }\n\t\telse if (Place == TEXT("halab")) { E = ErtMap::HalabE; Nn = ErtMap::HalabN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(DamE, DamN, 130.f, TEXT("Damashq"), Ink);', '\tMark(DamE, DamN, 130.f, TEXT("Damashq"), Ink);\n\tMark(HalabE, HalabN, HalabR, TEXT("Halab"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(27.0f)) Teleport(720.f, -1000.f, 11.f + 45.f, -18.f, 0.f);\n\tif (At(28.6f)) TakeShot(TEXT("damascus"));',
        '\tif (At(26.65f)) Teleport(720.f, -1000.f, 11.f + 45.f, -18.f, 0.f);\n\tif (At(27.6f)) TakeShot(TEXT("damascus"));\n\tif (At(27.7f)) Teleport(985.f, 110.f, 14.f + 55.f, -17.f, 180.f);\n\tif (At(28.7f)) TakeShot(TEXT("halab"));')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'halab_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'halab_savdogar', 'name': 'chr.halab_savdogar.name', 'place': 'halab', 'u': 95, 'v': 7, 'yaw': 180, 'woman': False, 'kaftan': [0.3, 0.3, 0.55], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'halab_hokimi', 'name': 'chr.halab_hokimi.name', 'place': 'halab', 'u': -8, 'v': -4, 'yaw': 270, 'woman': False, 'kaftan': [0.6, 0.1, 0.1], 'dialog': 'ep007_aziz'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.halab_savdogar.name","Halab savdogari","Halep tüccarı","Aleppo merchant"', '"chr.halab_hokimi.name","Al-Aziz, Halab hokimi","El-Aziz, Halep hükümdarı","Al-Aziz, ruler of Aleppo"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
