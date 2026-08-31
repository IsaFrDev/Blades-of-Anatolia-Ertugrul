// Copyright (c) FayzInc. Diriliş: The Last March.
// ─────────────────────────────────────────────────────────────────────────────
//  MIX (Mıh) TIZIMI — o'yinning markaziy mexanikasi.
//
//  EP024 da Ertug'rulning o'ng kaftiga mix qoqiladi (tarixiy `ṣalb`/`tasmīr`
//  jazosi — saljuqiy xronikalarida eng ko'p tilga olinadigan qatl usuli).
//  Undan keyin qo'l HECH QACHON to'liq tuzalmaydi: `MaxIntegrity` 100 dan 55 ga
//  tushadi va faqat bir marta (EP043, yunon suyak protezi) 15 ga ko'tariladi.
//
//  Bu komponent butun o'yin bo'ylab quyidagilarga ta'sir qiladi:
//    • parry oynasi (180ms → 110ms)
//    • qurol tanlash (og'ir qurollar bloklanadi)
//    • kamon (o'q soni cheklanadi)
//    • parkur (arqonni tishlab ushlash)
//    • ijtimoiy stealth (qo'l — tanib olish belgisi)
//    • PTSD flashback ehtimoli
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ErtWoundComponent.generated.h"

class UErtTraumaSubsystem;

/** Jarohatning hayotiy fazasi — narrativ va mexanik ikkalasini boshqaradi. */
UENUM(BlueprintType)
enum class EErtHandPhase : uint8
{
	/** EP001–EP023. Mix hali qoqilmagan, lekin har epizodda ko'rinadi. */
	Intact   UMETA(DisplayName = "Butun"),
	/** EP024–EP027. Yangi jarohat: eng agressiv cheklovlar. */
	Fresh    UMETA(DisplayName = "Yangi jarohat"),
	/** EP028–EP042. Surunkali og'riq; o'yinchi moslashgan. */
	Chronic  UMETA(DisplayName = "Surunkali"),
	/** EP043–EP048. Protezdan keyin; jarohat endi belgi va afzallik. */
	Adapted  UMETA(DisplayName = "Moslashgan")
};

/** Bitta jarohat ta'siri. `SourceTag` telemetriya va balans uchun muhim. */
USTRUCT(BlueprintType)
struct FErtWoundModifier
{
	GENERATED_BODY()

	/** Wound.Source.Cold / Parry / BowDraw / Climb / NoyanStrike */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mih")
	FGameplayTag SourceTag;

	/** Uzluksiz ta'sir (sovuq, yomg'ir). Manfiy = pasayish. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mih")
	float DeltaPerSecond = 0.f;

	/** Bir martalik ta'sir (parry, zarba, dori). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mih")
	float InstantDelta = 0.f;

	/** true bo'lsa `MaxIntegrity` shipini ko'taradi — FAQAT EP043 protezi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mih")
	bool bRaisesCeiling = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnHandIntegrityChanged, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnHandPhaseChanged, EErtHandPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNailDriven);

UCLASS(ClassGroup = (Ertugrul), meta = (BlueprintSpawnableComponent))
class ERTUGRULGAMEPLAY_API UErtWoundComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UErtWoundComponent();

	// ══════════════════════════════════════════════════════════════════════
	//  HOLAT
	// ══════════════════════════════════════════════════════════════════════

	/** Joriy qo'l butunligi, 0–`MaxIntegrity`. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mih")
	float HandIntegrity = 100.f;

	/**
	 * Ship. Qo'l bundan yuqoriga KO'TARILMAYDI.
	 * 100 → 55 (EP024, mix) → 70 (EP043, protez).
	 * Har mavsumda −5 (qarilik), har 5-afyun −5 (bog'liqlik).
	 */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mih")
	float MaxIntegrity = 100.f;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mih")
	EErtHandPhase Phase = EErtHandPhase::Intact;

	/** Sabr — flashback'ga qarshi psixologik chidamlilik (0–100). */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mih")
	float Sabr = 50.f;

	/** Afyun ishlatishlar soni. Har 5-marta `MaxIntegrity` −5. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mih")
	int32 OpiumUses = 0;

	// ══════════════════════════════════════════════════════════════════════
	//  HODISALAR
	// ══════════════════════════════════════════════════════════════════════
	UPROPERTY(BlueprintAssignable, Category = "Mih") FOnHandIntegrityChanged OnHandIntegrityChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mih") FOnHandPhaseChanged     OnHandPhaseChanged;
	UPROPERTY(BlueprintAssignable, Category = "Mih") FOnNailDriven           OnNailDriven;

	// ══════════════════════════════════════════════════════════════════════
	//  API
	// ══════════════════════════════════════════════════════════════════════

	UFUNCTION(BlueprintCallable, Category = "Mih")
	void ApplyWound(const FErtWoundModifier& Modifier);

	/**
	 * EP024 — QAYTARIB BO'LMAYDIGAN nuqta.
	 * `HandIntegrity` = 0, `MaxIntegrity` = NewCeiling (55), faza = Fresh.
	 * Butun jang tizimini chap qo'lga o'tkazadi va darhol save yozadi.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mih")
	void TriggerNailEvent(float NewCeiling = 55.f);

	/** EP043 — o'yindagi YAGONA shipni ko'taruvchi hodisa (yunon suyak protezi). */
	UFUNCTION(BlueprintCallable, Category = "Mih")
	void ApplyProsthesis(float CeilingBonus = 15.f);

	/** Parry oynasi (millisekund). Jang tizimi har parry'da shuni so'raydi. */
	UFUNCTION(BlueprintPure, Category = "Mih")
	float GetParryWindowMs() const;

	/** Kamon tortish mumkinmi. 12% dan past — yo'q. */
	UFUNCTION(BlueprintPure, Category = "Mih")
	bool CanDrawBow() const { return HandIntegrity > 12.f; }

	/** Og'ir qurol (templar uzun qilichi, bolta) — 60% dan yuqorida. */
	UFUNCTION(BlueprintPure, Category = "Mih")
	bool CanWieldHeavy() const { return HandIntegrity > 60.f; }

	/** Kamondan otish mumkin bo'lgan o'q soni (og'riqdan oldin). */
	UFUNCTION(BlueprintPure, Category = "Mih")
	int32 GetRemainingBowShots() const;

	/** `Sabr` ga qarab flashback ehtimolini modulyatsiya qiladi. */
	UFUNCTION(BlueprintPure, Category = "Mih")
	float GetFlashbackChance(float BaseChance) const;

	/** Ijtimoiy stealth: qo'l tanib olish belgisi bo'lgani uchun fosh qilish og'irligi. */
	UFUNCTION(BlueprintPure, Category = "Mih")
	float GetSocialExposureWeight() const;

	UFUNCTION(BlueprintCallable, Category = "Mih")
	void ModifySabr(float Delta);

	/** Mavsum o'tishida chaqiriladi — qarilik shipni pasaytiradi. */
	UFUNCTION(BlueprintCallable, Category = "Mih")
	void OnSeasonAdvanced();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	void  RecomputePhase();
	void  ClampAndBroadcast(float OldValue);
	float SampleEnvironmentalDrain() const;

	/** Sozlamalardan: o'yinchi mix tezligini 40–160% oralig'ida sozlashi mumkin. */
	float GetUserDecayScale() const;

	UPROPERTY(Transient) TWeakObjectPtr<UErtTraumaSubsystem> TraumaSubsystem;

	/** Kamon otishlari — dam olgandan keyin tiklanadi. */
	int32 BowShotsSinceRest = 0;
};
