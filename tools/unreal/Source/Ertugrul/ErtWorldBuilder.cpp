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
	// Oba yonidagi kichik ko'l
	{
		const float LE = -700.f, LN = 700.f;
		for (int32 i = 0; i < 24; ++i)
		{
			const float A0 = 2.f * PI * i / 24, A1 = 2.f * PI * (i + 1) / 24;
			const float R0 = 38.f + 6.f * FMath::Sin(A0 * 3), R1 = 38.f + 6.f * FMath::Sin(A1 * 3);
			M.AddTri(W(LE, LN, WaterZ + 1.5f), W(LE + FMath::Cos(A0) * R0, LN + FMath::Sin(A0) * R0, WaterZ + 1.5f), W(LE + FMath::Cos(A1) * R1, LN + FMath::Sin(A1) * R1, WaterZ + 1.5f), FVector::UpVector, Wc);
		}
	}
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
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(-700.f, 700.f)) < 50.f) return false; // ko'l
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
		if (H > 150.f || Nm.Z < 0.72f || H < WaterZ + 1.5f) continue;
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
