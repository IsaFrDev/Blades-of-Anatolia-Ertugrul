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
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsDead() const { return bDead; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsBlocking() const { return bBlocking; }
	float GetHurtFlash() const { return HurtFlash; }
	/** Dushman zarbasi (blok bo'lsa kamayadi) */
	void ReceiveHit(float Damage, const FVector& From);
	void AddArrows(int32 N) { Arrows = FMath::Clamp(Arrows + N, 0, MaxArrows); }
	void Heal(float V) { Health = FMath::Min(MaxHealth, Health + V); }
	/** Nazorat nuqtasi / epizod boshi: joyga qo'yish, sog'liqni tiklash */
	void ResetAt(const FVector& Pos, float Yaw);
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsMantling() const { return bMantling; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetGroundSlopeDeg() const { return SlopeDeg; }

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

private:
	void OnAttack();
	void OnBlockOn();
	void OnBlockOff();
	void OnShoot();
	void UpdateCombat(float Dt);

	float Health = 100.f;
	int32 Arrows = 12;
	float AttackCD = 0.f, HurtFlash = 0.f, NoDamageT = 0.f, ShootCD = 0.f;
	bool bBlocking = false, bDead = false;

	void BuildInput();
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
