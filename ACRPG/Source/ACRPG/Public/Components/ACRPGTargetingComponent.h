// ACRPGTargetingComponent.h — Ep.#12 "Target Lock and Dodge Roll"
//
// Ikki mexanika bitta komponentda, chunki ular bir-biriga bog'liq:
// lock-on paytida dodge nishonga nisbatan qilinadi (yon tomonga aylanib o'tish),
// lock-onsiz esa kirish yo'nalishi bo'yicha.
//
// Nishon tanlash mezoni — faqat masofa emas:
//   Ball = Masofa + Burchak_jarimasi
// Ya'ni ekran markaziga yaqin dushman, yonroqdagi yaqinroq dushmandan afzal.
// Bu "eng yaqinini olaver" dan ancha tabiiy his qilinadi.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACRPGTargetingComponent.generated.h"

class AACRPGCharacterBase;
class UAnimMontage;
class UUserWidget;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetChanged, AActor*, NewTarget);

UCLASS(ClassGroup = (ACRPG), meta = (BlueprintSpawnableComponent))
class ACRPG_API UACRPGTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRPGTargetingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "ACRPG|Event")
	FOnTargetChanged OnTargetChanged;

	// --- Lock-on ---

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Nishon")
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Nishon")
	void ClearTarget();

	/** Geympad stigi bilan qo'shni dushmanga o'tish. */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Nishon")
	void SwitchTarget(bool bRight);

	UFUNCTION(BlueprintPure, Category = "ACRPG|Nishon")
	bool HasTarget() const { return CurrentTarget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "ACRPG|Nishon")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	// --- Dodge (Ep.#12) ---

	UFUNCTION(BlueprintCallable, Category = "ACRPG|Dodge")
	void PerformDodge();

	UFUNCTION(BlueprintPure, Category = "ACRPG|Dodge")
	bool IsDodging() const { return bIsDodging; }

	/** Anim Notify: shu oynada urish o'tmaydi (i-frames). */
	UFUNCTION(BlueprintCallable, Category = "ACRPG|Dodge")
	void SetInvulnerable(bool bNewInvulnerable) { bInvulnerable = bNewInvulnerable; }

	UFUNCTION(BlueprintPure, Category = "ACRPG|Dodge")
	bool IsInvulnerable() const { return bInvulnerable; }

protected:
	virtual void BeginPlay() override;

	// --- Lock-on sozlamalari ---

	UPROPERTY(EditDefaultsOnly, Category = "Nishon", meta = (ClampMin = "100"))
	float MaxLockDistance = 1500.f;

	/** Nishon shu masofadan uzoqlashsa, lock avtomatik uziladi. */
	UPROPERTY(EditDefaultsOnly, Category = "Nishon", meta = (ClampMin = "100"))
	float BreakLockDistance = 2000.f;

	/** Ekran markazidan qanchalik chetdagi dushmanlar hisobga olinadi (gradus). */
	UPROPERTY(EditDefaultsOnly, Category = "Nishon", meta = (ClampMin = "10", ClampMax = "180"))
	float MaxLockAngle = 70.f;

	/** Burchak jarimasi og'irligi. Katta bo'lsa — ekran markazi kuchliroq afzal. */
	UPROPERTY(EditDefaultsOnly, Category = "Nishon", meta = (ClampMin = "0"))
	float AngleWeight = 15.f;

	/** Nishon ustida ko'rinadigan belgi (WBP_LockOnMarker). */
	UPROPERTY(EditDefaultsOnly, Category = "Nishon")
	TSubclassOf<UUserWidget> LockOnMarkerClass;

	// --- Dodge sozlamalari ---

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeForwardMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeBackwardMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeLeftMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge")
	TObjectPtr<UAnimMontage> DodgeRightMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = "0"))
	float DodgeStaminaCost = 25.f;

	/** Ketma-ket dodge oralig'i (spam qilib bo'lmasin). */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge", meta = (ClampMin = "0"))
	float DodgeCooldown = 0.6f;

private:
	AActor* FindBestTarget() const;
	void UpdateLockValidity();
	void SetTarget(AActor* NewTarget);

	UFUNCTION()
	void OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(Transient) TObjectPtr<AACRPGCharacterBase> OwnerCharacter;
	UPROPERTY(Transient) TObjectPtr<AActor> CurrentTarget;
	UPROPERTY(Transient) TObjectPtr<UWidgetComponent> MarkerWidget;

	bool bIsDodging = false;
	bool bInvulnerable = false;
	float LastDodgeTime = -100.f;
};
