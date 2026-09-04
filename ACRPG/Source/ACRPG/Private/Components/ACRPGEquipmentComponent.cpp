#include "Components/ACRPGEquipmentComponent.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGGameInstance.h"
#include "Character/ACRPGCharacterBase.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGInventoryComponent.h"
#include "Items/ACRPGItemBase.h"
#include "Items/ACRPGWeaponBase.h"
#include "Items/ACRPGArrowProjectile.h"

#include "Blueprint/UserWidget.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UACRPGEquipmentComponent::UACRPGEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRPGEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AACRPGCharacterBase>(GetOwner());
}

void UACRPGEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Personaj o'chganda qurollar dunyoda osilib qolmasin.
	for (auto& Pair : SpawnedActors)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	SpawnedActors.Empty();

	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// KIYISH (Ep.#14, #17)
// ---------------------------------------------------------------------------

bool UACRPGEquipmentComponent::EquipItem(FName ItemID)
{
	if (ItemID.IsNone() || !OwnerCharacter)
	{
		return false;
	}

	const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
	const FACRPGItemData* Data = GI ? GI->FindItem(ItemID) : nullptr;
	if (!Data)
	{
		UE_LOG(LogACRPG, Warning, TEXT("EquipItem: '%s' topilmadi."), *ItemID.ToString());
		return false;
	}

	// Ep.#17-18 — slot buyumning O'ZIDA yozilgan, ya'ni qilichni boshga kiyib bo'lmaydi.
	const EEquipmentSlot Slot = Data->Slot;
	if (Slot == EEquipmentSlot::None)
	{
		UE_LOG(LogACRPG, Warning, TEXT("'%s' kiyiladigan buyum emas."), *ItemID.ToString());
		return false;
	}

	// Slotda boshqa narsa bo'lsa — avval yechamiz.
	if (IsSlotOccupied(Slot))
	{
		UnequipSlot(Slot);
	}

	EquippedItems.Add(Slot, ItemID);

	// --- Ep.#44-45: zirh bo'lsa mesh almashtiramiz ---
	if (Data->Category == EItemCategory::Armor)
	{
		if (const FName* CompName = ArmorMeshComponentNames.Find(Slot))
		{
			// Nomi bo'yicha mavjud komponentni topamiz (BP da oldindan qo'shilgan).
			USkeletalMeshComponent* ArmorComp = nullptr;

			TArray<USkeletalMeshComponent*> Comps;
			OwnerCharacter->GetComponents<USkeletalMeshComponent>(Comps);
			for (USkeletalMeshComponent* C : Comps)
			{
				if (C->GetFName() == *CompName)
				{
					ArmorComp = C;
					break;
				}
			}

			if (ArmorComp)
			{
				// Soft ref — kerak bo'lganda yuklaymiz.
				if (USkeletalMesh* Mesh = Data->ArmorMesh.LoadSynchronous())
				{
					ArmorComp->SetSkeletalMesh(Mesh);

					// Ep.#79 — zirh asosiy tananing pozasiga ergashadi.
					ArmorComp->SetLeaderPoseComponent(OwnerCharacter->GetMesh());
					ArmorMeshComponents.Add(Slot, ArmorComp);
				}
			}
		}
	}
	// --- Qurol/qalqon bo'lsa aktyor yaratamiz (Ep.#19) ---
	else if (Data->ItemActorClass.IsValid() || !Data->ItemActorClass.IsNull())
	{
		SpawnAndAttachItem(*Data, Slot);
	}

	RecalculateStats();
	OnEquipmentChanged.Broadcast(Slot, ItemID);

	UE_LOG(LogACRPG, Log, TEXT("Kiyildi: %s -> slot %d"), *ItemID.ToString(), static_cast<int32>(Slot));
	return true;
}

AACRPGItemBase* UACRPGEquipmentComponent::SpawnAndAttachItem(const FACRPGItemData& Data, EEquipmentSlot Slot)
{
	UWorld* World = OwnerCharacter ? OwnerCharacter->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	UClass* ActorClass = Data.ItemActorClass.LoadSynchronous();
	if (!ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = OwnerCharacter;
	Params.Instigator = OwnerCharacter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AACRPGItemBase* NewItem = World->SpawnActor<AACRPGItemBase>(
		ActorClass, FTransform::Identity, Params);

	if (!NewItem)
	{
		return nullptr;
	}

	NewItem->OnEquipped(OwnerCharacter);
	SpawnedActors.Add(Slot, NewItem);

	UpdateAttachments();
	return NewItem;
}

bool UACRPGEquipmentComponent::UnequipSlot(EEquipmentSlot Slot)
{
	if (!IsSlotOccupied(Slot))
	{
		return false;
	}

	DestroySlotActor(Slot);

	// Zirh bo'lsa mesh'ni bo'shatamiz.
	if (TObjectPtr<USkeletalMeshComponent>* ArmorComp = ArmorMeshComponents.Find(Slot))
	{
		if (*ArmorComp)
		{
			(*ArmorComp)->SetSkeletalMesh(nullptr);
		}
		ArmorMeshComponents.Remove(Slot);
	}

	EquippedItems.Remove(Slot);

	RecalculateStats();
	OnEquipmentChanged.Broadcast(Slot, NAME_None);
	return true;
}

void UACRPGEquipmentComponent::DestroySlotActor(EEquipmentSlot Slot)
{
	if (TObjectPtr<AACRPGItemBase>* Found = SpawnedActors.Find(Slot))
	{
		if (*Found)
		{
			(*Found)->OnUnequipped();
			(*Found)->Destroy();
		}
		SpawnedActors.Remove(Slot);
	}
}

FName UACRPGEquipmentComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	const FName* Found = EquippedItems.Find(Slot);
	return Found ? *Found : NAME_None;
}

bool UACRPGEquipmentComponent::IsSlotOccupied(EEquipmentSlot Slot) const
{
	return !GetEquippedItem(Slot).IsNone();
}

AACRPGWeaponBase* UACRPGEquipmentComponent::GetEquippedWeapon() const
{
	if (const TObjectPtr<AACRPGItemBase>* Found = SpawnedActors.Find(EEquipmentSlot::MainHand))
	{
		return Cast<AACRPGWeaponBase>(Found->Get());
	}
	return nullptr;
}

float UACRPGEquipmentComponent::GetWeaponDamage() const
{
	return CachedWeaponDamage;
}

// ---------------------------------------------------------------------------
// STATISTIKANI QAYTA HISOBLASH (Ep.#44)
// ---------------------------------------------------------------------------

void UACRPGEquipmentComponent::RecalculateStats()
{
	const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
	if (!GI)
	{
		return;
	}

	float TotalArmor = 0.f;
	CachedWeaponDamage = 0.f;

	for (const auto& Pair : EquippedItems)
	{
		const FACRPGItemData* Data = GI->FindItem(Pair.Value);
		if (!Data)
		{
			continue;
		}

		TotalArmor += Data->ArmorValue;

		// Urish faqat asosiy qo'ldagi quroldan olinadi.
		if (Pair.Key == EEquipmentSlot::MainHand)
		{
			CachedWeaponDamage = Data->BaseDamage;
		}
	}

	// Yagona haqiqat manbai: zirh qiymati StatsComponent'da saqlanadi.
	if (OwnerCharacter)
	{
		if (UACRPGStatsComponent* Stats = OwnerCharacter->GetStats())
		{
			Stats->SetArmorValue(TotalArmor);
		}
	}
}

// ---------------------------------------------------------------------------
// SOCKETLARGA ULASH (Ep.#19)
// ---------------------------------------------------------------------------

void UACRPGEquipmentComponent::UpdateAttachments()
{
	if (!OwnerCharacter)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh)
	{
		return;
	}

	for (const auto& Pair : SpawnedActors)
	{
		AACRPGItemBase* Item = Pair.Value;
		if (!Item)
		{
			continue;
		}

		// Qurol chiqarilgan bo'lsa qo'lga, aks holda belga.
		const TMap<EEquipmentSlot, FName>& SocketMap = bWeaponDrawn ? DrawnSockets : SheathedSockets;
		const FName* Socket = SocketMap.Find(Pair.Key);

		if (!Socket || !Mesh->DoesSocketExist(*Socket))
		{
			UE_LOG(LogACRPG, Warning, TEXT("Socket topilmadi: slot %d"), static_cast<int32>(Pair.Key));
			continue;
		}

		Item->AttachToComponent(
			Mesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			*Socket);
	}
}

void UACRPGEquipmentComponent::ToggleWeaponDrawn()
{
	bWeaponDrawn = !bWeaponDrawn;
	UpdateAttachments();
}

// ---------------------------------------------------------------------------
// KAMON (Ep.#46-51)
// ---------------------------------------------------------------------------

void UACRPGEquipmentComponent::SetAiming(bool bNewAiming)
{
	if (!HasBowEquipped())
	{
		bIsAiming = false;
		return;
	}

	bIsAiming = bNewAiming;

	// Nishonga olayotganda kamon albatta qo'lda bo'lishi kerak.
	if (bIsAiming && !bWeaponDrawn)
	{
		bWeaponDrawn = true;
		UpdateAttachments();
	}
}

int32 UACRPGEquipmentComponent::GetArrowCount() const
{
	if (!OwnerCharacter)
	{
		return 0;
	}

	if (const UACRPGInventoryComponent* Inv =
		OwnerCharacter->FindComponentByClass<UACRPGInventoryComponent>())
	{
		return Inv->GetItemCount(ArrowItemID);
	}
	return 0;
}

bool UACRPGEquipmentComponent::FireArrow()
{
	if (!bIsAiming || !OwnerCharacter || !ArrowProjectileClass)
	{
		return false;
	}

	UWorld* World = OwnerCharacter->GetWorld();
	if (!World)
	{
		return false;
	}

	// Otish tezligi cheklovi.
	if (World->GetTimeSeconds() - LastFireTime < FireCooldown)
	{
		return false;
	}

	// --- Ep.#51: o'q-dori bormi? ---
	UACRPGInventoryComponent* Inv = OwnerCharacter->FindComponentByClass<UACRPGInventoryComponent>();
	if (!Inv || !Inv->RemoveItem(ArrowItemID, 1))
	{
		UE_LOG(LogACRPG, Log, TEXT("O'q tugadi."));
		return false;
	}

	LastFireTime = World->GetTimeSeconds();

	// --- O'q qayerdan uchadi ---
	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		if (Mesh->DoesSocketExist(ArrowSpawnSocket))
		{
			SpawnLocation = Mesh->GetSocketLocation(ArrowSpawnSocket);
		}
	}

	// --- Qayerga uchadi ---
	// Kamera markazidan trace qilamiz: o'yinchi ekranda ko'rgan joyga tegsin.
	FVector AimDirection = OwnerCharacter->GetBaseAimRotation().Vector();

	if (const APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		const FVector TraceEnd = CamLoc + CamRot.Vector() * 20000.f;

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ArrowAim), false, OwnerCharacter);

		if (World->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params))
		{
			AimDirection = (Hit.ImpactPoint - SpawnLocation).GetSafeNormal();
		}
		else
		{
			AimDirection = (TraceEnd - SpawnLocation).GetSafeNormal();
		}
	}

	FActorSpawnParameters Params;
	Params.Owner = OwnerCharacter;
	Params.Instigator = OwnerCharacter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AACRPGArrowProjectile* Arrow = World->SpawnActor<AACRPGArrowProjectile>(
		ArrowProjectileClass, SpawnLocation, AimDirection.Rotation(), Params);

	if (Arrow)
	{
		// Ep.#50 — o'q urishi kamon xususiyatiga bog'liq.
		const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
		float BowDamage = ArrowBaseDamage;

		if (GI)
		{
			if (const FACRPGItemData* BowData = GI->FindItem(GetEquippedItem(EEquipmentSlot::Ranged)))
			{
				BowDamage = BowData->BaseDamage > 0.f ? BowData->BaseDamage : ArrowBaseDamage;
			}
		}

		Arrow->InitializeArrow(AimDirection * ArrowSpeed, BowDamage, OwnerCharacter);
	}

	if (ShootMontage)
	{
		OwnerCharacter->PlayAnimMontage(ShootMontage);
	}

	OnAmmoChanged.Broadcast(GetArrowCount());
	return true;
}

// ---------------------------------------------------------------------------
// MENYU (Ep.#15)
// ---------------------------------------------------------------------------

void UACRPGEquipmentComponent::ToggleEquipmentMenu()
{
	APlayerController* PC = OwnerCharacter
		? Cast<APlayerController>(OwnerCharacter->GetController())
		: nullptr;

	if (!PC || !EquipmentMenuClass)
	{
		return;
	}

	if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport())
	{
		EquipmentMenuWidget->RemoveFromParent();
		EquipmentMenuWidget = nullptr;

		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
		return;
	}

	EquipmentMenuWidget = CreateWidget<UUserWidget>(PC, EquipmentMenuClass);
	if (EquipmentMenuWidget)
	{
		EquipmentMenuWidget->AddToViewport();

		// Ep.#17 — menyuda personaj aylanadigan bo'lsa GameAndUI kerak.
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
}

// ---------------------------------------------------------------------------
// SAQLASH (Ep.#80)
// ---------------------------------------------------------------------------

void UACRPGEquipmentComponent::LoadFromSave(const TMap<EEquipmentSlot, FName>& SavedEquipment)
{
	// Avval hammasini tozalaymiz.
	TArray<EEquipmentSlot> CurrentSlots;
	EquippedItems.GetKeys(CurrentSlots);
	for (EEquipmentSlot Slot : CurrentSlots)
	{
		UnequipSlot(Slot);
	}

	// Keyin saqlanganini kiyamiz.
	for (const auto& Pair : SavedEquipment)
	{
		EquipItem(Pair.Value);
	}
}
