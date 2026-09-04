// ACRPGQuestComponent.h
//
// Ep.#34 (Quest System), #35 (Quest Data), #36 (Active Quest Selection),
// #37 (Objectives), #38 (Dialogue), #39 (Objective Completion),
// #41 (Eliminate Enemy), #42 (Treasure), #43 (Quest Completion), #65 (XP), #81 (Save)
//
// KVEST TIZIMINING ARXITEKTURASI
//
// Ma'lumot ikki qatlamda:
//   FACRPGQuestData     — o'zgarmas ta'rif (Data Table'da)
//   FACRPGQuestProgress — o'yinchining shu kvestdagi holati (saqlanadi)
//
// Bu ajratish muhim: Data Table'ni o'yin davomida o'zgartirib bo'lmaydi va kerak emas.
// Blueprint seriyasida ikkalasi aralashgan edi, shuning uchun Ep.#81 (saqlash)
// juda murakkab chiqqan.
//
// HODISALARNI SANASH
// Kvest maqsadlari "hodisalarga obuna". Dushman o'lganda EnemyCharacter
// NotifyEnemyKilled ni chaqiradi, biz esa mos maqsadlarni bir pog'ona oshiramiz.
// Kvest tizimi dushmanni bilmaydi, dushman kvestni bilmaydi — faqat hodisa orqali.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ACRPGTypes.h"
#include "ACRPGQuestComponent.generated.h"

class AACRPGCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestStarted, FName, QuestID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, FName, QuestID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectiveUpdated, FName, QuestID, int32, ObjectiveIndex, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveQuestChanged, FName, QuestID);

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGQuestComponent();

	// --- Event'lar (UI shularga ulanadi) ---
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnQuestStarted OnQuestStarted;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnQuestCompleted OnQuestCompleted;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnObjectiveUpdated OnObjectiveUpdated;
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event") FOnActiveQuestChanged OnActiveQuestChanged;

	// -----------------------------------------------------------------------
	// KVESTNI BOSHQARISH (Ep.#34, #36, #43)
	// -----------------------------------------------------------------------

	/** Ep.#34 — kvestni jurnalga qo'shadi va faol qiladi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	bool StartQuest(FName QuestID);

	/** Ep.#43 — barcha maqsadlar bajarilgan bo'lsa yakunlaydi va mukofot beradi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	bool CompleteQuest(FName QuestID);

	/** Ep.#36 — HUD da kuzatiladigan kvest. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	void SetActiveQuest(FName QuestID);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Kvest")
	FName GetActiveQuestID() const { return ActiveQuestID; }

	UFUNCTION(BlueprintPure, Category = "ACRPG|Kvest")
	EQuestState GetQuestState(FName QuestID) const;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Kvest")
	bool IsQuestActive(FName QuestID) const { return GetQuestState(QuestID) == EQuestState::Active; }

	/** Ep.#35 — jurnal oynasi uchun. */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Kvest")
	const TArray<FACRPGQuestProgress>& GetQuestLog() const { return QuestLog; }

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	bool GetQuestProgress(FName QuestID, FACRPGQuestProgress& OutProgress) const;

	// -----------------------------------------------------------------------
	// HODISALAR — maqsadlarni oshiradi (Ep.#39, #41, #42)
	// -----------------------------------------------------------------------

	/** Ep.#41 — dushman o'ldirilganda EnemyCharacter chaqiradi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	void NotifyEnemyKilled(FName EnemyTag);

	/** Ep.#42 — buyum olinganda. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	void NotifyItemCollected(FName ItemID, int32 Quantity = 1);

	/** Joyga yetib borilganda (AreaTrigger chaqiradi). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	void NotifyLocationReached(FName LocationTag);

	/** Ep.#38 — NPC bilan gaplashilganda. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	void NotifyTalkedTo(FName NPCTag);

	// -----------------------------------------------------------------------
	// O'ZARO TA'SIR (Ep.#38, #42)
	// -----------------------------------------------------------------------

	/** "E" bosilganda oldimizdagi obyektni topib ishga soladi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	bool TryInteract();

	/** Ep.#37 — faol kvestning birinchi bajarilmagan maqsadi (HUD marker uchun). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Kvest")
	bool GetCurrentObjective(FACRPGQuestObjective& OutObjective) const;

	// --- Ep.#81: saqlash ---

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Save")
	void LoadFromSave(const TArray<FACRPGQuestProgress>& SavedLog, FName SavedActiveQuest);

protected:
	virtual void BeginPlay() override;

	/** O'yin boshida avtomatik beriladigan kvest. */
	UPROPERTY(EditDefaultsOnly, Category = "Kvest")
	FName StartingQuestID = NAME_None;

	/** "E" tugmasi qancha masofadan ishlaydi. */
	UPROPERTY(EditDefaultsOnly, Category = "O'zaro ta'sir", meta = (ClampMin = "50"))
	float InteractionRange = 250.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Kvest")
	TArray<FACRPGQuestProgress> QuestLog;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "Kvest")
	FName ActiveQuestID = NAME_None;

private:
	/** Barcha faol kvestlarda mos maqsadlarni bir pog'ona oshiradi. */
	void AdvanceObjectives(EQuestObjectiveType Type, FName TargetTag, int32 Amount);

	/** Kvestning barcha maqsadlari bajarildimi? Bajarilgan bo'lsa yakunlaydi. */
	void CheckQuestCompletion(FACRPGQuestProgress& Progress);

	FACRPGQuestProgress* FindProgress(FName QuestID);
	const FACRPGQuestProgress* FindProgress(FName QuestID) const;

	void GrantRewards(const struct FACRPGQuestData& QuestData);

	UPROPERTY(Transient)
	TObjectPtr<AACRPGCharacterBase> OwnerCharacter;
};
