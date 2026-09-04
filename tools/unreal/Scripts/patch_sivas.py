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
h = rep(h, "// Qayseri va Erciyes\n", "// Qayseri va Erciyes\n\tconstexpr float SivE = -180.f, SivN = 400.f, SivR = 120.f, SivZ = 13.f, SivHillR = 40.f, SivHillH = 8.f;   // Sivas (qal'a tepaligi shimolda)\n")
h = rep(h, "\tvoid BuildKayseri();", "\tvoid BuildKayseri();\n\tvoid BuildSivas();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildKayseri();\n\t}", "\t\tBuildKayseri();\n\t\tBuildSivas();\n\t}")
c = rep(c, "{{300, 300}, {320, 110}, 6},", "{{300, 300}, {320, 110}, 6}, {{-300, 250}, {-180, 275}, 6},")
c = rep(c, "\t// Erciyes: vulqon konusi", '''\t// Sivas: tekis aylana + shimolida past qal'a tepaligi
\t{
\t\tconst float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN));
\t\tH = FMath::Lerp(H, SivZ, 1.f - Smooth01((DS - SivR - 10.f) / 50.f));
\t\tconst float DH = FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN + 70.f));
\t\tH += SivHillH * Smooth01(1.f - (DH - 10.f) / (SivHillR - 10.f));
\t}
\t// Erciyes: vulqon konusi''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN));
\t\tC = FMath::Lerp(C, FLinearColor(0.60f, 0.52f, 0.42f), 1.f - Smooth01((DS - SivR) / 8.f));
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Qayseri\n", "// Qayseri\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN)) < SivR + 30.f) return false;   // Sivas\n")
c += r'''
// ---------------- Sivas: madrasalar shahri ----------------

void AErtWorldBuilder::BuildSivas()
{
	FRandomStream RS(Seed + 151);
	FErtMeshData M(100.f);
	int32 S = 5500;
	const float Z = SivZ, TopZ = SivZ + SivHillH;
	auto SE = [](float u) { return SivE + u; };
	auto SN = [](float v) { return SivN + v; };
	const FLinearColor Brick(0.62f, 0.40f, 0.28f), BrickD(0.48f, 0.30f, 0.20f), SStone(0.74f, 0.68f, 0.56f), Turq(0.12f, 0.58f, 0.62f), Cobalt(0.15f, 0.25f, 0.6f), SWood(0.38f, 0.25f, 0.12f), Lead(0.45f, 0.47f, 0.52f), SGreen(0.10f, 0.35f, 0.15f), Willow(0.35f, 0.55f, 0.25f);
	// Minora yasovchi: g'isht tanasi, feruza halqalar, sharafa, konus
	auto Minaret = [&](float u, float v, float Zb, float Hh, float R0, int32 Bands)
	{
		M.AddBox(W(SE(u), SN(v), Zb + 2.5f), FVector(R0 * 130.f, R0 * 130.f, 250), SStone);
		M.AddCylinder(W(SE(u), SN(v), Zb + 5.f), R0, R0 * 0.8f, Hh, 12, ErtCol::Vary(Brick, 0.04f, ++S), true);
		for (int32 b = 1; b <= Bands; ++b) M.AddCylinder(W(SE(u), SN(v), Zb + 5.f + Hh * b / (Bands + 1)), R0 * 1.02f, R0 * 1.02f, 0.6f, 12, Turq, true);
		M.AddCylinder(W(SE(u), SN(v), Zb + 5.f + Hh), R0 * 1.35f, R0 * 1.35f, 1.f, 12, ErtCol::Vary(BrickD, 0.04f, ++S), true);
		M.AddCylinder(W(SE(u), SN(v), Zb + 6.f + Hh), R0 * 0.8f, R0 * 0.8f, 2.5f, 12, Brick, true);
		M.AddCylinder(W(SE(u), SN(v), Zb + 8.5f + Hh), R0 * 1.1f, 0.2f, 3.5f, 8, Lead, true);
	};
	// Tashqi devor: 14 burchak, tosh, burjlar; janubiy va sharqiy darvozalar
	const int32 Sides = 14;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * SivR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * SivR, cv = FMath::Sin(Am) * SivR;
		const bool bGate = (FMath::Abs(FMath::Sin(Am) + 1.f) < 0.25f) || (FMath::Abs(FMath::Cos(Am) - 1.f) < 0.12f);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) M.AddBox(W(SE(cu), SN(cv), Z + 7.f), FVector(Len * 50.f, 120, 80), SStone, FRotator(0, Yaw, 0));
		else
		{
			M.AddBox(W(SE(cu), SN(cv), Z + 3.5f), FVector(Len * 50.f + 20.f, 120, 350), ErtCol::Vary(SStone, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
				M.AddBox(W(SE(cu - FMath::Sin(Am) * t), SN(cv + FMath::Cos(Am) * t), Z + 7.6f), FVector(60, 120, 60), ErtCol::Vary(SStone * 0.92f, 0.05f, ++S), FRotator(0, Yaw, 0));
		}
		const float tu = FMath::Cos(A0) * SivR, tv = FMath::Sin(A0) * SivR;
		M.AddCylinder(W(SE(tu), SN(tv), Z), 3.8f, 3.5f, 10.f, 10, ErtCol::Vary(SStone, 0.04f, ++S), true);
		M.AddCylinder(W(SE(tu), SN(tv), Z + 10.f), 4.1f, 4.1f, 1.f, 10, SStone * 0.9f, true);
	}
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(SE(k * 9.f), SN(-SivR), Z + 6.f), FVector(300, 380, 600), ErtCol::Vary(SStone, 0.04f, ++S)); M.AddBox(W(SE(SivR), SN(k * 9.f), Z + 6.f), FVector(380, 300, 600), ErtCol::Vary(SStone, 0.04f, ++S)); }
	// Qal'a tepaligi (shimol): past devor halqasi, burj, saroy
	{
		const float HV = 70.f, CR = SivHillR - 8.f;
		for (int32 i = 0; i < 12; ++i)
		{
			if (i == 9) continue;   // janubiy kirish
			const float Am = (i + 0.5f) * 2.f * PI / 12, Len = 2.f * CR * FMath::Sin(PI / 12);
			M.AddBox(W(SE(FMath::Cos(Am) * CR), SN(HV + FMath::Sin(Am) * CR), TopZ + 2.f), FVector(Len * 50.f + 20.f, 100, 250), ErtCol::Vary(SStone * 0.85f, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
		}
		M.AddCylinder(W(SE(0.f), SN(HV + CR), TopZ), 4.f, 3.6f, 12.f, 10, SStone * 0.85f, true);
		M.AddBox(W(SE(-6.f), SN(HV + 4.f), TopZ + 3.5f), FVector(1100, 800, 350), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(-6.f), SN(HV + 4.f), TopZ + 7.3f), FVector(1100, 800, 30), SWood);
		M.AddSphere(W(SE(-6.f), SN(HV + 4.f), TopZ + 7.5f), 3.f, 10, Lead, FVector(1, 1, 0.7f));
		AddBanner(M, SE(-5.f), SN(HV - CR + 5.f), TopZ, 7.f, SGreen, false);
		AddBanner(M, SE(5.f), SN(HV - CR + 5.f), TopZ, 7.f, SGreen, false);
		AddFire(M, SE(8.f), SN(HV - 4.f), TopZ, true);
	}
	// Go'k Madrasa (markaz-g'arb): hovli, feruza koshinli portal, qo'sh minora
	{
		const float MU = -55.f, MV = 10.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 3.5f), FVector(1700, 1500, 350), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU), SN(MV), Z + 0.2f), FVector(900, 800, 20), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(SE(MU + 17.f), SN(MV), Z + 6.f), FVector(150, 600, 600), ErtCol::Vary(SStone, 0.03f, ++S));   // portal fasadi
		M.AddBox(W(SE(MU + 17.8f), SN(MV), Z + 6.f), FVector(20, 420, 500), Turq);                                  // koshin
		M.AddBox(W(SE(MU + 18.2f), SN(MV), Z + 3.f), FVector(20, 200, 400), SStone * 0.3f);                           // eshik
		Minaret(MU + 17.f, MV - 8.f, Z + 9.f, 14.f, 1.1f, 3); Minaret(MU + 17.f, MV + 8.f, Z + 9.f, 14.f, 1.1f, 3);
		M.AddSphere(W(SE(MU - 8.f), SN(MV + 9.f), Z + 7.f), 3.5f, 10, Turq, FVector(1, 1, 0.7f));
		M.AddSphere(W(SE(MU - 8.f), SN(MV - 9.f), Z + 7.f), 3.5f, 10, Turq, FVector(1, 1, 0.7f));
	}
	// Chifte Minorali madrasa (markaz-sharq): faqat baland fasad va qo'sh minora, orqada hovli
	{
		const float MU = 45.f, MV = 20.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 7.f), FVector(200, 1100, 700), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU - 1.1f), SN(MV), Z + 5.f), FVector(20, 450, 700), ErtCol::Vary(BrickD, 0.03f, ++S));
		M.AddBox(W(SE(MU - 1.3f), SN(MV), Z + 3.f), FVector(20, 220, 450), SStone * 0.3f);
		Minaret(MU, MV - 9.f, Z + 14.f, 16.f, 1.2f, 4); Minaret(MU, MV + 9.f, Z + 14.f, 16.f, 1.2f, 4);
		M.AddBox(W(SE(MU + 12.f), SN(MV), Z + 2.5f), FVector(1000, 1100, 250), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU + 12.f), SN(MV), Z + 0.2f), FVector(600, 700, 20), FLinearColor(0.9f, 0.88f, 0.82f));
	}
	// Burujiya madrasasi (janubi-sharq): hovli, gumbazli maqbara
	{
		const float MU = 50.f, MV = -50.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 3.f), FVector(1300, 1200, 300), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU), SN(MV), Z + 0.2f), FVector(700, 600, 20), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(SE(MU), SN(MV - 12.f), Z + 5.5f), FVector(500, 120, 550), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddSphere(W(SE(MU - 9.f), SN(MV + 8.f), Z + 6.f), 3.2f, 10, Turq, FVector(1, 1, 0.7f));
	}
	// Ulu Jome' (janubi-g'arb): keng past ibodatxona, ko'p ustunli, qiyshaygan g'isht minora
	{
		const float MU = -50.f, MV = -55.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 3.f), FVector(2200, 1500, 300), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU), SN(MV), Z + 6.3f), FVector(2200, 1500, 30), SWood);
		for (int32 i = 0; i < 4; ++i) M.AddSphere(W(SE(MU - 15.f + i * 10.f), SN(MV), Z + 6.f), 2.f, 8, ErtCol::Vary(SStone, 0.05f, ++S), FVector(1, 1, 0.5f));
		M.AddBox(W(SE(MU + 24.f), SN(MV + 10.f), Z + 1.f), FVector(220, 220, 100), SStone);
		M.AddCylinder(W(SE(MU + 24.f), SN(MV + 10.f), Z + 2.f), 1.4f, 1.1f, 26.f, 12, ErtCol::Vary(Brick, 0.04f, ++S), true, FRotator(2.5f, 0, 1.5f));
		M.AddCylinder(W(SE(MU + 25.1f), SN(MV + 10.7f), Z + 28.f), 1.8f, 1.8f, 0.8f, 12, BrickD, true);
		M.AddCylinder(W(SE(MU + 25.2f), SN(MV + 10.8f), Z + 28.8f), 1.2f, 0.2f, 3.f, 8, Lead, true);
	}
	// Bozor: janubiy darvozadan markazgacha yog'och soyabonli rastalar
	for (float v = -SivR + 16.f; v < -20.f; v += 7.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(SE(k * 8.f), SN(v), Z + 3.f), FVector(300, 330, 15), ErtCol::Vary(SWood, 0.1f, ++S), FRotator(0, 0, k * 8.f));
			M.AddCylinder(W(SE(k * 10.5f), SN(v - 3.f), Z), 0.25f, 0.25f, 3.2f, 6, SWood, true); M.AddCylinder(W(SE(k * 10.5f), SN(v + 3.f), Z), 0.25f, 0.25f, 3.2f, 6, SWood, true);
			M.AddBox(W(SE(k * 8.f), SN(v), Z + 0.8f), FVector(70, 220, 15), SWood);
			M.AddBox(W(SE(k * 8.f), SN(v), Z + 1.1f), FVector(40, 160, 20), FLinearColor(RS.FRand() * 0.5f + 0.2f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.5f + 0.2f));
		}
	// Uylar: g'isht-tosh, sopol tomli
	for (float R = 28.f; R < SivR - 12.f; R += 15.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 14.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(u) < 13.f && v < 0.f) continue;                                             // bozor
			if (FMath::Abs(v) < 8.f) continue;                                                        // sharq-g'arb ko'cha
			if (FVector2D::Distance(FVector2D(u, v), FVector2D(0.f, 70.f)) < SivHillR + 2.f) continue; // tepalik
			if (FMath::Abs(u + 55.f) < 22.f && FMath::Abs(v - 10.f) < 20.f) continue;
			if (FMath::Abs(u - 50.f) < 20.f && FMath::Abs(v - 20.f) < 16.f) continue;
			if (FMath::Abs(u - 50.f) < 17.f && FMath::Abs(v + 50.f) < 16.f) continue;
			if (FMath::Abs(u + 50.f) < 26.f && FMath::Abs(v + 55.f) < 18.f) continue;
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.4f ? 6.5f : 4.f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(SE(u), SN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(RS.FRand() < 0.5f ? Brick : SStone, 0.08f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(SE(u), SN(v), Z + HH + 0.4f), FVector(HW * 50.f + 40.f, HD * 50.f + 40.f, 40.f), ErtCol::Vary(FLinearColor(0.55f, 0.30f, 0.18f), 0.06f, ++S), FRotator(0, Yaw, 0));
		}
	}
	// Tollar (daryo bo'yi shahri) va darvoza bayroqlari
	for (int32 i = 0; i < 16; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(25.f, SivR - 10.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(u) < 14.f && v < 0.f) continue;
		if (FVector2D::Distance(FVector2D(u, v), FVector2D(0.f, 70.f)) < SivHillR + 2.f) continue;
		const float ZZ = HeightAt(SE(u), SN(v));
		M.AddCylinder(W(SE(u), SN(v), ZZ), 0.4f, 0.3f, 3.f, 6, SWood, true);
		M.AddSphere(W(SE(u), SN(v), ZZ + 4.5f), 3.f, 8, ErtCol::Vary(Willow, 0.08f, ++S), FVector(1, 1, 1.1f));
	}
	AddBanner(M, SE(-9.f), SN(-SivR - 3.f), Z, 6.f, SGreen, false);
	AddBanner(M, SE(9.f), SN(-SivR - 3.f), Z, 6.f, SGreen, false);
	M.Commit(NewPart(TEXT("Sivas"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("Kayseri"))) { PE = KayE - KayR - 45.f; PN = KayN; }   // Qayseri g\'arbiy darvozasi oldi',
        '\telse if (E.Region.Contains(TEXT("Kayseri"))) { PE = KayE - KayR - 45.f; PN = KayN; }   // Qayseri g\'arbiy darvozasi oldi\n\telse if (E.Region.Contains(TEXT("Sivas"))) { PE = SivE; PN = SivN - SivR - 45.f; }   // Sivas janubiy darvozasi oldi')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("kayseri")) { E = ErtMap::KayE; Nn = ErtMap::KayN; }', '\t\telse if (Place == TEXT("kayseri")) { E = ErtMap::KayE; Nn = ErtMap::KayN; }\n\t\telse if (Place == TEXT("sivas")) { E = ErtMap::SivE; Nn = ErtMap::SivN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(KayE, KayN, KayR, TEXT("Qayseri"), Ink);', '\tMark(KayE, KayN, KayR, TEXT("Qayseri"), Ink);\n\tMark(SivE, SivN, SivR, TEXT("Sivas"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
for a, b in [('At(27.2f)) TakeShot(TEXT("damascus"))', 'At(27.1f)) TakeShot(TEXT("damascus"))'), ('At(27.22f)) Teleport(965', 'At(27.12f)) Teleport(965'), ('At(27.8f)) TakeShot(TEXT("halab"))', 'At(27.6f)) TakeShot(TEXT("halab"))'),
             ('At(27.82f)) Teleport(310', 'At(27.62f)) Teleport(310'), ('At(28.4f)) TakeShot(TEXT("konya"));', 'At(28.1f)) TakeShot(TEXT("konya"));'), ('At(28.42f)) Teleport(260', 'At(28.12f)) Teleport(260'),
             ('At(28.98f)) TakeShot(TEXT("kayseri"));', 'At(28.55f)) TakeShot(TEXT("kayseri"));\n\tif (At(28.57f)) Teleport(-180.f, 220.f, 13.f + 55.f, -18.f, 0.f);\n\tif (At(28.98f)) TakeShot(TEXT("sivas"));')]:
    c = rep(c, a, b)
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'sivas_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'sivas_savdogar', 'name': 'chr.sivas_savdogar.name', 'place': 'sivas', 'u': 9, 'v': -60, 'yaw': 90, 'woman': False, 'kaftan': [0.45, 0.25, 0.15], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'sivas_mudarris', 'name': 'chr.sivas_mudarris.name', 'place': 'sivas', 'u': -36, 'v': 10, 'yaw': 0, 'woman': False, 'kaftan': [0.85, 0.82, 0.75], 'dialog': 'ep014_talk'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.sivas_savdogar.name","Sivas savdogari","Sivas tüccarı","Sivas merchant"', '"chr.sivas_mudarris.name","Go\'k Madrasa mudarrisi","Gök Medrese müderrisi","Teacher of the Gok Medrese"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
