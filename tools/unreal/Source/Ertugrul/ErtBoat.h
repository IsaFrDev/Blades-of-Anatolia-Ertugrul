// Qayiq: protsedural korpus + eshkaklar, suv sathida suzadi, o'yinchi [E] bilan minadi; faqat suv hududida yuradi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtBoat.generated.h"

class UProceduralMeshComponent;
class AErtCharacter;
class AErtWorldBuilder;

UCLASS()
class ERTUGRUL_API AErtBoat : public AActor
{
	GENERATED_BODY()
public:
	AErtBoat();
	bool IsOccupied() const { return Rider != nullptr; }
	void Board(AErtCharacter* H);
	void Leave();
	void SetInput(const FVector2D& In) { Input = In; }
	float GetSpeed() const { return Speed; }
	USceneComponent* GetSeat() const { return Seat; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Hull;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> OarL;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> OarR;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Seat;
	UPROPERTY(Transient) TObjectPtr<AErtCharacter> Rider;
	UPROPERTY(Transient) TObjectPtr<AErtWorldBuilder> World;
	FVector2D Input = FVector2D::ZeroVector;
	float Speed = 0.f, Yaw = 0.f, T = 0.f, RowPhase = 0.f;
	bool WaterAt(const FVector& P, float& SurfZ) const;
};
