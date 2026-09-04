// ACRPGDayNightCycle.h — Ep.#72 (Day Night Cycle), #81 (vaqtni saqlash)
//
// Xaritaga bitta shu aktyorni qo'yasiz va unga Directional Light + Sky Light +
// Sky Atmosphere ni tayinlaysiz.
//
// VAQT MODELI: TimeOfDay — 0..24 oralig'idagi float (soat).
//   0  = yarim tun
//   6  = quyosh chiqishi
//   12 = tush
//   18 = quyosh botishi
//
// Quyosh burchagi: Pitch = (TimeOfDay / 24) * 360 - 90
// Ya'ni soat 6 da Pitch = 0 (ufqda), soat 12 da Pitch = 90 (tepada).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACRPGDayNightCycle.generated.h"

class ADirectionalLight;
class ASkyLight;
class UCurveFloat;
class UCurveLinearColor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeOfDayChanged, float, NewTimeOfDay);

UCLASS()
class ACRPG_API AACRPGDayNightCycle : public AActor
{
	GENERATED_BODY()

public:
	AACRPGDayNightCycle();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event")
	FOnTimeOfDayChanged OnTimeOfDayChanged;

	UFUNCTION(BlueprintPure, Category = "ACRPG|Vaqt")
	float GetTimeOfDay() const { return TimeOfDay; }

	/** Ep.#81 — saqlangan vaqtni tiklash. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Vaqt")
	void SetTimeOfDay(float NewTime);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Vaqt")
	bool IsNight() const { return TimeOfDay < SunriseHour || TimeOfDay > SunsetHour; }

	/** "14:30" ko'rinishida — UI uchun. */
	UFUNCTION(BlueprintPure, Category = "ACRPG|Vaqt")
	FString GetTimeString() const;

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	/** Editor'da slayderni surganda darhol ko'rinsin. */
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	/** Boshlang'ich vaqt (soat). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vaqt", meta = (ClampMin = "0", ClampMax = "24"))
	float TimeOfDay = 12.f;

	/** Bitta o'yin kuni necha real daqiqa davom etadi. */
	UPROPERTY(EditAnywhere, Category = "Vaqt", meta = (ClampMin = "0.1"))
	float DayLengthMinutes = 20.f;

	UPROPERTY(EditAnywhere, Category = "Vaqt")
	bool bTimePaused = false;

	UPROPERTY(EditAnywhere, Category = "Vaqt", meta = (ClampMin = "0", ClampMax = "12"))
	float SunriseHour = 6.f;

	UPROPERTY(EditAnywhere, Category = "Vaqt", meta = (ClampMin = "12", ClampMax = "24"))
	float SunsetHour = 19.f;

	// --- Yoritish ---

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Yoritish")
	TObjectPtr<ADirectionalLight> SunLight;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Yoritish")
	TObjectPtr<ASkyLight> SkyLight;

	/** Vaqtga qarab quyosh yorqinligi (X: 0-24, Y: lux). */
	UPROPERTY(EditAnywhere, Category = "Yoritish")
	TObjectPtr<UCurveFloat> SunIntensityCurve;

	/** Vaqtga qarab quyosh rangi (tongda sariq, tushda oq, kechqurun qizil). */
	UPROPERTY(EditAnywhere, Category = "Yoritish")
	TObjectPtr<UCurveLinearColor> SunColorCurve;

	/** Quyosh o'qining Yaw burchagi (qaysi tomondan chiqadi). */
	UPROPERTY(EditAnywhere, Category = "Yoritish", meta = (ClampMin = "0", ClampMax = "360"))
	float SunYaw = 45.f;

private:
	void ApplyTimeOfDay();

	float LastBroadcastHour = -1.f;
};
