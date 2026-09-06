#include "ErtCutscene.h"
#include "Ertugrul.h"
#include "ErtHeroBody.h"
#include "ErtLoc.h"
#include "ErtAudio.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"

// ---------------- Aktyor ----------------

AErtCutActor::AErtCutActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = CreateDefaultSubobject<UErtHeroBody>(TEXT("Body"));
}

void AErtCutActor::Setup(const FString& ActorId, const FLinearColor& Tint, float Scale)
{
	// Har aktyor o'z rangida (id dan deterministik), Ertug'rul asl ranglarida
	if (ActorId != TEXT("ertugrul"))
	{
		FRandomStream RS(GetTypeHash(ActorId));
		const FLinearColor Pal[] = { FLinearColor(0.12f, 0.22f, 0.35f), FLinearColor(0.30f, 0.25f, 0.10f), FLinearColor(0.10f, 0.28f, 0.16f),
			FLinearColor(0.40f, 0.14f, 0.30f), FLinearColor(0.35f, 0.32f, 0.28f), FLinearColor(0.50f, 0.30f, 0.10f) };
		Body->Kaftan = Pal[RS.RandRange(0, 5)] * Tint;
		Body->Fur = FLinearColor(0.5f + RS.FRand() * 0.4f, 0.45f + RS.FRand() * 0.3f, 0.35f + RS.FRand() * 0.3f);
		Body->Beard = RS.FRand() < 0.3f ? FLinearColor(0.75f, 0.72f, 0.68f) : FLinearColor(0.15f, 0.1f, 0.06f);
	}
	static const TCHAR* Women[] = { TEXT("halime"), TEXT("hayme_ana"), TEXT("aykiz"), TEXT("selcan"), TEXT("gulbahor") };
	for (const TCHAR* W : Women) if (ActorId == W) Body->bWoman = true;
	Body->Build(RootComponent, 92.f);
	RootComponent->SetRelativeScale3D(FVector(Scale / 1.8f));
	RootComponent->AddLocalOffset(FVector(0, 0, 92.f * Scale / 1.8f));
}

void AErtCutActor::Pose(float Dt, float Speed, bool bTalk, bool bSit)
{
	if (!Body) return;
	Body->Animate(Dt, Speed, false, bSit, 0.f, 0.f);
	if (bTalk)
	{
		TalkT += Dt;
		if (FMath::Fmod(TalkT, 1.7f) < Dt) Body->TriggerHurt(); // engil imo-ishora (tana titrashi)
	}
}

// ---------------- Direktor ----------------

AErtCutsceneDirector::AErtCutsceneDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

static FVector JVec(const TArray<TSharedPtr<FJsonValue>>* A)
{
	if (!A || A->Num() < 3) return FVector::ZeroVector;
	return FVector((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber());
}

bool AErtCutsceneDirector::LoadScene(const FString& Path)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path)) return false;
	TSharedPtr<FJsonObject> R;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) || !R.IsValid()) return false;
	Scene = FErtCutScene();
	R->TryGetStringField(TEXT("id"), Scene.Id);
	R->TryGetStringField(TEXT("episode"), Scene.EpisodeId);
	R->TryGetBoolField(TEXT("letterbox"), Scene.bLetterbox);
	double D;
	if (R->TryGetNumberField(TEXT("fade_in"), D)) Scene.FadeIn = D;
	if (R->TryGetNumberField(TEXT("fade_out"), D)) Scene.FadeOut = D;
	if (R->TryGetNumberField(TEXT("duration"), D)) Scene.Duration = D;
	double IntroFly = 0; R->TryGetNumberField(TEXT("intro_fly"), IntroFly);
	const TArray<TSharedPtr<FJsonValue>>* Arr;
	if (R->TryGetArrayField(TEXT("actors"), Arr))
		for (const auto& V : *Arr)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O.IsValid()) continue;
			FErtCutActorDef A;
			O->TryGetStringField(TEXT("id"), A.Id);
			O->TryGetStringField(TEXT("loc_name"), A.LocName);
			if (O->TryGetNumberField(TEXT("scale"), D)) A.Scale = D;
			const TArray<TSharedPtr<FJsonValue>>* T;
			if (O->TryGetArrayField(TEXT("tint"), T) && T->Num() >= 3) A.Tint = FLinearColor((*T)[0]->AsNumber(), (*T)[1]->AsNumber(), (*T)[2]->AsNumber());
			const TArray<TSharedPtr<FJsonValue>>* K;
			if (O->TryGetArrayField(TEXT("keys"), K))
				for (const auto& KV : *K)
				{
					const TSharedPtr<FJsonObject> KO = KV->AsObject(); if (!KO.IsValid()) continue;
					FErtCutKey Key;
					if (KO->TryGetNumberField(TEXT("t"), D)) Key.T = D;
					if (KO->TryGetNumberField(TEXT("yaw"), D)) Key.Yaw = D;
					KO->TryGetStringField(TEXT("clip"), Key.Clip);
					const TArray<TSharedPtr<FJsonValue>>* P; if (KO->TryGetArrayField(TEXT("pos"), P)) Key.Pos = JVec(P);
					A.Keys.Add(Key);
				}
			Scene.Actors.Add(A);
		}
	if (R->TryGetArrayField(TEXT("camera"), Arr))
		for (const auto& V : *Arr)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O.IsValid()) continue;
			FErtCutCamKey C;
			if (O->TryGetNumberField(TEXT("t"), D)) C.T = D;
			if (O->TryGetNumberField(TEXT("fov"), D)) C.Fov = D;
			const TArray<TSharedPtr<FJsonValue>>* P;
			if (O->TryGetArrayField(TEXT("pos"), P)) C.Pos = JVec(P);
			if (O->TryGetArrayField(TEXT("look"), P)) C.Look = JVec(P);
			Scene.Camera.Add(C);
		}
	if (R->TryGetArrayField(TEXT("lines"), Arr))
		for (const auto& V : *Arr)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O.IsValid()) continue;
			FErtCutLine L;
			if (O->TryGetNumberField(TEXT("t"), D)) L.T = D;
			if (O->TryGetNumberField(TEXT("dur"), D)) L.Dur = D;
			O->TryGetStringField(TEXT("actor"), L.ActorId);
			O->TryGetStringField(TEXT("loc"), L.LocKey);
			O->TryGetStringField(TEXT("vo"), L.VoId);
			if (L.VoId.IsEmpty()) L.VoId = L.LocKey;
			if (L.Dur <= 0.f) L.Dur = 1.5f + 0.055f * FErtLoc::Get().Tr(L.LocKey).Len();
			Scene.Lines.Add(L);
		}
	if (Scene.Duration <= 0.f)
	{
		float End = 0.f;
		for (const auto& A : Scene.Actors) for (const auto& K : A.Keys) End = FMath::Max(End, K.T);
		for (const auto& C : Scene.Camera) End = FMath::Max(End, C.T);
		for (const auto& L : Scene.Lines) End = FMath::Max(End, L.T + L.Dur);
		Scene.Duration = End + 1.5f;
	}
	// Intro uchish: birinchi kamera kalitidan balanddan/uzoqdan tushib keladi (-ErtFly uslubi), hamma vaqtlar suriladi
	if (IntroFly > 0.0 && Scene.Camera.Num())
	{
		for (FErtCutCamKey& C : Scene.Camera) C.T += IntroFly;
		for (FErtCutActorDef& A : Scene.Actors) for (FErtCutKey& K : A.Keys) K.T += IntroFly;
		for (FErtCutLine& Ln : Scene.Lines) Ln.T += IntroFly;
		Scene.Duration += IntroFly;
		FErtCutCamKey F0 = Scene.Camera[0]; const FVector Dir = (F0.Pos - F0.Look).GetSafeNormal2D();
		F0.T = 0.f; F0.Pos = F0.Pos + Dir * 22.f + FVector(0, 26.f, 0); F0.Fov = FMath::Max(35.f, F0.Fov - 8.f);   // y-up dvijok koordinatasi: Y = balandlik
		FErtCutCamKey F1 = Scene.Camera[0]; F1.T = IntroFly * 0.55f; F1.Pos = Scene.Camera[0].Pos + Dir * 8.f + FVector(0, 9.f, 0);
		Scene.Camera.Insert(F1, 0); Scene.Camera.Insert(F0, 0);
	}
	return Scene.Camera.Num() > 0 || Scene.Actors.Num() > 0;
}

float AErtCutsceneDirector::GroundZ(float X, float Y) const
{
	FHitResult H;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtCutGround), true);
	for (AErtCutActor* A : Spawned) if (A) Q.AddIgnoredActor(A);
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) Q.AddIgnoredActor(P);
	if (GetWorld()->LineTraceSingleByChannel(H, FVector(X, Y, 60000.f), FVector(X, Y, -5000.f), ECC_Visibility, Q)) return H.ImpactPoint.Z;
	return Origin.Z;
}

FVector AErtCutsceneDirector::ToWorld(const FVector& E, bool bGround) const
{
	// Dvijok: x o'ng, y yuqori, z oldinga (metr). UE: X = z, Y = x, Z = y.
	const float X = Origin.X + E.Z * 100.f, Y = Origin.Y + E.X * 100.f;
	const float Base = bGround ? GroundZ(X, Y) : GroundZ(X, Y);
	return FVector(X, Y, Base + E.Y * 100.f);
}

bool AErtCutsceneDirector::Play(const FString& EpisodeId, const FVector& InOrigin, TFunction<void()> OnDone)
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/cutscenes") / (EpisodeId.ToLower() + TEXT("_intro.json"));
	Done = OnDone;
	Origin = InOrigin;
	if (!LoadScene(Path))
	{
		UE_LOG(LogErtugrul, Warning, TEXT("Kat-sahna topilmadi: %s"), *Path);
		Finish();
		return false;
	}
	// Aktyorlar
	for (const FErtCutActorDef& A : Scene.Actors)
	{
		if (A.Keys.Num() == 0) continue;
		AErtCutActor* Act = GetWorld()->SpawnActor<AErtCutActor>(AErtCutActor::StaticClass(), ToWorld(A.Keys[0].Pos, true) + FVector(0, 0, 92.f), FRotator(0, A.Keys[0].Yaw, 0));
		if (!Act) continue;
		Act->Setup(A.Id, A.Tint, A.Scale);
		Spawned.Add(Act);
		PrevPos.Add(Act->GetActorLocation());
	}
	// Kamera
	Cam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), ToWorld(Scene.Camera.Num() ? Scene.Camera[0].Pos : FVector(0, 3, 6), true), FRotator::ZeroRotator);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTargetWithBlend(Cam, 0.f);
	bPlaying = true; Time = 0.f; Fade = 1.f; Letterbox = 0.f; CurLine = -1; Subtitle.Reset(); Speaker.Reset();
	UE_LOG(LogErtugrul, Log, TEXT("Kat-sahna %s: %d aktyor, %d kamera kaliti, %d replika, %.1f s"), *Scene.Id, Scene.Actors.Num(), Scene.Camera.Num(), Scene.Lines.Num(), Scene.Duration);
	return true;
}

void AErtCutsceneDirector::Skip() { if (bPlaying) Finish(); }

void AErtCutsceneDirector::Advance()
{
	if (!bPlaying) return;
	// Keyingi replikaga / kamera kalitiga sakrash
	float Next = Scene.Duration;
	for (const FErtCutLine& L : Scene.Lines) if (L.T > Time + 0.2f) { Next = FMath::Min(Next, L.T); }
	Time = FMath::Max(Time, Next - 0.05f);
}

void AErtCutsceneDirector::Finish()
{
	bPlaying = false;
	FErtAudio::StopVo();
	for (AErtCutActor* A : Spawned) if (A) A->Destroy();
	Spawned.Reset(); PrevPos.Reset();
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		if (APawn* P = PC->GetPawn()) PC->SetViewTargetWithBlend(P, 0.6f);
	if (Cam) { Cam->SetLifeSpan(1.f); Cam = nullptr; }
	Subtitle.Reset(); Speaker.Reset(); Letterbox = 0.f; Fade = 0.f;
	if (Done) { TFunction<void()> F = MoveTemp(Done); Done = nullptr; F(); }
}

void AErtCutsceneDirector::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bPlaying) return;
	Time += Dt;
	const float Dur = Scene.Duration;
	Fade = FMath::Max(1.f - Time / FMath::Max(0.1f, Scene.FadeIn), Time > Dur - Scene.FadeOut ? (Time - (Dur - Scene.FadeOut)) / FMath::Max(0.1f, Scene.FadeOut) : 0.f);
	Letterbox = Scene.bLetterbox ? FMath::Min(1.f, Time / 0.8f) : 0.f;

	// Aktyorlar: kalitlar orasida chiziqli
	for (int32 i = 0; i < Scene.Actors.Num() && i < Spawned.Num(); ++i)
	{
		const TArray<FErtCutKey>& K = Scene.Actors[i].Keys;
		AErtCutActor* A = Spawned[i];
		if (!A || K.Num() == 0) continue;
		int32 j = 0;
		while (j + 1 < K.Num() && K[j + 1].T <= Time) ++j;
		FVector P; float Yaw; FString Clip = K[j].Clip;
		if (j + 1 < K.Num())
		{
			const float U = FMath::Clamp((Time - K[j].T) / FMath::Max(0.01f, K[j + 1].T - K[j].T), 0.f, 1.f);
			P = FMath::Lerp(K[j].Pos, K[j + 1].Pos, U);
			Yaw = FMath::Lerp(K[j].Yaw, K[j].Yaw + FMath::FindDeltaAngleDegrees(K[j].Yaw, K[j + 1].Yaw), U);
			Clip = K[j + 1].Clip == TEXT("Walk") || K[j].Clip == TEXT("Walk") ? TEXT("Walk") : K[j].Clip;
		}
		else { P = K[j].Pos; Yaw = K[j].Yaw; }
		const FVector W = ToWorld(P, true) + FVector(0, 0, 92.f);   // tana ildizi kapsula markazida
		const float Speed = FVector::Dist2D(W, PrevPos[i]) / FMath::Max(Dt, 0.001f);
		PrevPos[i] = W;
		A->SetActorLocation(W);
		// Yurganda harakat yo'nalishiga, aks holda kalit yaw ga qaraydi
		const FVector Vel = (W - PrevPos[i]);
		A->SetActorRotation(FRotator(0, Yaw, 0));
		const bool bTalk = Clip == TEXT("Talk");
		const bool bSit = Clip == TEXT("Sit") || Clip == TEXT("Kneel") || Clip == TEXT("Crouch");
		A->Pose(Dt, FMath::Min(Speed, 400.f), bTalk, bSit);
	}
	// Kamera
	if (Cam && Scene.Camera.Num())
	{
		const TArray<FErtCutCamKey>& C = Scene.Camera;
		int32 j = 0;
		while (j + 1 < C.Num() && C[j + 1].T <= Time) ++j;
		FVector P = C[j].Pos, L = C[j].Look; float Fov = C[j].Fov;
		if (j + 1 < C.Num())
		{
			float U = FMath::Clamp((Time - C[j].T) / FMath::Max(0.01f, C[j + 1].T - C[j].T), 0.f, 1.f);
			U = U * U * (3.f - 2.f * U);
			// Silliq kamera: Catmull-Rom (pozitsiya va qarash nuqtasi), FOV smoothstep
			auto At = [&](int32 k) -> const FErtCutCamKey& { return C[FMath::Clamp(k, 0, C.Num() - 1)]; };
			P = FMath::CubicCRSplineInterp(At(j - 1).Pos, At(j).Pos, At(j + 1).Pos, At(j + 2).Pos, 0.f, 1.f, 2.f, 3.f, 1.f + U);
			L = FMath::CubicCRSplineInterp(At(j - 1).Look, At(j).Look, At(j + 1).Look, At(j + 2).Look, 0.f, 1.f, 2.f, 3.f, 1.f + U);
			Fov = FMath::Lerp(Fov, C[j + 1].Fov, FMath::SmoothStep(0.f, 1.f, U));
		}
		const FVector WP = ToWorld(P, true), WL = ToWorld(L, true);
		Cam->SetActorLocation(WP);
		Cam->SetActorRotation((WL - WP).Rotation());
		Cam->GetCameraComponent()->SetFieldOfView(Fov);
	}
	// Replikalar
	if (CurLine >= 0 && Time > LineEnd) { CurLine = -1; Subtitle.Reset(); Speaker.Reset(); }
	for (int32 i = 0; i < Scene.Lines.Num(); ++i)
	{
		const FErtCutLine& L = Scene.Lines[i];
		if (i != CurLine && Time >= L.T && Time < L.T + L.Dur)
		{
			CurLine = i; LineEnd = L.T + L.Dur;
			Subtitle = FErtLoc::Get().Tr(L.LocKey);
			FErtAudio::PlayVo(GetWorld(), L.VoId, 1.f);
			Speaker.Reset();
			for (const FErtCutActorDef& A : Scene.Actors) if (A.Id == L.ActorId) Speaker = FErtLoc::Get().TrOr(A.LocName, A.Id);
		}
	}
	if (Time >= Dur) Finish();
}
