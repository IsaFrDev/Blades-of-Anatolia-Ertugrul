#include "ErtGameMode.h"
#include "Ertugrul.h"
#include "ErtCharacter.h"
#include "ErtCutscene.h"
#include "ErtEpisodeDb.h"
#include "ErtHUD.h"
#include "ErtHorse.h"
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

void AErtGameMode::MenuToggle()
{
	if (Cutscene && Cutscene->IsPlaying()) return;
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
	if (!bMenuOpen) return;
	const int32 N = UErtEpisodeDb::Get()->All().Num();
	if (N == 0) return;
	MenuIndex = (MenuIndex + Delta + N) % N;
}

void AErtGameMode::MenuConfirm()
{
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

void AErtGameMode::OnAdvance() { if (Cutscene && Cutscene->IsPlaying()) Cutscene->Advance(); }

void AErtGameMode::OnSkip()
{
	if (Cutscene && Cutscene->IsPlaying()) { Cutscene->Skip(); return; }
	MenuToggle();
}
