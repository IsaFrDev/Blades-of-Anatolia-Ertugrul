// CSV lokalizatsiya: "kalit","uz","tr","en" (Content/Ertugrul/Data/*.csv)
#pragma once

#include "CoreMinimal.h"

class ERTUGRUL_API FErtLoc
{
public:
	static FErtLoc& Get();
	/** 0 = uz, 1 = tr, 2 = en */
	void SetLanguage(int32 Index) { Lang = FMath::Clamp(Index, 0, 2); }
	int32 GetLanguage() const { return Lang; }
	FString Tr(const FString& Key) const;               // topilmasa kalitning o'zi
	FString TrOr(const FString& Key, const FString& Fallback) const;
	bool Has(const FString& Key) const { return Table.Contains(Key); }
	int32 Num() const { return Table.Num(); }

private:
	FErtLoc();
	void LoadCsv(const FString& Path);
	TMap<FString, TArray<FString>> Table;
	int32 Lang = 0;
};
