# -*- coding: utf-8 -*-
import io, json, re
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# ================= BURSA =================
h = load('ErtWorldBuilder.h')
h = rep(h, "// Erzurum (qal'a tepaligi shimoli-sharqda)\n", "// Erzurum (qal'a tepaligi shimoli-sharqda)\n\tconstexpr float BurE = -540.f, BurN = 60.f, BurR = 120.f, BurZ = 9.f, BurHillR = 45.f, BurHillH = 12.f, UluE = -620.f, UluN = -200.f, UluR = 110.f, UluH = 95.f;   // Bursa va Uludog'\n")
h = rep(h, "\tvoid BuildErzurum();", "\tvoid BuildErzurum();\n\tvoid BuildBursa();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildErzurum();\n\t}", "\t\tBuildErzurum();\n\t\tBuildBursa();\n\t}")
c = rep(c, "{{500, 520}, {420, 615}, 6},", "{{500, 520}, {420, 615}, 6}, {{-300, 250}, {-420, 65}, 6},")
c = rep(c, "\t// Erzurum: baland tekis aylana + shimoli-sharqda qal'a tepaligi", '''\t// Uludog': o'rmonli tog' (cho'qqisi qorli) va Bursa tekisligi + shimolda Hisor tepaligi
\t{
\t\tconst float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(UluE, UluN));
\t\tconst float T = Smooth01(1.f - DM / UluR);
\t\tH += UluH * T * T + Noise(E, N, 0.025f) * 7.f * T;
\t\tconst float DB = FVector2D::Distance(FVector2D(E, N), FVector2D(BurE, BurN));
\t\tH = FMath::Lerp(H, BurZ, 1.f - Smooth01((DB - BurR - 10.f) / 50.f));
\t\tconst float DH = FVector2D::Distance(FVector2D(E, N), FVector2D(BurE - 30.f, BurN + 55.f));
\t\tH += BurHillH * Smooth01(1.f - (DH - 8.f) / (BurHillR - 8.f));
\t}
\t// Erzurum: baland tekis aylana + shimoli-sharqda qal'a tepaligi''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(UluE, UluN));
\t\tconst float T = Smooth01(1.f - (DM - 10.f) / (UluR - 10.f));
\t\tC = FMath::Lerp(C, FLinearColor(0.20f, 0.36f, 0.16f), T * 0.7f);                                  // o'rmon yashili
\t\tC = FMath::Lerp(C, FLinearColor(0.95f, 0.96f, 1.0f), Smooth01((H - 62.f) / 18.f) * T);            // qor
\t\tconst float DB = FVector2D::Distance(FVector2D(E, N), FVector2D(BurE, BurN));
\t\tC = FMath::Lerp(C, FLinearColor(0.62f, 0.58f, 0.50f), 1.f - Smooth01((DB - BurR) / 8.f));
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Erzurum\n", "// Erzurum\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(BurE, BurN)) < BurR + 30.f) return false;   // Bursa\n")
c += r'''
// ---------------- Bursa: Uludog' etagidagi yashil shahar ----------------

void AErtWorldBuilder::BuildBursa()
{
	FRandomStream RS(Seed + 197);
	FErtMeshData M(100.f);
	int32 S = 7500;
	const float Z = BurZ, TopZ = BurZ + BurHillH;
	auto BE = [](float u) { return BurE + u; };
	auto BN = [](float v) { return BurN + v; };
	const FLinearColor BStone(0.80f, 0.74f, 0.62f), BStoneD(0.62f, 0.56f, 0.46f), Lead(0.45f, 0.47f, 0.52f), Turq(0.12f, 0.58f, 0.62f), TurqD(0.08f, 0.42f, 0.48f), Tile(0.62f, 0.30f, 0.18f), BWood(0.38f, 0.25f, 0.12f), White(0.92f, 0.90f, 0.85f), Plane(0.22f, 0.45f, 0.18f), Trunk(0.55f, 0.50f, 0.42f), BRed(0.55f, 0.10f, 0.08f), Steam(0.85f, 0.88f, 0.9f);
	auto Minaret = [&](float u, float v, float Zb, float Hh)
	{
		M.AddBox(W(BE(u), BN(v), Zb + 2.f), FVector(160, 160, 200), BStone);
		M.AddCylinder(W(BE(u), BN(v), Zb + 4.f), 1.1f, 0.9f, Hh, 12, ErtCol::Vary(BStone, 0.03f, ++S), true);
		M.AddCylinder(W(BE(u), BN(v), Zb + 4.f + Hh * 0.6f), 1.5f, 1.5f, 0.8f, 12, BStoneD, true);
		M.AddCylinder(W(BE(u), BN(v), Zb + 4.f + Hh), 0.9f, 0.2f, 4.f, 8, Lead, true);
	};
	// Tashqi devor faqat Hisor tepaligida; shahar ochiq, bog'lar bilan o'ralgan. Hisor: tosh devor, burjlar, Usmon va O'rxon maqbaralari
	{
		const float HU = -30.f, HV = 55.f, CR = BurHillR - 6.f;
		for (int32 i = 0; i < 14; ++i)
		{
			if (i == 10) continue;   // janubiy darvoza
			const float Am = (i + 0.5f) * 2.f * PI / 14, Len = 2.f * CR * FMath::Sin(PI / 14);
			M.AddBox(W(BE(HU + FMath::Cos(Am) * CR), BN(HV + FMath::Sin(Am) * CR), TopZ + 3.f), FVector(Len * 50.f + 20.f, 120, 350), ErtCol::Vary(BStoneD, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
			if (i % 2 == 0) { const float A0 = i * 2.f * PI / 14; M.AddBox(W(BE(HU + FMath::Cos(A0) * CR), BN(HV + FMath::Sin(A0) * CR), TopZ + 4.5f), FVector(300, 300, 500), ErtCol::Vary(BStoneD, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A0), 0)); }
		}
		for (int32 k = 0; k < 2; ++k)
		{
			const float u = HU - 10.f + k * 20.f, v = HV + 6.f;
			M.AddCylinder(W(BE(u), BN(v), TopZ), 4.5f, 4.5f, 6.f, 8, ErtCol::Vary(White, 0.03f, ++S), true);
			M.AddSphere(W(BE(u), BN(v), TopZ + 6.f), 4.7f, 12, Lead, FVector(1, 1, 0.75f));
		}
		M.AddBox(W(BE(HU + 2.f), BN(HV - 14.f), TopZ + 3.f), FVector(1200, 600, 300), ErtCol::Vary(BStone, 0.03f, ++S));   // bey saroyi
		M.AddBox(W(BE(HU + 2.f), BN(HV - 14.f), TopZ + 6.5f), FVector(1300, 700, 40), Tile);
		AddBanner(M, BE(HU - 6.f), BN(HV - CR + 4.f), TopZ, 7.f, BRed, false);
		AddBanner(M, BE(HU + 6.f), BN(HV - CR + 4.f), TopZ, 7.f, BRed, false);
		AddFire(M, BE(HU + 14.f), BN(HV + 4.f), TopZ, true);
	}
	// Ulu Jome' (markaz): 4x5 = 20 gumbazli to'rtburchak, ikki minora, shadirvon
	{
		const float MU = 0.f, MV = 5.f;
		M.AddBox(W(BE(MU), BN(MV), Z + 5.f), FVector(2600, 2100, 500), ErtCol::Vary(BStone, 0.03f, ++S));
		M.AddBox(W(BE(MU), BN(MV), Z + 10.2f), FVector(2600, 2100, 20), BStoneD);
		for (int32 i = 0; i < 5; ++i) for (int32 j = 0; j < 4; ++j)
			M.AddSphere(W(BE(MU - 20.f + i * 10.f), BN(MV - 15.f + j * 10.f), Z + 10.f), 4.8f, 10, (i == 2 && j == 1) ? Turq : ErtCol::Vary(Lead, 0.05f, ++S), FVector(1, 1, 0.6f));
		Minaret(MU - 24.f, MV - 22.f, Z, 26.f); Minaret(MU + 24.f, MV - 22.f, Z, 26.f);
		M.AddBox(W(BE(MU), BN(MV - 25.f), Z + 3.f), FVector(1400, 200, 300), ErtCol::Vary(BStone, 0.03f, ++S));   // kirish portali
		M.AddBox(W(BE(MU), BN(MV - 26.1f), Z + 2.f), FVector(200, 20, 350), BStone * 0.3f);
		M.AddCylinder(W(BE(MU), BN(MV - 34.f), Z), 3.f, 3.f, 0.8f, 12, BStoneD, true);                        // shadirvon
		M.AddCylinder(W(BE(MU), BN(MV - 34.f), Z + 0.8f), 0.4f, 0.3f, 2.f, 8, BStoneD, true);
	}
	// Yashil masjid va Yashil maqbara (sharq): feruza koshinli
	{
		const float MU = 62.f, MV = 15.f;
		M.AddBox(W(BE(MU), BN(MV), Z + 5.f), FVector(1200, 1200, 500), ErtCol::Vary(BStone, 0.03f, ++S));
		M.AddBox(W(BE(MU), BN(MV), Z + 10.2f), FVector(800, 800, 60), BStoneD);
		M.AddSphere(W(BE(MU), BN(MV), Z + 10.5f), 6.5f, 14, Lead, FVector(1, 1, 0.8f));
		M.AddBox(W(BE(MU - 6.5f), BN(MV), Z + 5.f), FVector(20, 500, 700), Turq);                                 // koshinli fasad
		M.AddBox(W(BE(MU - 6.7f), BN(MV), Z + 2.5f), FVector(20, 200, 400), BStone * 0.3f);
		Minaret(MU + 7.f, MV + 8.f, Z, 20.f);
		M.AddCylinder(W(BE(MU + 2.f), BN(MV - 26.f), Z), 5.f, 5.f, 8.f, 8, ErtCol::Vary(Turq, 0.03f, ++S), true);   // Yashil maqbara (sakkiz qirra)
		M.AddCylinder(W(BE(MU + 2.f), BN(MV - 26.f), Z + 8.f), 5.3f, 5.3f, 0.8f, 8, TurqD, true);
		M.AddSphere(W(BE(MU + 2.f), BN(MV - 26.f), Z + 8.5f), 5.2f, 12, TurqD, FVector(1, 1, 0.75f));
	}
	// Koza xon (ipak bozori, janub): ikki qavatli hovlili bino, o'rtada kichik masjid ustunlarda
	{
		const float MU = -8.f, MV = -55.f;
		M.AddBox(W(BE(MU), BN(MV), Z + 4.f), FVector(1800, 1500, 400), ErtCol::Vary(BStone, 0.03f, ++S));
		M.AddBox(W(BE(MU), BN(MV), Z + 8.3f), FVector(1900, 1600, 40), Tile);
		M.AddBox(W(BE(MU), BN(MV), Z + 0.2f), FVector(1000, 800, 20), FLinearColor(0.85f, 0.83f, 0.78f));
		for (float a = 0; a < 2.f * PI; a += PI / 12) { const float ru = FMath::Cos(a) * 10.f, rv = FMath::Sin(a) * 8.f; M.AddCylinder(W(BE(MU + ru), BN(MV + rv), Z), 0.4f, 0.4f, 8.f, 6, BStone, true); }
		for (int32 i = 0; i < 8; ++i) M.AddCylinder(W(BE(MU - 7.f + i * 2.f), BN(MV - 4.f), Z + 4.2f), 0.3f, 0.3f, 4.f, 6, BStone, true);
		M.AddBox(W(BE(MU), BN(MV), Z + 4.f), FVector(600, 600, 20), BStoneD);
		for (int32 i = 0; i < 8; ++i) { const float a = i * PI / 4; M.AddCylinder(W(BE(MU + FMath::Cos(a) * 2.6f), BN(MV + FMath::Sin(a) * 2.6f), Z + 4.2f), 0.3f, 0.3f, 3.f, 6, BStone, true); }
		M.AddBox(W(BE(MU), BN(MV), Z + 7.3f), FVector(300, 300, 20), BStoneD);
		M.AddSphere(W(BE(MU), BN(MV), Z + 7.5f), 2.6f, 10, Lead, FVector(1, 1, 0.8f));
		M.AddBox(W(BE(MU), BN(MV + 8.f), Z + 2.f), FVector(200, 40, 350), BStone * 0.3f);   // darvoza
	}
	// Hammom (g'arb): past ko'p gumbazli, issiq suv bug'i
	{
		const float MU = -60.f, MV = -20.f;
		M.AddBox(W(BE(MU), BN(MV), Z + 2.5f), FVector(1400, 1000, 250), ErtCol::Vary(BStone, 0.03f, ++S));
		M.AddSphere(W(BE(MU - 5.f), BN(MV), Z + 5.f), 5.f, 12, ErtCol::Vary(Lead, 0.04f, ++S), FVector(1, 1, 0.7f));
		for (int32 i = 0; i < 4; ++i) M.AddSphere(W(BE(MU + 6.f), BN(MV - 6.f + i * 4.f), Z + 5.f), 1.8f, 8, ErtCol::Vary(Lead, 0.04f, ++S), FVector(1, 1, 0.6f));
		M.AddSphere(W(BE(MU - 5.f), BN(MV), Z + 11.f), 2.f, 6, Steam, FVector(1.4f, 1.f, 0.6f));
		M.AddCylinder(W(BE(MU - 14.f), BN(MV + 8.f), Z), 1.4f, 1.4f, 0.8f, 10, BStoneD, true);   // buloq havzasi
	}
	// Uylar: oq suvoq, yog'och ayvon, sopol tom (Usmonli uslubi); radial va bog'li
	for (float R = 34.f; R < BurR - 12.f; R += 15.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 15.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(v) < 8.f && u > 0.f) continue;                                                // sharqiy ko'cha
			if (FMath::Abs(u) < 8.f && v < -20.f) continue;                                              // janubiy ko'cha
			if (FMath::Abs(u) < 16.f && FMath::Abs(v - 5.f) < 14.f) continue;                            // Ulu Jome'
			if (FVector2D::Distance(FVector2D(u, v), FVector2D(-30.f, 55.f)) < BurHillR + 2.f) continue; // Hisor
			if (FMath::Abs(u - 62.f) < 16.f && FMath::Abs(v - 2.f) < 30.f) continue;                     // Yashil
			if (FMath::Abs(u + 8.f) < 20.f && FMath::Abs(v + 55.f) < 17.f) continue;                     // Koza xon
			if (FMath::Abs(u + 60.f) < 16.f && FMath::Abs(v + 20.f) < 12.f) continue;                    // hammom
			if (RS.FRand() < 0.2f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.6f ? 6.5f : 4.f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(BE(u), BN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(White, 0.06f, ++S), FRotator(0, Yaw, 0));
			if (HH > 5.f) M.AddBox(W(BE(u), BN(v), Z + 5.2f), FVector(HW * 50.f + 40.f, HD * 50.f + 40.f, 100.f), ErtCol::Vary(BWood, 0.08f, ++S), FRotator(0, Yaw, 0));   // chiqma ayvon (cumba)
			M.AddCylinder(W(BE(u), BN(v), Z + HH), FMath::Max(HW, HD) * 0.8f, 0.3f, 2.2f, 4, ErtCol::Vary(Tile, 0.06f, ++S), true, FRotator(0, Yaw + 45.f, 0));   // to'rt nishabli sopol tom
		}
	}
	// Chinorlar va tut bog'lari (ipak uchun)
	for (int32 i = 0; i < 20; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(30.f, BurR - 8.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FVector2D::Distance(FVector2D(u, v), FVector2D(-30.f, 55.f)) < BurHillR + 2.f) continue;
		if (FMath::Abs(u) < 16.f && FMath::Abs(v - 5.f) < 14.f) continue;
		const float ZZ = HeightAt(BE(u), BN(v));
		M.AddCylinder(W(BE(u), BN(v), ZZ), 0.5f, 0.4f, 5.f, 6, Trunk, true);
		M.AddSphere(W(BE(u), BN(v), ZZ + 7.5f), 4.5f, 8, ErtCol::Vary(Plane, 0.08f, ++S), FVector(1, 1, 0.8f));
	}
	for (int32 i = 0; i < 40; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(BurR + 6.f, BurR + 28.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(v) < 10.f && u > 0.f) continue;
		const float ZZ = HeightAt(BE(u), BN(v));
		M.AddCylinder(W(BE(u), BN(v), ZZ), 0.25f, 0.2f, 1.8f, 5, Trunk * 0.8f, true);
		M.AddSphere(W(BE(u), BN(v), ZZ + 2.8f), 2.f, 6, ErtCol::Vary(FLinearColor(0.30f, 0.52f, 0.18f), 0.1f, ++S), FVector(1, 1, 0.8f));
	}
	AddBanner(M, BE(BurR + 3.f), BN(-9.f), Z, 6.f, BRed, false);
	AddBanner(M, BE(BurR + 3.f), BN(9.f), Z, 6.f, BRed, false);
	M.Commit(NewPart(TEXT("Bursa"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("Erzurum")) || E.Region.Contains(TEXT("Erzincan"))) { PE = ErzE; PN = ErzN - ErzR - 45.f; }   // Erzurum janubiy darvozasi oldi',
        '\telse if (E.Region.Contains(TEXT("Erzurum")) || E.Region.Contains(TEXT("Erzincan"))) { PE = ErzE; PN = ErzN - ErzR - 45.f; }   // Erzurum janubiy darvozasi oldi\n\telse if (E.Region.Contains(TEXT("Bursa")) || E.Region.Contains(TEXT("Nikeya"))) { PE = BurE + BurR + 45.f; PN = BurN; }   // Bursa sharqiy kirishi')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("erzurum")) { E = ErtMap::ErzE; Nn = ErtMap::ErzN; }', '\t\telse if (Place == TEXT("erzurum")) { E = ErtMap::ErzE; Nn = ErtMap::ErzN; }\n\t\telse if (Place == TEXT("bursa")) { E = ErtMap::BurE; Nn = ErtMap::BurN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(ErzE, ErzN, ErzR, TEXT("Erzurum"), Ink);', '\tMark(ErzE, ErzN, ErzR, TEXT("Erzurum"), Ink);\n\tMark(BurE, BurN, BurR, TEXT("Bursa"), Ink);\n\tMark(UluE, UluN, 40.f, TEXT("Uludog\'"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(42.9f)) TakeShot(TEXT("erzurum"));\n\tif (At(43.4f))', '\tif (At(42.9f)) TakeShot(TEXT("erzurum"));\n\tif (At(43.0f)) Teleport(-380.f, 60.f, 9.f + 55.f, -18.f, -90.f);\n\tif (At(43.9f)) TakeShot(TEXT("bursa"));\n\tif (At(44.0f)) { if (AErtHorse* Hh = NearestHorse(1e7f)) Teleport(Hh->GetActorLocation().Y / 100.f + 4.5f, Hh->GetActorLocation().X / 100.f - 3.f, Hh->GetActorLocation().Z / 100.f + 1.2f, -12.f, 145.f); }\n\tif (At(44.9f)) TakeShot(TEXT("horse"));\n\tif (At(45.4f))')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'bursa_savdogar' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'bursa_savdogar', 'name': 'chr.bursa_savdogar.name', 'place': 'bursa', 'u': -8, 'v': -44, 'yaw': 0, 'woman': False, 'kaftan': [0.2, 0.5, 0.45], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'bursa_qozi', 'name': 'chr.bursa_qozi.name', 'place': 'bursa', 'u': 0, 'v': -30, 'yaw': 0, 'woman': False, 'kaftan': [0.85, 0.82, 0.75], 'dialog': 'ep016_talk'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.bursa_savdogar.name","Bursa ipak savdogari","Bursa ipek tüccarı","Bursa silk merchant"', '"chr.bursa_qozi.name","Bursa qozisi","Bursa kadısı","Qadi of Bursa"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)

# ================= REALISTIC HORSE =================
h = load('ErtHorse.h')
h = rep(h, "\tUPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Legs;", "\tUPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Legs;\n\tUPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> LowerLegs;\n\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> TailMesh;\n\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> NeckMesh;")
save('ErtHorse.h', h)

c = load('ErtHorse.cpp')
start = c.index("\tconst FLinearColor Dark = Coat * 0.55f, Mane(0.12f, 0.09f, 0.06f), Leather(0.30f, 0.18f, 0.09f), Cloth(0.55f, 0.12f, 0.10f);")
end = c.index("void AErtHorse::ApplyDamage(float D)")
new_build = r'''	// ---- Realistik ot: ellipsoid tana, egilgan bo'yin, tumshuqli bosh, yol, dum, ikki bo'g'inli oyoqlar, egar-jabduq ----
	FRandomStream RS(GetUniqueID());
	const bool bDarkPoints = RS.FRand() < 0.55f;                          // qora oyoq/tumshuq (bay)
	const bool bBlaze = RS.FRand() < 0.35f;                               // peshonada oq dog'
	const int32 Socks = RS.RandRange(0, 3);                               // oq paypoqlar soni
	const FLinearColor Dark = bDarkPoints ? FLinearColor(0.08f, 0.06f, 0.05f) : Coat * 0.6f;
	const FLinearColor Mane = bDarkPoints ? FLinearColor(0.07f, 0.05f, 0.04f) : Coat * 0.45f;
	const FLinearColor Hoof(0.16f, 0.13f, 0.11f), Leather(0.30f, 0.18f, 0.09f), LeatherD(0.20f, 0.12f, 0.06f), Cloth(0.55f, 0.12f, 0.10f), ClothTrim(0.85f, 0.7f, 0.25f), Iron(0.6f, 0.6f, 0.62f), Eye(0.05f, 0.04f, 0.04f), WhiteM(0.92f, 0.9f, 0.86f);
	auto Sph = [&](FErtMeshData& D, const FVector& C, float R, const FLinearColor& Col, const FVector& Sc, int32 Sd) { D.AddSphere(C, R, 12, ErtCol::Vary(Col, 0.04f, Sd), Sc); };
	// Kapsula markazi yerdan 100 sm balandda. Tana markazi yerdan ~130 sm.
	BodyMesh = Make(TEXT("HorseBody"), GetCapsuleComponent(), FVector(0, 0, 25.f));
	FErtMeshData M;
	Sph(M, FVector(0, 0, 2), 30.f, Coat, FVector(2.6f, 1.0f, 1.05f), 1);                         // tana (bochka)
	Sph(M, FVector(-62, 0, 10), 30.f, Coat, FVector(1.15f, 0.95f, 1.0f), 2);                     // sag'ri
	Sph(M, FVector(-70, 0, -8), 22.f, Coat, FVector(0.9f, 1.05f, 1.1f), 3);                      // son
	Sph(M, FVector(66, 0, 6), 28.f, Coat, FVector(1.05f, 0.95f, 1.05f), 4);                      // ko'krak/yelka
	Sph(M, FVector(40, 0, 32), 18.f, Coat, FVector(1.4f, 0.8f, 0.6f), 5);                        // yag'rin (withers)
	Sph(M, FVector(74, 0, -6), 16.f, Coat, FVector(1.1f, 0.9f, 1.0f), 6);                        // ko'krak oldi
	// Egar ostidagi gilam, egar (o'rindiq, oldingi qosh, orqa qosh), ayil, uzangi tasmalari va uzangilar, ko'krak tasmasi
	M.AddBox(FVector(-8, 0, 30), FVector(36, 30, 3), Cloth);
	M.AddBox(FVector(-8, 0, 30.5f), FVector(37, 31, 1.5f), ClothTrim);
	M.AddBox(FVector(-8, 0, 33), FVector(36, 28, 2), Cloth);
	Sph(M, FVector(-8, 0, 36), 14.f, Leather, FVector(2.0f, 1.25f, 0.45f), 7);                  // o'rindiq
	Sph(M, FVector(-34, 0, 44), 8.f, LeatherD, FVector(0.5f, 1.4f, 1.0f), 8);                   // orqa qosh
	Sph(M, FVector(18, 0, 45), 6.f, LeatherD, FVector(0.6f, 1.0f, 1.2f), 9);                    // oldingi qosh
	for (int32 s = -1; s <= 1; s += 2)
	{
		M.AddBox(FVector(-8, s * 27.f, 10), FVector(3, 1.5f, 38), Leather);                     // uzangi tasmasi
		M.AddBox(FVector(-8, s * 28.f, -30), FVector(7, 2.5f, 1.5f), Iron);                     // uzangi (tagi)
		M.AddBox(FVector(-8, s * 28.f, -25), FVector(1.5f, 2.5f, 6), Iron);                     // uzangi (yon)
		M.AddBox(FVector(-8, s * 25.f, 33), FVector(20, 4, 4), LeatherD);                       // egar qanoti
	}
	for (float a = -1.2f; a <= 1.2f; a += 0.3f) M.AddBox(FVector(-4, FMath::Sin(a) * 31.f, 8 + FMath::Cos(a) * 30.f - 30.f), FVector(4, 3, 3), LeatherD);   // ayil
	M.AddBox(FVector(70, 0, 8), FVector(3, 26, 2.5f), LeatherD);                                  // ko'krak tasmasi
	M.Commit(BodyMesh, 0, false);

	// Bo'yin: yag'rindan yuqoriga egilgan; ustida yol tolalari
	NeckMesh = Make(TEXT("HorseNeck"), BodyMesh, FVector(52.f, 0, 22.f));
	M.Reset();
	M.AddCylinder(FVector(0, 0, 0), 20.f, 13.f, 74.f, 12, Coat, true, FRotator(-52.f, 0, 0));
	Sph(M, FVector(0, 0, 0), 20.f, Coat, FVector(1.0f, 1.0f, 1.0f), 10);
	for (int32 i = 0; i < 9; ++i)
	{
		const float t = i / 8.f, X = 0.f + t * 46.f - 10.f, Zz = 12.f + t * 58.f;
		M.AddBox(FVector(X - 6, 0, Zz + 6), FVector(4, 1.6f, 9 + RS.FRand() * 3.f), Mane, FRotator(-52.f + RS.FRandRange(-8.f, 8.f), 0, RS.FRandRange(-10.f, 10.f)));
	}
	M.Commit(NeckMesh, 0, false);

	// Bosh: kalla + peshona + tumshuq, burun teshiklari, ko'zlar, quloqlar, yugan, jilov
	HeadMesh = Make(TEXT("HorseHead"), NeckMesh, FVector(44.f, 0, 62.f));
	M.Reset();
	Sph(M, FVector(6, 0, 4), 13.f, Coat, FVector(1.3f, 1.0f, 1.15f), 11);                        // kalla
	M.AddCylinder(FVector(6, 0, 4), 11.f, 7.5f, 34.f, 10, Coat, true, FRotator(-100.f, 0, 0));    // yuz (pastga-oldinga)
	Sph(M, FVector(40, 0, -2), 8.f, Dark, FVector(1.2f, 1.0f, 0.9f), 12);                        // tumshuq
	if (bBlaze) M.AddBox(FVector(18, 0, 6), FVector(14, 1.5f, 3), WhiteM, FRotator(-100.f + 90.f, 0, 0));
	for (int32 s = -1; s <= 1; s += 2)
	{
		Sph(M, FVector(43, s * 4.5f, 1), 1.6f, Eye, FVector(1, 1, 1), 13);                       // burun teshigi
		Sph(M, FVector(12, s * 10.5f, 7), 2.6f, Eye, FVector(0.7f, 1, 1), 14);                    // ko'z
		M.AddCylinder(FVector(2, s * 6.f, 14), 3.2f, 0.6f, 12.f, 6, Coat, true, FRotator(-10.f, 0, s * 22.f));   // quloq
		M.AddBox(FVector(22, s * 8.5f, 3), FVector(1.2f, 1.f, 5.f), Leather, FRotator(-10.f, 0, 0));           // yugan yon tasma
		M.AddBox(FVector(30, s * 6.5f, -3), FVector(1.f, 1.f, 22.f), LeatherD, FRotator(85.f, 0, 0));          // jilov (orqaga)
	}
	M.AddBox(FVector(36, 0, -6), FVector(1.5f, 9, 1.5f), Leather);                               // burun tasmasi
	M.AddBox(FVector(-2, 0, 12), FVector(1.5f, 12, 1.5f), Leather);                               // peshona tasmasi
	M.AddBox(FVector(38, 0, -8), FVector(1, 8, 1), Iron);                                        // suvliq
	M.Commit(HeadMesh, 0, false);

	// Dum: sag'ridan tushuvchi qalin tola dastasi
	TailMesh = Make(TEXT("HorseTail"), BodyMesh, FVector(-92.f, 0, 22.f));
	M.Reset();
	M.AddCylinder(FVector(0, 0, 0), 5.f, 3.f, 22.f, 8, Mane, true, FRotator(-150.f, 0, 0));
	for (int32 i = 0; i < 7; ++i) M.AddBox(FVector(-12 + RS.FRandRange(-3.f, 3.f), RS.FRandRange(-4.f, 4.f), -26.f - i * 2.f), FVector(2.5f, 2.f, 30.f), ErtCol::Vary(Mane, 0.06f, 30 + i), FRotator(RS.FRandRange(-12.f, 4.f), 0, RS.FRandRange(-8.f, 8.f)));
	M.Commit(TailMesh, 0, false);

	// Oyoqlar: yuqori qism (son/yelka) kapsulaga, pastki qism (bilak+tuyoq) yuqori qismga bog'langan (tirsak/hock bo'g'ini)
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bFront = i < 2;
		const float X = bFront ? 56.f : -60.f, Y = (i & 1) ? 19.f : -19.f;
		const bool bSock = i < Socks;
		UProceduralMeshComponent* Leg = Make(TEXT("HorseLeg"), GetCapsuleComponent(), FVector(X, Y, 4.f));
		M.Reset();
		Sph(M, FVector(0, 0, 0), 12.f, Coat, FVector(1.1f, 0.8f, 1.0f), 40 + i);                                 // bo'g'in (yelka/son)
		M.AddCylinder(FVector(0, 0, 0), 9.f, 6.5f, 44.f, 10, Coat, true, FRotator(180.f + (bFront ? 6.f : -10.f), 0, 0));   // yuqori suyak
		M.Commit(Leg, 0, false);
		Legs.Add(Leg);
		UProceduralMeshComponent* Low = Make(TEXT("HorseLowerLeg"), Leg, FVector(bFront ? 4.5f : -7.6f, 0, -43.5f));
		M.Reset();
		Sph(M, FVector(0, 0, 0), 7.f, Coat, FVector(1.2f, 0.9f, 1.0f), 50 + i);                                    // tizza/hock
		M.AddCylinder(FVector(0, 0, 0), 5.5f, 4.2f, 40.f, 8, bSock ? WhiteM : (bDarkPoints ? Dark : Coat * 0.92f), true, FRotator(180.f, 0, 0));   // bilak (cannon)
		Sph(M, FVector(1, 0, -42), 5.2f, bSock ? WhiteM : Dark, FVector(1.1f, 1.0f, 0.8f), 60 + i);                  // to'piq (fetlock)
		M.AddCylinder(FVector(3, 0, -55), 6.5f, 5.f, 9.f, 8, Hoof, true);                                          // tuyoq
		M.Commit(Low, 0, false);
		LowerLegs.Add(Low);
	}
}

'''
c = c[:start] + new_build + c[end:]
# animation: two-joint legs, tail sway, neck bob
old_anim_start = c.index("void AErtHorse::Animate(float Dt)")
new_anim = r'''void AErtHorse::Animate(float Dt)
{
	const float Sp = GetCharacterMovement()->Velocity.Size2D();
	const float Stride = bCamel ? 340.f : (Sp > 600.f ? 300.f : 190.f);
	if (Sp > 10.f) Phase += Dt * (Sp / Stride) * 2.f * PI;
	const float Amp = FMath::Clamp(Sp / 500.f, 0.f, 1.3f) * 28.f;
	const bool bInAir = GetCharacterMovement()->IsFalling();
	const float T = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Legs.Num(); ++i)
	{
		// Yo'rtish: diagonal juftlar (FL+BR, FR+BL); chopishda oldingi/orqa juftlar
		float Ph = Phase;
		if (bCamel) Ph += ((i & 1) ? 0.f : PI);   // tuya: bir tomon oyoqlari birga (yo'rg'a)
		else if (Sp > 600.f) Ph += (i < 2) ? 0.f : PI * 0.6f; else Ph += ((i == 0 || i == 3) ? 0.f : PI);
		float Pitch = FMath::Sin(Ph) * Amp;
		if (bInAir) Pitch = (i < 2) ? -35.f : 30.f;
		Legs[i]->SetRelativeRotation(FRotator(Pitch, 0, 0));
		if (LowerLegs.IsValidIndex(i) && LowerLegs[i])
		{
			// Pastki bo'g'in: oyoq oldinga uchganda bukiladi (oldingi oyoq orqaga, orqa oyoq oldinga buklanadi)
			const float Bend = FMath::Max(0.f, FMath::Sin(Ph + PI * 0.5f)) * Amp * 1.6f;
			float LP = (i < 2) ? -Bend : Bend * 0.8f;
			if (bInAir) LP = (i < 2) ? -50.f : 25.f;
			else if (Sp < 10.f) LP = 0.f;
			LowerLegs[i]->SetRelativeRotation(FRotator(LP, 0, 0));
		}
	}
	HeadBob = FMath::Sin(Phase) * FMath::Min(Sp / 300.f, 1.f) * 6.f;
	if (NeckMesh)
	{
		// Chopishda bo'yin cho'ziladi, turganda bosh biroz pastga (o'tlash)
		const float Stretch = FMath::Clamp((Sp - 300.f) / 600.f, 0.f, 1.f) * 22.f;
		const float Graze = (Sp < 5.f && !Rider) ? (FMath::Sin(T * 0.35f) > 0.3f ? 38.f : 0.f) : 0.f;
		NeckMesh->SetRelativeRotation(FMath::RInterpTo(NeckMesh->GetRelativeRotation(), FRotator(HeadBob * 0.6f + Stretch + Graze, 0, 0), Dt, 4.f));
	}
	if (HeadMesh) HeadMesh->SetRelativeRotation(FRotator(HeadBob - (Sp > 600.f ? 8.f : 0.f) + FMath::Sin(T * 1.7f) * 2.f, FMath::Sin(T * 0.9f) * 3.f, 0));
	if (TailMesh) TailMesh->SetRelativeRotation(FRotator(FMath::Clamp(Sp / 900.f, 0.f, 1.f) * 25.f, FMath::Sin(T * 2.3f) * 12.f + FMath::Sin(Phase) * 4.f, 0));
	if (BodyMesh) BodyMesh->SetRelativeLocation(FVector(0, 0, (bCamel ? 45.f : 25.f) + FMath::Abs(FMath::Sin(Phase)) * FMath::Min(Sp / 400.f, 1.f) * (bCamel ? 8.f : 5.f)));
	if (BodyMesh && !bCamel && !bDead) BodyMesh->SetRelativeRotation(FRotator(FMath::Sin(Phase) * FMath::Min(Sp / 700.f, 1.f) * 3.f, 0, 0));
}
'''
c = c[:old_anim_start] + new_anim
c = rep(c, "\tfor (UProceduralMeshComponent* L : Legs) if (L) { L->SetRelativeRotation(FRotator(0, 0, 75.f)); L->AddRelativeLocation(FVector(0, 0, -60.f)); }",
        "\tfor (UProceduralMeshComponent* L : Legs) if (L) { L->SetRelativeRotation(FRotator(0, 0, 75.f)); L->AddRelativeLocation(FVector(0, 0, -60.f)); }\n\tfor (UProceduralMeshComponent* L : LowerLegs) if (L) L->SetRelativeRotation(FRotator(0, 0, 0));")
c = rep(c, "\tSaddle->SetRelativeLocation(FVector(-5.f, 0.f, 62.f));", "\tSaddle->SetRelativeLocation(FVector(-8.f, 0.f, 66.f));")
save('ErtHorse.cpp', c)
print('patched')
