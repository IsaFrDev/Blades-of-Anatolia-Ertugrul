// ACRPGHUDWidget.h — Ep.#8 (Sounds and UI), #37 (kvest markeri),
//                     #51 (o'q soni), #76 (hudud xabari), #77 (level up)
//
// C++ / UMG ISH BO'LINISHI (bu qism ko'p odamni chalkashtiradi):
//
//   C++ tomonda:  mantiq, hisob-kitob, event'larga obuna bo'lish
//   UMG tomonda:  chizish, animatsiya, joylashuv
//
// Bog'lovchi — meta=(BindWidget). Bu makro "Blueprint'da AYNAN shu nomli
// widget bo'lishi shart" degani. Bo'lmasa loyiha kompilyatsiya bo'lmaydi —
// ya'ni xato ish vaqtida emas, editor'da darhol chiqadi.
//
// Blueprint'da WBP_HUD yaratib, uni shu klassdan meros oldirasiz va ichiga
// HealthBar, StaminaBar... nomli elementlar qo'yasiz.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ACRPGHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UWidgetAnimation;
class UACRPGStatsComponent;
class UACRPGQuestComponent;
class UACRPGEquipmentComponent;

UCLASS(Abstract)
class ACRPG_API UACRPGHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Ep.#76 — hudud nomini ko'rsatadi va o'chiradi. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|HUD")
	void ShowAreaMessage(const FText& AreaName, bool bHostile);

	/** Ep.#77 — daraja ko'tarilganda. */
	UFUNCTION(BlueprintImplementableEvent, Category = "ACRPG|HUD")
	void PlayLevelUpEffect(int32 NewLevel);

	/** Blueprint'da animatsiya o'ynatish uchun. */
	UFUNCTION(BlueprintImplementableEvent, Category = "ACRPG|HUD")
	void OnAreaMessageShown(const FText& AreaName, bool bHostile);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// -----------------------------------------------------------------------
	// UMG BOG'LANISHLARI — Blueprint'da shu nomlar bilan element bo'lishi SHART
	// -----------------------------------------------------------------------

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> XPBar;

	/** BindWidgetOptional — bo'lmasa ham xato bermaydi. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AreaMessageText;

	/** Hudud xabari necha soniya turadi. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.5"))
	float AreaMessageDuration = 4.f;

	// -----------------------------------------------------------------------
	// EVENT ISHLOVCHILARI — komponentlar shularni chaqiradi
	// -----------------------------------------------------------------------

	UFUNCTION() void HandleHealthChanged(float NewHealth, float MaxHealth);
	UFUNCTION() void HandleStaminaChanged(float NewStamina, float MaxStamina);
	UFUNCTION() void HandleXPChanged(int32 CurrentXP, int32 XPToNext, int32 Level);
	UFUNCTION() void HandleLevelUp(int32 NewLevel);
	UFUNCTION() void HandleActiveQuestChanged(FName QuestID);
	UFUNCTION() void HandleObjectiveUpdated(FName QuestID, int32 ObjectiveIndex, int32 NewCount);
	UFUNCTION() void HandleAmmoChanged(int32 NewAmmo);

private:
	void RefreshQuestDisplay();
	void HideAreaMessage();

	UPROPERTY(Transient) TObjectPtr<UACRPGStatsComponent> Stats;
	UPROPERTY(Transient) TObjectPtr<UACRPGQuestComponent> Quests;
	UPROPERTY(Transient) TObjectPtr<UACRPGEquipmentComponent> Equipment;

	FTimerHandle AreaMessageTimer;
};
