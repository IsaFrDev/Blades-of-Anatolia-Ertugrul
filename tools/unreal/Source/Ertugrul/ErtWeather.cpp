#include "ErtWeather.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

AErtWeather::AErtWeather()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AErtWeather::BeginPlay()
{
	Super::BeginPlay();
	Mesh = NewObject<UProceduralMeshComponent>(this, TEXT("Particles"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->RegisterComponent();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"))) Mesh->SetMaterial(0, M);
	Fog = Cast<AExponentialHeightFog>(UGameplayStatics::GetActorOfClass(this, AExponentialHeightFog::StaticClass()));
}

void AErtWeather::Build(int32 Count, const FVector& Size, const FLinearColor& Col, float Streak)
{
	FErtMeshData D;
	FRandomStream RS(77);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector P(RS.FRandRange(-1500.f, 1500.f), RS.FRandRange(-1500.f, 1500.f), RS.FRandRange(0.f, BoxH));
		D.AddBox(P, FVector(Size.X, Size.Y, Size.Z * (1.f + Streak * RS.FRand())), Col, FRotator(0, RS.FRandRange(0.f, 180.f), 0));
	}
	D.Commit(Mesh, 0, false);
}

void AErtWeather::SetWeather(const FString& Name)
{
	Current = Name;
	Mesh->ClearAllMeshSections();
	FallSpeed = 0.f; Drift = 0.f;
	float FogDensity = 0.0006f;
	if (Name == TEXT("rain") || Name == TEXT("storm"))
	{
		Build(Name == TEXT("storm") ? 900 : 500, FVector(0.4f, 0.4f, 10.f), FLinearColor(0.85f, 0.9f, 1.f, 1.f), 1.5f);
		FallSpeed = 1400.f; Drift = Name == TEXT("storm") ? 500.f : 120.f; FogDensity = 0.0018f;
	}
	else if (Name == TEXT("snow") || Name == TEXT("blizzard"))
	{
		Build(Name == TEXT("blizzard") ? 900 : 450, FVector(1.6f, 1.6f, 1.6f), FLinearColor(1.f, 1.f, 1.f, 1.f), 0.f);
		FallSpeed = 180.f; Drift = Name == TEXT("blizzard") ? 400.f : 60.f; FogDensity = 0.0025f;
	}
	else if (Name == TEXT("dust") || Name == TEXT("sandstorm"))
	{
		Build(600, FVector(2.5f, 2.5f, 1.2f), FLinearColor(0.8f, 0.68f, 0.45f, 1.f), 0.f);
		FallSpeed = 40.f; Drift = 700.f; FogDensity = 0.004f;
	}
	else if (Name == TEXT("fog") || Name == TEXT("mist")) FogDensity = 0.006f;
	if (Fog && Fog->GetComponent()) Fog->GetComponent()->SetFogDensity(FogDensity);
	UE_LOG(LogErtugrul, Log, TEXT("Ob-havo: %s"), *Name);
}

void AErtWeather::Tick(float Dt)
{
	Super::Tick(Dt);
	if (FallSpeed <= 0.f && Drift <= 0.f) return;
	APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Cam) return;
	Offset = FMath::Fmod(Offset + FallSpeed * Dt, BoxH);
	const FVector C = Cam->GetCameraLocation();
	const float T = GetWorld()->GetTimeSeconds();
	const FVector DriftV(FMath::Sin(T * 0.7f) * Drift * 0.3f + Drift * 0.5f, FMath::Cos(T * 0.5f) * Drift * 0.3f, 0);
	// Zarralar qutisi kameraga ergashadi; pastga siljish qutini aylantiradi (ikki nusxa - uzilish sezilmaydi)
	const float Z0 = C.Z - BoxH * 0.5f - Offset;
	Mesh->SetWorldLocation(FVector(C.X + FMath::Fmod(DriftV.X * T, 1500.f), C.Y + FMath::Fmod(DriftV.Y * T, 1500.f), Z0 + (Offset > BoxH * 0.5f ? BoxH : 0.f)));
}
