#include "ErtGameMode.h"
#include "Ertugrul.h"
#include "ErtCharacter.h"
#include "ErtHUD.h"
#include "ErtMission.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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
	FString Ep = TEXT("EP001");
	FParse::Value(FCommandLine::Get(), TEXT("-ErtEpisode="), Ep);
	bool bNoMission = FParse::Param(FCommandLine::Get(), TEXT("ErtFreeRoam"));
	if (bNoMission || !Director) return;
	// Dunyo va o'yinchi tayyor bo'lishi uchun bir oz kutamiz
	FTimerHandle Th;
	GetWorldTimerManager().SetTimer(Th, [this, Ep]() { if (Director) Director->StartEpisode(Ep); }, 0.6f, false);
}
