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
h = rep(h, "// Konya\n", "// Konya\n\tconstexpr float KayE = 450.f, KayN = 100.f, KayR = 125.f, KayZ = 13.f, ErcE = 470.f, ErcN = -150.f, ErcR = 130.f, ErcH = 120.f;   // Qayseri va Erciyes\n")
h = rep(h, "\tvoid BuildKonya();", "\tvoid BuildKonya();\n\tvoid BuildKayseri();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildKonya();\n\t}", "\t\tBuildKonya();\n\t\tBuildKayseri();\n\t}")
c = rep(c, "\t\t{{300, 300}, {275, 480}, 6},", "\t\t{{300, 300}, {275, 480}, 6}, {{300, 300}, {320, 110}, 6},")
c = rep(c, "\t// Konya: tekis aylana + markazda past Aloiddin tepaligi", '''\t// Erciyes: vulqon konusi (qoyali, cho'qqisi qorli) va Qayseri tekisligi
\t{
\t\tconst float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(ErcE, ErcN));
\t\tconst float T = Smooth01(1.f - DM / ErcR);
\t\tH += ErcH * T * T + Noise(E, N, 0.03f) * 6.f * T;
\t\tconst float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KayE, KayN));
\t\tH = FMath::Lerp(H, KayZ, 1.f - Smooth01((DK - KayR - 10.f) / 50.f));
\t}
\t// Konya: tekis aylana + markazda past Aloiddin tepaligi''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(ErcE, ErcN));
\t\tconst float T = Smooth01(1.f - (DM - 15.f) / (ErcR - 15.f));
\t\tC = FMath::Lerp(C, FLinearColor(0.30f, 0.29f, 0.30f), T * 0.9f);                                   // bazalt qoya
\t\tC = FMath::Lerp(C, FLinearColor(0.95f, 0.96f, 1.0f), Smooth01((H - 75.f) / 20.f) * T);            // qor
\t\tconst float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KayE, KayN));
\t\tC = FMath::Lerp(C, FLinearColor(0.42f, 0.40f, 0.38f), 1.f - Smooth01((DK - KayR) / 8.f));          // qora tosh yo'laklar
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Konya\n", "// Konya\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(KayE, KayN)) < KayR + 30.f) return false;   // Qayseri\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(ErcE, ErcN)) < ErcR * 0.8f) return false;   // Erciyes\n")
c += r'''
// ---------------- Qayseri: bazalt shahar, Erciyes etagida ----------------

void AErtWorldBuilder::BuildKayseri()
{
	FRandomStream RS(Seed + 131);
	FErtMeshData M(100.f);
	int32 S = 4500;
	const float Z = KayZ;
	auto QE = [](float u) { return KayE + u; };
	auto QN = [](float v) { return KayN + v; };
	const FLinearColor Basalt(0.26f, 0.25f, 0.26f), BasaltL(0.40f, 0.38f, 0.37f), Mortar(0.62f, 0.58f, 0.52f), QWood(0.38f, 0.25f, 0.12f), Lead(0.45f, 0.47f, 0.52f), Turq(0.12f, 0.58f, 0.62f), QRed(0.55f, 0.10f, 0.08f), Poplar(0.30f, 0.50f, 0.20f);
	// Tashqi devor: 20 burchak, qora bazalt, kungurali, yarim dumaloq burjlar; g'arbiy va shimoliy darvozalar
	const int32 Sides = 20;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * KayR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * KayR, cv = FMath::Sin(Am) * KayR;
		const bool bGate = (i == Sides / 2 || i == Sides / 2 - 1) || (i == Sides / 4 || i == Sides / 4 - 1);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) M.AddBox(W(QE(cu), QN(cv), Z + 7.5f), FVector(Len * 50.f, 130, 90), Basalt, FRotator(0, Yaw, 0));
		else
		{
			M.AddBox(W(QE(cu), QN(cv), Z + 4.f), FVector(Len * 50.f + 20.f, 130, 400), ErtCol::Vary(Basalt, 0.05f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(QE(cu), QN(cv), Z + 4.f), FVector(Len * 50.f + 22.f, 132, 12), Mortar, FRotator(0, Yaw, 0));   // oq chok
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
				M.AddBox(W(QE(cu - FMath::Sin(Am) * t), QN(cv + FMath::Cos(Am) * t), Z + 8.6f), FVector(60, 130, 60), ErtCol::Vary(BasaltL, 0.05f, ++S), FRotator(0, Yaw, 0));
		}
		if (i % 2 == 0)
		{
			const float tu = FMath::Cos(A0) * KayR, tv = FMath::Sin(A0) * KayR;
			M.AddCylinder(W(QE(tu), QN(tv), Z), 4.f, 3.7f, 12.f, 10, ErtCol::Vary(Basalt, 0.04f, ++S), true);
			M.AddCylinder(W(QE(tu), QN(tv), Z + 12.f), 4.3f, 4.3f, 1.f, 10, BasaltL, true);
		}
	}
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(QE(-KayR), QN(k * 9.f), Z + 6.5f), FVector(400, 320, 650), ErtCol::Vary(Basalt, 0.04f, ++S)); M.AddBox(W(QE(k * 9.f), QN(KayR), Z + 6.5f), FVector(320, 400, 650), ErtCol::Vary(Basalt, 0.04f, ++S)); }
	// Ichki qo'rg'on (qal'a): to'rtburchak 70x50, 8 burj, janubiy darvoza, ichida saroy
	{
		const float CU = 20.f, CV = 25.f, HW = 35.f, HD = 25.f;
		for (int32 side = 0; side < 4; ++side)
		{
			const bool bEW = side < 2; const float Sg = (side & 1) ? 1.f : -1.f;
			const float u = bEW ? CU : CU + Sg * HW, v = bEW ? CV + Sg * HD : CV;
			M.AddBox(W(QE(u), QN(v), Z + 5.f), FVector(bEW ? HW * 100.f : 120.f, bEW ? 120.f : HD * 100.f, 500), ErtCol::Vary(Basalt, 0.04f, ++S));
			M.AddBox(W(QE(u), QN(v), Z + 10.4f), FVector(bEW ? HW * 100.f : 160.f, bEW ? 160.f : HD * 100.f, 40), BasaltL);
		}
		for (int32 i = 0; i < 8; ++i)
		{
			const float u = CU + ((i & 1) ? HW : -HW) * ((i < 4) ? 1.f : 0.5f), v = CV + ((i & 2) ? HD : -HD) * ((i < 4) ? 1.f : ((i & 1) ? 1.f : 1.f));
			M.AddCylinder(W(QE(u), QN(v), Z), 4.5f, 4.f, 14.f, 10, ErtCol::Vary(Basalt, 0.04f, ++S), true);
			M.AddCylinder(W(QE(u), QN(v), Z + 14.f), 4.9f, 4.9f, 1.f, 10, BasaltL, true);
		}
		M.AddBox(W(QE(CU), QN(CV - HD), Z + 5.f), FVector(300, 300, 80), Basalt * 1.4f);   // darvoza yo'lagi usti
		M.AddBox(W(QE(CU - 5.f), QN(CV + 5.f), Z + 4.f), FVector(1600, 1000, 400), ErtCol::Vary(BasaltL, 0.03f, ++S));   // saroy
		M.AddBox(W(QE(CU - 5.f), QN(CV + 5.f), Z + 9.5f), FVector(900, 600, 150), ErtCol::Vary(BasaltL, 0.03f, ++S));
		M.AddSphere(W(QE(CU - 5.f), QN(CV + 5.f), Z + 11.f), 4.5f, 12, Lead, FVector(1, 1, 0.7f));
		AddBanner(M, QE(CU - 8.f), QN(CV - HD + 3.f), Z, 7.f, QRed, false);
		AddBanner(M, QE(CU + 8.f), QN(CV - HD + 3.f), Z, 7.f, QRed, false);
		AddFire(M, QE(CU + 12.f), QN(CV + 8.f), Z, true);
	}
	// Hunat Xotun majmuasi (janubi-g'arb): masjid (qo'rg'oshin gumbaz, minora), madrasa (hovli, portal), sakkiz qirrali kumbet
	{
		const float MU = -55.f, MV = -45.f;
		M.AddBox(W(QE(MU), QN(MV), Z + 4.5f), FVector(2000, 1400, 450), ErtCol::Vary(BasaltL, 0.03f, ++S));
		M.AddBox(W(QE(MU), QN(MV), Z + 9.2f), FVector(2000, 1400, 20), Mortar);
		M.AddBox(W(QE(MU), QN(MV), Z + 9.5f), FVector(700, 700, 60), BasaltL);
		M.AddSphere(W(QE(MU), QN(MV), Z + 10.f), 7.f, 14, Lead, FVector(1, 1, 0.75f));
		M.AddCylinder(W(QE(MU + 22.f), QN(MV + 15.f), Z), 1.8f, 1.8f, 7.f, 8, BasaltL, true);
		M.AddCylinder(W(QE(MU + 22.f), QN(MV + 15.f), Z + 7.f), 1.3f, 1.1f, 20.f, 12, ErtCol::Vary(Basalt, 0.03f, ++S), true);
		M.AddCylinder(W(QE(MU + 22.f), QN(MV + 15.f), Z + 20.f), 1.8f, 1.8f, 0.8f, 12, Mortar, true);
		M.AddCylinder(W(QE(MU + 22.f), QN(MV + 15.f), Z + 27.f), 1.3f, 0.3f, 3.5f, 8, Lead, true);
		M.AddBox(W(QE(MU - 2.f), QN(MV - 22.f), Z + 3.f), FVector(1600, 800, 300), ErtCol::Vary(BasaltL, 0.03f, ++S));   // madrasa
		M.AddBox(W(QE(MU - 2.f), QN(MV - 22.f), Z + 0.2f), FVector(800, 400, 20), Mortar);
		M.AddBox(W(QE(MU - 2.f), QN(MV - 13.6f), Z + 5.5f), FVector(500, 120, 550), Basalt);
		M.AddCylinder(W(QE(MU + 24.f), QN(MV - 20.f), Z), 3.5f, 3.5f, 8.f, 8, ErtCol::Vary(BasaltL, 0.04f, ++S), true);   // kumbet
		M.AddCylinder(W(QE(MU + 24.f), QN(MV - 20.f), Z + 8.f), 3.9f, 0.2f, 5.f, 8, Basalt, true);
	}
	// Yopiq bozor (bedesten): g'arbiy darvozadan qo'rg'ongacha uzun gumbazli bino
	for (float u = -KayR + 16.f; u < -22.f; u += 8.f)
	{
		M.AddBox(W(QE(u), QN(0.f), Z + 2.5f), FVector(400, 1300, 250), ErtCol::Vary(BasaltL, 0.05f, ++S));
		M.AddBox(W(QE(u), QN(0.f), Z + 5.1f), FVector(400, 1300, 15), Mortar);
		for (int32 k = -1; k <= 1; ++k) M.AddSphere(W(QE(u), QN(k * 4.2f), Z + 5.f), 2.2f, 8, ErtCol::Vary(Lead, 0.05f, ++S), FVector(1, 1, 0.6f));
		M.AddBox(W(QE(u), QN(-6.6f), Z + 1.5f), FVector(150, 20, 220), Basalt * 0.5f);   // eshiklar
		M.AddBox(W(QE(u), QN(6.6f), Z + 1.5f), FVector(150, 20, 220), Basalt * 0.5f);
	}
	// Uylar: qora toshli, tekis tomli, oq choklar; radial ko'chalar
	for (float R = 40.f; R < KayR - 12.f; R += 15.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 14.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(v) < 11.f && u < 0.f) continue;                                             // bozor ko'chasi
			if (FMath::Abs(u) < 7.f && v > 0.f) continue;                                              // shimoliy yo'lak
			if (FMath::Abs(u - 20.f) < 44.f && FMath::Abs(v - 25.f) < 34.f) continue;                  // qo'rg'on
			if (FMath::Abs(u + 55.f) < 32.f && FMath::Abs(v + 45.f) < 30.f) continue;                  // majmua
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.4f ? 6.5f : 4.f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(QE(u), QN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(BasaltL, 0.10f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(QE(u), QN(v), Z + HH * 0.5f), FVector(HW * 50.f + 2.f, HD * 50.f + 2.f, 8.f), Mortar, FRotator(0, Yaw, 0));
			M.AddBox(W(QE(u), QN(v), Z + HH + 0.2f), FVector(HW * 50.f + 10.f, HD * 50.f + 10.f, 20.f), ErtCol::Vary(Basalt, 0.06f, ++S), FRotator(0, Yaw, 0));
			if (RS.FRand() < 0.3f) M.AddBox(W(QE(u), QN(v), Z + HH + 1.f), FVector(60, 60, 90), BasaltL);   // mo'ri
		}
	}
	// Teraklar (baland ingichka) va darvoza bayroqlari
	for (int32 i = 0; i < 22; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(30.f, KayR - 10.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(v) < 12.f && u < 0.f) continue;
		if (FMath::Abs(u - 20.f) < 44.f && FMath::Abs(v - 25.f) < 34.f) continue;
		M.AddCylinder(W(QE(u), QN(v), Z), 0.3f, 0.2f, 3.f, 6, QWood, true);
		M.AddCylinder(W(QE(u), QN(v), Z + 2.5f), 1.6f, 0.2f, RS.FRandRange(9.f, 13.f), 6, ErtCol::Vary(Poplar, 0.08f, ++S), true);
	}
	AddBanner(M, QE(-KayR - 3.f), QN(-9.f), Z, 6.f, QRed, false);
	AddBanner(M, QE(-KayR - 3.f), QN(9.f), Z, 6.f, QRed, false);
	M.Commit(NewPart(TEXT("Kayseri"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("Konya")) || E.Region.Contains(TEXT("Kubadabad")) || E.Region.Contains(TEXT("Kayseri"))) { PE = KonE + KonR + 45.f; PN = KonN; }   // Konya sharqiy darvozasi oldi',
        '\telse if (E.Region.Contains(TEXT("Konya")) || E.Region.Contains(TEXT("Kubadabad"))) { PE = KonE + KonR + 45.f; PN = KonN; }   // Konya sharqiy darvozasi oldi\n\telse if (E.Region.Contains(TEXT("Kayseri"))) { PE = KayE - KayR - 45.f; PN = KayN; }   // Qayseri g\'arbiy darvozasi oldi')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("konya")) { E = ErtMap::KonE; Nn = ErtMap::KonN; }', '\t\telse if (Place == TEXT("konya")) { E = ErtMap::KonE; Nn = ErtMap::KonN; }\n\t\telse if (Place == TEXT("kayseri")) { E = ErtMap::KayE; Nn = ErtMap::KayN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(KonE, KonN, KonR, TEXT("Konya"), Ink);', '\tMark(KonE, KonN, KonR, TEXT("Konya"), Ink);\n\tMark(KayE, KayN, KayR, TEXT("Qayseri"), Ink);\n\tMark(ErcE, ErcN, 40.f, TEXT("Erciyes"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
for a, b in [('At(27.3f)) TakeShot(TEXT("damascus"))', 'At(27.2f)) TakeShot(TEXT("damascus"))'), ('At(27.32f)) Teleport(965', 'At(27.22f)) Teleport(965'), ('At(28.0f)) TakeShot(TEXT("halab"))', 'At(27.8f)) TakeShot(TEXT("halab"))'),
             ('At(28.02f)) Teleport(310', 'At(27.82f)) Teleport(310'), ('At(28.7f)) TakeShot(TEXT("konya"))', 'At(28.4f)) TakeShot(TEXT("konya"))\n\tif (At(28.42f)) Teleport(260.f, 100.f, 13.f + 55.f, -18.f, 90.f);\n\tif (At(28.98f)) TakeShot(TEXT("kayseri"))')]:
    c = rep(c, a, b)
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'kayseri_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'kayseri_savdogar', 'name': 'chr.kayseri_savdogar.name', 'place': 'kayseri', 'u': -30, 'v': 9, 'yaw': 270, 'woman': False, 'kaftan': [0.35, 0.2, 0.15], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'kayseri_hokimi', 'name': 'chr.kayseri_hokimi.name', 'place': 'kayseri', 'u': 20, 'v': -6, 'yaw': 180, 'woman': False, 'kaftan': [0.5, 0.1, 0.1], 'dialog': 'ep013_talk'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.kayseri_savdogar.name","Qayseri savdogari","Kayseri tüccarı","Kayseri merchant"', '"chr.kayseri_hokimi.name","Qayseri subashisi","Kayseri subaşısı","Subashi of Kayseri"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
