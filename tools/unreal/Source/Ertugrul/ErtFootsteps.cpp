#include "ErtFootsteps.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Materials/MaterialInterface.h"

UErtFootsteps::UErtFootsteps()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UErtFootsteps::BeginPlay()
{
	Super::BeginPlay();
	DustMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtDust.M_ErtDust"));
	if (!DustMat) DustMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	Puffs.SetNum(6);
	for (int32 i = 0; i < 6; ++i)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(GetOwner(), *FString::Printf(TEXT("Puff%d"), i));
		P->SetupAttachment(nullptr);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		P->bUseAsyncCooking = false;
		P->RegisterComponent();
		P->SetAbsolute(true, true, true);
		if (DustMat) P->SetMaterial(0, DustMat);
		P->SetVisibility(false);
		Meshes.Add(P);
	}
	Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(22050);
	Wave->NumChannels = 1;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	Audio = NewObject<UAudioComponent>(GetOwner(), TEXT("FootAudio"));
	Audio->SetupAttachment(GetOwner()->GetRootComponent());
	Audio->bAutoActivate = false;
	Audio->RegisterComponent();
	Audio->SetSound(Wave);
	Audio->Play();
}

void UErtFootsteps::PlayNoise(float Seconds, float Pitch, float Softness, float Volume)
{
	if (!Wave) return;
	const int32 SR = 22050;
	const int32 N = FMath::Clamp((int32)(Seconds * SR / Pitch), 200, 22050);
	TArray<int16> Pcm; Pcm.SetNumUninitialized(N);
	FRandomStream RS(FMath::Rand());
	float Lp = 0.f;
	const float A = FMath::Clamp(Softness, 0.02f, 0.9f);   // past chastota filtri (qum yumshoqroq)
	for (int32 i = 0; i < N; ++i)
	{
		const float T = (float)i / N;
		const float Env = FMath::Exp(-T * 9.f) * (1.f - FMath::Exp(-T * 60.f));
		const float Noise = RS.FRandRange(-1.f, 1.f);
		Lp += (Noise - Lp) * A;
		Pcm[i] = (int16)FMath::Clamp(Lp * Env * Volume * 32000.f, -32000.f, 32000.f);
	}
	Wave->QueueAudio((const uint8*)Pcm.GetData(), N * sizeof(int16));
	if (Audio && !Audio->IsPlaying()) Audio->Play();
}

void UErtFootsteps::Step(const FVector& Pos, bool bSand, float Strength)
{
	// Chang
	for (FPuff& P : Puffs)
		if (P.T >= 1.f)
		{
			P.T = 0.f; P.Pos = Pos + FVector(FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f), 6.f);
			P.Size = (bSand ? 22.f : 13.f) * FMath::Clamp(Strength, 0.5f, 1.6f);
			P.Col = bSand ? FLinearColor(0.80f, 0.70f, 0.50f, 0.55f) : FLinearColor(0.45f, 0.42f, 0.30f, 0.35f);
			break;
		}
	// Ovoz: tasodifiy balandlik, qumda yumshoq va uzunroq
	PlayNoise(bSand ? 0.13f : 0.09f, FMath::FRandRange(0.85f, 1.18f), bSand ? 0.08f : 0.35f, FMath::Clamp(0.18f + 0.22f * Strength, 0.15f, 0.5f));
}

void UErtFootsteps::Splash(const FVector& Pos)
{
	for (int32 k = 0; k < 3; ++k)
		for (FPuff& P : Puffs)
			if (P.T >= 1.f) { P.T = 0.f; P.Pos = Pos + FVector(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), 10.f); P.Size = 20.f; P.Col = FLinearColor(0.7f, 0.85f, 0.95f, 0.6f); break; }
	PlayNoise(0.35f, FMath::FRandRange(0.9f, 1.1f), 0.5f, 0.45f);
}

void UErtFootsteps::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Tf)
{
	Super::TickComponent(Dt, TickType, Tf);
	for (int32 i = 0; i < Puffs.Num() && i < Meshes.Num(); ++i)
	{
		FPuff& P = Puffs[i];
		UProceduralMeshComponent* M = Meshes[i];
		if (P.T >= 1.f) { if (M->IsVisible()) M->SetVisibility(false); continue; }
		P.T = FMath::Min(1.f, P.T + Dt / 0.7f);
		const float S = P.Size * (0.6f + 2.2f * P.T);
		FLinearColor C = P.Col; C.A *= (1.f - P.T) * (1.f - P.T);
		FErtMeshData D;
		D.AddSphere(FVector::ZeroVector, S, 6, C, FVector(1.f, 1.f, 0.7f), 0.25f, i * 7 + 3);
		D.Commit(M, 0, false);
		M->SetWorldLocation(P.Pos + FVector(0, 0, P.T * 25.f));
		if (!M->IsVisible()) M->SetVisibility(true);
	}
}
