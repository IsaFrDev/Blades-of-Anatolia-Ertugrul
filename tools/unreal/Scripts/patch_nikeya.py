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
h = rep(h, "// Bursa va Uludog'\n", "// Bursa va Uludog'\n\tconstexpr float NikE = -300.f, NikN = -60.f, NikR = 110.f, NikZ = 5.5f, AskE = -300.f, AskN = 95.f, AskR = 65.f, AskZ = 4.5f;   // Nikeya va Askaniya ko'li\n")
h = rep(h, "\tvoid BuildBursa();", "\tvoid BuildBursa();\n\tvoid BuildNikeya();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildBursa();\n\t}", "\t\tBuildBursa();\n\t\tBuildNikeya();\n\t}")
c = rep(c, "{{-300, 250}, {-420, 65}, 6},", "{{-300, 250}, {-420, 65}, 6}, {{-150, -150}, {-190, -60}, 6},")
c = rep(c, "\t// Uludog': o'rmonli tog'", '''\t// Nikeya: tekis aylana; Askaniya ko'li: qirg'oq tekis, o'rtasi chuqur
\t{
\t\tconst float DN = FVector2D::Distance(FVector2D(E, N), FVector2D(NikE, NikN));
\t\tH = FMath::Lerp(H, NikZ, 1.f - Smooth01((DN - NikR - 10.f) / 45.f));
\t\tconst float DA = FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN));
\t\tH = FMath::Lerp(H, AskZ + 1.f, 1.f - Smooth01((DA - (AskR + 15.f)) / 35.f));
\t\tif (DA < AskR) H -= 5.f * Smooth01(1.f - DA / AskR);
\t}
\t// Uludog': o'rmonli tog\'''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DN = FVector2D::Distance(FVector2D(E, N), FVector2D(NikE, NikN));
\t\tC = FMath::Lerp(C, FLinearColor(0.66f, 0.60f, 0.52f), 1.f - Smooth01((DN - NikR) / 8.f));
\t\tconst float DA = FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN));
\t\tC = FMath::Lerp(C, FLinearColor(0.76f, 0.70f, 0.52f), 1.f - Smooth01((DA - AskR - 2.f) / 10.f));   // qumloq qirg'oq
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "\tDisk(LakeE, LakeN, LakeR + 6.f, LakeZ);", "\tDisk(LakeE, LakeN, LakeR + 6.f, LakeZ);\n\tDisk(AskE, AskN, AskR + 6.f, AskZ);")
c = rep(c, "\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN)) < LakeR + 12.f) return false; // ko'l",
        "\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN)) < LakeR + 12.f) return false; // ko'l\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN)) < AskR + 12.f) return false;     // Askaniya ko'li\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(NikE, NikN)) < NikR + 30.f) return false;     // Nikeya")
c = rep(c, "\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN)) < LakeR + 4.f) { SurfZ = LakeZ; return true; }",
        "\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN)) < LakeR + 4.f) { SurfZ = LakeZ; return true; }\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN)) < AskR + 4.f) { SurfZ = AskZ; return true; }")
c += r'''
// ---------------- Nikeya: Vizantiya shahri, Askaniya ko'li bo'yida ----------------

void AErtWorldBuilder::BuildNikeya()
{
	FRandomStream RS(Seed + 211);
	FErtMeshData M(100.f);
	int32 S = 8500;
	const float Z = NikZ;
	auto NE = [](float u) { return NikE + u; };
	auto NN = [](float v) { return NikN + v; };
	const FLinearColor NStone(0.70f, 0.66f, 0.58f), NStoneD(0.55f, 0.52f, 0.46f), Brick(0.60f, 0.32f, 0.22f), Marble(0.90f, 0.88f, 0.84f), Lead(0.45f, 0.47f, 0.52f), Tile(0.62f, 0.32f, 0.20f), NWood(0.38f, 0.25f, 0.12f), Purple(0.35f, 0.10f, 0.35f), GoldC(0.9f, 0.75f, 0.25f), Cyp(0.10f, 0.30f, 0.14f);
	// Vizantiya devori: g'isht qatorli tosh (band), dumaloq burjlar; qo'sh devor (tashqi past, ichki baland); 4 darvoza (N ko'l, S, E, W)
	auto Ring = [&](float R, float Hh, float Thick, int32 Sides, bool bTowers, bool bGates)
	{
		for (int32 i = 0; i < Sides; ++i)
		{
			const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
			const float Len = 2.f * R * FMath::Sin(PI / Sides);
			const float cu = FMath::Cos(Am) * R, cv = FMath::Sin(Am) * R;
			const bool bGate = bGates && (FMath::Abs(FMath::Sin(Am)) < 0.09f || FMath::Abs(FMath::Cos(Am)) < 0.09f);
			const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
			if (bGate) { M.AddBox(W(NE(cu), NN(cv), Z + Hh - 0.5f), FVector(Len * 50.f, Thick * 50.f, 100), NStone, FRotator(0, Yaw, 0)); continue; }
			M.AddBox(W(NE(cu), NN(cv), Z + Hh * 0.5f), FVector(Len * 50.f + 20.f, Thick * 50.f, Hh * 50.f), ErtCol::Vary(NStone, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float bz = 1.5f; bz < Hh - 0.5f; bz += 2.5f) M.AddBox(W(NE(cu), NN(cv), Z + bz), FVector(Len * 50.f + 22.f, Thick * 50.f + 2.f, 25.f), ErtCol::Vary(Brick, 0.05f, ++S), FRotator(0, Yaw, 0));   // g'isht qatori
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.4f)
				M.AddBox(W(NE(cu - FMath::Sin(Am) * t), NN(cv + FMath::Cos(Am) * t), Z + Hh + 0.5f), FVector(55, Thick * 50.f, 50), ErtCol::Vary(NStoneD, 0.05f, ++S), FRotator(0, Yaw, 0));
			if (bTowers)
			{
				const float tu = FMath::Cos(A0) * R, tv = FMath::Sin(A0) * R;
				M.AddCylinder(W(NE(tu), NN(tv), Z), 4.2f, 3.9f, Hh + 3.f, 12, ErtCol::Vary(NStone, 0.04f, ++S), true);
				for (float bz = 2.f; bz < Hh + 2.f; bz += 2.5f) M.AddCylinder(W(NE(tu), NN(tv), Z + bz), 4.25f, 4.25f, 0.25f, 12, ErtCol::Vary(Brick, 0.05f, ++S), true);
				M.AddCylinder(W(NE(tu), NN(tv), Z + Hh + 3.f), 4.5f, 4.5f, 0.8f, 12, NStoneD, true);
			}
		}
	};
	Ring(NikR, 9.f, 2.6f, 28, true, true);
	Ring(NikR + 12.f, 3.5f, 1.6f, 28, false, true);
	// Darvoza minoralari (Rim uslubidagi ark)
	for (int32 g = 0; g < 4; ++g)
	{
		const float A = g * PI * 0.5f, gu = FMath::Cos(A) * NikR, gv = FMath::Sin(A) * NikR;
		const float Yaw = FMath::RadiansToDegrees(A);
		for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(NE(gu - FMath::Sin(A) * k * 8.f), NN(gv + FMath::Cos(A) * k * 8.f), Z + 6.5f), FVector(350, 350, 650), ErtCol::Vary(NStone, 0.04f, ++S), FRotator(0, Yaw, 0));
		M.AddBox(W(NE(gu), NN(gv), Z + 11.f), FVector(300, 1900, 150), ErtCol::Vary(Brick, 0.04f, ++S), FRotator(0, Yaw, 0));
		M.AddCylinder(W(NE(gu), NN(gv), Z + 6.f), 5.f, 5.f, 3.f, 12, NStone * 0.35f, true, FRotator(90.f, Yaw, 0));   // ark (qorong'i yo'lak)
	}
	// Ayo Sofiya bazilikasi (markaz): uzun nef, apsida (yarim silindr), markaziy gumbaz, sopol tom
	{
		const float BU = 0.f, BV = 5.f;
		M.AddBox(W(NE(BU), NN(BV), Z + 5.f), FVector(1000, 2200, 500), ErtCol::Vary(NStone, 0.03f, ++S));
		for (float bz = 1.5f; bz < 9.5f; bz += 2.5f) M.AddBox(W(NE(BU), NN(BV), Z + bz), FVector(1002, 2202, 25), ErtCol::Vary(Brick, 0.05f, ++S));
		M.AddCylinder(W(NE(BU), NN(BV), Z + 10.f), 14.f, 0.5f, 3.f, 4, Tile, true, FRotator(0, 0, 0));          // tom
		M.AddCylinder(W(NE(BU), NN(BV + 22.f), Z), 5.f, 5.f, 8.f, 10, ErtCol::Vary(NStone, 0.03f, ++S), true);     // apsida
		M.AddSphere(W(NE(BU), NN(BV + 22.f), Z + 8.f), 5.f, 10, Tile, FVector(1, 1, 0.5f));
		M.AddBox(W(NE(BU), NN(BV), Z + 11.f), FVector(500, 500, 100), NStone);
		M.AddSphere(W(NE(BU), NN(BV), Z + 12.f), 6.f, 14, Lead, FVector(1, 1, 0.7f));
		M.AddBox(W(NE(BU), NN(BV), Z + 16.6f), FVector(20, 20, 120), GoldC); M.AddBox(W(NE(BU), NN(BV), Z + 17.2f), FVector(20, 60, 15), GoldC);   // xoch
		for (int32 k = -1; k <= 1; k += 2) for (int32 i = 0; i < 6; ++i) M.AddCylinder(W(NE(BU + k * 12.f), NN(BV - 18.f + i * 7.f), Z), 0.6f, 0.6f, 5.f, 8, Marble, true);   // yon ustunlar
		M.AddBox(W(NE(BU), NN(BV - 22.5f), Z + 3.f), FVector(20, 600, 400), NStone * 0.9f);
		M.AddBox(W(NE(BU), NN(BV - 22.7f), Z + 2.f), FVector(20, 200, 350), NStone * 0.3f);
	}
	// Agora (bazilika g'arbida): marmar ustunli maydon, favvora
	{
		const float AU = -45.f, AV = 0.f;
		M.AddBox(W(NE(AU), NN(AV), Z + 0.1f), FVector(1800, 1800, 10), Marble);
		for (int32 i = 0; i < 8; ++i) for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddCylinder(W(NE(AU - 16.f + i * 4.6f), NN(AV + k * 16.f), Z), 0.5f, 0.5f, 5.f, 8, Marble, true);
			M.AddCylinder(W(NE(AU + k * 16.f), NN(AV - 16.f + i * 4.6f), Z), 0.5f, 0.5f, 5.f, 8, Marble, true);
		}
		M.AddBox(W(NE(AU), NN(AV + 16.f), Z + 5.4f), FVector(1750, 120, 30), NStone); M.AddBox(W(NE(AU), NN(AV - 16.f), Z + 5.4f), FVector(1750, 120, 30), NStone);
		M.AddBox(W(NE(AU + 16.f), NN(AV), Z + 5.4f), FVector(120, 1750, 30), NStone); M.AddBox(W(NE(AU - 16.f), NN(AV), Z + 5.4f), FVector(120, 1750, 30), NStone);
		M.AddCylinder(W(NE(AU), NN(AV), Z), 3.f, 3.f, 0.8f, 12, Marble, true);
		M.AddCylinder(W(NE(AU), NN(AV), Z + 0.8f), 0.5f, 0.4f, 2.5f, 8, Marble, true);
		M.AddCylinder(W(NE(AU), NN(AV), Z + 3.3f), 1.6f, 1.6f, 0.3f, 12, Marble, true);
	}
	// Rim teatri (janubi-g'arb): yarim aylana zinapoya qatorlari, sahna devori (qisman xaroba)
	{
		const float TU = -50.f, TV = -60.f;
		for (int32 r = 0; r < 8; ++r)
		{
			const float R = 8.f + r * 2.6f;
			const int32 Cnt = FMath::RoundToInt(PI * R / 2.4f);
			for (int32 i = 0; i <= Cnt; ++i)
			{
				const float A = PI * 0.5f + PI * i / Cnt;   // shimolga ochiq yarim aylana? sahna janubda
				if (RS.FRand() < 0.06f) continue;   // yemirilgan joylar
				M.AddBox(W(NE(TU + FMath::Cos(A) * R), NN(TV + FMath::Sin(A) * R), Z + 0.4f + r * 0.8f), FVector(120, 130, 40 + r * 40.f), ErtCol::Vary(NStoneD, 0.06f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0));
			}
		}
		M.AddBox(W(NE(TU), NN(TV - 10.f), Z + 3.f), FVector(1400, 150, 300), ErtCol::Vary(NStone, 0.05f, ++S));   // sahna devori
		for (int32 i = 0; i < 5; ++i) M.AddCylinder(W(NE(TU - 12.f + i * 6.f), NN(TV - 8.f), Z), 0.5f, 0.5f, i == 2 ? 3.f : 5.f, 8, Marble, true);
	}
	// Ko'l darvozasi oldida iskala va qayiqlar
	{
		for (int32 i = 0; i < 6; ++i) M.AddBox(W(NE(0.f), NN(NikR + 14.f + i * 6.f), AskZ + 0.9f), FVector(600, 150, 12), ErtCol::Vary(NWood, 0.08f, ++S));
		for (int32 i = 0; i < 6; ++i) for (int32 k = -1; k <= 1; k += 2) M.AddCylinder(W(NE(k * 2.6f), NN(NikR + 14.f + i * 6.f), AskZ - 1.5f), 0.25f, 0.25f, 2.5f, 6, NWood, true);
	}
	// Uylar: to'g'ri burchakli Rim ko'chalari, g'isht-tosh uylar, sopol tom, hovlilar
	for (float u = -NikR + 16.f; u < NikR - 12.f; u += 14.f)
		for (float v = -NikR + 16.f; v < NikR - 12.f; v += 14.f)
		{
			if (FVector2D(u, v).Size() > NikR - 14.f) continue;
			if (FMath::Abs(u) < 8.f || FMath::Abs(v) < 8.f) continue;                                  // xoch ko'chalar (cardo/decumanus)
			if (FMath::Abs(u) < 16.f && FMath::Abs(v - 5.f) < 32.f) continue;                          // bazilika
			if (FMath::Abs(u + 45.f) < 20.f && FMath::Abs(v) < 20.f) continue;                         // agora
			if (FMath::Abs(u + 50.f) < 30.f && FMath::Abs(v + 60.f) < 30.f) continue;                  // teatr
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.5f, 6.f), HD = RS.FRandRange(4.5f, 6.f), HH = RS.FRand() < 0.4f ? 6.5f : 4.f;
			const float E = NE(u + RS.FRandRange(-1.5f, 1.5f)), N = NN(v + RS.FRandRange(-1.5f, 1.5f));
			M.AddBox(W(E, N, Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(RS.FRand() < 0.4f ? Brick : NStone, 0.08f, ++S));
			M.AddBox(W(E, N, Z + 1.2f), FVector(HW * 50.f + 2.f, HD * 50.f + 2.f, 20.f), ErtCol::Vary(Brick, 0.05f, ++S));
			M.AddCylinder(W(E, N, Z + HH), FMath::Max(HW, HD) * 0.8f, 0.3f, 2.f, 4, ErtCol::Vary(Tile, 0.06f, ++S), true, FRotator(0, 45.f, 0));
		}
	// Sarvlar
	for (int32 i = 0; i < 18; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(20.f, NikR - 14.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(u) < 16.f && FMath::Abs(v - 5.f) < 32.f) continue;
		if (FMath::Abs(u + 50.f) < 30.f && FMath::Abs(v + 60.f) < 30.f) continue;
		M.AddCylinder(W(NE(u), NN(v), Z), 0.3f, 0.2f, 2.f, 6, NWood, true);
		M.AddCylinder(W(NE(u), NN(v), Z + 1.5f), 1.4f, 0.1f, RS.FRandRange(8.f, 12.f), 6, ErtCol::Vary(Cyp, 0.08f, ++S), true);
	}
	AddBanner(M, NE(NikR + 16.f), NN(-9.f), Z, 6.f, Purple, false);
	AddBanner(M, NE(NikR + 16.f), NN(9.f), Z, 6.f, Purple, false);
	M.Commit(NewPart(TEXT("Nikeya"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("Bursa")) || E.Region.Contains(TEXT("Nikeya"))) { PE = BurE + BurR + 45.f; PN = BurN; }   // Bursa sharqiy kirishi',
        '\telse if (E.Region.Contains(TEXT("Bursa"))) { PE = BurE + BurR + 45.f; PN = BurN; }   // Bursa sharqiy kirishi\n\telse if (E.Region.Contains(TEXT("Nikeya"))) { PE = NikE + NikR + 55.f; PN = NikN; }   // Nikeya sharqiy darvozasi oldi')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("bursa")) { E = ErtMap::BurE; Nn = ErtMap::BurN; }', '\t\telse if (Place == TEXT("bursa")) { E = ErtMap::BurE; Nn = ErtMap::BurN; }\n\t\telse if (Place == TEXT("nikeya")) { E = ErtMap::NikE; Nn = ErtMap::NikN; }')
c = rep(c, "\t\t\tGetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector((ErtMap::LakeN - 20.f) * 100.f, (ErtMap::LakeE + 10.f) * 100.f, ErtMap::LakeZ * 100.f + 10.f), FRotator(0, 45.f, 0), SP);",
        "\t\t\tGetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector((ErtMap::LakeN - 20.f) * 100.f, (ErtMap::LakeE + 10.f) * 100.f, ErtMap::LakeZ * 100.f + 10.f), FRotator(0, 45.f, 0), SP);\n\t\t\tGetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector((ErtMap::AskN - ErtMap::AskR + 14.f) * 100.f, (ErtMap::AskE + 8.f) * 100.f, ErtMap::AskZ * 100.f + 10.f), FRotator(0, 0, 0), SP);\n\t\t\tGetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector((ErtMap::AskN - 10.f) * 100.f, (ErtMap::AskE - 30.f) * 100.f, ErtMap::AskZ * 100.f + 10.f), FRotator(0, 120.f, 0), SP);")
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(BurE, BurN, BurR, TEXT("Bursa"), Ink);', '\tMark(BurE, BurN, BurR, TEXT("Bursa"), Ink);\n\tMark(NikE, NikN, NikR, TEXT("Nikeya"), Ink);\n\tMark(AskE, AskN, AskR, TEXT("Askaniya ko\'li"), FLinearColor(0.2f, 0.4f, 0.7f));')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(44.9f)) TakeShot(TEXT("horse"));\n\tif (At(45.4f))', '\tif (At(44.9f)) TakeShot(TEXT("horse"));\n\tif (At(45.0f)) Teleport(-300.f, -240.f, 5.5f + 60.f, -20.f, 0.f);\n\tif (At(45.9f)) TakeShot(TEXT("nikeya"));\n\tif (At(46.4f))')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'nikeya_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'nikeya_savdogar', 'name': 'chr.nikeya_savdogar.name', 'place': 'nikeya', 'u': -45, 'v': -20, 'yaw': 0, 'woman': False, 'kaftan': [0.25, 0.25, 0.5], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'nikeya_tekfur', 'name': 'chr.nikeya_tekfur.name', 'place': 'nikeya', 'u': 0, 'v': -26, 'yaw': 180, 'woman': False, 'kaftan': [0.35, 0.1, 0.35], 'dialog': 'ep017_talk'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.nikeya_savdogar.name","Nikeya savdogari","İznik tüccarı","Nicaea merchant"', '"chr.nikeya_tekfur.name","Nikeya tekfuri","İznik tekfuru","Tekfur of Nicaea"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
