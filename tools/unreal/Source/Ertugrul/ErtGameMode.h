// O'yin oqimi: epizod menyusi -> kat-sahna (intro) -> missiya -> keyingi epizod.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ErtGameMode.generated.h"

class AErtMissionDirector;
class AErtCutsceneDirector;
struct FErtEpisode;

UCLASS()
class ERTUGRUL_API AErtGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AErtGameMode();
	AErtMissionDirector* GetDirector() const { return Director; }
	AErtCutsceneDirector* GetCutscene() const { return Cutscene; }

	// Menyu
	bool IsMenuOpen() const { return bMenuOpen; }
	int32 GetMenuIndex() const { return MenuIndex; }
	void MenuToggle();
	void MenuMove(int32 Delta);
	void MenuConfirm();
	bool IsUnlocked(const FErtEpisode& E) const;
	bool IsCompleted(const FString& Id) const;

	// Kat-sahna boshqaruvi (Space/Enter = tezlashtirish, Esc = tashlab ketish)
	void OnAdvance();
	void OnSkip();

	/** Epizodni boshlaydi: kat-sahna (bo'lsa) -> missiya */
	void BeginEpisode(const FString& Id, bool bWithCutscene);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient) TObjectPtr<AErtMissionDirector> Director;
	UPROPERTY(Transient) TObjectPtr<AErtCutsceneDirector> Cutscene;
	bool bMenuOpen = false;
	bool bUnlockAll = false;
	int32 MenuIndex = 0;
	TArray<FString> Completed;
	void SetPlayerInput(bool bEnabled, bool bHide);
};
