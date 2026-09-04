#include "AI/ACRPGCivilianController.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGEquipmentComponent.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "TimerManager.h"

AACRPGCivilianController::AACRPGCivilianController()
{
	// Tinch aholi sezgir: shovqinni uzoqdan eshitadi.
	SightRadius = 800.f;
	LoseSightRadius = 1100.f;
	PeripheralVisionAngle = 140.f;
	HearingRange = 2500.f;
}

void AACRPGCivilianController::HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	AACRPGCharacterBase* SeenChar = Cast<AACRPGCharacterBase>(Actor);
	if (!SeenChar || !SeenChar->IsAlive())
	{
		return;
	}

	// --- Ep.#29 tuzatish: har kimni ko'rganda emas ---
	// Faqat QUROLLI yoki hujum qilayotgan odamdan qochadi.
	bool bThreatening = false;

	const ECombatState State = SeenChar->GetCombatState();
	if (State == ECombatState::Attacking || State == ECombatState::Assassinating)
	{
		bThreatening = true;
	}
	else if (const UACRPGEquipmentComponent* Equip =
		SeenChar->FindComponentByClass<UACRPGEquipmentComponent>())
	{
		bThreatening = Equip->IsWeaponDrawn();
	}

	if (!bThreatening)
	{
		return;
	}

	// Uzoqda bo'lsa e'tibor bermaydi.
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	const float Distance = FVector::Dist(MyPawn->GetActorLocation(), SeenChar->GetActorLocation());
	if (Distance > PanicDistance)
	{
		return;
	}

	// Qochamiz. Blackboard'dagi ThreatActor ni UBTTask_FindFleeLocation o'qiydi.
	SetTargetActor(SeenChar);
	SetAIState(EAIState::Fleeing);

	GetWorldTimerManager().SetTimer(
		CalmTimer, this, &AACRPGCivilianController::CalmDown, CalmDownTime, false);

	UE_LOG(LogACRPG, Verbose, TEXT("%s: qo'rqib qochdi."), *GetName());
}

void AACRPGCivilianController::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed() || GetAIState() == EAIState::Fleeing)
	{
		return;
	}

	// Shovqin eshitsa tekshirmaydi — qarama-qarshi tomonga yuradi.
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(ACRPGBlackboardKeys::InvestigateLocation, Stimulus.StimulusLocation);
	}

	SetAIState(EAIState::Fleeing);

	GetWorldTimerManager().SetTimer(
		CalmTimer, this, &AACRPGCivilianController::CalmDown, CalmDownTime, false);
}

void AACRPGCivilianController::CalmDown()
{
	if (GetAIState() == EAIState::Fleeing)
	{
		SetTargetActor(nullptr);
		SetAIState(EAIState::Patrolling);
	}
}
