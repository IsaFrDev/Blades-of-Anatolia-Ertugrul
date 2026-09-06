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
	void RefreshCraftFlags();
	FString DialogSpeaker() const { return Dialog.IsActive() ? Dialog.Speaker() : FString(); }
	bool StartDialogId(const FString& Id); // NPCsiz dialog (tush jumboqi)
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
public:
	int32 TribeScore = 0;   // qabila boshqaruvi: hadyalar -> daraja 1..3 (ittifoqchi +, narx -5%/daraja)
	int32 TribeLevel() const { return TribeScore >= 120 ? 3 : (TribeScore >= 50 ? 2 : 1); }
private:
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
	int32 SettingsPage = 0, KeyRow = 0; bool bCapturing = false;   // 0 umumiy, 1 tugmalar
	TMap<FString, FString> SavedKeys;
	void ApplySavedKeys();
	int32 ResIndex = 1;   // 0 1280x720, 1 1600x900, 2 1920x1080, 3 2560x1440
	void ApplyDisplay();
	// Oba faoliyatlari: 0 yo'q, 1 kamon musobaqasi, 2 kurash
	int32 Activity = 0; float ActivityT = 0.f; int32 ActScore = 0, ActGoal = 5; float WrestleP = 0.5f; FString ActResult; float ActResultT = 0.f;
	void StartActivity(int32 Kind);
	void ActivityScore(int32 N) { if (Activity == 1) ActScore += N; }
	void WrestlePress();
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> ActTargets;
	void EndActivity(bool bWon);
	FString DisplayRow(int32 Row) const;
	void SettingsMove(int32 Delta);
	void SettingsAdjust(int32 Delta);
	int32 Language = 0; float MouseSens = 1.f; bool bInvertY = false;
	/** Grafika preseti: 0 PC Ultra (Lumen HW, VSM High, TSR 100%), 1 PS5/XSX (Lumen SW, VSM Medium, TSR 50-70%), 2 Series S (SSGI, CSM, SSR, TSR 50-60%) */
	int32 GfxPreset = 1;
	void ApplyGfxPreset();
	static const TCHAR* GfxPresetName(int32 P) { return P == 0 ? TEXT("PC Ultra") : (P == 1 ? TEXT("PS5 / Xbox Series X (60 FPS)") : TEXT("Xbox Series S (60 FPS)")); }
	/** 3D xarita aylantirish (chap/o'ng) */
	void MapRotate(float DeltaYaw);
	bool bGps = true;
	/** Tush (Ulukayin mifi) rejimi: binafsha tuslash, tuman, tun */
	bool bDream = false; float SavedDayT = 0.31f; FString SavedWeather;
	void SetDream(bool bOn);
	const FString& GetWeatherName() const;
	// AC/GTA uslubidagi xarita: yo'l nuqtasi, kashf qilingan hududlar, sichqoncha boshqaruvi
	FVector Waypoint = FVector::ZeroVector; bool bWaypoint = false;
	void SetWaypoint(const FVector& W) { Waypoint = W; bWaypoint = true; }
	void ClearWaypoint() { bWaypoint = false; }
	static constexpr int32 VisN = 40;   // 50 m hujayralar (2000 m)
	TArray<uint8> Visited;
	bool IsVisited(float E, float N) const { const int32 X = FMath::Clamp((int32)((E + 1000.f) / 50.f), 0, VisN - 1), Y = FMath::Clamp((int32)((N + 1000.f) / 50.f), 0, VisN - 1); return Visited.IsValidIndex(Y * VisN + X) && Visited[Y * VisN + X] != 0; }
	float VisitT = 0.f;
	/** Kashfiyot teksturasi (40x40, oq = kashf qilinmagan) - HUD xaritada affin proyeksiya bilan chiziladi */
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> FogTex;
	bool bFogDirty = true;
	void UpdateFogTex();
	/** HUD xarita kvadrati (X0, Y0, S) - sichqoncha koordinatalari uchun */
	FVector MapRect = FVector::ZeroVector;
	FVector2D MapMouse = FVector2D(-1, -1);   // xarita ichidagi [0..1] sichqoncha, tashqarida -1
	FVector2D LastMouse = FVector2D::ZeroVector; bool bDragging = false;
	void MapInput(float Dt);
private:
	int32 SettingsRow = 0;
	void SetPlayerInput(bool bEnabled, bool bHide);
};
