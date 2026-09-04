#include "ErtGameMode.h"
#include "Ertugrul.h"
#include "ErtCharacter.h"
#include "ErtCutscene.h"
#include "ErtEpisodeDb.h"
#include "ErtHUD.h"
#include "ErtHorse.h"
#include "ErtNpc.h"
#include "ErtWorldBuilder.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ErtMission.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

AErtGameMode::AErtGameMode()
{
	DefaultPawnClass = AErtCharacter::StaticClass();
	HUDClass = AErtHUD::StaticClass();
}

void AErtGameMode::BeginPlay()
{
	Super::BeginPlay();
	Director = GetWorld()->SpawnActor<AErtMissionDirector>();
	// Oba darvozasi oldida ikkita ot
	{
		FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FLinearColor Coats[] = { FLinearColor(0.36f, 0.22f, 0.11f), FLinearColor(0.15f, 0.12f, 0.10f) };
		for (int32 i = 0; i < 2; ++i)
		{
			AErtHorse* Hs = GetWorld()->SpawnActor<AErtHorse>(AErtHorse::StaticClass(), FVector(44600.f + i * 350.f, -56000.f + i * 250.f, 2250.f), FRotator(0, 0.f, 0), SP);
			if (Hs) Hs->Init(Coats[i]);
		}
	}
	Cutscene = GetWorld()->SpawnActor<AErtCutsceneDirector>();
	bUnlockAll = FParse::Param(FCommandLine::Get(), TEXT("ErtUnlockAll"));
	SpawnNpcs();
	FString Text;
	if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectSavedDir() / TEXT("ert_progress.txt")))) Text.ParseIntoArrayLines(Completed);

	FString Ep;
	const bool bDirect = FParse::Value(FCommandLine::Get(), TEXT("-ErtEpisode="), Ep);
	const bool bFree = FParse::Param(FCommandLine::Get(), TEXT("ErtFreeRoam"));
	const bool bCut = FParse::Param(FCommandLine::Get(), TEXT("ErtCutscene"));
	if (bFree) return;
	FTimerHandle Th;
	if (bDirect) GetWorldTimerManager().SetTimer(Th, [this, Ep, bCut]() { BeginEpisode(Ep, bCut); }, 0.6f, false);
	else GetWorldTimerManager().SetTimer(Th, [this]() { bMenuOpen = true; SetPlayerInput(false, false); }, 0.3f, false);
}

void AErtGameMode::SetPlayerInput(bool bEnabled, bool bHide)
{
	if (AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		H->bInputEnabled = bEnabled;
		H->SetActorHiddenInGame(bHide);
	}
}

bool AErtGameMode::IsCompleted(const FString& Id) const { return Completed.Contains(Id); }

bool AErtGameMode::IsUnlocked(const FErtEpisode& E) const
{
	if (bUnlockAll || E.GlobalIndex == 0 || IsCompleted(E.Id)) return true;
	for (const FString& P : E.Prerequisites) if (!IsCompleted(P)) return false;
	return true;
}

void AErtGameMode::SpawnNpcs()
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/npcs.json")))) return;
	TSharedPtr<FJsonObject> R;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) || !R.IsValid()) return;
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (!R->TryGetArrayField(TEXT("npcs"), Arr)) return;
	int32 N = 0;
	for (const auto& V : *Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O.IsValid()) continue;
		FString Id, Name, Place, Dlg; double U = 0, Vv = 0, Yaw = 0; bool bWoman = false;
		O->TryGetStringField(TEXT("id"), Id); O->TryGetStringField(TEXT("name"), Name); O->TryGetStringField(TEXT("place"), Place); O->TryGetStringField(TEXT("dialog"), Dlg);
		O->TryGetNumberField(TEXT("u"), U); O->TryGetNumberField(TEXT("v"), Vv); O->TryGetNumberField(TEXT("yaw"), Yaw); O->TryGetBoolField(TEXT("woman"), bWoman);
		FLinearColor Kaftan(0.3f, 0.2f, 0.1f);
		const TArray<TSharedPtr<FJsonValue>>* K = nullptr;
		if (O->TryGetArrayField(TEXT("kaftan"), K) && K->Num() >= 3) Kaftan = FLinearColor((*K)[0]->AsNumber(), (*K)[1]->AsNumber(), (*K)[2]->AsNumber());
		float E = ErtMap::ObaE, Nn = ErtMap::ObaN;
		if (Place == TEXT("city")) { E = ErtMap::CityE; Nn = ErtMap::CityN; }
		else if (Place == TEXT("caravan")) { E = ErtMap::CaravanE; Nn = ErtMap::CaravanN; }
		else if (Place == TEXT("camp")) { E = ErtMap::CampE; Nn = ErtMap::CampN; }
		E += U; Nn += Vv;
		const float X = Nn * 100.f, Y = E * 100.f;
		FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtNpcGround), true);
		float Z = 2000.f;
		if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(X, Y, 60000.f), FVector(X, Y, -5000.f), ECC_Visibility, Q)) Z = Hit.ImpactPoint.Z;
		AErtNpc* Npc = GetWorld()->SpawnActor<AErtNpc>(AErtNpc::StaticClass(), FVector(X, Y, Z + 92.f), FRotator(0, Yaw, 0));
		if (Npc) { Npc->Setup(Id, Name, Dlg, bWoman, Kaftan, Yaw); ++N; }
	}
	UE_LOG(LogErtugrul, Log, TEXT("NPC: %d"), N);
}

void AErtGameMode::StartDialog(AErtNpc* Npc)
{
	if (!Npc || Dialog.IsActive() || (Cutscene && Cutscene->IsPlaying())) return;
	if (!Dialog.Start(Npc->GetDialogId(), &Flags, &Honor)) return;
	SetPlayerInput(false, false);
}

void AErtGameMode::DialogChoose(int32 Index)
{
	if (!Dialog.IsActive()) return;
	Dialog.Choose(Index);
	if (!Dialog.IsActive()) EndDialog();
}

void AErtGameMode::EndDialog()
{
	Dialog.End();
	if (AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (Flags.Contains(TEXT("give_arrows"))) { Flags.Remove(TEXT("give_arrows")); H->AddArrows(8); }
		if (Flags.Contains(TEXT("sword_sharpened"))) { Flags.Remove(TEXT("sword_sharpened")); H->AttackDamage = 36.f; }
	}
	if (!bMenuOpen) SetPlayerInput(true, false);
	UE_LOG(LogErtugrul, Log, TEXT("Dialog tugadi, or/iymon: %d, bayroqlar: %d"), Honor, Flags.Num());
}

void AErtGameMode::MenuToggle()
{
	if (Cutscene && Cutscene->IsPlaying()) return;
	if (Dialog.IsActive()) { EndDialog(); return; }
	bMenuOpen = !bMenuOpen;
	if (bMenuOpen)
	{
		// Kursor: birinchi ochilmagan-bajarilmagan epizod
		const TArray<FErtEpisode>& All = UErtEpisodeDb::Get()->All();
		for (int32 i = 0; i < All.Num(); ++i) if (IsUnlocked(All[i]) && !IsCompleted(All[i].Id)) { MenuIndex = i; break; }
	}
	SetPlayerInput(!bMenuOpen, false);
}

void AErtGameMode::MenuMove(int32 Delta)
{
	if (Dialog.IsActive()) { Dialog.MoveSelection(Delta); return; }
	if (!bMenuOpen) return;
	const int32 N = UErtEpisodeDb::Get()->All().Num();
	if (N == 0) return;
	MenuIndex = (MenuIndex + Delta + N) % N;
}

void AErtGameMode::MenuConfirm()
{
	if (Dialog.IsActive()) { OnAdvance(); return; }
	if (!bMenuOpen) return;
	const TArray<FErtEpisode>& All = UErtEpisodeDb::Get()->All();
	if (!All.IsValidIndex(MenuIndex) || !IsUnlocked(All[MenuIndex])) return;
	bMenuOpen = false;
	BeginEpisode(All[MenuIndex].Id, true);
}

void AErtGameMode::BeginEpisode(const FString& Id, bool bWithCutscene)
{
	if (!Director) return;
	const FString EpId = Id;
	if (bWithCutscene && Cutscene)
	{
		Director->StopEpisode();
		SetPlayerInput(false, true);
		const FVector Origin = Director->GetAnchor(EpId);
		Cutscene->Play(EpId, Origin, [this, EpId]()
		{
			SetPlayerInput(true, false);
			if (Director) Director->StartEpisode(EpId);
		});
	}
	else
	{
		SetPlayerInput(true, false);
		Director->StartEpisode(EpId);
	}
}

void AErtGameMode::OnAdvance()
{
	if (Dialog.IsActive()) { Dialog.Advance(); if (!Dialog.IsActive()) EndDialog(); return; }
	if (Cutscene && Cutscene->IsPlaying()) Cutscene->Advance();
}

void AErtGameMode::OnSkip()
{
	if (Dialog.IsActive()) { EndDialog(); return; }
	if (Cutscene && Cutscene->IsPlaying()) { Cutscene->Skip(); return; }
	MenuToggle();
}
