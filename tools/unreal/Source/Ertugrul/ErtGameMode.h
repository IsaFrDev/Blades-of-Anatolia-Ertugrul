#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ErtGameMode.generated.h"

class AErtMissionDirector;

UCLASS()
class ERTUGRUL_API AErtGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AErtGameMode();
	AErtMissionDirector* GetDirector() const { return Director; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient) TObjectPtr<AErtMissionDirector> Director;
};
