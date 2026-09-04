#include "ErtHUD.h"
#include "ErtCharacter.h"
#include "ErtEnemy.h"
#include "ErtLoc.h"
#include "ErtMission.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

AErtMissionDirector* AErtHUD::Director() const
{
	return Cast<AErtMissionDirector>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtMissionDirector::StaticClass()));
}

void AErtHUD::Text(const FString& S, float X, float Y, const FLinearColor& C, float Scale, bool bShadow, bool bLarge)
{
	UFont* F = bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
	if (bShadow) { FCanvasTextItem Sh(FVector2D(X + 1.5f, Y + 1.5f), FText::FromString(S), F, FLinearColor(0, 0, 0, 0.8f)); Sh.Scale = FVector2D(Scale, Scale); Canvas->DrawItem(Sh); }
	FCanvasTextItem It(FVector2D(X, Y), FText::FromString(S), F, C);
	It.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(It);
}

float AErtHUD::TextWidth(const FString& S, float Scale, bool bLarge) const
{
	float W = 0, H = 0;
	Canvas->StrLen(bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont(), S, W, H);
	return W * Scale;
}

void AErtHUD::Bar(float X, float Y, float W, float H, float Frac, const FLinearColor& Fill)
{
	FCanvasTileItem Bg(FVector2D(X, Y), FVector2D(W, H), FLinearColor(0, 0, 0, 0.55f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	FCanvasTileItem Fg(FVector2D(X + 2, Y + 2), FVector2D(FMath::Max(0.f, (W - 4) * FMath::Clamp(Frac, 0.f, 1.f)), H - 4), Fill); Fg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Fg);
}

void AErtHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	const float SW = Canvas->SizeX, SH = Canvas->SizeY;
	const float Sc = FMath::Clamp(SH / 720.f, 0.8f, 2.f);
	AErtCharacter* H = Cast<AErtCharacter>(GetOwningPawn());
	AErtMissionDirector* D = Director();
	const FErtLoc& L = FErtLoc::Get();
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.6f, 0.6f, 0.55f), Green(0.5f, 0.9f, 0.4f), Red(0.9f, 0.25f, 0.2f);

	// --- pastki chap: sog'liq, stamina, o'q ---
	if (H)
	{
		const float X = 24 * Sc, Y = SH - 78 * Sc;
		Bar(X, Y, 260 * Sc, 16 * Sc, H->GetHealth() / H->GetMaxHealth(), H->GetHealth() > 30 ? FLinearColor(0.75f, 0.15f, 0.12f) : Red);
		Bar(X, Y + 20 * Sc, 200 * Sc, 10 * Sc, H->GetStamina() / 100.f, FLinearColor(0.85f, 0.7f, 0.25f));
		Text(FString::Printf(TEXT("%s %d   %s %d"), *L.Tr(TEXT("ui.hud.health")), (int32)H->GetHealth(), *L.Tr(TEXT("ui.hud.arrows")), H->GetArrows()), X, Y + 34 * Sc, White, Sc);
		if (H->GetHurtFlash() > 0.f)
		{
			FCanvasTileItem V(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.6f, 0.f, 0.f, 0.35f * H->GetHurtFlash())); V.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(V);
		}
	}
	if (!D || D->GetState() == EErtMissionState::Inactive)
	{
		Text(L.Tr(TEXT("ui.hud.free_roam")), 24 * Sc, 24 * Sc, Grey, Sc);
		return;
	}

	// --- yuqori chap: epizod, bosqich, maqsadlar ---
	float Y = 22 * Sc;
	Text(FString::Printf(TEXT("%s  %s"), *D->GetEpisodeId(), *D->GetEpisodeTitle()), 24 * Sc, Y, Gold, Sc, true, true); Y += 28 * Sc;
	Text(FString::Printf(TEXT("%s %d/%d: %s"), *L.Tr(TEXT("ui.hud.phase")), D->GetPhaseIndex() + 1, D->GetPhaseCount(), *D->GetPhaseTitle()), 24 * Sc, Y, White, Sc); Y += 22 * Sc;
	for (const FErtObjective& O : D->GetObjectives())
	{
		const FLinearColor C = O.bDone ? Green : (O.bFailed ? Red : (O.bOptional ? Grey : White));
		Text(FString::Printf(TEXT("%s %s"), O.bDone ? TEXT("[x]") : TEXT("[ ]"), *O.Text()), 36 * Sc, Y, C, Sc); Y += 19 * Sc;
	}
	if (D->GetWaveCount() > 0)
	{
		Text(FString::Printf(TEXT("%s %d/%d   %s %d"), *L.Tr(TEXT("ui.hud.wave")), D->GetWaveIndex() + 1, D->GetWaveCount(), *L.Tr(TEXT("ui.hud.enemies")), D->AliveEnemies()), 36 * Sc, Y + 4 * Sc, Grey, Sc);
	}

	// --- markerlar (dunyo -> ekran) ---
	TArray<FVector> Pts; D->GetMarkers(Pts);
	if (H)
	{
		for (const FVector& P : Pts)
		{
			const FVector S = Project(P + FVector(0, 0, 250.f));
			if (S.Z <= 0.f) continue;
			const float Dist = FVector::Dist(P, H->GetActorLocation()) / 100.f;
			const float Sz = 9 * Sc;
			FCanvasTileItem T(FVector2D(S.X - Sz, S.Y - Sz), FVector2D(Sz * 2, Sz * 2), Gold); T.Rotation = FRotator(0, 45.f, 0); T.PivotPoint = FVector2D(0.5f, 0.5f); T.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(T);
			const FString DS = FString::Printf(TEXT("%.0f m"), Dist);
			Text(DS, S.X - TextWidth(DS, Sc, false) * 0.5f, S.Y + Sz + 2, Gold, Sc);
		}
		// Dushman ko'rsatkichlari (faol, yaqin)
		for (const AErtEnemy* E : D->GetEnemies())
		{
			if (!E || E->IsDead() || E->IsAnimal() || !E->IsAlerted()) continue;
			const FVector S = Project(E->GetActorLocation() + FVector(0, 0, 130.f));
			if (S.Z <= 0.f) continue;
			Bar(S.X - 20 * Sc, S.Y, 40 * Sc, 5 * Sc, E->GetHealth() / E->GetMaxHealth(), Red);
		}
	}

	// --- markaziy xabarlar ---
	auto Center = [&](const FString& S, float YY, const FLinearColor& C, float Scale)
	{
		Text(S, (SW - TextWidth(S, Scale, true)) * 0.5f, YY, C, Scale, true, true);
	};
	const float T = D->GetStateTime();
	switch (D->GetState())
	{
	case EErtMissionState::Briefing:
		Center(D->GetEpisodeTitle(), SH * 0.30f, Gold, 1.6f * Sc);
		Center(D->GetEpisodeDate(), SH * 0.30f + 40 * Sc, White, Sc);
		if (D->GetPhaseIndex() == 0 && !D->GetIntroText().IsEmpty()) Center(D->GetIntroText().Left(120), SH * 0.30f + 70 * Sc, Grey, 0.9f * Sc);
		break;
	case EErtMissionState::WaveCleared:
		Center(L.Tr(TEXT("ui.hud.wave")) + TEXT(" +1"), SH * 0.28f, Gold, 1.3f * Sc);
		break;
	case EErtMissionState::Failed:
		Center(L.TrOr(TEXT("ui.hud.dead"), TEXT("HALOK BO'LDINGIZ")), SH * 0.35f, Red, 1.8f * Sc);
		Center(L.Tr(TEXT("ui.hud.checkpoint")) + TEXT(": ") + D->GetCheckpointName(), SH * 0.35f + 46 * Sc, White, Sc);
		break;
	case EErtMissionState::Cleared:
		Center(L.Tr(TEXT("ui.episodes.completed")), SH * 0.30f, Gold, 1.7f * Sc);
		Center(FString::Printf(TEXT("%d  |  %d"), D->GetKills(), D->GetDeaths()), SH * 0.30f + 44 * Sc, White, Sc);
		if (T > 2.f) Center(L.Tr(TEXT("ui.episodes.next")), SH * 0.30f + 70 * Sc, Grey, Sc);
		break;
	default:
		if (D->GetCheckpointFlash() > 0.f && T > 0.2f)
			Center(L.Tr(TEXT("ui.hud.checkpoint")), SH * 0.16f, FLinearColor(1, 0.85f, 0.35f, D->GetCheckpointFlash()), 1.1f * Sc);
		break;
	}
}
