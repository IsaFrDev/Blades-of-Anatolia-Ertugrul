// ACRPGAnimNotifies.h — animatsiya bilan kod orasidagi KO'PRIK
//
// Ep.#9 (kombo oynasi), #10 (qilich trace'i), #12 (i-frames), #55 (qadam tovushi)
//
// NEGA BU MUHIM?
//
// "Qilich qachon urishi kerak?" degan savolga dasturchi javob bera olmaydi —
// bu animatsiyaga bog'liq. Pastdan yuqoriga zarbada pichoq 0.4 soniyada,
// yondan zarbada 0.25 soniyada nishonga yetadi.
//
// Anim Notify shu muammoni yechadi: animator montaj timeline'ida
// "mana shu yerdan mana shu yergacha qilich urishi mumkin" deb belgilaydi,
// kod esa faqat "boshlandi / tugadi" xabarini oladi.
//
// EDITOR'DA SOZLASH:
//   1) Montajni oching
//   2) Notifies qatoriga o'ng tugma -> Add Notify State -> Weapon Trace
//   3) Boshlanish/tugash vaqtlarini pichoq harakatiga moslang

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ACRPGAnimNotifies.generated.h"

// ===========================================================================
// Ep.#55 — QADAM TOVUSHI
// Montajda oyoq yerga tekkan kadrga qo'yiladi.
// ===========================================================================

UCLASS(meta = (DisplayName = "ACRPG Qadam tovushi"))
class ACRPG_API UACRPGAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** Qaysi oyoq — trace shu socketdan tashlanadi. */
	UPROPERTY(EditAnywhere, Category = "Qadam")
	FName FootSocket = TEXT("foot_l");
};

// ===========================================================================
// Ep.#9 — KOMBO OYNASI
// Zarbaning oxiriga yaqin qo'yiladi: shu paytdan keyingi zarbani boshlash mumkin.
// ===========================================================================

UCLASS(meta = (DisplayName = "ACRPG Kombo oynasi"))
class ACRPG_API UACRPGAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};

// ===========================================================================
// Ep.#10 — QILICH TRACE'I
// Pichoq haqiqatan ham havoni kesayotgan oraliqqa qo'yiladi.
// ===========================================================================

UCLASS(meta = (DisplayName = "ACRPG Qilich trace"))
class ACRPG_API UACRPGAnimNotifyState_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};

// ===========================================================================
// Ep.#12 — DODGE I-FRAMES
// Dumalash animatsiyasining o'rtasiga qo'yiladi: shu oraliqda urish o'tmaydi.
// ===========================================================================

UCLASS(meta = (DisplayName = "ACRPG Zarbaga chidamlilik (i-frames)"))
class ACRPG_API UACRPGAnimNotifyState_Invulnerable : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
