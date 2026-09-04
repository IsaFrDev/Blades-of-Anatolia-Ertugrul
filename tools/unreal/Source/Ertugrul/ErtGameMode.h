// O'yin oqimi: epizod menyusi -> kat-sahna (intro) -> missiya -> keyingi epizod.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ErtDialog.h"
#include "ErtGameMode.generated.h"

class AErtMissionDirector;
class AErtCutsceneDirector;
class AErtNpc;

UENUM()
enum class EErtMenu : uint8 { None, Main, Pause, Episodes, Settings, Map, Inventory };
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
	bool IsMenuOpen() const { return Menu == EErtMenu::Episodes; }
	EErtMenu GetMenu() const { return Menu; }
	int32 GetMenuRow() const { return Menu == EErtMenu::Main ? RowMain : RowPause; }
	bool IsAnyMenu() const { return Menu != EErtMenu::None; }
	void OpenMenu(EErtMenu M);
	void ToggleMap();
	void ToggleInventory();
	bool HasEpisodeStarted() const { return bEpisodeStarted; }
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
	FString HonorTitle() const { return Honor >= 20 ? TEXT("Sharafli Bey") : (Honor >= 10 ? TEXT("Mo'tabar") : (Honor <= -10 ? TEXT("Badnom") : (Honor <= -5 ? TEXT("Shubhali") : TEXT("Oddiy")))); }
	const TSet<FString>& GetFlags() const { return Flags; }
	void AddFlag(const FString& F) { Flags.Add(F); }
	void RemoveFlag(const FString& F) { Flags.Remove(F); }
	void AddHonor(int32 N) { Honor += N; }
	void RefreshSideQuestFlags();
	bool HasFlag(const FString& F) const { return Flags.Contains(F); }
	/** Oxirgi tugagan dialog (missiya maqsadlari uchun) */
	FString ShopMsg; float ShopMsgT = 0.f;
	FString LastDialogId; int32 LastDuelPoints = 0, LastDuelThreshold = 0; float LastDialogEndTime = -1.f;

	/** Epizodni boshlaydi: kat-sahna (bo'lsa) -> missiya */
	void BeginEpisode(const FString& Id, bool bWithCutscene);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient) TObjectPtr<AErtMissionDirector> Director;
	UPROPERTY(Transient) TObjectPtr<AErtCutsceneDirector> Cutscene;
	EErtMenu Menu = EErtMenu::None, Prev = EErtMenu::None;
	int32 RowMain = 0, RowPause = 0;
	bool bEpisodeStarted = false;
	bool bUnlockAll = false;
	int32 MenuIndex = 0;
	TArray<FString> Completed;
	UPROPERTY(Transient) TObjectPtr<AErtNpc> TalkingNpc;
	UPROPERTY(Transient) TObjectPtr<class AErtWeather> Weather;
	FErtDialog Dialog;
	TSet<FString> Flags;
	int32 Honor = 0;
	void SpawnNpcs();
	UPROPERTY(Transient) TObjectPtr<class ADirectionalLight> Sun;
	UPROPERTY(Transient) TObjectPtr<class ASkyLight> Sky;
	UPROPERTY(Transient) TObjectPtr<class APostProcessVolume> PPV;
	float DayT = 0.35f;      // 0 = yarim tun, 0.25 = tong, 0.5 = peshin, 0.75 = shom
	float DayLength = 1200.f; // soniya (20 daqiqa)
	virtual void Tick(float Dt) override;
public:
	void SetTimeOfDay(const FString& Name);
	void SetWeather(const FString& Name);
	void Rumble(float Intensity, float Seconds);
	float GetDayT() const { return DayT; }
	void SaveGame();
	void LoadGame();
	// Sozlamalar
	bool IsSettingsOpen() const { return Menu == EErtMenu::Settings; }
	void HitStop(float Seconds, float Dilation = 0.12f);
	int32 GetSettingsRow() const { return SettingsRow; }
	void SettingsToggle();
	void SettingsMove(int32 Delta);
	void SettingsAdjust(int32 Delta);
	int32 Language = 0; float MouseSens = 1.f; bool bInvertY = false;
private:
	int32 SettingsRow = 0;
	void SetPlayerInput(bool bEnabled, bool bHide);
};
