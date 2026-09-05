#include "ErtWorldBuilder.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ErtFab.h"
#include "Engine/StaticMesh.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DecalComponent.h"
#include "ErtFire.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
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
		{{300, 300}, {275, 480}, 6}, {{300, 300}, {320, 110}, 6}, {{-300, 250}, {-180, 275}, 6}, {{500, 520}, {420, 615}, 6}, {{-300, 250}, {-420, 65}, 6}, {{-150, -150}, {-190, -60}, 6}, {{-30, 480}, {-60, 690}, 5}, {{-60, 690}, {-160, 610}, 5},
		{{-470, -280}, {-470, -470}, 8}, {{-470, -470}, {-470, -660}, 8}, {{-660, -470}, {-280, -470}, 8},
	};
	const FLinearColor Stone = ErtCol::Sty(FLinearColor(0.46f, 0.44f, 0.41f), ErtCol::StyleStone), Wood = ErtCol::Sty(FLinearColor(0.36f, 0.24f, 0.13f), ErtCol::StyleWood), DarkWood = ErtCol::Sty(FLinearColor(0.24f, 0.16f, 0.09f), ErtCol::StyleWood);
	const FLinearColor Felt = ErtCol::Sty(FLinearColor(0.86f, 0.82f, 0.72f), ErtCol::StyleFelt), FeltDark = ErtCol::Sty(FLinearColor(0.24f, 0.22f, 0.20f), ErtCol::StyleFelt), Cream = ErtCol::Sty(FLinearColor(0.90f, 0.86f, 0.74f), ErtCol::StyleFelt);
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
	// Olov effektlari (olov tili + tutun + uchqun + miltillovchi nur)
	int32 NF = 0;
	for (const FVector4& F : FireSpots) if (AErtFireFx::Spawn(GetWorld(), FVector(F.X, F.Y, F.Z), F.W, true)) ++NF;
	UE_LOG(LogErtugrul, Log, TEXT("Olov effektlari: %d"), NF);
	FString ExpDir; if (FParse::Value(FCommandLine::Get(), TEXT("-ErtExportLandscape="), ExpDir)) ExportLandscape(ExpDir.TrimQuotes());
}

FLinearColor AErtWorldBuilder::ColorAt(float E, float N) const
{
	const float H = HeightAt(E, N);
	const FVector Nm = TerrainNormal(E, N);
	return TerrainColor(E, N, H, 1.f - Nm.Z);
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

UHierarchicalInstancedStaticMeshComponent* AErtWorldBuilder::FabComp(UStaticMesh* M, bool bCollision)
{
	if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Found = FabInst.Find(M)) return Found->Get();
	UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("Fab")));
	H->SetupAttachment(RootComponent);
	H->SetMobility(EComponentMobility::Static);
	H->SetStaticMesh(M);
	H->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bCollision) { H->SetCollisionObjectType(ECC_WorldStatic); H->SetCollisionResponseToAllChannels(ECR_Block); }
	H->SetCastShadow(true);
	H->bAffectDistanceFieldLighting = false;
	H->RegisterComponent();
	FabInst.Add(M, H);
	return H;
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
	GrassMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtGrass.M_ErtGrass"));
	LeafMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtLeaf.M_ErtLeaf"));
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
		BuildKayseri();
		BuildSivas();
		BuildErzurum();
		BuildBursa();
		BuildNikeya();
		BuildKaracahisar();
		BuildSogut();
		BuildDomanic();
	}
	if (bBuildSettlements) { BuildSplineWalls(); BuildLandmarks(); }
	if (bBuildForest) { BuildForest(); BuildRocks(); BuildGrass(); BuildShoreFoliage(); BuildBushes(); BuildProps(); }
	if (bBuildSettlements) BuildDecals();
	if (FabTreesPlaced + FabRocksPlaced > 0) UE_LOG(LogErtugrul, Log, TEXT("Fab meshlari: %d daraxt, %d qoya (instans)"), FabTreesPlaced, FabRocksPlaced);
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
	// Domaniç yaylovi: keng baland gumbaz (yassi tepa), mayin to'lqinlar
	{
		const float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(DomE, DomN));
		const float T = Smooth01(1.f - (DD - 20.f) / (DomR + 60.f));
		H += DomH * T + Noise(E, N, 0.02f) * 2.5f * T;
	}
	// So'g'ut: yumshoq tekis vodiy
	{
		const float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SogE, SogN));
		H = FMath::Lerp(H, SogZ, 1.f - Smooth01((DS - SogR - 5.f) / 55.f));
	}
	// Karacahisar: tekis etak + tik qora qoya (tepasi tekis plato)
	{
		const float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KarE, KarN));
		H = FMath::Lerp(H, KarBaseZ, 1.f - Smooth01((DK - KarR - 25.f) / 60.f));
		const float T = Smooth01(1.f - (DK - 30.f) / (KarR - 30.f));   // 30 m gacha tekis plato, keyin tik qiyalik
		H += KarH * T + Noise(E, N, 0.08f) * 3.f * T * (1.f - T) * 4.f;
	}
	// Nikeya: tekis aylana; Askaniya ko'li: qirg'oq tekis, o'rtasi chuqur
	{
		const float DN = FVector2D::Distance(FVector2D(E, N), FVector2D(NikE, NikN));
		H = FMath::Lerp(H, NikZ, 1.f - Smooth01((DN - NikR - 10.f) / 45.f));
		const float DA = FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN));
		H = FMath::Lerp(H, AskZ + 1.f, 1.f - Smooth01((DA - (AskR + 15.f)) / 35.f));
		if (DA < AskR) H -= 5.f * Smooth01(1.f - DA / AskR);
	}
	// Uludog': o'rmonli tog' (cho'qqisi qorli) va Bursa tekisligi + shimolda Hisor tepaligi
	{
		const float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(UluE, UluN));
		const float T = Smooth01(1.f - DM / UluR);
		H += UluH * T * T + Noise(E, N, 0.025f) * 7.f * T;
		const float DB = FVector2D::Distance(FVector2D(E, N), FVector2D(BurE, BurN));
		H = FMath::Lerp(H, BurZ, 1.f - Smooth01((DB - BurR - 10.f) / 50.f));
		const float DH = FVector2D::Distance(FVector2D(E, N), FVector2D(BurE - 30.f, BurN + 55.f));
		H += BurHillH * Smooth01(1.f - (DH - 8.f) / (BurHillR - 8.f));
	}
	// Erzurum: baland tekis aylana + shimoli-sharqda qal'a tepaligi
	{
		const float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE, ErzN));
		H = FMath::Lerp(H, ErzZ, 1.f - Smooth01((DS - ErzR - 10.f) / 60.f));
		const float DH = FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE + 45.f, ErzN + 45.f));
		H += ErzHillH * Smooth01(1.f - (DH - 8.f) / (ErzHillR - 8.f));
	}
	// Sivas: tekis aylana + shimolida past qal'a tepaligi
	{
		const float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN));
		H = FMath::Lerp(H, SivZ, 1.f - Smooth01((DS - SivR - 10.f) / 50.f));
		const float DH = FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN + 70.f));
		H += SivHillH * Smooth01(1.f - (DH - 10.f) / (SivHillR - 10.f));
	}
	// Erciyes: vulqon konusi (qoyali, cho'qqisi qorli) va Qayseri tekisligi
	{
		const float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(ErcE, ErcN));
		const float T = Smooth01(1.f - DM / ErcR);
		H += ErcH * T * T + Noise(E, N, 0.03f) * 6.f * T;
		const float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KayE, KayN));
		H = FMath::Lerp(H, KayZ, 1.f - Smooth01((DK - KayR - 10.f) / 50.f));
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
	{
		const float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(ErcE, ErcN));
		const float T = Smooth01(1.f - (DM - 15.f) / (ErcR - 15.f));
		C = FMath::Lerp(C, FLinearColor(0.30f, 0.29f, 0.30f), T * 0.9f);                                   // bazalt qoya
		C = FMath::Lerp(C, FLinearColor(0.95f, 0.96f, 1.0f), Smooth01((H - 75.f) / 20.f) * T);            // qor
		const float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KayE, KayN));
		C = FMath::Lerp(C, FLinearColor(0.42f, 0.40f, 0.38f), 1.f - Smooth01((DK - KayR) / 8.f));          // qora tosh yo'laklar
	}
	{
		const float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN));
		C = FMath::Lerp(C, FLinearColor(0.60f, 0.52f, 0.42f), 1.f - Smooth01((DS - SivR) / 8.f));
	}
	{
		const float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE, ErzN));
		C = FMath::Lerp(C, FLinearColor(0.50f, 0.48f, 0.46f), 1.f - Smooth01((DS - ErzR) / 8.f));
		C = FMath::Lerp(C, FLinearColor(0.80f, 0.80f, 0.78f), (1.f - Smooth01((DS - ErzR - 20.f) / 60.f)) * 0.35f);   // sovuq qirov o'tloq
	}
	{
		const float DM = FVector2D::Distance(FVector2D(E, N), FVector2D(UluE, UluN));
		const float T = Smooth01(1.f - (DM - 10.f) / (UluR - 10.f));
		C = FMath::Lerp(C, FLinearColor(0.20f, 0.36f, 0.16f), T * 0.7f);                                  // o'rmon yashili
		C = FMath::Lerp(C, FLinearColor(0.95f, 0.96f, 1.0f), Smooth01((H - 62.f) / 18.f) * T);            // qor
		const float DB = FVector2D::Distance(FVector2D(E, N), FVector2D(BurE, BurN));
		C = FMath::Lerp(C, FLinearColor(0.62f, 0.58f, 0.50f), 1.f - Smooth01((DB - BurR) / 8.f));
	}
	{
		const float DN = FVector2D::Distance(FVector2D(E, N), FVector2D(NikE, NikN));
		C = FMath::Lerp(C, FLinearColor(0.66f, 0.60f, 0.52f), 1.f - Smooth01((DN - NikR) / 8.f));
		const float DA = FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN));
		C = FMath::Lerp(C, FLinearColor(0.76f, 0.70f, 0.52f), 1.f - Smooth01((DA - AskR - 2.f) / 10.f));   // qumloq qirg'oq
	}
	{
		const float DK = FVector2D::Distance(FVector2D(E, N), FVector2D(KarE, KarN));
		C = FMath::Lerp(C, FLinearColor(0.22f, 0.21f, 0.22f), 1.f - Smooth01((DK - KarR + 2.f) / 10.f));   // qora qoya
	}
	{
		const float DS = FVector2D::Distance(FVector2D(E, N), FVector2D(SogE, SogN));
		C = FMath::Lerp(C, FLinearColor(0.34f, 0.50f, 0.20f), (1.f - Smooth01((DS - SogR) / 30.f)) * 0.6f);   // yam-yashil vodiy
	}
	{
		const float DD = FVector2D::Distance(FVector2D(E, N), FVector2D(DomE, DomN));
		const float T = 1.f - Smooth01((DD - DomR) / 50.f);
		C = FMath::Lerp(C, FLinearColor(0.40f, 0.58f, 0.22f), T * 0.7f);                                       // yam-yashil yaylov
		const float F = Noise(E, N, 0.9f);                                                                       // gul dog'lari
		if (T > 0.3f && F > 0.55f) C = FMath::Lerp(C, F > 0.75f ? FLinearColor(0.95f, 0.85f, 0.25f) : FLinearColor(0.85f, 0.35f, 0.55f), 0.5f * T);
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
	if (!bBuildTerrainMesh || FParse::Param(FCommandLine::Get(), TEXT("ErtNoTerrainMesh"))) return;
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
					{
						float RW = 0.f; const float RD = RoadDist(E, N, &RW);
						const bool bRoad = RD < RW * 0.5f + 0.5f && N > DesertN;
						C.Add(ErtCol::Sty(TerrainColor(E, N, H, 1.f - Nm.Z), bRoad ? 0.03f : ErtCol::StyleGround));   // 0.03 = yo'l (tosh yotqizma)
					}
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
	Disk(AskE, AskN, AskR + 6.f, AskZ);
	Disk(OasisE, OasisN, OasisR + 4.f, OasisZ);
	M.Commit(NewPart(TEXT("Water"), false, WaterMat), 0, false);
	// Uzoq dengiz: dunyo chekkasidan 30 km gacha suv halqasi - ufqda bo'shliq o'rniga dengiz ko'rinadi
	{
		FErtMeshData S(100.f);
		const float R0 = WorldSizeM * 0.5f - 2.f, R1 = 30000.f, Zs = WaterZ - 0.6f;
		const int32 Segs = 48;
		for (int32 i = 0; i < Segs; ++i)
		{
			const float A0 = 2.f * PI * i / Segs, A1 = 2.f * PI * (i + 1) / Segs;
			// Kvadrat dunyo chekkasi: ichki nuqtalar kvadrat perimetrida
			auto Edge = [&](float A) { const float c = FMath::Cos(A), sn = FMath::Sin(A); const float k = R0 / FMath::Max(FMath::Abs(c), FMath::Abs(sn)); return FVector2D(c * k, sn * k); };
			const FVector2D I0 = Edge(A0), I1 = Edge(A1);
			S.AddQuad(W(I0.X, I0.Y, Zs), W(FMath::Cos(A0) * R1, FMath::Sin(A0) * R1, Zs), W(FMath::Cos(A1) * R1, FMath::Sin(A1) * R1, Zs), W(I1.X, I1.Y, Zs), FVector::UpVector, Wc);
		}
		UProceduralMeshComponent* Sea = NewPart(TEXT("FarSea"), false, WaterMat);
		S.Commit(Sea, 0, false);
		Sea->SetBoundsScale(20.f);
	}
}

// ---------------- Elementlar ----------------

void AErtWorldBuilder::AddYurt(FErtMeshData& M, float E, float N, float Z, float R, float WallH, float RoofH, const FLinearColor& Wall, const FLinearColor& Roof, float DoorYaw, int32 S)
{
	if (R >= 2.3f) Interiors.Add(FVector4(E, N, R - 0.7f, Z));
	{
		// AssetHub/Fab o'tov meshi bo'lsa - protsedural o'rniga (qorong'i kigiz = mo'g'ul chodiri)
		FErtFabLib& Fab = FErtFabLib::Get();
		const bool bDark = Wall.GetLuminance() < 0.35f;
		UStaticMesh* Mesh = bDark ? FErtFabLib::Pick(Fab.Tents.Num() ? Fab.Tents : Fab.Yurts, S) : FErtFabLib::Pick(Fab.Yurts, S);
		if (Mesh) { FabPlace(Mesh, E, N, Z, DoorYaw, R, false, true); ++FabYurts; return; }
	}
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
	const FLinearColor Trunk = ErtCol::Vary(ErtCol::Sty(FLinearColor(0.30f, 0.20f, 0.11f), ErtCol::StyleBark), 0.15f, S);
	FRandomStream RS(S * 131 + 7);
	if (CurLeaf)
	{
		// Realistik daraxt: tanasi + shoxlar (kolliziya meshida), barg kartochkalari (M_ErtLeaf, kolliziyasiz)
		FErtMeshData& L = *CurLeaf;
		auto Card = [&](const FVector& P, const FVector& Out0, float Size, const FLinearColor& C, float A)
		{
			FVector Out = Out0; if (Out.IsNearlyZero()) Out = FVector::UpVector; Out.Normalize();
			const FVector Rt = FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
			const FVector Up = FVector::CrossProduct(Out, Rt).GetSafeNormal();
			const FQuat Tw(Out, RS.FRandRange(-1.f, 1.f));
			const FVector R2 = Tw.RotateVector(Rt) * Size, U2 = Tw.RotateVector(Up) * Size;
			L.AddQuadUV(P - R2 - U2, P + R2 - U2, P + R2 + U2, P - R2 + U2, Out, ErtCol::Sty(C, A));
		};
		if (bPine)
		{
			const FLinearColor Leaf = ErtCol::Vary(FLinearColor(0.07f, 0.20f, 0.08f), 0.18f, S + 7);
			const float TH = 8.5f * Sc;
			M.AddCylinder(W(E, N, Z - 0.3f), 0.30f * Sc, 0.08f * Sc, TH + 0.3f, 6, Trunk, false);
			// Qavatli igna bargli shoxlar: pastda keng, tepada tor, pastga osilgan kartochkalar
			for (int32 tier = 0; tier < 6; ++tier)
			{
				const float F = tier / 5.f;
				const float Zt = Z + TH * (0.28f + 0.68f * F), Rr = (2.7f - 2.0f * F) * Sc;
				const int32 Nc = tier < 4 ? 7 : 5;
				for (int32 c = 0; c < Nc; ++c)
				{
					const float A = 2.f * PI * (c + RS.FRand() * 0.5f) / Nc;
					const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0.f);
					const FVector P = W(E, N, Zt) + Dir * Rr * 55.f;
					Card(P, (Dir + FVector(0, 0, 0.55f)).GetSafeNormal(), (0.9f - 0.45f * F) * Sc * 100.f, ErtCol::Vary(Leaf, 0.1f, S + tier * 9 + c), 0.4f + 0.6f * F);
				}
			}
			Card(W(E, N, Z + TH * 0.97f), FVector(0.3f, 0.2f, 1.f), 0.6f * Sc * 100.f, Leaf * 1.1f, 1.f);
		}
		else
		{
			const FLinearColor Leaf = ErtCol::Vary(FLinearColor(0.17f, 0.36f, 0.10f), 0.2f, S + 7);
			const float TH = 4.0f * Sc;
			M.AddCylinder(W(E, N, Z - 0.3f), 0.42f * Sc, 0.24f * Sc, TH + 0.3f, 7, Trunk, false, FRotator::ZeroRotator, 0.04f, S);
			// Shoxlar: 4-6 ta yuqoriga egilgan silindr, uchlarida barg to'plamlari
			const int32 NB = RS.RandRange(4, 6);
			const FVector Top = W(E, N, Z + TH);
			for (int32 b = 0; b < NB; ++b)
			{
				const float A = 2.f * PI * (b + RS.FRand() * 0.4f) / NB, Tilt = RS.FRandRange(35.f, 60.f), BL = RS.FRandRange(2.0f, 3.2f) * Sc;
				const FVector Dir(FMath::Cos(A) * FMath::Sin(FMath::DegreesToRadians(Tilt)), FMath::Sin(A) * FMath::Sin(FMath::DegreesToRadians(Tilt)), FMath::Cos(FMath::DegreesToRadians(Tilt)));
				const FVector Base = Top - FVector(0, 0, RS.FRandRange(0.2f, 1.0f) * Sc * 100.f);
				M.AddCylinder(Base, 0.14f * Sc, 0.05f * Sc, BL, 5, Trunk * 0.95f, false, FRotator(-(90.f - Tilt), FMath::RadiansToDegrees(A), 0));
				const FVector Tip = Base + Dir * BL * 100.f;
				const int32 Nc = RS.RandRange(5, 7);
				for (int32 c = 0; c < Nc; ++c)
				{
					FVector Off(RS.FRandRange(-1.f, 1.f), RS.FRandRange(-1.f, 1.f), RS.FRandRange(-0.5f, 1.f)); Off.Normalize();
					const FVector P = Tip + Off * RS.FRandRange(0.3f, 1.3f) * Sc * 100.f;
					Card(P, (Off + Dir * 0.5f).GetSafeNormal(), RS.FRandRange(0.9f, 1.5f) * Sc * 100.f, ErtCol::Vary(Leaf * (0.85f + 0.3f * (Off.Z * 0.5f + 0.5f)), 0.12f, S + b * 11 + c), 0.5f + 0.5f * FMath::Clamp(Off.Z, 0.f, 1.f));
				}
			}
			// Markaziy to'plam (yoriqlarni yopadi)
			for (int32 c = 0; c < 8; ++c)
			{
				FVector Off(RS.FRandRange(-1.f, 1.f), RS.FRandRange(-1.f, 1.f), RS.FRandRange(0.f, 1.f)); Off.Normalize();
				Card(Top + FVector(0, 0, 1.2f * Sc * 100.f) + Off * RS.FRandRange(0.5f, 1.8f) * Sc * 100.f, Off, RS.FRandRange(1.0f, 1.6f) * Sc * 100.f, ErtCol::Vary(Leaf, 0.12f, S + 90 + c), 0.9f);
			}
		}
		return;
	}
	if (bPine)
	{
		const FLinearColor Leaf = ErtCol::Vary(ErtCol::Sty(FLinearColor(0.09f, 0.24f, 0.10f), ErtCol::StyleLeaf), 0.18f, S + 7);
		M.AddCylinder(W(E, N, Z - 0.3f), 0.32f * Sc, 0.22f * Sc, 4.2f * Sc, 5, Trunk, false);
		M.AddCone(W(E, N, Z + 2.2f * Sc), 2.6f * Sc, 3.4f * Sc, 7, Leaf);
		M.AddCone(W(E, N, Z + 4.4f * Sc), 2.1f * Sc, 3.0f * Sc, 7, Leaf * 1.08f);
		M.AddCone(W(E, N, Z + 6.4f * Sc), 1.5f * Sc, 2.6f * Sc, 6, Leaf * 1.15f);
	}
	else
	{
		const FLinearColor Leaf = ErtCol::Vary(ErtCol::Sty(FLinearColor(0.20f, 0.38f, 0.12f), ErtCol::StyleLeaf), 0.2f, S + 7);
		M.AddCylinder(W(E, N, Z - 0.3f), 0.4f * Sc, 0.28f * Sc, 3.2f * Sc, 6, Trunk, false);
		M.AddSphere(W(E, N, Z + 4.4f * Sc), 2.6f * Sc, 8, Leaf, FVector(1.f, 1.f, 0.85f), 0.12f, S);
		M.AddSphere(W(E + 1.2f * Sc, N + 0.6f * Sc, Z + 5.6f * Sc), 1.8f * Sc, 7, ErtCol::Sty(Leaf * 1.1f, ErtCol::StyleLeaf), FVector::OneVector, 0.15f, S + 3);
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
	M.AddCone(W(E, N, Z + 0.2f), 0.35f, 0.5f, 6, Ember);   // cho'g' (olov tili protsedural effekt bilan)
	FireSpots.Add(FVector4(W(E, N, Z + 0.25f), bLight ? 1.f : 0.75f));
	if (bLight)
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(this, *FString::Printf(TEXT("Fire_%d"), Lights.Num()));
		L->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Static);
		L->SetRelativeLocation(W(E, N, Z + 1.4f));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(550.f);
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
	Interiors.Add(FVector4(E, N, FMath::Min(HU, HV) - 0.8f, Z));
	{
		FErtFabLib& Fab = FErtFabLib::Get();
		if (UStaticMesh* Mesh = FErtFabLib::Pick(Fab.Houses, S)) { FabPlace(Mesh, E, N, Z, Yaw, FMath::Max(HU, HV) * 1.1f, false, true); ++FabHouses; return; }
	}
	const FRotator R(0, Yaw, 0);
	const FLinearColor Wall = ErtCol::Vary(C, 0.12f, S);
	M.AddBox(W(E, N, Z + H * 0.5f), FVector(HV, HU, H * 0.5f) * 100.f, Wall, R);
	M.AddBox(W(E, N, Z + H + 0.15f), FVector(HV + 0.25f, HU + 0.25f, 0.15f) * 100.f, ErtCol::Sty(Wall * 0.75f, ErtCol::StyleMudRoof), R);
	M.AddBox(W(E, N, Z + H + 0.32f), FVector(HV + 0.28f, HU + 0.28f, 0.06f) * 100.f, ErtCol::Sty(Wall * 0.8f, ErtCol::StyleMudRoof), R);   // tom chekkasi
	if (S % 3 == 0) M.AddSphere(W(E, N, Z + H + 0.2f), FMath::Min(HU, HV) * 0.8f, 8, ErtCol::Sty(Wall * 0.9f, ErtCol::StylePlain), FVector(1, 1, 0.6f));
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
				M.AddCone(W(OE(u), ON(v), Z - 0.2f + H), 0.26f, 0.4f, 5, ErtCol::Sty(Wood * 0.8f, ErtCol::StyleWood));
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
				AddYurt(M, OE(u), ON(v), Z, 2.5f, 1.8f, 1.7f, Felt, ErtCol::Sty(FLinearColor(0.72f, 0.66f, 0.55f), ErtCol::StyleFelt), DoorYaw, ++K);
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
	// Bagras qal'asi: tog' cho'qqisidagi salibchilar qal'asi. Blokli tosh devorlar (PBR uslub), mashikuli, kungura,
	// konus cherepitsa tomli dumaloq burjlar, arkli darvozaxona, panjara, ko'tarma ko'prik, donjon, cherkov, kazarma, otxona, temirchi.
	const float Z = FortZ;
	auto FE = [](float u) { return FortE + u; };
	auto FN = [](float v) { return FortN + v; };
	FErtMeshData M(100.f);
	const float Half = FortHalf, WallH = 12.f, Th = 2.2f;
	int32 S = 0;
	using namespace ErtCol;
	const FLinearColor StoneS = Sty(FLinearColor(0.58f, 0.55f, 0.50f), StyleStone), StoneD = Sty(FLinearColor(0.42f, 0.40f, 0.37f), StyleStone);
	const FLinearColor RoofT = Sty(FLinearColor(0.50f, 0.26f, 0.16f), StyleRoof), WoodP = Sty(FLinearColor(0.40f, 0.27f, 0.13f), StyleWood), WoodD = Sty(FLinearColor(0.26f, 0.17f, 0.08f), StyleWood);
	const FLinearColor Iron(0.25f, 0.26f, 0.28f), Slit(0.03f, 0.03f, 0.03f), Blue(0.1f, 0.15f, 0.45f), Straw2 = Sty(FLinearColor(0.80f, 0.70f, 0.40f), StylePlain), Slate = Sty(FLinearColor(0.30f, 0.32f, 0.36f), StyleRoof);
	auto Tower = [&](float u, float v, float R, float Hh, float RoofH, bool bBig)
	{
		M.AddCylinder(W(FE(u), FN(v), Z - 2.f), R * 1.12f, R, Hh + 2.f, 14, Vary(StoneS, 0.05f, ++S), false);          // glasisli tana
		for (float bz = 3.f; bz < Hh - 1.f; bz += 3.f) M.AddCylinder(W(FE(u), FN(v), Z + bz), R * 1.01f, R * 1.01f, 0.18f, 14, StoneD, false);   // qator choklari
		M.AddCylinder(W(FE(u), FN(v), Z + Hh - 0.6f), R * 1.22f, R * 1.22f, 0.9f, 14, Vary(StoneD, 0.04f, ++S), true); // mashikuli tokchasi
		for (int32 i = 0; i < 14; ++i) { const float A = i * 2.f * PI / 14; M.AddBox(W(FE(u + FMath::Cos(A) * R * 1.12f), FN(v + FMath::Sin(A) * R * 1.12f), Z + Hh - 1.4f), FVector(22, 22, 70), StoneD, FRotator(0, FMath::RadiansToDegrees(A), 0)); }   // konsollar
		M.AddCylinder(W(FE(u), FN(v), Z + Hh + 0.3f), R * 1.15f, R * 1.15f, 1.3f, 14, Vary(StoneS, 0.05f, ++S), false); // parapet
		for (int32 i = 0; i < 14; i += 2) { const float A = (i + 0.5f) * 2.f * PI / 14; M.AddBox(W(FE(u + FMath::Cos(A) * R * 1.1f), FN(v + FMath::Sin(A) * R * 1.1f), Z + Hh + 2.f), FVector(28, 45, 55), Vary(StoneS, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0)); }   // kungura tishlari
		M.AddCone(W(FE(u), FN(v), Z + Hh + 1.6f), R * 1.2f, RoofH, 14, Vary(RoofT, 0.06f, ++S));
		M.AddSphere(W(FE(u), FN(v), Z + Hh + 1.6f + RoofH), 0.25f, 6, Iron);
		for (int32 i = 0; i < (bBig ? 8 : 4); ++i) { const float A = i * 2.f * PI / (bBig ? 8 : 4) + 0.4f; M.AddBox(W(FE(u + FMath::Cos(A) * R * 1.0f), FN(v + FMath::Sin(A) * R * 1.0f), Z + Hh * 0.55f), FVector(12, 14, 110), Slit, FRotator(0, FMath::RadiansToDegrees(A), 0)); }   // o'q tirqishlari
	};
	auto WallRun = [&](float u0, float v0, float u1, float v1, bool bGate)
	{
		const float Len = FVector2D::Distance(FVector2D(u0, v0), FVector2D(u1, v1));
		const int32 Blocks = FMath::CeilToInt(Len / 4.f);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(u1 - u0, v1 - v0));
		const float cu0 = (u0 + u1) * 0.5f, cv0 = (v0 + v1) * 0.5f;
		const float InU = (cu0 != 0) ? -FMath::Sign(cu0) : 0.f, InV = (cv0 != 0) ? -FMath::Sign(cv0) : 0.f;   // ichkariga yo'nalish
		for (int32 i = 0; i < Blocks; ++i)
		{
			const float t0 = (float)i / Blocks, t1 = (float)(i + 1) / Blocks;
			const float cu = FMath::Lerp(u0, u1, (t0 + t1) * 0.5f), cv = FMath::Lerp(v0, v1, (t0 + t1) * 0.5f);
			const float BL = Len * (t1 - t0) * 0.5f;
			if (bGate && FMath::Abs(cu) < 3.5f) continue;   // darvozaxona alohida
			M.AddBox(W(FE(cu), FN(cv), Z + WallH * 0.5f), FVector(BL, Th, WallH * 0.5f) * 100.f, Vary(StoneS, 0.06f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(FE(cu), FN(cv), Z + 1.2f), FVector(BL, Th * 1.25f, 1.2f) * 100.f, Vary(StoneD, 0.05f, ++S), FRotator(0, Yaw, 0));   // poydevor qalinlashuvi
			// Mashikuli tokchasi (tashqi tomonda) va parapet, kungura tishlari
			M.AddBox(W(FE(cu - InU * (Th * 0.5f + 0.4f)), FN(cv - InV * (Th * 0.5f + 0.4f)), Z + WallH - 0.3f), FVector(BL, 0.5f, 0.35f) * 100.f, Vary(StoneD, 0.05f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(FE(cu - InU * (Th * 0.5f + 0.35f)), FN(cv - InV * (Th * 0.5f + 0.35f)), Z + WallH + 0.7f), FVector(BL, 0.35f, 0.7f) * 100.f, Vary(StoneS, 0.06f, ++S), FRotator(0, Yaw, 0));
			const int32 Merlons = FMath::Max(1, FMath::RoundToInt(BL * 2.f / 1.6f));
			for (int32 k = 0; k < Merlons; ++k)
			{
				const float t = (k + 0.5f) / Merlons - 0.5f;
				const float du = (u1 - u0) / Len * t * BL * 2.f, dv = (v1 - v0) / Len * t * BL * 2.f;
				M.AddBox(W(FE(cu + du - InU * (Th * 0.5f + 0.35f)), FN(cv + dv - InV * (Th * 0.5f + 0.35f)), Z + WallH + 1.85f), FVector(0.45f, 0.35f, 0.5f) * 100.f, Vary(StoneS, 0.06f, ++S), FRotator(0, Yaw, 0));
				if (k % 2 == 0) M.AddBox(W(FE(cu + du - InU * (Th * 0.5f + 0.42f)), FN(cv + dv - InV * (Th * 0.5f + 0.42f)), Z + WallH * 0.6f), FVector(0.06f, 0.06f, 0.9f) * 100.f, Slit, FRotator(0, Yaw, 0));   // o'q tirqishi
			}
		}
		// Yo'lak (devor ichki tomonidagi yog'och supa) va tayanch ustunlar
		M.AddBox(W(FE(cu0 + InU * (Th * 0.5f + 0.8f)), FN(cv0 + InV * (Th * 0.5f + 0.8f)), Z + WallH - 1.6f), FVector(Len * 0.5f - 2.f, 0.85f, 0.12f) * 100.f, WoodP, FRotator(0, Yaw, 0));
		for (float t = -Len * 0.5f + 3.f; t < Len * 0.5f - 1.f; t += 4.f)
		{
			const float du = (u1 - u0) / Len * t, dv = (v1 - v0) / Len * t;
			M.AddBox(W(FE(cu0 + du + InU * (Th * 0.5f + 1.2f)), FN(cv0 + dv + InV * (Th * 0.5f + 1.2f)), Z + WallH * 0.5f - 0.8f), FVector(0.12f, 0.12f, WallH * 0.5f - 0.8f) * 100.f, WoodD, FRotator(0, Yaw, 0));
			M.AddBox(W(FE(cu0 + du + InU * (Th * 0.5f + 0.8f)), FN(cv0 + dv + InV * (Th * 0.5f + 0.8f)), Z + WallH - 1.05f), FVector(0.08f, 0.8f, 0.5f) * 100.f, WoodD, FRotator(0, Yaw, 0));   // panjara
		}
	};
	WallRun(-Half, -Half, Half, -Half, true);
	WallRun(-Half, Half, Half, Half, false);
	WallRun(-Half, -Half, -Half, Half, false);
	WallRun(Half, -Half, Half, Half, false);
	for (int32 i = 0; i < 4; ++i) Tower((i & 1) ? Half : -Half, (i & 2) ? Half : -Half, 5.2f, 17.f, 5.5f, true);
	Tower(0.f, Half, 3.6f, 15.f, 4.f, false);
	Tower(-Half, 0.f, 3.6f, 15.f, 4.f, false); Tower(Half, 0.f, 3.6f, 15.f, 4.f, false);
	// Darvozaxona: ikki dumaloq minora, ark, panjara, ko'tarma ko'prik, xandaq
	Tower(-6.f, -Half, 3.8f, 15.f, 4.5f, false); Tower(6.f, -Half, 3.8f, 15.f, 4.5f, false);
	M.AddBox(W(FE(0), FN(-Half), Z + 11.f), FVector(300, 260, 300), Vary(StoneS, 0.05f, ++S));                                 // ark usti
	M.AddCylinder(W(FE(0), FN(-Half + 1.4f), Z + 5.f), 3.f, 3.f, 2.8f, 12, Slit, true, FRotator(90.f, 0, 0));                 // ark (qorong'i yo'lak)
	for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(FE(k * 3.1f), FN(-Half), Z + 4.f), FVector(40, 260, 400), Vary(StoneD, 0.05f, ++S));   // ark tayanchlari
	for (int32 i = 0; i < 6; ++i) M.AddBox(W(FE(-2.5f + i * 1.f), FN(-Half - 1.35f), Z + 3.4f), FVector(5, 5, 340), Iron);        // panjara vertikal
	for (int32 i = 0; i < 5; ++i) M.AddBox(W(FE(0), FN(-Half - 1.35f), Z + 0.9f + i * 1.3f), FVector(300, 5, 5), Iron);           // panjara gorizontal
	M.AddBox(W(FE(0), FN(-Half - 5.5f), Z + 0.12f), FVector(300, 480, 10), Vary(WoodP, 0.06f, ++S));                            // ko'tarma ko'prik (tushirilgan)
	for (int32 i = 0; i < 7; ++i) M.AddBox(W(FE(0), FN(-Half - 1.8f - i * 1.3f), Z + 0.2f), FVector(300, 6, 6), WoodD);
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(FE(k * 1.7f), FN(-Half - 6.f), Z + 5.f), FVector(3, 800, 3), Iron, FRotator(-52.f, 0, 0)); }   // zanjirlar
	M.AddBox(W(FE(0), FN(-Half - 5.5f), Z - 1.6f), FVector(900, 480, 120), Sty(FLinearColor(0.30f, 0.28f, 0.22f), StyleGround));   // xandaq tubi
	for (int32 k = -1; k <= 1; k += 2) AddBanner(M, FE(k * 6.f), FN(-Half + 1.f), Z + 15.f + 1.6f + 4.5f, 3.5f, Blue, false);
	AddFire(M, FE(-3.6f), FN(-Half + 3.5f), Z, true); AddFire(M, FE(3.6f), FN(-Half + 3.5f), Z, true);
	// Donjon (qasr): kvadrat, burchak ustunlari, uch qavat derazalar, mashikuli, kungura, bayroq, ustki shiypon tom
	{
		const float DU = 0.f, DV = 8.f, DH = 26.f, HW = 6.f;
		M.AddBox(W(FE(DU), FN(DV), Z + DH * 0.5f), FVector(HW * 100.f, HW * 100.f, DH * 50.f), Vary(StoneS, 0.04f, ++S));
		M.AddBox(W(FE(DU), FN(DV), Z + 1.5f), FVector(HW * 100.f + 40.f, HW * 100.f + 40.f, 150), Vary(StoneD, 0.04f, ++S));   // poydevor
		for (int32 i = 0; i < 4; ++i)
		{
			const float u = DU + ((i & 1) ? HW : -HW), v = DV + ((i & 2) ? HW : -HW);
			M.AddBox(W(FE(u), FN(v), Z + DH * 0.5f + 1.f), FVector(110, 110, DH * 50.f + 100.f), Vary(StoneD, 0.04f, ++S));       // burchak ustunlari
			M.AddCone(W(FE(u), FN(v), Z + DH + 2.f), 1.2f, 2.2f, 8, Slate);
		}
		for (int32 f = 0; f < 3; ++f) for (int32 side = 0; side < 4; ++side)
		{
			const float zz = Z + 6.f + f * 7.f;
			const float su = (side == 0) ? -HW : (side == 1) ? HW : 0.f, sv = (side == 2) ? -HW : (side == 3) ? HW : 0.f;
			for (int32 k = -1; k <= 1; k += 2)
			{
				const float ou = (side < 2) ? 0.f : k * 1.8f, ov = (side < 2) ? k * 1.8f : 0.f;
				M.AddBox(W(FE(DU + su + ou), FN(DV + sv + ov), zz), FVector((side < 2) ? 14.f : 60.f, (side < 2) ? 60.f : 14.f, f == 2 ? 140.f : 90.f), Slit);   // deraza
				M.AddBox(W(FE(DU + su + ou), FN(DV + sv + ov), zz + (f == 2 ? 0.8f : 0.55f)), FVector((side < 2) ? 20.f : 80.f, (side < 2) ? 80.f : 20.f, 12.f), Vary(StoneD, 0.05f, ++S));   // deraza peshtoqi
			}
		}
		M.AddBox(W(FE(DU), FN(DV), Z + DH - 0.5f), FVector(HW * 100.f + 70.f, HW * 100.f + 70.f, 45.f), Vary(StoneD, 0.04f, ++S));   // mashikuli
		for (int32 i = 0; i < 12; ++i)
		{
			const float t = -HW + 0.5f + i * (2.f * HW - 1.f) / 11.f;
			M.AddBox(W(FE(DU + t), FN(DV - HW - 0.5f), Z + DH + 0.6f), FVector(40, 30, 60), Vary(StoneS, 0.05f, ++S)); M.AddBox(W(FE(DU + t), FN(DV + HW + 0.5f), Z + DH + 0.6f), FVector(40, 30, 60), Vary(StoneS, 0.05f, ++S));
			M.AddBox(W(FE(DU - HW - 0.5f), FN(DV + t), Z + DH + 0.6f), FVector(30, 40, 60), Vary(StoneS, 0.05f, ++S)); M.AddBox(W(FE(DU + HW + 0.5f), FN(DV + t), Z + DH + 0.6f), FVector(30, 40, 60), Vary(StoneS, 0.05f, ++S));
		}
		M.AddCone(W(FE(DU), FN(DV), Z + DH + 0.2f), HW * 0.75f, 4.5f, 4, Vary(Slate, 0.05f, ++S), FRotator(0, 45.f, 0));   // shiypon tom
		AddBanner(M, FE(DU), FN(DV), Z + DH + 4.6f, 4.f, Blue, false);
		M.AddBox(W(FE(DU), FN(DV - HW - 0.1f), Z + 2.2f), FVector(160, 20, 240), WoodD);   // katta eshik
		M.AddCylinder(W(FE(DU), FN(DV - HW - 0.15f), Z + 3.4f), 0.8f, 0.8f, 0.3f, 10, WoodD, true, FRotator(90.f, 0, 0));
		for (int32 i = 0; i < 6; ++i) M.AddBox(W(FE(DU - 3.f + i * 1.2f), FN(DV - HW - 1.2f - i * 0.0f), Z + 0.15f + i * 0.0f), FVector(50, 90, 12), Vary(StoneD, 0.05f, ++S));   // zinapoya toshlari
	}
	// Cherkov (g'arb): tosh nef, yarim dumaloq apsida, qiya tom, xoch; kazarma (sharq, cherepitsa tom); otxona (yog'och); temirchi; quduq; bochkalar; oshxona tutuni
	M.AddBox(W(FE(-20.f), FN(12.f), Z + 3.f), FVector(500, 1000, 300), Vary(StoneS, 0.05f, ++S));
	M.AddCylinder(W(FE(-20.f), FN(22.f), Z), 2.5f, 2.5f, 5.f, 10, Vary(StoneS, 0.05f, ++S), true);
	M.AddCone(W(FE(-20.f), FN(22.f), Z + 5.f), 2.7f, 1.6f, 10, Vary(RoofT, 0.06f, ++S));
	M.AddCone(W(FE(-20.f), FN(12.f), Z + 5.8f), 4.6f, 2.8f, 4, Vary(RoofT, 0.06f, ++S), FRotator(0, 45.f, 0));
	M.AddBox(W(FE(-20.f), FN(4.f), Z + 9.2f), FVector(8, 8, 90), FLinearColor(0.9f, 0.75f, 0.25f)); M.AddBox(W(FE(-20.f), FN(4.f), Z + 9.6f), FVector(8, 40, 8), FLinearColor(0.9f, 0.75f, 0.25f));
	for (int32 i = 0; i < 3; ++i) for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(FE(-20.f + k * 2.55f), FN(8.f + i * 3.5f), Z + 3.6f), FVector(10, 30, 100), Slit);   // uzun derazalar
	M.AddBox(W(FE(21.f), FN(14.f), Z + 2.5f), FVector(450, 900, 250), Vary(StoneS, 0.06f, ++S));                                    // kazarma
	M.AddCone(W(FE(21.f), FN(14.f), Z + 5.f), 5.6f, 2.6f, 4, Vary(RoofT, 0.06f, ++S), FRotator(0, 45.f, 0));
	for (int32 i = 0; i < 4; ++i) M.AddBox(W(FE(16.4f), FN(8.f + i * 4.f), Z + 2.8f), FVector(10, 50, 70), Slit);
	M.AddBox(W(FE(21.f), FN(-12.f), Z + 1.8f), FVector(400, 700, 180), Vary(WoodP, 0.08f, ++S));                                   // otxona
	M.AddCone(W(FE(21.f), FN(-12.f), Z + 3.6f), 5.f, 2.2f, 4, Straw2, FRotator(0, 45.f, 0));
	for (int32 i = 0; i < 3; ++i) M.AddBox(W(FE(16.9f), FN(-17.f + i * 5.f), Z + 1.2f), FVector(6, 140, 120), WoodD);
	AddHorse(M, FE(24.f), FN(-20.f), Z, 90.f, FLinearColor(0.36f, 0.22f, 0.11f));
	M.AddBox(W(FE(-20.f), FN(-14.f), Z + 1.6f), FVector(300, 350, 160), Vary(StoneD, 0.06f, ++S));                                  // temirchi
	M.AddCone(W(FE(-20.f), FN(-14.f), Z + 3.2f), 4.f, 1.8f, 4, Slate, FRotator(0, 45.f, 0));
	M.AddBox(W(FE(-20.f), FN(-14.f), Z + 4.6f), FVector(50, 50, 120), Vary(StoneD, 0.05f, ++S));                                    // mo'ri
	M.AddBox(W(FE(-16.f), FN(-14.f), Z + 0.7f), FVector(45, 25, 70), Iron);                                                         // sandon
	AddFire(M, FE(-17.5f), FN(-11.f), Z, true);
	M.AddCylinder(W(FE(12.f), FN(-6.f), Z), 1.2f, 1.2f, 1.f, 12, Vary(StoneS, 0.05f, ++S), false);                                   // quduq
	for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(FE(12.f + k * 1.3f), FN(-6.f), Z + 1.3f), FVector(10, 10, 260), WoodD);
	M.AddCone(W(FE(12.f), FN(-6.f), Z + 2.6f), 1.8f, 1.f, 8, Vary(RoofT, 0.06f, ++S));
	for (int32 i = 0; i < 7; ++i) { const float u = -12.f + (i % 4) * 1.1f, v = -24.f + (i / 4) * 1.1f; M.AddCylinder(W(FE(u), FN(v), Z), 0.42f, 0.42f, 0.95f, 8, Vary(WoodP, 0.08f, ++S), true); M.AddCylinder(W(FE(u), FN(v), Z + 0.45f), 0.44f, 0.44f, 0.06f, 8, Iron, true); }   // bochkalar
	for (int32 i = 0; i < 3; ++i) M.AddBox(W(FE(6.f + i * 1.5f), FN(-24.f), Z + 0.5f), FVector(60, 60, 50), Vary(WoodP, 0.1f, ++S));   // yashiklar
	for (int32 i = 0; i < 6; ++i) { const float A = i * PI / 3; M.AddBox(W(FE(-6.f), FN(-24.f), Z + 1.6f), FVector(6, 6, 160), WoodD, FRotator(0, 0, 0)); M.AddBox(W(FE(-6.f + FMath::Cos(A) * 0.5f), FN(-24.f + FMath::Sin(A) * 0.5f), Z + 3.2f), FVector(10, 10, 12), Iron); }   // qurol ustuni
	for (int32 i = 0; i < 4; ++i) { const float u = -Half + 3.f + i * 20.f; M.AddBox(W(FE(u), FN(Half - 2.6f), Z + 1.8f), FVector(6, 6, 180), WoodD); M.AddSphere(W(FE(u), FN(Half - 2.6f), Z + 3.7f), 0.25f, 6, FLinearColor(1.0f, 0.55f, 0.1f)); }   // mash'alalar
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
				M.AddBox(W(CE(u), CN(v), Z + 8.f), FVector(Len, 1.6f, 1.5f) * 100.f, ErtCol::Vary(ErtCol::Sty(Ochre * 0.9f, ErtCol::StyleStone), 0.08f, ++S), FRotator(0, TanYaw, 0));
				continue;
			}
			M.AddBox(W(CE(u), CN(v), Z + 4.5f), FVector(Len, 1.6f, 4.5f) * 100.f, ErtCol::Vary(ErtCol::Sty(Ochre * 0.9f, ErtCol::StyleStone), 0.08f, ++S), FRotator(0, TanYaw, 0));
			M.AddBox(W(CE(u), CN(v), Z + 9.4f), FVector(Len * 0.5f, 0.8f, 0.5f) * 100.f, ErtCol::Vary(ErtCol::Sty(Ochre * 0.85f, ErtCol::StyleStone), 0.08f, ++S), FRotator(0, TanYaw, 0));
			if (i % 10 == 0)
			{
				M.AddCylinder(W(CE(u), CN(v), Z - 0.5f), 4.2f, 3.8f, 13.f, 10, ErtCol::Vary(ErtCol::Sty(Ochre * 0.85f, ErtCol::StyleStone), 0.06f, ++S), true);
				M.AddCone(W(CE(u), CN(v), Z + 12.5f), 4.3f, 3.5f, 10, FLinearColor(0.35f, 0.18f, 0.12f));
			}
		}
		for (int32 g = 0; g < 4; ++g)
		{
			const float GA = g * HALF_PI;
			for (int32 s = -1; s <= 1; s += 2)
			{
				const float A = GA + s * (9.f / CityR);
				M.AddBox(W(CE(FMath::Cos(A) * CityR), CN(FMath::Sin(A) * CityR), Z + 6.f), FVector(280, 280, 650), ErtCol::Vary(ErtCol::Sty(Ochre * 0.9f, ErtCol::StyleStone), 0.06f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0));
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
		// Ko'chalar va markaziy maydon: tosh yotqizma (cobble uslubi), yer ustida 6 sm
		{
			const FLinearColor Cob = ErtCol::Sty(FLinearColor(0.58f, 0.54f, 0.48f), ErtCol::StyleCobble);
			M.AddBox(W(CE(0), CN(0), Z + 0.03f), FVector(4200, 4200, 6), Cob);                                  // maydon
			for (int32 g = 0; g < 4; ++g)
			{
				const float A = g * HALF_PI, L = CityR - 42.f + 14.f;
				const float mu = FMath::Cos(A) * (42.f + L * 0.5f), mv = FMath::Sin(A) * (42.f + L * 0.5f);
				M.AddBox(W(CE(mu), CN(mv), Z + 0.03f), FVector(L * 50.f, 800, 6), ErtCol::Vary(Cob, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0));   // 4 ko'cha
			}
			for (int32 i = 0; i < 40; ++i)                                                                            // uy oralaridagi tor ko'chalar
			{
				const float A = i * 2.f * PI / 40 + 0.08f, R0 = 60.f, R1 = CityR - 30.f;
				if (FMath::Abs(FMath::Sin(A)) < 0.12f || FMath::Abs(FMath::Cos(A)) < 0.12f) continue;
				M.AddBox(W(CE(FMath::Cos(A) * (R0 + R1) * 0.5f), CN(FMath::Sin(A) * (R0 + R1) * 0.5f), Z + 0.02f), FVector((R1 - R0) * 50.f, 130, 4), ErtCol::Vary(Cob * 0.95f, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0));
			}
			for (int32 i = 0; i < 6; ++i) M.AddCylinder(W(CE(-30.f + i * 12.f), CN(-36.f), Z + 0.06f), 0.5f, 0.5f, 0.9f, 8, ErtCol::Sty(FLinearColor(0.55f, 0.5f, 0.45f), ErtCol::StyleStone), true);   // maydon chetidagi tosh ustunchalar
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
	AddYurt(M, KE(0), KN(0), Z, 8.f, 3.f, 4.2f, ErtCol::Sty(FLinearColor(0.38f, 0.10f, 0.09f), ErtCol::StyleFelt), ErtCol::Sty(FLinearColor(0.22f, 0.08f, 0.07f), ErtCol::StyleFelt), -90.f, ++K);
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
			AddYurt(M, KE(u), KN(v), Z, 3.f, 1.7f, 1.5f, FeltDark, ErtCol::Sty(FeltDark * 0.85f, ErtCol::StyleFelt), FMath::RadiansToDegrees(FMath::Atan2(-v, -u)), ++K);
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
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN)) < AskR + 12.f) return false;     // Askaniya ko'li
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(NikE, NikN)) < NikR + 30.f) return false;     // Nikeya
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(KarE, KarN)) < KarR + 30.f) return false;     // Karacahisar
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(SogE, SogN)) < SogR + 20.f) return false;     // So'g'ut
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(DomE, DomN)) < DomR) return false;                // Domaniç (ochiq o'tloq)
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(OasisE, OasisN)) < OasisR + 30.f) return false; // voha
	if (FMath::Max(FMath::Abs(E - CaravanE), FMath::Abs(N - CaravanN)) < 40.f) return false;          // karvonsaroy
	if (FMath::Abs(E - DamE) < DamHalfE + 30.f && FMath::Abs(N - DamN) < DamHalfN + 30.f) return false;  // Damashq
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(HalabE, HalabN)) < HalabR + 30.f) return false;   // Halab
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(KonE, KonN)) < KonR + 30.f) return false;   // Konya
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(KayE, KayN)) < KayR + 30.f) return false;   // Qayseri
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(SivE, SivN)) < SivR + 30.f) return false;   // Sivas
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(ErzE, ErzN)) < ErzR + 30.f) return false;   // Erzurum
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(BurE, BurN)) < BurR + 30.f) return false;   // Bursa
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(ErcE, ErcN)) < ErcR * 0.8f) return false;   // Erciyes
	float Wd; if (RoadDist(E, N, &Wd) < Wd + 3.f) return false;
	return true;
}

void AErtWorldBuilder::BuildForest()
{
	FRandomStream RS(Seed + 3);
	const float Half = WorldSizeM * 0.5f;
	const int32 CellsPerSide = 4;
	TArray<FErtMeshData> Cells; Cells.Init(FErtMeshData(100.f), CellsPerSide * CellsPerSide);
	TArray<FErtMeshData> LeafCells; LeafCells.Init(FErtMeshData(1.f), CellsPerSide * CellsPerSide);
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
		{
			FErtFabLib& Fab = FErtFabLib::Get();
			if (Fab.Stumps.Num() && RS.FRand() < 0.04f)
			{
				UStaticMesh* Mesh = Fab.Stumps[RS.RandRange(0, Fab.Stumps.Num() - 1)];
				FabComp(Mesh, true)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, H - 0.05f), FVector(FErtFabLib::ScaleToRadius(Mesh, RS.FRandRange(0.9f, 1.6f)))), true);
				continue;
			}
			const TArray<UStaticMesh*>& Pool = (bPine && Fab.Pines.Num()) ? Fab.Pines : (Fab.Trees.Num() ? Fab.Trees : Fab.Pines);
			if (Pool.Num())
			{
				UStaticMesh* Mesh = Pool[RS.RandRange(0, Pool.Num() - 1)];
				const float Sc = FErtFabLib::ScaleToHeight(Mesh, RS.FRandRange(0.8f, 1.5f) * (bPine ? 11.f : 8.f));
				FabComp(Mesh, true)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, H - 0.05f), FVector(Sc)), true);
				++Placed; ++FabTreesPlaced;
				continue;
			}
		}
		const int32 cx = FMath::Clamp((int32)((E + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		const int32 cy = FMath::Clamp((int32)((N + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		CurLeaf = LeafMat ? &LeafCells[cy * CellsPerSide + cx] : nullptr;
		AddTree(Cells[cy * CellsPerSide + cx], E, N, H, RS.FRandRange(0.8f, 1.5f), bPine, Placed);
		CurLeaf = nullptr;
		++Placed;
	}
	for (int32 c = 0; c < Cells.Num(); ++c)
	{
		if (Cells[c].Verts.Num()) Cells[c].Commit(NewPart(FString::Printf(TEXT("Forest_%d"), c), true), 0, true);
		if (LeafCells[c].Verts.Num()) { UProceduralMeshComponent* P = NewPart(FString::Printf(TEXT("ForestLeaves_%d"), c), false, LeafMat); LeafCells[c].Commit(P, 0, false); }
	}
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
		{
			FErtFabLib& Fab = FErtFabLib::Get();
			if (Fab.Rocks.Num())
			{
				UStaticMesh* Mesh = Fab.Rocks[RS.RandRange(0, Fab.Rocks.Num() - 1)];
				const float Sc = FErtFabLib::ScaleToRadius(Mesh, R);
				FabComp(Mesh, true)->AddInstance(FTransform(FRotator(RS.FRandRange(-8.f, 8.f), RS.FRandRange(0.f, 360.f), RS.FRandRange(-8.f, 8.f)), W(E, N, H - R * 0.15f), FVector(Sc)), true);
				++Placed; ++FabRocksPlaced;
				continue;
			}
		}
		M.AddSphere(W(E, N, H - R * 0.35f), R, 8, ErtCol::Vary(ErtCol::Sty(H > 150.f ? FLinearColor(0.7f, 0.7f, 0.72f) : FLinearColor(0.43f, 0.41f, 0.38f), ErtCol::StyleRock), 0.12f, Placed), FVector(RS.FRandRange(0.7f, 1.4f), RS.FRandRange(0.7f, 1.4f), RS.FRandRange(0.5f, 0.9f)), 0.18f, Placed);
		++Placed;
	}
	if (M.Verts.Num()) M.Commit(NewPart(TEXT("Rocks"), true), 0, true);
}

// ---------------- Cho'l: voha, palmalar, karvonsaroy, xarobalar ----------------

bool AErtWorldBuilder::IsWater(float E, float N, float& SurfZ) const
{
	if (FMath::Abs(E - RiverE(N)) < 25.f) { SurfZ = WaterZ; return true; }
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(LakeE, LakeN)) < LakeR + 4.f) { SurfZ = LakeZ; return true; }
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(AskE, AskN)) < AskR + 4.f) { SurfZ = AskZ; return true; }
	if (FVector2D::Distance(FVector2D(E, N), FVector2D(OasisE, OasisN)) < OasisR + 2.f) { SurfZ = OasisZ; return true; }
	SurfZ = 0.f;
	return false;
}

void AErtWorldBuilder::AddPalm(FErtMeshData& M, float E, float N, float Z, float H, int32 S)
{
	FRandomStream RS(S);
	const float LeanA = RS.FRandRange(0.f, 2.f * PI), Lean = RS.FRandRange(0.6f, 1.8f);
	const FLinearColor Trunk = ErtCol::Sty(FLinearColor(0.42f, 0.32f, 0.18f), ErtCol::StyleBark), Leaf = ErtCol::Sty(FLinearColor(0.16f, 0.40f, 0.14f), ErtCol::StyleLeaf);
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
	const FLinearColor Lime = ErtCol::Sty(FLinearColor(0.86f, 0.82f, 0.70f), ErtCol::StyleStone), Basalt = ErtCol::Sty(FLinearColor(0.30f, 0.30f, 0.32f), ErtCol::StyleStone), DWood = ErtCol::Sty(FLinearColor(0.42f, 0.28f, 0.14f), ErtCol::StyleWood), DGold(0.9f, 0.75f, 0.25f), Teal(0.18f, 0.5f, 0.55f), Green(0.16f, 0.40f, 0.14f);
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
	const FLinearColor HStone = ErtCol::Sty(FLinearColor(0.72f, 0.68f, 0.60f), ErtCol::StyleStone), HDark = ErtCol::Sty(FLinearColor(0.45f, 0.42f, 0.38f), ErtCol::StyleStone), HWood = ErtCol::Sty(FLinearColor(0.40f, 0.27f, 0.13f), ErtCol::StyleWood), Copper(0.35f, 0.55f, 0.45f), HRed(0.6f, 0.12f, 0.1f);
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
		M.AddBox(W(HE(0.f), HN(-CR + 2.f), TopZ + 4.f), FVector(250, 400, 100), ErtCol::Sty(HStone * 0.3f, ErtCol::StylePlain));
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
	const FLinearColor KStone = ErtCol::Sty(FLinearColor(0.76f, 0.70f, 0.58f), ErtCol::StyleStone), KDark = ErtCol::Sty(FLinearColor(0.50f, 0.45f, 0.38f), ErtCol::StyleStone), KWood = ErtCol::Sty(FLinearColor(0.38f, 0.25f, 0.12f), ErtCol::StyleWood), Turq(0.12f, 0.58f, 0.62f), Tile = ErtCol::Sty(FLinearColor(0.16f, 0.32f, 0.62f), ErtCol::StyleRoof), KGold(0.9f, 0.75f, 0.25f), Plane = ErtCol::Sty(FLinearColor(0.22f, 0.45f, 0.18f), ErtCol::StyleLeaf), Trunk = ErtCol::Sty(FLinearColor(0.55f, 0.50f, 0.42f), ErtCol::StyleBark);
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
		M.AddBox(W(KE(MU), KN(MV - 14.6f), Z + 3.5f), FVector(200, 40, 350), ErtCol::Sty(KStone * 0.3f, ErtCol::StylePlain));
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

// ---------------- Qayseri: bazalt shahar, Erciyes etagida ----------------

void AErtWorldBuilder::BuildKayseri()
{
	FRandomStream RS(Seed + 131);
	FErtMeshData M(100.f);
	int32 S = 4500;
	const float Z = KayZ;
	auto QE = [](float u) { return KayE + u; };
	auto QN = [](float v) { return KayN + v; };
	const FLinearColor Basalt = ErtCol::Sty(FLinearColor(0.26f, 0.25f, 0.26f), ErtCol::StyleStone), BasaltL = ErtCol::Sty(FLinearColor(0.40f, 0.38f, 0.37f), ErtCol::StyleStone), Mortar(0.62f, 0.58f, 0.52f), QWood = ErtCol::Sty(FLinearColor(0.38f, 0.25f, 0.12f), ErtCol::StyleWood), Lead(0.45f, 0.47f, 0.52f), Turq(0.12f, 0.58f, 0.62f), QRed(0.55f, 0.10f, 0.08f), Poplar = ErtCol::Sty(FLinearColor(0.30f, 0.50f, 0.20f), ErtCol::StyleLeaf);
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
		M.AddBox(W(QE(CU), QN(CV - HD), Z + 5.f), FVector(300, 300, 80), ErtCol::Sty(Basalt * 1.4f, ErtCol::StyleStone));   // darvoza yo'lagi usti
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
		M.AddBox(W(QE(u), QN(-6.6f), Z + 1.5f), FVector(150, 20, 220), ErtCol::Sty(Basalt * 0.5f, ErtCol::StylePlain));   // eshiklar
		M.AddBox(W(QE(u), QN(6.6f), Z + 1.5f), FVector(150, 20, 220), ErtCol::Sty(Basalt * 0.5f, ErtCol::StylePlain));
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

// ---------------- Sivas: madrasalar shahri ----------------

void AErtWorldBuilder::BuildSivas()
{
	FRandomStream RS(Seed + 151);
	FErtMeshData M(100.f);
	int32 S = 5500;
	const float Z = SivZ, TopZ = SivZ + SivHillH;
	auto SE = [](float u) { return SivE + u; };
	auto SN = [](float v) { return SivN + v; };
	const FLinearColor Brick = ErtCol::Sty(FLinearColor(0.62f, 0.40f, 0.28f), ErtCol::StyleBrick), BrickD = ErtCol::Sty(FLinearColor(0.48f, 0.30f, 0.20f), ErtCol::StyleBrick), SStone = ErtCol::Sty(FLinearColor(0.74f, 0.68f, 0.56f), ErtCol::StyleStone), Turq(0.12f, 0.58f, 0.62f), Cobalt(0.15f, 0.25f, 0.6f), SWood = ErtCol::Sty(FLinearColor(0.38f, 0.25f, 0.12f), ErtCol::StyleWood), Lead(0.45f, 0.47f, 0.52f), SGreen(0.10f, 0.35f, 0.15f), Willow(0.35f, 0.55f, 0.25f);
	// Minora yasovchi: g'isht tanasi, feruza halqalar, sharafa, konus
	auto Minaret = [&](float u, float v, float Zb, float Hh, float R0, int32 Bands)
	{
		M.AddBox(W(SE(u), SN(v), Zb + 2.5f), FVector(R0 * 130.f, R0 * 130.f, 250), SStone);
		M.AddCylinder(W(SE(u), SN(v), Zb + 5.f), R0, R0 * 0.8f, Hh, 12, ErtCol::Vary(Brick, 0.04f, ++S), true);
		for (int32 b = 1; b <= Bands; ++b) M.AddCylinder(W(SE(u), SN(v), Zb + 5.f + Hh * b / (Bands + 1)), R0 * 1.02f, R0 * 1.02f, 0.6f, 12, Turq, true);
		M.AddCylinder(W(SE(u), SN(v), Zb + 5.f + Hh), R0 * 1.35f, R0 * 1.35f, 1.f, 12, ErtCol::Vary(BrickD, 0.04f, ++S), true);
		M.AddCylinder(W(SE(u), SN(v), Zb + 6.f + Hh), R0 * 0.8f, R0 * 0.8f, 2.5f, 12, Brick, true);
		M.AddCylinder(W(SE(u), SN(v), Zb + 8.5f + Hh), R0 * 1.1f, 0.2f, 3.5f, 8, Lead, true);
	};
	// Tashqi devor: 14 burchak, tosh, burjlar; janubiy va sharqiy darvozalar
	const int32 Sides = 14;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * SivR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * SivR, cv = FMath::Sin(Am) * SivR;
		const bool bGate = (FMath::Abs(FMath::Sin(Am) + 1.f) < 0.25f) || (FMath::Abs(FMath::Cos(Am) - 1.f) < 0.12f);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) M.AddBox(W(SE(cu), SN(cv), Z + 7.f), FVector(Len * 50.f, 120, 80), SStone, FRotator(0, Yaw, 0));
		else
		{
			M.AddBox(W(SE(cu), SN(cv), Z + 3.5f), FVector(Len * 50.f + 20.f, 120, 350), ErtCol::Vary(SStone, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
				M.AddBox(W(SE(cu - FMath::Sin(Am) * t), SN(cv + FMath::Cos(Am) * t), Z + 7.6f), FVector(60, 120, 60), ErtCol::Vary(SStone * 0.92f, 0.05f, ++S), FRotator(0, Yaw, 0));
		}
		const float tu = FMath::Cos(A0) * SivR, tv = FMath::Sin(A0) * SivR;
		M.AddCylinder(W(SE(tu), SN(tv), Z), 3.8f, 3.5f, 10.f, 10, ErtCol::Vary(SStone, 0.04f, ++S), true);
		M.AddCylinder(W(SE(tu), SN(tv), Z + 10.f), 4.1f, 4.1f, 1.f, 10, SStone * 0.9f, true);
	}
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(SE(k * 9.f), SN(-SivR), Z + 6.f), FVector(300, 380, 600), ErtCol::Vary(SStone, 0.04f, ++S)); M.AddBox(W(SE(SivR), SN(k * 9.f), Z + 6.f), FVector(380, 300, 600), ErtCol::Vary(SStone, 0.04f, ++S)); }
	// Qal'a tepaligi (shimol): past devor halqasi, burj, saroy
	{
		const float HV = 70.f, CR = SivHillR - 8.f;
		for (int32 i = 0; i < 12; ++i)
		{
			if (i == 9) continue;   // janubiy kirish
			const float Am = (i + 0.5f) * 2.f * PI / 12, Len = 2.f * CR * FMath::Sin(PI / 12);
			M.AddBox(W(SE(FMath::Cos(Am) * CR), SN(HV + FMath::Sin(Am) * CR), TopZ + 2.f), FVector(Len * 50.f + 20.f, 100, 250), ErtCol::Vary(SStone * 0.85f, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
		}
		M.AddCylinder(W(SE(0.f), SN(HV + CR), TopZ), 4.f, 3.6f, 12.f, 10, SStone * 0.85f, true);
		M.AddBox(W(SE(-6.f), SN(HV + 4.f), TopZ + 3.5f), FVector(1100, 800, 350), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(-6.f), SN(HV + 4.f), TopZ + 7.3f), FVector(1100, 800, 30), SWood);
		M.AddSphere(W(SE(-6.f), SN(HV + 4.f), TopZ + 7.5f), 3.f, 10, Lead, FVector(1, 1, 0.7f));
		AddBanner(M, SE(-5.f), SN(HV - CR + 5.f), TopZ, 7.f, SGreen, false);
		AddBanner(M, SE(5.f), SN(HV - CR + 5.f), TopZ, 7.f, SGreen, false);
		AddFire(M, SE(8.f), SN(HV - 4.f), TopZ, true);
	}
	// Go'k Madrasa (markaz-g'arb): hovli, feruza koshinli portal, qo'sh minora
	{
		const float MU = -55.f, MV = 10.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 3.5f), FVector(1700, 1500, 350), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU), SN(MV), Z + 0.2f), FVector(900, 800, 20), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(SE(MU + 17.f), SN(MV), Z + 6.f), FVector(150, 600, 600), ErtCol::Vary(SStone, 0.03f, ++S));   // portal fasadi
		M.AddBox(W(SE(MU + 17.8f), SN(MV), Z + 6.f), FVector(20, 420, 500), Turq);                                  // koshin
		M.AddBox(W(SE(MU + 18.2f), SN(MV), Z + 3.f), FVector(20, 200, 400), ErtCol::Sty(SStone * 0.3f, ErtCol::StylePlain));                           // eshik
		Minaret(MU + 17.f, MV - 8.f, Z + 9.f, 14.f, 1.1f, 3); Minaret(MU + 17.f, MV + 8.f, Z + 9.f, 14.f, 1.1f, 3);
		M.AddSphere(W(SE(MU - 8.f), SN(MV + 9.f), Z + 7.f), 3.5f, 10, Turq, FVector(1, 1, 0.7f));
		M.AddSphere(W(SE(MU - 8.f), SN(MV - 9.f), Z + 7.f), 3.5f, 10, Turq, FVector(1, 1, 0.7f));
	}
	// Chifte Minorali madrasa (markaz-sharq): faqat baland fasad va qo'sh minora, orqada hovli
	{
		const float MU = 45.f, MV = 20.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 7.f), FVector(200, 1100, 700), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU - 1.1f), SN(MV), Z + 5.f), FVector(20, 450, 700), ErtCol::Vary(BrickD, 0.03f, ++S));
		M.AddBox(W(SE(MU - 1.3f), SN(MV), Z + 3.f), FVector(20, 220, 450), ErtCol::Sty(SStone * 0.3f, ErtCol::StylePlain));
		Minaret(MU, MV - 9.f, Z + 14.f, 16.f, 1.2f, 4); Minaret(MU, MV + 9.f, Z + 14.f, 16.f, 1.2f, 4);
		M.AddBox(W(SE(MU + 12.f), SN(MV), Z + 2.5f), FVector(1000, 1100, 250), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU + 12.f), SN(MV), Z + 0.2f), FVector(600, 700, 20), FLinearColor(0.9f, 0.88f, 0.82f));
	}
	// Burujiya madrasasi (janubi-sharq): hovli, gumbazli maqbara
	{
		const float MU = 50.f, MV = -50.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 3.f), FVector(1300, 1200, 300), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU), SN(MV), Z + 0.2f), FVector(700, 600, 20), FLinearColor(0.9f, 0.88f, 0.82f));
		M.AddBox(W(SE(MU), SN(MV - 12.f), Z + 5.5f), FVector(500, 120, 550), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddSphere(W(SE(MU - 9.f), SN(MV + 8.f), Z + 6.f), 3.2f, 10, Turq, FVector(1, 1, 0.7f));
	}
	// Ulu Jome' (janubi-g'arb): keng past ibodatxona, ko'p ustunli, qiyshaygan g'isht minora
	{
		const float MU = -50.f, MV = -55.f;
		M.AddBox(W(SE(MU), SN(MV), Z + 3.f), FVector(2200, 1500, 300), ErtCol::Vary(SStone, 0.03f, ++S));
		M.AddBox(W(SE(MU), SN(MV), Z + 6.3f), FVector(2200, 1500, 30), SWood);
		for (int32 i = 0; i < 4; ++i) M.AddSphere(W(SE(MU - 15.f + i * 10.f), SN(MV), Z + 6.f), 2.f, 8, ErtCol::Vary(SStone, 0.05f, ++S), FVector(1, 1, 0.5f));
		M.AddBox(W(SE(MU + 24.f), SN(MV + 10.f), Z + 1.f), FVector(220, 220, 100), SStone);
		M.AddCylinder(W(SE(MU + 24.f), SN(MV + 10.f), Z + 2.f), 1.4f, 1.1f, 26.f, 12, ErtCol::Vary(Brick, 0.04f, ++S), true, FRotator(2.5f, 0, 1.5f));
		M.AddCylinder(W(SE(MU + 25.1f), SN(MV + 10.7f), Z + 28.f), 1.8f, 1.8f, 0.8f, 12, BrickD, true);
		M.AddCylinder(W(SE(MU + 25.2f), SN(MV + 10.8f), Z + 28.8f), 1.2f, 0.2f, 3.f, 8, Lead, true);
	}
	// Bozor: janubiy darvozadan markazgacha yog'och soyabonli rastalar
	for (float v = -SivR + 16.f; v < -20.f; v += 7.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(SE(k * 8.f), SN(v), Z + 3.f), FVector(300, 330, 15), ErtCol::Vary(SWood, 0.1f, ++S), FRotator(0, 0, k * 8.f));
			M.AddCylinder(W(SE(k * 10.5f), SN(v - 3.f), Z), 0.25f, 0.25f, 3.2f, 6, SWood, true); M.AddCylinder(W(SE(k * 10.5f), SN(v + 3.f), Z), 0.25f, 0.25f, 3.2f, 6, SWood, true);
			M.AddBox(W(SE(k * 8.f), SN(v), Z + 0.8f), FVector(70, 220, 15), SWood);
			M.AddBox(W(SE(k * 8.f), SN(v), Z + 1.1f), FVector(40, 160, 20), FLinearColor(RS.FRand() * 0.5f + 0.2f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.5f + 0.2f));
		}
	// Uylar: g'isht-tosh, sopol tomli
	for (float R = 28.f; R < SivR - 12.f; R += 15.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 14.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(u) < 13.f && v < 0.f) continue;                                             // bozor
			if (FMath::Abs(v) < 8.f) continue;                                                        // sharq-g'arb ko'cha
			if (FVector2D::Distance(FVector2D(u, v), FVector2D(0.f, 70.f)) < SivHillR + 2.f) continue; // tepalik
			if (FMath::Abs(u + 55.f) < 22.f && FMath::Abs(v - 10.f) < 20.f) continue;
			if (FMath::Abs(u - 50.f) < 20.f && FMath::Abs(v - 20.f) < 16.f) continue;
			if (FMath::Abs(u - 50.f) < 17.f && FMath::Abs(v + 50.f) < 16.f) continue;
			if (FMath::Abs(u + 50.f) < 26.f && FMath::Abs(v + 55.f) < 18.f) continue;
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.f, 6.f), HD = RS.FRandRange(4.f, 6.f), HH = RS.FRand() < 0.4f ? 6.5f : 4.f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(SE(u), SN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(RS.FRand() < 0.5f ? Brick : SStone, 0.08f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(SE(u), SN(v), Z + HH + 0.4f), FVector(HW * 50.f + 40.f, HD * 50.f + 40.f, 40.f), ErtCol::Vary(FLinearColor(0.55f, 0.30f, 0.18f), 0.06f, ++S), FRotator(0, Yaw, 0));
		}
	}
	// Tollar (daryo bo'yi shahri) va darvoza bayroqlari
	for (int32 i = 0; i < 16; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(25.f, SivR - 10.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(u) < 14.f && v < 0.f) continue;
		if (FVector2D::Distance(FVector2D(u, v), FVector2D(0.f, 70.f)) < SivHillR + 2.f) continue;
		const float ZZ = HeightAt(SE(u), SN(v));
		M.AddCylinder(W(SE(u), SN(v), ZZ), 0.4f, 0.3f, 3.f, 6, SWood, true);
		M.AddSphere(W(SE(u), SN(v), ZZ + 4.5f), 3.f, 8, ErtCol::Vary(Willow, 0.08f, ++S), FVector(1, 1, 1.1f));
	}
	AddBanner(M, SE(-9.f), SN(-SivR - 3.f), Z, 6.f, SGreen, false);
	AddBanner(M, SE(9.f), SN(-SivR - 3.f), Z, 6.f, SGreen, false);
	M.Commit(NewPart(TEXT("Sivas"), true), 0, true);
}

// ---------------- Erzurum: baland tekislik qal'a shahri ----------------

void AErtWorldBuilder::BuildErzurum()
{
	FRandomStream RS(Seed + 173);
	FErtMeshData M(100.f);
	int32 S = 6500;
	const float Z = ErzZ, TopZ = ErzZ + ErzHillH;
	auto XE = [](float u) { return ErzE + u; };
	auto XN = [](float v) { return ErzN + v; };
	const FLinearColor Grey = ErtCol::Sty(FLinearColor(0.58f, 0.56f, 0.52f), ErtCol::StyleStone), GreyD = ErtCol::Sty(FLinearColor(0.42f, 0.40f, 0.38f), ErtCol::StyleStone), Brick = ErtCol::Sty(FLinearColor(0.60f, 0.38f, 0.26f), ErtCol::StyleBrick), BrickD = ErtCol::Sty(FLinearColor(0.46f, 0.28f, 0.18f), ErtCol::StyleBrick), Turq(0.12f, 0.58f, 0.62f), XWood = ErtCol::Sty(FLinearColor(0.36f, 0.24f, 0.12f), ErtCol::StyleWood), Lead(0.45f, 0.47f, 0.52f), XRed(0.55f, 0.10f, 0.08f), Pine = ErtCol::Sty(FLinearColor(0.10f, 0.32f, 0.16f), ErtCol::StyleLeaf), Snow(0.95f, 0.96f, 1.0f);
	auto Minaret = [&](float u, float v, float Zb, float Hh, float R0)
	{
		M.AddBox(W(XE(u), XN(v), Zb + 2.5f), FVector(R0 * 130.f, R0 * 130.f, 250), Grey);
		M.AddCylinder(W(XE(u), XN(v), Zb + 5.f), R0, R0 * 0.85f, Hh, 12, ErtCol::Vary(Brick, 0.04f, ++S), true);
		for (int32 b = 1; b <= 3; ++b) M.AddCylinder(W(XE(u), XN(v), Zb + 5.f + Hh * b / 4.f), R0 * 1.03f, R0 * 1.03f, 0.5f, 12, Turq, true);
		M.AddCylinder(W(XE(u), XN(v), Zb + 5.f + Hh), R0 * 1.35f, R0 * 1.35f, 1.f, 12, BrickD, true);
		M.AddCylinder(W(XE(u), XN(v), Zb + 6.f + Hh), R0 * 0.85f, R0 * 0.85f, 2.5f, 12, Brick, true);
		M.AddCylinder(W(XE(u), XN(v), Zb + 8.5f + Hh), R0 * 1.1f, 0.2f, 3.5f, 8, Lead, true);
	};
	// Tashqi devor: 12 burchak, qalin kulrang tosh, katta kvadrat burjlar; janubiy va g'arbiy darvozalar
	const int32 Sides = 12;
	for (int32 i = 0; i < Sides; ++i)
	{
		const float A0 = i * 2.f * PI / Sides, A1 = (i + 1) * 2.f * PI / Sides, Am = (A0 + A1) * 0.5f;
		const float Len = 2.f * ErzR * FMath::Sin(PI / Sides);
		const float cu = FMath::Cos(Am) * ErzR, cv = FMath::Sin(Am) * ErzR;
		const bool bGate = (FMath::Abs(FMath::Sin(Am) + 1.f) < 0.3f) || (FMath::Abs(FMath::Cos(Am) + 1.f) < 0.15f);
		const float Yaw = FMath::RadiansToDegrees(Am) + 90.f;
		if (bGate) M.AddBox(W(XE(cu), XN(cv), Z + 8.f), FVector(Len * 50.f, 160, 100), Grey, FRotator(0, Yaw, 0));
		else
		{
			M.AddBox(W(XE(cu), XN(cv), Z + 4.5f), FVector(Len * 50.f + 20.f, 160, 450), ErtCol::Vary(Grey, 0.05f, ++S), FRotator(0, Yaw, 0));
			for (float t = -Len * 0.5f + 1.f; t < Len * 0.5f; t += 2.5f)
				M.AddBox(W(XE(cu - FMath::Sin(Am) * t), XN(cv + FMath::Cos(Am) * t), Z + 9.6f), FVector(60, 160, 60), ErtCol::Vary(GreyD, 0.05f, ++S), FRotator(0, Yaw, 0));
		}
		const float tu = FMath::Cos(A0) * ErzR, tv = FMath::Sin(A0) * ErzR;
		M.AddBox(W(XE(tu), XN(tv), Z + 6.f), FVector(400, 400, 600), ErtCol::Vary(Grey, 0.04f, ++S), FRotator(0, FMath::RadiansToDegrees(A0), 0));
		M.AddBox(W(XE(tu), XN(tv), Z + 12.4f), FVector(440, 440, 40), GreyD, FRotator(0, FMath::RadiansToDegrees(A0), 0));
		M.AddCylinder(W(XE(tu), XN(tv), Z + 12.6f), 2.6f, 0.2f, 2.5f, 4, Lead, true, FRotator(0, FMath::RadiansToDegrees(A0), 0));   // qorga qarshi qiya tom
	}
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(XE(k * 10.f), XN(-ErzR), Z + 7.f), FVector(350, 420, 700), ErtCol::Vary(Grey, 0.04f, ++S)); M.AddBox(W(XE(-ErzR), XN(k * 10.f), Z + 7.f), FVector(420, 350, 700), ErtCol::Vary(Grey, 0.04f, ++S)); }
	// Qal'a (shimoli-sharq tepaligi): ichki devor, Tepsi minora (dumaloq g'isht kuzatuv minorasi), saroy
	{
		const float HU = 45.f, HV = 45.f, CR = ErzHillR - 8.f;
		for (int32 i = 0; i < 10; ++i)
		{
			if (i == 6) continue;   // janubi-g'arbiy kirish
			const float Am = (i + 0.5f) * 2.f * PI / 10, Len = 2.f * CR * FMath::Sin(PI / 10);
			M.AddBox(W(XE(HU + FMath::Cos(Am) * CR), XN(HV + FMath::Sin(Am) * CR), TopZ + 2.5f), FVector(Len * 50.f + 20.f, 120, 300), ErtCol::Vary(GreyD, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(Am) + 90.f, 0));
		}
		M.AddCylinder(W(XE(HU + 8.f), XN(HV + 8.f), TopZ), 3.2f, 2.6f, 22.f, 12, ErtCol::Vary(Brick, 0.04f, ++S), true);   // Tepsi minora
		M.AddCylinder(W(XE(HU + 8.f), XN(HV + 8.f), TopZ + 22.f), 3.4f, 3.4f, 1.2f, 12, BrickD, true);
		M.AddBox(W(XE(HU + 8.f), XN(HV + 8.f), TopZ + 25.f), FVector(200, 200, 180), XWood);                                    // soat xonasi
		M.AddCylinder(W(XE(HU + 8.f), XN(HV + 8.f), TopZ + 26.8f), 2.6f, 0.2f, 2.5f, 4, Lead, true);
		M.AddBox(W(XE(HU - 8.f), XN(HV - 4.f), TopZ + 3.5f), FVector(1000, 700, 350), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddCylinder(W(XE(HU - 8.f), XN(HV - 4.f), TopZ + 7.f), 8.f, 0.3f, 3.5f, 4, Lead, true);
		AddBanner(M, XE(HU - 22.f), XN(HV - 18.f), TopZ, 7.f, XRed, false);
		AddFire(M, XE(HU), XN(HV - 12.f), TopZ, true);
	}
	// Chifte Minorali madrasa (markaz): juda baland portal, qo'sh minora, orqada hovli va kumbet
	{
		const float MU = 0.f, MV = 8.f;
		M.AddBox(W(XE(MU), XN(MV - 16.f), Z + 9.f), FVector(1100, 250, 900), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV - 17.3f), Z + 6.5f), FVector(400, 20, 900), ErtCol::Vary(GreyD, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV - 17.5f), Z + 3.f), FVector(200, 20, 450), ErtCol::Sty(Grey * 0.3f, ErtCol::StylePlain));
		Minaret(MU - 8.f, MV - 16.f, Z + 18.f, 14.f, 1.3f); Minaret(MU + 8.f, MV - 16.f, Z + 18.f, 14.f, 1.3f);
		M.AddBox(W(XE(MU), XN(MV + 4.f), Z + 3.5f), FVector(1100, 1800, 350), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV + 2.f), Z + 0.2f), FVector(500, 1000, 20), FLinearColor(0.85f, 0.85f, 0.82f));
		M.AddCylinder(W(XE(MU), XN(MV + 24.f), Z), 4.5f, 4.5f, 9.f, 12, ErtCol::Vary(Grey, 0.04f, ++S), true);                  // orqadagi katta kumbet
		M.AddCylinder(W(XE(MU), XN(MV + 24.f), Z + 9.f), 5.f, 0.2f, 6.f, 12, GreyD, true);
	}
	// Uch Kumbet (janubi-sharq): uch konus tomli maqbara
	for (int32 i = 0; i < 3; ++i)
	{
		const float u = 40.f + i * 11.f, v = -55.f + (i == 1 ? 8.f : 0.f), R0 = i == 0 ? 3.8f : 3.f;
		M.AddCylinder(W(XE(u), XN(v), Z), R0, R0, 6.f, 8, ErtCol::Vary(Grey, 0.05f, ++S), true);
		M.AddCylinder(W(XE(u), XN(v), Z + 6.f), R0 * 1.1f, 0.2f, 5.f, 8, i == 1 ? Brick : GreyD, true);
	}
	// Ulu Jome' (g'arb): keng past tosh bino, ko'p gumbazli tom, kalta minora
	{
		const float MU = -55.f, MV = -10.f;
		M.AddBox(W(XE(MU), XN(MV), Z + 3.f), FVector(1500, 1800, 300), ErtCol::Vary(Grey, 0.03f, ++S));
		M.AddBox(W(XE(MU), XN(MV), Z + 6.3f), FVector(1500, 1800, 30), GreyD);
		for (int32 i = 0; i < 3; ++i) for (int32 j = 0; j < 3; ++j) M.AddSphere(W(XE(MU - 8.f + i * 8.f), XN(MV - 10.f + j * 10.f), Z + 6.f), 2.6f, 8, ErtCol::Vary(Grey, 0.05f, ++S), FVector(1, 1, 0.55f));
		M.AddCylinder(W(XE(MU + 16.f), XN(MV + 19.f), Z), 1.4f, 1.2f, 14.f, 10, ErtCol::Vary(Brick, 0.04f, ++S), true);
		M.AddCylinder(W(XE(MU + 16.f), XN(MV + 19.f), Z + 14.f), 1.5f, 0.2f, 3.f, 8, Lead, true);
	}
	// Bozor (janubiy darvozadan madrasa oldigacha): tosh do'konlar, tunuka tomlar
	for (float v = -ErzR + 16.f; v < -30.f; v += 7.f)
		for (int32 k = -1; k <= 1; k += 2)
		{
			M.AddBox(W(XE(k * 9.f), XN(v), Z + 1.6f), FVector(250, 320, 160), ErtCol::Vary(Grey, 0.06f, ++S));
			M.AddCylinder(W(XE(k * 9.f), XN(v), Z + 3.2f), 2.4f, 0.2f, 1.6f, 4, Lead, true);
			M.AddBox(W(XE(k * 6.5f), XN(v), Z + 0.9f), FVector(30, 200, 90), ErtCol::Sty(Grey * 0.3f, ErtCol::StylePlain));
			M.AddBox(W(XE(k * 6.f), XN(v), Z + 0.8f), FVector(40, 160, 20), FLinearColor(RS.FRand() * 0.5f + 0.2f, RS.FRand() * 0.4f + 0.2f, RS.FRand() * 0.5f + 0.2f));
		}
	// Uylar: past, qalin tosh, tekis tuproq tom (qish uchun), mo'rilar
	for (float R = 30.f; R < ErzR - 12.f; R += 14.f)
	{
		const int32 Cnt = FMath::RoundToInt(2.f * PI * R / 13.f);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = (i + RS.FRandRange(-0.2f, 0.2f)) * 2.f * PI / Cnt;
			const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
			if (FMath::Abs(u) < 14.f && v < -20.f) continue;                                             // bozor
			if (FMath::Abs(v) < 8.f && u < 0.f) continue;                                                // g'arbiy ko'cha
			if (FMath::Abs(u) < 10.f && FMath::Abs(v - 8.f) < 28.f) continue;                            // madrasa
			if (FVector2D::Distance(FVector2D(u, v), FVector2D(45.f, 45.f)) < ErzHillR + 2.f) continue;   // qal'a
			if (FMath::Abs(u - 51.f) < 18.f && FMath::Abs(v + 52.f) < 10.f) continue;                    // kumbetlar
			if (FMath::Abs(u + 55.f) < 12.f && FMath::Abs(v + 10.f) < 14.f) continue;                    // jome'
			if (RS.FRand() < 0.15f) continue;
			const float HW = RS.FRandRange(4.5f, 6.5f), HD = RS.FRandRange(4.5f, 6.5f), HH = RS.FRand() < 0.3f ? 5.5f : 3.5f;
			const float Yaw = FMath::RadiansToDegrees(A);
			M.AddBox(W(XE(u), XN(v), Z + HH * 0.5f), FVector(HW * 50.f, HD * 50.f, HH * 50.f), ErtCol::Vary(Grey, 0.10f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(XE(u), XN(v), Z + HH + 0.2f), FVector(HW * 50.f + 10.f, HD * 50.f + 10.f, 20.f), ErtCol::Vary(FLinearColor(0.45f, 0.38f, 0.28f), 0.06f, ++S), FRotator(0, Yaw, 0));
			if (RS.FRand() < 0.5f) M.AddBox(W(XE(u), XN(v), Z + HH + 0.9f), FVector(50, 50, 80), GreyD);
			if (RS.FRand() < 0.4f) M.AddBox(W(XE(u), XN(v), Z + HH + 0.45f), FVector(HW * 50.f + 12.f, HD * 50.f + 12.f, 8.f), Snow, FRotator(0, Yaw, 0));   // tomdagi qor
		}
	}
	// Qarag'aylar va darvoza bayroqlari
	for (int32 i = 0; i < 14; ++i)
	{
		const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(25.f, ErzR - 10.f);
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		if (FMath::Abs(u) < 15.f && v < -20.f) continue;
		if (FVector2D::Distance(FVector2D(u, v), FVector2D(45.f, 45.f)) < ErzHillR + 2.f) continue;
		if (FMath::Abs(u) < 10.f && FMath::Abs(v - 8.f) < 28.f) continue;
		const float ZZ = HeightAt(XE(u), XN(v));
		M.AddCylinder(W(XE(u), XN(v), ZZ), 0.35f, 0.25f, 2.5f, 6, XWood, true);
		M.AddCylinder(W(XE(u), XN(v), ZZ + 2.f), 2.4f, 0.1f, RS.FRandRange(7.f, 10.f), 6, ErtCol::Vary(Pine, 0.08f, ++S), true);
	}
	AddBanner(M, XE(-10.f), XN(-ErzR - 3.f), Z, 6.f, XRed, false);
	AddBanner(M, XE(10.f), XN(-ErzR - 3.f), Z, 6.f, XRed, false);
	M.Commit(NewPart(TEXT("Erzurum"), true), 0, true);
}

// ---------------- Bursa: Uludog' etagidagi yashil shahar ----------------

void AErtWorldBuilder::BuildBursa()
{
	FRandomStream RS(Seed + 197);
	FErtMeshData M(100.f);
	int32 S = 7500;
	const float Z = BurZ, TopZ = BurZ + BurHillH;
	auto BE = [](float u) { return BurE + u; };
	auto BN = [](float v) { return BurN + v; };
	const FLinearColor BStone = ErtCol::Sty(FLinearColor(0.80f, 0.74f, 0.62f), ErtCol::StyleStone), BStoneD = ErtCol::Sty(FLinearColor(0.62f, 0.56f, 0.46f), ErtCol::StyleStone), Lead(0.45f, 0.47f, 0.52f), Turq(0.12f, 0.58f, 0.62f), TurqD(0.08f, 0.42f, 0.48f), Tile = ErtCol::Sty(FLinearColor(0.62f, 0.30f, 0.18f), ErtCol::StyleRoof), BWood = ErtCol::Sty(FLinearColor(0.38f, 0.25f, 0.12f), ErtCol::StyleWood), White(0.92f, 0.90f, 0.85f), Plane = ErtCol::Sty(FLinearColor(0.22f, 0.45f, 0.18f), ErtCol::StyleLeaf), Trunk = ErtCol::Sty(FLinearColor(0.55f, 0.50f, 0.42f), ErtCol::StyleBark), BRed(0.55f, 0.10f, 0.08f), Steam(0.85f, 0.88f, 0.9f);
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
		M.AddBox(W(BE(MU), BN(MV - 26.1f), Z + 2.f), FVector(200, 20, 350), ErtCol::Sty(BStone * 0.3f, ErtCol::StylePlain));
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
		M.AddBox(W(BE(MU - 6.7f), BN(MV), Z + 2.5f), FVector(20, 200, 400), ErtCol::Sty(BStone * 0.3f, ErtCol::StylePlain));
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
		M.AddBox(W(BE(MU), BN(MV + 8.f), Z + 2.f), FVector(200, 40, 350), ErtCol::Sty(BStone * 0.3f, ErtCol::StylePlain));   // darvoza
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
		M.AddCylinder(W(BE(u), BN(v), ZZ), 0.25f, 0.2f, 1.8f, 5, ErtCol::Sty(Trunk * 0.8f, ErtCol::StyleBark), true);
		M.AddSphere(W(BE(u), BN(v), ZZ + 2.8f), 2.f, 6, ErtCol::Vary(ErtCol::Sty(FLinearColor(0.30f, 0.52f, 0.18f), ErtCol::StyleLeaf), 0.1f, ++S), FVector(1, 1, 0.8f));
	}
	AddBanner(M, BE(BurR + 3.f), BN(-9.f), Z, 6.f, BRed, false);
	AddBanner(M, BE(BurR + 3.f), BN(9.f), Z, 6.f, BRed, false);
	M.Commit(NewPart(TEXT("Bursa"), true), 0, true);
}

// ---------------- Nikeya: Vizantiya shahri, Askaniya ko'li bo'yida ----------------

void AErtWorldBuilder::BuildNikeya()
{
	FRandomStream RS(Seed + 211);
	FErtMeshData M(100.f);
	int32 S = 8500;
	const float Z = NikZ;
	auto NE = [](float u) { return NikE + u; };
	auto NN = [](float v) { return NikN + v; };
	const FLinearColor NStone = ErtCol::Sty(FLinearColor(0.70f, 0.66f, 0.58f), ErtCol::StyleStone), NStoneD = ErtCol::Sty(FLinearColor(0.55f, 0.52f, 0.46f), ErtCol::StyleStone), Brick = ErtCol::Sty(FLinearColor(0.60f, 0.32f, 0.22f), ErtCol::StyleBrick), Marble(0.90f, 0.88f, 0.84f), Lead(0.45f, 0.47f, 0.52f), Tile = ErtCol::Sty(FLinearColor(0.62f, 0.32f, 0.20f), ErtCol::StyleRoof), NWood = ErtCol::Sty(FLinearColor(0.38f, 0.25f, 0.12f), ErtCol::StyleWood), Purple(0.35f, 0.10f, 0.35f), GoldC(0.9f, 0.75f, 0.25f), Cyp = ErtCol::Sty(FLinearColor(0.10f, 0.30f, 0.14f), ErtCol::StyleLeaf);
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
		M.AddCylinder(W(NE(gu), NN(gv), Z + 6.f), 5.f, 5.f, 3.f, 12, ErtCol::Sty(NStone * 0.35f, ErtCol::StylePlain), true, FRotator(90.f, Yaw, 0));   // ark (qorong'i yo'lak)
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
		M.AddBox(W(NE(BU), NN(BV - 22.7f), Z + 2.f), FVector(20, 200, 350), ErtCol::Sty(NStone * 0.3f, ErtCol::StylePlain));
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

// ---------------- Karacahisar: qora qoya ustidagi Vizantiya qal'asi ----------------

void AErtWorldBuilder::BuildKaracahisar()
{
	FRandomStream RS(Seed + 233);
	FErtMeshData M(100.f);
	int32 S = 9500;
	const float TopZ = KarBaseZ + KarH, Z0 = KarBaseZ;
	auto KE = [](float u) { return KarE + u; };
	auto KN = [](float v) { return KarN + v; };
	const FLinearColor KStone = ErtCol::Sty(FLinearColor(0.40f, 0.39f, 0.40f), ErtCol::StyleStone), KStoneL = ErtCol::Sty(FLinearColor(0.52f, 0.50f, 0.48f), ErtCol::StyleStone), Brick = ErtCol::Sty(FLinearColor(0.55f, 0.30f, 0.22f), ErtCol::StyleBrick), KWood = ErtCol::Sty(FLinearColor(0.36f, 0.24f, 0.12f), ErtCol::StyleWood), Lead(0.45f, 0.47f, 0.52f), Purple(0.35f, 0.10f, 0.35f), GoldC(0.9f, 0.75f, 0.25f), Iron(0.3f, 0.3f, 0.32f), Thatch(0.62f, 0.52f, 0.30f);
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
	for (int32 i = 0; i < 3; ++i) M.AddBox(W(KE(-4.f - 7.1f), KN(12.f - 3.f + i * 3.f), TopZ + 6.f + i * 3.f), FVector(10, 40, 90), ErtCol::Sty(KStone * 0.2f, ErtCol::StylePlain));   // tirqish derazalar
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
		const float StartV = -KarR - 6.f;
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

// ---------------- So'g'ut: tollar vodiysidagi qishloq ----------------

void AErtWorldBuilder::BuildSogut()
{
	FRandomStream RS(Seed + 251);
	FErtMeshData M(100.f);
	int32 S = 10500;
	const float Z = SogZ;
	auto GE = [](float u) { return SogE + u; };
	auto GN = [](float v) { return SogN + v; };
	const FLinearColor Timber = ErtCol::Sty(FLinearColor(0.42f, 0.28f, 0.14f), ErtCol::StyleWood), TimberD = ErtCol::Sty(FLinearColor(0.30f, 0.20f, 0.10f), ErtCol::StyleWood), Plaster(0.88f, 0.84f, 0.72f), Shingle = ErtCol::Sty(FLinearColor(0.36f, 0.26f, 0.16f), ErtCol::StyleRoof), WillowL = ErtCol::Sty(FLinearColor(0.42f, 0.60f, 0.28f), ErtCol::StyleLeaf), WillowT = ErtCol::Sty(FLinearColor(0.35f, 0.28f, 0.18f), ErtCol::StyleBark), Water(0.28f, 0.48f, 0.62f), StoneG = ErtCol::Sty(FLinearColor(0.62f, 0.60f, 0.55f), ErtCol::StyleStone), Lead(0.45f, 0.47f, 0.52f), GreenF(0.16f, 0.45f, 0.18f), Fruit = ErtCol::Sty(FLinearColor(0.30f, 0.52f, 0.20f), ErtCol::StyleLeaf), Wheat(0.80f, 0.68f, 0.35f), Hay(0.75f, 0.62f, 0.32f);
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

// ---------------- Domaniç: yozgi yaylov ----------------

void AErtWorldBuilder::BuildDomanic()
{
	FRandomStream RS(Seed + 271);
	FErtMeshData M(100.f);
	int32 S = 11500;
	auto DE = [](float u) { return DomE + u; };
	auto DN = [](float v) { return DomN + v; };
	auto Hz = [&](float u, float v) { return HeightAt(DomE + u, DomN + v); };
	const FLinearColor DFelt(0.86f, 0.82f, 0.72f), DFeltD(0.55f, 0.48f, 0.38f), DWood = ErtCol::Sty(FLinearColor(0.40f, 0.27f, 0.13f), ErtCol::StyleWood), Wool(0.92f, 0.90f, 0.84f), WoolD(0.35f, 0.30f, 0.25f), Water(0.28f, 0.50f, 0.62f), StoneG = ErtCol::Sty(FLinearColor(0.60f, 0.58f, 0.52f), ErtCol::StyleStone), Cheese(0.95f, 0.88f, 0.6f), DRed(0.55f, 0.10f, 0.08f);
	// Yozgi o'tovlar halqasi (8 ta kichik o'tov), markazda katta gulxan va tug'
	for (int32 i = 0; i < 8; ++i)
	{
		const float A = i * 2.f * PI / 8 + 0.2f, R = 16.f;
		const float u = FMath::Cos(A) * R, v = FMath::Sin(A) * R;
		AddYurt(M, DE(u), DN(v), Hz(u, v), 2.4f, 1.7f, 1.6f, ErtCol::Vary(DFelt, 0.05f, ++S), FLinearColor(0.72f, 0.66f, 0.55f), FMath::RadiansToDegrees(A) + 180.f, ++S);
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
		M.AddCylinder(W(DE(u), DN(v), ZZ + 1.85f), 0.2f, 0.2f, 0.3f, 6, DFelt, true);
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

// ---------------- O't-o'lan: kesishgan tolalar, shamol materiali (alfa = tebranish og'irligi) ----------------

static void ErtAddBlade(FErtMeshData& M, const FVector& Base, float Yaw, float Hgt, float Wd, const FLinearColor& Col, float Bend)
{
	const FQuat Q = FRotator(0, Yaw, 0).Quaternion();
	const FVector R = Q.RotateVector(FVector(0, Wd * 0.5f, 0)), Fw = Q.RotateVector(FVector(Bend, 0, 0));
	const int32 B = M.Verts.Num();
	FLinearColor Bot = Col; Bot.A = 0.f; FLinearColor Top = Col; Top.A = 1.f;
	M.Verts.Add(Base - R); M.Verts.Add(Base + R); M.Verts.Add(Base + R * 0.35f + Fw + FVector(0, 0, Hgt)); M.Verts.Add(Base - R * 0.35f + Fw + FVector(0, 0, Hgt));
	M.Colors.Add(Bot); M.Colors.Add(Bot); M.Colors.Add(Top); M.Colors.Add(Top);
	const FVector Nm = Q.RotateVector(FVector(1, 0, 0));
	for (int32 i = 0; i < 4; ++i) { M.Normals.Add(Nm); M.UVs.Add(FVector2D(i & 1, i >> 1)); M.Tangents.Add(FProcMeshTangent(0, 1, 0)); }
	M.Tris.Append({ B, B + 2, B + 1, B, B + 3, B + 2 });
}

void AErtWorldBuilder::BuildGrass()
{
	if (!GrassMat) return;
	FRandomStream RS(Seed + 17);
	const float Half = WorldSizeM * 0.5f;
	const int32 CellsPerSide = 6;
	TArray<FErtMeshData> Cells; Cells.Init(FErtMeshData(1.f), CellsPerSide * CellsPerSide);
	int32 Placed = 0;
	const FLinearColor G1(0.30f, 0.46f, 0.12f), G2(0.42f, 0.55f, 0.16f), G3(0.55f, 0.58f, 0.22f);
	for (int32 i = 0; i < GrassClumps * 8 && Placed < GrassClumps; ++i)
	{
		const float E = RS.FRandRange(-Half + 5.f, Half - 5.f), N = RS.FRandRange(-Half + 5.f, Half - 5.f);
		if (N < DesertN + 30.f) continue;
		const float H = HeightAt(E, N);
		const FVector Nm = TerrainNormal(E, N);
		if (H < WaterZ + 1.2f || H > 70.f || Nm.Z < 0.86f) continue;
		float Wd = 0.f; if (RoadDist(E, N, &Wd) < Wd * 0.5f + 1.f) continue;
		// Zichlik: yashil o'tloqlar (shimol) zich, quruq janub siyrak; oba, So'g'ut, yaylov atrofi zichroq
		float Dens = 0.35f + 0.5f * Smooth01((N + 100.f) / 500.f);
		Dens *= 0.6f + 0.4f * Noise(E, N, 0.02f);
		for (const FVector2D& Hot : { FVector2D(ObaE, ObaN), FVector2D(SogE, SogN), FVector2D(DomE, DomN), FVector2D(BurE, BurN) })
			Dens = FMath::Max(Dens, 0.95f * (1.f - Smooth01((FVector2D::Distance(FVector2D(E, N), Hot) - 120.f) / 200.f)));
		if (RS.FRand() > Dens) continue;
		if (!IsBuildable(E, N)) continue;
		const int32 cx = FMath::Clamp((int32)((E + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		const int32 cy = FMath::Clamp((int32)((N + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		{
			FErtFabLib& Fab = FErtFabLib::Get();
			if (Fab.Bushes.Num() && (Placed % 40) == 17)
			{
				UStaticMesh* Mesh = Fab.Bushes[RS.RandRange(0, Fab.Bushes.Num() - 1)];
				FabComp(Mesh, false)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, H - 0.03f), FVector(FErtFabLib::ScaleToHeight(Mesh, RS.FRandRange(0.5f, 1.1f)))), true);
				++Placed;
				continue;
			}
		}
		FErtMeshData& M = Cells[cy * CellsPerSide + cx];
		const FVector Base = W(E, N, H - 0.02f);
		const float Hgt = RS.FRandRange(22.f, 42.f) * (1.f + 0.3f * Smooth01((N + 100.f) / 500.f));
		const FLinearColor Col = ErtCol::Vary(RS.FRand() < 0.6f ? G1 : (RS.FRand() < 0.5f ? G2 : G3), 0.12f, i);
		const float Yaw0 = RS.FRandRange(0.f, 180.f);
		for (int32 b = 0; b < 4; ++b) ErtAddBlade(M, Base + FVector(RS.FRandRange(-16.f, 16.f), RS.FRandRange(-16.f, 16.f), 0), Yaw0 + b * 45.f, Hgt * RS.FRandRange(0.7f, 1.1f), RS.FRandRange(30.f, 50.f), Col, RS.FRandRange(-12.f, 12.f));
		++Placed;
	}
	for (int32 cidx = 0; cidx < Cells.Num(); ++cidx)
		if (Cells[cidx].Verts.Num())
		{
			UProceduralMeshComponent* P = NewPart(FString::Printf(TEXT("Grass_%d"), cidx), false, GrassMat);
			Cells[cidx].Commit(P, 0, false);
			P->SetCastShadow(false);
		}
	UE_LOG(LogErtugrul, Log, TEXT("O't-o'lan: %d tup"), Placed);
}

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
			const float TargetR = RS.FRandRange(0.45f, 0.75f);   // maksimal yarim o'lcham (m) - yupqa taxtalar ulkan bo'lmasin
			FabComp(Mesh, true)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), Wp, FVector(FErtFabLib::ScaleToRadius(Mesh, TargetR))), true);
			++i; ++Placed;
		}
	}
	// Ichki jihozlar: har o'tov/uyga 1-3 buyum (trace tekshiruvisiz - tom ostida), devordan uzoqroq
	int32 Inside = 0;
	for (const FVector4& In : Interiors)
	{
		const int32 Cnt = RS.RandRange(1, 3);
		for (int32 i = 0; i < Cnt; ++i)
		{
			const float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(0.3f, 0.8f) * In.Z;
			const float E = In.X + FMath::Cos(A) * R, N = In.Y + FMath::Sin(A) * R;
			UStaticMesh* Mesh = Fab.Props[RS.RandRange(0, Fab.Props.Num() - 1)];
			FabComp(Mesh, false)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, In.W), FVector(FErtFabLib::ScaleToRadius(Mesh, RS.FRandRange(0.3f, 0.5f)))), true);
			++Inside;
		}
	}
	UE_LOG(LogErtugrul, Log, TEXT("Buyumlar (Fab): %d tashqarida, %d ichkarida (%d xona)"), Placed, Inside, Interiors.Num());
}


// ---------------- Spline devorlar (Grid Snapping + Spline Mesh uslubi), dekallar, qirg'oq o'simliklari ----------------

void AErtWorldBuilder::AddWallSpline(FErtMeshData& M, const TArray<FVector2D>& InPts, float H, float Thick, const FLinearColor& Col, bool bBattlements, bool bTowers, int32 S)
{
	if (InPts.Num() < 2) return;
	TArray<FVector2D> Pts; for (const FVector2D& P : InPts) Pts.Add(ErtSnap(P, 1.f));   // 1 m to'r
	const float Module = 4.f;   // takrorlanuvchi devor bo'lagi uzunligi (m)
	for (int32 i = 0; i + 1 < Pts.Num(); ++i)
	{
		const FVector2D A = Pts[i], B = Pts[i + 1];
		const float Len = FVector2D::Distance(A, B); if (Len < 0.5f) continue;
		const FVector2D Dir = (B - A) / Len;
		const int32 NMod = FMath::Max(1, FMath::RoundToInt(Len / Module));
		const float ML = Len / NMod;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
		for (int32 m = 0; m < NMod; ++m)
		{
			const FVector2D C0 = A + Dir * (m + 0.5f) * ML;
			const float Z0 = FMath::Min(HeightAt(A.X + Dir.X * m * ML, A.Y + Dir.Y * m * ML), HeightAt(A.X + Dir.X * (m + 1) * ML, A.Y + Dir.Y * (m + 1) * ML)) - 0.6f;
			const float Zt = HeightAt(C0.X, C0.Y) + H;
			const FLinearColor Cm = ErtCol::Vary(Col, 0.08f, S + i * 97 + m);
			// Modul: egilgan spline bo'ylab joylashgan quti (devorning o'zi)
			M.AddBox(W(C0.X, C0.Y, (Z0 + Zt) * 0.5f), FVector(ML * 0.5f * 100.f + Thick * 55.f, Thick * 50.f, (Zt - Z0) * 50.f), Cm, FRotator(0, Yaw, 0));
			if (bBattlements)
				for (int32 t = 0; t < 2; ++t)
				{
					const FVector2D Ct = C0 + Dir * ((t - 0.5f) * ML * 0.5f);
					M.AddBox(W(Ct.X, Ct.Y, Zt + 0.35f), FVector(ML * 0.2f * 100.f, Thick * 50.f, 35.f), Cm * 0.95f, FRotator(0, Yaw, 0));
				}
			WallSegs.Add({ A + Dir * m * ML, A + Dir * (m + 1) * ML, Z0 + 0.6f, Zt - Z0 });
		}
	}
	// Tutashuv ustunlari (burchak bo'shliqlarini yopadi)
	for (int32 i = 0; i < Pts.Num(); ++i)
	{
		const float Zb = HeightAt(Pts[i].X, Pts[i].Y) - 0.6f;
		M.AddCylinder(W(Pts[i].X, Pts[i].Y, Zb), Thick * 0.62f, Thick * 0.6f, H + 0.75f, 8, ErtCol::Vary(Col, 0.05f, S + 7 * i), true, FRotator::ZeroRotator, 0.02f, S + i);
	}
	if (bTowers)
		for (int32 i = 0; i < Pts.Num(); ++i)
		{
			if (i > 0 && i + 1 < Pts.Num()) { const FVector2D D0 = (Pts[i] - Pts[i - 1]).GetSafeNormal(), D1 = (Pts[i + 1] - Pts[i]).GetSafeNormal(); if (FVector2D::DotProduct(D0, D1) > 0.9f) continue; }   // to'g'ri chiziqda minora yo'q
			const float Zb = HeightAt(Pts[i].X, Pts[i].Y) - 0.6f;
			M.AddCylinder(W(Pts[i].X, Pts[i].Y, Zb), Thick * 1.4f, Thick * 1.3f, H + 1.8f + 0.6f, 10, ErtCol::Vary(Col, 0.06f, S + i), true, FRotator::ZeroRotator, 0.03f, S + i);
			M.AddCone(W(Pts[i].X, Pts[i].Y, Zb + H + 2.4f), Thick * 1.5f, 1.2f, 10, ErtCol::Sty(FLinearColor(0.42f, 0.28f, 0.18f), ErtCol::StyleRoof));
		}
}

void AErtWorldBuilder::BuildSplineWalls()
{
	FErtMeshData M(100.f);
	const FLinearColor DryStone = ErtCol::Sty(FLinearColor(0.58f, 0.55f, 0.48f), ErtCol::StyleStone), Curtain = ErtCol::Sty(FLinearColor(0.50f, 0.48f, 0.45f), ErtCol::StyleStone), Quay = ErtCol::Sty(FLinearColor(0.55f, 0.56f, 0.55f), ErtCol::StyleStone);
	// So'g'ut: uzumzor devori (past quruq tosh, egri spline)
	AddWallSpline(M, { {SogE - 95.f, SogN + 20.f}, {SogE - 80.f, SogN + 45.f}, {SogE - 55.f, SogN + 62.f}, {SogE - 20.f, SogN + 70.f}, {SogE + 25.f, SogN + 66.f} }, 1.4f, 0.6f, DryStone, false, false, 3100);
	// Domaniç: qo'ra (halqa)
	{
		TArray<FVector2D> Ring; for (int32 i = 0; i <= 8; ++i) { const float A = 2.f * PI * i / 8; Ring.Add(FVector2D(DomE + 40.f + FMath::Cos(A) * 14.f, DomN - 30.f + FMath::Sin(A) * 14.f)); }
		AddWallSpline(M, Ring, 1.2f, 0.5f, DryStone, false, false, 3200);
	}
	// Bagras: yo'l bo'yidagi tashqi qo'rg'on devori (tishli, minorali)
	AddWallSpline(M, { {FortE - 150.f, FortN - 210.f}, {FortE - 120.f, FortN - 180.f}, {FortE - 95.f, FortN - 140.f}, {FortE - 80.f, FortN - 100.f} }, 4.5f, 1.4f, Curtain, true, true, 3300);
	// Nikeya: Askaniya ko'li qirg'og'idagi tosh qirg'oq devori (yoy)
	{
		TArray<FVector2D> Arc; for (int32 i = 0; i <= 10; ++i) { const float A = FMath::DegreesToRadians(200.f + 14.f * i); Arc.Add(FVector2D(AskE + FMath::Cos(A) * (AskR + 7.f), AskN + FMath::Sin(A) * (AskR + 7.f))); }
		AddWallSpline(M, Arc, 1.1f, 0.8f, Quay, false, false, 3400);
	}
	// Konya: shahar tashqarisidagi karvon yo'li devori
	AddWallSpline(M, { {KonE - 200.f, KonN - 40.f}, {KonE - 175.f, KonN - 10.f}, {KonE - 160.f, KonN + 30.f}, {KonE - 165.f, KonN + 70.f} }, 3.2f, 1.1f, Curtain, true, true, 3500);
	// Qayi obasi: janubiy chegara devori (yog'och ustunli tosh)
	AddWallSpline(M, { {ObaE - 130.f, ObaN - 140.f}, {ObaE - 90.f, ObaN - 150.f}, {ObaE - 40.f, ObaN - 152.f}, {ObaE + 10.f, ObaN - 148.f} }, 1.6f, 0.7f, DryStone, false, false, 3600);
	M.Commit(NewPart(TEXT("SplineWalls"), true), 0, true);
	UE_LOG(LogErtugrul, Log, TEXT("Spline devorlar: %d modul"), WallSegs.Num());
}

void AErtWorldBuilder::BuildDecals()
{
	DecalMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtDecal.M_ErtDecal"));
	if (!DecalMat) { UE_LOG(LogErtugrul, Warning, TEXT("M_ErtDecal yo'q - dekallar o'tkazib yuborildi (ert_make_decal.py)")); return; }
	FRandomStream RS(Seed + 41);
	int32 N = 0;
	auto Place = [&](const FVector& Pos, float Yaw, float Size, float Kind)
	{
		UDecalComponent* D = NewObject<UDecalComponent>(this, *FString::Printf(TEXT("Decal_%d"), N));
		D->SetupAttachment(RootComponent);
		D->SetDecalMaterial(DecalMat);
		D->DecalSize = FVector(40.f, Size, Size);
		D->SetWorldLocation(Pos);
		D->SetWorldRotation(FRotator(0.f, Yaw, RS.FRandRange(0.f, 360.f)));
		D->SetFadeScreenSize(0.005f);
		D->RegisterComponent();
		if (UMaterialInstanceDynamic* MID = D->CreateDynamicMaterialInstance()) { MID->SetScalarParameterValue(TEXT("Kind"), Kind); MID->SetScalarParameterValue(TEXT("Seed"), RS.FRandRange(0.f, 10.f)); }
		Decals.Add(D); ++N;
	};
	// Spline devorlar: har modulda 55% ehtimol bilan mog'or (pastda) yoki yoriq (o'rtada), ikki tomonda
	for (const FWallSeg& Sg : WallSegs)
	{
		if (RS.FRand() > 0.55f) continue;
		const FVector2D Dir = (Sg.B - Sg.A).GetSafeNormal(), Nrm(-Dir.Y, Dir.X);
		const FVector2D C = (Sg.A + Sg.B) * 0.5f + Dir * RS.FRandRange(-1.2f, 1.2f);
		const bool bMoss = RS.FRand() < 0.6f;
		const float Zc = Sg.Z + (bMoss ? 0.35f : Sg.H * RS.FRandRange(0.4f, 0.75f));
		const float Side = RS.FRand() < 0.5f ? 1.f : -1.f;
		const FVector2D P = C + Nrm * Side * 0.05f;
		// Dekal X o'qi devorga qarab (proyeksiya yo'nalishi): Nrm * -Side
		Place(W(P.X, P.Y, Zc) + FVector(0, 0, 0), FMath::RadiansToDegrees(FMath::Atan2(-Side * Nrm.Y, -Side * Nrm.X)), RS.FRandRange(70.f, 140.f), bMoss ? 0.f : 1.f);
	}
	// Uylar/o'tovlar (Interiors): tashqi devor tagida mog'or/dog'
	for (const FVector4& In : Interiors)
	{
		if (RS.FRand() > 0.5f) continue;
		const float A = RS.FRand() * 2.f * PI;
		const float R = In.Z + 0.02f;
		const FVector2D P(In.X + FMath::Cos(A) * R, In.Y + FMath::Sin(A) * R);
		Place(W(P.X, P.Y, In.W + 0.4f), FMath::RadiansToDegrees(A) + 180.f, RS.FRandRange(60.f, 110.f), RS.FRand() < 0.7f ? 0.f : 2.f);
	}
	UE_LOG(LogErtugrul, Log, TEXT("Dekallar: %d"), N);
}

void AErtWorldBuilder::BuildShoreFoliage()
{
	FRandomStream RS(Seed + 53);
	FErtMeshData G(1.f), Pebbles(100.f);
	const FLinearColor Reed(0.38f, 0.52f, 0.20f), ReedD(0.30f, 0.42f, 0.16f), Pebble = ErtCol::Sty(FLinearColor(0.55f, 0.53f, 0.48f), ErtCol::StyleRock);
	int32 Clumps = 0, Stones = 0;
	auto Clump = [&](float E, float N, float Hgt, const FLinearColor& C)
	{
		const float H = HeightAt(E, N);
		float SurfZ; if (IsWater(E, N, SurfZ) && H < SurfZ - 0.1f) return;
		const FVector Base = W(E, N, H - 0.02f);
		const float Yaw0 = RS.FRandRange(0.f, 180.f);
		for (int32 b = 0; b < 4; ++b) ErtAddBlade(G, Base + FVector(RS.FRandRange(-14.f, 14.f), RS.FRandRange(-14.f, 14.f), 0), Yaw0 + b * 45.f, Hgt * RS.FRandRange(0.7f, 1.15f), RS.FRandRange(30.f, 55.f), ErtCol::Vary(C, 0.12f, Clumps), RS.FRandRange(-10.f, 10.f));
		++Clumps;
	};
	auto Pebb = [&](float E, float N)
	{
		const float H = HeightAt(E, N);
		Pebbles.AddSphere(W(E, N, H - 0.05f), RS.FRandRange(0.08f, 0.22f), 5, ErtCol::Vary(Pebble, 0.15f, Stones), FVector(1.f, RS.FRandRange(0.7f, 1.f), 0.6f), 0.2f, Stones);
		++Stones;
	};
	// Ko'l va voha qirg'oqlari: qamish + mayda toshlar (halqa)
	struct FLk { float E, N, R; };
	for (const FLk& L : { FLk{LakeE, LakeN, LakeR}, FLk{AskE, AskN, AskR}, FLk{OasisE, OasisN, OasisR} })
		for (int32 i = 0; i < 260; ++i)
		{
			const float A = RS.FRand() * 2.f * PI, R = L.R + RS.FRandRange(4.5f, 12.f);
			const float E = L.E + FMath::Cos(A) * R, N = L.N + FMath::Sin(A) * R;
			if (!IsBuildable(E, N)) continue;
			if (RS.FRand() < 0.7f) Clump(E, N, RS.FRandRange(45.f, 80.f), RS.FRand() < 0.5f ? Reed : ReedD); else Pebb(E, N);
		}
	// Daryo bo'ylari
	for (float N = -980.f; N < 980.f; N += 3.f)
	{
		if (N < DesertN + 20.f) continue;
		for (int32 s = -1; s <= 1; s += 2)
		{
			if (RS.FRand() > 0.55f) continue;
			const float E = RiverE(N) + s * RS.FRandRange(27.f, 36.f);
			float Wd = 0.f; if (RoadDist(E, N, &Wd) < Wd * 0.5f + 1.f) continue;
			if (RS.FRand() < 0.75f) Clump(E, N, RS.FRandRange(40.f, 75.f), Reed); else Pebb(E, N);
		}
	}
	// Spline devor tagi: maysa va toshchalar (ikki tomon)
	for (const FWallSeg& Sg : WallSegs)
	{
		const FVector2D Dir = (Sg.B - Sg.A).GetSafeNormal(), Nrm(-Dir.Y, Dir.X);
		for (int32 k = 0; k < 3; ++k)
		{
			const FVector2D P = Sg.A + Dir * RS.FRandRange(0.f, FVector2D::Distance(Sg.A, Sg.B)) + Nrm * (RS.FRand() < 0.5f ? 1.f : -1.f) * RS.FRandRange(0.5f, 1.4f);
			if (RS.FRand() < 0.7f) Clump(P.X, P.Y, RS.FRandRange(25.f, 45.f), ReedD); else Pebb(P.X, P.Y);
		}
	}
	if (G.Verts.Num()) { UProceduralMeshComponent* P = NewPart(TEXT("ShoreGrass"), false, GrassMat ? GrassMat : Mat); G.Commit(P, 0, false); P->SetCastShadow(false); }
	if (Pebbles.Verts.Num()) Pebbles.Commit(NewPart(TEXT("Pebbles"), false), 0, false);
	UE_LOG(LogErtugrul, Log, TEXT("Qirg'oq/devor o'simliklari: %d tup, %d tosh"), Clumps, Stones);
}


// ---------------- Realistik butalar: barg kartochkalari (alfa niqobli M_ErtLeaf) + shoxlar ----------------

void AErtWorldBuilder::BuildBushes()
{
	FRandomStream RS(Seed + 61);
	const float Half = WorldSizeM * 0.5f;
	const int32 CellsPerSide = 5;
	TArray<FErtMeshData> Leaves; Leaves.Init(FErtMeshData(1.f), CellsPerSide * CellsPerSide);
	FErtMeshData Twigs(1.f);
	const FLinearColor Bark = ErtCol::Sty(FLinearColor(0.30f, 0.22f, 0.14f), ErtCol::StyleBark);
	int32 Placed = 0;
	auto Bush = [&](float E, float N, float H, int32 Kind)
	{
		// Kind 0 yashil buta (o'tloq), 1 to'q yashil (o'rmon), 2 quruq/kulrang (cho'l, tog'), 3 gullagan (oba/qishloq)
		const FLinearColor Base = Kind == 0 ? FLinearColor(0.16f, 0.34f, 0.09f) : (Kind == 1 ? FLinearColor(0.10f, 0.24f, 0.07f) : (Kind == 2 ? FLinearColor(0.36f, 0.36f, 0.20f) : FLinearColor(0.20f, 0.38f, 0.10f)));
		const float R = RS.FRandRange(80.f, 190.f) * (Kind == 2 ? 0.65f : 1.f), Hg = R * RS.FRandRange(0.75f, 1.2f);
		const FVector C = W(E, N, H) + FVector(0, 0, Hg * 0.55f);
		const int32 cx = FMath::Clamp((int32)((E + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		const int32 cy = FMath::Clamp((int32)((N + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		FErtMeshData& M = Leaves[cy * CellsPerSide + cx];
		const int32 Cards = Kind == 2 ? 10 : RS.RandRange(16, 26);
		for (int32 c = 0; c < Cards; ++c)
		{
			// Ellipsoid ichida tasodifiy nuqta, kartochka tashqariga qaraydi va biroz yuqoriga egilgan
			FVector Dir(RS.FRandRange(-1.f, 1.f), RS.FRandRange(-1.f, 1.f), RS.FRandRange(-0.4f, 1.f)); Dir.Normalize();
			const FVector P = C + FVector(Dir.X * R, Dir.Y * R, Dir.Z * Hg * 0.5f) * RS.FRandRange(0.35f, 0.85f);
			const float S = RS.FRandRange(0.5f, 0.85f) * R;
			FVector Out = Dir; Out.Z = FMath::Max(Out.Z, 0.1f); Out.Normalize();
			const FVector Rt = FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal();
			const FVector Up = FVector::CrossProduct(Out, Rt).GetSafeNormal();
			const FQuat Twist(Out, RS.FRandRange(-0.8f, 0.8f));
			const FVector R2 = Twist.RotateVector(Rt) * S, U2 = Twist.RotateVector(Up) * S;
			const FLinearColor Col = ErtCol::Vary(Base * (0.75f + 0.5f * (Dir.Z * 0.5f + 0.5f)), 0.12f, Placed * 31 + c);
			// Alfa: kartochka balandligi (0 pastda .. 1 tepada) - shamol tebranishi kuchi
			const float A = FMath::Clamp((P.Z - (C.Z - Hg * 0.55f)) / Hg, 0.f, 1.f);
			M.AddQuadUV(P - R2 - U2, P + R2 - U2, P + R2 + U2, P - R2 + U2, Out, ErtCol::Sty(Col, A));
			if (Kind == 3 && c % 5 == 0) M.AddQuadUV(P - R2 * 0.3f - U2 * 0.3f + Out * 3.f, P + R2 * 0.3f - U2 * 0.3f + Out * 3.f, P + R2 * 0.3f + U2 * 0.3f + Out * 3.f, P - R2 * 0.3f + U2 * 0.3f + Out * 3.f, Out, ErtCol::Sty(FLinearColor(0.95f, 0.85f, 0.9f), A));   // gullar
		}
		// Shoxlar: markazdan 3-5 ta ingichka silindr
		const int32 NT = RS.RandRange(3, 5);
		for (int32 t = 0; t < NT; ++t)
		{
			const float A = RS.FRandRange(0.f, 2.f * PI), Tilt = RS.FRandRange(15.f, 45.f);
			Twigs.AddCylinder(W(E, N, H - 0.05f), 2.2f, 1.f, Hg * 0.8f, 4, Bark, false, FRotator(0, FMath::RadiansToDegrees(A), 0) + FRotator(Tilt * FMath::Cos(A), 0, Tilt * FMath::Sin(A)));
		}
		++Placed;
	};
	for (int32 i = 0; i < BushCount * 10 && Placed < BushCount; ++i)
	{
		const float E = RS.FRandRange(-Half + 8.f, Half - 8.f), N = RS.FRandRange(-Half + 8.f, Half - 8.f);
		const float H = HeightAt(E, N);
		const FVector Nm = TerrainNormal(E, N);
		if (H < WaterZ + 0.8f || H > 80.f || Nm.Z < 0.8f) continue;
		float Wd = 0.f; if (RoadDist(E, N, &Wd) < Wd * 0.5f + 2.f) continue;
		float SurfZ; if (IsWater(E, N, SurfZ)) continue;
		if (!IsBuildable(E, N)) continue;
		const bool bDesert = N < DesertN + 40.f;
		// Zichlik: o'rmon chekkasi, daryo/ko'l bo'yi, oba/qishloq atrofi zich; ochiq dasht siyrak; cho'lda kam
		float Dens = bDesert ? 0.12f : 0.3f;
		Dens = FMath::Max(Dens, 0.8f * (1.f - Smooth01((FMath::Abs(E - RiverE(N)) - 30.f) / 50.f)));
		for (const FVector2D& Hot : { FVector2D(ObaE, ObaN), FVector2D(SogE, SogN), FVector2D(DomE, DomN), FVector2D(BurE, BurN), FVector2D(NikE, NikN), FVector2D(-330.f, 700.f), FVector2D(LakeE, LakeN), FVector2D(AskE, AskN) })
			Dens = FMath::Max(Dens, 0.9f * (1.f - Smooth01((FVector2D::Distance(FVector2D(E, N), Hot) - 100.f) / 220.f)));
		Dens *= 0.5f + 0.5f * Noise(E, N, 0.03f);
		if (RS.FRand() > Dens) continue;
		int32 Kind = 0;
		if (bDesert || H > 60.f) Kind = 2;
		else if (E < -120.f && N > 80.f && RS.FRand() < 0.6f) Kind = 1;   // shimoli-g'arb o'rmoni
		else if (RS.FRand() < 0.15f) Kind = 3;
		Bush(E, N, H, Kind);
	}
	for (int32 cidx = 0; cidx < Leaves.Num(); ++cidx)
		if (Leaves[cidx].Verts.Num())
		{
			UProceduralMeshComponent* P = NewPart(FString::Printf(TEXT("Bushes_%d"), cidx), false, LeafMat ? LeafMat : (GrassMat ? GrassMat : Mat));
			Leaves[cidx].Commit(P, 0, false);
		}
	if (Twigs.Verts.Num()) Twigs.Commit(NewPart(TEXT("BushTwigs"), false), 0, false);
	UE_LOG(LogErtugrul, Log, TEXT("Butalar: %d (barg kartochkali)"), Placed);
}


// ---------------- Landscape eksporti: heightmap (16-bit PNG, 2017x2017 = 1 m/piksel) va qatlam weightmap'lari (8-bit) ----------------

void AErtWorldBuilder::ExportLandscape(const FString& Dir) const
{
	IImageWrapperModule& IW = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
	const int32 Res = 2017;   // UE tavsiya o'lchami (2017 = 63*32+1 ... 8 komponent x 255 quad)
	const float Half = WorldSizeM * 0.5f;
	const float Step = WorldSizeM / (Res - 1);
	// Balandlik diapazoni: -20..300 m -> 0..65535; Landscape Z masshtabi: (320 m * 100 sm) / 512 * 100 = 6250
	const float Zmin = -20.f, Zmax = 300.f;
	TArray<uint16> Hm; Hm.SetNumUninitialized(Res * Res);
	TArray<uint8> Wgrass, Wdirt, Wrock, Wsnow, Wsand, Wroad; for (TArray<uint8>* Wt : { &Wgrass, &Wdirt, &Wrock, &Wsnow, &Wsand, &Wroad }) Wt->SetNumZeroed(Res * Res);
	for (int32 y = 0; y < Res; ++y)
		for (int32 x = 0; x < Res; ++x)
		{
			// Landscape X = +sharq (E), Y = +janub (UE Y o'qi); bizning reja: X=N, Y=E -> Landscape X <- E, Landscape Y <- -N
			const float E = -Half + x * Step, N = Half - y * Step;
			const float H = HeightAt(E, N);
			const int32 i = y * Res + x;
			Hm[i] = (uint16)FMath::Clamp((H - Zmin) / (Zmax - Zmin) * 65535.f, 0.f, 65535.f);
			const float Slope = 1.f - TerrainNormal(E, N).Z;
			float RW = 0.f; const float RD = RoadDist(E, N, &RW);
			const bool bRoad = RD < RW * 0.5f + 0.5f && N > DesertN;
			float Wr = FMath::Clamp((Slope - 0.25f) * 4.f, 0.f, 1.f), Ws = H > 75.f ? FMath::Clamp((H - 75.f) / 15.f, 0.f, 1.f) : 0.f;
			float Wsd = N < DesertN + 60.f ? FMath::Clamp((DesertN + 60.f - N) / 60.f, 0.f, 1.f) : 0.f;
			float Wd = FMath::Clamp(0.5f - 0.5f * Smooth01((N + 100.f) / 500.f) + 0.3f * Noise(E, N, 0.01f), 0.f, 1.f) * (1.f - Wsd);
			float Wg = FMath::Max(0.f, 1.f - Wd - Wsd);
			float Wrd = bRoad ? 1.f : 0.f;
			const float Rest = FMath::Max(0.f, 1.f - Wr - Ws - Wrd);
			Wgrass[i] = (uint8)(Wg * Rest * 255.f); Wdirt[i] = (uint8)(Wd * Rest * 255.f); Wsand[i] = (uint8)(Wsd * Rest * 255.f);
			Wrock[i] = (uint8)(Wr * 255.f); Wsnow[i] = (uint8)(Ws * 255.f); Wroad[i] = (uint8)(Wrd * 255.f);
		}
	IFileManager::Get().MakeDirectory(*Dir, true);
	{
		TSharedPtr<IImageWrapper> Png = IW.CreateImageWrapper(EImageFormat::PNG);
		if (Png.IsValid() && Png->SetRaw(Hm.GetData(), Hm.Num() * 2, Res, Res, ERGBFormat::Gray, 16))
		{
			const TArray64<uint8> Data = Png->GetCompressed();
			FFileHelper::SaveArrayToFile(TArrayView<const uint8>(Data.GetData(), Data.Num()), *(Dir / TEXT("heightmap.png")));
		}
	}
	auto SaveW = [&](const TArray<uint8>& Wt, const TCHAR* Name)
	{
		TSharedPtr<IImageWrapper> Png = IW.CreateImageWrapper(EImageFormat::PNG);
		if (Png.IsValid() && Png->SetRaw(Wt.GetData(), Wt.Num(), Res, Res, ERGBFormat::Gray, 8))
		{
			const TArray64<uint8> Data = Png->GetCompressed();
			FFileHelper::SaveArrayToFile(TArrayView<const uint8>(Data.GetData(), Data.Num()), *(Dir / FString::Printf(TEXT("weight_%s.png"), Name)));
		}
	};
	SaveW(Wgrass, TEXT("grass")); SaveW(Wdirt, TEXT("dirt")); SaveW(Wrock, TEXT("rock")); SaveW(Wsnow, TEXT("snow")); SaveW(Wsand, TEXT("sand")); SaveW(Wroad, TEXT("road"));
	FFileHelper::SaveStringToFile(FString::Printf(TEXT("Landscape import:\n  Section Size 63x63, Sections per component 2x2, Components 32x32 -> 2017x2017\n  Location X=%.0f Y=%.0f Z=%.0f (sm)\n  Scale X=100 Y=100 Z=%.2f  (balandlik %g..%g m)\n  Qatlamlar: grass, dirt, rock, snow, sand, road (weight_*.png)\n"), -Half * 100.f, -Half * 100.f, (Zmin + (Zmax - Zmin) * 0.5f) * 100.f, (Zmax - Zmin) * 100.f / 512.f, Zmin, Zmax), *(Dir / TEXT("README.txt")));
	UE_LOG(LogErtugrul, Log, TEXT("Landscape eksport: %s (heightmap %dx%d, 6 qatlam)"), *Dir, Res, Res);
}


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
