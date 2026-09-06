// Missiya direktori: epizod arxetipidan bosqichlar zanjirini quradi
// (Yo'l / Ov / Yashirin / Himoya / Qidiruv / Yakkama-yakka / Jang), maqsadlarni kuzatadi,
// to'lqinlarni chiqaradi, nazorat nuqtalarini saqlaydi, o'lim -> qayta boshlash, epizod -> keyingisi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtEnemy.h"
#include "ErtMission.generated.h"

class AErtCharacter;
class AErtHorse;
class AErtNpc;
class AErtWorldBuilder;
class UProceduralMeshComponent;
struct FErtEpisode;

UENUM(BlueprintType)
enum class EErtObjKind : uint8 { DefeatAll, SurviveTime, StayUndetected, Route, Hunt, Collect, HoldPoint, Timer, Council };

UENUM(BlueprintType)
enum class EErtMissionState : uint8 { Inactive, Briefing, Fighting, WaveCleared, Cleared, Failed };

USTRUCT(BlueprintType)
struct FErtObjective
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) EErtObjKind Kind = EErtObjKind::DefeatAll;
	UPROPERTY(BlueprintReadOnly) FString LocKey;
	UPROPERTY(BlueprintReadOnly) int32 Target = 0;
	UPROPERTY(BlueprintReadOnly) int32 Progress = 0;
	UPROPERTY(BlueprintReadOnly) bool bDone = false;
	UPROPERTY(BlueprintReadOnly) bool bFailed = false;
	UPROPERTY(BlueprintReadOnly) bool bOptional = false;
	UPROPERTY(BlueprintReadOnly) FVector Point = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) float Radius = 400.f;
	UPROPERTY(BlueprintReadOnly) TArray<FVector> Points;
	UPROPERTY(BlueprintReadOnly) TArray<bool> Collected;
	UPROPERTY(BlueprintReadOnly) bool bNeedZ = false;
	UPROPERTY(BlueprintReadOnly) TArray<FString> PointFlags;   // Collect: har nuqta uchun dalil bayrog'i
	UPROPERTY(BlueprintReadOnly) FString DialogId;              // Council: dialog grafi
	UPROPERTY(BlueprintReadOnly) int32 Threshold = 0;           // Council: duel ballari chegarasi
	float Hold = 0.f;
	FString Text() const;
};

struct FErtSpawn { EErtEnemyKind Kind = EErtEnemyKind::Footman; FVector Pos = FVector::ZeroVector; float Yaw = 0.f; float Patrol = 0.f; };
struct FErtWave { TArray<FErtSpawn> Spawns; float Delay = 0.f; };
struct FErtCouncilNpc { FString Id, NameKey; float U = 0, V = 0, Yaw = 0; bool bWoman = false; FLinearColor Kaftan = FLinearColor(0.3f, 0.2f, 0.1f); };
struct FErtPhase { FString TitleKey; TArray<FErtObjective> Objectives; TArray<FErtWave> Waves; FErtWave Reinforce; bool bReinforced = false; TArray<FErtCouncilNpc> Npcs; bool bDream = false; FVector DreamSpot = FVector::ZeroVector; };

UCLASS()
class ERTUGRUL_API AErtMissionDirector : public AActor
{
	GENERATED_BODY()
public:
	AErtMissionDirector();

	UFUNCTION(BlueprintCallable, Category = "Ertugrul") bool StartEpisode(const FString& EpisodeId);
	UFUNCTION(BlueprintCallable, Category = "Ertugrul") void RestartFromCheckpoint();
	UFUNCTION(BlueprintCallable, Category = "Ertugrul") void StopEpisode();
	/** Epizod boshlanadigan joy (dunyo, sm) - kat-sahna sahnasi uchun ham */
	FVector GetAnchor(const FString& InEpisodeId) const;
	/** Yon kvest (sidequests.json): o'yinchi turgan joydan boshlanadi */
	bool StartSideQuest(const FString& QuestId);
	bool IsSideQuest() const { return bSideQuest; }
	struct FSideInfo { FString Id, Giver, TitleKey, DoneKey, Reward; int32 XP = 0, Gold = 0, Honor = 0; };
	static TArray<FSideInfo> LoadSideQuests();

	// HUD uchun
	EErtMissionState GetState() const { return State; }
	float GetStateTime() const { return StateT; }
	const TArray<FErtObjective>& GetObjectives() const { return Objectives; }
	int32 GetPhaseIndex() const { return PhaseIdx; }
	int32 GetPhaseCount() const { return Phases.Num(); }
	const FString& GetPhaseTitle() const { return PhaseTitle; }
	const FString& GetEpisodeId() const { return EpisodeId; }
	const FString& GetEpisodeTitle() const { return EpisodeTitle; }
	const FString& GetEpisodeDate() const { return EpisodeDate; }
	const FString& GetIntroText() const { return IntroText; }
	const FString& GetCliffhanger() const { return Cliffhanger; }
	TArray<FString> LoadProgressPublic() const { return LoadProgress(); }
	/** Cliffhanger ekranini o'tkazib yuborish (Enter/Space) */
	void SkipCleared() { if (State == EErtMissionState::Cleared) StateT = 99.f; }
	/** Sinov: joriy bosqichning barcha maqsadlarini bajarilgan deb belgilash */
	void DebugCompletePhase() { for (FErtObjective& O : Objectives) if (!O.bOptional) { O.bDone = true; O.bFailed = false; O.Progress = FMath::Max(O.Progress, O.Target); } }
	int32 GetKills() const { return Kills; }
	int32 GetDeaths() const { return Deaths; }
	int32 AliveEnemies() const;
	int32 GetWaveIndex() const { return WaveIdx; }
	int32 GetWaveCount() const { return Waves.Num(); }
	float GetCheckpointFlash() const { return CpFlash; }
	const FString& GetCheckpointName() const { return CpName; }
	/** Faol maqsad nuqtalari (HUD markerlari): nuqta + tugallanganmi */
	void GetMarkers(TArray<FVector>& Out) const;
	const TArray<TObjectPtr<AErtEnemy>>& GetEnemies() const { return Enemies; }
	const TArray<TObjectPtr<AErtEnemy>>& GetAllies() const { return Allies; }
	int32 AliveAllies() const;
	/** Urush: o'yinchi atrofida N ittifoqchi alp (bir bosqichda bir marta) */
	void SpawnAllies(int32 N);
	const FString& GetCouncilResult() const { return CouncilResult; }
	float GetCouncilResultT() const { return CouncilResultT; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(Transient) TArray<TObjectPtr<AErtEnemy>> Enemies;
	UPROPERTY(Transient) TArray<TObjectPtr<AErtEnemy>> Allies;
	void ClearAllies();
	UPROPERTY(Transient) TObjectPtr<AErtHorse> Horse;
	UPROPERTY(Transient) TArray<TObjectPtr<AErtNpc>> PhaseNpcs;
	FString CouncilResult;   // HUD: duel natijasi
	float CouncilResultT = 0.f;
	bool ApplyOverride(const FErtEpisode& E);
	void ClearPhaseNpcs();
	UPROPERTY(Transient) TObjectPtr<AErtWorldBuilder> World;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> MarkerMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> MarkerMat;

	TArray<FErtPhase> Phases;
	TArray<FErtObjective> Objectives;
	TArray<FErtWave> Waves;
	int32 PhaseIdx = 0, WaveIdx = 0;
	float PhaseT = 0.f, StateT = 0.f, CpFlash = 0.f;
	EErtMissionState State = EErtMissionState::Inactive;
	FString EpisodeId, EpisodeTitle, EpisodeDate, IntroText, PhaseTitle, NextEpisodeId, CpName, Cliffhanger, EpisodeArchetype;
	int32 Kills = 0, Deaths = 0;
	bool bSideQuest = false; FSideInfo Side;
	bool StartEpisodeData(const FErtEpisode& E, const FVector& Start, const TSharedPtr<class FJsonObject>& PhaseOverride);
	TSharedPtr<class FJsonObject> PhaseOverrideObj;
	FRandomStream Rng;
	FVector Cursor = FVector::ZeroVector;
	FVector2D CouncilBase = FVector2D::ZeroVector;
	bool bDreamRiddleDone = false; int32 GlobalIdx = 0;
	FVector DreamReturn = FVector::ZeroVector; float DreamReturnYaw = 0.f;   // kengash NPClari bazasi (reja E, N): oba yoki epizod boshlanishi

	struct FCheckpoint { bool bValid = false; int32 Phase = 0; FVector Pos; float Yaw = 0.f; } Cp;

	AErtCharacter* Hero() const;
	FVector GroundAt(float X, float Y, bool* bOnProp = nullptr) const;
	FVector FindSpot(const FVector& Around, float MinR, float MaxR) const;
	FVector FindElevated(const FVector& Around, float MinR, float MaxR, bool& bElevated) const;
	FVector AnchorFor(const FErtEpisode& E) const;
	void BuildPhases(const FErtEpisode& E);
	void StartPhase(int32 Idx);
	void SpawnWave(int32 Idx);
	void ClearEnemies();
	void SaveCheckpoint();
	void RebuildMarkers();
	void UpdateObjectives(float Dt);
	void SaveProgress();
	TArray<FString> LoadProgress() const;
};
