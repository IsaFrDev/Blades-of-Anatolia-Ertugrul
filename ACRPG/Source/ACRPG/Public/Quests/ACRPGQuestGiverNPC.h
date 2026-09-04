// ACRPGQuestGiverNPC.h — Ep.#38 (Quest Dialogue), #40 (Head Look At), #43 (Completion)
//
// Kvest beruvchi NPC uch holatda bo'ladi:
//   1) Kvest berishga tayyor      -> gaplashsa kvest boshlanadi
//   2) Kvest ketmoqda             -> eslatma gapiradi
//   3) Kvest bajarilgan, topshirish -> mukofot beradi
//
// Ep.#40 — Control Rig bilan boshni o'yinchiga burish. C++ da biz faqat
// "qayerga qarash kerak" degan qiymatni hisoblaymiz; burishni Anim BP dagi
// Control Rig tuguni bajaradi.

#pragma once

#include "CoreMinimal.h"
#include "Character/ACRPGCharacterBase.h"
#include "Quests/ACRPGInteractableInterface.h"
#include "ACRPGQuestGiverNPC.generated.h"

class UWidgetComponent;

UCLASS()
class ACRPG_API AACRPGQuestGiverNPC : public AACRPGCharacterBase, public IACRPGInteractableInterface
{
	GENERATED_BODY()

public:
	AACRPGQuestGiverNPC();

	virtual void Tick(float DeltaSeconds) override;

	// --- Interfeys ---
	virtual void OnInteract_Implementation(AACRPGCharacterBase* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	/**
	 * Ep.#40 — Anim BP shu qiymatni Control Rig ga uzatadi.
	 * Boshning nishonga burilish burchagi (Pitch/Yaw).
	 */
	UFUNCTION(BlueprintPure, Category = "ACRPG|NPC")
	FRotator GetHeadLookAtRotation() const { return HeadLookRotation; }

	UFUNCTION(BlueprintPure, Category = "ACRPG|NPC")
	float GetHeadLookAlpha() const { return HeadLookAlpha; }

protected:
	virtual void BeginPlay() override;

	/** Bu NPC qaysi kvestni beradi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kvest")
	FName QuestToGive = NAME_None;

	/** Ep.#38 — dialog oynasi. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialog")
	TSubclassOf<class UUserWidget> DialogueWidgetClass;

	/** Kvest ketayotganda aytadigan gap. */
	UPROPERTY(EditAnywhere, Category = "Dialog", meta = (MultiLine = true))
	FText InProgressLine;

	/** Kvest topshirilgach aytadigan gap. */
	UPROPERTY(EditAnywhere, Category = "Dialog", meta = (MultiLine = true))
	FText CompletedLine;

	/** Boshi ustidagi "!" / "?" belgisi. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UWidgetComponent> QuestMarkerWidget;

	// --- Ep.#40: Head Look At ---

	UPROPERTY(EditDefaultsOnly, Category = "Head Look", meta = (ClampMin = "100"))
	float HeadLookRange = 500.f;

	/** Bosh maksimal shuncha buriladi (gradus) — bo'yin sinmasin. */
	UPROPERTY(EditDefaultsOnly, Category = "Head Look", meta = (ClampMin = "10", ClampMax = "90"))
	float MaxHeadYaw = 70.f;

	UPROPERTY(EditDefaultsOnly, Category = "Head Look", meta = (ClampMin = "1"))
	float HeadLookInterpSpeed = 5.f;

private:
	void UpdateHeadLookAt(float DeltaSeconds);
	void UpdateQuestMarker();

	FRotator HeadLookRotation = FRotator::ZeroRotator;
	float HeadLookAlpha = 0.f;
};
