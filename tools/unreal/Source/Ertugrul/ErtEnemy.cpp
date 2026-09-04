#include "ErtEnemy.h"
#include "Ertugrul.h"
#include "ErtHeroBody.h"
#include "ErtCharacter.h"
#include "ErtProcMesh.h"
#include "ErtHorse.h"
#include "ErtAudio.h"
#include "ErtGameMode.h"
#include "ErtArrow.h"
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
	case EErtEnemyKind::Boss:     MaxHealth = 400.f; AttackDamage = 22.f; AttackCooldown = 1.1f; MoveSpeed = 400.f; AttackRange = 240.f;
		Body->Kaftan = FLinearColor(0.06f, 0.05f, 0.06f); Body->Leather = FLinearColor(0.45f, 0.08f, 0.06f); Body->Trim = FLinearColor(0.95f, 0.8f, 0.2f); Body->Fur = FLinearColor(0.2f, 0.18f, 0.16f); Body->bHelmet = true; break;
	case EErtEnemyKind::Rider:    MaxHealth = 90.f;  AttackDamage = 15.f; AttackCooldown = 1.3f; MoveSpeed = 380.f; AttackRange = 300.f;
		Body->Kaftan = FLinearColor(0.38f, 0.10f, 0.08f); Body->Leather = FLinearColor(0.35f, 0.33f, 0.30f); Body->bHelmet = true; break;
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

void AErtEnemy::ApplyHit(float Damage, AActor* Source, bool bGuardBreak)
{
	if (bDead) return;
	// Qalqonli askarlar (serjant, elita, otliq) yengil zarbani 45% to'sadi - og'ir zarba/tepki yoki gangigan holatda yo'q
	if (!bGuardBreak && StaggerT <= 0.f && HitPending <= 0.f && Source && (Kind == EErtEnemyKind::Sergeant || Kind == EErtEnemyKind::Elite || Kind == EErtEnemyKind::Rider || Kind == EErtEnemyKind::Boss))
	{
		const FVector To = (Source->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (FVector::DotProduct(GetActorForwardVector(), To) > 0.3f && FMath::FRand() < (Kind == EErtEnemyKind::Boss ? 0.6f : 0.45f))
		{
			GuardT = 0.35f; bAlerted = true;
			if (Body && Body->IsBuilt()) Body->SetBlocking(true);
			FErtAudio::PlaySfx(GetWorld(), TEXT("block"), GetActorLocation(), 0.9f, 0.95f);
			return;
		}
	}
	if (Mount) Mount->ApplyDamage(Damage * 0.3f);
	Health -= Damage;
	bAlerted = true;
	if (Body && Body->IsBuilt()) Body->TriggerHurt();
	// Kaltak yeyish: orqaga uchadi, qisqa gangiydi (zarba bera olmaydi)
	if (!Mount && Kind != EErtEnemyKind::Deer && Source)
	{
		const FVector Away = (GetActorLocation() - Source->GetActorLocation()).GetSafeNormal2D();
		LaunchCharacter(Away * FMath::Clamp(Damage * 9.f, 180.f, 420.f) * (Kind == EErtEnemyKind::Boss ? 0.35f : 1.f) + FVector(0, 0, 90.f), true, true);
		StaggerT = FMath::Max(StaggerT, 0.35f);
		HitPending = -1.f;
	}
	if (Kind == EErtEnemyKind::Deer) FleeT = 6.f;
	if (Health <= 0.f) { Killer = Source; Die(); }
}

void AErtEnemy::MountHorse(AErtHorse* H)
{
	if (!H || Mount) return;
	Mount = H;
	H->Mount(this);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);   // o'q/qilich tegishi uchun
	AttachToComponent(H->GetSaddle(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(FVector(0, 0, 58.f));
	SetActorRelativeRotation(FRotator::ZeroRotator);
	AddTickPrerequisiteActor(H);
	if (Body) Body->SetRiding(true);
}

void AErtEnemy::TickRider(float Dt, AErtCharacter* Hero, float DP)
{
	if (!Mount) return;
	if (!bAlerted || !Hero) { Mount->SetRiderInput(FVector2D::ZeroVector, false); return; }
	const FVector To = Hero->GetActorLocation() - Mount->GetActorLocation();
	const float Delta = FMath::FindDeltaAngleDegrees(Mount->GetActorRotation().Yaw, To.Rotation().Yaw);
	FVector2D In;
	In.X = FMath::Clamp(Delta / 35.f, -1.f, 1.f);
	if (DP > 700.f) In.Y = 1.f;
	else if (DP < 260.f) { In.Y = 0.55f; In.X = FMath::Clamp(In.X + 0.8f, -1.f, 1.f); }   // yaqinda aylanib o'tadi
	else In.Y = 0.7f;
	Mount->SetRiderInput(In, DP > 1600.f);
	if (DP <= AttackRange && FMath::Abs(Delta) < 70.f && AttackCD <= 0.f)
	{
		AttackCD = AttackCooldown + FMath::FRandRange(0.f, 0.4f);
		Body->TriggerAttack();
		HitPending = 0.22f;
	}
}

void AErtEnemy::Stagger(float Seconds)
{
	if (bDead) return;
	StaggerT = FMath::Max(StaggerT, Kind == EErtEnemyKind::Boss ? Seconds * 0.5f : Seconds);
	HitPending = -1.f; bHeavyPending = false;
	AttackCD = FMath::Max(AttackCD, Seconds + 0.4f);
	if (Body && Body->IsBuilt()) Body->TriggerHurt();
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
}

void AErtEnemy::Die()
{
	bDead = true;
	if (Kind != EErtEnemyKind::Deer) FErtAudio::PlaySfx(GetWorld(), TEXT("death"), GetActorLocation(), 0.9f, FMath::FRandRange(0.85f, 1.1f));
	if (Mount)
	{
		// Otdan yiqiladi; ot bo'shaydi (o'yinchi minishi mumkin)
		AErtHorse* H = Mount; Mount = nullptr;
		H->Dismount();
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		FHitResult Hit; const FVector Side = H->GetActorLocation() + H->GetActorRightVector() * 120.f;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtFall), false, this); Q.AddIgnoredActor(H);
		FVector Ground = Side;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Side + FVector(0, 0, 300), Side - FVector(0, 0, 600), ECC_Visibility, Q)) Ground = Hit.ImpactPoint;
		SetActorLocation(Ground + FVector(0, 0, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()), false, nullptr, ETeleportType::TeleportPhysics);
		if (Body) Body->SetRiding(false);
	}
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
		if (Mount) Body->Animate(Dt, Mount->GetSpeed(), false, false, 0.f, 0.f);
		else Body->Animate(Dt, CM->Velocity.Size2D(), CM->IsFalling(), false, 0.f, 0.f);
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
	if (GuardT > 0.f) { GuardT -= Dt; if (GuardT <= 0.f && Body) Body->SetBlocking(false); }
	if (StaggerT > 0.f) { StaggerT -= Dt; return; }
	AErtCharacter* Hero = Cast<AErtCharacter>(Player);
	const bool bHeroAlive = Hero && !Hero->IsDead();
	const float DP = bHeroAlive ? FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation()) : 1e9f;

	// Kechiktirilgan zarba (animatsiya o'rtasida tegadi)
	if (HitPending >= 0.f)
	{
		HitPending -= Dt;
		if (HitPending < 0.f && bHeroAlive && DP < AttackRange + 80.f) { Hero->ReceiveHit(bHeavyPending ? AttackDamage * 2.f : AttackDamage, GetActorLocation(), this, bHeavyPending); bHeavyPending = false; }
	}

	if (Mount && bAlerted) { TickRider(Dt, bHeroAlive ? Hero : nullptr, DP); return; }
	if (!bAlerted && bHeroAlive)
	{
		const FVector To = (Hero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float Facing = FVector::DotProduct(GetActorForwardVector(), To);
		const float SeeRange = Mount ? 2600.f : (Hero->bIsCrouched ? 900.f : 1400.f);
		if ((DP < SeeRange && Facing > 0.35f && CanSee(Hero)) || DP < 320.f) bAlerted = true;
	}
	if (bAlerted && bHeroAlive && Kind == EErtEnemyKind::Footman && Health < MaxHealth * 0.3f)
	{
		// Or/iymon yuqori bo'lsa oddiy askarlar qo'rqib qochadi
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))
			if (GM->GetHonor() >= 15) { MoveToward(GetActorLocation() + (GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * 600.f, MoveSpeed); return; }
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
				// Ko'rinadigan o'q: o'yinchining hozirgi joyiga (biroz oldinga) - dodge bilan qochish mumkin
				const FVector Start = GetActorLocation() + FVector(0, 0, 60.f) + GetActorForwardVector() * 40.f;
				const FVector Target = Hero->GetActorLocation() + Hero->GetVelocity() * 0.35f + FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 0);
				const float Dist = FVector::Dist(Start, Target);
				FVector Dir = (Target - Start).GetSafeNormal(); Dir.Z += Dist / 9000.f;   // parabola kompensatsiyasi
				if (AErtArrow* Ar = GetWorld()->SpawnActor<AErtArrow>(AErtArrow::StaticClass(), Start, Dir.Rotation())) Ar->Launch(Dir, 2600.f, AttackDamage, false, this);
				FErtAudio::PlaySfx(GetWorld(), TEXT("bowshot"), GetActorLocation(), 0.8f, 0.9f);
			}
			return;
		}
		if (DP > AttackRange * 0.85f) MoveToward(Hero->GetActorLocation(), MoveSpeed);
		else SetActorRotation(FRotator(0, (Hero->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));
		if (Kind == EErtEnemyKind::Boss) HeavyCD -= Dt;
		if (Kind == EErtEnemyKind::Boss && HeavyCD <= 0.f && DP <= AttackRange + 60.f && AttackCD <= 0.f)
		{
			// Boss: to'sib bo'lmaydigan og'ir zarba (uzun ogohlantirish - dodge bilan qochish kerak)
			HeavyCD = 8.f; AttackCD = 1.6f;
			Body->TriggerAttack(2);
			HitPending = 0.6f; bHeavyPending = true;
			FErtAudio::PlaySfx(GetWorld(), TEXT("swing"), GetActorLocation(), 1.f, 0.7f);
		}
		else if (DP <= AttackRange && AttackCD <= 0.f)
		{
			AttackCD = AttackCooldown + FMath::FRandRange(0.f, 0.4f);
			Body->TriggerAttack();
			FErtAudio::PlaySfx(GetWorld(), TEXT("swing"), GetActorLocation(), 0.7f, FMath::FRandRange(0.85f, 1.05f));
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
