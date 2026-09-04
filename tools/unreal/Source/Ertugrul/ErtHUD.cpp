#include "ErtHUD.h"
#include "ErtCharacter.h"
#include "ErtEnemy.h"
#include "ErtHorse.h"
#include "ErtNpc.h"
#include "ErtLoc.h"
#include "ErtMission.h"
#include "ErtCutscene.h"
#include "ErtEpisodeDb.h"
#include "ErtGameMode.h"
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
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GM && GM->GetCutscene() && GM->GetCutscene()->IsPlaying()) { DrawCutscene(SW, SH, Sc); return; }
	if (GM && GM->IsMenuOpen()) { DrawMenu(SW, SH, Sc); return; }
	if (GM && GM->IsDialogActive()) { DrawDialog(SW, SH, Sc); return; }
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.6f, 0.6f, 0.55f), Green(0.5f, 0.9f, 0.4f), Red(0.9f, 0.25f, 0.2f);

	// --- pastki chap: sog'liq, stamina, o'q ---
	if (H)
	{
		const float X = 24 * Sc, Y = SH - 78 * Sc;
		Bar(X, Y, 260 * Sc, 16 * Sc, H->GetHealth() / H->GetMaxHealth(), H->GetHealth() > 30 ? FLinearColor(0.75f, 0.15f, 0.12f) : Red);
		Bar(X, Y + 20 * Sc, 200 * Sc, 10 * Sc, H->GetStamina() / 100.f, FLinearColor(0.85f, 0.7f, 0.25f));
		Text(FString::Printf(TEXT("%s %d   %s %d"), *L.Tr(TEXT("ui.hud.health")), (int32)H->GetHealth(), *L.Tr(TEXT("ui.hud.arrows")), H->GetArrows()), X, Y + 34 * Sc, White, Sc);
		if (H->GetParryFlash() > 0.f) Text(TEXT("PARRY!"), SW * 0.5f - TextWidth(TEXT("PARRY!"), 1.6f * Sc, true) * 0.5f, SH * 0.42f, FLinearColor(1.f, 0.85f, 0.3f, H->GetParryFlash()), 1.6f * Sc, true, true);
		else if (H->GetRiposteT() > 0.f) Text(TEXT("Zarba x2"), SW * 0.5f - TextWidth(TEXT("Zarba x2"), Sc, false) * 0.5f, SH * 0.46f, FLinearColor(1.f, 0.6f, 0.2f), Sc);
		if (H->IsRiding()) Text(TEXT("Otda: W yurish/yo'rtish, Shift chopish, A/D burilish, Space sakrash, E tushish"), X, Y - 22 * Sc, FLinearColor(0.85f, 0.8f, 0.6f), 0.9f * Sc);
		else if (AErtNpc* Np = H->NearestNpc(280.f)) Text(FString::Printf(TEXT("[E] %s bilan gaplashish"), *Np->GetDisplayName()), X, Y - 22 * Sc, FLinearColor(1.f, 0.85f, 0.35f), Sc);
		else if (H->NearestHorse(320.f)) Text(TEXT("[E] Otga minish"), X, Y - 22 * Sc, FLinearColor(1.f, 0.85f, 0.35f), Sc);
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
	{
		// Cliffhanger ekrani: qora fon, epizod nomi, cliffhanger matni, statistika
		FCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0, 0, 0, FMath::Min(0.85f, T * 0.6f))); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
		Center(L.Tr(TEXT("ui.episodes.completed")), SH * 0.18f, Gold, 1.7f * Sc);
		Center(D->GetEpisodeTitle(), SH * 0.18f + 46 * Sc, White, 1.1f * Sc);
		if (T > 1.5f && !D->GetCliffhanger().IsEmpty())
		{
			TArray<FString> Lines; Wrap(D->GetCliffhanger(), SW * 0.6f, 1.15f * Sc, Lines);
			float CY = SH * 0.42f;
			const int32 Shown = FMath::Min(Lines.Num(), (int32)((T - 1.5f) * 1.5f) + 1);
			for (int32 i = 0; i < Shown; ++i) { Text(Lines[i], (SW - TextWidth(Lines[i], 1.15f * Sc, false)) * 0.5f, CY, FLinearColor(0.95f, 0.9f, 0.8f), 1.15f * Sc); CY += 28 * Sc; }
		}
		Center(FString::Printf(TEXT("%s %d   |   %s %d"), *L.Tr(TEXT("ui.hud.enemies")), D->GetKills(), TEXT("O'lim"), D->GetDeaths()), SH * 0.78f, Grey, Sc);
		if (T > 3.f) Center(L.Tr(TEXT("ui.episodes.next")) + TEXT("   [Enter]"), SH * 0.78f + 30 * Sc, Grey, Sc);
		break;
	}
	default:
		if (D->GetCouncilResultT() > 0.f) Center(D->GetCouncilResult(), SH * 0.22f, FLinearColor(1.f, 0.85f, 0.35f, FMath::Min(1.f, D->GetCouncilResultT())), 1.1f * Sc);
		if (D->GetCheckpointFlash() > 0.f && T > 0.2f)
			Center(L.Tr(TEXT("ui.hud.checkpoint")), SH * 0.16f, FLinearColor(1, 0.85f, 0.35f, D->GetCheckpointFlash()), 1.1f * Sc);
		break;
	}
}

// ---------------- Kat-sahna qatlami ----------------

void AErtHUD::Wrap(const FString& S, float MaxW, float Scale, TArray<FString>& Out) const
{
	TArray<FString> Words; S.ParseIntoArray(Words, TEXT(" "), true);
	FString Line;
	for (const FString& W : Words)
	{
		const FString Try = Line.IsEmpty() ? W : Line + TEXT(" ") + W;
		if (TextWidth(Try, Scale, false) > MaxW && !Line.IsEmpty()) { Out.Add(Line); Line = W; }
		else Line = Try;
	}
	if (!Line.IsEmpty()) Out.Add(Line);
}

void AErtHUD::DrawCutscene(float SW, float SH, float Sc)
{
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	AErtCutsceneDirector* C = GM ? GM->GetCutscene() : nullptr;
	if (!C) return;
	const float LB = C->GetLetterbox() * SH * 0.11f;
	if (LB > 0.f)
	{
		FCanvasTileItem T(FVector2D(0, 0), FVector2D(SW, LB), FLinearColor::Black); Canvas->DrawItem(T);
		FCanvasTileItem B(FVector2D(0, SH - LB), FVector2D(SW, LB), FLinearColor::Black); Canvas->DrawItem(B);
	}
	if (!C->GetSubtitle().IsEmpty())
	{
		TArray<FString> Lines; Wrap(C->GetSubtitle(), SW * 0.7f, 1.15f * Sc, Lines);
		float Y = SH - LB - (Lines.Num() + 1) * 26 * Sc - 12 * Sc;
		if (!C->GetSpeaker().IsEmpty()) { Text(C->GetSpeaker(), (SW - TextWidth(C->GetSpeaker(), Sc, false)) * 0.5f, Y, FLinearColor(1.f, 0.85f, 0.35f), Sc); }
		Y += 24 * Sc;
		for (const FString& Ln : Lines) { Text(Ln, (SW - TextWidth(Ln, 1.15f * Sc, false)) * 0.5f, Y, FLinearColor(0.97f, 0.97f, 0.93f), 1.15f * Sc); Y += 26 * Sc; }
	}
	if (C->GetFade() > 0.f)
	{
		FCanvasTileItem F(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0, 0, 0, FMath::Clamp(C->GetFade(), 0.f, 1.f))); F.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(F);
	}
	const FString Hint = TEXT("Space/Enter: keyingi replika   Esc: tashlab ketish");
	Text(Hint, SW - TextWidth(Hint, 0.85f * Sc, false) - 18 * Sc, SH - 22 * Sc, FLinearColor(0.7f, 0.7f, 0.65f, 0.8f), 0.85f * Sc);
}

// ---------------- Epizod menyusi ----------------

void AErtHUD::DrawMenu(float SW, float SH, float Sc)
{
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM) return;
	const FErtLoc& L = FErtLoc::Get();
	const TArray<FErtEpisode>& All = UErtEpisodeDb::Get()->All();
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.5f, 0.5f, 0.45f), Green(0.5f, 0.9f, 0.4f), Dim(0.35f, 0.33f, 0.3f);
	FCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.02f, 0.02f, 0.03f, 0.82f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	Text(TEXT("BLADES OF ANATOLIA: ERTUG'RUL"), 40 * Sc, 26 * Sc, Gold, 1.5f * Sc, true, true);
	Text(L.Tr(TEXT("ui.episodes.title")), 40 * Sc, 66 * Sc, White, 1.1f * Sc, true, true);

	const int32 Rows = FMath::Clamp((int32)((SH - 150 * Sc) / (24 * Sc)), 6, 18);
	const int32 Sel = GM->GetMenuIndex();
	int32 First = FMath::Clamp(Sel - Rows / 2, 0, FMath::Max(0, All.Num() - Rows));
	float Y = 104 * Sc;
	FString LastSeason;
	for (int32 i = First; i < All.Num() && i < First + Rows; ++i)
	{
		const FErtEpisode& E = All[i];
		const bool bUnl = GM->IsUnlocked(E), bDone = GM->IsCompleted(E.Id);
		if (i == Sel) { FCanvasTileItem S(FVector2D(32 * Sc, Y - 3 * Sc), FVector2D(SW * 0.5f, 23 * Sc), FLinearColor(0.35f, 0.25f, 0.08f, 0.8f)); S.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(S); }
		const FLinearColor C = !bUnl ? Dim : (bDone ? Green : (i == Sel ? Gold : White));
		const FString Mark = bDone ? TEXT("[x] ") : (bUnl ? TEXT("[ ] ") : TEXT("[-] "));
		Text(FString::Printf(TEXT("%s%s  %s"), *Mark, *E.Id, *UErtEpisodeDb::Get()->Title(E)), 40 * Sc, Y, C, Sc);
		Text(FString::Printf(TEXT("%s  %s  T%d"), *E.Gregorian, *E.Archetype, E.DifficultyTier), SW * 0.5f - 250 * Sc, Y, i == Sel ? White : Grey, 0.85f * Sc);
		Y += 24 * Sc;
	}
	// O'ng panel: tanlangan epizod tafsiloti
	if (All.IsValidIndex(Sel))
	{
		const FErtEpisode& E = All[Sel];
		const float X = SW * 0.55f, W = SW * 0.42f;
		float PY = 104 * Sc;
		Text(UErtEpisodeDb::Get()->Title(E), X, PY, Gold, 1.3f * Sc, true, true); PY += 34 * Sc;
		Text(FString::Printf(TEXT("%s  |  %s  |  %s %d  |  %d %s"), *E.Gregorian, *E.Region, *L.Tr(TEXT("ui.episodes.tier")), E.DifficultyTier, E.EstimatedMinutes, *L.Tr(TEXT("ui.episodes.minutes"))), X, PY, Grey, 0.9f * Sc); PY += 26 * Sc;
		TArray<FString> Lines; Wrap(L.TrOr(E.LocSynopsis, TEXT("")), W, Sc, Lines);
		for (const FString& Ln : Lines) { Text(Ln, X, PY, White, Sc); PY += 20 * Sc; }
		PY += 12 * Sc;
		const FString Intro = L.TrOr(E.LocIntro, TEXT(""));
		if (!Intro.IsEmpty()) { Lines.Reset(); Wrap(Intro, W, 0.9f * Sc, Lines); for (const FString& Ln : Lines) { Text(Ln, X, PY, Grey, 0.9f * Sc); PY += 18 * Sc; } }
		PY += 16 * Sc;
		if (!GM->IsUnlocked(E)) Text(L.Tr(TEXT("ui.episodes.locked")), X, PY, FLinearColor(0.9f, 0.3f, 0.2f), Sc);
		else Text(L.Tr(TEXT("ui.episodes.start")) + TEXT("  [Enter]"), X, PY, Green, 1.1f * Sc);
	}
	const FString Hint = TEXT("Yuqori/Pastga: tanlash   Enter: boshlash   Esc/Tab: yopish");
	Text(Hint, 40 * Sc, SH - 30 * Sc, Grey, 0.9f * Sc);
}

// ---------------- Dialog paneli ----------------

void AErtHUD::DrawDialog(float SW, float SH, float Sc)
{
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM) return;
	const FErtDialog& D = GM->GetDialog();
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.6f, 0.6f, 0.55f);
	TArray<FString> Lines; Wrap(D.Text(), SW * 0.62f, 1.05f * Sc, Lines);
	const int32 NOpt = D.OptionTexts().Num();
	const float PanelH = (Lines.Num() * 24 + NOpt * 24 + 70) * Sc;
	const float PX = SW * 0.17f, PY = SH - PanelH - 40 * Sc, PW = SW * 0.66f;
	FCanvasTileItem Bg(FVector2D(PX, PY), FVector2D(PW, PanelH), FLinearColor(0.02f, 0.02f, 0.03f, 0.82f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	FCanvasTileItem Line(FVector2D(PX, PY), FVector2D(PW, 3 * Sc), Gold); Canvas->DrawItem(Line);
	float Y = PY + 12 * Sc;
	Text(D.Speaker(), PX + 18 * Sc, Y, Gold, 1.1f * Sc, true, true); Y += 30 * Sc;
	for (const FString& L : Lines) { Text(L, PX + 18 * Sc, Y, White, 1.05f * Sc); Y += 24 * Sc; }
	if (NOpt > 0)
	{
		Y += 6 * Sc;
		for (int32 i = 0; i < NOpt; ++i)
		{
			const bool bSel = i == D.Selection();
			if (bSel) { FCanvasTileItem S(FVector2D(PX + 12 * Sc, Y - 3 * Sc), FVector2D(PW - 24 * Sc, 22 * Sc), FLinearColor(0.35f, 0.25f, 0.08f, 0.8f)); S.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(S); }
			Text(FString::Printf(TEXT("%d. %s"), i + 1, *D.OptionTexts()[i]), PX + 24 * Sc, Y, bSel ? Gold : White, Sc); Y += 24 * Sc;
		}
		Text(TEXT("1-4 yoki Yuqori/Pastga + Enter: tanlash   Esc: chiqish"), PX + 18 * Sc, PY + PanelH - 22 * Sc, Grey, 0.85f * Sc);
	}
	else Text(TEXT("Space/Enter: davom   Esc: chiqish"), PX + 18 * Sc, PY + PanelH - 22 * Sc, Grey, 0.85f * Sc);
	Text(FString::Printf(TEXT("Or/iymon: %d"), GM->GetHonor()), SW - 150 * Sc, 24 * Sc, Grey, 0.9f * Sc);
}
