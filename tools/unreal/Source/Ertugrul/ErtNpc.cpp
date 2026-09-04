#include "ErtNpc.h"
#include "ErtHeroBody.h"
#include "ErtLoc.h"
#include "Kismet/GameplayStatics.h"

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
	Body->bWoman = bWoman;
	FRandomStream RS(GetTypeHash(InId));
	Body->Fur = FLinearColor(0.55f + RS.FRand() * 0.35f, 0.5f + RS.FRand() * 0.3f, 0.4f + RS.FRand() * 0.3f);
	Body->Beard = RS.FRand() < 0.35f ? FLinearColor(0.7f, 0.68f, 0.64f) : FLinearColor(0.16f, 0.11f, 0.06f);
	Body->Build(RootComponent, 92.f);
	SetActorRotation(FRotator(0, Yaw, 0));
}

FString AErtNpc::GetDisplayName() const
{
	return FErtLoc::Get().TrOr(LocName, Id);
}

void AErtNpc::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Body || !Body->IsBuilt()) return;
	float TargetYaw = HomeYaw;
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const FVector To = P->GetActorLocation() - GetActorLocation();
		if (To.Size2D() < 650.f) TargetYaw = To.Rotation().Yaw;
	}
	const FRotator R = GetActorRotation();
	SetActorRotation(FRotator(0, FMath::FixedTurn(R.Yaw, TargetYaw, 120.f * Dt), 0));
	Body->Animate(Dt, 0.f, false, false, 0.f, 0.f);
}
