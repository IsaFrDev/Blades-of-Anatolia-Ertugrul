// Copyright (c) FayzInc. Diriliş: The Last March.
#include "Wound/ErtWoundComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

// Loyihaning boshqa modullaridan — bu yerda faqat interfeys darajasida
#include "Env/ErtWeatherSubsystem.h"
#include "Trauma/ErtTraumaSubsystem.h"
#include "Settings/ErtGameUserSettings.h"
#include "Save/ErtSaveSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogErtWound, Log, All);

namespace ErtWoundTuning
{
	// Parry oynasi chegaralari (millisekund)
	constexpr float ParryIntactMs   = 180.f;
	constexpr float ParryFloorMs    = 110.f;   // eng yomon holat
	constexpr float ParryCeilingMs  = 165.f;   // to'liq sog'aygan (lekin <180)
	constexpr float ParryCurveGamma = 0.7f;    // chiziqli emas — og'riq tezroq o'sadi

	// Muhit ta'siri (birlik/sekund)
	constexpr float DrainCold       = -0.8f;   // BodyHeat < 40
	constexpr float DrainWet        = -0.5f;

	// Faza chegaralari
	constexpr float ProsthesisPhaseThreshold = 65.f;  // MaxIntegrity bundan yuqori = Adapted
	constexpr float FreshPhaseThreshold      = 25.f;  // HandIntegrity bundan past = Fresh

	// Qarilik / bog'liqlik
	constexpr float AgeingCeilingLossPerSeason = 5.f;
	constexpr int32 OpiumPenaltyEvery          = 5;
	constexpr float OpiumCeilingLoss           = 5.f;

	// Sabr
	constexpr float SabrHighThreshold = 70.f;
	constexpr float SabrLowThreshold  = 30.f;
	constexpr float SabrHighMul       = 0.4f;
	constexpr float SabrLowMul        = 2.0f;
}

UErtWoundComponent::UErtWoundComponent()
{
	PrimaryComponentTick.bCanEverTick        = true;
	PrimaryComponentTick.TickInterval        = 0.25f;  // 4 Hz yetarli — bu HP emas
	PrimaryComponentTick.bStartWithTickEnabled = false; // Intact fazada tick kerak emas
}

void UErtWoundComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const UWorld* World = GetWorld())
	{
		TraumaSubsystem = World->GetSubsystem<UErtTraumaSubsystem>();
	}
	RecomputePhase();
	SetComponentTickEnabled(Phase != EErtHandPhase::Intact);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SO'ROVLAR
// ═════════════════════════════════════════════════════════════════════════════

float UErtWoundComponent::GetParryWindowMs() const
{
	using namespace ErtWoundTuning;

	// EP001–EP023: qo'l butun. Faqat sovuq vaqtincha ta'sir qiladi
	// (bu — EP005 dagi "birinchi ta'm" — o'yinchi zo'rg'a sezadi).
	if (Phase == EErtHandPhase::Intact)
	{
		const float ColdPenalty = (SampleEnvironmentalDrain() < -0.5f) ? 30.f : 0.f;
		return ParryIntactMs - ColdPenalty;
	}

	// EP024+: chiziqli emas. Jarohat chuqurlashgani sari og'riq TEZROQ o'sadi,
	// shuning uchun `Pow(t, 0.7)` — pastki qiymatlarda egri chiziq keskinroq tushadi.
	const float T = FMath::Clamp(HandIntegrity / 100.f, 0.f, 1.f);
	return FMath::Lerp(ParryFloorMs, ParryCeilingMs, FMath::Pow(T, ParryCurveGamma));
}

int32 UErtWoundComponent::GetRemainingBowShots() const
{
	if (Phase == EErtHandPhase::Intact) return TNumericLimits<int32>::Max();

	// 100% → 12 o'q · 60% → 6 · 30% → 3 · 12% dan past → 0
	int32 Budget = 0;
	if      (HandIntegrity >= 60.f) Budget = 12;
	else if (HandIntegrity >= 30.f) Budget = 6;
	else if (HandIntegrity >= 12.f) Budget = 3;

	return FMath::Max(0, Budget - BowShotsSinceRest);
}

float UErtWoundComponent::GetFlashbackChance(float BaseChance) const
{
	using namespace ErtWoundTuning;

	// Mix qoqilmagunicha flashback yo'q — travma hali sodir bo'lmagan.
	if (Phase == EErtHandPhase::Intact) return 0.f;

	const float SabrMultiplier =
		(Sabr > SabrHighThreshold) ? SabrHighMul :
		(Sabr < SabrLowThreshold)  ? SabrLowMul  : 1.0f;

	return FMath::Clamp(BaseChance * SabrMultiplier, 0.f, 1.f);
}

float UErtWoundComponent::GetSocialExposureWeight() const
{
	// EP024 dan keyin qo'l — Ertug'rulning imzosi. Noyan uni shu bilan topgan.
	// Qo'lqop / latta / yeng bu og'irlikni kamaytiradi (ErtStealthComponent'da).
	if (Phase == EErtHandPhase::Intact) return 0.f;

	// Faza chuqurlashgani sari chandiq ko'proq taniladi (u qotib, oqarib boradi).
	switch (Phase)
	{
		case EErtHandPhase::Fresh:   return 0.9f;   // qonli bog'lam — juda ko'zga tashlanadi
		case EErtHandPhase::Chronic: return 0.7f;
		case EErtHandPhase::Adapted: return 0.5f;   // protez qisman yashiradi
		default:                     return 0.f;
	}
}

// ═════════════════════════════════════════════════════════════════════════════
//  O'ZGARTIRISHLAR
// ═════════════════════════════════════════════════════════════════════════════

void UErtWoundComponent::ApplyWound(const FErtWoundModifier& Modifier)
{
	if (Phase == EErtHandPhase::Intact && !Modifier.bRaisesCeiling)
	{
		// Butun qo'lga zarar yozilmaydi — lekin telemetriya uchun qayd etamiz,
		// chunki "EP005 da o'yinchi sovuqni sezdimi?" degan savol balansda muhim.
		return;
	}

	const float Old = HandIntegrity;

	if (Modifier.bRaisesCeiling)
	{
		MaxIntegrity = FMath::Clamp(MaxIntegrity + Modifier.InstantDelta, 0.f, 100.f);
		UE_LOG(LogErtWound, Log, TEXT("Ship ko'tarildi: %.1f (manba: %s)"),
		       MaxIntegrity, *Modifier.SourceTag.ToString());
	}
	else
	{
		HandIntegrity += Modifier.InstantDelta * GetUserDecayScale();
	}

	ClampAndBroadcast(Old);
}

void UErtWoundComponent::TriggerNailEvent(float NewCeiling)
{
	if (Phase != EErtHandPhase::Intact)
	{
		UE_LOG(LogErtWound, Warning, TEXT("TriggerNailEvent ikki marta chaqirildi — e'tiborsiz qoldirildi"));
		return;
	}

	const float Old = HandIntegrity;

	// ⚠️ Bu — o'yindagi eng muhim qator. Undan orqaga yo'l yo'q.
	HandIntegrity = 0.f;
	MaxIntegrity  = FMath::Clamp(NewCeiling, 0.f, 100.f);
	Phase         = EErtHandPhase::Fresh;
	Sabr          = FMath::Min(Sabr, 20.f);   // travma sabrni sindiradi

	SetComponentTickEnabled(true);

	// Butun jang tizimini chap qo'lga o'tkazadi:
	//   • ABP layer almashadi (mirror EMAS — alohida yozilgan animset)
	//   • «Qilich yo'li» skill daraxtining 6 ta mahorati o'chadi
	//   • «Sol Yol» daraxti 6 ta bepul ochilish bilan ochiladi
	//   • IMC_Combat → IMC_Combat_Left
	//   • Post_EP024 data layer yoqiladi
	if (TraumaSubsystem.IsValid())
	{
		TraumaSubsystem->OnNailDriven(GetOwner());
	}

	OnNailDriven.Broadcast();
	OnHandPhaseChanged.Broadcast(Phase);
	ClampAndBroadcast(Old);

	// Darhol saqlaymiz — o'yinchi bu lahzani "qayta yuklab" bekor qila olmasin.
	if (UWorld* World = GetWorld())
	{
		if (auto* SaveSys = World->GetGameInstance()->GetSubsystem<UErtSaveSubsystem>())
		{
			SaveSys->WriteIrreversibleCheckpoint(TEXT("EP024_NailDriven"));
		}
	}

	UE_LOG(LogErtWound, Warning, TEXT("MIX QOQILDI. Ship: %.0f. Bu nuqta qaytarilmaydi."), MaxIntegrity);
}

void UErtWoundComponent::ApplyProsthesis(float CeilingBonus)
{
	if (Phase == EErtHandPhase::Intact) return;

	MaxIntegrity = FMath::Clamp(MaxIntegrity + CeilingBonus, 0.f, 100.f);
	Phase        = EErtHandPhase::Adapted;

	OnHandPhaseChanged.Broadcast(Phase);
	UE_LOG(LogErtWound, Log, TEXT("Protez o'rnatildi. Yangi ship: %.0f"), MaxIntegrity);
}

void UErtWoundComponent::ModifySabr(float Delta)
{
	Sabr = FMath::Clamp(Sabr + Delta, 0.f, 100.f);
}

void UErtWoundComponent::OnSeasonAdvanced()
{
	if (Phase == EErtHandPhase::Intact) return;

	// Ertug'rul EP001 da ~28 yosh, EP048 da ~62. Qarilik shipni pasaytiradi.
	MaxIntegrity = FMath::Max(20.f, MaxIntegrity - ErtWoundTuning::AgeingCeilingLossPerSeason);
	ClampAndBroadcast(HandIntegrity);
}

// ═════════════════════════════════════════════════════════════════════════════
//  TICK
// ═════════════════════════════════════════════════════════════════════════════

void UErtWoundComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Phase == EErtHandPhase::Intact) return;

	const float Old = HandIntegrity;
	HandIntegrity += SampleEnvironmentalDrain() * GetUserDecayScale() * DeltaTime;
	ClampAndBroadcast(Old);
}

float UErtWoundComponent::SampleEnvironmentalDrain() const
{
	const UWorld* World = GetWorld();
	if (!World) return 0.f;

	const auto* Weather = World->GetSubsystem<UErtWeatherSubsystem>();
	if (!Weather) return 0.f;

	float Drain = 0.f;
	// S3 (1238–1243) qish arki — bu yerda sovuq asosiy antagonistlardan biri.
	if (Weather->GetBodyHeat(GetOwner()) < 40.f) Drain += ErtWoundTuning::DrainCold;
	if (Weather->IsWet(GetOwner()))              Drain += ErtWoundTuning::DrainWet;
	return Drain;
}

float UErtWoundComponent::GetUserDecayScale() const
{
	// Accessibility: o'yinchi mix tezligini 40–160% oralig'ida sozlashi mumkin.
	// Bu qiyinlik darajasidan MUSTAQIL — hikoyani his qilmoqchi bo'lgan
	// o'yinchi jangni qattiq qoldirib, jarohatni yumshatishi mumkin.
	if (const auto* Settings = UErtGameUserSettings::Get())
	{
		return FMath::Clamp(Settings->WoundDecayScale, 0.4f, 1.6f);
	}
	return 1.f;
}

// ═════════════════════════════════════════════════════════════════════════════
//  YORDAMCHI
// ═════════════════════════════════════════════════════════════════════════════

void UErtWoundComponent::ClampAndBroadcast(float OldValue)
{
	// ⚠️ Butun tizimning markaziy qoidasi: qo'l HECH QACHON shipdan oshmaydi.
	HandIntegrity = FMath::Clamp(HandIntegrity, 0.f, MaxIntegrity);

	const EErtHandPhase Before = Phase;
	RecomputePhase();

	if (Before != Phase)
	{
		OnHandPhaseChanged.Broadcast(Phase);
	}
	if (!FMath::IsNearlyEqual(OldValue, HandIntegrity, 0.01f))
	{
		OnHandIntegrityChanged.Broadcast(HandIntegrity, OldValue);
	}
}

void UErtWoundComponent::RecomputePhase()
{
	using namespace ErtWoundTuning;

	if (FMath::IsNearlyEqual(MaxIntegrity, 100.f))
	{
		Phase = EErtHandPhase::Intact;
	}
	else if (MaxIntegrity > ProsthesisPhaseThreshold)
	{
		Phase = EErtHandPhase::Adapted;   // protezdan keyin
	}
	else if (HandIntegrity < FreshPhaseThreshold)
	{
		Phase = EErtHandPhase::Fresh;
	}
	else
	{
		Phase = EErtHandPhase::Chronic;
	}
}
