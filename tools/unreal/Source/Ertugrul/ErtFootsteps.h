// Qadam changi (protsedural shar, shaffof material) va protsedural qadam tovushi (shovqin + konvert, tasodifiy balandlik).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ErtFootsteps.generated.h"

class UProceduralMeshComponent;
class UAudioComponent;
class USoundWaveProcedural;
class UMaterialInterface;

UCLASS(ClassGroup = (Ertugrul), meta = (BlueprintSpawnableComponent))
class ERTUGRUL_API UErtFootsteps : public UActorComponent
{
	GENERATED_BODY()
public:
	UErtFootsteps();
	/** Qadam: Pos - oyoq ostidagi nuqta, bSand - qum (yumshoqroq ovoz, kattaroq chang) */
	void Step(const FVector& Pos, bool bSand, float Strength);
	/** Suvga sho'ng'ish / suvda qadam */
	void Splash(const FVector& Pos);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FPuff { float T = 1.f; FVector Pos; float Size = 1.f; FLinearColor Col; };
	TArray<FPuff> Puffs;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Meshes;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> DustMat;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Audio;
	UPROPERTY(Transient) TObjectPtr<USoundWaveProcedural> Wave;
	void PlayNoise(float Seconds, float Pitch, float Softness, float Volume);
};
