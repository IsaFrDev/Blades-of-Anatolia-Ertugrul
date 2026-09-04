#include "ErtWorldBuilder.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"

using namespace ErtMap;

namespace
{
	inline FVector W(float E, float N, float Z) { return AErtWorldBuilder::PlanToWorld(E, N, Z); }
	inline float Noise(float E, float N, float Freq) { return FMath::PerlinNoise2D(FVector2D(E * Freq + 31.7f, N * Freq - 12.3f)); }
	inline float Smooth01(float X) { X = FMath::Clamp(X, 0.f, 1.f); return X * X * (3.f - 2.f * X); }
	// Yo'l segmentlari (E,N) va kengligi
	struct FRoadSeg { FVector2D A, B; float Wd; };
	const FRoadSeg GRoads[] = {
		{{-560, 435}, {-300, 250}, 6}, {{-300, 250}, {10, 40}, 6},
		{{10, 40}, {-150, -150}, 6}, {{-150, -150}, {-470, -280}, 6},
		{{10, 40}, {250, -200}, 6}, {{250, -200}, {370, -460}, 6},
		{{10, 40}, {300, 300}, 5}, {{300, 300}, {500, 520}, 5}, {{500, 520}, {620, 626}, 5},
		{{-560, 435}, {-560, 560}, 4}, {{-560, 560}, {-560, 680}, 4},
		{{300, 300}, {275, 480}, 6},
		{{-470, -280}, {-470, -470}, 8}, {{-470, -470}, {-470, -660}, 8}, {{-660, -470}, {-280, -470}, 8},
	};
	const FLinearColor Stone(0.46f, 0.44f, 0.41f), Wood(0.36f, 0.24f, 0.13f), DarkWood(0.24f, 0.16f, 0.09f);
	const FLinearColor Felt(0.86f, 0.82f, 0.72f), FeltDark(0.24f, 0.22f, 0.20f), Cream(0.90f, 0.86f, 0.74f);
	const FLinearColor Gold(0.95f, 0.76f, 0.22f), Red(0.55f, 0.10f, 0.08f), Ochre(0.74f, 0.63f, 0.46f);
	const FLinearColor Flame(1.0f, 0.55f, 0.08f), Ember(0.9f, 0.25f, 0.05f), Straw(0.80f, 0.70f, 0.40f);
}

AErtWorldBuilder::AErtWorldBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

void AErtWorldBuilder::OnConstruction(const FTransform& T)
{
	Super::OnConstruction(T);
	if (!bBuilt) Build();
}

void AErtWorldBuilder::BeginPlay()
{
	Super::BeginPlay();
	if (!bBuilt) Build();
}

void AErtWorldBuilder::Rebuild()
{
	Clear();
	Build();
}

void AErtWorldBuilder::Clear()
{
	for (UProceduralMeshComponent* P : Parts) if (P) P->DestroyComponent();
	for (UPointLightComponent* L : Lights) if (L) L->DestroyComponent();
	Parts.Reset(); Lights.Reset();
	bBuilt = false;
}

UProceduralMeshComponent* AErtWorldBuilder::NewPart(const FString& Name, bool bCollision, UMaterialInterface* M)
{
	UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, *Name);
	P->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	P->SetupAttachment(RootComponent);
	P->SetMobility(EComponentMobility::Static);
	P->bUseAsyncCooking = false;
	P->bUseComplexAsSimpleCollision = true;
	if (bCollision) { P->SetCollisionProfileName(TEXT("BlockAll")); P->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }
	else P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	P->RegisterComponent();
	P->SetMaterial(0, M ? M : Mat.Get());
	Parts.Add(P);
	return P;
}

void AErtWorldBuilder::Build()
{
	const double T0 = FPlatformTime::Seconds();
	Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	if (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
	WaterMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtWater.M_ErtWater"));
	if (!WaterMat) WaterMat = Mat;

	BuildTerrain();
	BuildWater();
	if (bBuildSettlements)
	{
		BuildOba();
		BuildFortress();
		BuildCity();
		BuildCamp();
		BuildDesert();
		BuildDamascus();
		BuildHalab();
		BuildKonya();
	}
	if (bBuildForest) { BuildForest(); BuildRocks(); }
	bBuilt = true;
	int32 Tris = 0;
	for (UProceduralMeshComponent* P : Parts) for (int32 s = 0; s < P->GetNumSections(); ++s) if (auto* Sec = P->GetProcMeshSection(s)) Tris += Sec->ProcIndexBuffer.Num() / 3;
	UE_LOG(LogErtugrul, Log, TEXT("Dunyo qurildi: %d qism, %d uchburchak, %.2f s"), Parts.Num(), Tris, FPlatformTime::Seconds() - T0);
}

// ---------------- Relyef ----------------

float AErtWorldBuilder::RiverE(float N) const
{
	return -820.f + 70.f * FMath::Sin(N / 260.f) + 28.f * FMath::Sin(N / 90.f + 1.3f);
}

float AErtWorldBuilder::RoadDist(float E, float N, float* OutWidth) const
{
	float Best = 1e9f, Wd = 6.f;
	const FVector2D P(E, N);
	for (const FRoadSeg& R : GRoads)
	{
		const FVector2D AB = R.B - R.A;
		const float T = FMath::Clamp(FVector2D::DotProduct(P - R.A, AB) / FMath::Max(1.f, AB.SizeSquared()), 0.f, 1.f);
		const float D = FVector2D::Distance(P, R.A + AB * T);
		if (D < Best) { Best = D; Wd = R.Wd; }
	}
	if (OutWidth) *OutWidth = Wd;
	return Best;
}

float AErtWorldBuilder::HeightAt(float E, float N) const
{
	float H = 8.f + 6.f * Noise(E, N, 0.0035f) + 2.2f * Noise(E, N, 0.012f) + 0.5f * Noise(E, N, 0.05f);

	// Shimoli-sharq tog' massivi
	const float DF = FVector2D::Distance(FVector2D(E, N), FVector2D(FortE, FortN));
	const float Massif = Smooth01((E - 60.f) / 260.f) * Smooth01((N - 120.f) / 260.f);
	H += Massif * (55.f + 45.f * FMath::Max(0.f, Noise(E, N, 0.006f)) + 12.f * FMath::Abs(Noise(E, N, 0.02f)));
	H += FMath::Exp(-FMath::Square(DF / 330.f)) * 190.f;
	// Qal'a tekisligi
	{
		const float Flat = 1.f - Smooth01((DF - (FortHalf + 18.f)) / 40.f);
		H = FMath::Lerp(H, FortZ, Flat);
	}
	// Oba, shahar, lager, chorraha tekisliklari
	auto FlatDisk = [&](float CE, float CN, float R, float Z, float Blend)
	{
		const float D = FVector2D::Distance(FVector2D(E, N), FVector2D(CE, CN));
		H = FMath::Lerp(H, Z, 1.f - Smooth01((D - R) / Blend));
	};
	{
		const float DU = FMath::Max(FMath::Abs(E - ObaE), FMath::Abs(N - ObaN));
		H = FMath::Lerp(H, ObaZ, 1.f - Smooth01((DU - (ObaHalf + 12.f)) / 45.f));
	}
	FlatDisk(CityE, CityN, CityR + 22.f, CityZ, 50.f);
	FlatDisk(CampE, CampN, CampR + 18.f, CampZ, 45.f);
	FlatDisk(CrossE, CrossN, 55.f, CrossZ, 60.f);
	// Shimoliy tog' tizmasi (N > 850)
	H += Smooth01((N - 850.f) / 130.f) * (55.f + 45.f * FMath::Abs(Noise(E, N, 0.008f)) + 10.f * FMath::Abs(Noise(E, N, 0.03f)));
	// Janubiy cho'l: barxanlar (N < -700)
	{
		const float Des = Smooth01((-N - 700.f) / 140.f);
		const float Dune = 10.f + 5.f * Noise(E, N, 0.004f) + 4.f * FMath::Abs(Noise(E + 300.f, N, 0.012f)) + 1.2f * FMath::Abs(Noise(E, N, 0.045f));
		H = FMath::Lerp(H, Dune, Des);
	}
	// Konya: tekis aylana + markazda past Aloiddin tepaligi
	{
		const float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(KonE, KonN));
		H = FMath::Lerp(H, KonZ, 1.f - Smooth01((DD - KonR - 10.f) / 50.f));
		H += KonHillH * Smooth01(1.f - (DD - 12.f) / (KonHillR - 12.f));
	}
	// Halab: tekis aylana + markazda qal'a tepaligi (glasis)
	{
		const float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(HalabE, HalabN));
		H = FMath::Lerp(H, HalabZ, 1.f - Smooth01((DD - HalabR - 10.f) / 45.f));
		H += HalabMoundH * Smooth01(1.f - (DD - 6.f) / (HalabMoundR - 6.f));
	}
	// Damashq: to'g'ri burchakli tekis maydon
	{
		const float DX = FMath::Max(0.f, FMath::Abs(E - DamE) - DamHalfE), DY = FMath::Max(0.f, FMath::Abs(N - DamN) - DamHalfN);
		const float DD = FMath::Sqrt(DX * DX + DY * DY);
		H = FMath::Lerp(H, DamZ, 1.f - Smooth01((DD - 15.f) / 45.f));
	}
	// Voha ko'li va oba yonidagi ko'l: qirg'oq tekis, o'rtasi chuqur
	{
		const float D = FVector2D::Distance(FVector2D(E, N), FVector2D(OasisE, OasisN));
		H = FMath::Lerp(H, OasisZ + 1.f, 1.f - Smooth01((D - (OasisR + 12.f)) / 40.f));
		if (D < OasisR) H -= 5.f * Smooth01(1.f - D / OasisR);
		const float DL = FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN));
		H = FMath::Lerp(H, LakeZ + 1.f, 1.f - Smooth01((DL - (LakeR + 15.f)) / 35.f));
		if (DL < LakeR) H -= 4.5f * Smooth01(1.f - DL / LakeR);
	}
	// Daryo (g'arb): qirg'oq tekisligi va o'zan
	{
		const float D = FMath::Abs(E - RiverE(N));
		H = FMath::Lerp(H, 6.f, 1.f - Smooth01((D - 20.f) / 55.f));
		if (D < 24.f) H -= 8.f * Smooth01(1.f - D / 24.f);
	}
	// Yo'llar biroz tekislanadi
	{
		float Wd; const float D = RoadDist(E, N, &Wd);
		if (D < Wd * 2.f) H -= 0.25f * (1.f - D / (Wd * 2.f));
	}
	return H;
}

FVector AErtWorldBuilder::TerrainNormal(float E, float N) const
{
	const float D = 2.f;
	const float dN = (HeightAt(E, N + D) - HeightAt(E, N - D)) / (2.f * D);
	const float dE = (HeightAt(E + D, N) - HeightAt(E - D, N)) / (2.f * D);
	return FVector(-dN, -dE, 1.f).GetSafeNormal();
}

FLinearColor AErtWorldBuilder::TerrainColor(float E, float N, float H, float Slope) const
{
	const float Nz = Noise(E, N, 0.03f);
	FLinearColor Grass = FLinearColor(0.27f, 0.40f, 0.12f) * (1.f + 0.12f * Nz);
	const FLinearColor Dry(0.50f, 0.44f, 0.20f);
	const float DryMix = Smooth01((-N + 100.f) / 500.f) * 0.7f + 0.3f * Smooth01((Nz - 0.3f) / 0.4f);
	FLinearColor C = FMath::Lerp(Grass, Dry, DryMix);
	const FLinearColor Rock(0.43f, 0.41f, 0.38f);
	C = FMath::Lerp(C, Rock * (1.f + 0.1f * Nz), Smooth01((Slope - 0.42f) / 0.25f));
	const float SnowLine = 150.f + 12.f * Nz;
	C = FMath::Lerp(C, FLinearColor(0.93f, 0.94f, 0.97f), Smooth01((H - SnowLine) / 25.f) * (1.f - Smooth01((Slope - 0.9f) / 0.3f)));
	// Cho'l qumi (janub) va voha atrofidagi yashillik
	{
		const float Des = Smooth01((-N - 700.f) / 140.f);
		const FLinearColor Sand = FLinearColor(0.80f, 0.68f, 0.44f) * (1.f + 0.07f * Nz + 0.05f * Noise(E, N, 0.15f));
		C = FMath::Lerp(C, Sand, Des);
		const float DO = FVector2D::Distance(FVector2D(E, N), FVector2D(OasisE, OasisN));
		C = FMath::Lerp(C, FLinearColor(0.30f, 0.45f, 0.14f), (1.f - Smooth01((DO - OasisR - 4.f) / 22.f)) * Des);
		C = FMath::Lerp(C, FLinearColor(0.55f, 0.50f, 0.34f), 1.f - Smooth01((DO - OasisR + 4.f) / 10.f));
	}
	{
		const float DX = FMath::Max(0.f, FMath::Abs(E - DamE) - DamHalfE), DY = FMath::Max(0.f, FMath::Abs(N - DamN) - DamHalfN);
		C = FMath::Lerp(C, FLinearColor(0.70f, 0.62f, 0.48f), 1.f - Smooth01((FMath::Sqrt(DX * DX + DY * DY) - 2.f) / 8.f));
	}
	{
		const float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(HalabE, HalabN));
		C = FMath::Lerp(C, FLinearColor(0.62f, 0.56f, 0.46f), 1.f - Smooth01((DD - HalabR) / 8.f));
		C = FMath::Lerp(C, FLinearColor(0.55f, 0.50f, 0.42f), 1.f - Smooth01((DD - 8.f) / (HalabMoundR - 8.f)));   // glasis tosh
	}
	{
		const float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(KonE, KonN));
		C = FMath::Lerp(C, FLinearColor(0.66f, 0.60f, 0.50f), 1.f - Smooth01((DD - KonR) / 8.f));
		C = FMath::Lerp(C, FLinearColor(0.42f, 0.52f, 0.30f), 1.f - Smooth01((DD - 14.f) / (KonHillR - 14.f)));   // tepalik o'tloq
	}
	// Daryo bo'yi
	const float DR = FMath::Abs(E - RiverE(N));
	C = FMath::Lerp(C, FLinearColor(0.60f, 0.54f, 0.38f), 1.f - Smooth01((DR - 22.f) / 14.f));
	C = FMath::Lerp(C, FLinearColor(0.33f, 0.29f, 0.21f), 1.f - Smooth01((DR - 12.f) / 8.f));
	// Yo'l
	float Wd; const float D = RoadDist(E, N, &Wd);
	C = FMath::Lerp(C, FLinearColor(0.42f, 0.34f, 0.22f) * (1.f + 0.08f * Nz), 1.f - Smooth01((D - Wd * 0.5f) / (Wd * 0.4f)));
	C.A = 1.f;
	return C;
}

void AErtWorldBuilder::BuildTerrain()
{
	const float Half = WorldSizeM * 0.5f;
	const int32 Chunks = FMath::Max(1, FMath::RoundToInt(WorldSizeM / ChunkM));
	const int32 Cells = FMath::Max(1, FMath::RoundToInt(ChunkM / CellM));
	const int32 Wd = Cells + 1;
	TArray<FVector> V; TArray<FLinearColor> C; TArray<FVector> Nrm;
	for (int32 cy = 0; cy < Chunks; ++cy)
		for (int32 cx = 0; cx < Chunks; ++cx)
		{
			V.Reset(); C.Reset(); Nrm.Reset();
			for (int32 y = 0; y < Wd; ++y)
				for (int32 x = 0; x < Wd; ++x)
				{
					const float E = -Half + cx * ChunkM + x * CellM;
					const float N = -Half + cy * ChunkM + y * CellM;
					const float H = HeightAt(E, N);
					const FVector Nm = TerrainNormal(E, N);
					V.Add(W(E, N, H));
					Nrm.Add(Nm);
					C.Add(TerrainColor(E, N, H, 1.f - Nm.Z));
				}
			FErtMeshData M(100.f);
			M.AddGrid(V, C, Wd, Wd);
			M.Normals = Nrm;
			M.Commit(NewPart(FString::Printf(TEXT("Terrain_%d_%d"), cx, cy), true), 0, true);
		}
}

void AErtWorldBuilder::BuildWater()
{
	FErtMeshData M(100.f);
	const float Half = WorldSizeM * 0.5f;
	const FLinearColor Wc(0.10f, 0.26f, 0.36f, 0.6f);
	for (float N = -Half; N < Half; N += 20.f)
	{
		const float E0 = RiverE(N), E1 = RiverE(N + 20.f);
		M.AddQuad(W(E0 - 27.f, N, WaterZ), W(E0 + 27.f, N, WaterZ), W(E1 + 27.f, N + 20.f, WaterZ), W(E1 - 27.f, N + 20.f, WaterZ), FVector::UpVector, Wc);
	}
	// Oba yonidagi ko'l va cho'ldagi voha
	auto Disk = [&](float CE, float CN, float R, float Z)
	{
		for (int32 i = 0; i < 28; ++i)
		{
			const float A0 = 2.f * PI * i / 28, A1 = 2.f * PI * (i + 1) / 28;
			M.AddTri(W(CE, CN, Z), W(CE + FMath::Cos(A0) * R, CN + FMath::Sin(A0) * R, Z), W(CE + FMath::Cos(A1) * R, CN + FMath::Sin(A1) * R, Z), FVector::UpVector, Wc);
		}
	};
	Disk(LakeE, LakeN, LakeR + 6.f, LakeZ);
	Disk(OasisE, OasisN, OasisR + 4.f, OasisZ);
	M.Commit(NewPart(TEXT("Water"), false, WaterMat), 0, false);
}

// ---------------- Elementlar ----------------

void AErtWorldBuilder::AddYurt(FErtMeshData& M, float E, float N, float Z, float R, float WallH, float RoofH, const FLinearColor& Wall, const FLinearColor& Roof, float DoorYaw, int32 S)
{
	M.AddCylinder(W(E, N, Z), R, R, WallH, 12, ErtCol::Vary(Wall, 0.08f, S), false);
	M.AddCylinder(W(E, N, Z + WallH), R * 1.08f, R * 0.12f, RoofH, 12, ErtCol::Vary(Roof, 0.08f, S + 1), true);
	M.AddCylinder(W(E, N, Z + WallH + RoofH - 0.2f), R * 0.16f, R * 0.16f, 0.5f, 8, DarkWood);
	const float Rad = FMath::DegreesToRadians(DoorYaw);
	const FVector DoorPos = W(E + FMath::Cos(Rad) * (R - 0.05f), N + FMath::Sin(Rad) * (R - 0.05f), Z + 0.9f);
	M.AddBox(DoorPos, FVector(50, 15, 90), DarkWood, FRotator(0, 90.f - DoorYaw, 0));
	M.AddBox(DoorPos + FVector(0, 0, 90.f), FVector(55, 20, 12), Wood, FRotator(0, 90.f - DoorYaw, 0));
}

void AErtWorldBuilder::AddTree(FErtMeshData& M, float E, float N, float Z, float Sc, bool bPine, int32 S)
{
	const FLinearColor Trunk = ErtCol::Vary(FLinearColor(0.30f, 0.20f, 0.11f), 0.15f, S);
	if (bPine)
	{
		const FLinearColor Leaf = ErtCol::Vary(FLinearColor(0.09f, 0.24f, 0.10f), 0.18f, S + 7);
		M.AddCylinder(W(E, N, Z - 0.3f), 0.32f * Sc, 0.22f * Sc, 4.2f * Sc, 5, Trunk, false);
		M.AddCone(W(E, N, Z + 2.2f * Sc), 2.6f * Sc, 3.4f * Sc, 7, Leaf);
		M.AddCone(W(E, N, Z + 4.4f * Sc), 2.1f * Sc, 3.0f * Sc, 7, Leaf * 1.08f);
		M.AddCone(W(E, N, Z + 6.4f * Sc), 1.5f * Sc, 2.6f * Sc, 6, Leaf * 1.15f);
	}
	else
	{
		const FLinearColor Leaf = ErtCol::Vary(FLinearColor(0.20f, 0.38f, 0.12f), 0.2f, S + 7);
		M.AddCylinder(W(E, N, Z - 0.3f), 0.4f * Sc, 0.28f * Sc, 3.2f * Sc, 6, Trunk, false);
		M.AddSphere(W(E, N, Z + 4.4f * Sc), 2.6f * Sc, 8, Leaf, FVector(1.f, 1.f, 0.85f), 0.12f, S);
		M.AddSphere(W(E + 1.2f * Sc, N + 0.6f * Sc, Z + 5.6f * Sc), 1.8f * Sc, 7, Leaf * 1.1f, FVector::OneVector, 0.15f, S + 3);
	}
}

void AErtWorldBuilder::AddWatchTower(FErtMeshData& M, float E, float N, float Z, float H)
{
	for (int32 i = 0; i < 4; ++i)
	{
		const float dU = (i & 1) ? 1.6f : -1.6f, dV = (i & 2) ? 1.6f : -1.6f;
		M.AddCylinder(W(E + dU, N + dV, Z), 0.22f, 0.2f, H, 6, Wood, false);
		if (i < 2) M.AddBox(W(E + dU, N, Z + H * 0.45f), FVector(8, 160, 8), Wood);
		else M.AddBox(W(E, N + dV, Z + H * 0.6f), FVector(160, 8, 8), Wood);
	}
	M.AddBox(W(E, N, Z + H), FVector(240, 240, 15), DarkWood);
	for (int32 i = 0; i < 4; ++i)
	{
		const float dU = (i & 1) ? 2.3f : -2.3f, dV = (i & 2) ? 2.3f : -2.3f;
		M.AddCylinder(W(E + dU, N + dV, Z + H), 0.08f, 0.08f, 1.1f, 4, Wood, false);
	}
	M.AddBox(W(E, N + 2.3f, Z + H + 1.0f), FVector(230, 6, 6), Wood);
	M.AddBox(W(E, N - 2.3f, Z + H + 1.0f), FVector(230, 6, 6), Wood);
	M.AddBox(W(E + 2.3f, N, Z + H + 1.0f), FVector(6, 230, 6), Wood);
	M.AddBox(W(E - 2.3f, N, Z + H + 1.0f), FVector(6, 230, 6), Wood);
	M.AddCylinder(W(E, N, Z + H + 1.1f), 0.1f, 0.1f, 2.2f, 4, Wood, false);
	M.AddCone(W(E, N, Z + H + 2.2f), 3.0f, 1.6f, 8, Straw * 0.9f);
}

void AErtWorldBuilder::AddFire(FErtMeshData& M, float E, float N, float Z, bool bLight)
{
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = 2.f * PI * i / 8;
		M.AddSphere(W(E + FMath::Cos(A) * 1.0f, N + FMath::Sin(A) * 1.0f, Z + 0.15f), 0.25f, 6, Stone * 0.9f, FVector(1, 1, 0.7f), 0.15f, i);
	}
	M.AddCylinder(W(E - 0.6f, N, Z + 0.15f), 0.12f, 0.12f, 1.2f, 5, DarkWood, true, FRotator(0, 0, 90));
	M.AddCylinder(W(E, N - 0.6f, Z + 0.25f), 0.12f, 0.12f, 1.2f, 5, DarkWood, true, FRotator(-90, 0, 0));
	M.AddCone(W(E, N, Z + 0.2f), 0.45f, 1.1f, 6, Flame);
	M.AddCone(W(E + 0.15f, N - 0.1f, Z + 0.2f), 0.3f, 1.5f, 5, Ember);
	if (bLight)
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(this, *FString::Printf(TEXT("Fire_%d"), Lights.Num()));
		L->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Static);
		L->SetRelativeLocation(W(E, N, Z + 1.4f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(900.f);
		L->SetLightColor(FLinearColor(1.f, 0.62f, 0.28f));
		L->SetAttenuationRadius(1800.f);
		L->SetCastShadows(false);
		L->RegisterComponent();
		Lights.Add(L);
	}
}

void AErtWorldBuilder::AddHorse(FErtMeshData& M, float E, float N, float Z, float Yaw, const FLinearColor& C)
{
	const FRotator R(0, Yaw, 0);
	const FQuat Q = R.Quaternion();
	const FVector O = W(E, N, Z);
	auto L = [&](float X, float Y, float Zz) { return O + Q.RotateVector(FVector(X, Y, Zz)); };
	for (int32 i = 0; i < 4; ++i)
		M.AddCylinder(L((i & 1) ? 55.f : -55.f, (i & 2) ? 18.f : -18.f, 0), 0.06f, 0.07f, 0.95f, 5, C * 0.85f, false, R);
	M.AddBox(L(0, 0, 118), FVector(75, 24, 26), C, R);
	M.AddBox(L(80, 0, 150), FVector(18, 12, 34), C, R + FRotator(35, 0, 0));
	M.AddBox(L(100, 0, 180), FVector(28, 10, 12), C * 0.95f, R + FRotator(-10, 0, 0));
	M.AddBox(L(-78, 0, 110), FVector(6, 4, 38), FLinearColor(0.12f, 0.09f, 0.06f), R + FRotator(-25, 0, 0));
	M.AddBox(L(30, 0, 152), FVector(45, 3, 8), FLinearColor(0.12f, 0.09f, 0.06f), R);
}

void AErtWorldBuilder::AddBanner(FErtMeshData& M, float E, float N, float Z, float H, const FLinearColor& Flag, bool bTugh)
{
	M.AddCylinder(W(E, N, Z), 0.07f, 0.06f, H, 5, DarkWood, false);
	if (bTugh)
	{
		M.AddSphere(W(E, N, Z + H), 0.45f, 8, FLinearColor(0.08f, 0.07f, 0.06f), FVector(1, 1, 1.4f), 0.2f, 5);
		M.AddCone(W(E, N, Z + H + 0.5f), 0.12f, 0.6f, 5, Gold);
	}
	else
	{
		M.AddBox(W(E, N + 0.55f, Z + H - 0.7f), FVector(3, 55, 55), Flag);
		M.AddCone(W(E, N, Z + H), 0.12f, 0.5f, 5, Gold);
	}
}

void AErtWorldBuilder::AddFenceRect(FErtMeshData& M, float E, float N, float Z, float HU, float HV, float Gap)
{
	auto Post = [&](float u, float v) { M.AddCylinder(W(E + u, N + v, Z), 0.09f, 0.08f, 1.3f, 4, Wood, false); };
	auto Rail = [&](float u0, float v0, float u1, float v1)
	{
		const FVector A = W(E + u0, N + v0, Z + 0.9f), B = W(E + u1, N + v1, Z + 0.9f);
		M.AddBox((A + B) * 0.5f, FVector((B - A).Size() * 0.5f, 0.04f * 100, 0.05f * 100), Wood, (B - A).Rotation());
		M.AddBox((A + B) * 0.5f - FVector(0, 0, 45), FVector((B - A).Size() * 0.5f, 4, 5), Wood, (B - A).Rotation());
	};
	for (float u = -HU; u <= HU; u += Gap) { Post(u, -HV); Post(u, HV); }
	for (float v = -HV + Gap; v < HV; v += Gap) { Post(-HU, v); Post(HU, v); }
	Rail(-HU, -HV, HU, -HV); Rail(-HU, HV, HU, HV); Rail(-HU, -HV, -HU, HV); Rail(HU, -HV, HU, HV);
}

void AErtWorldBuilder::AddHouse(FErtMeshData& M, float E, float N, float Z, float HU, float HV, float H, float Yaw, const FLinearColor& C, int32 S)
{
	const FRotator R(0, Yaw, 0);
	const FLinearColor Wall = ErtCol::Vary(C, 0.12f, S);
	M.AddBox(W(E, N, Z + H * 0.5f), FVector(HV, HU, H * 0.5f) * 100.f, Wall, R);
	M.AddBox(W(E, N, Z + H + 0.15f), FVector(HV + 0.25f, HU + 0.25f, 0.15f) * 100.f, Wall * 0.75f, R);
	if (S % 3 == 0) M.AddSphere(W(E, N, Z + H + 0.2f), FMath::Min(HU, HV) * 0.8f, 8, Wall * 0.9f, FVector(1, 1, 0.6f));
	const FQuat Q = R.Quaternion();
	M.AddBox(W(E, N, Z + 1.0f) + Q.RotateVector(FVector(HV * 100.f, 0, 0)), FVector(4, 45, 100), DarkWood, R);
}

// ---------------- Qayi obasi (250x250 m reja) ----------------

void AErtWorldBuilder::BuildOba()
{
	const float Z = ObaZ;
	auto OE = [](float u) { return ObaE + u; };
	auto ON = [](float v) { return ObaN + v; };
	FRandomStream RS(Seed);

	// 1) Qoziq devor + darvoza (janub, (0,-125)) + burchak minoralari
	{
		FErtMeshData M(100.f);
		const float Half = ObaHalf;
		for (float t = -Half; t <= Half; t += 0.6f)
		{
			auto Log = [&](float u, float v)
			{
				const float H = 3.4f + RS.FRandRange(-0.2f, 0.2f);
				M.AddCylinder(W(OE(u), ON(v), Z - 0.2f), 0.3f, 0.26f, H, 5, ErtCol::Vary(Wood, 0.12f, (int32)(t * 10) + (int32)u), false);
				M.AddCone(W(OE(u), ON(v), Z - 0.2f + H), 0.26f, 0.4f, 5, Wood * 0.8f);
			};
			if (FMath::Abs(t) > 4.5f) Log(t, -Half);
			Log(t, Half); Log(-Half, t); Log(Half, t);
		}
		for (int32 i = 0; i < 4; ++i)
		{
			const float u = (i & 1) ? Half - 3.f : -Half + 3.f, v = (i & 2) ? Half - 3.f : -Half + 3.f;
			AddWatchTower(M, OE(u), ON(v), Z, 8.5f);
		}
		// Darvoza minoralari va ustki to'sin
		for (int32 s = -1; s <= 1; s += 2)
		{
			M.AddBox(W(OE(s * 6.f), ON(-Half), Z + 3.5f), FVector(150, 150, 350), DarkWood);
			M.AddCone(W(OE(s * 6.f), ON(-Half), Z + 7.0f), 2.2f, 1.8f, 6, Straw * 0.9f);
			AddBanner(M, OE(s * 6.f), ON(-Half + 1.6f), Z + 7.f, 3.f, Red, false);
		}
		M.AddBox(W(0 + ObaE, ON(-Half), Z + 5.8f), FVector(60, 480, 35), DarkWood);
		M.AddBox(W(OE(-4.4f), ON(-Half + 0.4f), Z + 1.7f), FVector(20, 12, 170), Wood, FRotator(0, 25, 0)); // ochiq darvoza tavaqalari
		M.AddBox(W(OE(4.4f), ON(-Half + 0.4f), Z + 1.7f), FVector(20, 12, 170), Wood, FRotator(0, -25, 0));
		M.Commit(NewPart(TEXT("ObaWall"), true), 0, true);
	}
	// 2) Bey chodiri (12 m) + o'tovlar (5 m) halqalarda
	{
		FErtMeshData M(100.f);
		AddYurt(M, OE(0), ON(0), Z, 6.f, 2.8f, 3.8f, Cream, Cream * 0.95f, -90.f, 1);
		M.AddCylinder(W(OE(0), ON(0), Z + 2.75f), 6.6f, 6.6f, 0.35f, 12, Red, false);
		M.AddCylinder(W(OE(0), ON(0), Z + 6.5f), 0.9f, 0.9f, 0.5f, 8, Gold);
		AddBanner(M, OE(-8), ON(-8), Z, 6.f, Red, false);
		AddBanner(M, OE(8), ON(-8), Z, 6.f, Red, false);
		AddBanner(M, OE(0), ON(9), Z, 7.f, Gold, true);
		const float Rings[] = { 22.f, 40.f, 58.f, 78.f, 98.f };
		const int32 Counts[] = { 8, 13, 19, 25, 30 };
		int32 K = 0;
		for (int32 r = 0; r < 5; ++r)
			for (int32 i = 0; i < Counts[r]; ++i)
			{
				const float A = 2.f * PI * i / Counts[r] + r * 0.3f;
				const float u = FMath::Cos(A) * Rings[r] + RS.FRandRange(-2.5f, 2.5f);
				const float v = FMath::Sin(A) * Rings[r] + RS.FRandRange(-2.5f, 2.5f);
				if (FMath::Abs(u) < 5.5f) continue;                                   // asosiy yo'l (4 m)
				if (v < -100.f) continue;                                           // darvoza oldi maydoni
				if (u > -56 && u < -32 && v > -38 && v < -12) continue;              // temirchi 15x15
				if (u > 32 && u < 60 && v > -40 && v < -8) continue;                 // mashq maydoni 20x20
				if (u > -84 && u < -54 && v > 38 && v < 66) continue;                // otxona
				if (FMath::Abs(u) > ObaHalf - 8.f || FMath::Abs(v) > ObaHalf - 8.f) continue;
				const float DoorYaw = FMath::RadiansToDegrees(FMath::Atan2(-v, -u));
				AddYurt(M, OE(u), ON(v), Z, 2.5f, 1.8f, 1.7f, Felt, FLinearColor(0.72f, 0.66f, 0.55f), DoorYaw, ++K);
			}
		M.Commit(NewPart(TEXT("ObaYurts"), true), 0, true);
	}
	// 3) Temirchi (15x15, (-45,-25)), mashq maydoni (20x20, (45,-25)), otxona, quduq, gulxanlar
	{
		FErtMeshData M(100.f);
		// Temirchi: ochiq bostirma
		const float BE = OE(-45), BN = ON(-25);
		for (int32 i = 0; i < 4; ++i)
			M.AddCylinder(W(BE + ((i & 1) ? 6.f : -6.f), BN + ((i & 2) ? 6.f : -6.f), Z), 0.25f, 0.22f, 3.6f, 6, Wood, false);
		M.AddBox(W(BE, BN, Z + 3.8f), FVector(750, 750, 15), Straw * 0.85f, FRotator(6, 0, 0));
		M.AddBox(W(BE - 4.f, BN, Z + 1.5f), FVector(80, 700, 150), Stone * 0.8f);              // orqa devor
		M.AddBox(W(BE, BN + 3.f, Z + 0.6f), FVector(90, 90, 60), Stone * 0.7f);                 // o'choq
		M.AddCone(W(BE, BN + 3.f, Z + 1.2f), 0.5f, 0.9f, 6, Ember);
		M.AddCylinder(W(BE, BN + 3.f, Z + 1.2f), 0.35f, 0.3f, 3.5f, 6, Stone * 0.6f, false);    // mo'ri
		M.AddBox(W(BE + 1.5f, BN - 1.f, Z + 0.45f), FVector(20, 20, 45), DarkWood);              // sandon tagi
		M.AddBox(W(BE + 1.5f, BN - 1.f, Z + 1.0f), FVector(45, 18, 12), FLinearColor(0.3f, 0.3f, 0.32f));
		M.AddBox(W(BE + 3.f, BN + 2.f, Z + 0.4f), FVector(60, 40, 40), Wood);                    // suv chelagi/stol
		M.AddBox(W(BE - 2.f, BN - 5.f, Z + 1.0f), FVector(30, 120, 100), DarkWood);             // qurol tokchasi
		// Mashq maydoni
		const float TE = OE(46), TN = ON(-24);
		AddFenceRect(M, TE, TN, Z, 10.f, 10.f, 2.f);
		for (int32 i = 0; i < 3; ++i)
		{
			const float u = -6.f + i * 6.f;
			M.AddCylinder(W(TE + u, TN + 8.5f, Z), 0.12f, 0.1f, 1.6f, 4, Wood, false);
			M.AddCylinder(W(TE + u, TN + 8.5f, Z + 1.2f), 0.7f, 0.7f, 0.18f, 12, Straw, true, FRotator(-90, 0, 0));
			M.AddCylinder(W(TE + u, TN + 8.5f, Z + 1.2f), 0.4f, 0.4f, 0.2f, 12, Red, true, FRotator(-90, 0, 0));
			M.AddCylinder(W(TE + u, TN + 8.5f, Z + 1.2f), 0.15f, 0.15f, 0.22f, 8, Gold, true, FRotator(-90, 0, 0));
		}
		for (int32 i = 0; i < 4; ++i)
		{
			const float u = -6.f + i * 4.f, v = -5.f;
			M.AddCylinder(W(TE + u, TN + v, Z), 0.16f, 0.14f, 1.9f, 5, Wood, false);
			M.AddBox(W(TE + u, TN + v, Z + 1.5f), FVector(12, 60, 10), Wood);
			M.AddCylinder(W(TE + u, TN + v, Z + 1.9f), 0.2f, 0.2f, 0.3f, 6, Straw);
		}
		M.AddBox(W(TE - 8.f, TN + 2.f, Z + 0.4f), FVector(40, 250, 40), Wood); // o'rindiq
		// Otxona (-70, 52)
		const float SE = OE(-69), SN = ON(52);
		AddFenceRect(M, SE, SN, Z, 14.f, 12.f, 2.f);
		M.AddBox(W(SE - 10.f, SN, Z + 2.6f), FVector(1000, 380, 12), Straw * 0.85f, FRotator(0, 0, 8));
		for (int32 i = 0; i < 4; ++i) M.AddCylinder(W(SE - 10.f + ((i & 1) ? 3.5f : -3.5f), SN + ((i & 2) ? 9.f : -9.f), Z), 0.2f, 0.18f, 2.5f, 5, Wood, false);
		const FLinearColor HorseCols[] = { FLinearColor(0.35f, 0.22f, 0.12f), FLinearColor(0.15f, 0.12f, 0.10f), FLinearColor(0.62f, 0.55f, 0.45f), FLinearColor(0.45f, 0.30f, 0.16f) };
		for (int32 i = 0; i < 5; ++i)
			AddHorse(M, SE + RS.FRandRange(-8.f, 8.f), SN + RS.FRandRange(-8.f, 8.f), Z, RS.FRandRange(0.f, 360.f), HorseCols[i % 4]);
		// Quduq
		M.AddCylinder(W(OE(13), ON(13), Z), 1.1f, 1.1f, 1.0f, 10, Stone, false);
		M.AddCylinder(W(OE(13), ON(13), Z), 0.85f, 0.85f, 1.0f, 10, FLinearColor(0.08f, 0.1f, 0.12f), true);
		M.AddCylinder(W(OE(12), ON(13), Z + 1.f), 0.08f, 0.08f, 2.f, 4, Wood, false);
		M.AddCylinder(W(OE(14), ON(13), Z + 1.f), 0.08f, 0.08f, 2.f, 4, Wood, false);
		M.AddCone(W(OE(13), ON(13), Z + 3.f), 1.5f, 0.9f, 8, Straw * 0.9f);
		// Gulxanlar
		AddFire(M, OE(-16), ON(16), Z, true);
		AddFire(M, OE(18), ON(18), Z, true);
		AddFire(M, OE(-30), ON(-52), Z, true);
		AddFire(M, OE(28), ON(-60), Z, true);
		AddFire(M, OE(-60), ON(-80), Z, false);
		AddFire(M, OE(70), ON(60), Z, false);
		// Darvoza oldidagi mash'ala ustunlari
		for (int32 s = -1; s <= 1; s += 2)
		{
			M.AddCylinder(W(OE(s * 3.5f), ON(-ObaHalf + 6.f), Z), 0.1f, 0.08f, 2.4f, 4, DarkWood, false);
			M.AddCone(W(OE(s * 3.5f), ON(-ObaHalf + 6.f), Z + 2.4f), 0.22f, 0.5f, 5, Flame);
		}
		M.Commit(NewPart(TEXT("ObaProps"), true), 0, true);
	}
}

// ---------------- Tosh qal'a (shimoli-sharq cho'qqi) ----------------

void AErtWorldBuilder::BuildFortress()
{
	const float Z = FortZ;
	auto FE = [](float u) { return FortE + u; };
	auto FN = [](float v) { return FortN + v; };
	FErtMeshData M(100.f);
	const float Half = FortHalf, WallH = 12.f, Th = 1.6f;
	int32 S = 0;
	auto WallRun = [&](float u0, float v0, float u1, float v1, bool bGate)
	{
		const float Len = FVector2D::Distance(FVector2D(u0, v0), FVector2D(u1, v1));
		const int32 Blocks = FMath::CeilToInt(Len / 4.f);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(u1 - u0, v1 - v0)); // UE: X=N, Y=E
		for (int32 i = 0; i < Blocks; ++i)
		{
			const float t0 = (float)i / Blocks, t1 = (float)(i + 1) / Blocks;
			const float cu = FMath::Lerp(u0, u1, (t0 + t1) * 0.5f), cv = FMath::Lerp(v0, v1, (t0 + t1) * 0.5f);
			const float BL = Len * (t1 - t0) * 0.5f;
			if (bGate && FMath::Abs(cu) < 3.5f)
			{
				M.AddBox(W(FE(cu), FN(cv), Z + 9.5f), FVector(BL, Th, 2.5f) * 100.f, ErtCol::Vary(Stone, 0.1f, ++S), FRotator(0, Yaw, 0));
				continue;
			}
			M.AddBox(W(FE(cu), FN(cv), Z + WallH * 0.5f), FVector(BL, Th, WallH * 0.5f) * 100.f, ErtCol::Vary(Stone, 0.1f, ++S), FRotator(0, Yaw, 0));
			// Tishlar
			M.AddBox(W(FE(cu), FN(cv), Z + WallH + 0.6f), FVector(BL * 0.45f, Th * 0.5f, 0.6f) * 100.f, ErtCol::Vary(Stone, 0.1f, ++S), FRotator(0, Yaw, 0));
		}
		// Yo'lak (devor ichki tomonidagi supa)
		const float cu = (u0 + u1) * 0.5f, cv = (v0 + v1) * 0.5f;
		const float InU = -cu / FMath::Max(1.f, FMath::Abs(cu)) * (cu != 0 ? 1.f : 0.f), InV = -cv / FMath::Max(1.f, FMath::Abs(cv)) * (cv != 0 ? 1.f : 0.f);
		M.AddBox(W(FE(cu + InU * (Th + 0.9f)), FN(cv + InV * (Th + 0.9f)), Z + WallH - 2.5f), FVector(Len * 0.5f, 1.0f, 0.15f) * 100.f, Wood, FRotator(0, Yaw, 0));
	};
	WallRun(-Half, -Half, Half, -Half, true);
	WallRun(-Half, Half, Half, Half, false);
	WallRun(-Half, -Half, -Half, Half, false);
	WallRun(Half, -Half, Half, Half, false);
	// Burchak minoralari
	for (int32 i = 0; i < 4; ++i)
	{
		const float u = (i & 1) ? Half : -Half, v = (i & 2) ? Half : -Half;
		M.AddCylinder(W(FE(u), FN(v), Z - 1.f), 5.5f, 5.0f, 17.f, 10, ErtCol::Vary(Stone, 0.06f, ++S), false);
		M.AddCylinder(W(FE(u), FN(v), Z + 16.f), 5.4f, 5.4f, 1.2f, 10, Stone * 0.9f, true);
		M.AddCone(W(FE(u), FN(v), Z + 17.f), 5.6f, 5.f, 10, FLinearColor(0.28f, 0.2f, 0.17f));
	}
	// Darvoza minoralari
	for (int32 s = -1; s <= 1; s += 2)
	{
		M.AddBox(W(FE(s * 5.5f), FN(-Half), Z + 7.f), FVector(220, 220, 700), ErtCol::Vary(Stone, 0.06f, ++S));
		M.AddCone(W(FE(s * 5.5f), FN(-Half), Z + 14.f), 3.f, 3.f, 6, FLinearColor(0.28f, 0.2f, 0.17f));
		AddBanner(M, FE(s * 5.5f), FN(-Half + 2.5f), Z + 14.f, 3.5f, FLinearColor(0.1f, 0.15f, 0.45f), false);
	}
	M.AddBox(W(FE(0), FN(-Half - 1.9f), Z + 3.5f), FVector(20, 300, 350), DarkWood); // ko'tarilgan panjara (ochiq holat belgisi)
	// Qasr (keep)
	M.AddBox(W(FE(0), FN(6), Z + 12.f), FVector(1000, 1000, 1200), ErtCol::Vary(Stone, 0.05f, ++S));
	for (int32 i = 0; i < 4; ++i)
	{
		const float u = (i & 1) ? 9.f : -9.f, v = 6.f + ((i & 2) ? 9.f : -9.f);
		M.AddBox(W(FE(u), FN(v), Z + 25.f), FVector(150, 150, 150), Stone * 0.95f);
	}
	M.AddCylinder(W(FE(0), FN(6), Z + 24.f), 3.f, 2.8f, 6.f, 8, Stone, false);
	M.AddCone(W(FE(0), FN(6), Z + 30.f), 3.2f, 3.5f, 8, FLinearColor(0.28f, 0.2f, 0.17f));
	AddBanner(M, FE(0), FN(6), Z + 33.f, 4.f, FLinearColor(0.1f, 0.15f, 0.45f), false);
	M.AddBox(W(FE(0), FN(-4.2f), Z + 2.f), FVector(20, 130, 200), DarkWood); // qasr eshigi
	// Ichki binolar: kazarma, otxona, quduq
	M.AddBox(W(FE(-20), FN(12), Z + 2.5f), FVector(500, 900, 250), ErtCol::Vary(Stone, 0.08f, ++S));
	M.AddBox(W(FE(-20), FN(12), Z + 5.8f), FVector(560, 960, 80), FLinearColor(0.28f, 0.2f, 0.17f), FRotator(0, 0, 0));
	M.AddBox(W(FE(20), FN(14), Z + 2.f), FVector(400, 800, 200), Wood);
	M.AddBox(W(FE(20), FN(14), Z + 4.6f), FVector(460, 860, 60), Straw * 0.85f);
	M.AddCylinder(W(FE(18), FN(-14), Z), 1.1f, 1.1f, 1.f, 10, Stone, false);
	AddFire(M, FE(-16), FN(-16), Z, true);
	M.Commit(NewPart(TEXT("Fortress"), true), 0, true);
}

// ---------------- Devorli shahar (janubi-g'arb, oltin gumbaz) ----------------

void AErtWorldBuilder::BuildCity()
{
	const float Z = CityZ;
	auto CE = [](float u) { return CityE + u; };
	auto CN = [](float v) { return CityN + v; };
	FRandomStream RS(Seed + 11);
	int32 S = 100;
	// Devor
	{
		FErtMeshData M(100.f);
		const int32 Segs = 120;
		const float GateHalf = 6.f / CityR;
		for (int32 i = 0; i < Segs; ++i)
		{
			const float A0 = 2.f * PI * i / Segs, A1 = 2.f * PI * (i + 1) / Segs, Am = (A0 + A1) * 0.5f;
			bool bGate = false;
			for (int32 g = 0; g < 4; ++g) { const float GA = g * HALF_PI; float D = FMath::Abs(FMath::FindDeltaAngleRadians(Am, GA)); if (D < GateHalf) bGate = true; }
			const float u = FMath::Cos(Am) * CityR, v = FMath::Sin(Am) * CityR;
			const float Len = CityR * (A1 - A0) * 0.5f + 0.05f;
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-FMath::Sin(Am) * -1.f, FMath::Cos(Am) * -1.f)); // teginma yo'nalishi (UE yaw)
			const float TanYaw = FMath::RadiansToDegrees(FMath::Atan2(FMath::Cos(Am), -FMath::Sin(Am)));
			(void)Yaw;
			if (bGate)
			{
				M.AddBox(W(CE(u), CN(v), Z + 8.f), FVector(Len, 1.6f, 1.5f) * 100.f, ErtCol::Vary(Ochre * 0.9f, 0.08f, ++S), FRotator(0, TanYaw, 0));
				continue;
			}
			M.AddBox(W(CE(u), CN(v), Z + 4.5f), FVector(Len, 1.6f, 4.5f) * 100.f, ErtCol::Vary(Ochre * 0.9f, 0.08f, ++S), FRotator(0, TanYaw, 0));
			M.AddBox(W(CE(u), CN(v), Z + 9.4f), FVector(Len * 0.5f, 0.8f, 0.5f) * 100.f, ErtCol::Vary(Ochre * 0.85f, 0.08f, ++S), FRotator(0, TanYaw, 0));
			if (i % 10 == 0)
			{
				M.AddCylinder(W(CE(u), CN(v), Z - 0.5f), 4.2f, 3.8f, 13.f, 10, ErtCol::Vary(Ochre * 0.85f, 0.06f, ++S), true);
				M.AddCone(W(CE(u), CN(v), Z + 12.5f), 4.3f, 3.5f, 10, FLinearColor(0.35f, 0.18f, 0.12f));
			}
		}
		for (int32 g = 0; g < 4; ++g)
		{
			const float GA = g * HALF_PI;
			for (int32 s = -1; s <= 1; s += 2)
			{
				const float A = GA + s * (9.f / CityR);
				M.AddBox(W(CE(FMath::Cos(A) * CityR), CN(FMath::Sin(A) * CityR), Z + 6.f), FVector(280, 280, 650), ErtCol::Vary(Ochre * 0.9f, 0.06f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0));
				M.AddCone(W(CE(FMath::Cos(A) * CityR), CN(FMath::Sin(A) * CityR), Z + 12.5f), 3.6f, 3.f, 6, FLinearColor(0.35f, 0.18f, 0.12f));
			}
		}
		M.Commit(NewPart(TEXT("CityWall"), true), 0, true);
	}
	// Uylar, masjid, saroy, bozor, sarv daraxtlari
	{
		FErtMeshData M(100.f);
		for (float v = -160.f; v <= 160.f; v += 24.f)
			for (float u = -160.f; u <= 160.f; u += 24.f)
			{
				const float ju = u + RS.FRandRange(-4.f, 4.f), jv = v + RS.FRandRange(-4.f, 4.f);
				if (FVector2D(ju, jv).Size() > CityR - 24.f) continue;
				if (FMath::Abs(ju) < 8.f || FMath::Abs(jv) < 8.f) continue;           // 4 ko'cha
				if (FMath::Abs(ju) < 42.f && FMath::Abs(jv) < 42.f) continue;         // markaziy maydon
				if (ju > 30 && jv > 90) continue;                                     // saroy hovlisi
				const float HU = RS.FRandRange(4.5f, 8.5f), HV = RS.FRandRange(4.5f, 7.5f), H = RS.FRandRange(3.2f, 6.5f);
				AddHouse(M, CE(ju), CN(jv), Z, HU, HV, H, RS.FRandRange(-8.f, 8.f), Ochre, ++S);
				if (RS.FRand() < 0.25f) M.AddCone(W(CE(ju + HU + 2.f), CN(jv), Z), 1.1f, 9.f, 6, FLinearColor(0.08f, 0.18f, 0.08f)); // sarv
			}
		// Masjid: oltin gumbaz + 2 minora
		M.AddBox(W(CE(0), CN(0), Z + 5.f), FVector(1500, 1500, 500), FLinearColor(0.86f, 0.83f, 0.76f));
		M.AddBox(W(CE(0), CN(0), Z + 10.4f), FVector(1560, 1560, 40), FLinearColor(0.72f, 0.68f, 0.60f));
		M.AddCylinder(W(CE(0), CN(0), Z + 10.f), 11.f, 11.f, 2.5f, 16, FLinearColor(0.86f, 0.83f, 0.76f), false);
		M.AddSphere(W(CE(0), CN(0), Z + 12.f), 11.5f, 16, Gold, FVector(1, 1, 0.95f));
		M.AddCone(W(CE(0), CN(0), Z + 23.f), 0.6f, 2.5f, 6, Gold);
		for (int32 s = -1; s <= 1; s += 2)
		{
			M.AddCylinder(W(CE(s * 20.f), CN(-20.f), Z), 2.f, 1.7f, 30.f, 10, FLinearColor(0.88f, 0.85f, 0.78f), false);
			M.AddCylinder(W(CE(s * 20.f), CN(-20.f), Z + 20.f), 2.6f, 2.6f, 0.8f, 10, Gold * 0.9f, true);
			M.AddCone(W(CE(s * 20.f), CN(-20.f), Z + 30.f), 2.f, 4.f, 10, Gold);
		}
		M.AddBox(W(CE(0), CN(-15.2f), Z + 2.f), FVector(20, 250, 200), DarkWood);
		// Saroy (shimoli-sharq hovli)
		M.AddBox(W(CE(80), CN(120), Z + 4.5f), FVector(1600, 2400, 450), FLinearColor(0.80f, 0.72f, 0.58f));
		for (float t = -22.f; t <= 22.f; t += 4.f) M.AddBox(W(CE(80 + t), CN(120 + 16.f), Z + 9.5f), FVector(80, 120, 60), FLinearColor(0.80f, 0.72f, 0.58f) * 0.9f);
		M.AddSphere(W(CE(80), CN(120), Z + 9.f), 6.f, 12, FLinearColor(0.2f, 0.45f, 0.5f), FVector(1, 1, 0.7f));
		AddBanner(M, CE(60), CN(100), Z, 8.f, FLinearColor(0.2f, 0.45f, 0.5f), false);
		AddBanner(M, CE(100), CN(100), Z, 8.f, FLinearColor(0.2f, 0.45f, 0.5f), false);
		// Bozor (sharqiy darvoza ko'chasi bo'ylab): rastalar
		const FLinearColor Awn[] = { FLinearColor(0.6f, 0.15f, 0.12f), FLinearColor(0.15f, 0.3f, 0.55f), FLinearColor(0.2f, 0.45f, 0.2f), FLinearColor(0.75f, 0.6f, 0.2f) };
		for (int32 i = 0; i < 14; ++i)
		{
			const float u = 60.f + i * 8.f, v = (i & 1) ? 11.f : -11.f;
			M.AddBox(W(CE(u), CN(v), Z + 0.9f), FVector(150, 70, 90), Wood);
			for (int32 k = 0; k < 2; ++k) M.AddCylinder(W(CE(u + (k ? 1.4f : -1.4f)), CN(v + (v > 0 ? -1.f : 1.f)), Z), 0.06f, 0.06f, 2.6f, 4, Wood, false);
			M.AddBox(W(CE(u), CN(v), Z + 2.7f), FVector(180, 130, 6), Awn[i % 4], FRotator(0, 0, v > 0 ? 12.f : -12.f));
			M.AddBox(W(CE(u), CN(v), Z + 1.95f), FVector(60, 40, 15), Awn[(i + 1) % 4]);
		}
		// Chorraha favvorasi
		M.AddCylinder(W(CE(0), CN(28), Z), 3.f, 3.f, 0.8f, 12, Stone, false);
		M.AddCylinder(W(CE(0), CN(28), Z + 0.05f), 2.6f, 2.6f, 0.7f, 12, FLinearColor(0.15f, 0.35f, 0.45f), true);
		AddFire(M, CE(15), CN(30), Z, true);
		AddFire(M, CE(-15), CN(30), Z, true);
		M.Commit(NewPart(TEXT("CityBuildings"), true), 0, true);
	}
}

// ---------------- Mo'g'ul lageri (janubi-sharq) ----------------

void AErtWorldBuilder::BuildCamp()
{
	const float Z = CampZ;
	auto KE = [](float u) { return CampE + u; };
	auto KN = [](float v) { return CampN + v; };
	FRandomStream RS(Seed + 23);
	FErtMeshData M(100.f);
	int32 K = 500;
	// Xon chodiri + tug'lar
	AddYurt(M, KE(0), KN(0), Z, 8.f, 3.f, 4.2f, FLinearColor(0.38f, 0.10f, 0.09f), FLinearColor(0.22f, 0.08f, 0.07f), -90.f, ++K);
	M.AddCylinder(W(KE(0), KN(0), Z + 2.9f), 8.6f, 8.6f, 0.4f, 12, Gold * 0.8f, false);
	for (int32 i = 0; i < 6; ++i) { const float A = 2.f * PI * i / 6; AddBanner(M, KE(FMath::Cos(A) * 11.f), KN(FMath::Sin(A) * 11.f), Z, 6.5f, Red, true); }
	// Gerlar halqalarda
	const float Rings[] = { 26.f, 50.f, 76.f, 102.f, 128.f };
	const int32 Counts[] = { 7, 12, 18, 24, 30 };
	for (int32 r = 0; r < 5; ++r)
		for (int32 i = 0; i < Counts[r]; ++i)
		{
			const float A = 2.f * PI * i / Counts[r] + r * 0.45f;
			const float u = FMath::Cos(A) * Rings[r] + RS.FRandRange(-4.f, 4.f), v = FMath::Sin(A) * Rings[r] + RS.FRandRange(-4.f, 4.f);
			if (u < -CampR + 30.f && FMath::Abs(v) < 12.f) continue; // g'arbiy kirish yo'li
			if (RS.FRand() < 0.12f) { AddFire(M, KE(u), KN(v), Z, Lights.Num() < 12); continue; }
			AddYurt(M, KE(u), KN(v), Z, 3.f, 1.7f, 1.5f, FeltDark, FeltDark * 0.85f, FMath::RadiansToDegrees(FMath::Atan2(-v, -u)), ++K);
		}
	// Tikanli qoziq halqasi (tashqariga qiya)
	for (int32 i = 0; i < 300; ++i)
	{
		const float A = 2.f * PI * i / 300;
		const float u = FMath::Cos(A) * (CampR - 2.f), v = FMath::Sin(A) * (CampR - 2.f);
		if (FMath::Abs(FMath::FindDeltaAngleRadians(A, PI)) < 0.09f) continue; // g'arbiy darvoza
		const float YawUE = FMath::RadiansToDegrees(FMath::Atan2(FMath::Cos(A), FMath::Sin(A))); // tashqi yo'nalish (UE)
		M.AddCylinder(W(u + CampE, v + CampN, Z - 0.3f), 0.16f, 0.05f, 2.8f, 4, ErtCol::Vary(DarkWood, 0.15f, i), false, FRotator(-50.f, YawUE, 0));
	}
	// Aravalar
	for (int32 i = 0; i < 6; ++i)
	{
		const float u = 60.f + RS.FRandRange(-10.f, 10.f), v = 110.f - i * 9.f;
		M.AddBox(W(KE(u), KN(v), Z + 1.1f), FVector(120, 220, 45), Wood);
		M.AddBox(W(KE(u), KN(v), Z + 1.85f), FVector(130, 230, 45), FeltDark * 1.2f, FRotator(0, 0, 0));
		for (int32 s = -1; s <= 1; s += 2) M.AddCylinder(W(KE(u + s * 1.35f), KN(v), Z + 0.7f), 0.7f, 0.7f, 0.15f, 10, DarkWood, true, FRotator(0, 0, 90));
	}
	// Otlar (shimoliy qator)
	const FLinearColor HC[] = { FLinearColor(0.2f, 0.14f, 0.1f), FLinearColor(0.5f, 0.38f, 0.25f), FLinearColor(0.12f, 0.1f, 0.09f) };
	for (int32 i = 0; i < 10; ++i) AddHorse(M, KE(-40.f + i * 8.f), KN(118.f + RS.FRandRange(-3.f, 3.f)), Z, 90.f + RS.FRandRange(-15.f, 15.f), HC[i % 3]);
	// Qurol tokchalari va o'q sandiqlari
	for (int32 i = 0; i < 5; ++i) M.AddBox(W(KE(-20.f + i * 6.f), KN(-30.f), Z + 0.5f), FVector(60, 40, 50), DarkWood);
	M.Commit(NewPart(TEXT("Camp"), true), 0, true);
}

// ---------------- O'rmon va toshlar ----------------

bool AErtWorldBuilder::IsBuildable(float E, float N) const
{
	if (FMath::Max(FMath::Abs(E - ObaE), FMath::Abs(N - ObaN)) < ObaHalf + 14.f) return false;
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(CityE, CityN)) < CityR + 18.f) return false;
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(CampE, CampN)) < CampR + 14.f) return false;
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(FortE, FortN)) < FortHalf + 40.f) return false;
	if (FMath::Abs(E - RiverE(N)) < 30.f) return false;
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN)) < LakeR + 12.f) return false; // ko'l
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(OasisE, OasisN)) < OasisR + 30.f) return false; // voha
	if (FMath::Max(FMath::Abs(E - CaravanE), FMath::Abs(N - CaravanN)) < 40.f) return false;          // karvonsaroy
	if (FMath::Abs(E - DamE) < DamHalfE + 30.f && FMath::Abs(N - DamN) < DamHalfN + 30.f) return false;  // Damashq
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(HalabE, HalabN)) < HalabR + 30.f) return false;   // Halab
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(KonE, KonN)) < KonR + 30.f) return false;   // Konya
	float Wd; if (RoadDist(E, N, &Wd) < Wd + 3.f) return false;
	return true;
}

void AErtWorldBuilder::BuildForest()
{
	FRandomStream RS(Seed + 3);
	const float Half = WorldSizeM * 0.5f;
	const int32 CellsPerSide = 4;
	TArray<FErtMeshData> Cells; Cells.Init(FErtMeshData(100.f), CellsPerSide * CellsPerSide);
	int32 Placed = 0;
	for (int32 i = 0; i < TreeCount * 6 && Placed < TreeCount; ++i)
	{
		const float E = RS.FRandRange(-Half + 10.f, Half - 10.f), N = RS.FRandRange(-Half + 10.f, Half - 10.f);
		// Zichlik: shimoli-g'arb o'rmoni, tog' etaklari, daryo bo'yi, boshqa joyda siyrak
		float Dens = 0.06f;
		Dens = FMath::Max(Dens, 0.9f * Smooth01((-E - 120.f) / 200.f) * Smooth01((N - 80.f) / 200.f));
		const float DF = FVector2D::Distance(FVector2D(E, N), FVector2D(FortE, FortN));
		Dens = FMath::Max(Dens, 0.55f * Smooth01((DF - 180.f) / 120.f) * (1.f - Smooth01((DF - 480.f) / 120.f)));
		Dens = FMath::Max(Dens, 0.5f * (1.f - Smooth01((FMath::Abs(E - RiverE(N)) - 32.f) / 60.f)));
		Dens *= 0.6f + 0.4f * Noise(E, N, 0.01f);
		if (RS.FRand() > Dens) continue;
		if (!IsBuildable(E, N)) continue;
		const float H = HeightAt(E, N);
		const FVector Nm = TerrainNormal(E, N);
		if (H > 150.f || Nm.Z < 0.72f || H < WaterZ + 1.5f || N < DesertN + 40.f) continue;
		const bool bPine = H > 60.f || RS.FRand() < 0.55f;
		const int32 cx = FMath::Clamp((int32)((E + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		const int32 cy = FMath::Clamp((int32)((N + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		AddTree(Cells[cy * CellsPerSide + cx], E, N, H, RS.FRandRange(0.8f, 1.5f), bPine, Placed);
		++Placed;
	}
	for (int32 c = 0; c < Cells.Num(); ++c)
		if (Cells[c].Verts.Num()) Cells[c].Commit(NewPart(FString::Printf(TEXT("Forest_%d"), c), true), 0, true);
	UE_LOG(LogErtugrul, Log, TEXT("O'rmon: %d daraxt"), Placed);
}

void AErtWorldBuilder::BuildRocks()
{
	FRandomStream RS(Seed + 5);
	FErtMeshData M(100.f);
	const float Half = WorldSizeM * 0.5f;
	int32 Placed = 0;
	for (int32 i = 0; i < 4000 && Placed < 320; ++i)
	{
		const float E = RS.FRandRange(-Half + 10.f, Half - 10.f), N = RS.FRandRange(-Half + 10.f, Half - 10.f);
		const float H = HeightAt(E, N);
		const FVector Nm = TerrainNormal(E, N);
		const float DF = FVector2D::Distance(FVector2D(E, N), FVector2D(FortE, FortN));
		float P = 0.02f;
		if (H > 60.f) P = 0.5f;
		if (Nm.Z < 0.8f) P += 0.3f;
		if (FMath::Abs(E - RiverE(N)) < 45.f) P = 0.25f;
		if (RS.FRand() > P || !IsBuildable(E, N) || DF < FortHalf + 45.f) continue;
		const float R = RS.FRandRange(1.0f, 4.5f) * (H > 100.f ? 1.6f : 1.f);
		M.AddSphere(W(E, N, H - R * 0.35f), R, 8, ErtCol::Vary(H > 150.f ? FLinearColor(0.7f, 0.7f, 0.72f) : Stone, 0.12f, Placed), FVector(RS.FRandRange(0.7f, 1.4f), RS.FRandRange(0.7f, 1.4f), RS.FRandRange(0.5f, 0.9f)), 0.18f, Placed);
		++Placed;
	}
	if (M.Verts.Num()) M.Commit(NewPart(TEXT("Rocks"), true), 0, true);
}

// ---------------- Cho'l: voha, palmalar, karvonsaroy, xarobalar ----------------

bool AErtWorldBuilder::IsWater(float E, float N, float& SurfZ) const
{
	if (FMath::Abs(E - RiverE(N)) < 25.f) { SurfZ = WaterZ; return true; }
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN)) < LakeR + 4.f) { SurfZ = LakeZ; return true; }
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(OasisE, OasisN)) < OasisR + 2.f) { SurfZ = OasisZ; return true; }
	SurfZ = 0.f;
	return false;
}

void AErtWorldBuilder::AddPalm(FErtMeshData& M, float E, float N, float Z, float H, int32 S)
{
	FRandomStream RS(S);
	const float LeanA = RS.FRandRange(0.f, 2.f * PI), Lean = RS.FRandRange(0.6f, 1.8f);
	const FLinearColor Trunk(0.42f, 0.32f, 0.18f), Leaf(0.16f, 0.40f, 0.14f);
	FVector Top = W(E, N, Z);
	const int32 Segs = 6;
	for (int32 i = 0; i < Segs; ++i)
	{
		const float T0 = (float)i / Segs, T1 = (float)(i + 1) / Segs;
		const float Off0 = Lean * T0 * T0, Off1 = Lean * T1 * T1;
		const FVector A = W(E + FMath::Cos(LeanA) * Off0, N + FMath::Sin(LeanA) * Off0, Z + H * T0);
		const FVector B = W(E + FMath::Cos(LeanA) * Off1, N + FMath::Sin(LeanA) * Off1, Z + H * T1);
		M.AddCylinder(A, 0.30f - 0.12f * T0, 0.30f - 0.12f * T1, (B - A).Size() / 100.f, 6, ErtCol::Vary(Trunk, 0.1f, S + i), false, (B - A).Rotation() + FRotator(-90.f, 0, 0));
		Top = B;
	}
	// Bargli toj: 8 ta uzun yassi quti, tashqariga-pastga qiya
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = 2.f * PI * i / 8 + RS.FRandRange(-0.2f, 0.2f);
		const FRotator R(0, FMath::RadiansToDegrees(A), 0);
		const FVector Dir = R.RotateVector(FVector::ForwardVector);
		M.AddBox(Top + Dir * 170.f + FVector(0, 0, 20.f), FVector(180.f, 22.f, 4.f), ErtCol::Vary(Leaf, 0.15f, S + 10 + i), R + FRotator(-28.f + RS.FRandRange(-8.f, 8.f), 0, 0));
	}
	M.AddSphere(Top + FVector(0, 0, 10.f), 0.35f, 6, FLinearColor(0.55f, 0.35f, 0.1f));
}

void AErtWorldBuilder::BuildDesert()
{
	FRandomStream RS(Seed + 41);
	FErtMeshData M(100.f);
	int32 S = 900;
	// Palmalar - voha atrofida
	for (int32 i = 0; i < 26; ++i)
	{
		const float A = 2.f * PI * i / 26 + RS.FRandRange(-0.15f, 0.15f);
		const float R = OasisR + RS.FRandRange(4.f, 22.f);
		const float E = OasisE + FMath::Cos(A) * R, N = OasisN + FMath::Sin(A) * R;
		AddPalm(M, E, N, HeightAt(E, N), RS.FRandRange(6.f, 10.f), ++S);
	}
	// Butalar va toshlar
	for (int32 i = 0; i < 40; ++i)
	{
		const float A = RS.FRandRange(0.f, 2.f * PI), R = OasisR + RS.FRandRange(6.f, 60.f);
		const float E = OasisE + FMath::Cos(A) * R, N = OasisN + FMath::Sin(A) * R;
		if (i % 3 == 0) M.AddSphere(W(E, N, HeightAt(E, N) - 0.3f), RS.FRandRange(0.8f, 2.2f), 7, ErtCol::Vary(FLinearColor(0.62f, 0.52f, 0.36f), 0.1f, ++S), FVector(1.2f, 1.f, 0.6f), 0.2f, S);
		else M.AddSphere(W(E, N, HeightAt(E, N)), RS.FRandRange(0.5f, 1.1f), 6, ErtCol::Vary(FLinearColor(0.35f, 0.42f, 0.16f), 0.2f, ++S), FVector(1.f, 1.f, 0.6f), 0.25f, S);
	}
	// Karvonsaroy: 40x40 m devor, darvoza shimolda, ichkarida hovli + hujralar, burchak minoralari, gumbazli masjid
	{
		const float Z = HeightAt(CaravanE, CaravanN);
		auto KE = [](float u) { return CaravanE + u; };
		auto KN = [](float v) { return CaravanN + v; };
		const FLinearColor Mud(0.72f, 0.58f, 0.38f);
		const float Half = 20.f, WallH = 6.f;
		for (int32 side = 0; side < 4; ++side)
		{
			const bool bNS = side < 2;
			const float Sign = (side & 1) ? 1.f : -1.f;
			for (float t = -Half; t < Half; t += 4.f)
			{
				const float u = bNS ? t + 2.f : Sign * Half, v = bNS ? Sign * Half : t + 2.f;
				if (side == 1 && FMath::Abs(u) < 4.f) { M.AddBox(W(KE(u), KN(v), Z + 5.5f), FVector(bNS ? 100 : 200, bNS ? 200 : 100, 60), ErtCol::Vary(Mud, 0.08f, ++S)); continue; }
				M.AddBox(W(KE(u), KN(v), Z + WallH * 0.5f), FVector(bNS ? 100 : 200, bNS ? 200 : 100, WallH * 50.f), ErtCol::Vary(Mud, 0.08f, ++S));
				M.AddBox(W(KE(u), KN(v), Z + WallH + 0.4f), FVector(bNS ? 50 : 90, bNS ? 90 : 50, 40), ErtCol::Vary(Mud * 0.9f, 0.08f, ++S));
			}
		}
		for (int32 i = 0; i < 4; ++i)
		{
			const float u = (i & 1) ? Half : -Half, v = (i & 2) ? Half : -Half;
			M.AddCylinder(W(KE(u), KN(v), Z), 3.2f, 2.8f, 9.f, 10, ErtCol::Vary(Mud, 0.06f, ++S), true);
			M.AddSphere(W(KE(u), KN(v), Z + 9.f), 2.9f, 10, Mud * 0.95f, FVector(1, 1, 0.6f));
		}
		// Hujralar (ichki devor bo'ylab), masjid gumbazi, quduq, yuk to'plari
		for (float t = -14.f; t <= 14.f; t += 7.f)
		{
			M.AddBox(W(KE(t), KN(-Half + 4.f), Z + 1.8f), FVector(300, 330, 180), ErtCol::Vary(Mud * 1.05f, 0.06f, ++S));
			M.AddBox(W(KE(-Half + 4.f), KN(t), Z + 1.8f), FVector(330, 300, 180), ErtCol::Vary(Mud * 1.05f, 0.06f, ++S));
			M.AddBox(W(KE(Half - 4.f), KN(t), Z + 1.8f), FVector(330, 300, 180), ErtCol::Vary(Mud * 1.05f, 0.06f, ++S));
		}
		M.AddBox(W(KE(0), KN(0), Z + 2.5f), FVector(600, 600, 250), FLinearColor(0.85f, 0.80f, 0.68f));
		M.AddSphere(W(KE(0), KN(0), Z + 5.f), 5.f, 12, FLinearColor(0.20f, 0.45f, 0.55f), FVector(1, 1, 0.75f));
		M.AddCylinder(W(KE(9), KN(9), Z), 1.1f, 1.1f, 0.9f, 10, Stone, false);
		for (int32 i = 0; i < 6; ++i) M.AddBox(W(KE(-8.f + i * 3.f), KN(10.f), Z + 0.6f), FVector(70, 50, 60), ErtCol::Vary(FLinearColor(0.45f, 0.35f, 0.2f), 0.2f, ++S));
		for (int32 s2 = -1; s2 <= 1; s2 += 2) AddBanner(M, KE(s2 * 5.f), KN(Half + 1.5f), Z, 5.f, FLinearColor(0.2f, 0.45f, 0.55f), false);
		AddFire(M, KE(-10), KN(-2), Z, true);
	}
	// Qadimiy xarobalar (g'arbda): singan ustunlar, devor parchalari
	{
		const float RE = -120.f, RN = -860.f;
		const float Z = HeightAt(RE, RN);
		const FLinearColor Marble(0.78f, 0.74f, 0.66f);
		for (int32 i = 0; i < 14; ++i)
		{
			const float u = -18.f + (i % 7) * 6.f, v = (i / 7) ? 8.f : -8.f;
			const float Hh = RS.FRandRange(1.5f, 7.f);
			M.AddCylinder(W(RE + u, RN + v, HeightAt(RE + u, RN + v) - 0.2f), 0.7f, 0.62f, Hh, 8, ErtCol::Vary(Marble, 0.08f, ++S), true);
			M.AddBox(W(RE + u, RN + v, HeightAt(RE + u, RN + v)), FVector(110, 110, 25), Marble * 0.9f);
		}
		M.AddBox(W(RE, RN - 14.f, Z + 2.f), FVector(1800, 120, 200), ErtCol::Vary(Marble * 0.85f, 0.1f, ++S), FRotator(0, 0, 4.f));
		M.AddBox(W(RE + 16.f, RN + 2.f, Z + 1.f), FVector(120, 900, 100), ErtCol::Vary(Marble * 0.85f, 0.1f, ++S));
		for (int32 i = 0; i < 10; ++i)
			M.AddSphere(W(RE + RS.FRandRange(-25.f, 25.f), RN + RS.FRandRange(-20.f, 20.f), Z), RS.FRandRange(0.5f, 1.4f), 6, ErtCol::Vary(Marble * 0.8f, 0.1f, ++S), FVector(1.3f, 1.f, 0.5f), 0.3f, S);
	}
	// Cho'l toshlari
	for (int32 i = 0; i < 90; ++i)
	{
		const float E = RS.FRandRange(-950.f, 950.f), N = RS.FRandRange(-990.f, DesertN - 20.f);
		if (!IsBuildable(E, N)) continue;
		M.AddSphere(W(E, N, HeightAt(E, N) - 0.4f), RS.FRandRange(0.8f, 3.5f), 7, ErtCol::Vary(FLinearColor(0.66f, 0.55f, 0.38f), 0.12f, ++S), FVector(RS.FRandRange(0.8f, 1.6f), 1.f, RS.FRandRange(0.4f, 0.8f)), 0.2f, S);
	}
	M.Commit(NewPart(TEXT("Desert"), true), 0, true);
}

// ---------------- Damashq: devorli katta shahar ----------------

void AErtWorldBuilder::BuildDamascus()
{
	FRandomStream RS(Seed + 77);
	FErtMeshData M(100.f);
	int32 S = 1500;
	const float Z = DamZ;
	auto DE = [](float u) { return DamE + u; };
	auto DN = [](float v) { return DamN + v; };
	const FLinearColor Lime(0.86f, 0.82f, 0.70f), Basalt(0.30f, 0.30f, 0.32f), DWood(0.42f, 0.28f, 0.14f), DGold(0.9f, 0.75f, 0.25f), Teal(0.18f, 0.5f, 0.55f), Green(0.16f, 0.40f, 0.14f);
	// Devorlar (8 m baland, 2.5 m qalin), kungura, burjlar
	const float WallH = 8.f;
	for (int32 side = 0; side < 4; ++side)
	{
		const bool bEW = side < 2;                       // 0,1: shimol/janub devori (E bo'ylab), 2,3: g'arb/sharq
		const float Sign = (side & 1) ? 1.f : -1.f;
		const float Len = bEW ? DamHalfE : DamHalfN;
		for (float t = -Len; t < Len; t += 5.f)
		{
			const float u = bEW ? t + 2.5f : Sign * DamHalfE, v = bEW ? Sign * DamHalfN : t + 2.5f;
			const bool bGate = (side == 0 && FMath::Abs(u) < 6.f) || (side == 2 && FMath::Abs(v) < 6.f);
			if (bGate) { M.AddBox(W(DE(u), DN(v), Z + 7.5f), FVector(bEW ? 250 : 125, bEW ? 125 : 250, 100), Lime * 0.95f); continue; }
			M.AddBox(W(DE(u), DN(v), Z + WallH * 0.5f), FVector(bEW ? 250 : 125, bEW ? 125 : 250, WallH * 50.f), ErtCol::Vary(Lime, 0.05f, ++S));
			M.AddBox(W(DE(u), DN(v), Z + WallH + 0.5f), FVector(bEW ? 60 : 125, bEW ? 125 : 60, 50), ErtCol::Vary(Lime * 0.92f, 0.05f, ++S));
		}
	}
	for (int32 i = 0; i < 4; ++i)
	{
		const float u = (i & 1) ? DamHalfE : -DamHalfE, v = (i & 2) ? DamHalfN : -DamHalfN;
		M.AddCylinder(W(DE(u), DN(v), Z), 5.f, 4.6f, 13.f, 12, ErtCol::Vary(Lime, 0.04f, ++S), true);
		M.AddCylinder(W(DE(u), DN(v), Z + 13.f), 5.4f, 5.4f, 1.2f, 12, Lime * 0.9f, true);
	}
	// Darvoza minoralari
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(DE(k * 9.f), DN(-DamHalfN), Z + 6.f), FVector(300, 300, 600), ErtCol::Vary(Lime, 0.04f, ++S)); M.AddBox(W(DE(-DamHalfE), DN(k * 9.f), Z + 6.f), FVector(300, 300, 600), ErtCol::Vary(Lime, 0.04f, ++S)); }
	// Umaviylar masjidi: hovli devori 60x40, ibodatxona 60x18, katta gumbaz, baland kvadrat minora
	{
		const float MU = 20.f, MV = 25.f;
		M.AddBox(W(DE(MU), DN(MV), Z + 0.1f), FVector(3000, 2000, 10), FLinearColor(0.92f, 0.9f, 0.85f));                       // marmar hovli
		for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(DE(MU + k * 30.f), DN(MV), Z + 3.f), FVector(80, 2000, 300), Lime); M.AddBox(W(DE(MU), DN(MV + k * 20.f), Z + 3.f), FVector(3000, 80, 300), Lime); }
		M.AddBox(W(DE(MU), DN(MV - 12.f), Z + 5.f), FVector(3000, 900, 500), ErtCol::Vary(Lime, 0.03f, ++S));                    // ibodatxona
		M.AddBox(W(DE(MU), DN(MV - 12.f), Z + 10.5f), FVector(1200, 900, 60), Lime * 0.9f);
		M.AddSphere(W(DE(MU), DN(MV - 12.f), Z + 10.f), 8.5f, 14, Basalt, FVector(1, 1, 0.8f));                                   // gumbaz
		M.AddCylinder(W(DE(MU + 28.f), DN(MV + 18.f), Z), 2.4f, 2.2f, 34.f, 4, Lime, true);                                        // kvadrat minora
		M.AddCylinder(W(DE(MU + 28.f), DN(MV + 18.f), Z + 34.f), 2.8f, 2.8f, 1.5f, 8, Lime * 0.9f, true);
		M.AddCylinder(W(DE(MU + 28.f), DN(MV + 18.f), Z + 35.5f), 1.6f, 0.6f, 5.f, 8, Teal, true);
		M.AddSphere(W(DE(MU + 28.f), DN(MV + 18.f), Z + 41.f), 0.6f, 6, DGold);
		M.AddCylinder(W(DE(MU), DN(MV + 6.f), Z), 2.5f, 2.5f, 0.8f, 12, Basalt, true);                                             // hovli favvorasi
	}
	// Saroy (janubi-sharq): ikki qavatli, gumbazli, ayvonli
	{
		const float PU = 85.f, PV = -55.f;
		M.AddBox(W(DE(PU), DN(PV), Z + 4.f), FVector(2200, 1600, 400), ErtCol::Vary(Lime, 0.03f, ++S));
		M.AddBox(W(DE(PU), DN(PV), Z + 10.f), FVector(1400, 1000, 200), ErtCol::Vary(Lime * 0.97f, 0.03f, ++S));
		M.AddSphere(W(DE(PU), DN(PV), Z + 12.f), 6.f, 12, Teal, FVector(1, 1, 0.8f));
		for (int32 i = 0; i < 6; ++i) M.AddCylinder(W(DE(PU - 22.f), DN(PV - 12.f + i * 5.f), Z), 0.5f, 0.5f, 4.f, 8, Lime * 0.9f, true);
		M.AddBox(W(DE(PU - 22.f), DN(PV), Z + 4.3f), FVector(150, 1400, 30), DWood);
	}
	// Bozor ko'chasi (shimoliy darvozadan masjidgacha): arkadalar va rastalar
	for (float v = -DamHalfN + 12.f; v < 5.f; v += 8.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(DE(k * 9.f), DN(v), Z + 3.2f), FVector(150, 350, 20), ErtCol::Vary(Lime, 0.05f, ++S));                    // ark usti
			M.AddCylinder(W(DE(k * 9.f), DN(v - 3.f), Z), 0.4f, 0.4f, 3.2f, 8, Lime, true); M.AddCylinder(W(DE(k * 9.f), DN(v + 3.f), Z), 0.4f, 0.4f, 3.2f, 8, Lime, true);
			M.AddBox(W(DE(k * 9.f), DN(v), Z + 0.8f), FVector(70, 200, 15), DWood);
			M.AddBox(W(DE(k * 9.f), DN(v), Z + 1.1f), FVector(40, 150, 20), FLinearColor(RS.FRand() * 0.6f + 0.3f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.3f + 0.1f));
		}
	// Uylar: tekis tomli, ba'zilari gumbazli, 2 qavatli; ko'chalar grid
	for (float u = -DamHalfE + 14.f; u < DamHalfE - 10.f; u += 16.f)
		for (float v = -DamHalfN + 14.f; v < DamHalfN - 10.f; v += 16.f)
		{
			if (FMath::Abs(u) < 14.f && v < 8.f) continue;                                    // bozor ko'chasi
			if (FMath::Abs(u - 20.f) < 36.f && FMath::Abs(v - 25.f) < 26.f) continue;         // masjid
			if (FMath::Abs(u - 85.f) < 28.f && FMath::Abs(v + 55.f) < 22.f) continue;         // saroy
			if (RS.FRand() < 0.12f) continue;
			const float HW = RS.FRandRange(4.5f, 6.5f), HD = RS.FRandRange(4.5f, 6.5f), HH = RS.FRand() < 0.35f ? 6.5f : 3.8f;
			const float E = DE(u + RS.FRandRange(-2.f, 2.f)), N = DN(v + RS.FRandRange(-2.f, 2.f));
			M.AddBox(W(E, N, Z + HH * 0.5f), FVector(HW * 100.f, HD * 100.f, HH * 50.f), ErtCol::Vary(Lime, 0.08f, ++S));
			M.AddBox(W(E, N, Z + HH + 0.3f), FVector(HW * 100.f + 15.f, HD * 100.f + 15.f, 30.f), ErtCol::Vary(Lime * 0.9f, 0.06f, ++S));
			if (RS.FRand() < 0.3f) M.AddSphere(W(E, N, Z + HH), FMath::Min(HW, HD) * 0.7f, 8, ErtCol::Vary(Lime * 0.95f, 0.05f, ++S), FVector(1, 1, 0.6f));
			M.AddBox(W(E + HW * 0.5f, N, Z + 1.1f), FVector(12, 60, 110), DWood);
		}
	// Palmalar (hovlilar va darvoza oldi)
	for (int32 i = 0; i < 24; ++i)
	{
		const float u = RS.FRandRange(-DamHalfE + 8.f, DamHalfE - 8.f), v = RS.FRandRange(-DamHalfN + 8.f, DamHalfN - 8.f);
		if (FMath::Abs(u) < 14.f && v < 8.f) continue;
		AddPalm(M, DE(u), DN(v), Z, RS.FRandRange(6.f, 9.f), ++S);
	}
	for (int32 i = 0; i < 8; ++i) AddPalm(M, DE(-30.f + i * 9.f), DN(-DamHalfN - 12.f), HeightAt(DE(-30.f + i * 9.f), DN(-DamHalfN - 12.f)), RS.FRandRange(7.f, 10.f), ++S);
	// Bayroqlar va darvoza oldi olovi
	AddBanner(M, DE(-9.f), DN(-DamHalfN - 3.f), Z, 6.f, Green, false);
	AddBanner(M, DE(9.f), DN(-DamHalfN - 3.f), Z, 6.f, Green, false);
	AddFire(M, DE(18.f), DN(-DamHalfN + 10.f), Z, true);
	M.Commit(NewPart(TEXT("Damascus"), true), 0, true);
}

// ---------------- Halab: tepalikdagi qal'a va aylana shahar ----------------

void AErtWorldBuilder::BuildHalab()
{
	FRandomStream RS(Seed + 91);
	FErtMeshData M(100.f);
	int32 S = 2500;
	const float Z = HalabZ, TopZ = HalabZ + HalabMoundH;
	auto HE = [](float u) { return HalabE + u; };
	auto HN = [](float v) { return HalabN + v; };
	const FLinearColor HStone(0.72f, 0.68f, 0.60f), HDark(0.45f, 0.42f, 0.38f), HWood(0.40f, 0.27f, 0.13f), Copper(0.35f, 0.55f, 0.45f), HRed(0.6f, 0.12f, 0.1f);
	// Tashqi devor: 16 burchakli halqa, har ikkinchi burchakda burj, sharqda darvoza
	const int32 Sides = 16;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * HalabR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * HalabR, cv = FMath::Sin(Am) * HalabR;
		const bool bGate = (i == 0 || i == Sides - 1);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) { M.AddBox(W(HE(cu), HN(cv), Z + 7.5f), FVector(Len * 50.f, 130, 90), HStone, FRotator(0, Yaw, 0)); }
		else
		{
			M.AddBox(W(HE(cu), HN(cv), Z + 4.f), FVector(Len * 50.f + 20.f, 130, 400), ErtCol::Vary(HStone, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
			{
				const float du = -FMath::Sin(Am) * t, dv = FMath::Cos(Am) * t;
				M.AddBox(W(HE(cu + du), HN(cv + dv), Z + 8.6f), FVector(60, 130, 60), ErtCol::Vary(HStone * 0.92f, 0.05f, ++S), FRotator(0, Yaw, 0));
			}
		}
		if (i % 2 == 0)
		{
			const float tu = FMath::Cos(A0) * HalabR, tv = FMath::Sin(A0) * HalabR;
			M.AddCylinder(W(HE(tu), HN(tv), Z), 4.2f, 3.8f, 11.f, 10, ErtCol::Vary(HStone, 0.04f, ++S), true);
			M.AddCylinder(W(HE(tu), HN(tv), Z + 11.f), 4.4f, 4.4f, 1.f, 10, HStone * 0.9f, true);
		}
	}
	// Sharqiy darvoza minoralari
	for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(HE(HalabR), HN(k * 9.f), Z + 6.5f), FVector(400, 300, 650), ErtCol::Vary(HStone, 0.04f, ++S));
	// Qal'a (tepalik ustida): halqa devor, burjlar, ichida saroy, hammom, minora
	{
		const float CR = 30.f;
		for (int32 i = 0; i < 12; ++i)
		{
			const float A0 = i * 2.f * PI / 12, A1 = (i + 1) * 2.f * PI / 12, Am = (A0 + A1) * 0.5f;
			const float Len = 2.f * CR * FMath::Sin(PI / 12);
			M.AddBox(W(HE(FMath::Cos(Am) * CR), HN(FMath::Sin(Am) * CR), TopZ + 5.f), FVector(Len * 50.f + 20.f, 150, 500), ErtCol::Vary(HDark, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
			if (i % 3 == 0) { M.AddCylinder(W(HE(FMath::Cos(A0) * CR), HN(FMath::Sin(A0) * CR), TopZ), 4.5f, 4.f, 14.f, 10, ErtCol::Vary(HDark, 0.04f, ++S), true); M.AddCylinder(W(HE(FMath::Cos(A0) * CR), HN(FMath::Sin(A0) * CR), TopZ + 14.f), 4.8f, 4.8f, 1.f, 10, HDark * 0.9f, true); }
		}
		// Qal'a darvozasi (janubda) va ko'prik: tepalikdan shahar tekisligiga qiya yo'lak, arklar ustida
		M.AddBox(W(HE(0.f), HN(-CR), TopZ + 8.f), FVector(1000, 500, 800), ErtCol::Vary(HDark, 0.03f, ++S));
		M.AddBox(W(HE(0.f), HN(-CR + 2.f), TopZ + 4.f), FVector(250, 400, 100), HStone * 0.3f);
		const int32 Steps = 14;
		const float Run = HalabMoundR + 26.f - CR;
		const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(HalabMoundH, Run));
		for (int32 i = 0; i < Steps; ++i)
		{
			const float t = (i + 0.5f) / Steps;
			const float v = -CR - 2.f - t * Run, Zc = FMath::Lerp(TopZ + 0.5f, Z + 0.5f, t);
			M.AddBox(W(HE(0.f), HN(v), Zc), FVector(300, 260, 30), ErtCol::Vary(HStone, 0.03f, ++S), FRotator(-Pitch, 0, 0));
			const float G = HeightAt(HE(0.f), HN(v));
			if (i % 2 == 1 && Zc - G > 2.f) M.AddBox(W(HE(0.f), HN(v), (Zc + G) * 0.5f), FVector(240, 120, (Zc - G) * 50.f), ErtCol::Vary(HStone * 0.9f, 0.03f, ++S));
			for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(HE(k * 3.2f), HN(v), Zc + 0.9f), FVector(30, 260, 90), HStone * 0.85f, FRotator(-Pitch, 0, 0));
		}
		M.AddBox(W(HE(-8.f), HN(6.f), TopZ + 4.f), FVector(1400, 1000, 400), ErtCol::Vary(HStone, 0.03f, ++S));
		M.AddBox(W(HE(-8.f), HN(6.f), TopZ + 9.f), FVector(800, 600, 100), ErtCol::Vary(HStone, 0.03f, ++S));
		M.AddSphere(W(HE(-8.f), HN(6.f), TopZ + 10.f), 4.f, 10, Copper, FVector(1, 1, 0.7f));
		M.AddBox(W(HE(12.f), HN(12.f), TopZ + 1.5f), FVector(900, 400, 150), ErtCol::Vary(HStone, 0.03f, ++S));
		for (int32 i = 0; i < 3; ++i) M.AddSphere(W(HE(7.f + i * 5.f), HN(12.f), TopZ + 3.f), 2.4f, 8, ErtCol::Vary(HStone, 0.05f, ++S), FVector(1, 1, 0.8f));
		M.AddCylinder(W(HE(14.f), HN(-8.f), TopZ), 1.6f, 1.4f, 22.f, 10, HStone, true);
		M.AddCylinder(W(HE(14.f), HN(-8.f), TopZ + 22.f), 2.f, 2.f, 1.2f, 10, HStone * 0.9f, true);
		M.AddCylinder(W(HE(14.f), HN(-8.f), TopZ + 23.2f), 1.2f, 0.4f, 3.5f, 8, Copper, true);
		AddBanner(M, HE(-7.f), HN(-CR + 4.f), TopZ, 7.f, HRed, false);
		AddBanner(M, HE(7.f), HN(-CR + 4.f), TopZ, 7.f, HRed, false);
		AddFire(M, HE(0.f), HN(0.f), TopZ, true);
	}
	// Shahar: tepalik atrofida halqa-halqa uylar
	for (float R = HalabMoundR + 12.f; R < HalabR - 10.f; R += 14.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 13.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(u) < 6.f && v < -HalabMoundR + 4.f) continue;
			if (FMath::Abs(v) < 7.f && u > 0.f) continue;
			if (FMath::Abs(u + 60.f) < 14.f && FMath::Abs(v - 55.f) < 12.f) continue;
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.45f ? 6.5f : 3.8f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(HE(u), HN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(HStone, 0.08f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(HE(u), HN(v), Z + HH + 0.25f), FVector(HW * 50.f + 12.f, HD * 50.f + 12.f, 25.f), ErtCol::Vary(HStone * 0.85f, 0.06f, ++S), FRotator(0, Yaw, 0));
			if (RS.FRand() < 0.25f) M.AddSphere(W(HE(u), HN(v), Z + HH), FMath::Min(HW, HD) * 0.35f, 8, ErtCol::Vary(HStone * 0.95f, 0.05f, ++S), FVector(1, 1, 0.6f));
		}
	}
	// Yopiq bozor: sharqiy darvozadan tepalikkacha gumbazli tomli ko'cha
	for (float u = HalabMoundR + 14.f; u < HalabR - 8.f; u += 7.f)
	{
		M.AddBox(W(HE(u), HN(0.f), Z + 4.f), FVector(330, 1100, 25), ErtCol::Vary(HStone * 0.9f, 0.05f, ++S));
		M.AddSphere(W(HE(u), HN(0.f), Z + 4.f), 2.f, 8, ErtCol::Vary(HStone, 0.05f, ++S), FVector(1, 1, 0.5f));
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddCylinder(W(HE(u - 3.f), HN(k * 5.4f), Z), 0.35f, 0.35f, 4.f, 8, HStone, true); M.AddCylinder(W(HE(u + 3.f), HN(k * 5.4f), Z), 0.35f, 0.35f, 4.f, 8, HStone, true);
			M.AddBox(W(HE(u), HN(k * 4.f), Z + 0.8f), FVector(220, 70, 15), HWood);
			M.AddBox(W(HE(u), HN(k * 4.f), Z + 1.1f), FVector(160, 40, 20), FLinearColor(RS.FRand() * 0.6f + 0.3f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.3f + 0.1f));
		}
	}
	// Katta masjid (shimoli-g'arb): hovli, gumbaz, dumaloq minora
	{
		const float MU = -60.f, MV = 55.f;
		M.AddBox(W(HE(MU), HN(MV), Z + 0.1f), FVector(2200, 1600, 10), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(HE(MU), HN(MV - 9.f), Z + 4.5f), FVector(2200, 700, 450), ErtCol::Vary(HStone, 0.03f, ++S));
		M.AddSphere(W(HE(MU), HN(MV - 9.f), Z + 9.f), 6.5f, 12, Copper, FVector(1, 1, 0.75f));
		for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(HE(MU + k * 22.f), HN(MV + 3.f), Z + 2.5f), FVector(80, 1000, 250), HStone);
		M.AddBox(W(HE(MU), HN(MV + 16.f), Z + 2.5f), FVector(2200, 80, 250), HStone);
		M.AddCylinder(W(HE(MU + 20.f), HN(MV + 14.f), Z), 1.8f, 1.4f, 28.f, 12, HStone, true);
		M.AddCylinder(W(HE(MU + 20.f), HN(MV + 14.f), Z + 28.f), 2.2f, 2.2f, 1.2f, 12, HStone * 0.9f, true);
		M.AddCylinder(W(HE(MU + 20.f), HN(MV + 14.f), Z + 29.2f), 1.3f, 0.4f, 4.f, 8, Copper, true);
	}
	for (int32 i = 0; i < 18; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(HalabMoundR + 10.f, HalabR - 12.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(v) < 8.f && u > 0.f) continue;
		AddTree(M, HE(u), HN(v), Z, RS.FRandRange(0.8f, 1.2f), true, ++S);
	}
	AddBanner(M, HE(HalabR + 3.f), HN(-9.f), Z, 6.f, HRed, false);
	AddBanner(M, HE(HalabR + 3.f), HN(9.f), Z, 6.f, HRed, false);
	M.Commit(NewPart(TEXT("Halab"), true), 0, true);
}

// ---------------- Konya: Saljuqiylar poytaxti ----------------

void AErtWorldBuilder::BuildKonya()
{
	FRandomStream RS(Seed + 113);
	FErtMeshData M(100.f);
	int32 S = 3500;
	const float Z = KonZ, TopZ = KonZ + KonHillH;
	auto KE = [](float u) { return KonE + u; };
	auto KN = [](float v) { return KonN + v; };
	const FLinearColor KStone(0.76f, 0.70f, 0.58f), KDark(0.50f, 0.45f, 0.38f), KWood(0.38f, 0.25f, 0.12f), Turq(0.12f, 0.58f, 0.62f), Tile(0.16f, 0.32f, 0.62f), KGold(0.9f, 0.75f, 0.25f), Plane(0.22f, 0.45f, 0.18f), Trunk(0.55f, 0.50f, 0.42f);
	// Devor: 24 burchak, har burchakda burj (Konya "yuz burj" shahri), sharqiy va g'arbiy darvozalar
	const int32 Sides = 24;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * KonR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * KonR, cv = FMath::Sin(Am) * KonR;
		const bool bGate = (i == 0 || i == Sides - 1 || i == Sides / 2 || i == Sides / 2 - 1);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) M.AddBox(W(KE(cu), KN(cv), Z + 7.f), FVector(Len * 50.f, 120, 80), KStone, FRotator(0, Yaw, 0));
		else
		{
			M.AddBox(W(KE(cu), KN(cv), Z + 3.5f), FVector(Len * 50.f + 20.f, 120, 350), ErtCol::Vary(KStone, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
				M.AddBox(W(KE(cu - FMath::Sin(Am) * t), KN(cv + FMath::Cos(Am) * t), Z + 7.6f), FVector(60, 120, 60), ErtCol::Vary(KStone * 0.92f, 0.05f, ++S), FRotator(0, Yaw, 0));
		}
		const float tu = FMath::Cos(A0) * KonR, tv = FMath::Sin(A0) * KonR;
		M.AddBox(W(KE(tu), KN(tv), Z + 5.f), FVector(320, 320, 500), ErtCol::Vary(KStone, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A0), 0));
		M.AddBox(W(KE(tu), KN(tv), Z + 10.4f), FVector(360, 360, 40), KStone * 0.9f, FRotator(0, FMath::RadiansToDegrees(A0), 0));
	}
	for (int32 side = -1; side <= 1; side += 2)
		for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(KE(side * KonR), KN(k * 9.f), Z + 6.f), FVector(380, 300, 600), ErtCol::Vary(KStone, 0.04f, ++S)); M.AddBox(W(KE(side * KonR), KN(k * 9.f), Z + 12.4f), FVector(420, 340, 40), KStone * 0.9f); }
	// Aloiddin tepaligi: masjid (uzun ibodatxona, feruza gumbaz, qalin kvadrat minora), saroy ko'shki, ichki devor
	{
		M.AddBox(W(KE(-10.f), KN(4.f), TopZ + 3.5f), FVector(2600, 1300, 350), ErtCol::Vary(KStone, 0.03f, ++S));
		M.AddBox(W(KE(-10.f), KN(4.f), TopZ + 7.3f), FVector(2600, 1300, 30), KStone * 0.85f);
		M.AddBox(W(KE(2.f), KN(-2.f), TopZ + 8.f), FVector(500, 500, 100), KStone);
		M.AddSphere(W(KE(2.f), KN(-2.f), TopZ + 9.f), 5.2f, 12, Turq, FVector(1, 1, 0.8f));
		M.AddBox(W(KE(-30.f), KN(12.f), TopZ + 0.5f), FVector(600, 400, 50), KStone);
		M.AddCylinder(W(KE(-30.f), KN(12.f), TopZ), 2.f, 1.7f, 26.f, 4, KStone, true);
		M.AddCylinder(W(KE(-30.f), KN(12.f), TopZ + 26.f), 2.4f, 2.4f, 1.f, 8, KStone * 0.9f, true);
		M.AddCylinder(W(KE(-30.f), KN(12.f), TopZ + 27.f), 1.6f, 0.5f, 4.f, 8, Turq, true);
		// Saroy ko'shki (tepalik chetida, ustunli ayvon, chodir tom)
		M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 4.f), FVector(700, 700, 400), ErtCol::Vary(KStone, 0.03f, ++S));
		for (int32 i = 0; i < 4; ++i) M.AddCylinder(W(KE(30.f + ((i & 1) ? 8.f : -8.f)), KN(-28.f + ((i & 2) ? 8.f : -8.f)), TopZ + 8.f), 0.5f, 0.5f, 4.f, 8, KStone, true);
		M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 12.2f), FVector(950, 950, 25), KWood);
		M.AddCylinder(W(KE(30.f), KN(-28.f), TopZ + 12.4f), 9.f, 0.3f, 5.f, 4, Tile, true);
		M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 10.5f), FVector(400, 30, 250), KWood); M.AddBox(W(KE(30.f), KN(-28.f), TopZ + 10.5f), FVector(30, 400, 250), KWood);
		// Ichki devor halqasi (past)
		for (int32 i = 0; i < 16; ++i)
		{
			const float Am = (i + 0.5f) * 2.f * PI / 16, Len = 2.f * (KonHillR - 6.f) * FMath::Sin(PI / 16);
			if (i == 12) continue;   // janubiy yo'lak
			M.AddBox(W(KE(FMath::Cos(Am) * (KonHillR - 6.f)), KN(FMath::Sin(Am) * (KonHillR - 6.f)), TopZ - 1.5f + 1.5f), FVector(Len * 50.f + 20.f, 100, 200), ErtCol::Vary(KDark, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
		}
		AddBanner(M, KE(24.f), KN(-40.f), TopZ, 7.f, Tile, false);
		AddBanner(M, KE(36.f), KN(-40.f), TopZ, 7.f, Tile, false);
		AddFire(M, KE(10.f), KN(14.f), TopZ, true);
	}
	// Kumbetlar (konus tomli maqbaralar) - tepalik janubi-g'arbida
	for (int32 i = 0; i < 4; ++i)
	{
		const float u = -70.f + i * 12.f, v = -70.f + (i & 1) * 8.f;
		M.AddCylinder(W(KE(u), KN(v), Z), 3.2f, 3.2f, 7.f, 8, ErtCol::Vary(KStone, 0.05f, ++S), true);
		M.AddCylinder(W(KE(u), KN(v), Z + 7.f), 3.6f, 0.2f, 5.5f, 8, i % 2 ? Turq : KDark, true);
	}
	// Madrasa (shimoli-sharq): hovli, katta portal, naqshli baland minora (Ince Minareli)
	{
		const float MU = 60.f, MV = 60.f;
		M.AddBox(W(KE(MU), KN(MV), Z + 3.f), FVector(1600, 1400, 300), ErtCol::Vary(KStone, 0.03f, ++S));
		M.AddBox(W(KE(MU), KN(MV), Z + 0.2f), FVector(800, 700, 20), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(KE(MU), KN(MV - 14.f), Z + 6.f), FVector(500, 150, 600), ErtCol::Vary(KStone, 0.03f, ++S));   // portal
		M.AddBox(W(KE(MU), KN(MV - 14.6f), Z + 3.5f), FVector(200, 40, 350), KStone * 0.3f);
		M.AddSphere(W(KE(MU - 10.f), KN(MV + 4.f), Z + 6.f), 4.5f, 12, Turq, FVector(1, 1, 0.75f));
		M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z), 1.5f, 1.5f, 8.f, 8, KStone, true);
		M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z + 8.f), 1.2f, 1.0f, 22.f, 12, Tile, true);
		for (int32 k = 0; k < 2; ++k) M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z + 14.f + k * 8.f), 1.6f, 1.6f, 0.8f, 12, Turq, true);
		M.AddCylinder(W(KE(MU + 10.f), KN(MV - 10.f), Z + 30.f), 1.2f, 0.3f, 3.5f, 8, KGold, true);
	}
	// Karvonsaroy (janubi-sharq) va bozor
	{
		const float CU = 75.f, CV = -60.f;
		M.AddBox(W(KE(CU), KN(CV), Z + 3.f), FVector(1800, 1400, 300), ErtCol::Vary(KStone, 0.03f, ++S));
		M.AddBox(W(KE(CU), KN(CV), Z + 0.2f), FVector(1000, 700, 20), FLinearColor(0.7f, 0.65f, 0.55f));
		M.AddBox(W(KE(CU - 18.f), KN(CV), Z + 5.f), FVector(150, 500, 500), KStone);
		M.AddBox(W(KE(CU), KN(CV + 5.f), Z + 6.5f), FVector(1000, 700, 50), KStone * 0.9f);
	}
	for (float u = KonHillR + 8.f; u < KonR - 12.f; u += 7.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(KE(u), KN(k * 7.f), Z + 3.f), FVector(330, 250, 20), ErtCol::Vary(KWood, 0.1f, ++S));
			M.AddCylinder(W(KE(u - 3.f), KN(k * 9.4f), Z), 0.3f, 0.3f, 3.f, 6, KWood, true); M.AddCylinder(W(KE(u + 3.f), KN(k * 9.4f), Z), 0.3f, 0.3f, 3.f, 6, KWood, true);
			M.AddBox(W(KE(u), KN(k * 7.f), Z + 0.8f), FVector(220, 70, 15), KWood);
			M.AddBox(W(KE(u), KN(k * 7.f), Z + 1.1f), FVector(160, 40, 20), FLinearColor(RS.FRand() * 0.5f + 0.2f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.5f + 0.2f));
		}
	// Uylar: halqa-halqa, yog'och ayvonli ikki qavatli
	for (float R = KonHillR + 14.f; R < KonR - 12.f; R += 15.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 14.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(v) < 12.f) continue;                                                  // sharq-g'arb xiyobon
			if (FMath::Abs(u) < 7.f && v < 0.f) continue;                                        // janubiy yo'lak
			if (FMath::Abs(u - 60.f) < 12.f && FMath::Abs(v - 60.f) < 12.f) continue;
			if (FMath::Abs(u - 75.f) < 14.f && FMath::Abs(v + 60.f) < 12.f) continue;
			if (FMath::Abs(u + 60.f) < 26.f && FMath::Abs(v + 68.f) < 10.f) continue;
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.5f ? 6.5f : 4.f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(KE(u), KN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(KStone, 0.08f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(KE(u), KN(v), Z + HH + 0.4f), FVector(HW * 50.f + 40.f, HD * 50.f + 40.f, 40.f), ErtCol::Vary(FLinearColor(0.55f, 0.32f, 0.2f), 0.06f, ++S), FRotator(0, Yaw, 0));   // sopol tom
			if (HH > 5.f) M.AddBox(W(KE(u), KN(v), Z + 3.9f), FVector(HW * 50.f + 50.f, HD * 50.f + 50.f, 12.f), KWood, FRotator(0, Yaw, 0));   // ayvon
		}
	}
	// Chinor xiyoboni (sharq-g'arb) va tepalik daraxtlari
	for (float u = -KonR + 14.f; u < KonR - 12.f; u += 11.f)
	{
		if (FMath::Abs(u) < KonHillR + 4.f) continue;
		for (int32 k = -1; k <= 1; k += 2)
		{
			const float E = KE(u), N = KN(k * 11.5f);
			M.AddCylinder(W(E, N, Z), 0.45f, 0.35f, 5.f, 6, Trunk, true);
			M.AddSphere(W(E, N, Z + 7.f), 4.f, 8, ErtCol::Vary(Plane, 0.08f, ++S), FVector(1, 1, 0.8f));
		}
	}
	for (int32 i = 0; i < 10; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(20.f, KonHillR - 12.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(u + 10.f) < 16.f && FMath::Abs(v - 4.f) < 9.f) continue;
		if (FMath::Abs(u - 30.f) < 10.f && FMath::Abs(v + 28.f) < 10.f) continue;
		const float ZZ = HeightAt(KE(u), KN(v));
		M.AddCylinder(W(KE(u), KN(v), ZZ), 0.4f, 0.3f, 4.f, 6, Trunk, true);
		M.AddSphere(W(KE(u), KN(v), ZZ + 5.5f), 3.2f, 8, ErtCol::Vary(Plane, 0.08f, ++S), FVector(1, 1, 0.8f));
	}
	AddBanner(M, KE(KonR + 3.f), KN(-9.f), Z, 6.f, Tile, false);
	AddBanner(M, KE(KonR + 3.f), KN(9.f), Z, 6.f, Tile, false);
	M.Commit(NewPart(TEXT("Konya"), true), 0, true);
}
