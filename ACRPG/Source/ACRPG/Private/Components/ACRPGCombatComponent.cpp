#include "Components/ACRPGCombatComponent.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGEquipmentComponent.h"
#include "Items/ACRPGWeaponBase.h"
#include "Items/ACRPGThrowableActor.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

UACRPGCombatComponent::UACRPGCombatComponent()
{
	// Tick faqat trace paytida kerak — BeginPlay'da o'chirib qo'yamiz.
	PrimaryComponentTick.bCanEverTick = true;
}

void UACRPGCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());

	// Boshida Tick kerak emas — EnableWeaponTrace uni yoqadi.
	SetComponentTickEnabled(false);
}

void UACRPGCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bWeaponTraceActive)
	{
		PerformWeaponTrace();
	}
}

// ---------------------------------------------------------------------------
// KOMBO TIZIMI (Ep.#9)
// ---------------------------------------------------------------------------

void UACRPGCombatComponent::RequestAttack()
{
	if (!OwnerCharacter || ComboSteps.Num() == 0)
	{
		return;
	}

	if (!OwnerCharacter->CanStartAction())
	{
		return;
	}

	// --- Holat 1: hozir hujum qilmayapmiz -> komboni boshdan boshlaymiz ---
	if (!bIsAttacking)
	{
		StartComboStep(0);
		return;
	}

	// --- Holat 2: hujum ketyapti va kombo oynasi OCHIQ -> keyingisiga o'tamiz ---
	if (bComboWindowOpen)
	{
		const int32 NextIndex = CurrentComboIndex + 1;
		if (ComboSteps.IsValidIndex(NextIndex))
		{
			StartComboStep(NextIndex);
		}
		else
		{
			// Zanjir tugadi — boshidan (loop qilmoqchi bo'lsangiz).
			StartComboStep(0);
		}
		return;
	}

	// --- Holat 3: oyna yopiq -> bosishni ESLAB QOLAMIZ (input buffer) ---
	// Bu jangni "javob beruvchi" qiladi: o'yinchi biroz erta bossa ham zarba o'tadi.
	bInputBuffered = true;
}

void UACRPGCombatComponent::StartComboStep(int32 StepIndex)
{
	if (!ComboSteps.IsValidIndex(StepIndex) || !OwnerCharacter)
	{
		return;
	}

	const FACRPGComboStep& Step = ComboSteps[StepIndex];
	if (!Step.Montage)
	{
		UE_LOG(LogACRPG, Warning, TEXT("Kombo qadam %d da montaj yo'q."), StepIndex);
		return;
	}

	// Ep.#6 — chidamlilik yetmasa hujum bo'lmaydi.
	if (UACRPGStatsComponent* Stats = OwnerCharacter->GetStats())
	{
		if (!Stats->ConsumeStamina(Step.StaminaCost))
		{
			return;
		}
	}

	// Ep.#75 — zarbani eng yaqin dushmanga qaratamiz.
	if (bOrientToNearestEnemy)
	{
		OrientTowardsNearestEnemy();
	}

	CurrentComboIndex = StepIndex;
	bIsAttacking = true;
	bComboWindowOpen = false;
	bInputBuffered = false;
	AlreadyHitActors.Empty();

	OwnerCharacter->SetCombatState(ECombatState::Attacking);
	OwnerCharacter->PlayAnimMontage(Step.Montage);

	if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UACRPGCombatComponent::OnAttackMontageEnded);
		AnimInst->Montage_SetEndDelegate(EndDelegate, Step.Montage);
	}
}

void UACRPGCombatComponent::OpenComboWindow()
{
	bComboWindowOpen = true;

	// Oyna ochilishidan OLDIN bosilgan bo'lsa — shu zahoti keyingisiga o'tamiz.
	if (bInputBuffered)
	{
		bInputBuffered = false;
		RequestAttack();
	}
}

void UACRPGCombatComponent::CloseComboWindow()
{
	bComboWindowOpen = false;
}

void UACRPGCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// bInterrupted true bo'lsa — boshqa montaj (masalan hit reaction) uzdi.
	// Bunday holda kombo tugaydi.
	bIsAttacking = false;
	bComboWindowOpen = false;
	bInputBuffered = false;
	CurrentComboIndex = -1;

	DisableWeaponTrace();

	if (OwnerCharacter && OwnerCharacter->GetCombatState() == ECombatState::Attacking)
	{
		OwnerCharacter->SetCombatState(ECombatState::Idle);
	}
}

// ---------------------------------------------------------------------------
// QILICH TRACE'I (Ep.#10)
// ---------------------------------------------------------------------------

USceneComponent* UACRPGCombatComponent::GetTraceSourceComponent() const
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	// Ep.#19 — qo'lda qurol bo'lsa, trace o'sha quroldan o'tadi.
	if (const UACRPGEquipmentComponent* Equip =
		OwnerCharacter->FindComponentByClass<UACRPGEquipmentComponent>())
	{
		if (AACRPGWeaponBase* Weapon = Equip->GetEquippedWeapon())
		{
			return Weapon->GetWeaponMesh();
		}
	}

	// Qurolsiz bo'lsa — personaj mesh'ining o'zi (musht uchun socket qo'yiladi).
	return OwnerCharacter->GetMesh();
}

void UACRPGCombatComponent::EnableWeaponTrace()
{
	bWeaponTraceActive = true;
	bHasPrevTrace = false;			// Yangi zarba — oldingi pozitsiyalar eskirgan.
	AlreadyHitActors.Empty();
	SetComponentTickEnabled(true);
}

void UACRPGCombatComponent::DisableWeaponTrace()
{
	bWeaponTraceActive = false;
	bHasPrevTrace = false;
	SetComponentTickEnabled(false);
}

void UACRPGCombatComponent::PerformWeaponTrace()
{
	USceneComponent* Source = GetTraceSourceComponent();
	UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
	if (!Source || !World)
	{
		return;
	}

	const FVector CurStart = Source->GetSocketLocation(WeaponTraceStartSocket);
	const FVector CurEnd   = Source->GetSocketLocation(WeaponTraceEndSocket);

	// Birinchi kadr — solishtirish uchun oldingi holat yo'q.
	if (!bHasPrevTrace)
	{
		PrevTraceStart = CurStart;
		PrevTraceEnd = CurEnd;
		bHasPrevTrace = true;
		return;
	}

	// --- Qilich pichog'i bo'ylab bir nechta nuqtadan sweep ---
	//
	// Faqat uchi bo'yicha tekshirsak, dastaga yaqin turgan dushman uriladi emas.
	// Shuning uchun pichoqni bir necha bo'lakka bo'lamiz.
	constexpr int32 NumSegments = 5;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponTrace), false, OwnerCharacter);
	Params.AddIgnoredActor(OwnerCharacter);
	Params.bTraceComplex = false;

	const FCollisionShape Sphere = FCollisionShape::MakeSphere(WeaponTraceRadius);

	for (int32 i = 0; i <= NumSegments; ++i)
	{
		const float Alpha = static_cast<float>(i) / NumSegments;

		// Oldingi kadrdagi nuqtadan hozirgi kadrdagi nuqtaga sweep —
		// shunda tez harakatda ham hech kim "o'tkazib yuborilmaydi".
		const FVector From = FMath::Lerp(PrevTraceStart, PrevTraceEnd, Alpha);
		const FVector To   = FMath::Lerp(CurStart, CurEnd, Alpha);

		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(Hits, From, To, FQuat::Identity, ECC_Pawn, Sphere, Params);

		if (bShowTraceDebug)
		{
			DrawDebugLine(World, From, To, FColor::Red, false, 0.5f, 0, 1.f);
		}

		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == OwnerCharacter)
			{
				continue;
			}

			// Bir zarbada bir marta — bu tekshiruv bo'lmasa dushman 10 barobar urish yeydi.
			if (AlreadyHitActors.Contains(HitActor))
			{
				continue;
			}

			AACRPGCharacterBase* Victim = Cast<AACRPGCharacterBase>(HitActor);
			if (!Victim || !Victim->IsAlive())
			{
				continue;
			}

			AlreadyHitActors.Add(HitActor);

			// --- Urish miqdorini hisoblaymiz ---
			float BaseDamage = UnarmedDamage;
			if (const UACRPGEquipmentComponent* Equip =
				OwnerCharacter->FindComponentByClass<UACRPGEquipmentComponent>())
			{
				BaseDamage = Equip->GetWeaponDamage() > 0.f ? Equip->GetWeaponDamage() : UnarmedDamage;
			}

			float Multiplier = 1.f;
			if (ComboSteps.IsValidIndex(CurrentComboIndex))
			{
				Multiplier = ComboSteps[CurrentComboIndex].DamageMultiplier;
			}

			const float FinalDamage = BaseDamage * Multiplier;

			// PointDamage — zarba nuqtasi ham uzatiladi (Ep.#11 qon effekti uchun).
			FPointDamageEvent DamageEvent;
			DamageEvent.Damage = FinalDamage;
			DamageEvent.HitInfo = Hit;
			DamageEvent.ShotDirection = (To - From).GetSafeNormal();

			Victim->TakeDamage(FinalDamage, DamageEvent,
				OwnerCharacter->GetController(), OwnerCharacter);

			OnDamageDealt.Broadcast(Victim, FinalDamage);
		}
	}

	PrevTraceStart = CurStart;
	PrevTraceEnd = CurEnd;
}

// ---------------------------------------------------------------------------
// Ep.#75 — HUJUM YO'NALISHI
// ---------------------------------------------------------------------------

void UACRPGCombatComponent::OrientTowardsNearestEnemy()
{
	if (!OwnerCharacter)
	{
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AttackOrient), false, OwnerCharacter);

	World->OverlapMultiByObjectType(
		Overlaps, OwnerCharacter->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(OrientSearchRadius), Params);

	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& R : Overlaps)
	{
		AACRPGCharacterBase* Other = Cast<AACRPGCharacterBase>(R.GetActor());
		if (!Other || Other == OwnerCharacter || !Other->IsAlive())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(
			Other->GetActorLocation(), OwnerCharacter->GetActorLocation());

		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Other;
		}
	}

	if (!Nearest)
	{
		return;
	}

	// Faqat Yaw — aks holda personaj yerga qarab egiladi.
	const FRotator Look = UKismetMathLibrary::FindLookAtRotation(
		OwnerCharacter->GetActorLocation(), Nearest->GetActorLocation());

	OwnerCharacter->SetActorRotation(FRotator(0.f, Look.Yaw, 0.f));
}

// ---------------------------------------------------------------------------
// BLOK (Ep.#69-70)
// ---------------------------------------------------------------------------

void UACRPGCombatComponent::StartBlocking()
{
	if (!OwnerCharacter || !OwnerCharacter->CanStartAction() || bIsAttacking)
	{
		return;
	}

	bIsBlocking = true;
	OwnerCharacter->SetCombatState(ECombatState::Blocking);
}

void UACRPGCombatComponent::StopBlocking()
{
	if (!bIsBlocking)
	{
		return;
	}

	bIsBlocking = false;
	if (OwnerCharacter && OwnerCharacter->GetCombatState() == ECombatState::Blocking)
	{
		OwnerCharacter->SetCombatState(ECombatState::Idle);
	}
}

float UACRPGCombatComponent::ModifyIncomingDamage(float IncomingDamage, const FVector& AttackerLocation)
{
	if (!bIsBlocking || !OwnerCharacter)
	{
		return IncomingDamage;
	}

	// --- Zarba OLDINDAN keldimi? ---
	// Orqadan kelgan zarba bloklanmaydi — aks holda blok "o'lmaslik tugmasi" bo'lib qoladi.
	const FVector ToAttacker = (AttackerLocation - OwnerCharacter->GetActorLocation()).GetSafeNormal2D();
	const float Dot = FVector::DotProduct(OwnerCharacter->GetActorForwardVector(), ToAttacker);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(BlockAngleDegrees * 0.5f));

	if (Dot < MinDot)
	{
		return IncomingDamage;	// Yon yoki orqadan — blok ishlamaydi.
	}

	// --- Chidamlilik yetadimi? ---
	UACRPGStatsComponent* Stats = OwnerCharacter->GetStats();
	if (!Stats || !Stats->ConsumeStamina(BlockStaminaCost))
	{
		// Chidamlilik tugadi — blok sinadi (guard break).
		StopBlocking();
		return IncomingDamage;
	}

	// Blok muvaffaqiyatli.
	if (BlockImpactMontage)
	{
		OwnerCharacter->PlayAnimMontage(BlockImpactMontage);
	}

	return IncomingDamage * BlockDamageMultiplier;
}

// ---------------------------------------------------------------------------
// CHALG'ITISH (Ep.#27)
// ---------------------------------------------------------------------------

void UACRPGCombatComponent::ThrowDistraction()
{
	if (!OwnerCharacter || !ThrowableClass || !OwnerCharacter->CanStartAction())
	{
		return;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return;
	}

	if (ThrowMontage)
	{
		OwnerCharacter->PlayAnimMontage(ThrowMontage);
	}

	// Ko'krak balandligidan, kamera qaragan tomonga.
	const FVector SpawnLoc = OwnerCharacter->GetActorLocation()
		+ OwnerCharacter->GetActorForwardVector() * 50.f
		+ FVector(0.f, 0.f, 50.f);

	const FRotator AimRot = OwnerCharacter->GetBaseAimRotation();

	FActorSpawnParameters Params;
	Params.Owner = OwnerCharacter;
	Params.Instigator = OwnerCharacter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AACRPGThrowableActor* Thrown =
		World->SpawnActor<AACRPGThrowableActor>(ThrowableClass, SpawnLoc, AimRot, Params))
	{
		// Biroz yuqoriga — parabola chiroyli chiqsin.
		const FVector LaunchDir = (AimRot.Vector() + FVector(0.f, 0.f, 0.25f)).GetSafeNormal();
		Thrown->Launch(LaunchDir * ThrowSpeed);
	}
}
