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

	void Init(const FLinearColor& InCoat);
	bool IsMounted() const { return Rider != nullptr; }
	AActor* GetRider() const { return Rider; }
	void Mount(AActor* InRider);
	void Dismount();
	/** Chavandoz kirishi: X - burilish (-1..1), Y - oldinga/orqaga, bGallop - chopish */
	void SetRiderInput(const FVector2D& In, bool bGallop);
	void RiderJump();
	USceneComponent* GetSaddle() const { return Saddle; }
	float GetSpeed() const { return CurSpeed; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<AActor> Rider;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Saddle;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> BodyMesh;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> HeadMesh;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Legs;
	FVector2D Input = FVector2D::ZeroVector;
	bool bGallopIn = false, bBuilt = false;
	float CurSpeed = 0.f, Phase = 0.f, WanderT = 0.f, HeadBob = 0.f;
	FVector HomePos = FVector::ZeroVector, WanderTarget = FVector::ZeroVector;
	void Build();
	void Animate(float Dt);
};
