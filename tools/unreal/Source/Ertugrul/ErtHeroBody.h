// Ertug'rul tanasi: tashqi resurssiz, 0 dan protsedural qurilgan bo'g'imli figura.
// Har bir bo'g'im - o'z pivotida joylashgan ProceduralMesh; yurish/yugurish/sakrash/cho'kish/zarba pozalari kodda.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ErtHeroBody.generated.h"

class UProceduralMeshComponent;
class USceneComponent;
class UMaterialInterface;

UCLASS(ClassGroup = (Ertugrul), meta = (BlueprintSpawnableComponent))
class ERTUGRUL_API UErtHeroBody : public UActorComponent
{
	GENERATED_BODY()
public:
	UErtHeroBody();

	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Skin = FLinearColor(0.78f, 0.58f, 0.44f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Kaftan = FLinearColor(0.35f, 0.08f, 0.07f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Trousers = FLinearColor(0.16f, 0.12f, 0.09f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Leather = FLinearColor(0.28f, 0.17f, 0.09f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Fur = FLinearColor(0.82f, 0.76f, 0.66f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Beard = FLinearColor(0.20f, 0.13f, 0.08f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Trim = FLinearColor(0.85f, 0.70f, 0.25f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") FLinearColor Steel = FLinearColor(0.75f, 0.77f, 0.80f);
	/** Bo'rk o'rniga dubulg'a (dushman askarlari) */
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") bool bHelmet = false;
	/** Ayol: soqolsiz, ro'mol va uzun ko'ylak */
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") bool bWoman = false;
	/** Qo'lida qilich (zarba pozasi uchun) */
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") bool bSwordInHand = false;
	// Kiyim detallari (dushman/NPC): salla, lamellar zirh, yelka himoyasi, plash, sadoq, orqadagi qalqon, etik, zanjir ko'ylak
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bTurban = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bLamellar = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bPauldrons = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bCloak = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bQuiver = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bBackShield = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bBoots = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bMail = false;
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") FLinearColor Cloak = FLinearColor(0.35f, 0.08f, 0.07f);
	UPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") FLinearColor Turban = FLinearColor(0.9f, 0.88f, 0.8f);

	/** Tanani quradi (bo'g'imlar + geometriya). Parent - kapsula yoki ildiz komponent. */
	void Build(USceneComponent* Parent, float CapsuleHalfHeight);

	/** Protsedural poza. Speed sm/s (gorizontal), Lean - yon egilish (-1..1). */
	void Animate(float Dt, float Speed, bool bInAir, bool bCrouched, float Lean, float SlopeDeg);

	/** Qilich zarbasi: 0 o'ngdan, 1 chapdan, 2 tepadan og'ir, 3 tepki */
	void TriggerAttack(int32 Kind = 0);
	/** Zarba yedi - qisqa titrash */
	void TriggerHurt(float SideSign = 0.f);
	/** Jarohat izi: ko'krak/orqa/yon yuzasida qora-qizil dog' (12 tagacha) */
	void AddWound(float SideSign, float Strength);   // SideSign: -1 chapdan, +1 o'ngdan, 0 old/orqa
	/** Suzish pozasi (gorizontal, qo'l-oyoq harakati) */
	void SetSwimming(bool bOn) { bSwim = bOn; }
	void SetRiding(bool bOn) { bRide = bOn; }
	void SetBlocking(bool bOn) { bBlock = bOn; }
	/** Chap bilakda qalqon */
	void SetShield(bool bOn);
	/** Damashq po'lati: tig' rangi */
	void SetSwordTier(int32 Tier);
	/** Parry: qisqa keskin harakat */
	void TriggerParry() { ParryT = 1.f; }
	/** Yiqilgan holat (o'lim) */
	void SetDead(float CapsuleHalfHeight, int32 Variant = -1);   // -1 tasodifiy: 0 orqaga yiqilish, 1 yuztuban, 2 tiz cho'kib yonboshga
	bool IsBuilt() const { return Pelvis != nullptr; }
	float AttackPhase() const { return AttackT; }

private:
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Pelvis;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Torso;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Head;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> UpperArmL;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> UpperArmR;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> LowerArmL;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> LowerArmR;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> ThighL;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> ThighR;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> ShinL;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> ShinR;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Shield;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Wounds;
	TArray<FVector4> WoundList;   // x,y,z markaz (Torso koordinatasi), w o'lcham
	int32 WoundCount = 0;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Mat;

	float Phase = 0.f;
	float IdleT = 0.f;
	float AttackT = 0.f;
	int32 AttackKind = 0;
	float HurtT = 0.f, HurtDir = 0.f;
	bool bDead = false;
	bool bSwim = false;
	bool bRide = false;
	bool bBlock = false;
	float ParryT = 0.f;
	float RideBob = 0.f;
	FVector PelvisBase = FVector::ZeroVector;

	struct FPose
	{
		float PelvisZ = 0.f, PelvisPitch = 0.f, TorsoPitch = 0.f, TorsoRoll = 0.f, TorsoYaw = 0.f, HeadPitch = 0.f;
		float ThighL = 0.f, ThighR = 0.f, KneeL = 0.f, KneeR = 0.f;
		float ArmL = 0.f, ArmR = 0.f, ElbowL = 0.f, ElbowR = 0.f, ArmSpread = 0.f, LegSpread = 0.f;
	} Cur;

	UProceduralMeshComponent* MakePart(const FName& Name, USceneComponent* Parent, const FVector& RelLoc);
	void Apply(const FPose& P);
};
