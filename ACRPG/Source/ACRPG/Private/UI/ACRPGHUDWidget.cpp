#include "UI/ACRPGHUDWidget.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGGameInstance.h"
#include "Character/ACRPGPlayerCharacter.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGQuestComponent.h"
#include "Components/ACRPGEquipmentComponent.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UACRPGHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AACRPGPlayerCharacter* Player =
		Cast<AACRPGPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

	if (!Player)
	{
		UE_LOG(LogACRPG, Warning, TEXT("HUD: o'yinchi topilmadi."));
		return;
	}

	Stats = Player->GetStats();
	Quests = Player->GetQuests();
	Equipment = Player->GetEquipment();

	// --- Event'larga obuna bo'lamiz ---
	// Bu yondashuvning kuchi: HUD hech kimga o'zini majburlamaydi,
	// shunchaki "menga xabar bering" deydi.
	if (Stats)
	{
		Stats->OnHealthChanged.AddDynamic(this, &UACRPGHUDWidget::HandleHealthChanged);
		Stats->OnStaminaChanged.AddDynamic(this, &UACRPGHUDWidget::HandleStaminaChanged);
		Stats->OnXPChanged.AddDynamic(this, &UACRPGHUDWidget::HandleXPChanged);
		Stats->OnLevelUp.AddDynamic(this, &UACRPGHUDWidget::HandleLevelUp);

		// Boshlang'ich qiymatlar (event hali chiqmagan bo'lishi mumkin).
		HandleHealthChanged(Stats->GetHealth(), Stats->GetMaxHealth());
		HandleStaminaChanged(Stats->GetStamina(), Stats->GetMaxStamina());
		HandleXPChanged(Stats->GetCurrentXP(), Stats->GetXPToNextLevel(), Stats->GetLevel());
	}

	if (Quests)
	{
		Quests->OnActiveQuestChanged.AddDynamic(this, &UACRPGHUDWidget::HandleActiveQuestChanged);
		Quests->OnObjectiveUpdated.AddDynamic(this, &UACRPGHUDWidget::HandleObjectiveUpdated);
		RefreshQuestDisplay();
	}

	if (Equipment)
	{
		Equipment->OnAmmoChanged.AddDynamic(this, &UACRPGHUDWidget::HandleAmmoChanged);
		HandleAmmoChanged(Equipment->GetArrowCount());
	}

	if (AreaMessageText)
	{
		AreaMessageText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UACRPGHUDWidget::NativeDestruct()
{
	// Obunani bekor qilamiz — aks holda widget o'chgandan keyin ham
	// chaqirishga urinib, crash bo'ladi.
	if (Stats)
	{
		Stats->OnHealthChanged.RemoveDynamic(this, &UACRPGHUDWidget::HandleHealthChanged);
		Stats->OnStaminaChanged.RemoveDynamic(this, &UACRPGHUDWidget::HandleStaminaChanged);
		Stats->OnXPChanged.RemoveDynamic(this, &UACRPGHUDWidget::HandleXPChanged);
		Stats->OnLevelUp.RemoveDynamic(this, &UACRPGHUDWidget::HandleLevelUp);
	}
	if (Quests)
	{
		Quests->OnActiveQuestChanged.RemoveDynamic(this, &UACRPGHUDWidget::HandleActiveQuestChanged);
		Quests->OnObjectiveUpdated.RemoveDynamic(this, &UACRPGHUDWidget::HandleObjectiveUpdated);
	}
	if (Equipment)
	{
		Equipment->OnAmmoChanged.RemoveDynamic(this, &UACRPGHUDWidget::HandleAmmoChanged);
	}

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Ep.#8 — CHIZIQLAR
// ---------------------------------------------------------------------------

void UACRPGHUDWidget::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	if (HealthBar && MaxHealth > 0.f)
	{
		HealthBar->SetPercent(NewHealth / MaxHealth);
	}
}

void UACRPGHUDWidget::HandleStaminaChanged(float NewStamina, float MaxStamina)
{
	if (StaminaBar && MaxStamina > 0.f)
	{
		StaminaBar->SetPercent(NewStamina / MaxStamina);
	}
}

void UACRPGHUDWidget::HandleXPChanged(int32 CurrentXP, int32 XPToNext, int32 Level)
{
	if (XPBar)
	{
		// Joriy daraja ichidagi foiz (0 dan boshlab emas).
		int32 XPForCurrentLevel = 0;
		if (Stats)
		{
			XPForCurrentLevel = Stats->CalculateXPForLevel(Level);
		}

		const int32 Span = FMath::Max(1, XPToNext - XPForCurrentLevel);
		const int32 Progress = FMath::Max(0, CurrentXP - XPForCurrentLevel);

		XPBar->SetPercent(FMath::Clamp(static_cast<float>(Progress) / Span, 0.f, 1.f));
	}

	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(Level));
	}
}

void UACRPGHUDWidget::HandleLevelUp(int32 NewLevel)
{
	// Ep.#67, #77 — vizual effektni Blueprint bajaradi.
	PlayLevelUpEffect(NewLevel);
}

void UACRPGHUDWidget::HandleAmmoChanged(int32 NewAmmo)
{
	if (AmmoText)
	{
		AmmoText->SetText(FText::AsNumber(NewAmmo));	// Ep.#51
	}
}

// ---------------------------------------------------------------------------
// Ep.#36-37 — KVEST KO'RSATKICHI
// ---------------------------------------------------------------------------

void UACRPGHUDWidget::HandleActiveQuestChanged(FName QuestID)
{
	RefreshQuestDisplay();
}

void UACRPGHUDWidget::HandleObjectiveUpdated(FName QuestID, int32 ObjectiveIndex, int32 NewCount)
{
	// Faqat faol kvest o'zgarganda yangilaymiz.
	if (Quests && Quests->GetActiveQuestID() == QuestID)
	{
		RefreshQuestDisplay();
	}
}

void UACRPGHUDWidget::RefreshQuestDisplay()
{
	if (!Quests)
	{
		return;
	}

	const FName ActiveID = Quests->GetActiveQuestID();

	// Faol kvest yo'q — panelni yashiramiz.
	if (ActiveID.IsNone())
	{
		if (QuestNameText) QuestNameText->SetVisibility(ESlateVisibility::Hidden);
		if (ObjectiveText) ObjectiveText->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	if (QuestNameText)
	{
		QuestNameText->SetVisibility(ESlateVisibility::Visible);

		if (const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this))
		{
			if (const FACRPGQuestData* Data = GI->FindQuest(ActiveID))
			{
				QuestNameText->SetText(Data->QuestName);
			}
		}
	}

	if (ObjectiveText)
	{
		FACRPGQuestObjective Objective;
		if (Quests->GetCurrentObjective(Objective))
		{
			ObjectiveText->SetVisibility(ESlateVisibility::Visible);

			// "Qaroqchilarni yo'q qil (2/3)"
			const FText Formatted = FText::Format(
				NSLOCTEXT("ACRPG", "ObjectiveFormat", "{0} ({1}/{2})"),
				Objective.ObjectiveText,
				FText::AsNumber(Objective.CurrentCount),
				FText::AsNumber(Objective.RequiredCount));

			ObjectiveText->SetText(Formatted);
		}
		else
		{
			ObjectiveText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

// ---------------------------------------------------------------------------
// Ep.#76 — HUDUD XABARI
// ---------------------------------------------------------------------------

void UACRPGHUDWidget::ShowAreaMessage(const FText& AreaName, bool bHostile)
{
	if (AreaMessageText)
	{
		AreaMessageText->SetText(AreaName);
		AreaMessageText->SetVisibility(ESlateVisibility::Visible);

		// Dushman hududi — qizil.
		AreaMessageText->SetColorAndOpacity(
			bHostile ? FSlateColor(FLinearColor(0.9f, 0.2f, 0.2f))
					 : FSlateColor(FLinearColor::White));
	}

	// Blueprint tomonda paydo bo'lish animatsiyasi o'ynaydi.
	OnAreaMessageShown(AreaName, bHostile);

	GetWorld()->GetTimerManager().SetTimer(
		AreaMessageTimer, this, &UACRPGHUDWidget::HideAreaMessage, AreaMessageDuration, false);
}

void UACRPGHUDWidget::HideAreaMessage()
{
	if (AreaMessageText)
	{
		AreaMessageText->SetVisibility(ESlateVisibility::Hidden);
	}
}
