#include "Components/ACRPGStatsComponent.h"

#include "Core/ACRPG.h"
#include "GameFramework/Actor.h"

UACRPGStatsComponent::UACRPGStatsComponent()
{
	// Tiklanish uchun Tick kerak. Agar loyihangizda yuzlab AI bo'lsa,
	// ularga TickInterval = 0.2f qo'yish mumkin — sezilmaydi, lekin ancha tejaydi.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f;
}

void UACRPGStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	Stamina = MaxStamina;

	// UI birinchi kadrdayoq to'g'ri qiymatni ko'rsin.
	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
	OnXPChanged.Broadcast(CurrentXP, GetXPToNextLevel(), Level);
}

void UACRPGStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Health <= 0.f)
	{
		return;	// O'lganda hech narsa tiklanmaydi.
	}

	TimeSinceLastDamage += DeltaTime;
	TimeSinceLastStaminaUse += DeltaTime;

	// Ep.#6 — yugurish chidamlilikni yeydi.
	if (bIsSprinting)
	{
		const float Drain = SprintStaminaDrain * DeltaTime;
		if (!ConsumeStamina(Drain))
		{
			// Chidamlilik tugadi — yugurish avtomatik to'xtaydi.
			SetSprinting(false);
		}
	}

	HandleRegeneration(DeltaTime);
}

void UACRPGStatsComponent::HandleRegeneration(float DeltaTime)
{
	// --- Chidamlilik (Ep.#6) ---
	if (!bIsSprinting
		&& Stamina < MaxStamina
		&& TimeSinceLastStaminaUse >= StaminaRegenDelay)
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenRate * DeltaTime);
		OnStaminaChanged.Broadcast(Stamina, MaxStamina);
	}

	// --- Sog'liq (Ep.#60) ---
	if (HealthRegenRate > 0.f
		&& Health < MaxHealth
		&& TimeSinceLastDamage >= HealthRegenDelay)
	{
		Health = FMath::Min(MaxHealth, Health + HealthRegenRate * DeltaTime);
		OnHealthChanged.Broadcast(Health, MaxHealth);
	}
}

// ---------------------------------------------------------------------------
// SOG'LIQ
// ---------------------------------------------------------------------------

float UACRPGStatsComponent::GetHealthPercent() const
{
	return MaxHealth > 0.f ? Health / MaxHealth : 0.f;
}

void UACRPGStatsComponent::ApplyDamage(float RawDamage, AActor* DamageCauser)
{
	if (RawDamage <= 0.f || Health <= 0.f)
	{
		return;
	}

	// --- Ep.#44: zirh himoyasi ---
	// Foizli kamaytirish: Armor=0 -> 1.0 (to'liq urish); Armor=100 -> 0.5 (yarmi).
	const float Mitigation = ArmorConstant / (ArmorConstant + ArmorValue);
	const float FinalDamage = RawDamage * Mitigation;

	Health = FMath::Max(0.f, Health - FinalDamage);
	TimeSinceLastDamage = 0.f;

	OnHealthChanged.Broadcast(Health, MaxHealth);

	UE_LOG(LogACRPG, Verbose, TEXT("%s: %.1f urish (xom %.1f, zirh %.0f) -> HP %.1f"),
		*GetOwner()->GetName(), FinalDamage, RawDamage, ArmorValue, Health);

	if (Health <= 0.f)
	{
		// Bu yerda personajni o'ldirmaymiz — faqat xabar beramiz.
		// Kim nima qilishini ACRPGCharacterBase::HandleDeath hal qiladi.
		OnDeath.Broadcast(DamageCauser);
	}
}

void UACRPGStatsComponent::Heal(float Amount)
{
	if (Amount <= 0.f || Health <= 0.f)
	{
		return;
	}

	Health = FMath::Min(MaxHealth, Health + Amount);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

// ---------------------------------------------------------------------------
// CHIDAMLILIK (Ep.#6)
// ---------------------------------------------------------------------------

float UACRPGStatsComponent::GetStaminaPercent() const
{
	return MaxStamina > 0.f ? Stamina / MaxStamina : 0.f;
}

bool UACRPGStatsComponent::ConsumeStamina(float Amount)
{
	if (Amount <= 0.f)
	{
		return true;
	}

	if (Stamina < Amount)
	{
		return false;	// Yetmaydi — chaqiruvchi harakatni bekor qiladi.
	}

	Stamina -= Amount;
	TimeSinceLastStaminaUse = 0.f;
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
	return true;
}

void UACRPGStatsComponent::SetSprinting(bool bNewSprinting)
{
	// Chidamlilik bo'lmasa, yugurishga ruxsat bermaymiz.
	if (bNewSprinting && Stamina <= 0.f)
	{
		return;
	}
	bIsSprinting = bNewSprinting;
}

// ---------------------------------------------------------------------------
// TAJRIBA VA DARAJA (Ep.#7, #65, #67)
// ---------------------------------------------------------------------------

int32 UACRPGStatsComponent::CalculateXPForLevel(int32 TargetLevel) const
{
	if (TargetLevel <= 1)
	{
		return 0;
	}

	// Masalan BaseXP=100, Exponent=1.5:
	//   2-daraja -> 100 * 1^1.5  = 100
	//   3-daraja -> 100 * 2^1.5  = 283
	//   4-daraja -> 100 * 3^1.5  = 519
	// Ya'ni har daraja oldingisidan sezilarli qiyinroq.
	return FMath::RoundToInt(BaseXPPerLevel * FMath::Pow(static_cast<float>(TargetLevel - 1), XPCurveExponent));
}

void UACRPGStatsComponent::AddXP(int32 Amount)
{
	if (Amount <= 0 || Level >= MaxLevel)
	{
		return;
	}

	CurrentXP += Amount;
	UE_LOG(LogACRPG, Log, TEXT("+%d XP (jami %d)"), Amount, CurrentXP);

	CheckLevelUp();
	OnXPChanged.Broadcast(CurrentXP, GetXPToNextLevel(), Level);
}

void UACRPGStatsComponent::CheckLevelUp()
{
	// `while` — bitta katta mukofot bir necha daraja ko'tarishi mumkin.
	while (Level < MaxLevel && CurrentXP >= CalculateXPForLevel(Level + 1))
	{
		++Level;

		// Ep.#67 — daraja ko'tarilganda statistika oshadi va to'liq tiklanadi.
		MaxHealth += HealthPerLevel;
		MaxStamina += StaminaPerLevel;
		Health = MaxHealth;
		Stamina = MaxStamina;

		OnHealthChanged.Broadcast(Health, MaxHealth);
		OnStaminaChanged.Broadcast(Stamina, MaxStamina);

		// Ep.#67, #77 — animatsiya va Niagara effekti shu event'ga ulanadi.
		OnLevelUp.Broadcast(Level);

		UE_LOG(LogACRPG, Log, TEXT("DARAJA KO'TARILDI -> %d"), Level);
	}
}

// ---------------------------------------------------------------------------
// SAQLASH (Ep.#80)
// ---------------------------------------------------------------------------

void UACRPGStatsComponent::LoadFromSave(float InHealth, float InStamina, int32 InLevel, int32 InXP)
{
	Level = FMath::Clamp(InLevel, 1, MaxLevel);
	CurrentXP = FMath::Max(0, InXP);

	// Daraja bo'yicha maksimal qiymatlarni qayta hisoblaymiz.
	// (Aks holda 10-darajali o'yinchi 100 HP bilan yuklanadi — Ep.#82 dagi bug.)
	const int32 LevelsGained = Level - 1;
	MaxHealth += HealthPerLevel * LevelsGained;
	MaxStamina += StaminaPerLevel * LevelsGained;

	Health = FMath::Clamp(InHealth, 0.f, MaxHealth);
	Stamina = FMath::Clamp(InStamina, 0.f, MaxStamina);

	OnHealthChanged.Broadcast(Health, MaxHealth);
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
	OnXPChanged.Broadcast(CurrentXP, GetXPToNextLevel(), Level);
}
