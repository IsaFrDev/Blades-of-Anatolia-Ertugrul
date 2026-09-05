// Dushmanlar va ov hayvonlari: protsedural tana, oddiy AI (qorovul/patrul, sezish, ta'qib, zarba; kiyik qochadi).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ErtEnemy.generated.h"

class UErtHeroBody;
class UProceduralMeshComponent;
class AErtHorse;

UENUM(BlueprintType)
enum class EErtEnemyKind : uint8 { Footman, Sergeant, Crossbow, Elite, Deer, Rider, Boss };

UCLASS()
class ERTUGRUL_API AErtEnemy : public ACharacter
{
	GENERATED_BODY()
public:
	AErtEnemy();

	void Init(EErtEnemyKind InKind, const FVector& Home, float PatrolRadius);
	/** Jamoa: 0 dushman, 1 ittifoqchi (Qayi alplari) - Init dan oldin qo'yiladi */
	int32 Team = 0;
	bool IsAlly() const { return Team == 1; }
	/** Hozirgi raqib (o'yinchi yoki boshqa jamoa) */
	AActor* CurrentTarget() const { return TargetActor.Get(); }
	/** bGuardBreak - og'ir zarba/tepki: qalqonni chetlab o'tadi */
	void ApplyHit(float Damage, AActor* Source, bool bGuardBreak = false);
	bool IsGuarding() const { return GuardT > 0.f; }
	/** Parry natijasi: gangib qoladi (harakat/zarba yo'q) */
	void Stagger(float Seconds);
	bool IsStaggered() const { return StaggerT > 0.f; }
	/** Zarba oldidan (ogohlantirish belgisi uchun) */
	bool IsWindingUp() const { return HitPending > 0.f; }
	/** Otliq dushman: otga o'tqazish */
	void MountHorse(AErtHorse* H);
	AErtHorse* GetMount() const { return Mount; }

	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsDead() const { return bDead; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsAlerted() const { return bAlerted; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") EErtEnemyKind GetKind() const { return Kind; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float GetMaxHealth() const { return MaxHealth; }
	bool IsAnimal() const { return Kind == EErtEnemyKind::Deer; }
	bool IsBoss() const { return Kind == EErtEnemyKind::Boss; }
	float ExecuteThreshold() const { return IsBoss() ? 0.12f : 0.25f; }
	bool bLooted = false;   // kiyik go'shti olindi
	int32 XPValue() const { if (Kind == EErtEnemyKind::Boss) return 400; switch (Kind) { case EErtEnemyKind::Footman: return 20; case EErtEnemyKind::Sergeant: return 35; case EErtEnemyKind::Crossbow: return 25; case EErtEnemyKind::Elite: return 80; case EErtEnemyKind::Rider: return 50; default: return 10; } }
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
	float AttackCD = 0.f, HitPending = -1.f, StaggerT = 0.f, GuardT = 0.f, HeavyCD = 6.f;
	bool bHeavyPending = false;
	UPROPERTY(Transient) TObjectPtr<AErtHorse> Mount;
	void TickRider(float Dt, class AErtCharacter* Hero, float DP);
	float WanderT = 0.f, FleeT = 0.f, DeerPhase = 0.f;
	FVector WanderTarget = FVector::ZeroVector;
	bool bAlerted = false, bDead = false, bInit = false;

	void BuildDeer();
	void TickDeer(float Dt, APawn* Player);
	void TickGuard(float Dt, APawn* Player);
	void MoveToward(const FVector& Target, float Speed);
	bool CanSee(const AActor* Target) const;
	TWeakObjectPtr<AActor> TargetActor;
	float RetargetT = 0.f;
	AActor* PickTarget(APawn* Player);
	void Die();
};
