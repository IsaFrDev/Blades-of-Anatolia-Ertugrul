#include "ErtArrow.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ErtCharacter.h"
#include "ErtEnemy.h"
#include "ErtAudio.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AErtArrow::AErtArrow()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AErtArrow::BeginPlay()
{
	Super::BeginPlay();
	Mesh = NewObject<UProceduralMeshComponent>(this, TEXT("ArrowMesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->RegisterComponent();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"))) Mesh->SetMaterial(0, M);
	FErtMeshData D;
	D.AddBox(FVector(0, 0, 0), FVector(38.f, 0.7f, 0.7f), FLinearColor(0.45f, 0.32f, 0.15f));        // dasta
	D.AddBox(FVector(40.f, 0, 0), FVector(4.f, 1.2f, 0.4f), FLinearColor(0.7f, 0.72f, 0.75f));       // uch
	D.AddBox(FVector(-34.f, 0, 0), FVector(5.f, 0.3f, 2.2f), FLinearColor(0.85f, 0.85f, 0.8f));      // pat
	D.AddBox(FVector(-34.f, 0, 0), FVector(5.f, 2.2f, 0.3f), FLinearColor(0.85f, 0.85f, 0.8f));
	D.Commit(Mesh, 0, false);
	SetLifeSpan(14.f);
}

void AErtArrow::Launch(const FVector& Dir, float Speed, float InDamage, bool bFromPlayer, AActor* Shooter)
{
	Vel = Dir.GetSafeNormal() * Speed;
	Damage = InDamage; bPlayer = bFromPlayer; Owner_ = Shooter;
	SetActorRotation(Vel.Rotation());
}

void AErtArrow::Tick(float Dt)
{
	Super::Tick(Dt);
	if (bStuck) return;
	Life += Dt;
	Vel.Z -= 980.f * 0.6f * Dt;   // yengil gravitatsiya (kamon o'qi)
	const FVector From = GetActorLocation();
	const FVector To = From + Vel * Dt;
	// Nishonga tegish: yaqin kapsula
	if (bPlayer)
	{
		TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtEnemy::StaticClass(), All);
		for (AActor* A : All)
		{
			AErtEnemy* E = Cast<AErtEnemy>(A);
			if (!E || E->IsDead()) continue;
			const FVector C = E->GetActorLocation();
			const float DistSeg = FMath::PointDistToSegment(C, From, To);
			if (DistSeg < 70.f)
			{
				E->ApplyHit(Damage, Owner_, false);
				FErtAudio::PlaySfx(GetWorld(), TEXT("arrow_hit"), C, 1.f);
				if (AErtCharacter* H = Cast<AErtCharacter>(Owner_)) if (E->IsDead()) { H->AddXP(E->XPValue()); H->AddGold(E->IsAnimal() ? 0 : FMath::RandRange(4, 14)); }
				Destroy(); return;
			}
		}
	}
	else if (AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (!H->IsDead() && FMath::PointDistToSegment(H->GetActorLocation(), From, To) < 60.f)
		{
			H->ReceiveHit(Damage, From, Cast<AErtEnemy>(Owner_));
			Destroy(); return;
		}
	}
	// Yer / to'siq
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtArrowTrace), false, this);
	if (Owner_) Q.AddIgnoredActor(Owner_);
	if (GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Q))
	{
		SetActorLocation(Hit.ImpactPoint - Vel.GetSafeNormal() * 30.f);
		bStuck = true;
		FErtAudio::PlaySfx(GetWorld(), TEXT("arrow_wall"), Hit.ImpactPoint, 0.7f);
		return;
	}
	SetActorLocation(To);
	SetActorRotation(Vel.Rotation());
	if (Life > 6.f) Destroy();
}
