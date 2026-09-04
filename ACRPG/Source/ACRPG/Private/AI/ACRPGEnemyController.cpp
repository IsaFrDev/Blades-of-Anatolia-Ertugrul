#include "AI/ACRPGEnemyController.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AACRPGEnemyController::AACRPGEnemyController()
{
}

void AACRPGEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// BT Decorator "Distance Less Than" shu qiymatni Blackboard'dan o'qiydi.
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(ACRPGBlackboardKeys::AttackRange, AttackRange);
	}

	// Ep.#28 — boss g'azablanishi uchun sog'liqni kuzatamiz.
	if (bIsBoss && ControlledCharacter)
	{
		if (UACRPGStatsComponent* Stats = ControlledCharacter->GetStats())
		{
			Stats->OnHealthChanged.AddDynamic(this, &AACRPGEnemyController::OnHealthChangedCheckEnrage);
		}
	}
}

bool AACRPGEnemyController::IsHostileTo(const AActor* Other) const
{
	// Dushman uchun faqat o'yinchi nishon.
	// (Ep.#57 — hayvonlarni ham nishon qilmoqchi bo'lsangiz, shu yerni kengaytirasiz.)
	return Super::IsHostileTo(Other);
}

int32 AACRPGEnemyController::RollComboLength() const
{
	int32 Length = FMath::RandRange(MinComboLength, MaxComboLength);

	// G'azablangan boss uzunroq kombo qiladi.
	if (bEnraged)
	{
		Length = FMath::Min(Length + 1, MaxComboLength + 1);
	}
	return Length;
}

float AACRPGEnemyController::GetAttackCooldown() const
{
	const float Base = FMath::FRandRange(MinAttackCooldown, MaxAttackCooldown);
	return bEnraged ? Base / EnrageSpeedMultiplier : Base;
}

void AACRPGEnemyController::OnHealthChangedCheckEnrage(float NewHealth, float MaxHealth)
{
	if (bEnraged || MaxHealth <= 0.f)
	{
		return;
	}

	if (NewHealth / MaxHealth > EnrageHealthPercent)
	{
		return;
	}

	bEnraged = true;
	UE_LOG(LogACRPG, Log, TEXT("Boss g'azablandi!"));

	// Tezroq harakat qiladi.
	if (ControlledCharacter)
	{
		if (UCharacterMovementComponent* Move = ControlledCharacter->GetCharacterMovement())
		{
			Move->MaxWalkSpeed *= EnrageSpeedMultiplier;
		}
	}
}
