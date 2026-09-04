// ACRPGPlayerCharacter.h — o'yinchi personaji.
//
// Bu klass "dirijyor": u hech qanday og'ir mantiqni o'zida saqlamaydi, faqat
// kiritishni (input) tegishli komponentga uzatadi. Blueprint seriyasida BP_Player
// ~3000 tugunga o'sib ketgan edi (#33 va #82 epizodlar aynan shu chalkashlikni
// tozalashga bag'ishlangan). Komponentlarga bo'lish shu muammoni yechadi.
//
// Qamrab olingan epizodlar: #2 (harakat, cho'kkalash), #3 (vault), #4 (assassination),
// #9 (hujum), #12 (lock-on, dodge), #30-32 (tirmashish), #48 (kamon), #56 (suzish),
// #63 (minish), #69 (blok), #78 (geympad).

#pragma once

#include "CoreMinimal.h"
#include "Character/ACRPGCharacterBase.h"
#include "ACRPGPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

class UACRPGVaultingComponent;
class UACRPGAssassinationComponent;
class UACRPGClimbingComponent;
class UACRPGTargetingComponent;
class UACRPGEquipmentComponent;
class UACRPGInventoryComponent;
class UACRPGQuestComponent;
class UACRPGSwimmingComponent;
class UACRPGRidingComponent;

UCLASS()
class ACRPG_API AACRPGPlayerCharacter : public AACRPGCharacterBase
{
	GENERATED_BODY()

public:
	AACRPGPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "ACRPG") UCameraComponent* GetCamera() const { return Camera; }
	UFUNCTION(BlueprintPure, Category = "ACRPG") USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintPure, Category = "ACRPG") UACRPGEquipmentComponent* GetEquipment() const { return EquipmentComponent; }
	UFUNCTION(BlueprintPure, Category = "ACRPG") UACRPGInventoryComponent* GetInventory() const { return InventoryComponent; }
	UFUNCTION(BlueprintPure, Category = "ACRPG") UACRPGQuestComponent* GetQuests() const { return QuestComponent; }
	UFUNCTION(BlueprintPure, Category = "ACRPG") UACRPGClimbingComponent* GetClimbing() const { return ClimbingComponent; }
	UFUNCTION(BlueprintPure, Category = "ACRPG") UACRPGTargetingComponent* GetTargeting() const { return TargetingComponent; }

	virtual bool CanMove() const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnCombatStateChanged(ECombatState OldState, ECombatState NewState) override;

	// -----------------------------------------------------------------------
	// KAMERA (Ep.#2)
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kamera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kamera")
	TObjectPtr<UCameraComponent> Camera;

	// -----------------------------------------------------------------------
	// KOMPONENTLAR — har bir epizod o'z komponentini qo'shadi
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGVaultingComponent> VaultingComponent;		// Ep.#3

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGAssassinationComponent> AssassinationComponent;	// Ep.#4

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGTargetingComponent> TargetingComponent;	// Ep.#12

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGInventoryComponent> InventoryComponent;	// Ep.#14

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGEquipmentComponent> EquipmentComponent;	// Ep.#14-19

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGClimbingComponent> ClimbingComponent;		// Ep.#30-32

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGQuestComponent> QuestComponent;			// Ep.#34-43

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGSwimmingComponent> SwimmingComponent;		// Ep.#56

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Komponent")
	TObjectPtr<UACRPGRidingComponent> RidingComponent;			// Ep.#63

	// -----------------------------------------------------------------------
	// INPUT ACTION'LAR (Ep.#2, #78)
	//
	// Har biri editor'da IA_Move, IA_Look... assetlariga tayinlanadi.
	// Bitta IA'ga ham klaviatura, ham geympad tugmasi bog'lanadi — shuning uchun
	// Ep.#78 dagi "geympad qo'llab-quvvatlash" alohida kod talab qilmaydi.
	// -----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Jump;		// Ep.#3 vault ham shu tugmada
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Sprint;		// Ep.#6
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Crouch;		// Ep.#2
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Attack;		// Ep.#9
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Block;		// Ep.#69
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Dodge;		// Ep.#12
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_LockOn;		// Ep.#12
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Interact;	// Ep.#38, #42
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Aim;		// Ep.#47-48 kamon
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_ToggleMenu;	// Ep.#15
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Pause;		// Ep.#64
	UPROPERTY(EditDefaultsOnly, Category = "Input") TObjectPtr<UInputAction> IA_Throw;		// Ep.#27

	// --- Input ishlovchilari ---
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpStarted();
	void Input_JumpCompleted();
	void Input_SprintStarted();
	void Input_SprintCompleted();
	void Input_CrouchToggle();
	void Input_Attack();
	void Input_BlockStarted();
	void Input_BlockCompleted();
	void Input_Dodge();
	void Input_LockOn();
	void Input_Interact();
	void Input_AimStarted();
	void Input_AimCompleted();
	void Input_ToggleMenu();
	void Input_Pause();
	void Input_Throw();

	// --- Sozlamalar ---

	/** Ep.#6 — yugurish tezligi (chidamlilik sarflaydi). */
	UPROPERTY(EditDefaultsOnly, Category = "Harakat", meta = (ClampMin = "100"))
	float SprintSpeed = 750.f;

	UPROPERTY(EditDefaultsOnly, Category = "Harakat", meta = (ClampMin = "100"))
	float WalkSpeed = 500.f;

	/** Ep.#47 — nishonga olayotganda sekinlashadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Harakat", meta = (ClampMin = "50"))
	float AimSpeed = 200.f;

	/** Ep.#12 — lock-on paytida kamera nishonga qaraydi. */
	UPROPERTY(EditDefaultsOnly, Category = "Kamera", meta = (ClampMin = "1"))
	float LockOnInterpSpeed = 8.f;

private:
	/** Ep.#12 — lock-on bo'lsa kamerani nishonga burish. */
	void UpdateLockOnCamera(float DeltaSeconds);

	/** Ep.#2 — kirish vektorini kamera yo'nalishiga moslash. */
	FVector GetMovementDirection(bool bForward) const;

	bool bWantsToSprint = false;
};
