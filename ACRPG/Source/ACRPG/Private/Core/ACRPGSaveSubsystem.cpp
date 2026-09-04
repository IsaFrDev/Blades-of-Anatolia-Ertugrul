#include "Core/ACRPGSaveSubsystem.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGSaveGame.h"
#include "Core/ACRPGGameInstance.h"
#include "Character/ACRPGPlayerCharacter.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGInventoryComponent.h"
#include "Components/ACRPGEquipmentComponent.h"
#include "Components/ACRPGQuestComponent.h"
#include "World/ACRPGDayNightCycle.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"	// TActorIterator uchun

UACRPGSaveSubsystem* UACRPGSaveSubsystem::Get(const UObject* WorldContextObject)
{
	if (const UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UACRPGSaveSubsystem>();
	}
	return nullptr;
}

FString UACRPGSaveSubsystem::ResolveSlotName(const FString& SlotName) const
{
	if (!SlotName.IsEmpty())
	{
		return SlotName;
	}

	if (const UACRPGGameInstance* GI = Cast<UACRPGGameInstance>(GetGameInstance()))
	{
		return GI->SaveSlotName;
	}
	return TEXT("ACRPG_Slot0");
}

// ---------------------------------------------------------------------------
// SAQLASH (Ep.#80-81)
// ---------------------------------------------------------------------------

bool UACRPGSaveSubsystem::SaveGame(const FString& SlotName)
{
	UWorld* World = GetWorld();
	AACRPGPlayerCharacter* Player =
		Cast<AACRPGPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));

	if (!Player)
	{
		UE_LOG(LogACRPG, Error, TEXT("SaveGame: o'yinchi topilmadi."));
		OnGameSaved.Broadcast(false);
		return false;
	}

	UACRPGSaveGame* Save = Cast<UACRPGSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UACRPGSaveGame::StaticClass()));

	if (!Save)
	{
		OnGameSaved.Broadcast(false);
		return false;
	}

	// --- Meta ---
	Save->SaveSlotName = ResolveSlotName(SlotName);
	Save->SaveDate = FDateTime::Now();
	Save->LevelName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));

	// --- O'yinchi ---
	Save->PlayerLocation = Player->GetActorLocation();
	Save->PlayerRotation = Player->GetActorRotation();

	if (const UACRPGStatsComponent* Stats = Player->GetStats())
	{
		Save->Health = Stats->GetHealth();
		Save->Stamina = Stats->GetStamina();
		Save->Level = Stats->GetLevel();
		Save->CurrentXP = Stats->GetCurrentXP();
	}

	// --- Inventar / ekipirovka ---
	if (const UACRPGInventoryComponent* Inv = Player->GetInventory())
	{
		Save->InventoryItems = Inv->GetAllItems();
	}
	if (const UACRPGEquipmentComponent* Equip = Player->GetEquipment())
	{
		Save->EquippedItems = Equip->GetEquippedMap();
	}

	// --- Ep.#81: kvestlar ---
	if (const UACRPGQuestComponent* Quests = Player->GetQuests())
	{
		Save->QuestLog = Quests->GetQuestLog();
		Save->ActiveQuestID = Quests->GetActiveQuestID();
	}

	// --- Ep.#81: kun vaqti ---
	if (AActor* Cycle = UGameplayStatics::GetActorOfClass(World, AACRPGDayNightCycle::StaticClass()))
	{
		Save->TimeOfDay = Cast<AACRPGDayNightCycle>(Cycle)->GetTimeOfDay();
	}

	Save->DestroyedActorNames = DestroyedActors;

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(Save, Save->SaveSlotName, 0);

	UE_LOG(LogACRPG, Log, TEXT("Saqlash %s: slot '%s'"),
		bSuccess ? TEXT("muvaffaqiyatli") : TEXT("muvaffaqiyatsiz"), *Save->SaveSlotName);

	OnGameSaved.Broadcast(bSuccess);
	return bSuccess;
}

// ---------------------------------------------------------------------------
// YUKLASH (Ep.#80-81)
// ---------------------------------------------------------------------------

bool UACRPGSaveSubsystem::LoadGame(const FString& SlotName)
{
	const FString Slot = ResolveSlotName(SlotName);

	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		UE_LOG(LogACRPG, Warning, TEXT("LoadGame: '%s' sloti yo'q."), *Slot);
		OnGameLoaded.Broadcast(false);
		return false;
	}

	UACRPGSaveGame* Save = Cast<UACRPGSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!Save)
	{
		OnGameLoaded.Broadcast(false);
		return false;
	}

	UWorld* World = GetWorld();
	AACRPGPlayerCharacter* Player =
		Cast<AACRPGPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));

	if (!Player)
	{
		OnGameLoaded.Broadcast(false);
		return false;
	}

	// --- TARTIB MUHIM ---
	// 1) Pozitsiya  2) Statistika  3) Inventar  4) Ekipirovka  5) Kvestlar
	//
	// Ekipirovka inventardan KEYIN yuklanadi, chunki kiyish uchun buyum
	// inventarda bo'lishi kerak. Bu Ep.#80 dagi klassik xato.

	Player->SetActorLocationAndRotation(Save->PlayerLocation, Save->PlayerRotation, false, nullptr,
		ETeleportType::TeleportPhysics);

	if (UACRPGStatsComponent* Stats = Player->GetStats())
	{
		Stats->LoadFromSave(Save->Health, Save->Stamina, Save->Level, Save->CurrentXP);
	}

	if (UACRPGInventoryComponent* Inv = Player->GetInventory())
	{
		Inv->LoadFromSave(Save->InventoryItems);
	}

	if (UACRPGEquipmentComponent* Equip = Player->GetEquipment())
	{
		Equip->LoadFromSave(Save->EquippedItems);
	}

	if (UACRPGQuestComponent* Quests = Player->GetQuests())
	{
		Quests->LoadFromSave(Save->QuestLog, Save->ActiveQuestID);
	}

	// --- Ep.#81: kun vaqti ---
	if (AActor* Cycle = UGameplayStatics::GetActorOfClass(World, AACRPGDayNightCycle::StaticClass()))
	{
		Cast<AACRPGDayNightCycle>(Cycle)->SetTimeOfDay(Save->TimeOfDay);
	}

	// --- O'ldirilgan aktyorlarni qayta o'chiramiz ---
	DestroyedActors = Save->DestroyedActorNames;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (DestroyedActors.Contains(It->GetFName()))
		{
			It->Destroy();
		}
	}

	UE_LOG(LogACRPG, Log, TEXT("Yuklandi: slot '%s' (%s)"),
		*Slot, *Save->SaveDate.ToString());

	OnGameLoaded.Broadcast(true);
	return true;
}

bool UACRPGSaveSubsystem::DoesSaveExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(ResolveSlotName(SlotName), 0);
}

bool UACRPGSaveSubsystem::DeleteSave(const FString& SlotName)
{
	return UGameplayStatics::DeleteGameInSlot(ResolveSlotName(SlotName), 0);
}

void UACRPGSaveSubsystem::MarkActorDestroyed(AActor* Actor)
{
	if (Actor)
	{
		DestroyedActors.Add(Actor->GetFName());
	}
}
