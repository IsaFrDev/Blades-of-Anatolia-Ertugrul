#include "Items/ACRPGItemBase.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGInventoryComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AACRPGItemBase::AACRPGItemBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	// Kiyilganda mesh personajga ulanadi, shuning uchun fizika kerak emas.
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(ItemMesh);
	PickupSphere->SetSphereRadius(120.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AACRPGItemBase::OnEquipped(AACRPGCharacterBase* NewOwner)
{
	bIsEquipped = true;
	SetOwner(NewOwner);

	// Kiyilgan buyum hech kim bilan to'qnashmasligi kerak —
	// aks holda personaj o'z qilichiga urilib turadi.
	if (PickupSphere)
	{
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (ItemMesh)
	{
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AACRPGItemBase::OnUnequipped()
{
	bIsEquipped = false;
	SetOwner(nullptr);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void AACRPGItemBase::Interact(AACRPGCharacterBase* Interactor)
{
	if (bIsEquipped || !Interactor || ItemID.IsNone())
	{
		return;
	}

	UACRPGInventoryComponent* Inv = Interactor->FindComponentByClass<UACRPGInventoryComponent>();
	if (!Inv)
	{
		return;
	}

	const int32 Added = Inv->AddItem(ItemID, PickupQuantity);
	if (Added <= 0)
	{
		UE_LOG(LogACRPG, Log, TEXT("Inventar to'la — '%s' olinmadi."), *ItemID.ToString());
		return;
	}

	UE_LOG(LogACRPG, Log, TEXT("Olindi: %s x%d"), *ItemID.ToString(), Added);

	if (bDestroyOnPickup)
	{
		Destroy();
	}
}
