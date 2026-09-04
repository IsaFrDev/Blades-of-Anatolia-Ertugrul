#include "ErtEnemy.h"
#include "Ertugrul.h"
#include "ErtHeroBody.h"
#include "ErtCharacter.h"
#include "ErtProcMesh.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"

AErtEnemy::AErtEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* CM = GetCharacterMovement();
	CM->bOrientRotationToMovement = true;
	CM->RotationRate = FRotator(0, 420.f, 0);
	CM->bRunPhysicsWithNoController = true;   // kontrollersiz ham harakat/gravitatsiya
	CM->MaxWalkSpeed = 380.f;
	CM->GravityScale = 1.5f;
	CM->SetWalkableFloorAngle(48.f);
	CM->MaxStepHeight = 42.f;
	CM->bUseRVOAvoidance = false;
	if (USkeletalMeshComponent* SK = GetMesh()) SK->SetVisibility(false);
	Body = CreateDefaultSubobject<UErtHeroBody>(TEXT("Body"));
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void AErtEnemy::Init(EErtEnemyKind InKind, const FVector& Home, float PatrolRadius)
{
	Kind = InKind; HomePos = Home; Patrol = PatrolRadius; bInit = true;
	switch (Kind)
	{
	case EErtEnemyKind::Footman:  MaxHealth = 60.f;  AttackDamage = 10.f; AttackCooldown = 1.5f; MoveSpeed = 370.f; AttackRange = 200.f;
		Body->Kaftan = FLinearColor(0.12f, 0.22f, 0.12f); Body->Trim = FLinearColor(0.5f, 0.5f, 0.45f); Body->bHelmet = true; break;
	case EErtEnemyKind::Sergeant: MaxHealth = 95.f;  AttackDamage = 14.f; AttackCooldown = 1.4f; MoveSpeed = 390.f; AttackRange = 210.f;
		Body->Kaftan = FLinearColor(0.10f, 0.13f, 0.32f); Body->Leather = FLinearColor(0.5f, 0.5f, 0.52f); Body->bHelmet = true; break;
	case EErtEnemyKind::Crossbow: MaxHealth = 50.f;  AttackDamage = 9.f;  AttackCooldown = 2.6f; MoveSpeed = 330.f; AttackRange = 1600.f;
		Body->Kaftan = FLinearColor(0.30f, 0.22f, 0.12f); Body->bHelmet = false; Body->Fur = FLinearColor(0.35f, 0.3f, 0.25f); break;
	case EErtEnemyKind::Elite:    MaxHealth = 170.f; AttackDamage = 18.f; AttackCooldown = 1.2f; MoveSpeed = 430.f; AttackRange = 220.f;
		Body->Kaftan = FLinearColor(0.08f, 0.06f, 0.07f); Body->Leather = FLinearColor(0.55f, 0.08f, 0.06f); Body->Trim = FLinearColor(0.9f, 0.75f, 0.2f); Body->bHelmet = true; break;
	case EErtEnemyKind::Deer:     MaxHealth = 30.f;  MoveSpeed = 140.f; break;
	}
	Body->bSwordInHand = Kind != EErtEnemyKind::Deer && Kind != EErtEnemyKind::Crossbow;
	Health = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	WanderTarget = HomePos;
	WanderT = FMath::FRandRange(0.5f, 2.f);
	if (Kind == EErtEnemyKind::Deer) { GetCapsuleComponent()->SetCapsuleSize(45.f, 70.f); BuildDeer(); }
	else Body->Build(GetCapsuleComponent(), GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
}

void AErtEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (!bInit) Init(EErtEnemyKind::Footman, GetActorLocation(), 0.f);
}

void AErtEnemy::BuildDeer()
{
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	const FLinearColor Coat(0.55f, 0.36f, 0.18f), Belly(0.75f, 0.62f, 0.45f), Dark(0.25f, 0.16f, 0.08f);
	const float HH = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	auto Make = [&](const TCHAR* Name, const FVector& Rel)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, MakeUniqueObjectName(this, UProceduralMeshComponent::StaticClass(), Name));
		P->SetupAttachment(GetCapsuleComponent());
		P->SetRelativeLocation(Rel);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->RegisterComponent();
		if (Mat) P->SetMaterial(0, Mat);
		DeerParts.Add(P);
		return P;
	};
	FErtMeshData M;
	// Tana (yerdan 70 sm), bo'yin, bosh, shoxlar, dum
	UProceduralMeshComponent* BodyP = Make(TEXT("DeerBody"), FVector(0, 0, -HH + 75.f));
	M.AddBox(FVector(0, 0, 20), FVector(48, 17, 22), Coat);
	M.AddBox(FVector(0, 0, 4), FVector(44, 15, 8), Belly);
	M.AddBox(FVector(52, 0, 45), FVector(10, 8, 26), Coat, FRotator(-30, 0, 0));
	M.AddBox(FVector(66, 0, 70), FVector(14, 7, 8), Coat);
	M.AddBox(FVector(80, 0, 68), FVector(4, 4, 4), Dark);
	M.AddBox(FVector(62, -6, 84), FVector(1.5f, 1.5f, 14), Dark, FRotator(0, 0, -25));
	M.AddBox(FVector(62, 6, 84), FVector(1.5f, 1.5f, 14), Dark, FRotator(0, 0, 25));
	M.AddBox(FVector(62, -10, 92), FVector(1.2f, 1.2f, 8), Dark, FRotator(20, 0, -60));
	M.AddBox(FVector(62, 10, 92), FVector(1.2f, 1.2f, 8), Dark, FRotator(20, 0, 60));
	M.AddBox(FVector(-50, 0, 32), FVector(5, 4, 8), Belly);
	M.Commit(BodyP, 0, false);
	for (int32 i = 0; i < 4; ++i)
	{
		const float X = (i < 2) ? 36.f : -36.f, Y = (i & 1) ? 11.f : -11.f;
		UProceduralMeshComponent* Leg = Make(TEXT("DeerLeg"), FVector(X, Y, -HH + 78.f));
		M.Reset();
		M.AddBox(FVector(0, 0, -38), FVector(5, 4.5f, 38), i < 2 ? Coat : Coat * 0.95f);
		M.AddBox(FVector(1, 0, -78), FVector(6, 5, 3), Dark);
		M.Commit(Leg, 0, false);
	}
}

void AErtEnemy::ApplyHit(float Damage, AActor* Source)
{
	if (bDead) return;
	Health -= Damage;
	bAlerted = true;
	if (Body && Body->IsBuilt()) Body->TriggerHurt();
	if (Kind == EErtEnemyKind::Deer) FleeT = 6.f;
	if (Health <= 0.f) { Killer = Source; Die(); }
}

void AErtEnemy::Stagger(float Seconds)
{
	if (bDead) return;
	StaggerT = FMath::Max(StaggerT, Seconds);
	HitPending = -1.f;
	AttackCD = FMath::Max(AttackCD, Seconds + 0.4f);
	if (Body && Body->IsBuilt()) Body->TriggerHurt();
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
}

void AErtEnemy::Die()
{
	bDead = true;
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	if (Body && Body->IsBuilt()) Body->SetDead(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	for (UProceduralMeshComponent* P : DeerParts) if (P) { P->SetRelativeRotation(FRotator(0, 0, 80.f)); P->AddRelativeLocation(FVector(0, 0, -30.f)); }
	SetLifeSpan(30.f);
}

bool AErtEnemy::CanSee(const APawn* Player) const
{
	FHitResult H;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtSee), false, this);
	Q.AddIgnoredActor(Player);
	const FVector A = GetActorLocation() + FVector(0, 0, 60), B = Player->GetActorLocation() + FVector(0, 0, 40);
	return !GetWorld()->LineTraceSingleByChannel(H, A, B, ECC_Visibility, Q);
}

void AErtEnemy::MoveToward(const FVector& Target, float Speed)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	const FVector D = (Target - GetActorLocation()).GetSafeNormal2D();
	if (!D.IsNearlyZero()) AddMovementInput(D, 1.f);
}

void AErtEnemy::Tick(float Dt)
{
	Super::Tick(Dt);
	if (bDead) return;
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Kind == EErtEnemyKind::Deer) TickDeer(Dt, Player);
	else TickGuard(Dt, Player);
	if (Body && Body->IsBuilt())
	{
		const UCharacterMovementComponent* CM = GetCharacterMovement();
		Body->Animate(Dt, CM->Velocity.Size2D(), CM->IsFalling(), false, 0.f, 0.f);
	}
}

void AErtEnemy::TickDeer(float Dt, APawn* Player)
{
	const float Speed = GetCharacterMovement()->Velocity.Size2D();
	DeerPhase += Dt * (0.8f + Speed / 120.f) * 2.f * PI;
	for (int32 i = 1; i < DeerParts.Num() && i <= 4; ++i)
	{
		const float Sg = ((i == 1) || (i == 4)) ? 1.f : -1.f;
		DeerParts[i]->SetRelativeRotation(FRotator(FMath::Sin(DeerPhase) * Sg * FMath::Min(30.f, Speed * 0.08f + 2.f), 0, 0));
	}
	const float DP = Player ? FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) : 1e9f;
	if (DP < 1500.f) FleeT = FMath::Max(FleeT, 3.f);
	FleeT -= Dt;
	if (FleeT > 0.f && Player)
	{
		const FVector Away = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
		MoveToward(GetActorLocation() + Away * 800.f, 640.f);
		return;
	}
	WanderT -= Dt;
	if (WanderT <= 0.f)
	{
		WanderT = FMath::FRandRange(3.f, 7.f);
		const float A = FMath::FRandRange(0.f, 2.f * PI), R = FMath::FRandRange(0.f, FMath::Max(300.f, Patrol));
		WanderTarget = HomePos + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0);
	}
	if (FVector::Dist2D(WanderTarget, GetActorLocation()) > 80.f) MoveToward(WanderTarget, 140.f);
}

void AErtEnemy::TickGuard(float Dt, APawn* Player)
{
	AttackCD -= Dt;
	if (StaggerT > 0.f) { StaggerT -= Dt; return; }
	AErtCharacter* Hero = Cast<AErtCharacter>(Player);
	const bool bHeroAlive = Hero && !Hero->IsDead();
	const float DP = bHeroAlive ? FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation()) : 1e9f;

	// Kechiktirilgan zarba (animatsiya o'rtasida tegadi)
	if (HitPending >= 0.f)
	{
		HitPending -= Dt;
		if (HitPending < 0.f && bHeroAlive && DP < AttackRange + 60.f) Hero->ReceiveHit(AttackDamage, GetActorLocation(), this);
	}

	if (!bAlerted && bHeroAlive)
	{
		const FVector To = (Hero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float Facing = FVector::DotProduct(GetActorForwardVector(), To);
		const float SeeRange = Hero->bIsCrouched ? 900.f : 1400.f;
		if ((DP < SeeRange && Facing > 0.35f && CanSee(Hero)) || DP < 320.f) bAlerted = true;
	}
	if (bAlerted && bHeroAlive)
	{
		if (Kind == EErtEnemyKind::Crossbow)
		{
			if (DP > AttackRange) MoveToward(Hero->GetActorLocation(), MoveSpeed);
			else if (DP < 500.f) MoveToward(GetActorLocation() + (GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * 400.f, MoveSpeed * 0.8f);
			else SetActorRotation(FRotator(0, (Hero->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));
			if (DP <= AttackRange && AttackCD <= 0.f && CanSee(Hero))
			{
				AttackCD = AttackCooldown;
				Body->TriggerAttack();
				if (FMath::FRand() < 0.65f) Hero->ReceiveHit(AttackDamage, GetActorLocation(), this);
			}
			return;
		}
		if (DP > AttackRange * 0.85f) MoveToward(Hero->GetActorLocation(), MoveSpeed);
		else SetActorRotation(FRotator(0, (Hero->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));
		if (DP <= AttackRange && AttackCD <= 0.f)
		{
			AttackCD = AttackCooldown + FMath::FRandRange(0.f, 0.4f);
			Body->TriggerAttack();
			HitPending = 0.22f;
		}
		return;
	}
	// Patrul / qorovul
	if (Patrol > 0.f)
	{
		WanderT -= Dt;
		if (WanderT <= 0.f)
		{
			WanderT = FMath::FRandRange(2.f, 5.f);
			const float A = FMath::FRandRange(0.f, 2.f * PI), R = FMath::FRandRange(0.3f, 1.f) * Patrol;
			WanderTarget = HomePos + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0);
		}
		if (FVector::Dist2D(WanderTarget, GetActorLocation()) > 90.f) MoveToward(WanderTarget, 150.f);
	}
}
