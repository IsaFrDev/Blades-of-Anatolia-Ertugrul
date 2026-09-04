// ACRPGBTNodes.h — Behavior Tree tugunlari (Ep.#20-24, #28)
//
// Blueprint'da bu tugunlar "BTTask_BlueprintBase" dan meros olgan alohida assetlar edi.
// C++ da ularni bitta faylga jamlash mumkin va tavsiya etiladi: ular kichik,
// bir-biriga bog'liq va birga o'qilgani ma'qul.
//
// TUGUN TURLARI:
//   Task      — biror ishni bajaradi ("shu nuqtaga bor"). Succeeded/Failed qaytaradi.
//   Service   — Composite tugun faol bo'lganda muntazam ishlaydi (holatni yangilaydi).
//   Decorator — shart ("nishon yaqinmi?"). Shoxni yoqadi yoki o'chiradi.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "ACRPGBTNodes.generated.h"

// ===========================================================================
// Ep.#20 — PATRUL NUQTASINI TOPISH
//
// Ikki rejim:
//   - Patrol Route aktyori bo'lsa: uning nuqtalari bo'ylab navbat bilan.
//   - Bo'lmasa: "uy" nuqtasi atrofida tasodifiy nuqta (NavMesh ustidan).
// ===========================================================================

UCLASS()
class ACRPG_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPoint();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** Natija shu kalitga yoziladi (Vector). */
	UPROPERTY(EditAnywhere, Category = "Patrul")
	FBlackboardKeySelector PatrolLocationKey;

	/** Uy nuqtasi (Vector). */
	UPROPERTY(EditAnywhere, Category = "Patrul")
	FBlackboardKeySelector HomeLocationKey;

	/** Uy atrofida qancha radiusda yuradi. */
	UPROPERTY(EditAnywhere, Category = "Patrul", meta = (ClampMin = "100"))
	float PatrolRadius = 1000.f;
};

// ===========================================================================
// Ep.#22-23 — YAQIN JANG HUJUMI
//
// CombatComponent->RequestAttack() ni kombo uzunligi marta chaqiradi.
// Har chaqiruv orasida kutish bor — aks holda hammasi bir kadrda bo'lib o'tadi.
// ===========================================================================

UCLASS()
class ACRPG_API UBTTask_MeleeAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MeleeAttack();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	/** Zarbalar orasidagi vaqt. */
	UPROPERTY(EditAnywhere, Category = "Hujum", meta = (ClampMin = "0.1"))
	float TimeBetweenHits = 0.9f;
};

/** UBTTask_MeleeAttack uchun har bir AI ga xos xotira. */
struct FBTMeleeAttackMemory
{
	int32 HitsRemaining = 0;
	float TimeUntilNextHit = 0.f;
};

// ===========================================================================
// NISHONGA BURILISH — hujumdan oldin (Ep.#22)
// ===========================================================================

UCLASS()
class ACRPG_API UBTTask_FaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FaceTarget();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Nishon")
	FBlackboardKeySelector TargetActorKey;
};

// ===========================================================================
// Ep.#21 — HOLATNI YANGILAB TURUVCHI SERVICE
//
// Har 0.3 soniyada: nishon bormi, tirikmi, yaqinmi — shunga qarab
// AIState ni Chasing yoki Attacking ga o'zgartiradi.
//
// Nega Service? Chunki bu tekshiruv Task bajarilayotgan paytda ham davom etishi kerak.
// ===========================================================================

UCLASS()
class ACRPG_API UBTService_UpdateCombatState : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatState();
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Holat")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Holat")
	FBlackboardKeySelector AIStateKey;
};

// ===========================================================================
// Ep.#29 — TINCH AHOLI: qochish nuqtasini topish
// ===========================================================================

UCLASS()
class ACRPG_API UBTTask_FindFleeLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindFleeLocation();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Qochish")
	FBlackboardKeySelector ThreatActorKey;

	UPROPERTY(EditAnywhere, Category = "Qochish")
	FBlackboardKeySelector FleeLocationKey;

	/** Tahdiddan qancha uzoqqa qochadi. */
	UPROPERTY(EditAnywhere, Category = "Qochish", meta = (ClampMin = "200"))
	float FleeDistance = 1500.f;
};
