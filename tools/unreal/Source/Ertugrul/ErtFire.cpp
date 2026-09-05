#include "ErtFire.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

AErtFireFx::AErtFireFx()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AErtFireFx* AErtFireFx::Spawn(UWorld* W, const FVector& Pos, float Scale, bool bLight)
{
	if (!W) return nullptr;
	AErtFireFx* F = W->SpawnActor<AErtFireFx>(AErtFireFx::StaticClass(), Pos, FRotator::ZeroRotator);
	if (F) F->Init(Scale, bLight);
	return F;
}

void AErtFireFx::Init(float InScale, bool bLight)
{
	Scale = InScale;
	Seed = FMath::FRandRange(0.f, 100.f);
	UMaterialInterface* Dust = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtDust.M_ErtDust"));
	auto Make = [&](const TCHAR* Name)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, Name);
		P->SetupAttachment(RootComponent);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		P->bUseAsyncCooking = false;
		P->RegisterComponent();
		if (Dust) P->SetMaterial(0, Dust);
		return P;
	};
	FlameMesh = Make(TEXT("Flames"));
	SmokeMesh = Make(TEXT("Smoke"));
	if (bLight)
	{
		Light = NewObject<UPointLightComponent>(this, TEXT("FireLight"));
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(FVector(0, 0, 90.f * Scale));
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		BaseIntensity = 350.f * Scale;
		Light->SetIntensity(BaseIntensity);
		Light->SetLightColor(FLinearColor(1.f, 0.6f, 0.25f));
		Light->SetAttenuationRadius(1800.f * FMath::Sqrt(Scale));
		Light->SetCastShadows(false);
		Light->RegisterComponent();
	}
}

void AErtFireFx::Tick(float Dt)
{
	Super::Tick(Dt);
	T += Dt;
	// Miltillash: uch chastotali sinus + shovqin (Light Flicker Script)
	if (Light)
	{
		const float F = 0.78f + 0.12f * FMath::Sin(T * 9.3f + Seed) + 0.07f * FMath::Sin(T * 23.1f + Seed * 2.f) + 0.06f * FMath::PerlinNoise1D(T * 6.f + Seed);
		Light->SetIntensity(BaseIntensity * F);
		Light->SetLightColor(FMath::Lerp(FLinearColor(1.f, 0.5f, 0.15f), FLinearColor(1.f, 0.72f, 0.35f), (F - 0.6f) * 2.5f));
	}
	APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CamPos = Cam ? Cam->GetCameraLocation() : FVector::ZeroVector;
	const float DistCam = FVector::Dist(CamPos, GetActorLocation());
	if (DistCam > 15000.f) { if (Ps.Num()) { Ps.Reset(); FlameMesh->ClearAllMeshSections(); SmokeMesh->ClearAllMeshSections(); } return; }
	// Emissiya
	EmitAcc += Dt * 26.f * Scale;
	while (EmitAcc >= 1.f)
	{
		EmitAcc -= 1.f;
		const float R = FMath::FRand();
		FP P;
		const float Rad = 28.f * Scale;
		P.P = GetActorLocation() + FVector(FMath::FRandRange(-Rad, Rad), FMath::FRandRange(-Rad, Rad), 10.f * Scale);
		if (R < 0.62f) { P.Kind = 0; P.V = FVector(FMath::FRandRange(-10.f, 10.f), FMath::FRandRange(-10.f, 10.f), FMath::FRandRange(70.f, 130.f) * Scale); P.MaxLife = FMath::FRandRange(0.5f, 0.9f); P.Size = FMath::FRandRange(18.f, 34.f) * Scale; }
		else if (R < 0.9f) { P.Kind = 1; P.V = FVector(FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(60.f, 110.f) * Scale); P.MaxLife = FMath::FRandRange(2.2f, 3.6f); P.Size = FMath::FRandRange(20.f, 36.f) * Scale; P.P.Z += 60.f * Scale; }
		else { P.Kind = 2; P.V = FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(160.f, 300.f) * Scale); P.MaxLife = FMath::FRandRange(0.6f, 1.4f); P.Size = FMath::FRandRange(2.f, 4.f); }
		P.Life = P.MaxLife;
		Ps.Add(P);
	}
	// Simulyatsiya
	const FVector Wind(FMath::Sin(T * 0.7f) * 18.f, FMath::Cos(T * 0.5f) * 12.f, 0.f);
	for (int32 i = Ps.Num() - 1; i >= 0; --i)
	{
		FP& P = Ps[i];
		P.Life -= Dt;
		if (P.Life <= 0.f) { Ps.RemoveAtSwap(i); continue; }
		if (P.Kind == 2) P.V.Z -= 250.f * Dt;
		if (P.Kind == 1) { P.V += Wind * Dt; P.Size += 22.f * Scale * Dt; }
		P.P += P.V * Dt;
	}
	Rebuild(CamPos);
}

void AErtFireFx::Rebuild(const FVector& CamPos)
{
	FErtMeshData Fl, Sm;
	const FVector ToCam = (CamPos - GetActorLocation()).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToCam).GetSafeNormal();
	const FVector Up = FVector::CrossProduct(ToCam, Right).GetSafeNormal();
	for (const FP& P : Ps)
	{
		const float L = P.Life / P.MaxLife;
		if (P.Kind == 0)
		{
			// Olov tili: pastda sariq-oq, tepada qizil, so'nadi; kengligi pasayadi, balandligi cho'ziladi
			const float A = FMath::Clamp(L * 1.6f, 0.f, 1.f) * 0.85f;
			const FLinearColor C = FMath::Lerp(FLinearColor(1.f, 0.25f, 0.03f, A * 0.7f), FLinearColor(1.f, 0.85f, 0.35f, A), L);
			const float Wd = P.Size * (0.5f + 0.5f * L), Ht = P.Size * (1.2f + 0.8f * (1.f - L));
			const FVector Cn = P.P;
			Fl.AddQuad(Cn - Right * Wd - Up * Ht * 0.3f, Cn + Right * Wd - Up * Ht * 0.3f, Cn + Right * Wd * 0.4f + Up * Ht, Cn - Right * Wd * 0.4f + Up * Ht, ToCam, C);
		}
		else if (P.Kind == 1)
		{
			const float A = 0.22f * FMath::Sin(L * PI);
			const FLinearColor C(0.22f, 0.2f, 0.19f, A);
			const float S = P.Size;
			Sm.AddQuad(P.P - Right * S - Up * S, P.P + Right * S - Up * S, P.P + Right * S + Up * S, P.P - Right * S + Up * S, ToCam, C);
		}
		else
		{
			const FLinearColor C(1.f, 0.7f, 0.2f, FMath::Clamp(L * 2.f, 0.f, 1.f));
			const float S = P.Size;
			Fl.AddQuad(P.P - Right * S - Up * S, P.P + Right * S - Up * S, P.P + Right * S + Up * S, P.P - Right * S + Up * S, ToCam, C);
		}
	}
	if (Fl.Verts.Num()) Fl.Commit(FlameMesh, 0, false); else FlameMesh->ClearAllMeshSections();
	if (Sm.Verts.Num()) Sm.Commit(SmokeMesh, 0, false); else SmokeMesh->ClearAllMeshSections();
}
