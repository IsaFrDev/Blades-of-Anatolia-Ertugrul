#include "ErtWeather.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/LocalFogVolumeComponent.h"
#include "ErtWorldBuilder.h"
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
	if (Fog && Fog->GetComponent())
	{
		// Ufqni yumshatish: ikki qatlamli tuman - past qatlam sekin so'nadi, ikkinchi qatlam ufqdagi keskin oq chiziqni yopadi
		UExponentialHeightFogComponent* F = Fog->GetComponent();
		F->SetFogHeightFalloff(0.12f);
		F->SetStartDistance(2500.f);
		F->SetFogMaxOpacity(0.92f);
		F->SetFogCutoffDistance(0.f);
		F->SetDirectionalInscatteringExponent(12.f);
		F->SetDirectionalInscatteringStartDistance(8000.f);
		F->SetDirectionalInscatteringColor(FLinearColor(0.9f, 0.7f, 0.45f));
		// Vodiy tumani: 18 m dan pastda qalin, tez so'nadi (tepaliklar ochiq)
		F->SecondFogData.FogDensity = 0.0022f;
		F->SecondFogData.FogHeightFalloff = 0.09f;
		F->SecondFogData.FogHeightOffset = 1800.f;
		// Volumetric Fog: nur shu'lalari va suv/o'rmon ustidagi hajmli tuman
		F->SetVolumetricFog(true);
		F->SetVolumetricFogScatteringDistribution(0.45f);
		F->SetVolumetricFogExtinctionScale(1.6f);
		F->VolumetricFogDistance = 14000.f;
		F->VolumetricFogAlbedo = FColor(235, 238, 245);
		F->MarkRenderStateDirty();
	}
	// Mahalliy tuman hajmlari (LocalFogVolume) sinovda butun dunyoni qopladi - o'rniga global hajmli tuman + ob-havo zichligi ishlatiladi
	// Bulut qatlami: 2 km balandlikda 36 km kenglikdagi tekislik (ikki qatlam), yulduz gumbazi: 60 km radiusli sfera (ichkaridan ko'rinadi)
	auto MakeSky = [&](const TCHAR* Name, const TCHAR* MatPath, TObjectPtr<UMaterialInstanceDynamic>& OutMID)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, Name);
		P->SetupAttachment(RootComponent);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		P->bUseAsyncCooking = false;
		P->RegisterComponent();
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, MatPath)) { OutMID = UMaterialInstanceDynamic::Create(M, this); P->SetMaterial(0, OutMID); }
		return P;
	};
	{
		CloudMesh = MakeSky(TEXT("Clouds"), TEXT("/Game/Ertugrul/Materials/M_ErtClouds.M_ErtClouds"), CloudMID);
		FErtMeshData D;
		const float Hs = 1800000.f;
		for (int32 L = 0; L < 2; ++L)
		{
			const float Z = 200000.f + L * 70000.f;
			const int32 N = 12;
			for (int32 i = 0; i < N; ++i) for (int32 j = 0; j < N; ++j)
			{
				const float x0 = -Hs + i * 2.f * Hs / N, x1 = x0 + 2.f * Hs / N, y0 = -Hs + j * 2.f * Hs / N, y1 = y0 + 2.f * Hs / N;
				D.AddQuad(FVector(x0, y0, Z), FVector(x1, y0, Z), FVector(x1, y1, Z), FVector(x0, y1, Z), -FVector::UpVector, FLinearColor::White);
			}
		}
		D.Commit(CloudMesh, 0, false);
		CloudMesh->SetBoundsScale(50.f);
	}
	{
		StarMesh = MakeSky(TEXT("Stars"), TEXT("/Game/Ertugrul/Materials/M_ErtStars.M_ErtStars"), StarMID);
		FErtMeshData D;
		D.AddSphere(FVector(0, 0, -200000.f), 6000000.f, 28, FLinearColor::White);
		D.Commit(StarMesh, 0, false);
		StarMesh->SetBoundsScale(50.f);
	}
}

void AErtWeather::UpdateSky(float Day, const FLinearColor& SunColor, float ElevDeg)
{
	const float Twilight = 1.f - FMath::Clamp(FMath::Abs(ElevDeg) / 12.f, 0.f, 1.f);   // ufq yaqinida
	if (CloudMID)
	{
		// Kunduzi oq, tong/shomda quyosh rangida (zarhal), tunda ko'kimtir-qorong'i; bo'ronda qoraytiriladi
		FLinearColor Tint = FMath::Lerp(FLinearColor(0.06f, 0.07f, 0.12f), FLinearColor(0.95f, 0.96f, 1.0f), Day);
		Tint = FMath::Lerp(Tint, SunColor * 1.05f, Twilight * Day);
		Tint *= (1.f - 0.65f * CloudDark);
		CloudMID->SetVectorParameterValue(TEXT("Tint"), Tint);
		CloudMID->SetScalarParameterValue(TEXT("Coverage"), Coverage);
		CloudMID->SetScalarParameterValue(TEXT("Density"), 0.85f + 0.15f * CloudDark);
	}
	if (StarMID) StarMID->SetScalarParameterValue(TEXT("Vis"), (1.f - Day) * (1.f - Coverage * 0.6f));
	if (Fog && Fog->GetComponent())
	{
		FLinearColor FogCol = FMath::Lerp(FLinearColor(0.02f, 0.03f, 0.06f), FLinearColor(0.62f, 0.72f, 0.86f), Day);
		FogCol = FMath::Lerp(FogCol, FLinearColor(0.62f, 0.38f, 0.24f), Twilight * Day * 0.7f);
		if (CloudDark > 0.f) FogCol = FMath::Lerp(FogCol, FLinearColor(0.35f, 0.37f, 0.4f), CloudDark * 0.7f);
		Fog->GetComponent()->SetFogInscatteringColor(FogCol);
		Fog->GetComponent()->SetDirectionalInscatteringColor(FMath::Lerp(FLinearColor(0.05f, 0.06f, 0.1f), FMath::Lerp(FLinearColor(0.75f, 0.72f, 0.65f), SunColor * 1.1f, Twilight), Day));
	}
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
	Coverage = 0.55f; CloudDark = 0.f;
	if (Name == TEXT("rain")) { Coverage = 0.25f; CloudDark = 0.55f; }
	else if (Name == TEXT("storm") || Name == TEXT("blizzard")) { Coverage = 0.12f; CloudDark = 0.85f; }
	else if (Name == TEXT("snow") || Name == TEXT("fog")) { Coverage = 0.3f; CloudDark = 0.3f; }
	else if (Name == TEXT("dust")) { Coverage = 0.6f; CloudDark = 0.4f; }
	else if (Name == TEXT("wind")) { Coverage = 0.45f; }
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
