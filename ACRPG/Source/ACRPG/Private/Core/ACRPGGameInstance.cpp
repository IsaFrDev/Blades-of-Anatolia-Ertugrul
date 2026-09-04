#include "Core/ACRPGGameInstance.h"

#include "Core/ACRPG.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"

UACRPGGameInstance::UACRPGGameInstance()
{
}

UACRPGGameInstance* UACRPGGameInstance::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	return Cast<UACRPGGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
}

const FACRPGItemData* UACRPGGameInstance::FindItem(FName ItemID) const
{
	if (!ItemDataTable || ItemID.IsNone())
	{
		return nullptr;
	}

	// Oxirgi argument — xato bo'lsa log'ga yozadigan kontekst matni.
	// `false` = topilmasa warning chiqarmasin (biz o'zimiz hal qilamiz).
	return ItemDataTable->FindRow<FACRPGItemData>(ItemID, TEXT("UACRPGGameInstance::FindItem"), false);
}

bool UACRPGGameInstance::GetItemData(FName ItemID, FACRPGItemData& OutItem) const
{
	if (const FACRPGItemData* Found = FindItem(ItemID))
	{
		OutItem = *Found;
		return true;
	}
	UE_LOG(LogACRPG, Warning, TEXT("Buyum topilmadi: %s"), *ItemID.ToString());
	return false;
}

const FACRPGQuestData* UACRPGGameInstance::FindQuest(FName QuestID) const
{
	if (!QuestDataTable || QuestID.IsNone())
	{
		return nullptr;
	}
	return QuestDataTable->FindRow<FACRPGQuestData>(QuestID, TEXT("UACRPGGameInstance::FindQuest"), false);
}

bool UACRPGGameInstance::GetQuestData(FName QuestID, FACRPGQuestData& OutQuest) const
{
	if (const FACRPGQuestData* Found = FindQuest(QuestID))
	{
		OutQuest = *Found;
		return true;
	}
	UE_LOG(LogACRPG, Warning, TEXT("Kvest topilmadi: %s"), *QuestID.ToString());
	return false;
}
