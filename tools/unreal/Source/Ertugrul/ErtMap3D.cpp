#include "ErtMap3D.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ErtWorldBuilder.h"
#include "ErtMission.h"
#include "ErtGameMode.h"
#include "ErtNav.h"
#include "ErtEnemy.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInterface.h"

using namespace ErtMap;

AErtMap3D::AErtMap3D()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AErtMap3D* AErtMap3D::Get(UWorld* W)
{
	if (!W) return nullptr;
	if (AErtMap3D* M = Cast<AErtMap3D>(UGameplayStatics::GetActorOfClass(W, AErtMap3D::StaticClass()))) return M;
	return W->SpawnActor<AErtMap3D>(AErtMap3D::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
}

void AErtMap3D::BeginPlay()
{
	Super::BeginPlay();
	World = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(this, AErtWorldBuilder::StaticClass()));
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	auto Make = [&](const TCHAR* Name)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, Name);
		P->SetupAttachment(RootComponent);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		P->bUseAsyncCooking = false;
		P->RegisterComponent();
		if (Mat) P->SetMaterial(0, Mat);
		return P;
	};
	Terrain = Make(TEXT("MiniTerrain"));
	Markers = Make(TEXT("MiniMarkers"));
	RT = UKismetRenderingLibrary::CreateRenderTarget2D(this, 1024, 1024, ETextureRenderTargetFormat::RTF_RGBA8);
	Capture = NewObject<USceneCaptureComponent2D>(this, TEXT("MapCapture"));
	Capture->SetupAttachment(RootComponent);
	Capture->ProjectionType = ECameraProjectionMode::Orthographic;
	Capture->OrthoWidth = Ortho;
	Capture->TextureTarget = RT;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetVolumetricFog(false);
	Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetMotionBlur(false);
	Capture->ShowFlags.SetBloom(false);
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->ShowOnlyComponents.Add(Terrain);
	Capture->ShowOnlyComponents.Add(Markers);
	Capture->RegisterComponent();
	BuildTerrain();
	UpdateCamera();
}

void AErtMap3D::BuildTerrain()
{
	if (!World) return;
	const float Half = World->WorldSizeM * 0.5f, Cell = 20.f;
	const int32 Wd = FMath::RoundToInt(World->WorldSizeM / Cell) + 1;
	TArray<FVector> V; TArray<FLinearColor> C;
	for (int32 y = 0; y < Wd; ++y)
		for (int32 x = 0; x < Wd; ++x)
		{
			const float E = -Half + x * Cell, N = -Half + y * Cell;
			const float H = World->HeightAt(E, N);
			V.Add(Mini(AErtWorldBuilder::PlanToWorld(E, N, H)));
			FLinearColor Col = World->ColorAt(E, N);
			// Yo'llar va suv xaritada ajralib tursin
			float SurfZ; if (World->IsWater(E, N, SurfZ) && H < SurfZ - 0.2f) Col = FLinearColor(0.16f, 0.32f, 0.55f);
			C.Add(ErtCol::Sty(Col * 1.15f, ErtCol::StylePlain));
		}
	FErtMeshData M(1.f);
	M.AddGrid(V, C, Wd, Wd);
	// Suv sathi (tekis ko'k plastinka)
	const FVector A = Mini(AErtWorldBuilder::PlanToWorld(-Half, -Half, WaterZ)), B = Mini(AErtWorldBuilder::PlanToWorld(Half, Half, WaterZ));
	M.AddQuad(FVector(A.X, A.Y, A.Z), FVector(B.X, A.Y, A.Z), FVector(B.X, B.Y, A.Z), FVector(A.X, B.Y, A.Z), FVector::UpVector, ErtCol::Sty(FLinearColor(0.2f, 0.4f, 0.65f), ErtCol::StylePlain));
	// Shahar belgilari: kichik gumbazlar
	struct FCity { float E, N, R; FLinearColor C; };
	const FCity Cities[] = { {ObaE, ObaN, 60.f, FLinearColor(0.9f, 0.85f, 0.7f)}, {FortE, FortN, 30.f, FLinearColor(0.5f, 0.5f, 0.5f)}, {CityE, CityN, 80.f, FLinearColor(0.85f, 0.7f, 0.3f)}, {CampE, CampN, 60.f, FLinearColor(0.6f, 0.15f, 0.1f)},
		{DamE, DamN, 80.f, FLinearColor(0.8f, 0.7f, 0.5f)}, {HalabE, HalabN, 60.f, FLinearColor(0.7f, 0.6f, 0.45f)}, {KonE, KonN, 70.f, FLinearColor(0.75f, 0.65f, 0.5f)}, {KayE, KayN, 60.f, FLinearColor(0.55f, 0.5f, 0.45f)}, {SivE, SivN, 55.f, FLinearColor(0.6f, 0.55f, 0.5f)},
		{ErzE, ErzN, 50.f, FLinearColor(0.5f, 0.45f, 0.4f)}, {BurE, BurN, 55.f, FLinearColor(0.7f, 0.65f, 0.55f)}, {NikE, NikN, 50.f, FLinearColor(0.65f, 0.6f, 0.5f)}, {KarE, KarN, 25.f, FLinearColor(0.5f, 0.5f, 0.5f)}, {SogE, SogN, 35.f, FLinearColor(0.8f, 0.72f, 0.55f)} };
	for (const FCity& Ct : Cities)
	{
		const FVector P = Mini(AErtWorldBuilder::PlanToWorld(Ct.E, Ct.N, World->HeightAt(Ct.E, Ct.N)));
		M.AddSphere(P, Ct.R * 100.f * K, 8, ErtCol::Sty(Ct.C, ErtCol::StylePlain), FVector(1, 1, 0.5f));
	}
	M.Commit(Terrain, 0, false);
	UE_LOG(LogErtugrul, Log, TEXT("3D xarita: %d uchburchak"), M.NumTris());
}

void AErtMap3D::UpdateCamera()
{
	if (!Capture) return;
	const FRotator R(Pitch, Yaw, 0.f);
	const FVector Center = Origin + FVector(0, 0, 400.f);
	Capture->SetWorldLocation(Center - R.Vector() * 60000.f);
	Capture->SetWorldRotation(R);
	Capture->OrthoWidth = Ortho;
}

void AErtMap3D::Zoom(float Factor) { Ortho = FMath::Clamp(Ortho * Factor, 1200.f, 5200.f); UpdateCamera(); }

void AErtMap3D::SetActive(bool bOn)
{
	bActive = bOn;
	if (Capture) Capture->bCaptureEveryFrame = bOn;
	if (bOn) { UpdateMarkers(); if (Capture) Capture->CaptureScene(); }
}

bool AErtMap3D::Project(const FVector& WorldPos, float& U, float& V) const
{
	if (!Capture) return false;
	const FVector D = Mini(WorldPos) - Capture->GetComponentLocation();
	const FRotator R = Capture->GetComponentRotation();
	const FVector Right = FRotationMatrix(R).GetUnitAxis(EAxis::Y), Up = FRotationMatrix(R).GetUnitAxis(EAxis::Z);
	U = FVector::DotProduct(D, Right) / Ortho + 0.5f;
	V = 0.5f - FVector::DotProduct(D, Up) / Ortho;
	return U >= 0.f && U <= 1.f && V >= 0.f && V <= 1.f;
}

void AErtMap3D::UpdateMarkers()
{
	if (!Markers) return;
	FErtMeshData M(1.f);
	auto Cone = [&](const FVector& WorldPos, float R, float H, const FLinearColor& C)
	{
		const FVector P = Mini(WorldPos);
		M.AddCone(P, R, H, 6, ErtCol::Sty(C, ErtCol::StylePlain));
	};
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Cone(P->GetActorLocation(), 28.f, 90.f, FLinearColor(0.15f, 0.95f, 0.3f));
	}
	if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))
		if (AErtMissionDirector* D = GM->GetDirector())
		{
			TArray<FVector> Pts; D->GetMarkers(Pts);
			for (const FVector& Pt : Pts) Cone(Pt, 34.f, 130.f, FLinearColor(1.f, 0.8f, 0.2f));
			for (const AErtEnemy* E : D->GetEnemies()) if (E && !E->IsDead() && !E->IsAnimal()) Cone(E->GetActorLocation(), 14.f, 50.f, FLinearColor(0.9f, 0.15f, 0.1f));
		}
	// GPS yo'li
	if (AErtGps* G = Cast<AErtGps>(UGameplayStatics::GetActorOfClass(this, AErtGps::StaticClass())))
	{
		const TArray<FVector>& Path = G->GetPath();
		for (int32 i = 0; i + 1 < Path.Num(); ++i)
		{
			const FVector A = Mini(Path[i]) + FVector(0, 0, 12.f), B = Mini(Path[i + 1]) + FVector(0, 0, 12.f);
			const FVector Dir = (B - A).GetSafeNormal2D();
			const FVector Rt = FVector::CrossProduct(FVector::UpVector, Dir) * 8.f;
			M.AddQuad(A - Rt, A + Rt, B + Rt, B - Rt, FVector::UpVector, ErtCol::Sty(FLinearColor(1.f, 0.85f, 0.3f), ErtCol::StylePlain));
		}
	}
	if (M.Verts.Num()) M.Commit(Markers, 0, false); else Markers->ClearAllMeshSections();
}

void AErtMap3D::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bActive) return;
	MarkerT += Dt;
	if (MarkerT > 0.5f) { MarkerT = 0.f; UpdateMarkers(); }
}
