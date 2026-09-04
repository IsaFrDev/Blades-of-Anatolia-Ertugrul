// ACRPGPauseMenuWidget.h — Ep.#64 (Main Menu and Pause Menu), #74 (tugma tovushlari)
//
// Pauza menyusi: Davom etish / Saqlash / Yuklash / Sozlamalar / Chiqish.
//
// Ep.#74 — har bir tugmaga hover va click tovushi. Buni har bir tugmaga
// alohida qo'shish o'rniga, NativeConstruct'da avtomatik ulaymiz.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ACRPGPauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class USoundBase;

UCLASS(Abstract)
class ACRPG_API UACRPGPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// --- UMG bog'lanishlari ---
	UPROPERTY(meta = (BindWidget))			TObjectPtr<UButton> ResumeButton;
	UPROPERTY(meta = (BindWidgetOptional))	TObjectPtr<UButton> SaveButton;
	UPROPERTY(meta = (BindWidgetOptional))	TObjectPtr<UButton> LoadButton;
	UPROPERTY(meta = (BindWidgetOptional))	TObjectPtr<UButton> QuitButton;
	UPROPERTY(meta = (BindWidgetOptional))	TObjectPtr<UTextBlock> StatusText;

	// --- Ep.#74: tovushlar ---
	UPROPERTY(EditDefaultsOnly, Category = "Tovush") TObjectPtr<USoundBase> ButtonHoverSound;
	UPROPERTY(EditDefaultsOnly, Category = "Tovush") TObjectPtr<USoundBase> ButtonClickSound;

	/** Asosiy menyu xaritasining nomi. */
	UPROPERTY(EditDefaultsOnly, Category = "Menyu")
	FName MainMenuLevelName = TEXT("MainMenu");

	UFUNCTION() void OnResumeClicked();
	UFUNCTION() void OnSaveClicked();
	UFUNCTION() void OnLoadClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnAnyButtonHovered();

private:
	void PlayClickSound() const;
};
