// Jang effektlari: protsedural zarralar (qon sachrashi, blok uchqunlari, chang) va qilich izi (yoy shaklidagi shaffof mesh).
// Bitta ProceduralMesh har kadr qayta quriladi (zarralar oz - 60 tagacha).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtFx.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ERTUGRUL_API AErtBurst : public AActor
{
	GENERATED_BODY()
public:
	AErtBurst();
	/** Zarralar: Pos dan Dir yo'nalishida sochiladi. Kind: 0 qon, 1 uchqun, 2 chang */
	void InitParticles(const FVector& Pos, const FVector& Dir, int32 Kind, int32 Count);
	/** Qilich izi: Center atrofida, Yaw yo'nalishida, StartAng..EndAng gradus yoy (gorizontal), Radius sm, Kind 2 og'ir zarba (kengroq) */
	void InitArc(const FVector& Center, float Yaw, float StartAng, float EndAng, float Radius, float Tilt, bool bHeavy);

	static void Blood(UWorld* W, const FVector& Pos, const FVector& Dir, float Strength = 1.f);
	static void Sparks(UWorld* W, const FVector& Pos, const FVector& Dir);
	static void Dust(UWorld* W, const FVector& Pos, float Strength = 1.f);
	static void SwordArc(UWorld* W, const FVector& Center, float Yaw, int32 AttackKind);

protected:
	virtual void Tick(float Dt) override;

private:
	struct FP { FVector P, V; float Life, Size; FLinearColor C; bool bStuck; };
	TArray<FP> Ps;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Mesh;
	float T = 0.f, Duration = 1.f;
	int32 Mode = 0;   // 0 zarralar, 1 yoy
	// Yoy
	FVector ArcCenter; float ArcYaw = 0.f, ArcA0 = 0.f, ArcA1 = 0.f, ArcR = 0.f, ArcTilt = 0.f; bool bArcHeavy = false;
	void Rebuild();
};
