#include "ErtEpisodeDb.h"
#include "Ertugrul.h"
#include "ErtLoc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UErtEpisodeDb* UErtEpisodeDb::Get()
{
	static TWeakObjectPtr<UErtEpisodeDb> Inst;
	if (!Inst.IsValid())
	{
		UErtEpisodeDb* Db = NewObject<UErtEpisodeDb>(GetTransientPackage(), TEXT("ErtEpisodeDb"));
		Db->AddToRoot();
		Db->Load();
		Inst = Db;
	}
	return Inst.Get();
}

static FString Str(const TSharedPtr<FJsonObject>& O, const TCHAR* Field)
{
	FString S; if (O.IsValid()) O->TryGetStringField(FString(Field), S); return S;
}
static int32 Int(const TSharedPtr<FJsonObject>& O, const TCHAR* Field, int32 Def = 0)
{
	int32 V = Def; if (O.IsValid()) O->TryGetNumberField(FString(Field), V); return V;
}

void UErtEpisodeDb::Load()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/episodes_v2.json");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path)) { UE_LOG(LogErtugrul, Error, TEXT("Epizod bazasi topilmadi: %s"), *Path); return; }
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) { UE_LOG(LogErtugrul, Error, TEXT("episodes_v2.json noto'g'ri")); return; }
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (!Root->TryGetArrayField(TEXT("episodes"), Arr)) return;
	for (const TSharedPtr<FJsonValue>& V : *Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O.IsValid()) continue;
		FErtEpisode E;
		E.Id = Str(O, TEXT("id"));
		E.SeasonId = Str(O, TEXT("season_id"));
		E.GlobalIndex = Int(O, TEXT("global_index"));
		E.Archetype = Str(O, TEXT("archetype"));
		E.DifficultyTier = Int(O, TEXT("difficulty_tier"), 1);
		E.EstimatedMinutes = Int(O, TEXT("estimated_minutes"));
		E.LocTitle = Str(O, TEXT("loc_key_title"));
		E.LocSynopsis = Str(O, TEXT("loc_key_synopsis"));
		E.LocCliffhanger = Str(O, TEXT("loc_key_cliffhanger"));
		const TSharedPtr<FJsonObject>* Sub = nullptr;
		if (O->TryGetObjectField(TEXT("enemy_composition"), Sub)) E.MaxSimultaneous = Int(*Sub, TEXT("max_simultaneous"));
		if (O->TryGetObjectField(TEXT("anchor"), Sub)) E.Gregorian = Str(*Sub, TEXT("gregorian"));
		if (O->TryGetObjectField(TEXT("intro"), Sub)) E.LocIntro = Str(*Sub, TEXT("loc_key_description"));
		if (O->TryGetObjectField(TEXT("environment"), Sub))
		{
			E.Season = Str(*Sub, TEXT("season")); E.TimeOfDay = Str(*Sub, TEXT("time_of_day"));
			E.Weather = Str(*Sub, TEXT("weather")); E.Region = Str(*Sub, TEXT("region"));
		}
		const TArray<TSharedPtr<FJsonValue>>* L = nullptr;
		if (O->TryGetArrayField(TEXT("unlocks"), L) && L->Num() > 0) E.NextId = (*L)[0]->AsString();
		if (O->TryGetArrayField(TEXT("prerequisites"), L)) for (const auto& P : *L) E.Prerequisites.Add(P->AsString());
		if (O->TryGetArrayField(TEXT("traversal"), L)) for (const auto& T : *L) { const FString S = T->AsString(); if (S == TEXT("HORSE") || S == TEXT("CAMEL")) E.bHorse = true; }
		Episodes.Add(E);
	}
	Episodes.Sort([](const FErtEpisode& A, const FErtEpisode& B) { return A.GlobalIndex < B.GlobalIndex; });
	UE_LOG(LogErtugrul, Log, TEXT("Epizod bazasi: %d epizod"), Episodes.Num());
}

const FErtEpisode* UErtEpisodeDb::ById(const FString& Id) const
{
	return Episodes.FindByPredicate([&](const FErtEpisode& E) { return E.Id == Id; });
}

FString UErtEpisodeDb::Title(const FErtEpisode& E) const
{
	return FErtLoc::Get().TrOr(E.LocTitle, E.Id);
}
