// O'yin oqimi: epizod menyusi -> kat-sahna (intro) -> missiya -> keyingi epizod.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ErtDialog.h"
#include "ErtGameMode.generated.h"

class AErtMissionDirector;
class AErtCutsceneDirector;
class AErtNpc;
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

	// Dialog
	void StartDialog(AErtNpc* Npc);
	void DialogChoose(int32 Index);
	void EndDialog();
	const FErtDialog& GetDialog() const { return Dialog; }
	bool IsDialogActive() const { return Dialog.IsActive(); }
	int32 GetHonor() const { return Honor; }
	const TSet<FString>& GetFlags() const { return Flags; }

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
	FErtDialog Dialog;
	TSet<FString> Flags;
	int32 Honor = 0;
	void SpawnNpcs();
	void SetPlayerInput(bool bEnabled, bool bHide);
};
