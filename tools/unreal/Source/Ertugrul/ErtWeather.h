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
	/** Osmon: kun ulushi (0 tun .. 1 kun), quyosh rangi, quyosh balandligi (gradus) - bulut tusi, yulduzlar, tuman rangi */
	void UpdateSky(float Day, const FLinearColor& SunColor, float ElevDeg);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<AExponentialHeightFog> Fog;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> CloudMesh;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> StarMesh;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> CloudMID;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> StarMID;
	float Coverage = 0.55f, CloudDark = 0.f;
	FString Current = TEXT("clear");
	float FallSpeed = 0.f, Drift = 0.f, BoxH = 3000.f, Offset = 0.f;
	void Build(int32 Count, const FVector& Size, const FLinearColor& Col, float Streak);
};
