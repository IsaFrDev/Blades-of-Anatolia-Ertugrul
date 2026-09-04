#include "AI/ACRPGBTNodes.h"

#include "Core/ACRPG.h"
#include "AI/ACRPGAIControllerBase.h"
#include "AI/ACRPGEnemyController.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGCombatComponent.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"

// ===========================================================================
// Ep.#20 — PATRUL NUQTASI
// ===========================================================================

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
	NodeName = TEXT("Patrul nuqtasini top");

	// Blackboard kalitini faqat Vector turiga cheklaymiz — editor'da xato tanlab bo'lmasin.
	PatrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, PatrolLocationKey));
	HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, HomeLocationKey));
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;

	if (!BB || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	// Uy nuqtasi — patrul markazi.
	const FVector Home = BB->GetValueAsVector(HomeLocationKey.SelectedKeyName);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	// NavMesh ustidan tasodifiy nuqta — dushman devor ichiga yurmasin.
	FNavLocation RandomPoint;
	if (!NavSys->GetRandomReachablePointInRadius(Home, PatrolRadius, RandomPoint))
	{
		return EBTNodeResult::Failed;
	}

	BB->SetValueAsVector(PatrolLocationKey.SelectedKeyName, RandomPoint.Location);
	return EBTNodeResult::Succeeded;
}

// ===========================================================================
// Ep.#22-23 — YAQIN JANG KOMBOSI
// ===========================================================================

UBTTask_MeleeAttack::UBTTask_MeleeAttack()
{
	NodeName = TEXT("Yaqin jang hujumi");
	bNotifyTick = true;			// TickTask chaqirilishi uchun SHART
	bCreateNodeInstance = false;	// Xotirani NodeMemory orqali boshqaramiz (tezroq)
}

uint16 UBTTask_MeleeAttack::GetInstanceMemorySize() const
{
	return sizeof(FBTMeleeAttackMemory);
}

EBTNodeResult::Type UBTTask_MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AACRPGEnemyController* AI = Cast<AACRPGEnemyController>(OwnerComp.GetAIOwner());
	AACRPGCharacterBase* Character = AI ? Cast<AACRPGCharacterBase>(AI->GetPawn()) : nullptr;

	if (!Character || !Character->IsAlive())
	{
		return EBTNodeResult::Failed;
	}

	UACRPGCombatComponent* Combat = Character->GetCombat();
	if (!Combat)
	{
		return EBTNodeResult::Failed;
	}

	FBTMeleeAttackMemory* Memory = reinterpret_cast<FBTMeleeAttackMemory*>(NodeMemory);

	// Ep.#23 — bu safar nechta zarba? Tasodifiy.
	Memory->HitsRemaining = AI->RollComboLength();
	Memory->TimeUntilNextHit = 0.f;

	Character->SetCombatState(ECombatState::Attacking);

	// InProgress qaytaramiz — Task TickTask orqali davom etadi.
	return EBTNodeResult::InProgress;
}

void UBTTask_MeleeAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTMeleeAttackMemory* Memory = reinterpret_cast<FBTMeleeAttackMemory*>(NodeMemory);

	AAIController* AI = OwnerComp.GetAIOwner();
	AACRPGCharacterBase* Character = AI ? Cast<AACRPGCharacterBase>(AI->GetPawn()) : nullptr;

	// O'lib qolsa — darhol to'xtaymiz.
	if (!Character || !Character->IsAlive())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Memory->TimeUntilNextHit -= DeltaSeconds;
	if (Memory->TimeUntilNextHit > 0.f)
	{
		return;
	}

	if (Memory->HitsRemaining <= 0)
	{
		Character->SetCombatState(ECombatState::Idle);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (UACRPGCombatComponent* Combat = Character->GetCombat())
	{
		Combat->RequestAttack();
	}

	--Memory->HitsRemaining;
	Memory->TimeUntilNextHit = TimeBetweenHits;
}

// ===========================================================================
// NISHONGA BURILISH
// ===========================================================================

UBTTask_FaceTarget::UBTTask_FaceTarget()
{
	NodeName = TEXT("Nishonga burilish");
	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_FaceTarget, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;

	if (!BB || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	// Faqat Yaw — dushman yerga qarab egilmasin.
	const FRotator Look = UKismetMathLibrary::FindLookAtRotation(
		Pawn->GetActorLocation(), Target->GetActorLocation());

	Pawn->SetActorRotation(FRotator(0.f, Look.Yaw, 0.f));
	return EBTNodeResult::Succeeded;
}

// ===========================================================================
// Ep.#21 — HOLAT SERVICE
// ===========================================================================

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
	NodeName = TEXT("Jang holatini yangilash");
	Interval = 0.3f;			// Har kadr emas — 0.3s da bir marta yetarli.
	RandomDeviation = 0.05f;	// Barcha AI bir vaqtda hisoblamasin (frame spike).
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, TargetActorKey), AActor::StaticClass());
	AIStateKey.AddEnumFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, AIStateKey), StaticEnum<EAIState>());
}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AACRPGEnemyController* AI = Cast<AACRPGEnemyController>(OwnerComp.GetAIOwner());
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;

	if (!BB || !Pawn)
	{
		return;
	}

	AACRPGCharacterBase* Target = Cast<AACRPGCharacterBase>(
		BB->GetValueAsObject(TargetActorKey.SelectedKeyName));

	// --- Nishon yo'q yoki o'lgan -> tozalaymiz ---
	if (!Target || !Target->IsAlive())
	{
		BB->ClearValue(TargetActorKey.SelectedKeyName);

		const EAIState Current = static_cast<EAIState>(BB->GetValueAsEnum(AIStateKey.SelectedKeyName));
		if (Current == EAIState::Chasing || Current == EAIState::Attacking)
		{
			BB->SetValueAsEnum(AIStateKey.SelectedKeyName, static_cast<uint8>(EAIState::Patrolling));
		}
		return;
	}

	// --- Masofaga qarab: hujummi yoki quvishmi? ---
	const float Distance = FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation());
	const float Range = AI->GetAttackRange();

	// Kichik "hysteresis": chegarada turganda holat titramasin.
	const EAIState NewState = (Distance <= Range * 1.05f)
		? EAIState::Attacking
		: EAIState::Chasing;

	BB->SetValueAsEnum(AIStateKey.SelectedKeyName, static_cast<uint8>(NewState));
}

// ===========================================================================
// Ep.#29 — TINCH AHOLI QOCHADI
// ===========================================================================

UBTTask_FindFleeLocation::UBTTask_FindFleeLocation()
{
	NodeName = TEXT("Qochish nuqtasini top");
	ThreatActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_FindFleeLocation, ThreatActorKey), AActor::StaticClass());
	FleeLocationKey.AddVectorFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_FindFleeLocation, FleeLocationKey));
}

EBTNodeResult::Type UBTTask_FindFleeLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;

	if (!BB || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Threat = Cast<AActor>(BB->GetValueAsObject(ThreatActorKey.SelectedKeyName));
	if (!Threat)
	{
		return EBTNodeResult::Failed;
	}

	// Tahdiddan TESKARI yo'nalish.
	const FVector AwayDir = (Pawn->GetActorLocation() - Threat->GetActorLocation()).GetSafeNormal2D();
	const FVector DesiredPoint = Pawn->GetActorLocation() + AwayDir * FleeDistance;

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	// Aynan o'sha nuqta NavMesh'da bo'lmasligi mumkin — atrofidan eng yaqinini olamiz.
	FNavLocation Projected;
	if (!NavSys->ProjectPointToNavigation(DesiredPoint, Projected, FVector(500.f, 500.f, 500.f)))
	{
		// Bo'lmasa, hech bo'lmasa atrofidagi tasodifiy nuqtaga qochsin.
		if (!NavSys->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), FleeDistance, Projected))
		{
			return EBTNodeResult::Failed;
		}
	}

	BB->SetValueAsVector(FleeLocationKey.SelectedKeyName, Projected.Location);
	return EBTNodeResult::Succeeded;
}
