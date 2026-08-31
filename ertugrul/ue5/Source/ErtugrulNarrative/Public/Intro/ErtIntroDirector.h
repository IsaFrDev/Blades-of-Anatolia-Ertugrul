// Copyright (c) FayzInc. Diriliş: The Last March.
// ─────────────────────────────────────────────────────────────────────────────
//  O'YNALADIGAN INTRO DIREKTORI  (Interactive Prologue Director)
//
//  ❗ Bu tizim pre-rendered `.mp4` intro'larni TO'LIQ ALMASHTIRADI.
//  48 epizodning har birida o'yinchi introni O'YNAYDI, tomosha qilmaydi.
//
//  BUZILMAS QOIDALAR:
//    1. Kamera hech qachon o'yinchidan olinmaydi (DirectorCameraWeight ≤ 0.35).
//    2. Boshqaruv doim bor — hatto EP024 da bog'langan holatda ham (kamera).
//    3. Yuklanish ekrani yo'q — Level Streaming bilan uzluksiz o'tish.
//    4. Yil HAR DOIM ko'rsatiladi (diegetik / overlay / ovozli).
//    5. Intro → gameplay o'tishi ko'rinmaydi. "Endi o'ynang" signali yo'q.
//    6. 90–400 soniya. Undan uzun bo'lsa — bu epizodning qismi, intro emas.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "ErtIntroDirector.generated.h"

class UErtIntroSequenceAsset;
class UErtRecapBuilder;
class AErtPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntroFinished, FName, EpisodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntroBeatAdvanced, int32, BeatIndex);

/** Sana kartasining ko'rinish parametrlari. */
USTRUCT(BlueprintType)
struct FErtDateCardParams
{
	GENERATED_BODY()

	/** "641 Muharram" — sozlamada hijriy o'chirilgan bo'lsa bo'sh. */
	UPROPERTY(BlueprintReadWrite) FString Hijri;
	/** "26 Iyun 1243" */
	UPROPERTY(BlueprintReadWrite) FString Gregorian;
	/** "KÖSE DAĞ" — faqat "To'liq" verbosity'da */
	UPROPERTY(BlueprintReadWrite) FText   Place;
	/** "Sivas — Erzincan yo'li" */
	UPROPERTY(BlueprintReadWrite) FText   SubCaption;

	UPROPERTY(BlueprintReadWrite) float FadeInSec  = 1.2f;
	UPROPERTY(BlueprintReadWrite) float HoldSec    = 4.5f;
	UPROPERTY(BlueprintReadWrite) float FadeOutSec = 1.8f;
};

UCLASS()
class ERTUGRULNARRATIVE_API UErtIntroDirector : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable) FOnIntroFinished     OnIntroFinished;
	UPROPERTY(BlueprintAssignable) FOnIntroBeatAdvanced OnIntroBeatAdvanced;

	/** Introni boshlaydi. Cutscene EMAS — boshqaruv o'yinchida qoladi. */
	UFUNCTION(BlueprintCallable, Category = "Intro")
	void BeginIntro(const UErtIntroSequenceAsset* Asset);

	/** Faqat qayta o'ynashda ko'rinadi (birinchi marta yashiringan). */
	UFUNCTION(BlueprintCallable, Category = "Intro")
	bool CanSkip() const;

	UFUNCTION(BlueprintCallable, Category = "Intro")
	void RequestSkip();

	UFUNCTION(BlueprintPure, Category = "Intro")
	bool IsIntroActive() const { return ActiveAsset != nullptr; }

protected:
	virtual void Tick(float DeltaTime);

private:
	/** Sana ko'rsatish — o'yinchi sozlamasiga to'liq bo'ysunadi. */
	void PresentDate(const UErtIntroSequenceAsset* Asset);

	/** Dunyodagi obyektni yoritish (kitoba, tanga, hujjat) — diegetik rejim. */
	void HighlightDiegeticActor(const FGameplayTag& ActorTag);

	void AdvanceBeat();
	bool BeatCompleted(const struct FErtIntroBeat& Beat) const;

	/**
	 * Introni yakunlaydi.
	 * ❗ HECH QANDAY "endi o'ynang" signali yo'q — faqat cheklovlar olinadi
	 *    va HUD 1.5 soniyada sekin paydo bo'ladi.
	 */
	void EndIntro();

	void PossessTemporaryPawn(TSubclassOf<APawn> PawnClass);   // EP010 = Turgut
	void RestoreOriginalPawn();

	UPROPERTY(Transient) TObjectPtr<const UErtIntroSequenceAsset> ActiveAsset = nullptr;
	UPROPERTY(Transient) TObjectPtr<UErtRecapBuilder>             RecapBuilder = nullptr;
	UPROPERTY(Transient) TWeakObjectPtr<APawn>                    CachedOriginalPawn;

	int32 CurrentBeat   = 0;
	float ElapsedSec    = 0.f;
	float BeatElapsed   = 0.f;
	bool  bNudgeIssued  = false;
};
