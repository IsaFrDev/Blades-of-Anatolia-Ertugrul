#include "ErtLoc.h"
#include "Ertugrul.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FErtLoc& FErtLoc::Get()
{
	static FErtLoc Inst;
	return Inst;
}

FErtLoc::FErtLoc()
{
	const FString Dir = FPaths::ProjectContentDir() / TEXT("Ertugrul/Data");
	LoadCsv(Dir / TEXT("ui_loc.csv"));
	LoadCsv(Dir / TEXT("episodes_loc.csv"));
	LoadCsv(Dir / TEXT("cutscene_loc.csv"));
	UE_LOG(LogErtugrul, Log, TEXT("Lokalizatsiya: %d kalit"), Table.Num());
}

// Qo'shtirnoqli CSV qatorini maydonlarga ajratadi ("" = qo'shtirnoq)
static void ParseCsvLine(const FString& Line, TArray<FString>& Out)
{
	Out.Reset();
	FString Cur;
	bool bQuoted = false;
	for (int32 i = 0; i < Line.Len(); ++i)
	{
		const TCHAR C = Line[i];
		if (bQuoted)
		{
			if (C == TEXT('"'))
			{
				if (i + 1 < Line.Len() && Line[i + 1] == TEXT('"')) { Cur.AppendChar(TEXT('"')); ++i; }
				else bQuoted = false;
			}
			else Cur.AppendChar(C);
		}
		else
		{
			if (C == TEXT('"')) bQuoted = true;
			else if (C == TEXT(',')) { Out.Add(Cur); Cur.Reset(); }
			else Cur.AppendChar(C);
		}
	}
	Out.Add(Cur);
}

void FErtLoc::LoadCsv(const FString& Path)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path)) { UE_LOG(LogErtugrul, Warning, TEXT("CSV topilmadi: %s"), *Path); return; }
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, true);
	TArray<FString> F;
	for (const FString& L : Lines)
	{
		if (L.StartsWith(TEXT("#"))) continue;
		ParseCsvLine(L, F);
		if (F.Num() < 2 || F[0] == TEXT("key") || F[0] == TEXT("keys")) continue;
		TArray<FString> Vals;
		for (int32 i = 1; i < F.Num() && i <= 3; ++i) Vals.Add(F[i]);
		Table.Add(F[0], Vals);
	}
}

FString FErtLoc::Tr(const FString& Key) const
{
	return TrOr(Key, Key);
}

FString FErtLoc::TrOr(const FString& Key, const FString& Fallback) const
{
	const TArray<FString>* V = Table.Find(Key);
	if (!V || V->Num() == 0) return Fallback;
	const int32 I = FMath::Min(Lang, V->Num() - 1);
	return (*V)[I].IsEmpty() ? (*V)[0] : (*V)[I];
}
