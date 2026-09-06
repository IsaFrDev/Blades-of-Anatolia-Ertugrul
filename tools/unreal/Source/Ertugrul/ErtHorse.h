// Ot: protsedural tana (tana, bo'yin, bosh, yol, dum, 4 oyoq, egar), minish/tushish,
// yurish 250 / yo'rtish 520 / chopish 950 sm/s, burilish tezlikka bog'liq, sakrash. Minilmaganda o'tlaydi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ErtHorse.generated.h"

class AErtCharacter;
class UProceduralMeshComponent;

UCLASS()
class ERTUGRUL_API AErtHorse : public ACharacter
{
	GENERATED_BODY()
public:
	AErtHorse();

	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ot") FLinearColor Coat = FLinearColor(0.36f, 0.22f, 0.11f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ot") float WalkSpeed = 250.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ot") float TrotSpeed = 520.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ot") float GallopSpeed = 950.f;

	void Init(const FLinearColor& InCoat, bool bInCamel = false);
	bool bCamel = false;
	bool IsCamel() const { return bCamel; }
	bool IsMounted() const { return Rider != nullptr; }
	AActor* GetRider() const { return Rider; }
	void Mount(AActor* InRider);
	void Dismount();
	/** Chavandoz kirishi: X - burilish (-1..1), Y - oldinga/orqaga, bGallop - chopish */
	void SetRiderInput(const FVector2D& In, bool bGallop);
	void RiderJump();
	USceneComponent* GetSaddle() const { return Saddle; }
	float GetSpeed() const { return CurSpeed; }
	float Health = 200.f, MaxHealth = 200.f;
	bool bDead = false;
	void ApplyDamage(float D);
	/** Hushtak: ot o'yinchi tomon yo'rtib keladi (25 s) */
	void Summon(const FVector& To) { SummonTo = To; SummonT = 25.f; }
	// Parvarish: boqish (go'sht/olma) va tarash - Care 0..1, tezlik +12% * Care, sog'liq tiklanishi tezroq, chaqirilganda tezroq keladi
	float Care = 0.f, CareFxT = 0.f;
	FString HorseName;   // nom (Bo'ra, Tulpor...) - HUD da
	void Feed() { Health = MaxHealth; Care = FMath::Min(1.f, Care + 0.35f); CareFxT = 2.5f; }
	void Groom() { Care = FMath::Min(1.f, Care + 0.5f); CareFxT = 2.5f; }
	bool bSaddled = false;   // egar (o'yinchi hunarmandchiligi)
	float CareSpeed() const { return 1.f + 0.12f * Care + (bSaddled ? 0.08f : 0.f); }
	float GetHealth() const { return Health; } float GetMaxHealth() const { return MaxHealth; }
	bool IsSummoned() const { return SummonT > 0.f; }
	bool IsDead() const { return bDead; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<AActor> Rider;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Saddle;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> BodyMesh;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> HeadMesh;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Legs;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> LowerLegs;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> TailMesh;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> NeckMesh;
	// Skeletli rejim (character.json: "horse" / "camel"): SingleNode, tezlikka qarab idle/walk/trot/gallop/jump
	UPROPERTY(Transient) TObjectPtr<class USkeletalMeshComponent> Skel;
	TMap<FString, TArray<TObjectPtr<class UAnimSequence>>> SkelAnims;
	UPROPERTY(Transient) TArray<TObjectPtr<class UAnimSequence>> SkelAnimRefs;
	UPROPERTY(Transient) TObjectPtr<class USkeletalMesh> SkelMeshRef;
	UPROPERTY(Transient) TObjectPtr<class UAnimSequence> CurAnim;
	float WalkRef = 250.f, TrotRef = 520.f, GallopRef = 950.f;
	bool TryBuildSkeletal();
	void SkelPlay(const FString& Key, float Rate, const TCHAR* Fallback = nullptr);
	FVector2D Input = FVector2D::ZeroVector;
	bool bGallopIn = false, bBuilt = false;
	float CurSpeed = 0.f, Phase = 0.f, WanderT = 0.f, HeadBob = 0.f;
	FVector HomePos = FVector::ZeroVector, WanderTarget = FVector::ZeroVector, SummonTo = FVector::ZeroVector;
	float SummonT = 0.f;
	void Build();
	void Animate(float Dt);
};
