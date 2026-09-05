// 3D xarita: dunyoning 1/50 miniatyurasi (relyef + suv + shahar belgilari) osmonda uzoqda quriladi,
// ortografik SceneCapture bilan render-teksturaga olinadi; HUD uni ko'rsatadi, belgilarni Project() bilan ustiga chizadi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtMap3D.generated.h"

class UProceduralMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class AErtWorldBuilder;

UCLASS()
class ERTUGRUL_API AErtMap3D : public AActor
{
	GENERATED_BODY()
public:
	AErtMap3D();
	static AErtMap3D* Get(UWorld* W);
	void SetActive(bool bOn);
	UTextureRenderTarget2D* GetTexture() const { return RT; }
	/** Dunyo nuqtasi -> xarita teksturasidagi [0..1] koordinata */
	bool Project(const FVector& WorldPos, float& U, float& V) const;
	void Rotate(float DeltaYaw) { Yaw += DeltaYaw; UpdateCamera(); }
	void Zoom(float Factor);
	float GetYaw() const { return Yaw; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Terrain;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Markers;
	UPROPERTY(Transient) TObjectPtr<USceneCaptureComponent2D> Capture;
	UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> RT;
	UPROPERTY(Transient) TObjectPtr<AErtWorldBuilder> World;
	FVector Origin = FVector(-380000.f, -380000.f, 260000.f);   // miniatyura markazi (dunyodan uzoqda, 2.6 km balandda)
	float K = 0.02f, KZ = 0.05f;   // gorizontal 1/50, balandlik 2.5x bo'rttirilgan (H m * 5 sm)
	float Yaw = 35.f, Pitch = -52.f, Ortho = 4600.f;
	bool bActive = false; float MarkerT = 0.f;
	FVector Mini(const FVector& WorldPos) const { return Origin + FVector(WorldPos.X * K, WorldPos.Y * K, WorldPos.Z * KZ); }
	void BuildTerrain();
	void UpdateCamera();
	void UpdateMarkers();
};
