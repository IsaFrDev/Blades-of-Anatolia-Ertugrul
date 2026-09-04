// Dunyo quruvchi: 2000x2000 m xarita 0 dan, resurssiz.
// Shimoli-g'arb: o'rmon + Qayi obasi (250x250 m reja), shimoli-sharq: qorli tog'lar + tosh qal'a,
// janubi-g'arb: devorli shahar (oltin gumbaz), janubi-sharq: mo'g'ul lageri, g'arbda daryo, yo'llar.
// Koordinatalar: reja (E=sharq, N=shimol, metr) -> UE (X=N*100, Y=E*100, Z*100).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtWorldBuilder.generated.h"

class UProceduralMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
struct FErtMeshData;

// Reja markazlari (metr)
namespace ErtMap
{
	constexpr float ObaE = -560.f, ObaN = 560.f, ObaZ = 20.f, ObaHalf = 125.f;
	constexpr float FortE = 620.f, FortN = 660.f, FortZ = 232.f, FortHalf = 34.f;
	constexpr float CityE = -470.f, CityN = -470.f, CityZ = 12.f, CityR = 190.f;
	constexpr float CampE = 520.f, CampN = -460.f, CampZ = 10.f, CampR = 150.f;
	constexpr float CrossE = 10.f, CrossN = 40.f, CrossZ = 9.f;
	constexpr float WaterZ = 3.5f;
	constexpr float LakeE = -700.f, LakeN = 700.f, LakeR = 40.f, LakeZ = 2.5f;
	constexpr float OasisE = 150.f, OasisN = -880.f, OasisR = 55.f, OasisZ = 6.f;
	constexpr float CaravanE = 300.f, CaravanN = -900.f;
	constexpr float DesertN = -700.f;   // shundan janubda cho'l
	constexpr float DamE = 720.f, DamN = -850.f, DamHalfE = 140.f, DamHalfN = 110.f, DamZ = 11.f;   // Damashq
	constexpr float HalabE = 800.f, HalabN = 120.f, HalabR = 130.f, HalabZ = 14.f, HalabMoundR = 42.f, HalabMoundH = 24.f;   // Halab
	constexpr float KonE = 120.f, KonN = 480.f, KonR = 150.f, KonZ = 12.f, KonHillR = 55.f, KonHillH = 9.f;   // Konya
	constexpr float KayE = 450.f, KayN = 100.f, KayR = 125.f, KayZ = 13.f, ErcE = 470.f, ErcN = -150.f, ErcR = 130.f, ErcH = 120.f;   // Qayseri va Erciyes
	constexpr float SivE = -180.f, SivN = 400.f, SivR = 120.f, SivZ = 13.f, SivHillR = 40.f, SivHillH = 8.f;   // Sivas (qal'a tepaligi shimolda)
}

UCLASS()
class ERTUGRUL_API AErtWorldBuilder : public AActor
{
	GENERATED_BODY()
public:
	AErtWorldBuilder();

	UPROPERTY(EditAnywhere, Category = "Ertugrul|Dunyo") int32 Seed = 7;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Dunyo") float WorldSizeM = 2000.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Dunyo") float ChunkM = 200.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Dunyo") float CellM = 10.f;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Dunyo") bool bBuildForest = true;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Dunyo") bool bBuildSettlements = true;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Dunyo") int32 TreeCount = 1600;


	/** Reja koordinatasidan relyef balandligi (metr). */
	UFUNCTION(BlueprintPure, Category = "Ertugrul") float HeightAt(float E, float N) const;
	/** Reja -> UE dunyo nuqtasi (sm). */
	UFUNCTION(BlueprintPure, Category = "Ertugrul") static FVector PlanToWorld(float E, float N, float Z) { return FVector(N * 100.f, E * 100.f, Z * 100.f); }
	UFUNCTION(CallInEditor, Category = "Ertugrul") void Rebuild();
	/** Reja nuqtasi suv ustidami (daryo/ko'l/voha); SurfZ - suv sathi (metr) */
	UFUNCTION(BlueprintPure, Category = "Ertugrul") bool IsWater(float E, float N, float& SurfZ) const;
	UFUNCTION(BlueprintPure, Category = "Ertugrul") static bool IsDesert(float E, float N) { return N < -700.f + 60.f; }

protected:
	virtual void OnConstruction(const FTransform& T) override;
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Parts;
	UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> Lights;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Mat;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> WaterMat;
	bool bBuilt = false;

	UProceduralMeshComponent* NewPart(const FString& Name, bool bCollision, UMaterialInterface* M = nullptr);
	void Build();
	void Clear();
	void BuildTerrain();
	void BuildWater();
	void BuildOba();
	void BuildFortress();
	void BuildCity();
	void BuildCamp();
	void BuildDesert();
	void BuildDamascus();
	void BuildHalab();
	void BuildKonya();
	void BuildKayseri();
	void BuildSivas();
	void AddPalm(FErtMeshData& M, float E, float N, float Z, float H, int32 S);
	void BuildForest();
	void BuildRocks();

public:
	float RiverE(float N) const;
private:
	float RoadDist(float E, float N, float* OutWidth = nullptr) const;
	FVector TerrainNormal(float E, float N) const;
	FLinearColor TerrainColor(float E, float N, float H, float Slope) const;
	bool IsBuildable(float E, float N) const; // daraxt/tosh uchun bo'sh joy

	// Element yordamchilari (reja koordinatalarida)
	void AddYurt(FErtMeshData& M, float E, float N, float Z, float R, float WallH, float RoofH, const FLinearColor& Wall, const FLinearColor& Roof, float DoorYaw, int32 S);
	void AddTree(FErtMeshData& M, float E, float N, float Z, float Scale, bool bPine, int32 S);
	void AddWatchTower(FErtMeshData& M, float E, float N, float Z, float H);
	void AddFire(FErtMeshData& M, float E, float N, float Z, bool bLight);
	void AddHorse(FErtMeshData& M, float E, float N, float Z, float Yaw, const FLinearColor& C);
	void AddBanner(FErtMeshData& M, float E, float N, float Z, float H, const FLinearColor& Flag, bool bTugh);
	void AddFenceRect(FErtMeshData& M, float E, float N, float Z, float HalfU, float HalfV, float Gap);
	void AddHouse(FErtMeshData& M, float E, float N, float Z, float HU, float HV, float H, float Yaw, const FLinearColor& C, int32 S);
};
