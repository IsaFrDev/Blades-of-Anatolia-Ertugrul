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
h = rep(h, "// Sivas (qal'a tepaligi shimolda)\n", "// Sivas (qal'a tepaligi shimolda)\n\tconstexpr float ErzE = 420.f, ErzN = 730.f, ErzR = 110.f, ErzZ = 22.f, ErzHillR = 38.f, ErzHillH = 10.f;   // Erzurum (qal'a tepaligi shimoli-sharqda)\n")
h = rep(h, "\tvoid BuildSivas();", "\tvoid BuildSivas();\n\tvoid BuildErzurum();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildSivas();\n\t}", "\t\tBuildSivas();\n\t\tBuildErzurum();\n\t}")
c = rep(c, "{{-300, 250}, {-180, 275}, 6},", "{{-300, 250}, {-180, 275}, 6}, {{500, 520}, {420, 615}, 6},")
c = rep(c, "\t// Sivas: tekis aylana + shimolida past qal'a tepaligi", '''\t// Erzurum: baland tekis aylana + shimoli-sharqda qal'a tepaligi
\t{
\t\tconst float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE, ErzN));
\t\tH = FMath::Lerp(H, ErzZ, 1.f - Smooth01((DS - ErzR - 10.f) / 60.f));
\t\tconst float DH = FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE + 45.f, ErzN + 45.f));
\t\tH += ErzHillH * Smooth01(1.f - (DH - 8.f) / (ErzHillR - 8.f));
\t}
\t// Sivas: tekis aylana + shimolida past qal'a tepaligi''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE, ErzN));
\t\tC = FMath::Lerp(C, FLinearColor(0.50f, 0.48f, 0.46f), 1.f - Smooth01((DS - ErzR) / 8.f));
\t\tC = FMath::Lerp(C, FLinearColor(0.80f, 0.80f, 0.78f), (1.f - Smooth01((DS - ErzR - 20.f) / 60.f)) * 0.35f);   // sovuq qirov o'tloq
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Sivas\n", "// Sivas\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE, ErzN)) < ErzR + 30.f) return false;   // Erzurum\n")
c += r'''
// ---------------- Erzurum: baland tekislik qal'a shahri ----------------

void AErtWorldBuilder::BuildErzurum()
{
	FRandomStream RS(Seed + 173);
	FErtMeshData M(100.f);
	int32 S = 6500;
	const float Z = ErzZ, TopZ = ErzZ + ErzHillH;
	auto XE = [](float u) { return ErzE + u; };
	auto XN = [](float v) { return ErzN + v; };
	const FLinearColor Grey(0.58f, 0.56f, 0.52f), GreyD(0.42f, 0.40f, 0.38f), Brick(0.60f, 0.38f, 0.26f), BrickD(0.46f, 0.28f, 0.18f), Turq(0.12f, 0.58f, 0.62f), XWood(0.36f, 0.24f, 0.12f), Lead(0.45f, 0.47f, 0.52f), XRed(0.55f, 0.10f, 0.08f), Pine(0.10f, 0.32f, 0.16f), Snow(0.95f, 0.96f, 1.0f);
	auto Minaret = [&](float u, float v, float Zb, float Hh, float R0)
	{
		M.AddBox(W(XE(u), XN(v), Zb + 2.5f), FVector(R0 * 130.f, R0 * 130.f, 250), Grey);
		M.AddCylinder(W(XE(u), XN(v), Zb + 5.f), R0, R0 * 0.85f, Hh, 12, ErtCol::Vary(Brick, 0.04f, ++S), true);
		for (int32 b = 1; b <= 3; ++b) M.AddCylinder(W(XE(u), XN(v), Zb + 5.f + Hh * b / 4.f), R0 * 1.03f, R0 * 1.03f, 0.5f, 12, Turq, true);
		M.AddCylinder(W(XE(u), XN(v), Zb + 5.f + Hh), R0 * 1.35f, R0 * 1.35f, 1.f, 12, BrickD, true);
		M.AddCylinder(W(XE(u), XN(v), Zb + 6.f + Hh), R0 * 0.85f, R0 * 0.85f, 2.5f, 12, Brick, true);
		M.AddCylinder(W(XE(u), XN(v), Zb + 8.5f + Hh), R0 * 1.1f, 0.2f, 3.5f, 8, Lead, true);
	};
	// Tashqi devor: 12 burchak, qalin kulrang tosh, katta kvadrat burjlar; janubiy va g'arbiy darvozalar
	const int32 Sides = 12;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * ErzR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * ErzR, cv = FMath::Sin(Am) * ErzR;
		const bool bGate = (FMath::Abs(FMath::Sin(Am) + 1.f) < 0.3f) || (FMath::Abs(FMath::Cos(Am) + 1.f) < 0.15f);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) M.AddBox(W(XE(cu), XN(cv), Z + 8.f), FVector(Len * 50.f, 160, 100), Grey, FRotator(0, Yaw, 0));
		else
		{
			M.AddBox(W(XE(cu), XN(cv), Z + 4.5f), FVector(Len * 50.f + 20.f, 160, 450), ErtCol::Vary(Grey, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
				M.AddBox(W(XE(cu - FMath::Sin(Am) * t), XN(cv + FMath::Cos(Am) * t), Z + 9.6f), FVector(60, 160, 60), ErtCol::Vary(GreyD, 0.05f, ++S), FRotator(0, Yaw, 0));
		}
		const float tu = FMath::Cos(A0) * ErzR, tv = FMath::Sin(A0) * ErzR;
		M.AddBox(W(XE(tu), XN(tv), Z + 6.f), FVector(400, 400, 600), ErtCol::Vary(Grey, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A0), 0));
		M.AddBox(W(XE(tu), XN(tv), Z + 12.4f), FVector(440, 440, 40), GreyD, FRotator(0, FMath::RadiansToDegrees(A0), 0));
		M.AddCylinder(W(XE(tu), XN(tv), Z + 12.6f), 2.6f, 0.2f, 2.5f, 4, Lead, true, FRotator(0, FMath::RadiansToDegrees(A0), 0));   // qorga qarshi qiya tom
	}
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(XE(k * 10.f), XN(-ErzR), Z + 7.f), FVector(350, 420, 700), ErtCol::Vary(Grey, 0.04f, ++S)); M.AddBox(W(XE(-ErzR), XN(k * 10.f), Z + 7.f), FVector(420, 350, 700), ErtCol::Vary(Grey, 0.04f, ++S)); }
	// Qal'a (shimoli-sharq tepaligi): ichki devor, Tepsi minora (dumaloq g'isht kuzatuv minorasi), saroy
	{
		const float HU = 45.f, HV = 45.f, CR = ErzHillR - 8.f;
		for (int32 i = 0; i < 10; ++i)
		{
			if (i == 6) continue;   // janubi-g'arbiy kirish
			const float Am = (i + 0.5f) * 2.f * PI / 10, Len = 2.f * CR * FMath::Sin(PI / 10);
			M.AddBox(W(XE(HU + FMath::Cos(Am) * CR), XN(HV + FMath::Sin(Am) * CR), TopZ + 2.5f), FVector(Len * 50.f + 20.f, 120, 300), ErtCol::Vary(GreyD, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
		}
		M.AddCylinder(W(XE(HU + 8.f), XN(HV + 8.f), TopZ), 3.2f, 2.6f, 22.f, 12, ErtCol::Vary(Brick, 0.04f, ++S), true);   // Tepsi minora
		M.AddCylinder(W(XE(HU + 8.f), XN(HV + 8.f), TopZ + 22.f), 3.4f, 3.4f, 1.2f, 12, BrickD, true);
		M.AddBox(W(XE(HU + 8.f), XN(HV + 8.f), TopZ + 25.f), FVector(200, 200, 180), XWood);                                    // soat xonasi
		M.AddCylinder(W(XE(HU + 8.f), XN(HV + 8.f), TopZ + 26.8f), 2.6f, 0.2f, 2.5f, 4, Lead, true);
		M.AddBox(W(XE(HU - 8.f), XN(HV - 4.f), TopZ + 3.5f), FVector(1000, 700, 350), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddCylinder(W(XE(HU - 8.f), XN(HV - 4.f), TopZ + 7.f), 8.f, 0.3f, 3.5f, 4, Lead, true);
		AddBanner(M, XE(HU - 22.f), XN(HV - 18.f), TopZ, 7.f, XRed, false);
		AddFire(M, XE(HU), XN(HV - 12.f), TopZ, true);
	}
	// Chifte Minorali madrasa (markaz): juda baland portal, qo'sh minora, orqada hovli va kumbet
	{
		const float MU = 0.f, MV = 8.f;
		M.AddBox(W(XE(MU), XN(MV - 16.f), Z + 9.f), FVector(1100, 250, 900), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV - 17.3f), Z + 6.5f), FVector(400, 20, 900), ErtCol::Vary(GreyD, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV - 17.5f), Z + 3.f), FVector(200, 20, 450), Grey * 0.3f);
		Minaret(MU - 8.f, MV - 16.f, Z + 18.f, 14.f, 1.3f); Minaret(MU + 8.f, MV - 16.f, Z + 18.f, 14.f, 1.3f);
		M.AddBox(W(XE(MU), XN(MV + 4.f), Z + 3.5f), FVector(1100, 1800, 350), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV + 2.f), Z + 0.2f), FVector(500, 1000, 20), FLinearColor(0.85f, 0.85f, 0.82f));
		M.AddCylinder(W(XE(MU), XN(MV + 24.f), Z), 4.5f, 4.5f, 9.f, 12, ErtCol::Vary(Grey, 0.04f, ++S), true);                  // orqadagi katta kumbet
		M.AddCylinder(W(XE(MU), XN(MV + 24.f), Z + 9.f), 5.f, 0.2f, 6.f, 12, GreyD, true);
	}
	// Uch Kumbet (janubi-sharq): uch konus tomli maqbara
	for (int32 i = 0; i < 3; ++i)
	{
		const float u = 40.f + i * 11.f, v = -55.f + (i == 1 ? 8.f : 0.f), R0 = i == 0 ? 3.8f : 3.f;
		M.AddCylinder(W(XE(u), XN(v), Z), R0, R0, 6.f, 8, ErtCol::Vary(Grey, 0.05f, ++S), true);
		M.AddCylinder(W(XE(u), XN(v), Z + 6.f), R0 * 1.1f, 0.2f, 5.f, 8, i == 1 ? Brick : GreyD, true);
	}
	// Ulu Jome' (g'arb): keng past tosh bino, ko'p gumbazli tom, kalta minora
	{
		const float MU = -55.f, MV = -10.f;
		M.AddBox(W(XE(MU), XN(MV), Z + 3.f), FVector(1500, 1800, 300), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV), Z + 6.3f), FVector(1500, 1800, 30), GreyD);
		for (int32 i = 0; i < 3; ++i) for (int32 j = 0; j < 3; ++j) M.AddSphere(W(XE(MU - 8.f + i * 8.f), XN(MV - 10.f + j * 10.f), Z + 6.f), 2.6f, 8, ErtCol::Vary(Grey, 0.05f, ++S), FVector(1, 1, 0.55f));
		M.AddCylinder(W(XE(MU + 16.f), XN(MV + 19.f), Z), 1.4f, 1.2f, 14.f, 10, ErtCol::Vary(Brick, 0.04f, ++S), true);
		M.AddCylinder(W(XE(MU + 16.f), XN(MV + 19.f), Z + 14.f), 1.5f, 0.2f, 3.f, 8, Lead, true);
	}
	// Bozor (janubiy darvozadan madrasa oldigacha): tosh do'konlar, tunuka tomlar
	for (float v = -ErzR + 16.f; v < -30.f; v += 7.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(XE(k * 9.f), XN(v), Z + 1.6f), FVector(250, 320, 160), ErtCol::Vary(Grey, 0.06f, ++S));
			M.AddCylinder(W(XE(k * 9.f), XN(v), Z + 3.2f), 2.4f, 0.2f, 1.6f, 4, Lead, true);
			M.AddBox(W(XE(k * 6.5f), XN(v), Z + 0.9f), FVector(30, 200, 90), Grey * 0.3f);
			M.AddBox(W(XE(k * 6.f), XN(v), Z + 0.8f), FVector(40, 160, 20), FLinearColor(RS.FRand() * 0.5f + 0.2f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.5f + 0.2f));
		}
	// Uylar: past, qalin tosh, tekis tuproq tom (qish uchun), mo'rilar
	for (float R = 30.f; R < ErzR - 12.f; R += 14.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 13.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(u) < 14.f && v < -20.f) continue;                                             // bozor
			if (FMath::Abs(v) < 8.f && u < 0.f) continue;                                                // g'arbiy ko'cha
			if (FMath::Abs(u) < 10.f && FMath::Abs(v - 8.f) < 28.f) continue;                            // madrasa
			if (FVector2D::Distance(FVector2D(u, v), FVector2D(45.f, 45.f)) < ErzHillR + 2.f) continue;   // qal'a
			if (FMath::Abs(u - 51.f) < 18.f && FMath::Abs(v + 52.f) < 10.f) continue;                    // kumbetlar
			if (FMath::Abs(u + 55.f) < 12.f && FMath::Abs(v + 10.f) < 14.f) continue;                    // jome'
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.5f, 6.5f), HD = RS.FRandRange(4.5f, 6.5f), HH = RS.FRand() < 0.3f ? 5.5f : 3.5f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(XE(u), XN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(Grey, 0.10f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(XE(u), XN(v), Z + HH + 0.2f), FVector(HW * 50.f + 10.f, HD * 50.f + 10.f, 20.f), ErtCol::Vary(FLinearColor(0.45f, 0.38f, 0.28f), 0.06f, ++S), FRotator(0, Yaw, 0));
			if (RS.FRand() < 0.5f) M.AddBox(W(XE(u), XN(v), Z + HH + 0.9f), FVector(50, 50, 80), GreyD);
			if (RS.FRand() < 0.4f) M.AddBox(W(XE(u), XN(v), Z + HH + 0.45f), FVector(HW * 50.f + 12.f, HD * 50.f + 12.f, 8.f), Snow, FRotator(0, Yaw, 0));   // tomdagi qor
		}
	}
	// Qarag'aylar va darvoza bayroqlari
	for (int32 i = 0; i < 14; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(25.f, ErzR - 10.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(u) < 15.f && v < -20.f) continue;
		if (FVector2D::Distance(FVector2D(u, v), FVector2D(45.f, 45.f)) < ErzHillR + 2.f) continue;
		if (FMath::Abs(u) < 10.f && FMath::Abs(v - 8.f) < 28.f) continue;
		const float ZZ = HeightAt(XE(u), XN(v));
		M.AddCylinder(W(XE(u), XN(v), ZZ), 0.35f, 0.25f, 2.5f, 6, XWood, true);
		M.AddCylinder(W(XE(u), XN(v), ZZ + 2.f), 2.4f, 0.1f, RS.FRandRange(7.f, 10.f), 6, ErtCol::Vary(Pine, 0.08f, ++S), true);
	}
	AddBanner(M, XE(-10.f), XN(-ErzR - 3.f), Z, 6.f, XRed, false);
	AddBanner(M, XE(10.f), XN(-ErzR - 3.f), Z, 6.f, XRed, false);
	M.Commit(NewPart(TEXT("Erzurum"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("Sivas"))) { PE = SivE; PN = SivN - SivR - 45.f; }   // Sivas janubiy darvozasi oldi',
        '\telse if (E.Region.Contains(TEXT("Sivas"))) { PE = SivE; PN = SivN - SivR - 45.f; }   // Sivas janubiy darvozasi oldi\n\telse if (E.Region.Contains(TEXT("Erzurum")) || E.Region.Contains(TEXT("Erzincan"))) { PE = ErzE; PN = ErzN - ErzR - 45.f; }   // Erzurum janubiy darvozasi oldi')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("sivas")) { E = ErtMap::SivE; Nn = ErtMap::SivN; }', '\t\telse if (Place == TEXT("sivas")) { E = ErtMap::SivE; Nn = ErtMap::SivN; }\n\t\telse if (Place == TEXT("erzurum")) { E = ErtMap::ErzE; Nn = ErtMap::ErzN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(SivE, SivN, SivR, TEXT("Sivas"), Ink);', '\tMark(SivE, SivN, SivR, TEXT("Sivas"), Ink);\n\tMark(ErzE, ErzN, ErzR, TEXT("Erzurum"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(41.8f)) TakeShot(TEXT("npc_choice"));\n\tif (At(42.6f))', '\tif (At(41.8f)) TakeShot(TEXT("npc_choice"));\n\tif (At(41.9f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->OnAdvance(); }\n\tif (At(42.0f)) Teleport(420.f, 560.f, 22.f + 55.f, -18.f, 0.f);\n\tif (At(42.9f)) TakeShot(TEXT("erzurum"));\n\tif (At(43.4f))')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'erzurum_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'erzurum_savdogar', 'name': 'chr.erzurum_savdogar.name', 'place': 'erzurum', 'u': -6, 'v': -60, 'yaw': 90, 'woman': False, 'kaftan': [0.3, 0.3, 0.35], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'erzurum_beyi', 'name': 'chr.erzurum_beyi.name', 'place': 'erzurum', 'u': 0, 'v': -22, 'yaw': 180, 'woman': False, 'kaftan': [0.5, 0.1, 0.1], 'dialog': 'ep015_talk'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.erzurum_savdogar.name","Erzurum savdogari","Erzurum tüccarı","Erzurum merchant"', '"chr.erzurum_beyi.name","Erzurum beyi","Erzurum beyi","Bey of Erzurum"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
