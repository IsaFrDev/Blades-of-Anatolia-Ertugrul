// GPS navigatsiya: relyef ustida qo'pol to'r (10 m) bo'yicha A* yo'l, yerda yorug' lenta (ribbon) va xaritada chiziq.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtNav.generated.h"

class UProceduralMeshComponent;
class AErtWorldBuilder;

UCLASS()
class ERTUGRUL_API AErtGps : public AActor
{
	GENERATED_BODY()
public:
	AErtGps();
	static AErtGps* Get(UWorld* W);
	/** Maqsad (dunyo, sm); ZeroVector - o'chirish */
	void SetTarget(const FVector& T);
	const TArray<FVector>& GetPath() const { return Path; }
	bool HasPath() const { return Path.Num() > 1; }
	float GetPathLengthM() const { return PathLenM; }
	bool bEnabled = true;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Ribbon;
	UPROPERTY(Transient) TObjectPtr<AErtWorldBuilder> World;
	FVector Target = FVector::ZeroVector, LastFrom = FVector::ZeroVector, LastTarget = FVector::ZeroVector;
	TArray<FVector> Path;   // dunyo nuqtalari (sm), yer sathi
	float PathLenM = 0.f, RecalcT = 0.f, Anim = 0.f;
	static constexpr float CellM = 10.f;
	int32 Cells = 200;
	TArray<float> CostGrid;   // hujayra narxi (1 = tekis, katta = qiya/suv)
	void BuildCostGrid();
	bool FindPath(const FVector& From, const FVector& To);
	void RebuildRibbon();
};
