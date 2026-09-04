#include "AI/ACRPGAIControllerBase.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

AACRPGAIControllerBase::AACRPGAIControllerBase()
{
	PrimaryActorTick.bCanEverTick = false;

	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*PerceptionComponent);
}

void AACRPGAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledCharacter = Cast<AACRPGCharacterBase>(InPawn);

	SetupPerception();

	// Personaj o'lganda AI ni to'xtatamiz.
	if (ControlledCharacter)
	{
		if (UACRPGStatsComponent* Stats = ControlledCharacter->GetStats())
		{
			Stats->OnDeath.AddDynamic(this, &AACRPGAIControllerBase::OnControlledCharacterDeath);
		}
	}

	// Behavior Tree ni ishga tushiramiz. RunBehaviorTree Blackboard'ni ham o'zi yaratadi.
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);

		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			// Boshlang'ich holat.
			BB->SetValueAsEnum(ACRPGBlackboardKeys::AIState, static_cast<uint8>(EAIState::Patrolling));
			BB->SetValueAsVector(ACRPGBlackboardKeys::HomeLocation, InPawn->GetActorLocation());
		}
	}
	else
	{
		UE_LOG(LogACRPG, Warning, TEXT("%s: BehaviorTreeAsset tayinlanmagan."), *GetName());
	}
}

void AACRPGAIControllerBase::OnUnPossess()
{
	if (PerceptionComponent)
	{
		PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(
			this, &AACRPGAIControllerBase::OnTargetPerceptionUpdated);
	}
	Super::OnUnPossess();
}

// ---------------------------------------------------------------------------
// SEZGI SOZLASH (Ep.#21, #24)
// ---------------------------------------------------------------------------

void AACRPGAIControllerBase::SetupPerception()
{
	if (!PerceptionComponent)
	{
		return;
	}

	// --- Ko'rish (Ep.#21) ---
	SightConfig = NewObject<UAISenseConfig_Sight>(this, TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle * 0.5f;
	SightConfig->SetMaxAge(SightMaxAge);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = -1.f;

	// KIMNI ko'radi. Uchalasi ham kerak, aks holda hech kimni ko'rmaydi —
	// bu Ep.#21 da eng ko'p uchraydigan xato.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComponent->ConfigureSense(*SightConfig);

	// --- Eshitish (Ep.#24, #27) ---
	HearingConfig = NewObject<UAISenseConfig_Hearing>(this, TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(HearingMaxAge);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComponent->ConfigureSense(*HearingConfig);

	// Asosiy sezgi — ko'rish (nishon tanlashda ustuvor).
	PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());

	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &AACRPGAIControllerBase::OnTargetPerceptionUpdated);
}

// ---------------------------------------------------------------------------
// SEZGI HODISALARI
// ---------------------------------------------------------------------------

void AACRPGAIControllerBase::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || GetAIState() == EAIState::Dead)
	{
		return;
	}

	// Qaysi sezgi ishladi?
	const TSubclassOf<UAISense> SenseClass =
		UAIPerceptionSystem::GetSenseClassForStimulus(this, Stimulus);

	if (SenseClass == UAISense_Sight::StaticClass())
	{
		HandleSightStimulus(Actor, Stimulus);
	}
	else if (SenseClass == UAISense_Hearing::StaticClass())
	{
		HandleHearingStimulus(Actor, Stimulus);
	}
}

void AACRPGAIControllerBase::HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (!IsHostileTo(Actor))
	{
		return;
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// KO'RDI -> quvishga o'tamiz.
		GetWorldTimerManager().ClearTimer(SearchTimerHandle);

		SetTargetActor(Actor);
		SetAIState(EAIState::Chasing);
		BB->SetValueAsVector(ACRPGBlackboardKeys::LastKnownLocation, Actor->GetActorLocation());

		UE_LOG(LogACRPG, Verbose, TEXT("%s: nishonni ko'rdi."), *GetName());
	}
	else
	{
		// YO'QOTDI -> darhol patrulga qaytmaymiz, avval qidiramiz (Ep.#21).
		// Bu AI ni ancha ishonarli qiladi.
		BB->SetValueAsVector(ACRPGBlackboardKeys::LastKnownLocation, Actor->GetActorLocation());
		BB->SetValueAsVector(ACRPGBlackboardKeys::InvestigateLocation, Actor->GetActorLocation());
		SetAIState(EAIState::Investigating);

		GetWorldTimerManager().SetTimer(SearchTimerHandle, [this]()
		{
			// Qidiruv vaqti tugadi va hali ham topa olmadi -> patrulga.
			if (GetAIState() == EAIState::Investigating)
			{
				SetTargetActor(nullptr);
				SetAIState(EAIState::Patrolling);
			}
		}, SearchDuration, false);
	}
}

void AACRPGAIControllerBase::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	// Quvib ketayotgan bo'lsa, shovqin uni chalg'itmaydi.
	if (GetAIState() == EAIState::Chasing || GetAIState() == EAIState::Attacking)
	{
		return;
	}

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	// Ep.#27 — shovqin joyiga borib tekshiradi.
	BB->SetValueAsVector(ACRPGBlackboardKeys::InvestigateLocation, Stimulus.StimulusLocation);
	SetAIState(EAIState::Investigating);

	GetWorldTimerManager().SetTimer(SearchTimerHandle, [this]()
	{
		if (GetAIState() == EAIState::Investigating)
		{
			SetAIState(EAIState::Patrolling);
		}
	}, SearchDuration, false);

	UE_LOG(LogACRPG, Verbose, TEXT("%s: shovqin eshitdi %s"),
		*GetName(), *Stimulus.StimulusLocation.ToCompactString());
}

bool AACRPGAIControllerBase::IsHostileTo(const AActor* Other) const
{
	// Bazada: o'yinchi dushman. Hosila klasslar buni o'zgartiradi
	// (masalan hayvonlar hammaga betaraf — Ep.#57).
	const AACRPGCharacterBase* OtherChar = Cast<AACRPGCharacterBase>(Other);
	return OtherChar && OtherChar->CharacterTag == TEXT("Player") && OtherChar->IsAlive();
}

// ---------------------------------------------------------------------------
// BLACKBOARD YORDAMCHILARI
// ---------------------------------------------------------------------------

EAIState AACRPGAIControllerBase::GetAIState() const
{
	if (const UBlackboardComponent* BB = GetBlackboardComponent())
	{
		return static_cast<EAIState>(BB->GetValueAsEnum(ACRPGBlackboardKeys::AIState));
	}
	return EAIState::Passive;
}

void AACRPGAIControllerBase::SetAIState(EAIState NewState)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsEnum(ACRPGBlackboardKeys::AIState, static_cast<uint8>(NewState));
	}
}

AActor* AACRPGAIControllerBase::GetTargetActor() const
{
	if (const UBlackboardComponent* BB = GetBlackboardComponent())
	{
		return Cast<AActor>(BB->GetValueAsObject(ACRPGBlackboardKeys::TargetActor));
	}
	return nullptr;
}

void AACRPGAIControllerBase::SetTargetActor(AActor* NewTarget)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(ACRPGBlackboardKeys::TargetActor, NewTarget);
	}
}

void AACRPGAIControllerBase::OnControlledCharacterDeath(AActor* Killer)
{
	SetAIState(EAIState::Dead);

	// Behavior Tree ni to'xtatamiz — o'lgan dushman yugurmasin (Ep.#82 bug fix).
	if (UBrainComponent* Brain = GetBrainComponent())
	{
		Brain->StopLogic(TEXT("O'ldi"));
	}

	if (PerceptionComponent)
	{
		PerceptionComponent->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
		PerceptionComponent->SetSenseEnabled(UAISense_Hearing::StaticClass(), false);
	}

	GetWorldTimerManager().ClearTimer(SearchTimerHandle);
}
