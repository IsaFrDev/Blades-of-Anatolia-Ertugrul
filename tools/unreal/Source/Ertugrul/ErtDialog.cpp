#include "ErtDialog.h"
#include "Ertugrul.h"
#include "ErtLoc.h"
#include "ErtAudio.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FErtDialog::Start(const FString& Id, TSet<FString>* InFlags, int32* InHonor)
{
	End();
	if (Id.IsEmpty()) return false;
	Flags = InFlags; Honor = InHonor;
	const FString Path = FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/dialogue") / (Id + TEXT(".json"));
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path)) { UE_LOG(LogErtugrul, Warning, TEXT("Dialog topilmadi: %s"), *Path); return false; }
	TSharedPtr<FJsonObject> R;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) || !R.IsValid()) return false;
	Nodes.Reset();
	const TSharedPtr<FJsonObject>* NodesObj = nullptr;
	if (!R->TryGetObjectField(TEXT("nodes"), NodesObj)) return false;
	for (const auto& Pair : (*NodesObj)->Values)
	{
		const TSharedPtr<FJsonObject> O = Pair.Value->AsObject();
		if (!O.IsValid()) continue;
		FErtDlgNode N;
		O->TryGetStringField(TEXT("speaker"), N.Speaker);
		O->TryGetStringField(TEXT("text_key"), N.TextKey);
		O->TryGetStringField(TEXT("next"), N.Next);
		O->TryGetStringField(TEXT("set_flag"), N.SetFlag);
		const TArray<TSharedPtr<FJsonValue>>* Opts = nullptr;
		if (O->TryGetArrayField(TEXT("options"), Opts))
			for (const auto& V : *Opts)
			{
				const TSharedPtr<FJsonObject> OO = V->AsObject(); if (!OO.IsValid()) continue;
				FErtDlgOption Op;
				OO->TryGetStringField(TEXT("text_key"), Op.TextKey);
				OO->TryGetStringField(TEXT("next"), Op.Next);
				OO->TryGetStringField(TEXT("set_flag"), Op.SetFlag);
				OO->TryGetStringField(TEXT("option_id"), Op.OptionId);
				OO->TryGetStringField(TEXT("requires_evidence"), Op.RequiresEvidence);
				OO->TryGetNumberField(TEXT("honor"), Op.Honor);
				OO->TryGetNumberField(TEXT("duel_points"), Op.DuelPoints);
				N.Options.Add(Op);
			}
		const FString NodeKey(Pair.Key.ToView());
		Nodes.FindOrAdd(NodeKey) = N;
	}
	DuelPoints = 0; DuelThreshold = 0;
	R->TryGetNumberField(TEXT("duel_threshold"), DuelThreshold);
	FString StartId;
	R->TryGetStringField(TEXT("start"), StartId);
	R->TryGetStringField(TEXT("id"), DialogId);
	if (DialogId.IsEmpty()) DialogId = Id;
	if (!Nodes.Contains(StartId)) return false;
	bActive = true;
	Enter(StartId);
	UE_LOG(LogErtugrul, Log, TEXT("Dialog %s boshlandi (%d tugun)"), *DialogId, Nodes.Num());
	return true;
}

void FErtDialog::Enter(const FString& NodeId)
{
	if (NodeId == TEXT("end") || !Nodes.Contains(NodeId)) { End(); return; }
	Cur = NodeId;
	const FErtDlgNode& N = Nodes[Cur];
	if (GEngine && GEngine->GetWorldContexts().Num()) FErtAudio::PlayVo(GEngine->GetWorldContexts()[0].World(), N.TextKey, 1.f);
	if (!N.SetFlag.IsEmpty() && Flags) Flags->Add(N.SetFlag);
	VisibleOpts.Reset(); OptTexts.Reset(); Sel = 0;
	for (int32 i = 0; i < N.Options.Num(); ++i)
	{
		const FErtDlgOption& O = N.Options[i];
		if (!O.RequiresEvidence.IsEmpty() && (!Flags || !Flags->Contains(O.RequiresEvidence))) continue;
		VisibleOpts.Add(i);
		OptTexts.Add(FErtLoc::Get().Tr(O.TextKey));
	}
}

void FErtDialog::Advance()
{
	if (!bActive) return;
	if (OptTexts.Num() > 0) { Choose(Sel); return; }
	Enter(Nodes[Cur].Next);
}

void FErtDialog::Choose(int32 Index)
{
	if (!bActive || !VisibleOpts.IsValidIndex(Index)) return;
	const FErtDlgOption O = Nodes[Cur].Options[VisibleOpts[Index]];
	if (!O.SetFlag.IsEmpty() && Flags) Flags->Add(O.SetFlag);
	if (Honor) *Honor += O.Honor;
	DuelPoints += O.DuelPoints;
	// Dalil ishlatilgach, u qayta ko'rsatilmaydi
	if (!O.RequiresEvidence.IsEmpty() && Flags) Flags->Remove(O.RequiresEvidence);
	Enter(O.Next.IsEmpty() ? TEXT("end") : O.Next);
}

void FErtDialog::MoveSelection(int32 Delta)
{
	if (OptTexts.Num() == 0) return;
	Sel = (Sel + Delta + OptTexts.Num()) % OptTexts.Num();
}

void FErtDialog::End()
{
	if (bActive) FErtAudio::StopVo();
	bActive = false;
	Cur.Reset(); OptTexts.Reset(); VisibleOpts.Reset();
}

FString FErtDialog::Speaker() const
{
	if (!bActive || !Nodes.Contains(Cur)) return FString();
	const FString& S = Nodes[Cur].Speaker;
	return FErtLoc::Get().TrOr(S, S);
}

FString FErtDialog::Text() const
{
	if (!bActive || !Nodes.Contains(Cur)) return FString();
	return FErtLoc::Get().Tr(Nodes[Cur].TextKey);
}
