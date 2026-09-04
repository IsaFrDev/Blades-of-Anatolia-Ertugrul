// ACRPGAIControllerBase.h — Ep.#20 (Behavior Trees), #21 (Detection), #24 (Hearing)
//
// AI ARXITEKTURASI — kim nima qiladi:
//
//   AIController  = MIYA. Sezgi (perception), Blackboard, Behavior Tree'ni ishga tushirish.
//   Behavior Tree = QAROR. "Ko'rdimi? Quv. Ko'rmadimi? Patrul qil."
//   BT Task/Service = HARAKAT. "Shu nuqtaga bor", "hujum qil".
//   Character     = TANA. Animatsiya, sog'liq, urish.
//
// Blueprint seriyasida sezgi mantiqi BP_EnemyCharacter ichida edi va bu noto'g'ri:
// personaj o'lganda uning "miyasi" ham o'chishi kerak, lekin tanasi qolishi mumkin.
//
// Blackboard kalitlari C++ da FName konstanta sifatida e'lon qilinadi — shunda
// noto'g'ri yozilgan kalit nomi kompilyatsiyada emas, hech bo'lmasa bitta joyda tuziladi.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGAIControllerBase.generated.h"

class UBehaviorTree;
class UBlackboardComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AACRPGCharacterBase;

/** Blackboard kalit nomlari — bitta joyda. */
namespace ACRPGBlackboardKeys
{
	const FName TargetActor		= TEXT("TargetActor");
	const FName AIState			= TEXT("AIState");
	const FName PatrolIndex		= TEXT("PatrolIndex");
	const FName PatrolLocation	= TEXT("PatrolLocation");
	const FName InvestigateLocation = TEXT("InvestigateLocation");
	const FName HomeLocation		= TEXT("HomeLocation");
	const FName AttackRange		= TEXT("AttackRange");
	const FName LastKnownLocation = TEXT("LastKnownLocation");
}

UCLASS(Abstract)
class ACRPG_API AACRPGAIControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	AACRPGAIControllerBase();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "ACRPG|AI")
	EAIState GetAIState() const;

	UFUNCTION(BlueprintCallable, Category = "ACRPG|AI")
	void SetAIState(EAIState NewState);

	UFUNCTION(BlueprintPure, Category = "ACRPG|AI")
	AActor* GetTargetActor() const;

	UFUNCTION(BlueprintCallable, Category = "ACRPG|AI")
	void SetTargetActor(AActor* NewTarget);

protected:
	/** Editor'da tayinlanadi: BT_Enemy, BT_Animal... */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> PerceptionComponent;

	// --- Ep.#21: ko'rish ---

	UPROPERTY(EditDefaultsOnly, Category = "Sezgi|Ko'rish", meta = (ClampMin = "100"))
	float SightRadius = 1200.f;

	/** Ko'rgandan keyin shu masofagacha "eslab qoladi". Radiusdan katta bo'lishi kerak. */
	UPROPERTY(EditDefaultsOnly, Category = "Sezgi|Ko'rish", meta = (ClampMin = "100"))
	float LoseSightRadius = 1600.f;

	/** Ko'rish konusi (to'liq burchak, gradus). 90 = har tomonga 45. */
	UPROPERTY(EditDefaultsOnly, Category = "Sezgi|Ko'rish", meta = (ClampMin = "10", ClampMax = "360"))
	float PeripheralVisionAngle = 90.f;

	/** Ko'rgandan keyin xotirada necha soniya turadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Sezgi|Ko'rish", meta = (ClampMin = "0"))
	float SightMaxAge = 5.f;

	// --- Ep.#24: eshitish ---

	UPROPERTY(EditDefaultsOnly, Category = "Sezgi|Eshitish", meta = (ClampMin = "100"))
	float HearingRange = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Sezgi|Eshitish", meta = (ClampMin = "0"))
	float HearingMaxAge = 3.f;

	/** Sezgi hodisasi kelganda. Hosila klasslar o'zgartiradi. */
	UFUNCTION()
	virtual void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Ko'rish orqali aniqlandi. */
	virtual void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus);

	/** Shovqin eshitildi (Ep.#24, #27). */
	virtual void HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus);

	/** Bu aktyor dushmanmi? (o'yinchi/tinch aholi/hayvon farqi) */
	virtual bool IsHostileTo(const AActor* Other) const;

	UPROPERTY(Transient)
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(Transient)
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(Transient)
	TObjectPtr<AACRPGCharacterBase> ControlledCharacter;

	/** Nishonni yo'qotgandan keyin necha soniya qidiradi. */
	UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0"))
	float SearchDuration = 8.f;

private:
	UFUNCTION()
	void OnControlledCharacterDeath(AActor* Killer);

	void SetupPerception();

	FTimerHandle SearchTimerHandle;
};
