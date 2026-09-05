# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def rep(p, a, b):
    s = io.open(SRC + p, encoding='utf-8').read()
    if b in s: return
    assert a in s, ("MISSING in " + p + ": " + a[:80])
    io.open(SRC + p, 'w', encoding='utf-8', newline='\n').write(s.replace(a, b))

rep('ErtFab.h', "\tTArray<UStaticMesh*> Trees, Pines, Rocks, Bushes, Grass, Props, Stumps;",
    "\tTArray<UStaticMesh*> Trees, Pines, Rocks, Bushes, Grass, Props, Stumps;\n\t/** Nomli inshoot/buyum meshlari (AssetHub/Tripo, Fab): SM_Yurt_*, SM_House_*, SM_Gate_*, SM_Well_*, SM_Cart_*, SM_Stall_*, SM_Tent_* */\n\tTArray<UStaticMesh*> Yurts, Houses, Gates, Wells, Carts, Stalls, Tents;\n\tstatic UStaticMesh* Pick(const TArray<UStaticMesh*>& L, int32 Seed) { return L.Num() ? L[FMath::Abs(Seed) % L.Num()] : nullptr; }")
rep('ErtFab.cpp', "\tTrees.Reset(); Pines.Reset(); Rocks.Reset(); Bushes.Reset(); Grass.Reset(); Props.Reset(); Stumps.Reset();",
    "\tTrees.Reset(); Pines.Reset(); Rocks.Reset(); Bushes.Reset(); Grass.Reset(); Props.Reset(); Stumps.Reset();\n\tYurts.Reset(); Houses.Reset(); Gates.Reset(); Wells.Reset(); Carts.Reset(); Stalls.Reset(); Tents.Reset();")
rep('ErtFab.cpp', "\t\t\tif (Name.Contains(TEXT(\"stump\")) || Name.Contains(TEXT(\"trunk\")) || Name.Contains(TEXT(\"log\"))) Target = &Stumps;",
    "\t\t\tif (Name.Contains(TEXT(\"yurt\")) || Name.Contains(TEXT(\"ger_\"))) Target = &Yurts;\n\t\t\telse if (Name.Contains(TEXT(\"tent\"))) Target = &Tents;\n\t\t\telse if (Name.Contains(TEXT(\"house\")) || Name.Contains(TEXT(\"hut\")) || Name.Contains(TEXT(\"cottage\"))) Target = &Houses;\n\t\t\telse if (Name.Contains(TEXT(\"gate\"))) Target = &Gates;\n\t\t\telse if (Name.Contains(TEXT(\"well\"))) Target = &Wells;\n\t\t\telse if (Name.Contains(TEXT(\"cart\")) || Name.Contains(TEXT(\"wagon\"))) Target = &Carts;\n\t\t\telse if (Name.Contains(TEXT(\"stall\")) || Name.Contains(TEXT(\"market\")) || Name.Contains(TEXT(\"bazaar\"))) Target = &Stalls;\n\t\t\telse if (Name.Contains(TEXT(\"stump\")) || Name.Contains(TEXT(\"trunk\")) || Name.Contains(TEXT(\"log\"))) Target = &Stumps;")
rep('ErtFab.cpp', "buyum %d, to'nka %d (registrdan %d)\"), Trees.Num(), Pines.Num(), Rocks.Num(), Bushes.Num(), Grass.Num(), Props.Num(), Stumps.Num(), Found);",
    "buyum %d, to'nka %d; o'tov %d, uy %d, darvoza %d, quduq %d, arava %d, rasta %d, chodir %d (registrdan %d)\"), Trees.Num(), Pines.Num(), Rocks.Num(), Bushes.Num(), Grass.Num(), Props.Num(), Stumps.Num(), Yurts.Num(), Houses.Num(), Gates.Num(), Wells.Num(), Carts.Num(), Stalls.Num(), Tents.Num(), Found);")
rep('ErtWorldBuilder.h', "\tclass UHierarchicalInstancedStaticMeshComponent* FabComp(UStaticMesh* M, bool bCollision);",
    "\tclass UHierarchicalInstancedStaticMeshComponent* FabComp(UStaticMesh* M, bool bCollision);\n\t/** Mesh instansini reja nuqtasiga qo'yish: TargetM - balandlik (bByHeight) yoki eng katta yarim o'lcham metrda; poydevori yerga tegadi */\n\tvoid FabPlace(UStaticMesh* M, float E, float N, float Z, float Yaw, float TargetM, bool bByHeight, bool bCollision);\n\tvoid BuildLandmarks();\n\tint32 FabYurts = 0, FabHouses = 0;")
rep('ErtWorldBuilder.cpp', "\tif (bBuildSettlements) BuildSplineWalls();", "\tif (bBuildSettlements) { BuildSplineWalls(); BuildLandmarks(); }")
rep('ErtWorldBuilder.cpp', "\tif (R >= 2.3f) Interiors.Add(FVector4(E, N, R - 0.7f, Z));\n\tM.AddCylinder(W(E, N, Z), R, R, WallH, 12, ErtCol::Vary(Wall, 0.08f, S), false);",
    "\tif (R >= 2.3f) Interiors.Add(FVector4(E, N, R - 0.7f, Z));\n\t{\n\t\t// AssetHub/Fab o'tov meshi bo'lsa - protsedural o'rniga (qorong'i kigiz = mo'g'ul chodiri)\n\t\tFErtFabLib& Fab = FErtFabLib::Get();\n\t\tconst bool bDark = Wall.GetLuminance() < 0.35f;\n\t\tUStaticMesh* Mesh = bDark ? FErtFabLib::Pick(Fab.Tents.Num() ? Fab.Tents : Fab.Yurts, S) : FErtFabLib::Pick(Fab.Yurts, S);\n\t\tif (Mesh) { FabPlace(Mesh, E, N, Z, DoorYaw, R, false, true); ++FabYurts; return; }\n\t}\n\tM.AddCylinder(W(E, N, Z), R, R, WallH, 12, ErtCol::Vary(Wall, 0.08f, S), false);")
rep('ErtWorldBuilder.cpp', "\tInteriors.Add(FVector4(E, N, FMath::Min(HU, HV) - 0.8f, Z));\n\tconst FRotator R(0, Yaw, 0);",
    "\tInteriors.Add(FVector4(E, N, FMath::Min(HU, HV) - 0.8f, Z));\n\t{\n\t\tFErtFabLib& Fab = FErtFabLib::Get();\n\t\tif (UStaticMesh* Mesh = FErtFabLib::Pick(Fab.Houses, S)) { FabPlace(Mesh, E, N, Z, Yaw, FMath::Max(HU, HV) * 1.1f, false, true); ++FabHouses; return; }\n\t}\n\tconst FRotator R(0, Yaw, 0);")
s = io.open(SRC + 'ErtWorldBuilder.cpp', encoding='utf-8').read()
if 'void AErtWorldBuilder::BuildLandmarks()' not in s:
    s += r'''

// ---------------- AssetHub/Fab meshlarini joylashtirish va landmarklar (darvoza, quduq, arava, rasta) ----------------

void AErtWorldBuilder::FabPlace(UStaticMesh* M, float E, float N, float Z, float Yaw, float TargetM, bool bByHeight, bool bCollision)
{
	if (!M) return;
	const FBoxSphereBounds B = M->GetBounds();
	const float Sc = bByHeight ? FErtFabLib::ScaleToHeight(M, TargetM) : FErtFabLib::ScaleToRadius(M, TargetM);
	const float BottomZ = (B.Origin.Z - B.BoxExtent.Z) * Sc;   // poydevor
	FabComp(M, bCollision)->AddInstance(FTransform(FRotator(0, Yaw, 0), W(E, N, Z) - FVector(0, 0, BottomZ + 4.f), FVector(Sc)), true);
}

void AErtWorldBuilder::BuildLandmarks()
{
	FErtFabLib& Fab = FErtFabLib::Get();
	int32 N = 0;
	auto Hz = [&](float E, float Nn) { return HeightAt(E, Nn); };
	// Darvoza: Bagras qo'rg'on devori boshida (yo'lga qaragan), shahar janubiy kirishi, Karacahisar etagi
	if (UStaticMesh* G = FErtFabLib::Pick(Fab.Gates, 0))
	{
		FabPlace(G, FortE - 80.f, FortN - 100.f, Hz(FortE - 80.f, FortN - 100.f), 200.f, 12.f, true, true); ++N;
		FabPlace(G, CityE, CityN - CityR - 6.f, Hz(CityE, CityN - CityR - 6.f), 90.f, 13.f, true, true); ++N;
		FabPlace(G, KarE, KarN - KarR - 2.f, Hz(KarE, KarN - KarR - 2.f), 90.f, 10.f, true, true); ++N;
	}
	// Quduqlar: oba, So'g'ut, Domaniç, shahar maydonlari
	if (UStaticMesh* Wl = FErtFabLib::Pick(Fab.Wells, 0))
	{
		struct FP { float E, N; };
		for (const FP& P : { FP{ObaE + 24.f, ObaN + 8.f}, FP{SogE + 6.f, SogN - 14.f}, FP{DomE + 12.f, DomN - 6.f}, FP{CityE + 22.f, CityN + 40.f}, FP{KonE + 18.f, KonN - 20.f}, FP{DamE - 20.f, DamN + 10.f}, FP{NikE + 16.f, NikN + 18.f}, FP{BurE - 14.f, BurN + 12.f} })
		{
			FabPlace(Wl, P.E, P.N, Hz(P.E, P.N), FMath::Fmod(P.E * 7.f, 360.f), 3.2f, true, true); ++N;
		}
	}
	// Aravalar: oba, So'g'ut, karvonsaroy, shahar darvozasi oldi
	if (Fab.Carts.Num())
	{
		struct FP { float E, N, Yaw; };
		int32 k = 0;
		for (const FP& P : { FP{ObaE - 40.f, ObaN - 30.f, 20.f}, FP{ObaE + 60.f, ObaN + 70.f, 110.f}, FP{SogE - 20.f, SogN + 22.f, 75.f}, FP{CaravanE + 12.f, CaravanN - 8.f, 160.f}, FP{CityE - 10.f, CityN - CityR - 16.f, 95.f}, FP{DomE - 25.f, DomN + 15.f, 40.f}, FP{KayE + 30.f, KayN - 60.f, 0.f} })
		{
			FabPlace(FErtFabLib::Pick(Fab.Carts, k++), P.E, P.N, Hz(P.E, P.N), P.Yaw, 1.9f, false, true); ++N;
		}
	}
	// Bozor rastalari: shahar bozorlari
	if (Fab.Stalls.Num())
	{
		struct FP { float E, N, Yaw; };
		int32 k = 0;
		for (const FP& P : { FP{CityE - 30.f, CityN + 30.f, 0.f}, FP{CityE - 30.f, CityN + 42.f, 0.f}, FP{CityE + 40.f, CityN + 30.f, 180.f}, FP{DamE + 30.f, DamN - 20.f, 90.f}, FP{DamE + 30.f, DamN - 32.f, 90.f}, FP{HalabE - 40.f, HalabN + 10.f, 270.f}, FP{KonE - 30.f, KonN + 30.f, 0.f}, FP{KayE - 20.f, KayN + 30.f, 0.f}, FP{SivE + 25.f, SivN - 20.f, 180.f}, FP{BurE + 30.f, BurN - 30.f, 270.f}, FP{NikE - 25.f, NikN - 30.f, 0.f}, FP{SogE + 20.f, SogN + 4.f, 90.f} })
		{
			FabPlace(FErtFabLib::Pick(Fab.Stalls, k++), P.E, P.N, Hz(P.E, P.N), P.Yaw, 3.4f, true, true); ++N;
		}
	}
	UE_LOG(LogErtugrul, Log, TEXT("Landmarklar (AssetHub/Fab): %d; o'tov meshlari %d, uy meshlari %d"), N, FabYurts, FabHouses);
}
'''
    io.open(SRC + 'ErtWorldBuilder.cpp', 'w', encoding='utf-8', newline='\n').write(s)
print('ok')
