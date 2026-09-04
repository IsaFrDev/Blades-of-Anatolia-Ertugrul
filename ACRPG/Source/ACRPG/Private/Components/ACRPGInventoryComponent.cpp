#include "Components/ACRPGInventoryComponent.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGGameInstance.h"

UACRPGInventoryComponent::UACRPGInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRPGInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	for (const FACRPGInventoryEntry& Entry : StartingItems)
	{
		AddItem(Entry.ItemID, Entry.Quantity);
	}
}

int32 UACRPGInventoryComponent::AddItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0)
	{
		return 0;
	}

	const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
	const FACRPGItemData* Data = GI ? GI->FindItem(ItemID) : nullptr;
	if (!Data)
	{
		UE_LOG(LogACRPG, Warning, TEXT("AddItem: '%s' Data Table'da yo'q."), *ItemID.ToString());
		return 0;
	}

	const int32 MaxStack = FMath::Max(1, Data->MaxStack);
	int32 Remaining = Quantity;

	// 1) Avval mavjud stack'larni to'ldiramiz.
	for (FACRPGInventoryEntry& Entry : Items)
	{
		if (Entry.ItemID != ItemID || Remaining <= 0)
		{
			continue;
		}

		const int32 Space = MaxStack - Entry.Quantity;
		if (Space <= 0)
		{
			continue;
		}

		const int32 ToAdd = FMath::Min(Space, Remaining);
		Entry.Quantity += ToAdd;
		Remaining -= ToAdd;
	}

	// 2) Qolganini yangi stack'larga solamiz.
	while (Remaining > 0)
	{
		if (MaxSlots > 0 && Items.Num() >= MaxSlots)
		{
			UE_LOG(LogACRPG, Log, TEXT("Inventar to'la — %d dona '%s' sig'madi."),
				Remaining, *ItemID.ToString());
			break;
		}

		FACRPGInventoryEntry NewEntry;
		NewEntry.ItemID = ItemID;
		NewEntry.Quantity = FMath::Min(MaxStack, Remaining);
		Items.Add(NewEntry);

		Remaining -= NewEntry.Quantity;
	}

	const int32 Added = Quantity - Remaining;
	if (Added > 0)
	{
		OnInventoryChanged.Broadcast();
	}
	return Added;
}

bool UACRPGInventoryComponent::RemoveItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0 || GetItemCount(ItemID) < Quantity)
	{
		return false;	// Hammasi bormi — avval tekshiramiz, keyin olamiz.
	}

	int32 Remaining = Quantity;

	// Teskari tartibda yuramiz — o'chirish indekslarni buzmasin.
	for (int32 i = Items.Num() - 1; i >= 0 && Remaining > 0; --i)
	{
		if (Items[i].ItemID != ItemID)
		{
			continue;
		}

		const int32 ToRemove = FMath::Min(Items[i].Quantity, Remaining);
		Items[i].Quantity -= ToRemove;
		Remaining -= ToRemove;

		if (Items[i].Quantity <= 0)
		{
			Items.RemoveAt(i);
		}
	}

	OnInventoryChanged.Broadcast();
	return true;
}

int32 UACRPGInventoryComponent::GetItemCount(FName ItemID) const
{
	int32 Total = 0;
	for (const FACRPGInventoryEntry& Entry : Items)
	{
		if (Entry.ItemID == ItemID)
		{
			Total += Entry.Quantity;
		}
	}
	return Total;
}

TArray<FACRPGInventoryEntry> UACRPGInventoryComponent::GetItemsByCategory(EItemCategory Category) const
{
	TArray<FACRPGInventoryEntry> Result;

	const UACRPGGameInstance* GI = UACRPGGameInstance::Get(this);
	if (!GI)
	{
		return Result;
	}

	for (const FACRPGInventoryEntry& Entry : Items)
	{
		if (const FACRPGItemData* Data = GI->FindItem(Entry.ItemID))
		{
			if (Data->Category == Category)
			{
				Result.Add(Entry);
			}
		}
	}
	return Result;
}

void UACRPGInventoryComponent::LoadFromSave(const TArray<FACRPGInventoryEntry>& SavedItems)
{
	Items = SavedItems;
	OnInventoryChanged.Broadcast();
}
