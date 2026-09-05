// Olov effekti (Niagara o'rnida protsedural): olov tili (billboard to'rtburchaklar), tutun, uchqunlar + miltillovchi nuqtaviy nur.
// Har gulxan/mash'ala uchun bitta aktor; faqat kameraga 150 m ichida jonlanadi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtFire.generated.h"

class UProceduralMeshComponent;
class UPointLightComponent;

UCLASS()
class ERTUGRUL_API AErtFireFx : public AActor
{
	GENERATED_BODY()
public:
	AErtFireFx();
	/** Scale: 1 gulxan, 0.4 mash'ala. bLight: miltillovchi nur */
	void Init(float InScale, bool bLight);
	static AErtFireFx* Spawn(UWorld* W, const FVector& Pos, float Scale, bool bLight);

protected:
	virtual void Tick(float Dt) override;

private:
	struct FP { FVector P, V; float Life, MaxLife, Size; int32 Kind; };   // Kind: 0 olov, 1 tutun, 2 uchqun
	TArray<FP> Ps;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> FlameMesh;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> SmokeMesh;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> Light;
	float Scale = 1.f, T = 0.f, Seed = 0.f, EmitAcc = 0.f, BaseIntensity = 900.f;
	void Rebuild(const FVector& CamPos);
};
