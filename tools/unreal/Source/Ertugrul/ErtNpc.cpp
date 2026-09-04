#include "ErtNpc.h"
#include "ErtHeroBody.h"
#include "ErtLoc.h"
#include "Kismet/GameplayStatics.h"
#include "ErtGameMode.h"
#include "Engine/World.h"
#include "ErtAudio.h"

AErtNpc::AErtNpc()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = CreateDefaultSubobject<UErtHeroBody>(TEXT("Body"));
}

void AErtNpc::Setup(const FString& InId, const FString& NameKey, const FString& InDialogId, bool bWoman, const FLinearColor& Kaftan, float Yaw)
{
	Id = InId; LocName = NameKey; DialogId = InDialogId; HomeYaw = Yaw;
	Body->Kaftan = Kaftan;
	Body->bWoman = bWoman; bWomanNpc = bWoman;
	FRandomStream RS(GetTypeHash(InId));
	Body->Fur = FLinearColor(0.55f + RS.FRand() * 0.35f, 0.5f + RS.FRand() * 0.3f, 0.4f + RS.FRand() * 0.3f);
	Body->Beard = RS.FRand() < 0.35f ? FLinearColor(0.7f, 0.68f, 0.64f) : FLinearColor(0.16f, 0.11f, 0.06f);
	Body->Build(RootComponent, 92.f);
	SetActorRotation(FRotator(0, Yaw, 0));
	HomePos = GetActorLocation(); Target = HomePos;
	WanderT = FMath::FRandRange(4.f, 12.f);
}

FString AErtNpc::GetDisplayName() const
{
	return FErtLoc::Get().TrOr(LocName, Id);
}

void AErtNpc::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Body || !Body->IsBuilt()) return;
	// Kundalik hayot: kunduzi uyi atrofida 6 m radiusda yuradi, tunda uyiga qaytib turadi
	float Speed = 0.f;
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this));
	const bool bNight = GM && (GM->GetDayT() > 0.82f || GM->GetDayT() < 0.2f);
	APawn* Pl = UGameplayStatics::GetPlayerPawn(this, 0);
	const float DPl = Pl ? FVector::Dist2D(Pl->GetActorLocation(), GetActorLocation()) : 1e9f;
	if (!bTalking && DPl > 350.f)
	{
		WanderT -= Dt;
		if (WanderT <= 0.f)
		{
			WanderT = FMath::FRandRange(6.f, 16.f);
			if (bNight) Target = HomePos;
			else { const float A = FMath::FRandRange(0.f, 2.f * PI), R = FMath::FRandRange(150.f, 600.f); Target = HomePos + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0); }
		}
		FVector To = Target - GetActorLocation(); To.Z = 0;
		if (To.Size() > 40.f)
		{
			Speed = 120.f;
			FVector NewPos = GetActorLocation() + To.GetSafeNormal() * Speed * Dt;
			FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtNpcWalk), true, this);
			if (GetWorld()->LineTraceSingleByChannel(Hit, NewPos + FVector(0, 0, 300), NewPos - FVector(0, 0, 300), ECC_Visibility, Q)) NewPos.Z = Hit.ImpactPoint.Z + 92.f;
			// To'siq: oldinda devor bo'lsa maqsadni bekor qilamiz
			if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + To.GetSafeNormal() * 90.f, ECC_Visibility, Q)) { Target = GetActorLocation(); Speed = 0.f; }
			else SetActorLocation(NewPos);
		}
	}
	// Salomlashuv: o'yinchi 4 m ga yaqinlashsa (20 s da bir), or/iymonga qarab
	GreetCD = FMath::Max(0.f, GreetCD - Dt); GreetT = FMath::Max(0.f, GreetT - Dt);
	if (!bTalking && GreetCD <= 0.f && DPl < 400.f && Pl)
	{
		GreetCD = 20.f; GreetT = 3.f;
		const int32 Hn = GM ? GM->GetHonor() : 0;
		FString Key;
		if (Hn >= 10) Key = FMath::RandBool() ? TEXT("greet.high.1") : TEXT("greet.high.2");
		else if (Hn <= -5) Key = FMath::RandBool() ? TEXT("greet.low.1") : TEXT("greet.low.2");
		else if (bWomanNpc && FMath::FRand() < 0.5f) Key = TEXT("greet.woman.1");
		else Key = FString::Printf(TEXT("greet.normal.%d"), FMath::RandRange(1, 3));
		GreetText = FErtLoc::Get().Tr(Key);
		FErtAudio::PlayVo(GetWorld(), Key, 0.9f);
	}
	float TargetYaw = HomeYaw;
	if (Speed > 0.f) TargetYaw = (Target - GetActorLocation()).Rotation().Yaw;
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector To = P->GetActorLocation() - GetActorLocation();
		if (To.Size2D() < 650.f) TargetYaw = To.Rotation().Yaw;
	}
	const FRotator R = GetActorRotation();
	SetActorRotation(FRotator(0, FMath::FixedTurn(R.Yaw, TargetYaw, 120.f * Dt), 0));
	Body->Animate(Dt, Speed, false, false, 0.f, 0.f);
}
