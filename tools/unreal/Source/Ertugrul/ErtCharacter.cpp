#include "ErtCharacter.h"
#include "Ertugrul.h"
#include "ErtHeroBody.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "UnrealClient.h"
#include "ErtEnemy.h"
#include "ErtGameMode.h"
#include "ErtWorldBuilder.h"
#include "ErtFootsteps.h"
#include "ErtHorse.h"
#include "ErtNpc.h"
#include "ErtAudio.h"
#include "ErtArrow.h"
#include "ErtLoot.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AErtCharacter::AErtCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* CM = GetCharacterMovement();
	CM->bOrientRotationToMovement = true;
	CM->RotationRate = FRotator(0.f, 540.f, 0.f);
	CM->MaxWalkSpeed = JogSpeed;
	CM->MaxWalkSpeedCrouched = CrouchSpeed;
	CM->JumpZVelocity = 460.f;
	CM->AirControl = 0.3f;
	CM->GravityScale = 1.5f;
	CM->BrakingDecelerationWalking = 1600.f;
	CM->MaxAcceleration = 1400.f;
	CM->GroundFriction = 7.f;
	CM->SetWalkableFloorAngle(48.f);
	CM->MaxStepHeight = 42.f;
	CM->PerchRadiusThreshold = 20.f;
	CM->bUseSeparateBrakingFriction = true;
	CM->BrakingFriction = 5.f;
	CM->GetNavAgentPropertiesRef().bCanCrouch = true;
	CM->SetCrouchedHalfHeight(62.f);

	if (USkeletalMeshComponent* SK = GetMesh()) SK->SetVisibility(false);

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	Boom->TargetArmLength = TargetArm;
	Boom->SocketOffset = FVector(0, 45.f, 60.f);
	Boom->bUsePawnControlRotation = true;
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = 12.f;
	Boom->bEnableCameraRotationLag = true;
	Boom->CameraRotationLagSpeed = 18.f;
	Boom->bDoCollisionTest = true;
	Boom->ProbeSize = 14.f;

	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Cam->bUsePawnControlRotation = false;
	Cam->FieldOfView = 78.f;

	Body = CreateDefaultSubobject<UErtHeroBody>(TEXT("Body"));
	Body->bSwordInHand = true;
	Footsteps = CreateDefaultSubobject<UErtFootsteps>(TEXT("Footsteps"));
}

// ---------------- Suzish va qadamlar ----------------

void AErtCharacter::UpdateSwim(float Dt)
{
	if (!WorldRef) WorldRef = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(this, AErtWorldBuilder::StaticClass()));
	if (!WorldRef || bDead || bMantling) return;
	const FVector L = GetActorLocation();
	float Surf = 0.f;
	const bool bWater = WorldRef->IsWater(L.Y / 100.f, L.X / 100.f, Surf);
	const float SurfZ = Surf * 100.f;
	const float FeetZ = L.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (!bSwimming && bWater && FeetZ < SurfZ - 105.f)
	{
		bSwimming = true;
		if (bIsCrouched) UnCrouch();
		CM->SetMovementMode(MOVE_Flying);
		CM->MaxFlySpeed = 240.f;
		CM->BrakingDecelerationFlying = 450.f;
		if (Body) Body->SetSwimming(true);
		if (Footsteps) Footsteps->Splash(FVector(L.X, L.Y, SurfZ));
		UE_LOG(LogErtugrul, Log, TEXT("Suzish boshlandi (suv sathi %.0f, oyoq %.0f)"), SurfZ, FeetZ);
	}
	else if (bSwimming && (!bWater || FeetZ > SurfZ - 70.f))
	{
		bSwimming = false;
		CM->SetMovementMode(MOVE_Walking);
		if (Body) Body->SetSwimming(false);
	}
	if (bSwimming)
	{
		// Suzuvchanlik: kapsula markazi suv sathidan 35 sm past; qirg'oqqa yaqinlashganda yuqoriga
		FVector V = CM->Velocity;
		V.Z = (SurfZ - 35.f - L.Z) * 4.f;
		CM->Velocity = V;
		CM->MaxFlySpeed = bWantSprint && Stamina > 1.f ? 330.f : 240.f;
		Stamina = FMath::Max(0.f, Stamina - (bWantSprint ? 9.f : 3.f) * Dt);
		if (Stamina <= 0.f) { Health = FMath::Max(1.f, Health - 4.f * Dt); HurtFlash = 0.4f; }
	}
}

void AErtCharacter::UpdateSteps(float Dt)
{
	const UCharacterMovementComponent* CM = GetCharacterMovement();
	if (!Footsteps || bSwimming || bDead || !CM->IsMovingOnGround()) { return; }
	const float Speed = CM->Velocity.Size2D();
	if (Speed < 20.f) { StepDist = 0.f; return; }
	StepDist += Speed * Dt;
	const float Stride = (bIsCrouched ? 70.f : 95.f) + Speed * 0.22f;   // sm: sekin - qisqa, chopish - uzun
	if (StepDist >= Stride)
	{
		StepDist -= Stride;
		StepFoot ^= 1;
		const FVector L = GetActorLocation();
		const FVector Foot = L - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 2.f) + GetActorRightVector() * (StepFoot ? 12.f : -12.f);
		const bool bSand = AErtWorldBuilder::IsDesert(L.Y / 100.f, L.X / 100.f);
		Footsteps->Step(Foot, bSand, FMath::Clamp(Speed / 400.f, 0.4f, 1.6f) * (bIsCrouched ? 0.4f : 1.f));
	}
}

// ---------------- Jang ----------------

void AErtCharacter::OnAttackPressed()
{
	if (!bInputEnabled || bMantling || bDead) return;
	if (AErtGameMode* GMw = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) if (GMw->Activity == 2) { GMw->WrestlePress(); return; }
	AttackHoldT = 0.f; bHeavyDone = false;
}

void AErtCharacter::OnAttackReleased()
{
	if (AttackHoldT < 0.f) return;
	const bool bHeavy = bHeavyDone;
	AttackHoldT = -1.f;
	if (bHeavy) return;                       // og'ir zarba ushlab turganda allaqachon berildi
	OnAttack();
}

void AErtCharacter::OnKick()
{
	if (!bInputEnabled || bMantling || bDead || AttackCD > 0.f || Horse) return;
	AttackCD = 0.9f;
	Stamina = FMath::Max(0.f, Stamina - 8.f);
	DoAttack(3, 0.3f, true, 0.9f, 420.f);
}

void AErtCharacter::OnAttack()
{
	if (!bInputEnabled || bMantling || bDead || AttackCD > 0.f) return;
	// Seriya: oyna ichida bosilsa keyingi zarba (0 o'ng, 1 chap, 2 yakunlovchi kuchli)
	if (ComboWindowT <= 0.f) ComboStep = 0;
	const int32 Step = ComboStep;
	ComboStep = (ComboStep + 1) % 3;
	ComboWindowT = 1.1f;
	AttackCD = Step == 2 ? 0.75f : 0.5f;
	Stamina = FMath::Max(0.f, Stamina - 6.f);
	DoAttack(Step == 1 ? 1 : (Step == 2 ? 2 : 0), Step == 2 ? 1.6f : 1.f, Step == 2, Step == 2 ? 0.6f : 0.f, Step == 2 ? 380.f : 0.f);
}

void AErtCharacter::DoAttack(int32 Kind, float DamageMul, bool bGuardBreak, float StaggerSec, float Knock)
{
	if (Body) Body->TriggerAttack(Kind);
	if (LockTarget && !LockTarget->IsDead()) SetActorRotation(FRotator(0, (LockTarget->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));
	FErtAudio::PlaySfx(GetWorld(), TEXT("swing"), GetActorLocation(), Kind == 2 ? 1.f : 0.8f, Kind == 2 ? 0.8f : FMath::FRandRange(0.9f, 1.1f));
	const FVector C = GetActorLocation() + GetActorForwardVector() * 130.f + FVector(0, 0, Horse ? 0.f : 45.f);
	TArray<FOverlapResult> Hits;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtAttack), false, this);
	if (GetWorld()->OverlapMultiByChannel(Hits, C, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Kind == 3 ? 130.f : 150.f), Q))
	{
		TSet<AActor*> Done;
		for (const FOverlapResult& R : Hits)
		{
			AErtEnemy* E = Cast<AErtEnemy>(R.GetActor());
			if (!E || Done.Contains(E)) continue;
			Done.Add(E);
			// IJRO: gangigan yoki holdan toygan (<25%) raqib - bir zarbda
			const bool bExecute = Kind != 3 && ((E->IsStaggered() && !E->IsBoss()) || E->GetHealth() < E->GetMaxHealth() * E->ExecuteThreshold());
			if (bExecute) { ExecuteFlash = 1.f; if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->Rumble(0.9f, 0.25f); }
			const float Dmg = bExecute ? 999.f : AttackDamage * DamageMul * (RiposteT > 0.f ? 2.f : 1.f);
			const float HpBefore = E->GetHealth();
			E->ApplyHit(Dmg, this, bGuardBreak || bExecute);
			const bool bHit = E->GetHealth() < HpBefore || E->IsDead();
			if (!bHit) { ShakeT = FMath::Max(ShakeT, 0.08f); continue; }   // to'sildi
			if (StaggerSec > 0.f && !E->IsDead()) E->Stagger(StaggerSec);
			if (Knock > 0.f && !E->IsDead()) E->LaunchCharacter((E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * Knock + FVector(0, 0, 120.f), true, true);
			ShakeT = FMath::Max(ShakeT, bExecute ? 0.3f : (Kind == 2 ? 0.2f : 0.12f));
			if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->HitStop(bExecute ? 0.22f : (Kind == 2 ? 0.12f : 0.07f));
			FErtAudio::PlaySfx(GetWorld(), E->IsDead() ? TEXT("kill") : TEXT("hit"), E->GetActorLocation(), 1.f, FMath::FRandRange(0.9f, 1.1f));
			if (E->IsDead()) { AddXP(E->XPValue()); /* oltin o'ljadan */ }
			RiposteT = 0.f;
		}
	}
}

void AErtCharacter::OnBlockOn() { if (!bDead) { bBlocking = true; BlockT = 0.f; if (Body) Body->SetBlocking(true); } }
void AErtCharacter::OnBlockOff() { bBlocking = false; if (Body) Body->SetBlocking(false); }

void AErtCharacter::OnInteract()
{
	if (!bInputEnabled || bDead) return;
	if (Horse) { DismountHorse(); return; }
	if (AErtLoot* Lt = NearestLoot(260.f))
	{
		const FString Desc = Lt->Describe();
		Lt->GiveTo(this);
		FErtAudio::PlaySfx(GetWorld(), TEXT("block"), GetActorLocation(), 0.45f, 1.5f);
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->ShopMsg = TEXT("O'lja: ") + Desc; GM->ShopMsgT = 3.f; }
		return;
	}
	if (AErtEnemy* Cc = NearestCarcass(260.f))
	{
		Cc->bLooted = true; Meat += 2; AddXP(5);
		FErtAudio::PlaySfx(GetWorld(), TEXT("block"), GetActorLocation(), 0.4f, 0.8f);
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->ShopMsg = TEXT("Kiyik go'shti +2 (H bilan yeyiladi, +25)"); GM->ShopMsgT = 3.f; }
		return;
	}
	if (AErtNpc* N = NearestNpc(280.f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->StartDialog(N); return; }
	if (AErtHorse* H = NearestHorse(320.f)) MountHorse(H);
}

AErtLoot* AErtCharacter::NearestLoot(float MaxDist) const
{
	AErtLoot* Best = nullptr; float BestD = MaxDist;
	TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtLoot::StaticClass(), All);
	for (AActor* A : All) { const float D = FVector::Dist2D(A->GetActorLocation(), GetActorLocation()); if (D < BestD) { BestD = D; Best = Cast<AErtLoot>(A); } }
	return Best;
}

AErtEnemy* AErtCharacter::NearestCarcass(float MaxDist) const
{
	AErtEnemy* Best = nullptr; float BestD = MaxDist;
	TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtEnemy::StaticClass(), All);
	for (AActor* A : All)
	{
		AErtEnemy* E = Cast<AErtEnemy>(A);
		if (!E || !E->IsAnimal() || !E->IsDead() || E->bLooted) continue;
		const float D = FVector::Dist2D(E->GetActorLocation(), GetActorLocation());
		if (D < BestD) { BestD = D; Best = E; }
	}
	return Best;
}

AErtNpc* AErtCharacter::NearestNpc(float MaxDist) const
{
	AErtNpc* Best = nullptr; float BestD = MaxDist;
	TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtNpc::StaticClass(), All);
	for (AActor* A : All)
	{
		const float D = FVector::Dist2D(A->GetActorLocation(), GetActorLocation());
		if (D < BestD) { BestD = D; Best = Cast<AErtNpc>(A); }
	}
	return Best;
}

AErtHorse* AErtCharacter::NearestHorse(float MaxDist) const
{
	AErtHorse* Best = nullptr; float BestD = MaxDist;
	TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtHorse::StaticClass(), All);
	for (AActor* A : All)
	{
		AErtHorse* H = Cast<AErtHorse>(A);
		if (!H || H->IsMounted() || H->IsDead()) continue;
		const float D = FVector::Dist2D(H->GetActorLocation(), GetActorLocation());
		if (D < BestD) { BestD = D; Best = H; }
	}
	return Best;
}

void AErtCharacter::MountHorse(AErtHorse* H)
{
	if (!H || Horse || bSwimming || bMantling) return;
	Horse = H;
	H->Mount(this);
	if (bIsCrouched) UnCrouch();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToComponent(H->GetSaddle(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	AddTickPrerequisiteActor(H);
	SetActorRelativeLocation(FVector(0, 0, 58.f));
	SetActorRelativeRotation(FRotator::ZeroRotator);
	if (Body) Body->SetRiding(true);
}

void AErtCharacter::DismountHorse()
{
	if (!Horse) return;
	AErtHorse* H = Horse;
	Horse = nullptr;
	H->Dismount();
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorRotation(FRotator(0, H->GetActorRotation().Yaw, 0));
	SetActorLocation(H->GetActorLocation() + H->GetActorRightVector() * -130.f + FVector(0, 0, 10.f), false, nullptr, ETeleportType::TeleportPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (Body) Body->SetRiding(false);
}

void AErtCharacter::OnShoot()
{
	if (!bInputEnabled || bMantling || bDead || ShootCD > 0.f || Arrows <= 0) return;
	ShootCD = 0.9f;
	--Arrows;
	if (Body) Body->TriggerAttack();
	FErtAudio::PlaySfx(GetWorld(), TEXT("bowshot"), GetActorLocation(), 0.9f);
	const FVector A = Cam->GetComponentLocation();
	FVector Dir = Cam->GetForwardVector();
	// Nishon: kamera nuri tekkan nuqta (yoki 60 m), o'q qo'ldan uchadi
	FHitResult H;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtArrowAim), false, this);
	FVector Target = A + Dir * 6000.f;
	if (GetWorld()->LineTraceSingleByChannel(H, A, A + Dir * 6000.f, ECC_Pawn, Q)) Target = H.ImpactPoint;
	else if (GetWorld()->LineTraceSingleByChannel(H, A, A + Dir * 6000.f, ECC_Visibility, Q)) Target = H.ImpactPoint;
	const FVector Start = GetActorLocation() + GetActorForwardVector() * 50.f + FVector(0, 0, 40.f);
	FVector D2 = (Target - Start).GetSafeNormal(); D2.Z += FVector::Dist(Start, Target) / 14000.f;
	if (AErtArrow* Ar = GetWorld()->SpawnActor<AErtArrow>(AErtArrow::StaticClass(), Start, D2.Rotation())) Ar->Launch(D2, 4200.f, ArrowDamage, true, this);
}

void AErtCharacter::ReceiveHit(float Damage, const FVector& From, AErtEnemy* Attacker, bool bUnblockable)
{
	if (bDead) return;
	if (DodgeT > 0.2f) return;   // dodge daxlsizligi
	const FVector To = (From - GetActorLocation()).GetSafeNormal2D();
	const bool bFacing = FVector::DotProduct(GetActorForwardVector(), To) > 0.2f;
	if (bBlocking && bFacing && BlockT < 0.25f && !bUnblockable)
	{
		// PARRY: zarar yo'q, raqib gangiydi, keyingi zarba ikki baravar
		ParryFlash = 1.f; RiposteT = 1.6f;
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->Rumble(0.5f, 0.15f);
		FErtAudio::PlaySfx(GetWorld(), TEXT("parry"), GetActorLocation(), 1.f);
		Stamina = FMath::Min(StaminaMax, Stamina + 6.f);
		if (Attacker) Attacker->Stagger(1.3f);
		if (Body) Body->TriggerParry();
		if (Footsteps) Footsteps->Step(GetActorLocation() - FVector(0, 0, 60.f), false, 1.2f);
		return;
	}
	if (bBlocking && bFacing && Stamina > 5.f && !bUnblockable) { Damage *= bShield ? 0.05f : 0.2f; Stamina = FMath::Max(0.f, Stamina - (bShield ? 6.f : 12.f)); FErtAudio::PlaySfx(GetWorld(), TEXT("block"), GetActorLocation(), 0.9f); }
	else FErtAudio::PlaySfx(GetWorld(), TEXT("hit"), GetActorLocation(), 0.8f, 0.9f);
	if (bPeltArmor) Damage *= 0.85f;   // bo'ri terisi zirhi
	if (Horse) { Horse->ApplyDamage(Damage * 0.4f); Damage *= 0.6f; }
	Health -= Damage;
	HurtFlash = 1.f;
	ShakeT = FMath::Min(0.35f, 0.15f + Damage * 0.01f);
	if (!Horse && !bSwimming && !bMantling) LaunchCharacter(-To * 220.f + FVector(0, 0, 60.f), false, false);
	if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->Rumble(FMath::Clamp(Damage / 20.f, 0.3f, 1.f), 0.3f);
	NoDamageT = 0.f;
	if (Body) Body->TriggerHurt();
	if (Health <= 0.f)
	{
		Health = 0.f; bDead = true;
		FErtAudio::PlaySfx(GetWorld(), TEXT("death"), GetActorLocation(), 1.f);
		GetCharacterMovement()->DisableMovement();
		if (Body) Body->SetDead(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	}
}

void AErtCharacter::ResetAt(const FVector& Pos, float Yaw)
{
	if (Horse) DismountHorse();
	bDead = false; bMantling = false; bBlocking = false;
	Health = MaxHealth; Stamina = StaminaMax; HurtFlash = 0.f;
	Arrows = FMath::Max(Arrows, 8);
	SetActorHiddenInGame(false);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	SetActorLocation(Pos, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0, Yaw, 0));
	if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-12.f, Yaw, 0.f));
	Boom->bDoCollisionTest = true;
	Boom->bEnableCameraLag = true;
	// Yiqilgan tana pozasi qayta tiklanmaydi - komponentni qayta quramiz
	if (Body && Body->IsBuilt())
	{
		TArray<USceneComponent*> Kids;
		GetCapsuleComponent()->GetChildrenComponents(true, Kids);
		for (USceneComponent* K : Kids) if (K && K != Boom && K != Cam && !K->IsA<USpringArmComponent>() && !K->IsA<UCameraComponent>()) K->DestroyComponent();
		Body->DestroyComponent();
		Body = NewObject<UErtHeroBody>(this, TEXT("BodyR"));
		Body->bSwordInHand = true;
		Body->RegisterComponent();
		Body->Build(GetCapsuleComponent(), GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	}
	ApplyEquipment();
}

void AErtCharacter::UpdateCombat(float Dt)
{
	AttackCD = FMath::Max(0.f, AttackCD - Dt);
	ShootCD = FMath::Max(0.f, ShootCD - Dt);
	HurtFlash = FMath::Max(0.f, HurtFlash - Dt * 2.f);
	ParryFlash = FMath::Max(0.f, ParryFlash - Dt * 1.6f);
	LevelFlash = FMath::Max(0.f, LevelFlash - Dt * 0.6f);
	ExecuteFlash = FMath::Max(0.f, ExecuteFlash - Dt * 1.4f);
	RiposteT = FMath::Max(0.f, RiposteT - Dt);
	BlockT += Dt;
	NoDamageT += Dt;
	float RegenMul = 1.f;
	if (AErtGameMode* GMh = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) RegenMul = GMh->GetHonor() >= 10 ? 1.6f : (GMh->GetHonor() <= -10 ? 0.6f : 1.f);
	if (!bDead && NoDamageT > 5.f) Health = FMath::Min(MaxHealth, Health + 4.f * RegenMul * Dt);
}

void AErtCharacter::BeginPlay()
{
	Super::BeginPlay();
	Stamina = StaminaMax;
	if (Body) Body->Build(GetCapsuleComponent(), GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(FRotator(-12.f, GetActorRotation().Yaw, 0.f));
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("-ErtShot="), ShotDir))
	{
		ShotDir = ShotDir.TrimQuotes();
		ShotT = 0.f;
		SetTickableWhenPaused(true);   // sinov ssenariysi pauzada ham davom etsin
		UE_LOG(LogErtugrul, Log, TEXT("Sinov ssenariysi: skrinshotlar -> %s"), *ShotDir);
	}
}

void AErtCharacter::TakeShot(const TCHAR* Name)
{
	const FString File = FString::Printf(TEXT("%s/%02d_%s.png"), *ShotDir, ++ShotIdx, Name);
	FScreenshotRequest::RequestScreenshot(File, true, false);
	UE_LOG(LogErtugrul, Log, TEXT("Skrinshot %s (pos %s, tezlik %.0f)"), *File, *GetActorLocation().ToCompactString(), GetVelocity().Size2D());
}

void AErtCharacter::Teleport(float E, float N, float Z, float Pitch, float Yaw)
{
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	SetActorLocation(FVector(N * 100.f, E * 100.f, Z * 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(Pitch, Yaw, 0.f));
	Boom->bDoCollisionTest = false;
	Boom->bEnableCameraLag = false;
	TargetArm = 250.f;
	SetActorHiddenInGame(true); // havo suratlarida personaj ko'rinishni to'smasin
}

void AErtCharacter::UpdateShotScript(float Dt)
{
	const float T0 = ShotT;
	ShotT += Dt;
	auto At = [&](float T) { return T0 < T && ShotT >= T; };
	DebugMove = FVector2D::ZeroVector;
	if (ShotT > 1.5f && ShotT < 4.5f) DebugMove = FVector2D(0, 1);
	if (ShotT > 4.5f && ShotT < 7.5f) { DebugMove = FVector2D(0, 1); bWantSprint = true; }
	if (At(7.5f)) { bWantSprint = false; }
	if (ShotT > 7.5f && ShotT < 9.0f) DebugMove = FVector2D(0, 1);
	if (At(7.6f)) Jump();
	if (At(9.2f)) Crouch();
	if (ShotT > 9.4f && ShotT < 10.4f) DebugMove = FVector2D(1, 0);
	if (At(10.6f)) UnCrouch();
	if (At(1.4f)) TakeShot(TEXT("idle"));
	if (At(2.2f)) OnAttack();
	if (At(2.42f)) TakeShot(TEXT("attack"));
	if (At(4.0f)) TakeShot(TEXT("jog"));
	if (At(7.0f)) TakeShot(TEXT("sprint"));
	if (At(7.95f)) TakeShot(TEXT("jump"));
	if (At(10.2f)) TakeShot(TEXT("crouch"));
	if (At(11.0f)) Teleport(-560.f, 470.f, 20.f + 45.f, -38.f, 0.f);
	if (At(12.6f)) TakeShot(TEXT("oba"));
	if (At(13.0f)) Teleport(-560.f, 380.f, 20.f + 140.f, -55.f, 0.f);
	if (At(14.6f)) TakeShot(TEXT("oba_air"));
	if (At(15.0f)) Teleport(620.f, 540.f, 232.f + 40.f, -22.f, 0.f);
	if (At(16.6f)) TakeShot(TEXT("fort"));
	if (At(17.0f)) Teleport(-470.f, -720.f, 12.f + 70.f, -28.f, 0.f);
	if (At(18.6f)) TakeShot(TEXT("city"));
	if (At(19.0f)) Teleport(520.f, -700.f, 10.f + 60.f, -28.f, 0.f);
	if (At(20.6f)) TakeShot(TEXT("camp"));
	if (At(21.0f)) Teleport(-150.f, -1100.f, 700.f, -32.f, 0.f);
	if (At(22.6f)) TakeShot(TEXT("world"));
	if (At(23.0f)) Teleport(-820.f, 300.f, 6.f + 25.f, -25.f, 0.f);
	if (At(24.6f)) TakeShot(TEXT("river"));
	if (At(24.7f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->ToggleMap(); }
	if (At(24.9f)) TakeShot(TEXT("map"));
	if (At(24.95f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->ToggleMap(); }
	if (At(25.0f)) Teleport(150.f, -975.f, 6.f + 28.f, -22.f, 0.f);
	if (At(26.6f)) TakeShot(TEXT("oasis"));
	if (At(27.0f)) Teleport(300.f, -975.f, 12.f + 18.f, -14.f, 0.f);
	if (At(28.6f)) TakeShot(TEXT("caravan"));
	if (At(29.0f))
	{
		// Suzish sinovi: ko'lga tushiriladi
		SetActorHiddenInGame(false);
		Boom->bDoCollisionTest = false; TargetArm = 300.f;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		SetActorLocation(FVector(700.f * 100.f, -700.f * 100.f, 6.f * 100.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-18.f, 0.f, 0.f));
	}
	if (ShotT > 29.5f && ShotT < 32.f) DebugMove = FVector2D(0, 1);
	if (At(30.6f)) { if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-8.f, 90.f, 0.f)); TargetArm = 380.f; }
	if (At(31.4f)) TakeShot(TEXT("swim"));
	if (At(32.5f))
	{
		// Ot minish sinovi: otning yoniga qo'yib minamiz
		bSwimming = false; if (Body) Body->SetSwimming(false);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		SetActorLocation(FVector(44400.f, -56000.f, 2200.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-14.f, 0.f, 0.f));
		TargetArm = 520.f;
	}
	if (At(33.2f)) { if (AErtHorse* Hh = NearestHorse(2500.f)) MountHorse(Hh); }
	if (ShotT > 33.5f && ShotT < 37.f) { DebugMove = FVector2D(0, 1); bWantSprint = ShotT > 34.5f; }
	if (At(36.2f)) TakeShot(TEXT("ride"));
	if (At(37.2f)) { bWantSprint = false; DismountHorse(); }
	if (At(38.0f)) TakeShot(TEXT("dismount"));
	if (At(38.6f))
	{
		// NPC dialog sinovi: Hayma Ona (oba u=4, v=-9)
		SetActorLocation(FVector(55250.f, -56500.f, 2200.f), false, nullptr, ETeleportType::TeleportPhysics);
		SetActorRotation(FRotator(0, 0, 0));
		if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-10.f, 20.f, 0.f));
		TargetArm = 320.f;
	}
	if (At(39.0f)) { if (AErtNpc* Np = NearestNpc(2500.f)) SetActorLocation(Np->GetActorLocation() + Np->GetActorForwardVector() * 160.f, false, nullptr, ETeleportType::TeleportPhysics); }
	if (At(39.6f)) OnInteract();
	if (At(40.6f)) TakeShot(TEXT("npc"));
	if (At(41.0f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->OnAdvance(); }
	if (At(41.8f)) TakeShot(TEXT("npc_choice"));
	if (At(42.6f)) { UE_LOG(LogErtugrul, Log, TEXT("Sinov ssenariysi tugadi")); FPlatformMisc::RequestExit(false); }
	if (!DebugMove.IsNearlyZero())
	{
		MoveInput = DebugMove;
		if (Horse) return;
		const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), DebugMove.Y);
		AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y), DebugMove.X);
	}
}

// ---------------- Kirish (Enhanced Input, resurssiz) ----------------

void AErtCharacter::BuildInput()
{
	if (IMC) return;
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this, Name);
		A->ValueType = Type;
		return A;
	};
	IA_Move = MakeAction(TEXT("IA_ErtMove"), EInputActionValueType::Axis2D);
	IA_Look = MakeAction(TEXT("IA_ErtLook"), EInputActionValueType::Axis2D);
	IA_Jump = MakeAction(TEXT("IA_ErtJump"), EInputActionValueType::Boolean);
	IA_Sprint = MakeAction(TEXT("IA_ErtSprint"), EInputActionValueType::Boolean);
	IA_Crouch = MakeAction(TEXT("IA_ErtCrouch"), EInputActionValueType::Boolean);
	IA_Walk = MakeAction(TEXT("IA_ErtWalk"), EInputActionValueType::Boolean);
	IA_Zoom = MakeAction(TEXT("IA_ErtZoom"), EInputActionValueType::Axis1D);
	IA_Attack = MakeAction(TEXT("IA_ErtAttack"), EInputActionValueType::Boolean);
	IA_Block = MakeAction(TEXT("IA_ErtBlock"), EInputActionValueType::Boolean);
	IA_Shoot = MakeAction(TEXT("IA_ErtShoot"), EInputActionValueType::Boolean);
	IA_Menu = MakeAction(TEXT("IA_ErtMenu"), EInputActionValueType::Boolean);
	IA_MenuUp = MakeAction(TEXT("IA_ErtMenuUp"), EInputActionValueType::Boolean);
	IA_MenuDown = MakeAction(TEXT("IA_ErtMenuDown"), EInputActionValueType::Boolean);
	IA_Confirm = MakeAction(TEXT("IA_ErtConfirm"), EInputActionValueType::Boolean);
	IA_Interact = MakeAction(TEXT("IA_ErtInteract"), EInputActionValueType::Boolean);
	IA_Choice1 = MakeAction(TEXT("IA_ErtChoice1"), EInputActionValueType::Boolean);
	IA_Choice2 = MakeAction(TEXT("IA_ErtChoice2"), EInputActionValueType::Boolean);
	IA_Choice3 = MakeAction(TEXT("IA_ErtChoice3"), EInputActionValueType::Boolean);
	IA_Choice4 = MakeAction(TEXT("IA_ErtChoice4"), EInputActionValueType::Boolean);
	IA_MenuLeft = MakeAction(TEXT("IA_ErtMenuLeft"), EInputActionValueType::Boolean);
	IA_MenuRight = MakeAction(TEXT("IA_ErtMenuRight"), EInputActionValueType::Boolean);
	IA_Settings = MakeAction(TEXT("IA_ErtSettings"), EInputActionValueType::Boolean);
	IA_Map = MakeAction(TEXT("IA_ErtMap"), EInputActionValueType::Boolean);
	IA_Lock = MakeAction(TEXT("IA_ErtLock"), EInputActionValueType::Boolean);
	IA_Dodge = MakeAction(TEXT("IA_ErtDodge"), EInputActionValueType::Boolean);
	IA_Inventory = MakeAction(TEXT("IA_ErtInventory"), EInputActionValueType::Boolean); IA_Inventory->bTriggerWhenPaused = true;
	IA_Potion = MakeAction(TEXT("IA_ErtPotion"), EInputActionValueType::Boolean);
	IA_Kick = MakeAction(TEXT("IA_ErtKick"), EInputActionValueType::Boolean);
	// Menyu harakatlari pauzada ham ishlaydi
	for (UInputAction* A : { IA_Menu, IA_MenuUp, IA_MenuDown, IA_Confirm, IA_MenuLeft, IA_MenuRight, IA_Settings, IA_Map, IA_Choice1, IA_Choice2, IA_Choice3, IA_Choice4, IA_Jump }) if (A) A->bTriggerWhenPaused = true;

	IMC = NewObject<UInputMappingContext>(this, TEXT("IMC_Ertugrul"));
	auto Map = [this](UInputAction* A, const FKey& K) -> FEnhancedActionKeyMapping& { return IMC->MapKey(A, K); };
	auto Swizzle = [this]() { return NewObject<UInputModifierSwizzleAxis>(this); };
	auto Negate = [this]() { return NewObject<UInputModifierNegate>(this); };

	// Harakat: X = o'ngga, Y = oldinga
	{ FEnhancedActionKeyMapping& M = Map(IA_Move, EKeys::W); M.Modifiers.Add(Swizzle()); }
	{ FEnhancedActionKeyMapping& M = Map(IA_Move, EKeys::S); M.Modifiers.Add(Swizzle()); M.Modifiers.Add(Negate()); }
	Map(IA_Move, EKeys::D);
	{ FEnhancedActionKeyMapping& M = Map(IA_Move, EKeys::A); M.Modifiers.Add(Negate()); }
	{
		FEnhancedActionKeyMapping& M = Map(IA_Move, EKeys::Gamepad_Left2D);
		UInputModifierDeadZone* DZ = NewObject<UInputModifierDeadZone>(this); DZ->LowerThreshold = 0.2f; M.Modifiers.Add(DZ);
	}
	// Qarash
	{
		FEnhancedActionKeyMapping& M = Map(IA_Look, EKeys::Mouse2D);
		UInputModifierNegate* N = Negate(); N->bX = false; N->bY = true; N->bZ = false; M.Modifiers.Add(N);
	}
	{
		FEnhancedActionKeyMapping& M = Map(IA_Look, EKeys::Gamepad_Right2D);
		UInputModifierDeadZone* DZ = NewObject<UInputModifierDeadZone>(this); DZ->LowerThreshold = 0.25f; M.Modifiers.Add(DZ);
		UInputModifierScalar* Sc = NewObject<UInputModifierScalar>(this); Sc->Scalar = FVector(2.4f, 1.6f, 1.f); M.Modifiers.Add(Sc);
	}
	Map(IA_Jump, EKeys::SpaceBar);
	Map(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	Map(IA_Sprint, EKeys::LeftShift);
	Map(IA_Sprint, EKeys::Gamepad_LeftThumbstick);
	Map(IA_Crouch, EKeys::LeftControl);
	Map(IA_Crouch, EKeys::C);
	Map(IA_Crouch, EKeys::Gamepad_FaceButton_Right);
	Map(IA_Walk, EKeys::LeftAlt);
	Map(IA_Walk, EKeys::CapsLock);
	Map(IA_Zoom, EKeys::MouseWheelAxis);
	Map(IA_Attack, EKeys::LeftMouseButton);
	Map(IA_Attack, EKeys::Gamepad_FaceButton_Left);
	Map(IA_Block, EKeys::RightMouseButton);
	Map(IA_Block, EKeys::Gamepad_LeftShoulder);
	Map(IA_Shoot, EKeys::F);
	Map(IA_Shoot, EKeys::Gamepad_RightShoulder);
	Map(IA_Menu, EKeys::Escape);
	Map(IA_Menu, EKeys::Tab);
	Map(IA_Menu, EKeys::Gamepad_Special_Right);
	Map(IA_MenuUp, EKeys::Up);
	Map(IA_MenuUp, EKeys::Gamepad_DPad_Up);
	Map(IA_MenuDown, EKeys::Down);
	Map(IA_MenuDown, EKeys::Gamepad_DPad_Down);
	Map(IA_Confirm, EKeys::Enter);
	Map(IA_Confirm, EKeys::Gamepad_FaceButton_Bottom);
	Map(IA_Interact, EKeys::E);
	Map(IA_Interact, EKeys::Gamepad_FaceButton_Top);
	Map(IA_Choice1, EKeys::One); Map(IA_Choice2, EKeys::Two); Map(IA_Choice3, EKeys::Three); Map(IA_Choice4, EKeys::Four);
	Map(IA_MenuLeft, EKeys::Left); Map(IA_MenuLeft, EKeys::Gamepad_DPad_Left);
	Map(IA_MenuRight, EKeys::Right); Map(IA_MenuRight, EKeys::Gamepad_DPad_Right);
	Map(IA_Settings, EKeys::O); Map(IA_Settings, EKeys::Gamepad_Special_Left);
	Map(IA_Map, EKeys::M); Map(IA_Map, EKeys::Gamepad_RightThumbstick);
	Map(IA_Lock, EKeys::Q); Map(IA_Lock, EKeys::MiddleMouseButton); Map(IA_Lock, EKeys::Gamepad_LeftThumbstick);
	Map(IA_Dodge, EKeys::X); Map(IA_Dodge, EKeys::Gamepad_FaceButton_Right);
	Map(IA_Inventory, EKeys::I); Map(IA_Inventory, EKeys::Gamepad_DPad_Right);
	Map(IA_Potion, EKeys::H); Map(IA_Potion, EKeys::Gamepad_DPad_Left);
	Map(IA_Kick, EKeys::V); Map(IA_Kick, EKeys::Gamepad_RightTrigger);
}

const TArray<FString>& AErtCharacter::BindableActions()
{
	static const TArray<FString> A = { TEXT("Jump"), TEXT("Sprint"), TEXT("Crouch"), TEXT("Walk"), TEXT("Attack"), TEXT("Block"), TEXT("Shoot"), TEXT("Interact"), TEXT("Dodge"), TEXT("Lock"), TEXT("Kick"), TEXT("Inventory"), TEXT("Potion"), TEXT("Map"), TEXT("Settings") };
	return A;
}

UInputAction* AErtCharacter::ActionByName(const FString& N) const
{
	if (N == TEXT("Jump")) return IA_Jump; if (N == TEXT("Sprint")) return IA_Sprint; if (N == TEXT("Crouch")) return IA_Crouch; if (N == TEXT("Walk")) return IA_Walk;
	if (N == TEXT("Attack")) return IA_Attack; if (N == TEXT("Block")) return IA_Block; if (N == TEXT("Shoot")) return IA_Shoot; if (N == TEXT("Interact")) return IA_Interact;
	if (N == TEXT("Dodge")) return IA_Dodge; if (N == TEXT("Lock")) return IA_Lock; if (N == TEXT("Kick")) return IA_Kick; if (N == TEXT("Inventory")) return IA_Inventory;
	if (N == TEXT("Potion")) return IA_Potion; if (N == TEXT("Map")) return IA_Map; if (N == TEXT("Settings")) return IA_Settings;
	return nullptr;
}

FString AErtCharacter::GetBindingName(const FString& Action) const
{
	if (const FKey* K = Bindings.Find(Action)) return K->GetDisplayName().ToString();
	UInputAction* A = ActionByName(Action);
	if (!A || !IMC) return TEXT("-");
	for (const FEnhancedActionKeyMapping& M : IMC->GetMappings()) if (M.Action == A && !M.Key.IsGamepadKey()) return M.Key.GetDisplayName().ToString();
	return TEXT("-");
}

void AErtCharacter::SetBinding(const FString& Action, const FKey& Key)
{
	if (Key.IsGamepadKey() || !Key.IsValid()) return;
	Bindings.Add(Action, Key);
	ApplyBindings();
}

void AErtCharacter::ApplyBindings()
{
	if (!IMC) return;
	for (const auto& P : Bindings)
	{
		UInputAction* A = ActionByName(P.Key);
		if (!A) continue;
		// Klaviatura/sichqoncha xaritalarini olib tashlab, yangisini qo'yamiz (gamepad qoladi)
		TArray<FKey> Old;
		for (const FEnhancedActionKeyMapping& M : IMC->GetMappings()) if (M.Action == A && !M.Key.IsGamepadKey()) Old.Add(M.Key);
		for (const FKey& K : Old) IMC->UnmapKey(A, K);
		IMC->MapKey(A, P.Value);
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())) Sub->RequestRebuildControlMappings();
}

void AErtCharacter::OnMenuLeft() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (GM->IsSettingsOpen()) GM->SettingsAdjust(-1); } }
void AErtCharacter::OnMenuRight() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (GM->IsSettingsOpen()) GM->SettingsAdjust(1); } }
void AErtCharacter::OnInventory() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->ToggleInventory(); }
void AErtCharacter::OnPotion() { if (bInputEnabled && !bDead) UsePotion(); }

bool AErtCharacter::UsePotion()
{
	if (Health >= MaxHealth) return false;
	if (Potions > 0) { --Potions; Heal(45.f); }
	else if (Meat > 0) { --Meat; Heal(25.f); }
	else return false;
	HurtFlash = 0.f;
	FErtAudio::PlaySfx(GetWorld(), TEXT("block"), GetActorLocation(), 0.5f, 1.4f);
	return true;
}

void AErtCharacter::AddXP(int32 N)
{
	XP += N;
	while (XP >= XPToNext())
	{
		XP -= XPToNext(); ++Level;
		MaxHealth += 10.f; StaminaMax += 5.f; Health = MaxHealth; Stamina = StaminaMax;
		LevelFlash = 1.f;
		ApplyEquipment();
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->Rumble(0.6f, 0.4f);
	}
}

void AErtCharacter::ApplyEquipment()
{
	AttackDamage = 30.f + (Level - 1) * 3.f + (SwordTier >= 2 ? 12.f : 0.f);
	ArrowDamage = 45.f + (Level - 1) * 3.f + (BowTier >= 2 ? 20.f : 0.f);
	MaxArrows = BowTier >= 2 ? 24 : 16;
	if (Body && Body->IsBuilt()) { Body->SetShield(bShield); Body->SetSwordTier(SwordTier); }
}

void AErtCharacter::OnLock()
{
	if (!bInputEnabled || bDead) return;
	if (LockTarget) { LockTarget = nullptr; GetCharacterMovement()->bOrientRotationToMovement = true; return; }
	// Oldindagi eng yaqin tirik dushman (18 m)
	TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtEnemy::StaticClass(), All);
	AErtEnemy* Best = nullptr; float BestScore = 1e9f;
	for (AActor* A : All)
	{
		AErtEnemy* E = Cast<AErtEnemy>(A);
		if (!E || E->IsDead() || E->IsAnimal()) continue;
		const FVector To = E->GetActorLocation() - GetActorLocation();
		const float D = To.Size2D(); if (D > 1800.f) continue;
		const float Facing = FVector::DotProduct(Cam->GetForwardVector().GetSafeNormal2D(), To.GetSafeNormal2D());
		const float Score = D * (1.6f - Facing);
		if (Score < BestScore) { BestScore = Score; Best = E; }
	}
	LockTarget = Best;
	if (LockTarget) GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AErtCharacter::UpdateLock(float Dt)
{
	if (!LockTarget) return;
	if (LockTarget->IsDead() || FVector::Dist2D(LockTarget->GetActorLocation(), GetActorLocation()) > 2600.f || Horse || bSwimming)
	{
		LockTarget = nullptr; GetCharacterMovement()->bOrientRotationToMovement = true; return;
	}
	const FVector To = LockTarget->GetActorLocation() - GetActorLocation();
	const float Yaw = To.Rotation().Yaw;
	// Personaj nishonga qaraydi (strafe), kamera yumshoq ergashadi
	SetActorRotation(FRotator(0, FMath::FixedTurn(GetActorRotation().Yaw, Yaw, 540.f * Dt), 0));
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		const FRotator C = PC->GetControlRotation();
		PC->SetControlRotation(FRotator(FMath::FInterpTo(C.Pitch, -14.f, Dt, 2.f), FMath::FixedTurn(C.Yaw, Yaw, 200.f * Dt), 0));
	}
}

void AErtCharacter::OnDodge()
{
	if (!bInputEnabled || bDead || bMantling || Horse || bSwimming || DodgeT > 0.f || Stamina < 8.f) return;
	if (!GetCharacterMovement()->IsMovingOnGround()) return;
	DodgeT = 0.5f;
	Stamina = FMath::Max(0.f, Stamina - 12.f);
	FVector Dir = GetLastMovementInputVector().GetSafeNormal2D();
	if (Dir.IsNearlyZero()) Dir = -GetActorForwardVector();   // kirish bo'lmasa orqaga
	LaunchCharacter(Dir * 620.f + FVector(0, 0, 140.f), true, true);
	if (bIsCrouched) UnCrouch();
	FErtAudio::PlaySfx(GetWorld(), TEXT("swing"), GetActorLocation(), 0.4f, 1.3f);
}

void AErtCharacter::OnMap() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->ToggleMap(); }
void AErtCharacter::OnSettings() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (!GM->IsDialogActive()) GM->SettingsToggle(); } }

void AErtCharacter::OnChoice1() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->DialogChoose(0); }
void AErtCharacter::OnChoice2() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->DialogChoose(1); }
void AErtCharacter::OnChoice3() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->DialogChoose(2); }
void AErtCharacter::OnChoice4() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->DialogChoose(3); }

void AErtCharacter::OnMenu() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->OnSkip(); }
void AErtCharacter::OnMenuUp() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->MenuMove(-1); }
void AErtCharacter::OnMenuDown() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->MenuMove(1); }
void AErtCharacter::OnConfirm()
{
	if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (GM->IsAnyMenu()) GM->MenuConfirm(); else GM->OnAdvance();
	}
}

void AErtCharacter::SetupPlayerInputComponent(UInputComponent* PIC)
{
	Super::SetupPlayerInputComponent(PIC);
	BuildInput();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->ClearAllMappings();
			Sub->AddMappingContext(IMC, 0);
		}
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PIC);
	if (!EIC) { UE_LOG(LogErtugrul, Error, TEXT("EnhancedInputComponent topilmadi - DefaultInput.ini tekshiring")); return; }
	EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AErtCharacter::OnMove);
	EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AErtCharacter::OnLook);
	EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AErtCharacter::OnJumpPressed);
	EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AErtCharacter::OnJumpReleased);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &AErtCharacter::OnSprintOn);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AErtCharacter::OnSprintOff);
	EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AErtCharacter::OnCrouchToggle);
	EIC->BindAction(IA_Walk, ETriggerEvent::Started, this, &AErtCharacter::OnWalkToggle);
	EIC->BindAction(IA_Zoom, ETriggerEvent::Triggered, this, &AErtCharacter::OnZoom);
	EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AErtCharacter::OnAttackPressed);
	EIC->BindAction(IA_Attack, ETriggerEvent::Completed, this, &AErtCharacter::OnAttackReleased);
	EIC->BindAction(IA_Kick, ETriggerEvent::Started, this, &AErtCharacter::OnKick);
	EIC->BindAction(IA_Block, ETriggerEvent::Started, this, &AErtCharacter::OnBlockOn);
	EIC->BindAction(IA_Block, ETriggerEvent::Completed, this, &AErtCharacter::OnBlockOff);
	EIC->BindAction(IA_Shoot, ETriggerEvent::Started, this, &AErtCharacter::OnShoot);
	EIC->BindAction(IA_Menu, ETriggerEvent::Started, this, &AErtCharacter::OnMenu);
	EIC->BindAction(IA_MenuUp, ETriggerEvent::Started, this, &AErtCharacter::OnMenuUp);
	EIC->BindAction(IA_MenuDown, ETriggerEvent::Started, this, &AErtCharacter::OnMenuDown);
	EIC->BindAction(IA_Confirm, ETriggerEvent::Started, this, &AErtCharacter::OnConfirm);
	EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AErtCharacter::OnInteract);
	EIC->BindAction(IA_Choice1, ETriggerEvent::Started, this, &AErtCharacter::OnChoice1);
	EIC->BindAction(IA_Choice2, ETriggerEvent::Started, this, &AErtCharacter::OnChoice2);
	EIC->BindAction(IA_Choice3, ETriggerEvent::Started, this, &AErtCharacter::OnChoice3);
	EIC->BindAction(IA_Choice4, ETriggerEvent::Started, this, &AErtCharacter::OnChoice4);
	EIC->BindAction(IA_MenuLeft, ETriggerEvent::Started, this, &AErtCharacter::OnMenuLeft);
	EIC->BindAction(IA_MenuRight, ETriggerEvent::Started, this, &AErtCharacter::OnMenuRight);
	EIC->BindAction(IA_Settings, ETriggerEvent::Started, this, &AErtCharacter::OnSettings);
	EIC->BindAction(IA_Map, ETriggerEvent::Started, this, &AErtCharacter::OnMap);
	ApplyBindings();
	EIC->BindAction(IA_Lock, ETriggerEvent::Started, this, &AErtCharacter::OnLock);
	EIC->BindAction(IA_Dodge, ETriggerEvent::Started, this, &AErtCharacter::OnDodge);
	EIC->BindAction(IA_Inventory, ETriggerEvent::Started, this, &AErtCharacter::OnInventory);
	EIC->BindAction(IA_Potion, ETriggerEvent::Started, this, &AErtCharacter::OnPotion);
}

void AErtCharacter::OnMove(const FInputActionValue& V)
{
	if (!bInputEnabled || bMantling) return;
	const FVector2D In = V.Get<FVector2D>();
	MoveInput = In;
	if (Horse) return;   // ot Tick da boshqariladi
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(Fwd, In.Y);
	AddMovementInput(Right, In.X);
}

float GErtMouseSens = 1.f; bool GErtInvertY = false;
void AErtCharacter::OnLook(const FInputActionValue& V)
{
	if (!bInputEnabled) return;
	const FVector2D L = V.Get<FVector2D>();
	AddControllerYawInput(GErtMouseSens * L.X);
	AddControllerPitchInput((GErtInvertY ? -1.f : 1.f) * GErtMouseSens * L.Y);
}

void AErtCharacter::OnJumpPressed()
{
	if (!bInputEnabled)
	{
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (GM->IsAnyMenu()) GM->MenuConfirm(); else GM->OnAdvance(); }
		return;
	}
	if (bMantling) return;
	if (Horse) { Horse->RiderJump(); return; }
	if (bIsCrouched) { UnCrouch(); return; }
	if (TryMantle()) return;
	Jump();
}

void AErtCharacter::OnJumpReleased() { StopJumping(); }
void AErtCharacter::OnSprintOn() { bWantSprint = true; }
void AErtCharacter::OnSprintOff() { bWantSprint = false; }

void AErtCharacter::OnCrouchToggle()
{
	if (!bInputEnabled || bMantling || Horse) return;
	if (bIsCrouched) UnCrouch(); else Crouch();
}

void AErtCharacter::OnWalkToggle() { bWalkToggle = !bWalkToggle; }

void AErtCharacter::OnZoom(const FInputActionValue& V)
{
	TargetArm = FMath::Clamp(TargetArm - V.Get<float>() * 40.f, CamMin, CamMax);
}

// ---------------- Mexanika ----------------

void AErtCharacter::UpdateGait(float Dt)
{
	UCharacterMovementComponent* CM = GetCharacterMovement();
	const bool bMoving = MoveInput.SizeSquared() > 0.01f && CM->Velocity.SizeSquared2D() > 100.f;
	const bool bCanSprint = bWantSprint && Stamina > 1.f && !bIsCrouched && !CM->IsFalling() && MoveInput.Y > -0.2f;
	if (bCanSprint && bMoving) Gait = EErtGait::Sprint;
	else if (bWalkToggle) Gait = EErtGait::Walk;
	else Gait = EErtGait::Jog;

	float Speed = Gait == EErtGait::Sprint ? SprintSpeed : (Gait == EErtGait::Walk ? WalkSpeed : JogSpeed);
	// Nishab: tepaga sekin, pastga bir oz tez
	const FVector Vel2D = CM->Velocity.GetSafeNormal2D();
	const float Uphill = -FVector::DotProduct(Vel2D, FVector(FloorNormal.X, FloorNormal.Y, 0.f).GetSafeNormal()) * FMath::Sin(FMath::DegreesToRadians(SlopeDeg));
	Speed *= FMath::Clamp(1.f - Uphill * 0.9f, 0.55f, 1.15f);
	CM->MaxWalkSpeed = FMath::FInterpTo(CM->MaxWalkSpeed, Speed, Dt, 8.f);

	if (Gait == EErtGait::Sprint && bMoving) Stamina = FMath::Max(0.f, Stamina - StaminaDrain * Dt);
	else Stamina = FMath::Min(StaminaMax, Stamina + StaminaRegen * Dt * (bMoving ? 0.5f : 1.f));
	if (Stamina <= 0.f) bWantSprint = false;
}

void AErtCharacter::UpdateSlope(float Dt)
{
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (CM->IsMovingOnGround() && CM->CurrentFloor.bBlockingHit)
	{
		FloorNormal = CM->CurrentFloor.HitResult.ImpactNormal;
		SlopeDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FloorNormal.Z, -1.f, 1.f)));
		if (SlopeDeg > SlideAngle)
		{
			// Tik qiyalik: pastga sirpanish kuchi (gradient bo'ylab)
			const FVector Down = FVector::VectorPlaneProject(FVector::DownVector, FloorNormal).GetSafeNormal();
			const float K = (SlopeDeg - SlideAngle) / FMath::Max(1.f, CM->GetWalkableFloorAngle() - SlideAngle);
			CM->AddForce(Down * 60000.f * K);
		}
	}
	else
	{
		SlopeDeg = FMath::FInterpTo(SlopeDeg, 0.f, Dt, 5.f);
		FloorNormal = FVector::UpVector;
	}
}

bool AErtCharacter::TryMantle()
{
	UWorld* W = GetWorld();
	if (!W) return false;
	const UCapsuleComponent* Cap = GetCapsuleComponent();
	const float R = Cap->GetScaledCapsuleRadius();
	const float HH = Cap->GetScaledCapsuleHalfHeight();
	const FVector Loc = GetActorLocation();
	const FVector Feet = Loc - FVector(0, 0, HH);
	FVector Fwd = GetActorForwardVector();
	if (MoveInput.SizeSquared() > 0.04f)
	{
		const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
		Fwd = (FRotationMatrix(YawRot).GetUnitAxis(EAxis::X) * MoveInput.Y + FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y) * MoveInput.X).GetSafeNormal();
	}
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtMantle), false, this);

	// 1) Oldinda devor bormi? (ko'krak balandligi)
	FHitResult Wall;
	const FVector A = Feet + FVector(0, 0, MantleMinHeight);
	if (!W->LineTraceSingleByChannel(Wall, A, A + Fwd * (R + 70.f), ECC_Visibility, Q)) return false;
	if (Wall.ImpactNormal.Z > 0.6f) return false;

	// 2) Devor ortidan pastga: chekka nuqtasi
	FHitResult Ledge;
	const FVector Top = Wall.ImpactPoint + Fwd * (R * 0.9f) + FVector(0, 0, MantleMaxHeight + 10.f - MantleMinHeight);
	if (!W->LineTraceSingleByChannel(Ledge, Top, Top - FVector(0, 0, MantleMaxHeight + 20.f), ECC_Visibility, Q)) return false;
	const float H = Ledge.ImpactPoint.Z - Feet.Z;
	if (H < MantleMinHeight || H > MantleMaxHeight || Ledge.ImpactNormal.Z < 0.7f) return false;

	// 3) Tepada kapsula sig'adimi?
	const FVector Target = Ledge.ImpactPoint + Fwd * 8.f + FVector(0, 0, HH + 4.f);
	if (W->OverlapBlockingTestByChannel(Target, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(R * 0.95f, HH * 0.95f), Q)) return false;

	bMantling = true;
	MantleT = 0.f;
	MantleStart = Loc;
	MantleEnd = Target;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	SetActorRotation(FRotator(0.f, Fwd.Rotation().Yaw, 0.f));
	return true;
}

void AErtCharacter::UpdateMantle(float Dt)
{
	MantleT += Dt / FMath::Max(0.05f, MantleDuration);
	const float T = FMath::Clamp(MantleT, 0.f, 1.f);
	// Avval ko'tariladi, keyin oldinga o'tadi
	const float Up = FMath::Sin(FMath::Min(T * 1.35f, 1.f) * HALF_PI);
	const float Fw = FMath::Clamp((T - 0.35f) / 0.65f, 0.f, 1.f);
	const float FwS = Fw * Fw * (3.f - 2.f * Fw);
	FVector P;
	P.Z = FMath::Lerp(MantleStart.Z, MantleEnd.Z, Up);
	P.X = FMath::Lerp(MantleStart.X, MantleEnd.X, FwS);
	P.Y = FMath::Lerp(MantleStart.Y, MantleEnd.Y, FwS);
	SetActorLocation(P, false, nullptr, ETeleportType::TeleportPhysics);
	if (MantleT >= 1.f)
	{
		bMantling = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AErtCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float Fall = -GetVelocity().Z;
	LandSquash = FMath::Clamp(Fall / 900.f, 0.f, 1.f);
}

void AErtCharacter::Tick(float Dt)
{
	Super::Tick(Dt);
	UpdateCombat(Dt);
	DodgeT = FMath::Max(0.f, DodgeT - Dt);
	ComboWindowT = FMath::Max(0.f, ComboWindowT - Dt);
	if (AttackHoldT >= 0.f)
	{
		AttackHoldT += Dt;
		if (!bHeavyDone && AttackHoldT >= 0.35f && AttackCD <= 0.f && bInputEnabled && !bDead && !bMantling)
		{
			// Og'ir zarba: 2.2x zarar, qalqonni sindiradi, 0.6 s gangitadi
			bHeavyDone = true; ComboStep = 0; ComboWindowT = 0.f;
			AttackCD = 1.05f; Stamina = FMath::Max(0.f, Stamina - 14.f);
			DoAttack(2, 2.2f, true, 0.6f, 300.f);
		}
	}
	UpdateLock(Dt);
	if (ShakeT > 0.f) { ShakeT = FMath::Max(0.f, ShakeT - Dt); const float A = ShakeT * 40.f; Boom->SocketOffset = BoomBase + FVector(0, FMath::FRandRange(-A, A), FMath::FRandRange(-A, A)); }
	else if (Boom->SocketOffset != BoomBase) Boom->SocketOffset = BoomBase;
	if (ShotT >= 0.f) UpdateShotScript(Dt);
	if (bDead) { if (Horse) DismountHorse(); MoveInput = FVector2D::ZeroVector; if (bShowDebug) DrawDebug(); return; }
	if (Horse)
	{
		Horse->SetRiderInput(MoveInput, bWantSprint && Stamina > 1.f);
		if (bWantSprint && !MoveInput.IsNearlyZero()) Stamina = FMath::Max(0.f, Stamina - 5.f * Dt); else Stamina = FMath::Min(StaminaMax, Stamina + StaminaRegen * 0.6f * Dt);
		Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, FMath::Max(TargetArm, 480.f), Dt, 6.f);
		if (Body) Body->Animate(Dt, Horse->GetSpeed(), false, false, 0.f, 0.f);
		MoveInput = FVector2D::ZeroVector;
		return;
	}
	UpdateSwim(Dt);
	if (bMantling) UpdateMantle(Dt);
	else if (!bSwimming) { UpdateSlope(Dt); UpdateGait(Dt); UpdateSteps(Dt); }
	if (bBlocking) GetCharacterMovement()->MaxWalkSpeed = FMath::Min(GetCharacterMovement()->MaxWalkSpeed, WalkSpeed);

	Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, TargetArm, Dt, 10.f);
	LandSquash = FMath::FInterpTo(LandSquash, 0.f, Dt, 6.f);

	if (Body)
	{
		const UCharacterMovementComponent* CM = GetCharacterMovement();
		const FVector V = CM->Velocity;
		const float Lean = FVector::DotProduct(GetActorRightVector(), V.GetSafeNormal2D()) * FMath::Min(V.Size2D() / SprintSpeed, 1.f);
		Body->Animate(Dt, V.Size2D(), CM->IsFalling() || bMantling, bIsCrouched || LandSquash > 0.35f || DodgeT > 0.15f, Lean, SlopeDeg);
	}
	MoveInput = FVector2D::ZeroVector;
	if (bShowDebug) DrawDebug();
}

void AErtCharacter::DrawDebug()
{
	if (!GEngine) return;
	const UCharacterMovementComponent* CM = GetCharacterMovement();
	const TCHAR* G = Gait == EErtGait::Sprint ? TEXT("CHOPISH") : (Gait == EErtGait::Walk ? TEXT("YURISH") : TEXT("YUGURISH"));
	const FString S = FString::Printf(TEXT("%s  tezlik %.0f sm/s  stamina %.0f  nishab %.0f%s  %s%s"),
		G, CM->Velocity.Size2D(), Stamina, SlopeDeg, TEXT("°"),
		bIsCrouched ? TEXT("[cho'kkan] ") : TEXT(""), bMantling ? TEXT("[mantle] ") : (CM->IsFalling() ? TEXT("[havoda] ") : TEXT("")));
	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, S);
	GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Silver, TEXT("WASD yurish | Shift chopish | Space sakrash/mantle | Ctrl/C cho'kish | Alt yurish rejimi | g'ildirak zoom"));
}
