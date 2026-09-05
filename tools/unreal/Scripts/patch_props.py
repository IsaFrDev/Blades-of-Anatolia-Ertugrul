# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtFab.h')
h = rep(h, "\tTArray<UStaticMesh*> Trees, Pines, Rocks, Bushes, Grass;", "\tTArray<UStaticMesh*> Trees, Pines, Rocks, Bushes, Grass, Props, Stumps;   // Props: bochka/yashik/chelak/fonar/stol; Stumps: to'nka/yotgan tana")
save('ErtFab.h', h)
c = load('ErtFab.cpp')
c = rep(c, "\tTrees.Reset(); Pines.Reset(); Rocks.Reset(); Bushes.Reset(); Grass.Reset();", "\tTrees.Reset(); Pines.Reset(); Rocks.Reset(); Bushes.Reset(); Grass.Reset(); Props.Reset(); Stumps.Reset();")
c = rep(c, "\t\t\tFill(TEXT(\"trees\"), Trees); Fill(TEXT(\"pines\"), Pines); Fill(TEXT(\"rocks\"), Rocks); Fill(TEXT(\"bushes\"), Bushes); Fill(TEXT(\"grass\"), Grass);",
        "\t\t\tFill(TEXT(\"trees\"), Trees); Fill(TEXT(\"pines\"), Pines); Fill(TEXT(\"rocks\"), Rocks); Fill(TEXT(\"bushes\"), Bushes); Fill(TEXT(\"grass\"), Grass); Fill(TEXT(\"props\"), Props); Fill(TEXT(\"stumps\"), Stumps);")
c = rep(c, "\t\t\tTArray<UStaticMesh*>* Target = nullptr;\n\t\t\tif (Name.Contains(TEXT(\"pine\"))",
        "\t\t\tTArray<UStaticMesh*>* Target = nullptr;\n\t\t\tif (Name.Contains(TEXT(\"stump\")) || Name.Contains(TEXT(\"trunk\")) || Name.Contains(TEXT(\"log\"))) Target = &Stumps;\n\t\t\telse if (Name.Contains(TEXT(\"barrel\")) || Name.Contains(TEXT(\"crate\")) || Name.Contains(TEXT(\"bucket\")) || Name.Contains(TEXT(\"lantern\")) || Name.Contains(TEXT(\"table\")) || Name.Contains(TEXT(\"stool\")) || Name.Contains(TEXT(\"fire_pit\")) || Name.Contains(TEXT(\"shield\"))) Target = &Props;\n\t\t\telse if (Name.Contains(TEXT(\"pine\"))")
c = rep(c, "\tUE_LOG(LogErtugrul, Log, TEXT(\"Fab kutubxonasi: daraxt %d, qarag'ay %d, qoya %d, buta %d, o't %d (registrdan %d)\"), Trees.Num(), Pines.Num(), Rocks.Num(), Bushes.Num(), Grass.Num(), Found);",
        "\tUE_LOG(LogErtugrul, Log, TEXT(\"Fab kutubxonasi: daraxt %d, qarag'ay %d, qoya %d, buta %d, o't %d, buyum %d, to'nka %d (registrdan %d)\"), Trees.Num(), Pines.Num(), Rocks.Num(), Bushes.Num(), Grass.Num(), Props.Num(), Stumps.Num(), Found);")
save('ErtFab.cpp', c)

h = load('ErtWorldBuilder.h')
h = rep(h, "\tvoid BuildGrass();", "\tvoid BuildGrass();\n\tvoid BuildProps();")
save('ErtWorldBuilder.h', h)
c = load('ErtWorldBuilder.cpp')
c = rep(c, "\tif (bBuildForest) { BuildForest(); BuildRocks(); BuildGrass(); }", "\tif (bBuildForest) { BuildForest(); BuildRocks(); BuildGrass(); BuildProps(); }")
# O'rmonda to'nkalar (3%)
c = rep(c, "\t\tconst bool bPine = H > 60.f || RS.FRand() < 0.55f;\n\t\t{\n\t\t\tFErtFabLib& Fab = FErtFabLib::Get();",
        "\t\tconst bool bPine = H > 60.f || RS.FRand() < 0.55f;\n\t\t{\n\t\t\tFErtFabLib& Fab = FErtFabLib::Get();\n\t\t\tif (Fab.Stumps.Num() && RS.FRand() < 0.04f)\n\t\t\t{\n\t\t\t\tUStaticMesh* Mesh = Fab.Stumps[RS.RandRange(0, Fab.Stumps.Num() - 1)];\n\t\t\t\tFabComp(Mesh, true)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, H - 0.05f), FVector(FErtFabLib::ScaleToRadius(Mesh, RS.FRandRange(0.9f, 1.6f)))), true);\n\t\t\t\tcontinue;\n\t\t\t}")
# O't orasida butalar (har 40 tupdan bittasi buta)
c = rep(c, "\t\tFErtMeshData& M = Cells[cy * CellsPerSide + cx];\n\t\tconst FVector Base = W(E, N, H - 0.02f);",
        "\t\t{\n\t\t\tFErtFabLib& Fab = FErtFabLib::Get();\n\t\t\tif (Fab.Bushes.Num() && (Placed % 40) == 17)\n\t\t\t{\n\t\t\t\tUStaticMesh* Mesh = Fab.Bushes[RS.RandRange(0, Fab.Bushes.Num() - 1)];\n\t\t\t\tFabComp(Mesh, false)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, H - 0.03f), FVector(FErtFabLib::ScaleToHeight(Mesh, RS.FRandRange(0.5f, 1.1f)))), true);\n\t\t\t\t++Placed;\n\t\t\t\tcontinue;\n\t\t\t}\n\t\t}\n\t\tFErtMeshData& M = Cells[cy * CellsPerSide + cx];\n\t\tconst FVector Base = W(E, N, H - 0.02f);")
c += r'''
// ---------------- Buyumlar (Fab/Poly Haven): oba, qishloq, shahar maydonlari atrofida ----------------

void AErtWorldBuilder::BuildProps()
{
	FErtFabLib& Fab = FErtFabLib::Get();
	if (!Fab.Props.Num()) return;
	FRandomStream RS(Seed + 23);
	struct FHot { float E, N, R; int32 Count; };
	const FHot Hots[] = { {ObaE, ObaN, 90.f, 40}, {SogE, SogN, 50.f, 24}, {DomE, DomN, 30.f, 14}, {BurE, BurN, 70.f, 24}, {CityE, CityN, 60.f, 24}, {DamE, DamN, 80.f, 24}, {HalabE, HalabN, 60.f, 18}, {KonE, KonN, 70.f, 18}, {KayE, KayN, 60.f, 16}, {SivE, SivN, 60.f, 16}, {ErzE, ErzN, 50.f, 14}, {NikE, NikN, 60.f, 16}, {CampE, CampN, 70.f, 20}, {CaravanE, CaravanN, 30.f, 14}, {KarE, KarN - KarR - 30.f, 30.f, 10} };
	int32 Placed = 0;
	for (const FHot& Hn : Hots)
	{
		for (int32 i = 0, Tries = 0; i < Hn.Count && Tries < Hn.Count * 12; ++Tries)
		{
			const float A = RS.FRand() * 2.f * PI, R = FMath::Sqrt(RS.FRand()) * Hn.R;
			const float E = Hn.E + FMath::Cos(A) * R, N = Hn.N + FMath::Sin(A) * R;
			const float H = HeightAt(E, N);
			float SurfZ; if (IsWater(E, N, SurfZ)) continue;
			// Binolar ichiga tushmasligi uchun: yer ustidagi bo'sh joyni trace bilan tekshiramiz (mesh kolliziyasi)
			FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtProp), true);
			const FVector Wp = W(E, N, H);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Wp + FVector(0, 0, 400.f), Wp + FVector(0, 0, 20.f), ECC_Visibility, Q)) continue;   // ustida narsa bor (tom)
			if (GetWorld()->OverlapAnyTestByChannel(Wp + FVector(0, 0, 60.f), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(70.f), Q)) continue;   // devor/uy yonida
			UStaticMesh* Mesh = Fab.Props[RS.RandRange(0, Fab.Props.Num() - 1)];
			const float TargetH = RS.FRandRange(0.7f, 1.1f);
			FabComp(Mesh, true)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), Wp, FVector(FErtFabLib::ScaleToHeight(Mesh, TargetH))), true);
			++i; ++Placed;
		}
	}
	UE_LOG(LogErtugrul, Log, TEXT("Buyumlar (Fab): %d"), Placed);
}
'''
save('ErtWorldBuilder.cpp', c)
print('patched')
