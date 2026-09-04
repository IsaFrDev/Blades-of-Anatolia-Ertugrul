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
h = rep(h, "// Karacahisar qoyasi\n", "// Karacahisar qoyasi\n\tconstexpr float SogE = -220.f, SogN = 600.f, SogR = 70.f, SogZ = 16.f;   // So'g'ut qishlog'i\n")
h = rep(h, "\tvoid BuildKaracahisar();", "\tvoid BuildKaracahisar();\n\tvoid BuildSogut();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildKaracahisar();\n\t}", "\t\tBuildKaracahisar();\n\t\tBuildSogut();\n\t}")
c = rep(c, "{{-30, 480}, {-60, 690}, 5},", "{{-30, 480}, {-60, 690}, 5}, {{-60, 690}, {-160, 610}, 5},")
c = rep(c, "\t// Karacahisar: tekis etak + tik qora qoya", '''\t// So'g'ut: yumshoq tekis vodiy
\t{
\t\tconst float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SogE, SogN));
\t\tH = FMath::Lerp(H, SogZ, 1.f - Smooth01((DS - SogR - 5.f) / 55.f));
\t}
\t// Karacahisar: tekis etak + tik qora qoya''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SogE, SogN));
\t\tC = FMath::Lerp(C, FLinearColor(0.34f, 0.50f, 0.20f), (1.f - Smooth01((DS - SogR) / 30.f)) * 0.6f);   // yam-yashil vodiy
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// Karacahisar\n", "// Karacahisar\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(SogE, SogN)) < SogR + 20.f) return false;     // So'g'ut\n")
c += r'''
// ---------------- So'g'ut: tollar vodiysidagi qishloq ----------------

void AErtWorldBuilder::BuildSogut()
{
	FRandomStream RS(Seed + 251);
	FErtMeshData M(100.f);
	int32 S = 10500;
	const float Z = SogZ;
	auto GE = [](float u) { return SogE + u; };
	auto GN = [](float v) { return SogN + v; };
	const FLinearColor Timber(0.42f, 0.28f, 0.14f), TimberD(0.30f, 0.20f, 0.10f), Plaster(0.88f, 0.84f, 0.72f), Shingle(0.36f, 0.26f, 0.16f), WillowL(0.42f, 0.60f, 0.28f), WillowT(0.35f, 0.28f, 0.18f), Water(0.28f, 0.48f, 0.62f), StoneG(0.62f, 0.60f, 0.55f), Lead(0.45f, 0.47f, 0.52f), GreenF(0.16f, 0.45f, 0.18f), Fruit(0.30f, 0.52f, 0.20f), Wheat(0.80f, 0.68f, 0.35f), Hay(0.75f, 0.62f, 0.32f);
	auto Willow = [&](float u, float v, float Sc, int32 Sd)
	{
		const float ZZ = HeightAt(GE(u), GN(v));
		M.AddCylinder(W(GE(u), GN(v), ZZ), 0.45f * Sc, 0.3f * Sc, 4.f * Sc, 7, ErtCol::Vary(WillowT, 0.1f, Sd), true);
		M.AddSphere(W(GE(u), GN(v), ZZ + 5.5f * Sc), 3.6f * Sc, 8, ErtCol::Vary(WillowL, 0.08f, Sd + 1), FVector(1, 1, 0.75f));
		for (int32 i = 0; i < 10; ++i)
		{
			const float A = i * 2.f * PI / 10 + Sd * 0.3f, R = 3.2f * Sc;
			M.AddBox(W(GE(u + FMath::Cos(A) * R), GN(v + FMath::Sin(A) * R), ZZ + 3.2f * Sc), FVector(12, 12, 200 * Sc), ErtCol::Vary(WillowL * 0.9f, 0.1f, Sd + i), FRotator(FMath::Cos(A) * 6.f, 0, FMath::Sin(A) * 6.f));   // osilgan shoxlar
		}
	};
	// Ariq: sharqdan g'arbga egri suv yo'li (yer sathida ko'k tasma), ustida ikki yog'och ko'prik, tegirmon
	TArray<FVector2D> Brook;
	for (float u = -SogR - 20.f; u <= SogR + 20.f; u += 4.f) Brook.Add(FVector2D(u, -18.f + FMath::Sin(u * 0.06f) * 9.f + FMath::Sin(u * 0.17f + 1.f) * 3.f));
	for (int32 i = 0; i + 1 < Brook.Num(); ++i)
	{
		const FVector2D A = Brook[i], B = Brook[i + 1], Mid = (A + B) * 0.5f;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
		M.AddBox(W(GE(Mid.X), GN(Mid.Y), Z - 0.35f), FVector(230, 170, 12), ErtCol::Vary(Water, 0.04f, ++S), FRotator(0, Yaw, 0));
		M.AddBox(W(GE(Mid.X), GN(Mid.Y), Z - 0.2f), FVector(230, 230, 8), FLinearColor(0.55f, 0.50f, 0.40f), FRotator(0, Yaw, 0));   // qirg'oq loyi
		M.AddBox(W(GE(Mid.X), GN(Mid.Y), Z - 0.3f), FVector(230, 165, 10), ErtCol::Vary(Water, 0.04f, ++S), FRotator(0, Yaw, 0));
	}
	for (float bu : { -22.f, 26.f })
	{
		const float bv = -18.f + FMath::Sin(bu * 0.06f) * 9.f + FMath::Sin(bu * 0.17f + 1.f) * 3.f;
		M.AddBox(W(GE(bu), GN(bv), Z + 0.5f), FVector(140, 320, 14), Timber);
		for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(GE(bu + k * 1.2f), GN(bv), Z + 1.f), FVector(6, 320, 6), TimberD); for (int32 j = -1; j <= 1; ++j) M.AddBox(W(GE(bu + k * 1.2f), GN(bv + j * 1.4f), Z + 0.7f), FVector(6, 6, 60), TimberD); }
	}
	{
		const float mu = 48.f, mv = -18.f + FMath::Sin(mu * 0.06f) * 9.f + FMath::Sin(mu * 0.17f + 1.f) * 3.f;
		M.AddBox(W(GE(mu), GN(mv + 5.f), Z + 2.2f), FVector(350, 300, 220), ErtCol::Vary(StoneG, 0.05f, ++S));   // tegirmon binosi
		M.AddCylinder(W(GE(mu), GN(mv + 5.f), Z + 4.4f), 4.6f, 0.2f, 2.2f, 4, Shingle, true);
		M.AddCylinder(W(GE(mu), GN(mv + 1.6f), Z + 1.6f), 2.2f, 2.2f, 0.4f, 12, TimberD, true, FRotator(90.f, 0, 0));   // charx
		for (int32 i = 0; i < 8; ++i) M.AddBox(W(GE(mu), GN(mv + 1.6f), Z + 1.6f), FVector(10, 40, 230), Timber, FRotator(i * 22.5f, 0, 0));
	}
	// Masjid: tosh poydevor, yog'och minora, qo'rg'oshin gumbaz; yonida Ertug'rul turbasi (sakkiz qirra)
	M.AddBox(W(GE(8.f), GN(22.f), Z + 2.5f), FVector(700, 600, 250), ErtCol::Vary(StoneG, 0.04f, ++S));
	M.AddBox(W(GE(8.f), GN(22.f), Z + 5.2f), FVector(720, 620, 20), Shingle);
	M.AddSphere(W(GE(8.f), GN(22.f), Z + 5.3f), 3.4f, 12, Lead, FVector(1, 1, 0.7f));
	M.AddCylinder(W(GE(15.5f), GN(28.f), Z), 0.9f, 0.7f, 11.f, 8, ErtCol::Vary(Timber, 0.04f, ++S), true);
	M.AddCylinder(W(GE(15.5f), GN(28.f), Z + 9.f), 1.3f, 1.3f, 0.6f, 8, TimberD, true);
	M.AddCylinder(W(GE(15.5f), GN(28.f), Z + 11.f), 1.f, 0.2f, 2.5f, 8, Lead, true);
	M.AddCylinder(W(GE(-8.f), GN(30.f), Z), 3.f, 3.f, 4.f, 8, ErtCol::Vary(StoneG, 0.04f, ++S), true);
	M.AddSphere(W(GE(-8.f), GN(30.f), Z + 4.f), 3.2f, 10, GreenF, FVector(1, 1, 0.7f));
	AddBanner(M, GE(-12.f), GN(26.f), Z, 6.f, FLinearColor(0.55f, 0.10f, 0.08f), true);
	// Bozor maydoni (markaz): to'qilgan soyabonli rastalar, quduq, gulxan
	M.AddBox(W(GE(0.f), GN(2.f), Z + 0.05f), FVector(1400, 1200, 5), FLinearColor(0.62f, 0.55f, 0.42f));
	for (int32 i = 0; i < 6; ++i)
	{
		const float u = -10.f + (i % 3) * 10.f, v = (i < 3) ? -6.f : 10.f;
		M.AddBox(W(GE(u), GN(v), Z + 2.4f), FVector(220, 180, 10), ErtCol::Vary(Hay, 0.12f, ++S), FRotator(0, 0, (i < 3) ? -10.f : 10.f));
		for (int32 k = 0; k < 4; ++k) M.AddCylinder(W(GE(u + ((k & 1) ? 2.f : -2.f)), GN(v + ((k & 2) ? 1.6f : -1.6f)), Z), 0.12f, 0.12f, 2.4f, 5, TimberD, true);
		M.AddBox(W(GE(u), GN(v), Z + 0.8f), FVector(180, 60, 12), Timber);
		M.AddBox(W(GE(u), GN(v), Z + 1.05f), FVector(140, 40, 18), FLinearColor(RS.FRand() * 0.6f + 0.2f, RS.FRand() * 0.5f + 0.2f, RS.FRand() * 0.3f + 0.1f));
	}
	M.AddCylinder(W(GE(0.f), GN(2.f), Z), 1.3f, 1.3f, 1.f, 8, StoneG, true);
	M.AddBox(W(GE(0.f), GN(2.f), Z + 2.4f), FVector(160, 20, 14), Timber); for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(GE(k * 1.4f), GN(2.f), Z + 1.2f), FVector(10, 10, 240), TimberD);
	AddFire(M, GE(12.f), GN(2.f), Z, true);
	// Uylar: yog'och to'sinli, oq suvoqli, yog'och taxta tomli; halqa-halqa, orasida bog'chalar
	for (float R = 22.f; R < SogR - 6.f; R += 15.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 16.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.25f, 0.25f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			const float bv = -18.f + FMath::Sin(u * 0.06f) * 9.f + FMath::Sin(u * 0.17f + 1.f) * 3.f;
			if (FMath::Abs(v - bv) < 6.f) continue;                                            // ariq
			if (FMath::Abs(u - 8.f) < 12.f && FMath::Abs(v - 24.f) < 12.f) continue;          // masjid
			if (FMath::Abs(u - 48.f) < 6.f && FMath::Abs(v - bv - 5.f) < 6.f) continue;       // tegirmon
			if (RS.FRand() < 0.25f) continue;
			const float HU = RS.FRandRange(4.f, 6.f), HV = RS.FRandRange(4.f, 6.f);
			const float ZZ = HeightAt(GE(u), GN(v));
			AddHouse(M, GE(u), GN(v), ZZ, HU, HV, RS.FRandRange(3.f, 4.2f), FMath::RadiansToDegrees(A) + 90.f, RS.FRand() < 0.5f ? Plaster : ErtCol::Vary(Timber, 0.08f, ++S), ++S);
			if (RS.FRand() < 0.5f) { const float fu = u + FMath::Cos(A) * 7.f, fv = v + FMath::Sin(A) * 7.f; AddFenceRect(M, GE(fu), GN(fv), HeightAt(GE(fu), GN(fv)), 3.5f, 3.f, 1.f); }
		}
	}
	// Qo'ylar qo'rasi (g'arb) va pichan g'aramlari
	AddFenceRect(M, GE(-48.f), GN(12.f), HeightAt(GE(-48.f), GN(12.f)), 9.f, 7.f, 2.f);
	for (int32 i = 0; i < 9; ++i) M.AddSphere(W(GE(-48.f + RS.FRandRange(-7.f, 7.f)), GN(12.f + RS.FRandRange(-5.f, 5.f)), HeightAt(GE(-48.f), GN(12.f)) + 0.5f), 0.55f, 6, FLinearColor(0.9f, 0.88f, 0.82f), FVector(1.5f, 1, 1));
	for (int32 i = 0; i < 4; ++i) M.AddCylinder(W(GE(-40.f + i * 4.f), GN(26.f), HeightAt(GE(-40.f + i * 4.f), GN(26.f))), 1.6f, 0.3f, 2.6f, 8, ErtCol::Vary(Hay, 0.08f, ++S), true);
	// Mevazor (janub): qator-qator mevali daraxtlar; bug'doy dalasi (sharq)
	for (int32 r = 0; r < 4; ++r) for (int32 i = 0; i < 8; ++i)
	{
		const float u = -30.f + i * 7.f, v = -SogR + 4.f + r * 6.f;
		const float ZZ = HeightAt(GE(u), GN(v));
		M.AddCylinder(W(GE(u), GN(v), ZZ), 0.22f, 0.18f, 1.8f, 5, WillowT, true);
		M.AddSphere(W(GE(u), GN(v), ZZ + 2.8f), 1.8f, 7, ErtCol::Vary(Fruit, 0.1f, ++S), FVector(1, 1, 0.85f));
	}
	for (int32 r = 0; r < 6; ++r) M.AddBox(W(GE(SogR - 10.f), GN(-40.f + r * 5.f), HeightAt(GE(SogR - 10.f), GN(-40.f + r * 5.f)) + 0.4f), FVector(1400, 200, 40), ErtCol::Vary(Wheat, 0.08f, ++S));
	// Tollar: ariq bo'ylab va maydon chetlarida
	for (int32 i = 0; i < 14; ++i)
	{
		const float u = -SogR + 8.f + i * (2.f * SogR - 16.f) / 13.f + RS.FRandRange(-2.f, 2.f);
		const float bv = -18.f + FMath::Sin(u * 0.06f) * 9.f + FMath::Sin(u * 0.17f + 1.f) * 3.f;
		if (FMath::Abs(u + 22.f) < 4.f || FMath::Abs(u - 26.f) < 4.f || FMath::Abs(u - 48.f) < 6.f) continue;
		Willow(u, bv + ((i & 1) ? 4.5f : -4.5f), RS.FRandRange(0.8f, 1.2f), ++S);
	}
	for (int32 i = 0; i < 8; ++i) { const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(SogR + 2.f, SogR + 14.f); Willow(FMath::Cos(A) * R, FMath::Sin(A) * R, RS.FRandRange(0.9f, 1.3f), ++S); }
	// Kuzatuv minorasi (kirish, sharq) va Qayi tug'i
	AddWatchTower(M, GE(SogR + 4.f), GN(-4.f), HeightAt(GE(SogR + 4.f), GN(-4.f)), 7.f);
	AddBanner(M, GE(SogR + 4.f), GN(4.f), HeightAt(GE(SogR + 4.f), GN(4.f)), 6.f, FLinearColor(0.55f, 0.10f, 0.08f), true);
	M.Commit(NewPart(TEXT("Sogut"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, "\telse if (E.Region.Contains(TEXT(\"Karacahisar\"))) { PE = KarE; PN = KarN - KarR - 60.f; }   // Karacahisar etagi (janub)",
        "\telse if (E.Region.Contains(TEXT(\"Karacahisar\"))) { PE = KarE; PN = KarN - KarR - 60.f; }   // Karacahisar etagi (janub)\n\telse if (E.Region.Contains(TEXT(\"So'g'ut\")) || E.Region.Contains(TEXT(\"Sogut\")) || E.Region.Contains(TEXT(\"Domani\"))) { PE = SogE + SogR + 20.f; PN = SogN; }   // So'g'ut sharqiy kirishi")
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("karacahisar")) { E = ErtMap::KarE; Nn = ErtMap::KarN; }', '\t\telse if (Place == TEXT("karacahisar")) { E = ErtMap::KarE; Nn = ErtMap::KarN; }\n\t\telse if (Place == TEXT("sogut")) { E = ErtMap::SogE; Nn = ErtMap::SogN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(KarE, KarN, KarR, TEXT("Karacahisar"), Ink);', '\tMark(KarE, KarN, KarR, TEXT("Karacahisar"), Ink);\n\tMark(SogE, SogN, SogR, TEXT("So\'g\'ut"), Ink);')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(46.9f)) TakeShot(TEXT("karacahisar"));\n\tif (At(47.4f))', '\tif (At(46.9f)) TakeShot(TEXT("karacahisar"));\n\tif (At(47.0f)) Teleport(-220.f, 480.f, 16.f + 40.f, -17.f, 0.f);\n\tif (At(47.9f)) TakeShot(TEXT("sogut"));\n\tif (At(48.4f))')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'sogut_tegirmonchi' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'sogut_tegirmonchi', 'name': 'chr.sogut_tegirmonchi.name', 'place': 'sogut', 'u': 44, 'v': -8, 'yaw': 270, 'woman': False, 'kaftan': [0.6, 0.55, 0.45], 'dialog': 'npc_caravan'})
    npcs['npcs'].append({'id': 'sogut_savdogar', 'name': 'chr.sogut_savdogar.name', 'place': 'sogut', 'u': 0, 'v': -10, 'yaw': 0, 'woman': False, 'kaftan': [0.3, 0.45, 0.25], 'dialog': 'npc_merchant'})
    npcs['npcs'].append({'id': 'sogut_imom', 'name': 'chr.sogut_imom.name', 'place': 'sogut', 'u': 8, 'v': 16, 'yaw': 180, 'woman': False, 'kaftan': [0.9, 0.88, 0.8], 'dialog': 'npc_geyikli_baba'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.sogut_tegirmonchi.name","So\'g\'ut tegirmonchisi","Söğüt değirmencisi","Miller of Sogut"', '"chr.sogut_savdogar.name","So\'g\'ut bozorchisi","Söğüt pazarcısı","Sogut market trader"', '"chr.sogut_imom.name","So\'g\'ut imomi","Söğüt imamı","Imam of Sogut"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
