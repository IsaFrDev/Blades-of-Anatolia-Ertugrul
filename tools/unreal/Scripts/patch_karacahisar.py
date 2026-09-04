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
h = rep(h, "// Nikeya va Askaniya ko'li\n", "// Nikeya va Askaniya ko'li\n\tconstexpr float KarE = -60.f, KarN = 760.f, KarR = 62.f, KarH = 45.f, KarBaseZ = 18.f;   // Karacahisar qoyasi\n")
h = rep(h, "\tvoid BuildNikeya();", "\tvoid BuildNikeya();\n\tvoid BuildKaracahisar();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildNikeya();\n\t}", "\t\tBuildNikeya();\n\t\tBuildKaracahisar();\n\t}")
c = rep(c, "{{-150, -150}, {-190, -60}, 6},", "{{-150, -150}, {-190, -60}, 6}, {{-30, 480}, {-60, 690}, 5},")
c = rep(c, "\t// Nikeya: tekis aylana; Askaniya ko'li", '''\t// Karacahisar: tekis etak + tik qora qoya (tepasi tekis plato)
\t{
\t\tconst float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KarE, KarN));
\t\tH = FMath::Lerp(H, KarBaseZ, 1.f - Smooth01((DK - KarR - 25.f) / 60.f));
\t\tconst float T = Smooth01(1.f - (DK - 30.f) / (KarR - 30.f));   // 30 m gacha tekis plato, keyin tik qiyalik
\t\tH += KarH * T + Noise(E, N, 0.08f) * 3.f * T * (1.f - T) * 4.f;
\t}
\t// Nikeya: tekis aylana; Askaniya ko'li''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KarE, KarN));
\t\tC = FMath::Lerp(C, FLinearColor(0.22f, 0.21f, 0.22f), 1.f - Smooth01((DK - KarR + 2.f) / 10.f));   // qora qoya
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Nikeya\n", "// Nikeya\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(KarE, KarN)) < KarR + 30.f) return false;     // Karacahisar\n")
c += r'''
// ---------------- Karacahisar: qora qoya ustidagi Vizantiya qal'asi ----------------

void AErtWorldBuilder::BuildKaracahisar()
{
	FRandomStream RS(Seed + 233);
	FErtMeshData M(100.f);
	int32 S = 9500;
	const float TopZ = KarBaseZ + KarH, Z0 = KarBaseZ;
	auto KE = [](float u) { return KarE + u; };
	auto KN = [](float v) { return KarN + v; };
	const FLinearColor KStone(0.40f, 0.39f, 0.40f), KStoneL(0.52f, 0.50f, 0.48f), Brick(0.55f, 0.30f, 0.22f), KWood(0.36f, 0.24f, 0.12f), Lead(0.45f, 0.47f, 0.52f), Purple(0.35f, 0.10f, 0.35f), GoldC(0.9f, 0.75f, 0.25f), Iron(0.3f, 0.3f, 0.32f), Thatch(0.62f, 0.52f, 0.30f);
	// Plato chetidagi notekis ko'pburchak devor (R ~ 28 m, 11 burchak), kungura, kvadrat burjlar, janubiy darvoza
	const int32 Sides = 11; const float CR = 28.f;
	TArray<FVector2D> Pts;
	for (int32 i = 0; i < Sides; ++i) { const float A = i * 2.f * PI / Sides, R = CR + RS.FRandRange(-3.f, 3.f); Pts.Add(FVector2D(FMath::Cos(A) * R, FMath::Sin(A) * R)); }
	for (int32 i = 0; i < Sides; ++i)
	{
		const FVector2D A = Pts[i], B = Pts[(i + 1) % Sides], Mid = (A + B) * 0.5f;
		const float Len = FVector2D::Distance(A, B), Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X)) ;
		const bool bGate = FMath::Abs(Mid.X) < 6.f && Mid.Y < 0.f;
		const float Hh = bGate ? 9.f : 7.f;
		if (bGate)
		{
			M.AddBox(W(KE(Mid.X), KN(Mid.Y), TopZ + 7.5f), FVector(Len * 50.f, 120, 150), KStone, FRotator(0, Yaw, 0));
			for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(KE(Mid.X + k * 5.f), KN(Mid.Y), TopZ + 6.f), FVector(300, 350, 1200), ErtCol::Vary(KStone, 0.04f, ++S)); M.AddBox(W(KE(Mid.X + k * 5.f), KN(Mid.Y), TopZ + 12.4f), FVector(340, 390, 40), KStoneL); }
			M.AddBox(W(KE(Mid.X), KN(Mid.Y - 1.6f), TopZ + 2.5f), FVector(160, 10, 500), Iron);   // temir panjara (ko'tarilgan)
		}
		else
		{
			M.AddBox(W(KE(Mid.X), KN(Mid.Y), TopZ + Hh * 0.5f), FVector(Len * 50.f + 20.f, 110, Hh * 50.f), ErtCol::Vary(KStone, 0.06f, ++S), FRotator(0, Yaw, 0));
			for (float bz = 2.f; bz < Hh; bz += 2.5f) M.AddBox(W(KE(Mid.X), KN(Mid.Y), TopZ + bz), FVector(Len * 50.f + 22.f, 112, 20), ErtCol::Vary(Brick, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.4f)
			{
				const FVector2D P = Mid + (B - A).GetSafeNormal() * t;
				M.AddBox(W(KE(P.X), KN(P.Y), TopZ + Hh + 0.5f), FVector(55, 110, 50), ErtCol::Vary(KStoneL, 0.05f, ++S), FRotator(0, Yaw, 0));
			}
		}
		if (i % 2 == 0) { M.AddBox(W(KE(A.X), KN(A.Y), TopZ + 5.f), FVector(320, 320, 1000), ErtCol::Vary(KStone, 0.04f, ++S), FRotator(0, Yaw, 0)); M.AddBox(W(KE(A.X), KN(A.Y), TopZ + 10.4f), FVector(360, 360, 40), KStoneL, FRotator(0, Yaw, 0)); }
	}
	// Donjon (asosiy minora, shimol): 18 m kvadrat minora, kungura, bayroq
	M.AddBox(W(KE(-4.f), KN(12.f), TopZ + 9.f), FVector(700, 700, 1800), ErtCol::Vary(KStone, 0.03f, ++S));
	for (float bz = 3.f; bz < 18.f; bz += 4.f) M.AddBox(W(KE(-4.f), KN(12.f), TopZ + bz), FVector(702, 702, 25), ErtCol::Vary(Brick, 0.05f, ++S));
	M.AddBox(W(KE(-4.f), KN(12.f), TopZ + 18.4f), FVector(760, 760, 40), KStoneL);
	for (int32 i = 0; i < 4; ++i) for (int32 j = 0; j < 4; ++j) if ((i + j) % 2 == 0) M.AddBox(W(KE(-4.f - 7.f + i * 4.6f), KN(12.f - 7.f + j * 4.6f), TopZ + 19.2f), FVector(60, 60, 60), KStoneL);
	for (int32 i = 0; i < 3; ++i) M.AddBox(W(KE(-4.f - 7.1f), KN(12.f - 3.f + i * 3.f), TopZ + 6.f + i * 3.f), FVector(10, 40, 90), KStone * 0.2f);   // tirqish derazalar
	AddBanner(M, KE(-4.f), KN(12.f), TopZ + 18.6f, 5.f, Purple, false);
	// Cherkov (g'arb): kichik bazilika, gumbaz, xoch
	M.AddBox(W(KE(-16.f), KN(-4.f), TopZ + 3.f), FVector(500, 900, 300), ErtCol::Vary(KStoneL, 0.03f, ++S));
	M.AddCylinder(W(KE(-16.f), KN(-4.f), TopZ + 6.f), 6.f, 0.3f, 2.f, 4, Brick, true);
	M.AddSphere(W(KE(-16.f), KN(-4.f), TopZ + 6.5f), 2.6f, 10, Lead, FVector(1, 1, 0.7f));
	M.AddBox(W(KE(-16.f), KN(-4.f), TopZ + 9.6f), FVector(10, 10, 80), GoldC); M.AddBox(W(KE(-16.f), KN(-4.f), TopZ + 10.f), FVector(10, 40, 10), GoldC);
	// Kazarma (sharq), sardoba, oshxona tutuni, o't yoqilgan gulxan, ombor
	M.AddBox(W(KE(14.f), KN(2.f), TopZ + 2.5f), FVector(400, 1000, 250), ErtCol::Vary(KStone, 0.05f, ++S));
	M.AddCylinder(W(KE(14.f), KN(2.f), TopZ + 5.f), 5.5f, 0.3f, 1.8f, 4, Thatch, true);
	M.AddCylinder(W(KE(4.f), KN(-8.f), TopZ), 2.5f, 2.5f, 1.f, 10, KStoneL, true);
	M.AddCylinder(W(KE(4.f), KN(-8.f), TopZ + 1.f), 2.f, 2.f, 0.2f, 10, FLinearColor(0.15f, 0.25f, 0.35f), true);
	M.AddBox(W(KE(12.f), KN(-14.f), TopZ + 1.5f), FVector(300, 250, 150), KWood);
	AddFire(M, KE(-2.f), KN(-2.f), TopZ, true);
	// Ilon izi ko'tarilish yo'li (janub, sharqdan g'arbga zigzag): tosh yo'lak + past devorcha
	{
		const float Run = KarR - 8.f, StartV = -KarR - 6.f;
		const int32 Segs = 3; const int32 StepsPer = 9;
		for (int32 s = 0; s < Segs; ++s)
			for (int32 i = 0; i < StepsPer; ++i)
			{
				const float t = (s * StepsPer + i + 0.5f) / (Segs * StepsPer);
				const float Zc = FMath::Lerp(Z0 + 0.6f, TopZ + 0.4f, t);
				const float v = StartV + (KarR + 6.f - 30.f + 2.f) * FMath::Lerp(0.f, 1.f, t);   // janubdan ichkariga
				const float dir = (s % 2 == 0) ? 1.f : -1.f;
				const float u = dir * (-20.f + 40.f * (i + 0.5f) / StepsPer);
				const float Slope = FMath::RadiansToDegrees(FMath::Atan2(KarH / Segs, 40.f));
				M.AddBox(W(KE(u), KN(v), Zc), FVector(260, 250, 30), ErtCol::Vary(KStoneL, 0.05f, ++S), FRotator(0, 0, dir * Slope));
				M.AddBox(W(KE(u), KN(v - 2.4f), Zc + 0.7f), FVector(260, 25, 70), ErtCol::Vary(KStone, 0.05f, ++S), FRotator(0, 0, dir * Slope));
			}
	}
	// Etakdagi qishloq: 6 kulba, quduq, tekfur bayrog'i
	for (int32 i = 0; i < 6; ++i)
	{
		const float A = PI * 1.15f + i * 0.14f, R = KarR + 22.f + (i % 2) * 8.f;
		const float E = KE(FMath::Cos(A) * R), N = KN(FMath::Sin(A) * R), ZZ = HeightAt(E, N);
		M.AddBox(W(E, N, ZZ + 1.6f), FVector(250, 220, 160), ErtCol::Vary(KStoneL, 0.08f, ++S));
		M.AddCylinder(W(E, N, ZZ + 3.2f), 3.4f, 0.2f, 1.8f, 4, ErtCol::Vary(Thatch, 0.08f, ++S), true);
	}
	M.AddCylinder(W(KE(-6.f), KN(-KarR - 26.f), HeightAt(KE(-6.f), KN(-KarR - 26.f))), 1.2f, 1.2f, 1.f, 8, KStoneL, true);
	AddBanner(M, KE(6.f), KN(-KarR - 12.f), HeightAt(KE(6.f), KN(-KarR - 12.f)), 6.f, Purple, false);
	M.Commit(NewPart(TEXT("Karacahisar"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("Nikeya"))) { PE = NikE + NikR + 55.f; PN = NikN; }   // Nikeya sharqiy darvozasi oldi',
        '\telse if (E.Region.Contains(TEXT("Nikeya"))) { PE = NikE + NikR + 55.f; PN = NikN; }   // Nikeya sharqiy darvozasi oldi\n\telse if (E.Region.Contains(TEXT("Karacahisar"))) { PE = KarE; PN = KarN - KarR - 60.f; }   // Karacahisar etagi (janub)')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("nikeya")) { E = ErtMap::NikE; Nn = ErtMap::NikN; }', '\t\telse if (Place == TEXT("nikeya")) { E = ErtMap::NikE; Nn = ErtMap::NikN; }\n\t\telse if (Place == TEXT("karacahisar")) { E = ErtMap::KarE; Nn = ErtMap::KarN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(NikE, NikN, NikR, TEXT("Nikeya"), Ink);', '\tMark(NikE, NikN, NikR, TEXT("Nikeya"), Ink);\n\tMark(KarE, KarN, KarR, TEXT("Karacahisar"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(45.9f)) TakeShot(TEXT("nikeya"));\n\tif (At(46.4f))', '\tif (At(45.9f)) TakeShot(TEXT("nikeya"));\n\tif (At(46.0f)) Teleport(-60.f, 610.f, 18.f + 40.f, -14.f, 0.f);\n\tif (At(46.9f)) TakeShot(TEXT("karacahisar"));\n\tif (At(47.4f))')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'karacahisar_tekfur' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'karacahisar_tekfur', 'name': 'chr.karacahisar_tekfur.name', 'place': 'karacahisar', 'u': 0, 'v': -2, 'yaw': 180, 'woman': False, 'kaftan': [0.35, 0.1, 0.35], 'dialog': 'ep041_talk'})
    npcs['npcs'].append({'id': 'karacahisar_dehqon', 'name': 'chr.karacahisar_dehqon.name', 'place': 'karacahisar', 'u': -6, 'v': -84, 'yaw': 0, 'woman': True, 'kaftan': [0.5, 0.45, 0.3], 'dialog': 'npc_caravan'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.karacahisar_tekfur.name","Karacahisar tekfuri","Karacahisar tekfuru","Tekfur of Karacahisar"', '"chr.karacahisar_dehqon.name","Qishloq ayoli","Köylü kadın","Village woman"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
