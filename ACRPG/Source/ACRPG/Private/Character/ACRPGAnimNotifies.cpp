#include "Character/ACRPGAnimNotifies.h"

#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGCombatComponent.h"
#include "Components/ACRPGFootstepComponent.h"
#include "Components/ACRPGTargetingComponent.h"

#include "Components/SkeletalMeshComponent.h"

/**
 * Barcha notify'lar uchun umumiy yordamchi: mesh'dan personajni oladi.
 * Notify Anim Preview oynasida ham chaqiriladi — u yerda o'yin dunyosi yo'q,
 * shuning uchun har doim nullptr tekshiruvi kerak.
 */
static AACRPGCharacterBase* GetOwnerCharacter(USkeletalMeshComponent* MeshComp)
{
	return MeshComp ? Cast<AACRPGCharacterBase>(MeshComp->GetOwner()) : nullptr;
}

// ---------------------------------------------------------------------------
// Ep.#55 — QADAM
// ---------------------------------------------------------------------------

void UACRPGAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AACRPGCharacterBase* Character = GetOwnerCharacter(MeshComp);
	if (!Character)
	{
		return;
	}

	if (UACRPGFootstepComponent* Footsteps =
		Character->FindComponentByClass<UACRPGFootstepComponent>())
	{
		Footsteps->OnFootstep(FootSocket);
	}
}

FString UACRPGAnimNotify_Footstep::GetNotifyName_Implementation() const
{
	// Montaj timeline'ida ko'rinadigan nom: "Qadam (foot_l)"
	return FString::Printf(TEXT("Qadam (%s)"), *FootSocket.ToString());
}

// ---------------------------------------------------------------------------
// Ep.#9 — KOMBO OYNASI
// ---------------------------------------------------------------------------

void UACRPGAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AACRPGCharacterBase* Character = GetOwnerCharacter(MeshComp))
	{
		if (UACRPGCombatComponent* Combat = Character->GetCombat())
		{
			Combat->OpenComboWindow();
		}
	}
}

void UACRPGAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AACRPGCharacterBase* Character = GetOwnerCharacter(MeshComp))
	{
		if (UACRPGCombatComponent* Combat = Character->GetCombat())
		{
			Combat->CloseComboWindow();
		}
	}
}

FString UACRPGAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Kombo oynasi");
}

// ---------------------------------------------------------------------------
// Ep.#10 — QILICH TRACE'I
// ---------------------------------------------------------------------------

void UACRPGAnimNotifyState_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AACRPGCharacterBase* Character = GetOwnerCharacter(MeshComp))
	{
		if (UACRPGCombatComponent* Combat = Character->GetCombat())
		{
			Combat->EnableWeaponTrace();
		}
	}
}

void UACRPGAnimNotifyState_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	// MUHIM: NotifyEnd montaj uzilib ketganda ham chaqiriladi.
	// Shuning uchun trace bu yerda albatta o'chadi — "abadiy urish" bugi bo'lmaydi.
	if (AACRPGCharacterBase* Character = GetOwnerCharacter(MeshComp))
	{
		if (UACRPGCombatComponent* Combat = Character->GetCombat())
		{
			Combat->DisableWeaponTrace();
		}
	}
}

FString UACRPGAnimNotifyState_WeaponTrace::GetNotifyName_Implementation() const
{
	return TEXT("Qilich trace");
}

// ---------------------------------------------------------------------------
// Ep.#12 — I-FRAMES
// ---------------------------------------------------------------------------

void UACRPGAnimNotifyState_Invulnerable::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AACRPGCharacterBase* Character = GetOwnerCharacter(MeshComp))
	{
		if (UACRPGTargetingComponent* Targeting =
			Character->FindComponentByClass<UACRPGTargetingComponent>())
		{
			Targeting->SetInvulnerable(true);
		}
	}
}

void UACRPGAnimNotifyState_Invulnerable::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AACRPGCharacterBase* Character = GetOwnerCharacter(MeshComp))
	{
		if (UACRPGTargetingComponent* Targeting =
			Character->FindComponentByClass<UACRPGTargetingComponent>())
		{
			Targeting->SetInvulnerable(false);
		}
	}
}

FString UACRPGAnimNotifyState_Invulnerable::GetNotifyName_Implementation() const
{
	return TEXT("Zarbaga chidamlilik");
}
