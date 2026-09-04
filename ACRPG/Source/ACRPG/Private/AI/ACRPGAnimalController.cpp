#include "AI/ACRPGAnimalController.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AACRPGAnimalController::AACRPGAnimalController()
{
	// Hayvonlar odamlarga qaraganda kamroq ko'radi, lekin yaxshi eshitadi.
	SightRadius = 900.f;
	LoseSightRadius = 1200.f;
	PeripheralVisionAngle = 200.f;	// keng ko'rish maydoni — o'txo'rlar kabi
	HearingRange = 2000.f;
}

void AACRPGAnimalController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(ACRPGBlackboardKeys::AttackRange, AttackRange);
	}

	ScheduleNextIdleSound();
}

bool AACRPGAnimalController::IsHostileTo(const AActor* Other) const
{
	// Faqat yirtqich hayvon o'yinchini dushman deb biladi.
	if (Behavior != EAnimalBehavior::Aggressive)
	{
		return false;
	}
	return Super::IsHostileTo(Other);
}

void AACRPGAnimalController::HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (bIsMounted)
	{
		return;
	}

	const AACRPGCharacterBase* SeenChar = Cast<AACRPGCharacterBase>(Actor);
	const bool bIsPlayer = SeenChar && SeenChar->CharacterTag == TEXT("Player");

	if (!bIsPlayer || !Stimulus.WasSuccessfullySensed())
	{
		Super::HandleSightStimulus(Actor, Stimulus);
		return;
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	switch (Behavior)
	{
	case EAnimalBehavior::Aggressive:
		// Ep.#58 — yirtqich hujum qiladi.
		SetTargetActor(Actor);
		SetAIState(EAIState::Chasing);
		break;

	case EAnimalBehavior::Passive:
	case EAnimalBehavior::Mountable:
		// Ep.#57 — beozor hayvon qochadi.
		SetTargetActor(Actor);
		SetAIState(EAIState::Fleeing);
		break;
	}

	// Ep.#59 — o'yinchini payqaganda tovush.
	if (AlertSound && GetPawn())
	{
		UGameplayStatics::PlaySoundAtLocation(this, AlertSound, GetPawn()->GetActorLocation());
	}
}

// ---------------------------------------------------------------------------
// Ep.#63 — MINISH
// ---------------------------------------------------------------------------

void AACRPGAnimalController::SetMounted(bool bMounted)
{
	bIsMounted = bMounted;

	if (UBrainComponent* Brain = GetBrainComponent())
	{
		if (bMounted)
		{
			// AI butunlay to'xtaydi — endi hayvonni o'yinchi boshqaradi.
			Brain->StopLogic(TEXT("Minildi"));
			GetWorldTimerManager().ClearTimer(IdleSoundTimer);
		}
		else
		{
			Brain->RestartLogic();
			SetAIState(EAIState::Patrolling);
			ScheduleNextIdleSound();
		}
	}
}

// ---------------------------------------------------------------------------
// Ep.#59 — TOVUSHLAR
// ---------------------------------------------------------------------------

void AACRPGAnimalController::ScheduleNextIdleSound()
{
	if (IdleSounds.Num() == 0)
	{
		return;
	}

	const float Delay = FMath::FRandRange(MinIdleSoundInterval, MaxIdleSoundInterval);
	GetWorldTimerManager().SetTimer(
		IdleSoundTimer, this, &AACRPGAnimalController::PlayIdleSound, Delay, false);
}

void AACRPGAnimalController::PlayIdleSound()
{
	APawn* Pawn = GetPawn();
	if (!Pawn || bIsMounted || IdleSounds.Num() == 0)
	{
		ScheduleNextIdleSound();
		return;
	}

	// Faqat tirik hayvon tovush chiqaradi.
	if (const AACRPGCharacterBase* Char = Cast<AACRPGCharacterBase>(Pawn))
	{
		if (!Char->IsAlive())
		{
			return;
		}
	}

	const int32 Index = FMath::RandRange(0, IdleSounds.Num() - 1);
	if (IdleSounds[Index])
	{
		UGameplayStatics::PlaySoundAtLocation(this, IdleSounds[Index], Pawn->GetActorLocation());
	}

	ScheduleNextIdleSound();
}
