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
	/** Xarita markazi (dunyo XY, sm) - surish */
	void SetCenter(const FVector2D& WorldXY) { Center = WorldXY; UpdateCamera(); }
	const FVector2D& GetCenter() const { return Center; }
	/** Ekran piksellari bo'yicha surish (S - xarita kvadrati piksel o'lchami) */
	void PanPixels(float Dx, float Dy, float S);
	void Tilt(float DeltaPitch) { Pitch = FMath::Clamp(Pitch + DeltaPitch, -89.f, -30.f); UpdateCamera(); }
	float GetOrtho() const { return Ortho; }
	/** Xarita teksturasi [0..1] -> dunyo XY (sm), relyef sathi bilan */
	FVector Unproject(float U, float V) const;

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
	float Yaw = 35.f, Pitch = -62.f, Ortho = 4600.f;
	FVector2D Center = FVector2D::ZeroVector;
	bool bActive = false; float MarkerT = 0.f;
	FVector Mini(const FVector& WorldPos) const { return Origin + FVector(WorldPos.X * K, WorldPos.Y * K, WorldPos.Z * KZ); }
	void BuildTerrain();
	void UpdateCamera();
	void UpdateMarkers();
};
