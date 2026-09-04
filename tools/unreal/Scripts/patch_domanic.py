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
h = rep(h, "// So'g'ut qishlog'i\n", "// So'g'ut qishlog'i\n\tconstexpr float DomE = -440.f, DomN = 270.f, DomR = 90.f, DomH = 18.f;   // Domaniç yaylovi (baland o'tloq)\n")
h = rep(h, "\tvoid BuildSogut();", "\tvoid BuildSogut();\n\tvoid BuildDomanic();")
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\tBuildSogut();\n\t}", "\t\tBuildSogut();\n\t\tBuildDomanic();\n\t}")
c = rep(c, "\t// So'g'ut: yumshoq tekis vodiy", '''\t// Domaniç yaylovi: keng baland gumbaz (yassi tepa), mayin to'lqinlar
\t{
\t\tconst float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(DomE, DomN));
\t\tconst float T = Smooth01(1.f - (DD - 20.f) / (DomR + 60.f));
\t\tH += DomH * T + Noise(E, N, 0.02f) * 2.5f * T;
\t}
\t// So'g'ut: yumshoq tekis vodiy''')
c = rep(c, "\t// Daryo bo'yi\n\tconst float DR = FMath::Abs(E - RiverE(N));", '''\t{
\t\tconst float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(DomE, DomN));
\t\tconst float T = 1.f - Smooth01((DD - DomR) / 50.f);
\t\tC = FMath::Lerp(C, FLinearColor(0.40f, 0.58f, 0.22f), T * 0.7f);                                       // yam-yashil yaylov
\t\tconst float F = Noise(E, N, 0.9f);                                                                       // gul dog'lari
\t\tif (T > 0.3f && F > 0.55f) C = FMath::Lerp(C, F > 0.75f ? FLinearColor(0.95f, 0.85f, 0.25f) : FLinearColor(0.85f, 0.35f, 0.55f), 0.5f * T);
\t}
\t// Daryo bo'yi
\tconst float DR = FMath::Abs(E - RiverE(N));''')
c = rep(c, "// So'g'ut\n", "// So'g'ut\n\tif (FVector2D::Distance(FVector2D(E, N), FVector2D(DomE, DomN)) < DomR) return false;                // Domaniç (ochiq o'tloq)\n")
c += r'''
// ---------------- Domaniç: yozgi yaylov ----------------

void AErtWorldBuilder::BuildDomanic()
{
	FRandomStream RS(Seed + 271);
	FErtMeshData M(100.f);
	int32 S = 11500;
	auto DE = [](float u) { return DomE + u; };
	auto DN = [](float v) { return DomN + v; };
	auto Hz = [&](float u, float v) { return HeightAt(DomE + u, DomN + v); };
	const FLinearColor Felt(0.86f, 0.82f, 0.72f), FeltD(0.55f, 0.48f, 0.38f), DWood(0.40f, 0.27f, 0.13f), Wool(0.92f, 0.90f, 0.84f), WoolD(0.35f, 0.30f, 0.25f), Water(0.28f, 0.50f, 0.62f), StoneG(0.60f, 0.58f, 0.52f), Cheese(0.95f, 0.88f, 0.6f), DRed(0.55f, 0.10f, 0.08f);
	// Yozgi o'tovlar halqasi (8 ta kichik o'tov), markazda katta gulxan va tug'
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = i * 2.f * PI / 8 + 0.2f, R = 16.f;
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		AddYurt(M, DE(u), DN(v), Hz(u, v), 2.4f, 1.7f, 1.6f, ErtCol::Vary(Felt, 0.05f, ++S), FLinearColor(0.72f, 0.66f, 0.55f), FMath::RadiansToDegrees(A) + 180.f, ++S);
	}
	AddFire(M, DE(0.f), DN(0.f), Hz(0.f, 0.f), true);
	AddBanner(M, DE(3.f), DN(3.f), Hz(3.f, 3.f), 6.f, DRed, true);
	// Kigiz yoyilgan joy, qozon, o'tin to'plami
	M.AddBox(W(DE(-6.f), DN(4.f), Hz(-6.f, 4.f) + 0.05f), FVector(250, 180, 4), FLinearColor(0.55f, 0.15f, 0.12f));
	M.AddBox(W(DE(-6.f), DN(4.f), Hz(-6.f, 4.f) + 0.08f), FVector(200, 130, 3), FLinearColor(0.8f, 0.65f, 0.25f));
	for (int32 i = 0; i < 6; ++i) M.AddCylinder(W(DE(6.f + (i % 3) * 0.5f), DN(-5.f + (i / 3) * 0.5f), Hz(6.f, -5.f) + (i / 3) * 0.35f), 0.15f, 0.15f, 1.6f, 5, DWood, true, FRotator(0, 0, 90.f));
	// Cho'pon kulbasi (tosh, tuproq tomli) va pishloq/qurut tokchalari
	M.AddBox(W(DE(-30.f), DN(20.f), Hz(-30.f, 20.f) + 1.3f), FVector(280, 220, 130), ErtCol::Vary(StoneG, 0.08f, ++S));
	M.AddBox(W(DE(-30.f), DN(20.f), Hz(-30.f, 20.f) + 2.7f), FVector(320, 260, 14), FLinearColor(0.45f, 0.40f, 0.28f));
	for (int32 i = 0; i < 3; ++i)
	{
		M.AddBox(W(DE(-24.f), DN(16.f + i * 2.5f), Hz(-24.f, 16.f) + 1.2f), FVector(120, 30, 4), DWood);
		for (int32 k = -1; k <= 1; k += 2) M.AddCylinder(W(DE(-24.f + k * 1.1f), DN(16.f + i * 2.5f), Hz(-24.f, 16.f)), 0.06f, 0.06f, 1.3f, 4, DWood, false);
		for (int32 j = 0; j < 4; ++j) M.AddSphere(W(DE(-24.6f + j * 0.4f), DN(16.f + i * 2.5f), Hz(-24.f, 16.f) + 1.3f), 0.14f, 5, Cheese, FVector(1, 1, 0.6f));
	}
	// Qo'y qo'rasi va suruv (~40 qo'y), ikki cho'pon iti, cho'pon (tayoq)
	AddFenceRect(M, DE(30.f), DN(-18.f), Hz(30.f, -18.f), 14.f, 10.f, 2.5f);
	for (int32 i = 0; i < 40; ++i)
	{
		const float u = 30.f + RS.FRandRange(-12.f, 12.f), v = -18.f + RS.FRandRange(-8.f, 8.f);
		const bool bBlack = RS.FRand() < 0.15f;
		M.AddSphere(W(DE(u), DN(v), Hz(u, v) + 0.55f), 0.5f, 6, ErtCol::Vary(bBlack ? WoolD : Wool, 0.05f, ++S), FVector(1.6f, 1.f, 1.f));
		M.AddBox(W(DE(u + 0.5f), DN(v), Hz(u, v) + 0.75f), FVector(18, 12, 14), bBlack ? WoolD * 0.8f : FLinearColor(0.2f, 0.16f, 0.12f));
	}
	for (int32 i = 0; i < 2; ++i) { const float u = 30.f + (i ? 16.f : -16.f), v = -18.f + (i ? 10.f : -10.f); M.AddBox(W(DE(u), DN(v), Hz(u, v) + 0.5f), FVector(45, 18, 22), FLinearColor(0.6f, 0.5f, 0.35f)); M.AddBox(W(DE(u + 0.5f), DN(v), Hz(u, v) + 0.7f), FVector(16, 12, 14), FLinearColor(0.6f, 0.5f, 0.35f)); }
	{
		const float u = 14.f, v = -18.f, ZZ = Hz(u, v);
		M.AddCylinder(W(DE(u), DN(v), ZZ), 0.24f, 0.22f, 1.55f, 6, FLinearColor(0.45f, 0.35f, 0.25f), true);
		M.AddSphere(W(DE(u), DN(v), ZZ + 1.75f), 0.18f, 6, FLinearColor(0.8f, 0.65f, 0.5f));
		M.AddCylinder(W(DE(u), DN(v), ZZ + 1.85f), 0.2f, 0.2f, 0.3f, 6, Felt, true);
		M.AddCylinder(W(DE(u + 0.4f), DN(v), ZZ), 0.03f, 0.03f, 2.f, 4, DWood, false);
	}
	// Yilqi uyuri (statik otlar), g'arbda
	const FLinearColor Coats[] = { FLinearColor(0.36f, 0.22f, 0.11f), FLinearColor(0.15f, 0.12f, 0.10f), FLinearColor(0.75f, 0.70f, 0.62f), FLinearColor(0.55f, 0.38f, 0.22f) };
	for (int32 i = 0; i < 9; ++i)
	{
		const float u = -40.f + RS.FRandRange(-14.f, 14.f), v = -26.f + RS.FRandRange(-12.f, 12.f);
		AddHorse(M, DE(u), DN(v), Hz(u, v), RS.FRandRange(0.f, 360.f), Coats[i % 4]);
	}
	// Buloq havzasi (tosh o'ralgan) va undan oqib chiqadigan kichik jilg'a
	{
		const float u = 22.f, v = 26.f, ZZ = Hz(u, v);
		for (int32 i = 0; i < 14; ++i) { const float A = i * 2.f * PI / 14; M.AddSphere(W(DE(u + FMath::Cos(A) * 3.2f), DN(v + FMath::Sin(A) * 3.2f), ZZ + 0.25f), 0.4f, 6, ErtCol::Vary(StoneG, 0.1f, ++S), FVector(1.2f, 1, 0.7f)); }
		M.AddCylinder(W(DE(u), DN(v), ZZ - 0.2f), 3.f, 3.f, 0.35f, 14, Water, true);
		for (int32 i = 0; i < 12; ++i) { const float ju = u + 3.5f + i * 2.2f, jv = v + FMath::Sin(i * 0.6f) * 1.5f; M.AddBox(W(DE(ju), DN(jv), Hz(ju, jv) - 0.12f), FVector(120, 45, 8), ErtCol::Vary(Water, 0.05f, ++S), FRotator(0, FMath::Cos(i * 0.6f) * 25.f, 0)); }
	}
	// Chetdagi qarag'ay to'plari va yakka archalar
	for (int32 i = 0; i < 26; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(DomR + 2.f, DomR + 40.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		AddTree(M, DE(u), DN(v), Hz(u, v), RS.FRandRange(0.9f, 1.5f), true, ++S);
	}
	for (int32 i = 0; i < 5; ++i) { const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(40.f, DomR - 10.f); const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R; if (FMath::Abs(u + 40.f) < 20.f && FMath::Abs(v + 26.f) < 18.f) continue; AddTree(M, DE(u), DN(v), Hz(u, v), RS.FRandRange(0.7f, 1.1f), true, ++S); }
	// Yovvoyi gullar (tup-tup rangli mayda kublar)
	for (int32 i = 0; i < 260; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = FMath::Sqrt(RS.FRand()) * (DomR - 4.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FVector2D(u, v).Size() < 22.f || (FMath::Abs(u - 30.f) < 16.f && FMath::Abs(v + 18.f) < 12.f)) continue;
		const int32 Kind = RS.RandRange(0, 3);
		const FLinearColor Col = Kind == 0 ? FLinearColor(0.95f, 0.85f, 0.2f) : Kind == 1 ? FLinearColor(0.85f, 0.3f, 0.5f) : Kind == 2 ? FLinearColor(0.95f, 0.95f, 0.9f) : FLinearColor(0.45f, 0.35f, 0.85f);
		M.AddBox(W(DE(u), DN(v), Hz(u, v) + 0.22f), FVector(9, 9, 6), Col);
		M.AddBox(W(DE(u), DN(v), Hz(u, v) + 0.1f), FVector(2, 2, 12), FLinearColor(0.25f, 0.45f, 0.15f));
	}
	// Tepadagi kuzatuv posti va yo'l boshidagi tosh belgi
	AddWatchTower(M, DE(0.f), DN(DomR - 30.f), Hz(0.f, DomR - 30.f), 6.f);
	M.AddBox(W(DE(40.f), DN(46.f), Hz(40.f, 46.f) + 0.9f), FVector(30, 30, 90), StoneG);
	M.Commit(NewPart(TEXT("Domanic"), true), 0, true);
}
'''
save('ErtWorldBuilder.cpp', c)

c = load('ErtMission.cpp')
c = rep(c, '\telse if (E.Region.Contains(TEXT("So\'g\'ut")) || E.Region.Contains(TEXT("Sogut")) || E.Region.Contains(TEXT("Domani"))) { PE = SogE + SogR + 20.f; PN = SogN; }   // So\'g\'ut sharqiy kirishi',
        '\telse if (E.Region.Contains(TEXT("So\'g\'ut")) || E.Region.Contains(TEXT("Sogut"))) { PE = SogE + SogR + 20.f; PN = SogN; }   // So\'g\'ut sharqiy kirishi\n\telse if (E.Region.Contains(TEXT("Domani"))) { PE = DomE + 40.f; PN = DomN + 50.f; }   // Domaniç yaylovi (yo\'l boshi)')
save('ErtMission.cpp', c)
c = load('ErtGameMode.cpp')
c = rep(c, '\t\telse if (Place == TEXT("sogut")) { E = ErtMap::SogE; Nn = ErtMap::SogN; }', '\t\telse if (Place == TEXT("sogut")) { E = ErtMap::SogE; Nn = ErtMap::SogN; }\n\t\telse if (Place == TEXT("domanic")) { E = ErtMap::DomE; Nn = ErtMap::DomN; }')
save('ErtGameMode.cpp', c)
c = load('ErtHUD.cpp')
c = rep(c, '\tMark(SogE, SogN, SogR, TEXT("So\'g\'ut"), Ink);', '\tMark(SogE, SogN, SogR, TEXT("So\'g\'ut"), Ink);\n\tMark(DomE, DomN, DomR, TEXT("Domaniç yaylovi"), FLinearColor(0.25f, 0.45f, 0.2f));')
save('ErtHUD.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, '\tif (At(47.9f)) TakeShot(TEXT("sogut"));\n\tif (At(48.4f))', '\tif (At(47.9f)) TakeShot(TEXT("sogut"));\n\tif (At(48.0f)) Teleport(-440.f, 150.f, 60.f, -16.f, 0.f);\n\tif (At(48.9f)) TakeShot(TEXT("domanic"));\n\tif (At(49.4f))')
save('ErtCharacter.cpp', c)
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
npcs = json.load(io.open(DATA + 'npcs.json', encoding='utf-8'))
if not any(n['id'] == 'domanic_chopon' for n in npcs['npcs']):
    npcs['npcs'].append({'id': 'domanic_chopon', 'name': 'chr.domanic_chopon.name', 'place': 'domanic', 'u': 10, 'v': -10, 'yaw': 90, 'woman': False, 'kaftan': [0.45, 0.38, 0.28], 'dialog': 'npc_caravan'})
    npcs['npcs'].append({'id': 'domanic_kampir', 'name': 'chr.domanic_kampir.name', 'place': 'domanic', 'u': -6, 'v': 6, 'yaw': 180, 'woman': True, 'kaftan': [0.5, 0.2, 0.2], 'dialog': 'npc_hayme_ana'})
io.open(DATA + 'npcs.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(npcs, ensure_ascii=False, indent=1))
loc = io.open(DATA + 'npc_loc.csv', encoding='utf-8').read()
for r in ['"chr.domanic_chopon.name","Yaylov cho\'poni","Yayla çobanı","Highland shepherd"', '"chr.domanic_kampir.name","Yaylov kampiri","Yayla ninesi","Highland grandmother"']:
    if r.split(',')[0] not in loc: loc += r + '\n'
io.open(DATA + 'npc_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
