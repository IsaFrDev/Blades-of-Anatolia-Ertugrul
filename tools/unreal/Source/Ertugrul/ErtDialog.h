// Dialog grafi ijrochisi: Content/Ertugrul/Data/dialogue/<id>.json
// Tugunlar: speaker, text_key, next ("end" = tugadi), set_flag, options[] (text_key, next, set_flag, honor, requires_evidence).
#pragma once

#include "CoreMinimal.h"

struct FErtDlgOption { FString TextKey, Next, SetFlag, OptionId, RequiresEvidence; int32 Honor = 0; int32 DuelPoints = 0; };
struct FErtDlgNode { FString Speaker, TextKey, Next, SetFlag; TArray<FErtDlgOption> Options; };

class ERTUGRUL_API FErtDialog
{
public:
	/** Flags - o'yin bayroqlari (dalillar, va'dalar), Honor - or/iymon hisobi */
	bool Start(const FString& Id, TSet<FString>* InFlags, int32* InHonor);
	bool IsActive() const { return bActive; }
	void Advance();               // tanlovsiz tugun: keyingisi; tanlovli: tanlanganini tanlaydi
	void Choose(int32 Index);
	void MoveSelection(int32 Delta);
	void End();

	const FString& GetId() const { return DialogId; }
	FString Speaker() const;
	FString Text() const;
	const TArray<FString>& OptionTexts() const { return OptTexts; }
	int32 Selection() const { return Sel; }
	bool HasOptions() const { return OptTexts.Num() > 0; }
	int32 GetDuelPoints() const { return DuelPoints; }
	int32 GetDuelThreshold() const { return DuelThreshold; }

private:
	void Enter(const FString& NodeId);
	bool bActive = false;
	FString DialogId, Cur;
	TMap<FString, FErtDlgNode> Nodes;
	TArray<int32> VisibleOpts;
	TArray<FString> OptTexts;
	int32 Sel = 0;
	int32 DuelPoints = 0, DuelThreshold = 0;
	TSet<FString>* Flags = nullptr;
	int32* Honor = nullptr;
};
