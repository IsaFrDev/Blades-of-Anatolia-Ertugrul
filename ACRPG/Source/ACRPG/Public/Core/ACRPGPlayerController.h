// ACRPGPlayerController.h — Ep.#8 (HUD), Ep.#64 (pause menu), Ep.#78 (geympad).
//
// Ep.#78 Blueprint'da "Controller Support" deb alohida input action'lar qo'shilgan edi.
// Enhanced Input'da bunga hojat yo'q: bitta IA_Move'ga ham WASD, ham stik bog'lanadi.
// Bu klass faqat ikki narsani qiladi:
//   1) Input Mapping Context'ni ro'yxatdan o'tkazadi;
//   2) Oxirgi kiritish klaviaturadanmi yoki geympaddanmi — kuzatadi (UI ikonkalarini
//      almashtirish uchun).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ACRPGPlayerController.generated.h"

class UInputMappingContext;
class UACRPGHUDWidget;
class UACRPGPauseMenuWidget;

UENUM(BlueprintType)
enum class EInputDeviceType : uint8
{
	KeyboardMouse	UMETA(DisplayName = "Klaviatura/sichqoncha"),
	Gamepad			UMETA(DisplayName = "Geympad")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputDeviceChanged, EInputDeviceType, NewDevice);

UCLASS()
class ACRPG_API AACRPGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AACRPGPlayerController();

	/** Ep.#78 — UI shu delegate'ga ulanib, tugma ikonkalarini almashtiradi. */
	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Input")
	FOnInputDeviceChanged OnInputDeviceChanged;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Input")
	EInputDeviceType GetCurrentInputDevice() const { return CurrentDevice; }

	/** Ep.#64 — pauza menyusini ochish/yopish. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|UI")
	void TogglePauseMenu();

	/** Ep.#8 — asosiy HUD (HP/stamina/XP paneli). */
	UFUNCTION(BlueprintPure, Category = "ACRPG|UI")
	UACRPGHUDWidget* GetHUDWidget() const { return HUDWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;

	/** IMC_Default — Project Settings emas, shu yerda tayinlanadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UACRPGHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UACRPGPauseMenuWidget> PauseMenuWidgetClass;

private:
	void SetInputDevice(EInputDeviceType NewDevice);

	UPROPERTY()
	TObjectPtr<UACRPGHUDWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<UACRPGPauseMenuWidget> PauseMenuWidget;

	EInputDeviceType CurrentDevice = EInputDeviceType::KeyboardMouse;
};
