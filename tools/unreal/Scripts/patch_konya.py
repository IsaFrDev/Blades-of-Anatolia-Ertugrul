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
h = rep(h, "// Halab\n", "// Halab\n\tconstexpr float KonE = 120.f, KonN = 480.f, KonR = 150.f, KonZ = 12.f, KonHillR = 55.f, KonHillH = 9.f;   // Konya\n")
h = rep(h, "\tvoid BuildHalab();", "\tvoid BuildHalab();\n\tvoid BuildKonya();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildHalab();\n\t}", "\t\tBuildHalab();\n\t\tBuildKonya();\n\t}")
c = rep(c, "\t\t{{-470, -280}, {-470, -470}, 8},", "\t\t{{300, 300}, {275, 480}, 6},\n\t\t{{-470, -280}, {-470, -470}, 8},")
c = rep(c, "\t// Halab: tekis aylana + markazda qal'a tepaligi (glasis)", '''\t// Konya: tekis aylana + markazda past Aloiddin tepaligi
\t{
\t\tconst float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(KonE, KonN));
\t\tH = FMath::Lerp(H, KonZ, 1.f - Smooth01((DD - KonR - 10.f) / 50.f));
\t\tH += KonHillH * Smooth01(1.f - (DD - 12.f) / (KonHillR - 12.f));
\t}
\t// Halab: tekis aylana + markazda qal'a tepaligi (glasis)''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(KonE, KonN));
\t\tC = FMath::Lerp(C, FLinearColor(0.66f, 0.60f, 0.50f), 1.f - Smooth01((DD - KonR) / 8.f));
\t\tC = FMath::Lerp(C, FLinearColor(0.42f, 0.52f, 0.30f), 1.f - Smooth01((DD - 14.f) / (KonHillR - 14.f)));   // tepalik o'tloq
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Halab\n", "// Halab\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(KonE, KonN)) < KonR + 30.f) return false;   // Konya\n")
c += r'''
// ---------------- Konya: Saljuqiylar poytaxti ----------------

void AErtWorldBuilder::BuildKonya()
{
	FRandomStream RS(Seed + 113);
	FErtMeshData M(100.f);
	int32 S = 3500;
	const float Z = KonZ, TopZ = KonZ + KonHillH;
	auto KE = [](float u) { return KonE + u; };
	auto KN = [](float v) { return KonN + v; };
	const FLinearColor KStone(0.76f, 0.70f, 0.58f), KDark(0.50f, 0.45f, 0.38f), KWood(0.38f, 0.25f, 0.12f), Turq(0.12f, 0.58f, 0.62f), Tile(0.16f, 0.32f, 0.62f), KGold(0.9f, 0.75f, 0.25f), Plane(0.22f, 0.45f, 0.18f), Trunk(0.55f, 0.50f, 0.42f);
	// Devor: 24 burchak, har burchakda burj (Konya "yuz burj" shahri), sharqiy va g'arbiy darvozalar
	const int32 Sides = 24;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * KonR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * KonR, cv = FMath::Sin(Am) * KonR;
		const bool bGate = (i == 0 || i == Sides - 1 || i == Sides / 2 || i == Sides / 2 - 1);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) M.AddBox(W(KE(cu), KN(cv), Z + 7.f), FVector(Len * 50.f, 120, 80), KStone, FRotator(0, Yaw, 0));
		else
		{
			M.AddBox(W(KE(cu), KN(cv), Z + 3.5f), FVector(Len * 50.f + 20.f, 120, 350), ErtCol::Vary(KStone, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
				M.AddBox(W(KE(cu - FMath::Sin(Am) * t), KN(cv + FMath::Cos(Am) * t), Z + 7.6f), FVector(60, 120, 60), ErtCol::Vary(KStone * 0.92f, 0.05f, ++S), FRotator(0, Yaw, 0));
		}
		const float tu = FMath::Cos(A0) * KonR, tv = FMath::Sin(A0) * KonR;
		M.AddBox(W(KE(tu), KN(tv), Z + 5.f), FVector(320, 320, 500), ErtCol::Vary(KStone, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A0), 0));
		M.AddBox(W(KE(tu), KN(tv), Z + 10.4f), FVector(360, 360, 40), KStone * 0.9f, FRotator(0, FMath::RadiansToDegrees(A0), 0));
	}
	for (int32 side = -1; side <= 1; side += 2)
		for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(KE(side * KonR), KN(k * 9.f), Z + 6.f), FVector(380, 300, 600), ErtCol::Vary(KStone, 0.04f, ++S)); M.AddBox(W(KE(side * KonR), KN(k * 9.f), Z + 12.4f), FVector(420, 340, 40), KStone * 0.9f); }
	// Aloiddin tepaligi: masjid (uzun ibodatxona, feruza gumbaz, qalin kvadrat minora), saroy ko'shki, ichki devor
	{
		M.AddBox(W(KE(-10.f), KN(4.f), TopZ + 3.5f), FVector(2600, 1300, 350), ErtCol::Vary(KStone, 0.03f, ++S));
		M.AddBox(W(KE(-10.f), KN(4.f), TopZ + 7.3f), FVector(2600, 1300, 30), KStone * 0.85f);
		M.AddBox(W(KE(2.f), KN(-2.f), TopZ + 8.f), FVector(500, 500, 100), KStone);
		M.AddSphere(W(KE(2.f), KN(-2.f), TopZ + 9.f), 5.2f, 12, Turq, FVector(1, 1, 0.8f));
		M.AddBox(W(KE(-30.f), KN(12.f), TopZ + 0.5f), FVector(600, 400, 50), KStone);
		M.AddCylinder(W(KE(-30.f), KN(12.f), TopZ), 2.f, 1.7f, 26.f, 4, KStone, true);
		M.AddCylinder(W(KE(-30.f), KN(12.f), TopZ + 26.f), 2.4f, 2.4f, 1.f, 8, KStone * 0.9f, true);
		M.AddCylinder(W(KE(-30.f), KN(12.f), TopZ + 27.f), 1.6f, 0.5f, 4.f, 8, Turq, true);
		// Saroy ko'shki (tepalik chetida, ustunli ayvon, chodir tom)
		M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 4.f), FVector(700, 700, 400), ErtCol::Vary(KStone, 0.03f, ++S));
		for (int32 i = 0; i < 4; ++i) M.AddCylinder(W(KE(30.f + ((i & 1) ? 8.f : -8.f)), KN(-28.f + ((i & 2) ? 8.f : -8.f)), TopZ + 8.f), 0.5f, 0.5f, 4.f, 8, KStone, true);
		M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 12.2f), FVector(950, 950, 25), KWood);
		M.AddCylinder(W(KE(30.f), KN(-28.f), TopZ + 12.4f), 9.f, 0.3f, 5.f, 4, Tile, true);
		M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 10.5f), FVector(400, 30, 250), KWood); M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 10.5f), FVector(30, 400, 250), KWood);
		// Ichki devor halqasi (past)
		for (int32 i = 0; i < 16; ++i)
		{
			const float Am = (i + 0.5f) * 2.f * PI / 16, Len = 2.f * (KonHillR - 6.f) * FMath::Sin(PI / 16);
			if (i == 12) continue;   // janubiy yo'lak
			M.AddBox(W(KE(FMath::Cos(Am) * (KonHillR - 6.f)), KN(FMath::Sin(Am) * (KonHillR - 6.f)), TopZ - 1.5f + 1.5f), FVector(Len * 50.f + 20.f, 100, 200), ErtCol::Vary(KDark, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
		}
		AddBanner(M, KE(24.f), KN(-40.f), TopZ, 7.f, Tile, false);
		AddBanner(M, KE(36.f), KN(-40.f), TopZ, 7.f, Tile, false);
		AddFire(M, KE(10.f), KN(14.f), TopZ, true);
	}
	// Kumbetlar (konus tomli maqbaralar) - tepalik janubi-g'arbida
	for (int32 i = 0; i < 4; ++i)
	{
		const float u = -70.f + i * 12.f, v = -70.f + (i & 1) * 8.f;
		M.AddCylinder(W(KE(u), KN(v), Z), 3.2f, 3.2f, 7.f, 8, ErtCol::Vary(KStone, 0.05f, ++S), true);
		M.AddCylinder(W(KE(u), KN(v), Z + 7.f), 3.6f, 0.2f, 5.5f, 8, i % 2 ? Turq : KDark, true);
	}
	// Madrasa (shimoli-sharq): hovli, katta portal, naqshli baland minora (Ince Minareli)
	{
		const float MU = 60.f, MV = 60.f;
		M.AddBox(W(KE(MU), KN(MV), Z + 3.f), FVector(1600, 1400, 300), ErtCol::Vary(KStone, 0.03f, ++S));
		M.AddBox(W(KE(MU), KN(MV), Z + 0.2f), FVector(800, 700, 20), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(KE(MU), KN(MV - 14.f), Z + 6.f), FVector(500, 150, 600), ErtCol::Vary(KStone, 0.03f, ++S));   // portal
		M.AddBox(W(KE(MU), KN(MV - 14.6f), Z + 3.5f), FVector(200, 40, 350), KStone * 0.3f);
		M.AddSphere(W(KE(MU - 10.f), KN(MV + 4.f), Z + 6.f), 4.5f, 12, Turq, FVector(1, 1, 0.75f));
		M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z), 1.5f, 1.5f, 8.f, 8, KStone, true);
		M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z + 8.f), 1.2f, 1.0f, 22.f, 12, Tile, true);
		for (int32 k = 0; k < 2; ++k) M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z + 14.f + k * 8.f), 1.6f, 1.6f, 0.8f, 12, Turq, true);
		M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z + 30.f), 1.2f, 0.3f, 3.5f, 8, KGold, true);
	}
	// Karvonsaroy (janubi-sharq) va bozor
	{
		const float CU = 75.f, CV = -60.f;
		M.AddBox(W(KE(CU), KN(CV), Z + 3.f), FVector(1800, 1400, 300), ErtCol::Vary(KStone, 0.03f, ++S));
		M.AddBox(W(KE(CU), KN(CV), Z + 0.2f), FVector(1000, 700, 20), FLinearColor(0.7f, 0.65f, 0.55f));
		M.AddBox(W(KE(CU - 18.f), KN(CV), Z + 5.f), FVector(150, 500, 500), KStone);
		M.AddBox(W(KE(CU), KN(CV + 5.f), Z + 6.5f), FVector(1000, 700, 50), KStone * 0.9f);
	}
	for (float u = KonHillR + 8.f; u < KonR - 12.f; u += 7.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(KE(u), KN(k * 7.f), Z + 3.f), FVector(330, 250, 20), ErtCol::Vary(KWood, 0.1f, ++S));
			M.AddCylinder(W(KE(u - 3.f), KN(k * 9.4f), Z), 0.3f, 0.3f, 3.f, 6, KWood, true); M.AddCylinder(W(KE(u + 3.f), KN(k * 9.4f), Z), 0.3f, 0.3f, 3.f, 6, KWood, true);
			M.AddBox(W(KE(u), KN(k * 7.f), Z + 0.8f), FVector(220, 70, 15), KWood);
			M.AddBox(W(KE(u), KN(k * 7.f), Z + 1.1f), FVector(160, 40, 20), FLinearColor(RS.FRand() * 0.5f + 0.2f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.5f + 0.2f));
		}
	// Uylar: halqa-halqa, yog'och ayvonli ikki qavatli
	for (float R = KonHillR + 14.f; R < KonR - 12.f; R += 15.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 14.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(v) < 12.f) continue;                                                  // sharq-g'arb xiyobon
			if (FMath::Abs(u) < 7.f && v < 0.f) continue;                                        // janubiy yo'lak
			if (FMath::Abs(u - 60.f) < 12.f && FMath::Abs(v - 60.f) < 12.f) continue;
			if (FMath::Abs(u - 75.f) < 14.f && FMath::Abs(v + 60.f) < 12.f) continue;
			if (FMath::Abs(u + 60.f) < 26.f && FMath::Abs(v + 68.f) < 10.f) continue;
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.5f ? 6.5f : 4.f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(KE(u), KN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(KStone, 0.08f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(KE(u), KN(v), Z + HH + 0.4f), FVector(HW * 50.f + 40.f, HD * 50.f + 40.f, 40.f), ErtCol::Vary(FLinearColor(0.55f, 0.32f, 0.2f), 0.06f, ++S), FRotator(0, Yaw, 0));   // sopol tom
			if (HH > 5.f) M.AddBox(W(KE(u), KN(v), Z + 3.9f), FVector(HW * 50.f + 50.f, HD * 50.f + 50.f, 12.f), KWood, FRotator(0, Yaw, 0));   // ayvon
		}
	}
	// Chinor xiyoboni (sharq-g'arb) va tepalik daraxtlari
	for (float u = -KonR + 14.f; u < KonR - 12.f; u += 11.f)
	{
		if (FMath::Abs(u) < KonHillR + 4.f) continue;
		for (int32 k = -1; k <= 1; k += 2)
		{
			const float E = KE(u), N = KN(k * 11.5f);
			M.AddCylinder(W(E, N, Z), 0.45f, 0.35f, 5.f, 6, Trunk, true);
			M.AddSphere(W(E, N, Z + 7.f), 4.f, 8, ErtCol::Vary(Plane, 0.08f, ++S), FVector(1, 1, 0.8f));
		}
	}
	for (int32 i = 0; i < 10; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(20.f, KonHillR - 12.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(u + 10.f) < 16.f && FMath::Abs(v - 4.f) < 9.f) continue;
		if (FMath::Abs(u - 30.f) < 10.f && FMath::Abs(v + 28.f) < 10.f) continue;
		const float ZZ = HeightAt(KE(u), KN(v));
		M.AddCylinder(W(KE(u), KN(v), ZZ), 0.4f, 0.3f, 4.f, 6, Trunk, true);
		M.AddSphere(W(KE(u), KN(v), ZZ + 5.5f), 3.2f, 8, ErtCol::Vary(Plane, 0.08f, ++S), FVector(1, 1, 0.8f));
	}
	AddBanner(M, KE(KonR + 3.f), KN(-9.f), Z, 6.f, Tile, false);
	AddBanner(M, KE(KonR + 3.f), KN(9.f), Z, 6.f, Tile, false);
	M.Commit(NewPart(TEXT("Konya"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("Halab"))) { PE = HalabE + HalabR + 45.f; PN = HalabN; }   // Halab sharqiy darvozasi oldi',
        '\telse if (E.Region.Contains(TEXT("Halab"))) { PE = HalabE + HalabR + 45.f; PN = HalabN; }   // Halab sharqiy darvozasi oldi\n\telse if (E.Region.Contains(TEXT("Konya")) || E.Region.Contains(TEXT("Kubadabad")) || E.Region.Contains(TEXT("Kayseri"))) { PE = KonE + KonR + 45.f; PN = KonN; }   // Konya sharqiy darvozasi oldi')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("halab")) { E = ErtMap::HalabE; Nn = ErtMap::HalabN; }', '\t\telse if (Place == TEXT("halab")) { E = ErtMap::HalabE; Nn = ErtMap::HalabN; }\n\t\telse if (Place == TEXT("konya")) { E = ErtMap::KonE; Nn = ErtMap::KonN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(HalabE, HalabN, HalabR, TEXT("Halab"), Ink);', '\tMark(HalabE, HalabN, HalabR, TEXT("Halab"), Ink);\n\tMark(KonE, KonN, KonR, TEXT("Konya"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(28.7f)) TakeShot(TEXT("halab"));', '\tif (At(28.7f)) TakeShot(TEXT("halab"));\n\tif (At(28.75f)) Teleport(310.f, 480.f, 12.f + 55.f, -18.f, -90.f);\n\tif (At(29.75f)) TakeShot(TEXT("konya"));')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'konya_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'konya_savdogar', 'name': 'chr.konya_savdogar.name', 'place': 'konya', 'u': 85, 'v': 8, 'yaw': 180, 'woman': False, 'kaftan': [0.2, 0.45, 0.5], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'konya_kopek', 'name': 'chr.kopek.name', 'place': 'konya', 'u': 30, 'v': -40, 'yaw': 0, 'woman': False, 'kaftan': [0.15, 0.15, 0.2], 'dialog': 'ep011_kopek'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.konya_savdogar.name","Konya savdogari","Konya tüccarı","Konya merchant"', '"chr.kopek.name","Sa\'duddin Ko\'pek","Sadeddin Köpek","Sadeddin Kopek"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
