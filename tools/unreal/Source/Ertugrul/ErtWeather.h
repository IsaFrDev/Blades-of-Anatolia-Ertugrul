// Ob-havo: kamera atrofida protsedural yomg'ir/qor/chang zarralari (bitta mesh, pastga siljib aylanadi), tuman zichligi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtWeather.generated.h"

class UProceduralMeshComponent;
class AExponentialHeightFog;

UCLASS()
class ERTUGRUL_API AErtWeather : public AActor
{
	GENERATED_BODY()
public:
	AErtWeather();
	/** clear / rain / storm / snow / dust / fog / wind */
	void SetWeather(const FString& Name);
	const FString& GetWeather() const { return Current; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<AExponentialHeightFog> Fog;
	FString Current = TEXT("clear");
	float FallSpeed = 0.f, Drift = 0.f, BoxH = 3000.f, Offset = 0.f;
	void Build(int32 Count, const FVector& Size, const FLinearColor& Col, float Streak);
};
