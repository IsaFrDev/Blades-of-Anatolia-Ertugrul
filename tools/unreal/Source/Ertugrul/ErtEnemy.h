// Dushmanlar va ov hayvonlari: protsedural tana, oddiy AI (qorovul/patrul, sezish, ta'qib, zarba; kiyik qochadi).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ErtEnemy.generated.h"

class UErtHeroBody;
class UProceduralMeshComponent;

UENUM(BlueprintType)
enum class EErtEnemyKind : uint8 { Footman, Sergeant, Crossbow, Elite, Deer };

UCLASS()
class ERTUGRUL_API AErtEnemy : public ACharacter
{
	GENERATED_BODY()
public:
	AErtEnemy();

	void Init(EErtEnemyKind InKind, const FVector& Home, float PatrolRadius);
	void ApplyHit(float Damage, AActor* Source);

	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsDead() const { return bDead; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsAlerted() const { return bAlerted; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") EErtEnemyKind GetKind() const { return Kind; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetMaxHealth() const { return MaxHealth; }
	bool IsAnimal() const { return Kind == EErtEnemyKind::Deer; }
	/** Kim o'ldirdi (o'yinchi bo'lsa hisoblanadi) */
	AActor* Killer = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UErtHeroBody> Body;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> DeerParts; // 0 tana, 1-4 oyoq, 5 bosh
	EErtEnemyKind Kind = EErtEnemyKind::Footman;
	FVector HomePos = FVector::ZeroVector;
	float Patrol = 0.f;
	float Health = 60.f, MaxHealth = 60.f;
	float AttackRange = 200.f, AttackDamage = 10.f, AttackCooldown = 1.5f, MoveSpeed = 380.f;
	float AttackCD = 0.f, HitPending = -1.f;
	float WanderT = 0.f, FleeT = 0.f, DeerPhase = 0.f;
	FVector WanderTarget = FVector::ZeroVector;
	bool bAlerted = false, bDead = false, bInit = false;

	void BuildDeer();
	void TickDeer(float Dt, APawn* Player);
	void TickGuard(float Dt, APawn* Player);
	void MoveToward(const FVector& Target, float Speed);
	bool CanSee(const APawn* Player) const;
	void Die();
};
