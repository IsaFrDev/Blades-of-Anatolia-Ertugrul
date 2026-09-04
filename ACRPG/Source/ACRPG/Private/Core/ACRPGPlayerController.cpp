#include "Core/ACRPGPlayerController.h"

#include "Core/ACRPG.h"
#include "UI/ACRPGHUDWidget.h"
#include "UI/ACRPGPauseMenuWidget.h"

#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AACRPGPlayerController::AACRPGPlayerController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AACRPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// --- Enhanced Input: mapping context'larni yoqamiz ---
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			int32 Priority = 0;
			for (UInputMappingContext* IMC : DefaultMappingContexts)
			{
				if (IMC)
				{
					Subsystem->AddMappingContext(IMC, Priority++);
				}
			}
		}
	}

	// --- Ep.#8: HUD ---
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UACRPGHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
}

void AACRPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// Pauza tugmasi personajda emas, shu yerda — chunki personaj o'lganda ham ishlashi kerak.
}

/**
 * Ep.#78 — har bir tugma bosilganda UE bizga "bu geympadmi?" deb aytadi.
 * Shu orqali qurilma turini kuzatamiz va o'zgarganda UI'ga xabar beramiz.
 */
bool AACRPGPlayerController::InputKey(const FInputKeyEventArgs& EventArgs)
{
	const EInputDeviceType Detected = EventArgs.Key.IsGamepadKey()
		? EInputDeviceType::Gamepad
		: EInputDeviceType::KeyboardMouse;

	SetInputDevice(Detected);

	return Super::InputKey(EventArgs);
}

void AACRPGPlayerController::SetInputDevice(EInputDeviceType NewDevice)
{
	if (CurrentDevice == NewDevice)
	{
		return;	// Har kadrda broadcast qilmaymiz — faqat o'zgarganda.
	}

	CurrentDevice = NewDevice;
	OnInputDeviceChanged.Broadcast(CurrentDevice);

	// Geympadda sichqoncha kursori kerak emas.
	bShowMouseCursor = (CurrentDevice == EInputDeviceType::KeyboardMouse) && bShowMouseCursor;
}

void AACRPGPlayerController::TogglePauseMenu()
{
	if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;

		UGameplayStatics::SetGamePaused(this, false);
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
		return;
	}

	if (!PauseMenuWidgetClass)
	{
		UE_LOG(LogACRPG, Warning, TEXT("PauseMenuWidgetClass tayinlanmagan."));
		return;
	}

	PauseMenuWidget = CreateWidget<UACRPGPauseMenuWidget>(this, PauseMenuWidgetClass);
	if (PauseMenuWidget)
	{
		PauseMenuWidget->AddToViewport(10);

		UGameplayStatics::SetGamePaused(this, true);

		// GameAndUI — pauzada ham kamerani aylantirish mumkin bo'lsin desangiz.
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);

		bShowMouseCursor = true;
	}
}
