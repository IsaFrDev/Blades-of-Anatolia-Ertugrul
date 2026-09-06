// Ertug'rul: uchinchi shaxs harakat mexanikasi (yurish/yugurish/chopish, sakrash, cho'kish,
// qiyalikda sirpanish, to'siqqa chiqish (mantle), orbital kamera). Barcha kirish (Enhanced Input) kodda yaratiladi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ErtCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UErtHeroBody;
class AErtHorse;
class AErtEnemy;
class AErtNpc;
class AErtBoat;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EErtGait : uint8 { Walk, Jog, Sprint };

UCLASS()
class ERTUGRUL_API AErtCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AErtCharacter();

	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float WalkSpeed = 135.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float JogSpeed = 330.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float SprintSpeed = 620.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float CrouchSpeed = 120.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float StaminaMax = 100.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float StaminaDrain = 14.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float StaminaRegen = 18.f;
	/** Shu burchakdan boshlab (gradus) qiyalikda pastga sirpanish kuchi qo'shiladi. */
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Harakat") float SlideAngle = 36.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Mantle") float MantleMinHeight = 45.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Mantle") float MantleMaxHeight = 185.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Mantle") float MantleDuration = 0.55f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kamera") float CamMin = 180.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kamera") float CamMax = 700.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kamera") bool bShowDebug = false;

	UPROPERTY(EditAnywhere, Category = "Ertugrul|Jang") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Jang") float AttackDamage = 30.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Jang") float ArrowDamage = 45.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Jang") int32 MaxArrows = 16;

	UFUNCTION(BlueprintPure, Category = "Ertugrul") EErtGait GetGait() const { return Gait; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetStamina() const { return Stamina; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetMaxHealth() const { return MaxHealth; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") int32 GetArrows() const { return Arrows; }
	// Inventar va daraja
	int32 Meat = 0;
	bool bPeltArmor = false;
	int32 Gold = 20, Potions = 1, Level = 1, XP = 0, SwordTier = 1, BowTier = 1; bool bShield = false;
	int32 XPToNext() const { return 80 + (Level - 1) * 60; }
	void AddXP(int32 N);
	void AddGold(int32 N) { Gold = FMath::Max(0, Gold + N); }
	bool UsePotion();
	void ApplyEquipment();
	float LevelFlash = 0.f;
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsDead() const { return bDead; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsBlocking() const { return bBlocking; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsSwimming() const { return bSwimming; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsRiding() const { return Horse != nullptr; }
	bool IsInBoat() const { return Boat != nullptr; }
	AErtBoat* NearestBoat(float MaxDist) const;
	UPROPERTY(Transient) TObjectPtr<AErtBoat> Boat;
	AErtHorse* GetHorse() const { return Horse; }
	AErtHorse* NearestHorse(float MaxDist) const;
	AErtNpc* NearestNpc(float MaxDist) const;
	float GetParryFlash() const { return ParryFlash; }
	float GetExecuteFlash() const { return ExecuteFlash; }
	float GetRiposteT() const { return RiposteT; }
	void MountHorse(AErtHorse* H);
	void DismountHorse();
	float GetHurtFlash() const { return HurtFlash; }
	/** Dushman zarbasi (blok bo'lsa kamayadi) */
	void ReceiveHit(float Damage, const FVector& From, AErtEnemy* Attacker = nullptr, bool bUnblockable = false);
	void AddArrows(int32 N) { Arrows = FMath::Clamp(Arrows + N, 0, MaxArrows); }
	void Heal(float V) { Health = FMath::Min(MaxHealth, Health + V); }
	/** Nazorat nuqtasi / epizod boshi: joyga qo'yish, sog'liqni tiklash */
	void ResetAt(const FVector& Pos, float Yaw);
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsMantling() const { return bMantling; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetGroundSlopeDeg() const { return SlopeDeg; }

	// Tugmalarni sozlash: harakat nomi -> klaviatura/sichqoncha tugmasi (gamepad o'zgarmaydi)
	static const TArray<FString>& BindableActions();
	FString GetBindingName(const FString& Action) const;   // hozirgi tugma (ko'rsatish uchun)
	void SetBinding(const FString& Action, const FKey& Key);
	void ApplyBindings();
	TMap<FString, FKey> Bindings;
	/** Kirish yoqilgan/o'chirilgan (kat-sahna, dialog). */
	UPROPERTY(BlueprintReadWrite, Category = "Ertugrul") bool bInputEnabled = true;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PIC) override;
	virtual void Landed(const FHitResult& Hit) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Cam;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UErtHeroBody> Body;

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> IMC;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Crouch;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Walk;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Zoom;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Attack;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Block;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Shoot;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Menu;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuUp;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuDown;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Confirm;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Interact;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Choice1;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Choice2;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Choice3;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Choice4;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuLeft;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_MenuRight;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Settings;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Map;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Lock;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Dodge;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Inventory;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Potion;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Kick;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Skill;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Whistle;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Warrior1;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Warrior2;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Warrior3;

private:
	void OnMenu();
	void OnMenuUp();
	void OnMenuDown();
	void OnConfirm();
	void OnInteract();
	void OnChoice1(); void OnChoice2(); void OnChoice3(); void OnChoice4();
	void OnMenuLeft(); void OnMenuRight(); void OnSettings(); void OnMap();
public:
	AErtEnemy* NearestCarcass(float MaxDist) const;
	class AErtLoot* NearestLoot(float MaxDist) const;
private:
	void OnLock(); void OnDodge(); void UpdateLock(float Dt); void OnInventory(); void OnPotion();
	void OnAttackPressed(); void OnAttackReleased(); void OnKick();
public:
	// Jangchilar (raqobatchi tahlili): 0 Ertug'rul (qilich, muvozanat), 1 Turg'ut (bolta: sekin, kuchli, gangitadi), 2 Meryem (kamon, yashirinish, tez)
	int32 Warrior = 0;
	void SetWarrior(int32 W);
	const TCHAR* WarriorName() const { return Warrior == 1 ? TEXT("Turg'ut Alp") : (Warrior == 2 ? TEXT("Meryem") : TEXT("Ertug'rul Bey")); }
	float WarriorMelee = 1.f, WarriorArrow = 1.f, WarriorSpeed = 1.f, WarriorStagger = 0.f, WarriorKnock = 1.f;
	/** Alp mahorat shkalasi (0..100): zarba +7, parry +20; F - maxsus zarba */
	float AlpBar = 0.f;
	void AddAlp(float V) { AlpBar = FMath::Clamp(AlpBar + V, 0.f, 100.f); }
	float GetAlpBar() const { return AlpBar; }
	const TCHAR* SkillName() const { return Warrior == 1 ? TEXT("Yer zarbasi") : (Warrior == 2 ? TEXT("Uch o'q") : TEXT("Bo'ron qilichi")); }
	void OnSkill(); void OnWhistle(); void OnWarrior1() { SetWarrior(0); } void OnWarrior2() { SetWarrior(1); } void OnWarrior3() { SetWarrior(2); }
	float SkillFlash = 0.f;
private: void DoAttack(int32 Kind, float DamageMul, bool bGuardBreak, float Stagger, float Knock);
	float AttackHoldT = -1.f, ComboWindowT = 0.f; int32 ComboStep = 0; bool bHeavyDone = false;
public:
	int32 GetComboStep() const { return ComboStep; }
	float GetComboWindow() const { return ComboWindowT; }
private:
	UPROPERTY(Transient) TObjectPtr<AErtEnemy> LockTarget;
	float DodgeT = 0.f;
public:
	AErtEnemy* GetLockTarget() const { return LockTarget; }
	bool IsDodging() const { return DodgeT > 0.f; }
private:
	float ShakeT = 0.f; FVector BoomBase = FVector(0, 45.f, 60.f);
	UPROPERTY(Transient) TObjectPtr<AErtHorse> Horse;
	float BlockT = 99.f, ParryFlash = 0.f, RiposteT = 0.f, ExecuteFlash = 0.f;
	void OnAttack();
	void OnBlockOn();
	void OnBlockOff();
	void OnShoot();
	void UpdateCombat(float Dt);

	// Suzish (daryo/ko'l/voha) va qadamlar
	void UpdateSwim(float Dt);
	void UpdateSteps(float Dt);
	UPROPERTY(Transient) TObjectPtr<class AErtWorldBuilder> WorldRef;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UErtFootsteps> Footsteps;
	bool bSwimming = false;
	float StepDist = 0.f;
	int32 StepFoot = 0;

	float Health = 100.f;
	int32 Arrows = 12;
	float AttackCD = 0.f, HurtFlash = 0.f, NoDamageT = 0.f, ShootCD = 0.f;
	bool bBlocking = false, bDead = false;

	void BuildInput();
	UInputAction* ActionByName(const FString& Name) const;
	void OnMove(const FInputActionValue& V);
	void OnLook(const FInputActionValue& V);
	void OnJumpPressed();
	void OnJumpReleased();
	void OnSprintOn();
	void OnSprintOff();
	void OnCrouchToggle();
	void OnWalkToggle();
	void OnZoom(const FInputActionValue& V);

	bool TryMantle();
	bool TryVault();
	bool bVaulting = false; FVector VaultMid;
	void UpdateMantle(float Dt);
	void UpdateGait(float Dt);
	void UpdateSlope(float Dt);
	void DrawDebug();

	EErtGait Gait = EErtGait::Jog;
	bool bWantSprint = false;
	bool bWalkToggle = false;
	float Stamina = 100.f;
	float SlopeDeg = 0.f;
	FVector FloorNormal = FVector::UpVector;
	FVector2D MoveInput = FVector2D::ZeroVector;
	// Avtomatik o'yinchi (-ErtAutoPlay): epizodni o'zi o'ynaydi (sinov)
	bool bAutoPlay = false;
	float AutoT = 0.f, AutoTotalT = 0.f, AutoStuckT = 0.f, AutoPhaseT = 0.f, AutoIdleT = 0.f, AutoPotT = 0.f;
	int32 AutoLastPhase = -1, AutoTeleports = 0, AutoPhaseTeleports = 0;
	float AutoBestDist = 1e9f, AutoNoProgT = 0.f;
	int32 AutoShotN = 0;
	// -ErtCam=E,N,Z,pitch,yaw : 6 s dan keyin shu nuqtaga teleport, skrinshot, chiqish (diagnostika)
	TArray<float> CamShot; float CamShotT = -1.f;
	FVector AutoLastPos = FVector::ZeroVector;
	FString AutoEpisode;
	void UpdateAutoPlay(float Dt);
	float TargetArm = 380.f;
	float LandSquash = 0.f;

	bool bMantling = false;
	float MantleT = 0.f;
	FVector MantleStart, MantleEnd;

	// Avtomatik sinov: -ErtShot=<papka> bilan ishga tushirilsa, harakat ssenariysi va skrinshotlar
	FString ShotDir;
	float ShotT = -1.f;
	int32 ShotIdx = 0;
	FVector2D DebugMove = FVector2D::ZeroVector;
	void UpdateShotScript(float Dt);
	void TakeShot(const TCHAR* Name);
	void Teleport(float E, float N, float Z, float Pitch, float Yaw);
};
