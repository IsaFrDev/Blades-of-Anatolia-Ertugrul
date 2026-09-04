// O'lja to'rvasi: dushman o'lganda tushadi, [E] bilan olinadi (oltin, o'qlar, dori, go'sht).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtLoot.generated.h"

class UProceduralMeshComponent;
class AErtCharacter;

UCLASS()
class ERTUGRUL_API AErtLoot : public AActor
{
	GENERATED_BODY()
public:
	AErtLoot();
	int32 Gold = 0, Arrows = 0, Potions = 0, Meat = 0;
	bool bBoss = false;
	FString Describe() const;
	void GiveTo(AErtCharacter* H);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Mesh;
	float T = 0.f;
};
