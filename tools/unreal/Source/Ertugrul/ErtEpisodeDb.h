// episodes_v2.json (48 epizod, 4 mavsum) yuklovchi.
#pragma once

#include "CoreMinimal.h"
#include "ErtEpisodeDb.generated.h"

USTRUCT(BlueprintType)
struct FErtEpisode
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Id;
	UPROPERTY(BlueprintReadOnly) FString SeasonId;
	UPROPERTY(BlueprintReadOnly) int32 GlobalIndex = 0;
	UPROPERTY(BlueprintReadOnly) FString Archetype;      // INVESTIGATION / SIEGE / ...
	UPROPERTY(BlueprintReadOnly) int32 DifficultyTier = 1;
	UPROPERTY(BlueprintReadOnly) int32 MaxSimultaneous = 0;
	UPROPERTY(BlueprintReadOnly) int32 EstimatedMinutes = 0;
	UPROPERTY(BlueprintReadOnly) FString LocTitle;
	UPROPERTY(BlueprintReadOnly) FString LocSynopsis;
	UPROPERTY(BlueprintReadOnly) FString LocCliffhanger;
	UPROPERTY(BlueprintReadOnly) FString LocIntro;
	UPROPERTY(BlueprintReadOnly) FString Gregorian;      // "1227 Avgust"
	UPROPERTY(BlueprintReadOnly) FString Season;
	UPROPERTY(BlueprintReadOnly) FString TimeOfDay;
	UPROPERTY(BlueprintReadOnly) FString Weather;
	UPROPERTY(BlueprintReadOnly) FString Region;
	UPROPERTY(BlueprintReadOnly) FString NextId;         // unlocks[0]
	UPROPERTY(BlueprintReadOnly) TArray<FString> Prerequisites;
	UPROPERTY(BlueprintReadOnly) bool bHorse = false;   // traversal: HORSE / CAMEL
};

UCLASS()
class ERTUGRUL_API UErtEpisodeDb : public UObject
{
	GENERATED_BODY()
public:
	static UErtEpisodeDb* Get();
	bool IsLoaded() const { return Episodes.Num() > 0; }
	const TArray<FErtEpisode>& All() const { return Episodes; }
	const FErtEpisode* ById(const FString& Id) const;
	FString Title(const FErtEpisode& E) const;

private:
	void Load();
	UPROPERTY() TArray<FErtEpisode> Episodes;
};
