#include "World/ACRPGDayNightCycle.h"

#include "Core/ACRPG.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"

AACRPGDayNightCycle::AACRPGDayNightCycle()
{
	PrimaryActorTick.bCanEverTick = true;
	// Har kadr yangilash shart emas — 10 FPS yetarli va ancha tejaydi.
	PrimaryActorTick.TickInterval = 0.1f;
}

void AACRPGDayNightCycle::BeginPlay()
{
	Super::BeginPlay();
	ApplyTimeOfDay();
}

#if WITH_EDITOR
void AACRPGDayNightCycle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTimeOfDay();	// Editor'da darhol ko'rinadi
}
#endif

void AACRPGDayNightCycle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTimePaused || DayLengthMinutes <= 0.f)
	{
		return;
	}

	// 24 o'yin soati / (DayLengthMinutes * 60) real soniya
	const float HoursPerSecond = 24.f / (DayLengthMinutes * 60.f);

	TimeOfDay += HoursPerSecond * DeltaSeconds;

	// Sutka aylanadi.
	while (TimeOfDay >= 24.f)
	{
		TimeOfDay -= 24.f;
	}

	ApplyTimeOfDay();

	// Har soat boshida xabar beramiz (har kadr emas).
	const float CurrentHour = FMath::FloorToFloat(TimeOfDay);
	if (!FMath::IsNearlyEqual(CurrentHour, LastBroadcastHour))
	{
		LastBroadcastHour = CurrentHour;
		OnTimeOfDayChanged.Broadcast(TimeOfDay);
	}
}

void AACRPGDayNightCycle::SetTimeOfDay(float NewTime)
{
	TimeOfDay = FMath::Fmod(FMath::Max(0.f, NewTime), 24.f);
	ApplyTimeOfDay();
	OnTimeOfDayChanged.Broadcast(TimeOfDay);
}

void AACRPGDayNightCycle::ApplyTimeOfDay()
{
	if (!SunLight)
	{
		return;
	}

	UDirectionalLightComponent* SunComp =
		Cast<UDirectionalLightComponent>(SunLight->GetLightComponent());

	if (!SunComp)
	{
		return;
	}

	// --- Quyosh burchagi ---
	// Soat 0  -> Pitch -90 (yer ostida, tun)
	// Soat 6  -> Pitch 0   (ufqda, tong)
	// Soat 12 -> Pitch 90  (tepada, tush)
	// Soat 18 -> Pitch 180 -> normalizatsiyadan keyin -180 (botish)
	const float SunPitch = (TimeOfDay / 24.f) * 360.f - 90.f;

	SunLight->SetActorRotation(FRotator(SunPitch, SunYaw, 0.f));

	// --- Yorqinlik ---
	if (SunIntensityCurve)
	{
		SunComp->SetIntensity(SunIntensityCurve->GetFloatValue(TimeOfDay));
	}
	else
	{
		// Egri chiziq yo'q bo'lsa — oddiy formula.
		// Quyosh ufqdan yuqorida bo'lsa yorug', pastda bo'lsa qorong'i.
		const float SunHeight = FMath::Sin(FMath::DegreesToRadians(SunPitch));
		const float Intensity = FMath::Lerp(0.05f, 8.f, FMath::Clamp(SunHeight, 0.f, 1.f));
		SunComp->SetIntensity(Intensity);
	}

	// --- Rang ---
	if (SunColorCurve)
	{
		const FLinearColor Color = SunColorCurve->GetLinearColorValue(TimeOfDay);
		SunComp->SetLightColor(Color);
	}

	// --- Sky Light ni yangilaymiz ---
	// Bu qimmat amal, shuning uchun Tick Interval 0.1 s qilingan.
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			SkyComp->RecaptureSky();
		}
	}
}

FString AACRPGDayNightCycle::GetTimeString() const
{
	const int32 Hours = FMath::FloorToInt(TimeOfDay);
	const int32 Minutes = FMath::FloorToInt((TimeOfDay - Hours) * 60.f);
	return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}
