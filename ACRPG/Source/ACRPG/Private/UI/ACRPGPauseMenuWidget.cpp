#include "UI/ACRPGPauseMenuWidget.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGPlayerController.h"
#include "Core/ACRPGSaveSubsystem.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UACRPGPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Har bir tugmaga: hover tovushi (bir xil) + o'z click ishlovchisi.
	if (ResumeButton)
	{
		ResumeButton->OnHovered.AddDynamic(this, &UACRPGPauseMenuWidget::OnAnyButtonHovered);
		ResumeButton->OnClicked.AddDynamic(this, &UACRPGPauseMenuWidget::OnResumeClicked);
	}
	if (SaveButton)
	{
		SaveButton->OnHovered.AddDynamic(this, &UACRPGPauseMenuWidget::OnAnyButtonHovered);
		SaveButton->OnClicked.AddDynamic(this, &UACRPGPauseMenuWidget::OnSaveClicked);
	}
	if (LoadButton)
	{
		LoadButton->OnHovered.AddDynamic(this, &UACRPGPauseMenuWidget::OnAnyButtonHovered);
		LoadButton->OnClicked.AddDynamic(this, &UACRPGPauseMenuWidget::OnLoadClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnHovered.AddDynamic(this, &UACRPGPauseMenuWidget::OnAnyButtonHovered);
		QuitButton->OnClicked.AddDynamic(this, &UACRPGPauseMenuWidget::OnQuitClicked);
	}

	// Saqlash yo'q bo'lsa "Yuklash" tugmasi o'chirilgan bo'lsin.
	if (LoadButton)
	{
		const UACRPGSaveSubsystem* SaveSys = UACRPGSaveSubsystem::Get(this);
		LoadButton->SetIsEnabled(SaveSys && SaveSys->DoesSaveExist());
	}

	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
	}
}

void UACRPGPauseMenuWidget::OnAnyButtonHovered()
{
	if (ButtonHoverSound)
	{
		UGameplayStatics::PlaySound2D(this, ButtonHoverSound);
	}
}

void UACRPGPauseMenuWidget::PlayClickSound() const
{
	if (ButtonClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ButtonClickSound);
	}
}

// ---------------------------------------------------------------------------
// TUGMALAR (Ep.#64)
// ---------------------------------------------------------------------------

void UACRPGPauseMenuWidget::OnResumeClicked()
{
	PlayClickSound();

	if (AACRPGPlayerController* PC = Cast<AACRPGPlayerController>(GetOwningPlayer()))
	{
		PC->TogglePauseMenu();	// menyuni yopadi va pauzani bekor qiladi
	}
}

void UACRPGPauseMenuWidget::OnSaveClicked()
{
	PlayClickSound();

	UACRPGSaveSubsystem* SaveSys = UACRPGSaveSubsystem::Get(this);
	const bool bSuccess = SaveSys && SaveSys->SaveGame();

	if (StatusText)
	{
		StatusText->SetText(bSuccess
			? NSLOCTEXT("ACRPG", "SaveOK", "O'yin saqlandi")
			: NSLOCTEXT("ACRPG", "SaveFail", "Saqlab bo'lmadi"));
	}

	if (bSuccess && LoadButton)
	{
		LoadButton->SetIsEnabled(true);
	}
}

void UACRPGPauseMenuWidget::OnLoadClicked()
{
	PlayClickSound();

	UACRPGSaveSubsystem* SaveSys = UACRPGSaveSubsystem::Get(this);
	if (!SaveSys || !SaveSys->LoadGame())
	{
		if (StatusText)
		{
			StatusText->SetText(NSLOCTEXT("ACRPG", "LoadFail", "Yuklab bo'lmadi"));
		}
		return;
	}

	// Yuklangach menyuni yopamiz.
	if (AACRPGPlayerController* PC = Cast<AACRPGPlayerController>(GetOwningPlayer()))
	{
		PC->TogglePauseMenu();
	}
}

void UACRPGPauseMenuWidget::OnQuitClicked()
{
	PlayClickSound();

	// Chiqishdan oldin pauzani bekor qilamiz — aks holda yangi xarita muzlab qoladi.
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
