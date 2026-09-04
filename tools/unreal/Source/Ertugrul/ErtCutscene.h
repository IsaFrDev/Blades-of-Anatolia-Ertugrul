// Ma'lumotga asoslangan kat-sahna: Content/Ertugrul/Data/cutscenes/<epizod>_intro.json
// Aktyorlar (protsedural figuralar) kalitlar bo'ylab yuradi, kamera kalitlar orasida silliq o'tadi,
// subtitr + gapiruvchi nomi, letterbox va fade. Space/Enter - replikani tezlashtirish, Esc - tashlab ketish.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtCutscene.generated.h"

class UErtHeroBody;
class ACameraActor;

struct FErtCutKey { float T = 0.f; FVector Pos = FVector::ZeroVector; float Yaw = 0.f; FString Clip = TEXT("Idle"); };
struct FErtCutActorDef { FString Id, LocName; float Scale = 1.8f; FLinearColor Tint = FLinearColor::White; TArray<FErtCutKey> Keys; };
struct FErtCutCamKey { float T = 0.f; FVector Pos = FVector::ZeroVector; FVector Look = FVector::ZeroVector; float Fov = 45.f; };
struct FErtCutLine { float T = 0.f, Dur = 0.f; FString ActorId, LocKey; };
struct FErtCutScene
{
	FString Id, EpisodeId;
	bool bLetterbox = true;
	float FadeIn = 1.f, FadeOut = 1.f, Duration = 0.f;
	TArray<FErtCutActorDef> Actors;
	TArray<FErtCutCamKey> Camera;
	TArray<FErtCutLine> Lines;
};

/** Kat-sahna aktyori: kapsulasiz, faqat tana */
UCLASS()
class ERTUGRUL_API AErtCutActor : public AActor
{
	GENERATED_BODY()
public:
	AErtCutActor();
	void Setup(const FString& ActorId, const FLinearColor& Tint, float Scale);
	void Pose(float Dt, float Speed, bool bTalk, bool bSit);
private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UErtHeroBody> Body;
	float TalkT = 0.f;
};

UCLASS()
class ERTUGRUL_API AErtCutsceneDirector : public AActor
{
	GENERATED_BODY()
public:
	AErtCutsceneDirector();

	/** Epizod uchun sahnani yuklab o'ynatadi. Origin - sahna markazi (dunyo, sm). */
	bool Play(const FString& EpisodeId, const FVector& Origin, TFunction<void()> OnDone);
	void Skip();
	void Advance();
	bool IsPlaying() const { return bPlaying; }

	// HUD
	const FString& GetSubtitle() const { return Subtitle; }
	const FString& GetSpeaker() const { return Speaker; }
	float GetLetterbox() const { return Letterbox; }
	float GetFade() const { return Fade; }
	float GetTime() const { return Time; }
	float GetDuration() const { return Scene.Duration; }

protected:
	virtual void Tick(float Dt) override;

private:
	bool LoadScene(const FString& Path);
	void Finish();
	FVector ToWorld(const FVector& EnginePos, bool bGround) const;   // dvijok (x, y-up, z) metr -> UE sm
	float GroundZ(float X, float Y) const;

	FErtCutScene Scene;
	UPROPERTY(Transient) TArray<TObjectPtr<AErtCutActor>> Spawned;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> Cam;
	TArray<FVector> PrevPos;
	FVector Origin = FVector::ZeroVector;
	TFunction<void()> Done;
	bool bPlaying = false;
	float Time = 0.f, Letterbox = 0.f, Fade = 1.f;
	FString Subtitle, Speaker;
	int32 CurLine = -1;
	float LineEnd = 0.f;
};
