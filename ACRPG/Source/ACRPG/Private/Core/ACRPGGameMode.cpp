#include "Core/ACRPGGameMode.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGPlayerCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AACRPGGameMode::AACRPGGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AACRPGGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Boshlang'ich checkpoint = xaritadagi birinchi PlayerStart.
	if (AActor* Start = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass()))
	{
		RespawnTransform = Start->GetActorTransform();
	}
}

void AACRPGGameMode::HandlePlayerDeath(AController* DeadController)
{
	if (!DeadController)
	{
		return;
	}

	UE_LOG(LogACRPG, Log, TEXT("O'yinchi o'ldi. %.1f soniyadan keyin respawn."), RespawnDelay);

	// Boshqaruvni uzamiz — o'lgan tanani harakatlantirib bo'lmasin.
	if (APawn* Pawn = DeadController->GetPawn())
	{
		Pawn->DisableInput(Cast<APlayerController>(DeadController));
	}

	// Timer lambda ichida DeadController'ni kuchsiz (weak) ushlaymiz:
	// agar u shu 3 soniyada yo'q bo'lsa, crash bo'lmaydi.
	TWeakObjectPtr<AController> WeakController(DeadController);
	FTimerDelegate Del;
	Del.BindLambda([this, WeakController]()
	{
		if (WeakController.IsValid())
		{
			RespawnPlayer(WeakController.Get());
		}
	});

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, Del, RespawnDelay, false);
}

void AACRPGGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	// Eski tanani yo'q qilamiz.
	if (APawn* OldPawn = Controller->GetPawn())
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	// Yangisini checkpoint'da yaratamiz.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = Controller;

	if (APawn* NewPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, RespawnTransform, Params))
	{
		Controller->Possess(NewPawn);
		NewPawn->EnableInput(Cast<APlayerController>(Controller));
		UE_LOG(LogACRPG, Log, TEXT("O'yinchi qayta tug'ildi."));
	}
	else
	{
		UE_LOG(LogACRPG, Error, TEXT("Respawn muvaffaqiyatsiz — DefaultPawnClass tayinlanmagan?"));
	}
}
