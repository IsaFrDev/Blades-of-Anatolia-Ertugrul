// Uchadigan o'q: protsedural mesh, gravitatsiya bilan parabola, o'yinchiga (yoki dushmanga) tegsa zarar, yerga sanchiladi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtArrow.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ERTUGRUL_API AErtArrow : public AActor
{
	GENERATED_BODY()
public:
	AErtArrow();
	/** bFromPlayer: o'yinchi otgan (dushmanga tegadi), aks holda dushman otgan (o'yinchiga tegadi) */
	void Launch(const FVector& Dir, float Speed, float InDamage, bool bFromPlayer, AActor* Shooter);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<AActor> Owner_;
	FVector Vel = FVector::ZeroVector;
	float Damage = 10.f, Life = 0.f;
	bool bPlayer = false, bStuck = false;
};
