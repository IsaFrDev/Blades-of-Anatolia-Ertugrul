#include "ErtHUD.h"
#include "ErtCharacter.h"
#include "ErtEnemy.h"
#include "ErtHorse.h"
#include "ErtNpc.h"
#include "ErtWorldBuilder.h"
#include "CanvasItem.h"
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
	if (GM && GM->IsSettingsOpen()) { DrawSettings(SW, SH, Sc); return; }
	if (GM && GM->IsMenuOpen()) { DrawMenu(SW, SH, Sc); return; }
	if (GM && GM->GetMenu() == EErtMenu::Main) { DrawMainMenu(SW, SH, Sc, false); return; }
	if (GM && GM->GetMenu() == EErtMenu::Pause) { DrawMainMenu(SW, SH, Sc, true); return; }
	if (GM && GM->GetMenu() == EErtMenu::Map) { DrawMap(SW, SH, Sc); return; }
	if (GM && GM->GetMenu() == EErtMenu::Inventory) { DrawInventory(SW, SH, Sc); return; }
	if (GM && GM->IsDialogActive()) { DrawDialog(SW, SH, Sc); return; }
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.6f, 0.6f, 0.55f), Green(0.5f, 0.9f, 0.4f), Red(0.9f, 0.25f, 0.2f);

	// --- pastki chap: sog'liq, stamina, o'q ---
	if (H)
	{
		const float X = 24 * Sc, Y = SH - 78 * Sc;
		Bar(X, Y, 260 * Sc, 16 * Sc, H->GetHealth() / H->GetMaxHealth(), H->GetHealth() > 30 ? FLinearColor(0.75f, 0.15f, 0.12f) : Red);
		Bar(X, Y + 20 * Sc, 200 * Sc, 10 * Sc, H->GetStamina() / 100.f, FLinearColor(0.85f, 0.7f, 0.25f));
		Bar(X, Y + 32 * Sc, 200 * Sc, 5 * Sc, (float)H->XP / H->XPToNext(), FLinearColor(0.4f, 0.7f, 1.f));
		Text(FString::Printf(TEXT("%s %d   %s %d   Dori %d  Go'sht %d   Oltin %d   Daraja %d"), *L.Tr(TEXT("ui.hud.health")), (int32)H->GetHealth(), *L.Tr(TEXT("ui.hud.arrows")), H->GetArrows(), H->Potions, H->Meat, H->Gold, H->Level), X, Y + 40 * Sc, White, Sc);
		if (H->LevelFlash > 0.f) Text(FString::Printf(TEXT("DARAJA %d!"), H->Level), SW * 0.5f - TextWidth(FString::Printf(TEXT("DARAJA %d!"), H->Level), 1.6f * Sc, true) * 0.5f, SH * 0.30f, FLinearColor(0.5f, 0.85f, 1.f, H->LevelFlash), 1.6f * Sc, true, true);
		if (GM && GM->ShopMsgT > 0.f) Text(GM->ShopMsg, SW * 0.5f - TextWidth(GM->ShopMsg, Sc, false) * 0.5f, SH * 0.36f, FLinearColor(1.f, 0.85f, 0.35f), Sc);
		if (H->GetComboWindow() > 0.f && H->GetComboStep() > 0) Text(FString::Printf(TEXT("x%d"), H->GetComboStep() + 1), SW * 0.5f + 120 * Sc, SH * 0.5f - 40 * Sc, FLinearColor(1.f, 0.7f, 0.3f, FMath::Min(1.f, H->GetComboWindow() * 2.f)), 1.3f * Sc, true, true);
		if (H->GetExecuteFlash() > 0.f) Text(TEXT("IJRO!"), SW * 0.5f - TextWidth(TEXT("IJRO!"), 1.8f * Sc, true) * 0.5f, SH * 0.40f, FLinearColor(0.95f, 0.2f, 0.15f, H->GetExecuteFlash()), 1.8f * Sc, true, true);
		else if (H->GetParryFlash() > 0.f) Text(TEXT("PARRY!"), SW * 0.5f - TextWidth(TEXT("PARRY!"), 1.6f * Sc, true) * 0.5f, SH * 0.42f, FLinearColor(1.f, 0.85f, 0.3f, H->GetParryFlash()), 1.6f * Sc, true, true);
		else if (H->GetRiposteT() > 0.f) Text(TEXT("Zarba x2"), SW * 0.5f - TextWidth(TEXT("Zarba x2"), Sc, false) * 0.5f, SH * 0.46f, FLinearColor(1.f, 0.6f, 0.2f), Sc);
		if (H->GetLockTarget()) Text(TEXT("LMB x3 seriya | LMB ushlab: og'ir | V: tepki | X: dodge | Q: qulfni ochish"), X + 280 * Sc, Y + 34 * Sc, FLinearColor(0.85f, 0.8f, 0.6f), 0.9f * Sc);
		if (H->IsRiding()) Text(TEXT("Otda: W yurish/yo'rtish, Shift chopish, A/D burilish, Space sakrash, E tushish"), X, Y - 22 * Sc, FLinearColor(0.85f, 0.8f, 0.6f), 0.9f * Sc);
		else if (H->NearestCarcass(260.f)) Text(TEXT("[E] Go'sht olish"), X, Y - 22 * Sc, FLinearColor(1.f, 0.85f, 0.35f), Sc);
		else if (AErtNpc* Np = H->NearestNpc(280.f)) Text(FString::Printf(TEXT("[E] %s bilan gaplashish"), *Np->GetDisplayName()), X, Y - 22 * Sc, FLinearColor(1.f, 0.85f, 0.35f), Sc);
		else if (H->NearestHorse(320.f)) Text(TEXT("[E] Otga minish"), X, Y - 22 * Sc, FLinearColor(1.f, 0.85f, 0.35f), Sc);
		if (H->GetHurtFlash() > 0.f)
		{
			FCanvasTileItem V(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.6f, 0.f, 0.f, 0.35f * H->GetHurtFlash())); V.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(V);
		}
	}
	// --- minimap (o'ng yuqori burchak, 250 m radius) ---
	if (H) DrawMapArea(SW - 210 * Sc, 20 * Sc, 190 * Sc, H->GetActorLocation().Y / 100.f, H->GetActorLocation().X / 100.f, 250.f, false, Sc);
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

	// --- boss sog'liq chizig'i ---
	for (const AErtEnemy* E : D->GetEnemies())
	{
		if (!E || !E->IsBoss() || E->IsDead()) continue;
		const float BW = SW * 0.5f, BX = (SW - BW) * 0.5f, BY = 26 * Sc;
		Text(L.TrOr(TEXT("chr.noyan.name"), TEXT("No'yon")), BX, BY - 20 * Sc, FLinearColor(0.95f, 0.3f, 0.2f), 1.1f * Sc, true, true);
		Bar(BX, BY, BW, 14 * Sc, E->GetHealth() / E->GetMaxHealth(), FLinearColor(0.8f, 0.12f, 0.1f));
		if (E->IsWindingUp()) Text(TEXT("OG'IR ZARBA - DODGE (X)!"), SW * 0.5f - TextWidth(TEXT("OG'IR ZARBA - DODGE (X)!"), Sc, false) * 0.5f, BY + 18 * Sc, FLinearColor(1.f, 0.5f, 0.2f), Sc);
		break;
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
		// Dushman ko'rsatkichlari (faol, yaqin) + zarba oldidan "!" + qulf retikuli
		for (const AErtEnemy* E : D->GetEnemies())
		{
			if (!E || E->IsDead() || E->IsAnimal() || !E->IsAlerted()) continue;
			const FVector S = Project(E->GetActorLocation() + FVector(0, 0, 130.f));
			if (S.Z <= 0.f) continue;
			Bar(S.X - 20 * Sc, S.Y, 40 * Sc, 5 * Sc, E->GetHealth() / E->GetMaxHealth(), Red);
			if (E->IsWindingUp()) Text(TEXT("!"), S.X - 5 * Sc, S.Y - 30 * Sc, FLinearColor(1.f, 0.25f, 0.1f), 1.6f * Sc, true, true);
			if (E == H->GetLockTarget()) { const float R = 22 * Sc; for (int32 k = 0; k < 4; ++k) { const float A = k * PI * 0.5f + PI * 0.25f; FCanvasLineItem Ln(FVector2D(S.X + FMath::Cos(A) * R, S.Y + 8 * Sc + FMath::Sin(A) * R), FVector2D(S.X + FMath::Cos(A + 0.9f) * R, S.Y + 8 * Sc + FMath::Sin(A + 0.9f) * R)); Ln.SetColor(Gold); Ln.LineThickness = 2.f; Canvas->DrawItem(Ln); } }
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

// ---------------- Sozlamalar ----------------

void AErtHUD::DrawSettings(float SW, float SH, float Sc)
{
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM) return;
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.6f, 0.6f, 0.55f);
	FCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.02f, 0.02f, 0.03f, 0.85f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	Text(TEXT("SOZLAMALAR / AYARLAR / SETTINGS"), 40 * Sc, 30 * Sc, Gold, 1.4f * Sc, true, true);
	static const TCHAR* LangNames[] = { TEXT("O'zbek"), TEXT("Türkçe"), TEXT("English") };
	const FString Rows[3] = {
		FString::Printf(TEXT("Til / Dil / Language:   < %s >"), LangNames[FMath::Clamp(GM->Language, 0, 2)]),
		FString::Printf(TEXT("Sichqoncha sezgirligi / Mouse:   < %.1f >"), GM->MouseSens),
		FString::Printf(TEXT("Y o'qini teskari / Invert Y:   < %s >"), GM->bInvertY ? TEXT("Ha / Yes") : TEXT("Yo'q / No")) };
	for (int32 i = 0; i < 3; ++i)
	{
		const float Y = 110 * Sc + i * 34 * Sc;
		if (i == GM->GetSettingsRow()) { FCanvasTileItem S(FVector2D(32 * Sc, Y - 4 * Sc), FVector2D(SW * 0.6f, 30 * Sc), FLinearColor(0.35f, 0.25f, 0.08f, 0.8f)); S.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(S); }
		Text(Rows[i], 44 * Sc, Y, i == GM->GetSettingsRow() ? Gold : White, 1.1f * Sc);
	}
	Text(FString::Printf(TEXT("Or/iymon: %d    Bajarilgan epizodlar: saqlangan (Saved/ert_save.json)"), GM->GetHonor()), 44 * Sc, 240 * Sc, Grey, Sc);
	Text(TEXT("Yuqori/Pastga: qator   Chap/O'ng yoki Enter: o'zgartirish   O yoki Esc: yopish"), 40 * Sc, SH - 30 * Sc, Grey, 0.9f * Sc);
}

// ---------------- Bosh menyu / pauza ----------------

void AErtHUD::DrawMainMenu(float SW, float SH, float Sc, bool bPause)
{
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM) return;
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.6f, 0.6f, 0.55f);
	FCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.02f, 0.02f, 0.03f, bPause ? 0.7f : 0.88f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	const FString Title = TEXT("BLADES OF ANATOLIA: ERTUG'RUL");
	Text(Title, (SW - TextWidth(Title, 2.f * Sc, true)) * 0.5f, SH * 0.18f, Gold, 2.f * Sc, true, true);
	Text(bPause ? TEXT("PAUZA") : TEXT("1227 - Anadolu chegarasi"), (SW - TextWidth(bPause ? TEXT("PAUZA") : TEXT("1227 - Anadolu chegarasi"), Sc, false)) * 0.5f, SH * 0.18f + 50 * Sc, Grey, Sc);
	const TCHAR* MainRows[] = { TEXT("Boshlash / Davom etish"), TEXT("Epizodlar"), TEXT("Sozlamalar"), TEXT("Chiqish") };
	const TCHAR* PauseRows[] = { TEXT("Davom etish"), TEXT("Xarita"), TEXT("Epizodlar"), TEXT("Sozlamalar"), TEXT("Saqlab chiqish") };
	const int32 N = bPause ? 5 : 4;
	for (int32 i = 0; i < N; ++i)
	{
		const FString R = bPause ? PauseRows[i] : MainRows[i];
		const float Y = SH * 0.42f + i * 40 * Sc;
		const bool bSel = i == GM->GetMenuRow();
		if (bSel) { FCanvasTileItem S(FVector2D(SW * 0.5f - 200 * Sc, Y - 6 * Sc), FVector2D(400 * Sc, 34 * Sc), FLinearColor(0.35f, 0.25f, 0.08f, 0.85f)); S.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(S); }
		Text(R, (SW - TextWidth(R, 1.25f * Sc, true)) * 0.5f, Y, bSel ? Gold : White, 1.25f * Sc, true, true);
	}
	Text(TEXT("Yuqori/Pastga: tanlash   Enter/Space: tasdiqlash   Esc: orqaga   M: xarita   O: sozlamalar"), 40 * Sc, SH - 30 * Sc, Grey, 0.9f * Sc);
	if (!bPause) Text(FString::Printf(TEXT("Or/iymon: %d"), GM->GetHonor()), SW - 150 * Sc, 24 * Sc, Grey, 0.9f * Sc);
}

// ---------------- Xarita ----------------

void AErtHUD::Circle(float CX, float CY, float R, const FLinearColor& C, int32 Segs)
{
	for (int32 i = 0; i < Segs; ++i)
	{
		const float A0 = 2.f * PI * i / Segs, A1 = 2.f * PI * (i + 1) / Segs;
		FCanvasLineItem L(FVector2D(CX + FMath::Cos(A0) * R, CY + FMath::Sin(A0) * R), FVector2D(CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R));
		L.SetColor(C); L.LineThickness = 2.f; Canvas->DrawItem(L);
	}
}

void AErtHUD::DrawMapArea(float X0, float Y0, float S, float CE, float CN, float RadiusM, bool bLabels, float Sc)
{
	using namespace ErtMap;
	AErtCharacter* H = Cast<AErtCharacter>(GetOwningPawn());
	AErtWorldBuilder* W = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtWorldBuilder::StaticClass()));
	AErtMissionDirector* D = Director();
	const float K = S / (2.f * RadiusM);                       // px / m
	auto PX = [&](float E) { return X0 + S * 0.5f + (E - CE) * K; };
	auto PY = [&](float N) { return Y0 + S * 0.5f - (N - CN) * K; };
	auto Inside = [&](float X, float Y) { return X >= X0 - 4 && X <= X0 + S + 4 && Y >= Y0 - 4 && Y <= Y0 + S + 4; };
	// Fon (pergament) va ramka
	FCanvasTileItem Bg(FVector2D(X0, Y0), FVector2D(S, S), FLinearColor(0.72f, 0.62f, 0.42f, bLabels ? 0.95f : 0.75f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	// Cho'l (janub) va tog' (shimol) zonalari
	{
		const float Yd = PY(-700.f); if (Yd < Y0 + S) { FCanvasTileItem T(FVector2D(X0, FMath::Max(Y0, Yd)), FVector2D(S, Y0 + S - FMath::Max(Y0, Yd)), FLinearColor(0.85f, 0.75f, 0.5f, 0.8f)); T.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(T); }
		const float Ym = PY(850.f); if (Ym > Y0) { FCanvasTileItem T(FVector2D(X0, Y0), FVector2D(S, FMath::Min(Y0 + S, Ym) - Y0), FLinearColor(0.85f, 0.85f, 0.88f, 0.8f)); T.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(T); }
	}
	// Daryo
	if (W)
	{
		for (float N = -1000.f; N < 1000.f; N += 25.f)
		{
			const FVector2D A(PX(W->RiverE(N)), PY(N)), B(PX(W->RiverE(N + 25.f)), PY(N + 25.f));
			if (!Inside(A.X, A.Y) && !Inside(B.X, B.Y)) continue;
			FCanvasLineItem L(A, B); L.SetColor(FLinearColor(0.25f, 0.45f, 0.75f)); L.LineThickness = FMath::Max(2.f, 26.f * K); Canvas->DrawItem(L);
		}
	}
	const FLinearColor Ink(0.25f, 0.15f, 0.08f);
	auto Mark = [&](float E, float N, float Rm, const TCHAR* Label, const FLinearColor& C)
	{
		const float X = PX(E), Y = PY(N);
		if (!Inside(X, Y)) return;
		Circle(X, Y, FMath::Max(3.f, Rm * K), C, 20);
		if (bLabels) Text(Label, X + 6 * Sc, Y - 8 * Sc, Ink, 0.9f * Sc, false);
	};
	Mark(ObaE, ObaN, ObaHalf, TEXT("Qayi obasi"), Ink);
	Mark(FortE, FortN, FortHalf, TEXT("Qal'a"), Ink);
	Mark(CityE, CityN, CityR, TEXT("Shahar"), Ink);
	Mark(CampE, CampN, CampR, TEXT("Mo'g'ul lageri"), FLinearColor(0.6f, 0.1f, 0.1f));
	Mark(OasisE, OasisN, OasisR, TEXT("Voha"), FLinearColor(0.2f, 0.4f, 0.7f));
	Mark(CaravanE, CaravanN, 22.f, TEXT("Karvonsaroy"), Ink);
	Mark(LakeE, LakeN, LakeR, TEXT("Ko'l"), FLinearColor(0.2f, 0.4f, 0.7f));
	Mark(-120.f, -860.f, 20.f, TEXT("Xarobalar"), Ink);
	// Maqsad markerlari
	if (D)
	{
		TArray<FVector> Pts; D->GetMarkers(Pts);
		for (const FVector& P : Pts)
		{
			const float X = PX(P.Y / 100.f), Y = PY(P.X / 100.f);
			if (!Inside(X, Y)) continue;
			const float Sz = 6 * Sc;
			FCanvasTileItem T(FVector2D(X - Sz, Y - Sz), FVector2D(Sz * 2, Sz * 2), FLinearColor(1.f, 0.8f, 0.2f)); T.Rotation = FRotator(0, 45.f, 0); T.PivotPoint = FVector2D(0.5f, 0.5f); Canvas->DrawItem(T);
		}
		for (const AErtEnemy* E : D->GetEnemies())
		{
			if (!E || E->IsDead() || E->IsAnimal()) continue;
			const float X = PX(E->GetActorLocation().Y / 100.f), Y = PY(E->GetActorLocation().X / 100.f);
			if (Inside(X, Y)) { FCanvasTileItem T(FVector2D(X - 3 * Sc, Y - 3 * Sc), FVector2D(6 * Sc, 6 * Sc), FLinearColor(0.85f, 0.15f, 0.1f)); Canvas->DrawItem(T); }
		}
	}
	// NPClar
	{
		TArray<AActor*> Npcs; UGameplayStatics::GetAllActorsOfClass(GetWorld(), AErtNpc::StaticClass(), Npcs);
		for (AActor* A : Npcs) { const float X = PX(A->GetActorLocation().Y / 100.f), Y = PY(A->GetActorLocation().X / 100.f); if (Inside(X, Y)) { FCanvasTileItem T(FVector2D(X - 2.5f * Sc, Y - 2.5f * Sc), FVector2D(5 * Sc, 5 * Sc), FLinearColor(0.2f, 0.6f, 0.9f)); Canvas->DrawItem(T); } }
	}
	// O'yinchi (yo'nalish uchburchagi)
	if (H)
	{
		const float X = PX(H->GetActorLocation().Y / 100.f), Y = PY(H->GetActorLocation().X / 100.f);
		const float Yaw = FMath::DegreesToRadians(H->GetActorRotation().Yaw);
		const float Sz = 7 * Sc;
		FCanvasTileItem Tp(FVector2D(X - Sz, Y - Sz * 1.6f), FVector2D(Sz * 2, Sz * 3.2f), FLinearColor(0.1f, 0.9f, 0.3f));
		Tp.Rotation = FRotator(0, FMath::RadiansToDegrees(Yaw), 0); Tp.PivotPoint = FVector2D(0.5f, 0.5f); Canvas->DrawItem(Tp);
		FCanvasTileItem Tc(FVector2D(X - Sz * 0.5f, Y - Sz * 0.5f), FVector2D(Sz, Sz), FLinearColor(0.02f, 0.3f, 0.1f)); Canvas->DrawItem(Tc);
	}
	// Ramka
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector2D A(X0 + (i == 1 ? S : 0), Y0 + (i == 2 ? S : 0)), B(X0 + (i == 3 ? 0 : S), Y0 + (i == 0 ? 0 : S));
		FCanvasLineItem L(i == 0 ? FVector2D(X0, Y0) : (i == 1 ? FVector2D(X0 + S, Y0) : (i == 2 ? FVector2D(X0 + S, Y0 + S) : FVector2D(X0, Y0 + S))),
		                  i == 0 ? FVector2D(X0 + S, Y0) : (i == 1 ? FVector2D(X0 + S, Y0 + S) : (i == 2 ? FVector2D(X0, Y0 + S) : FVector2D(X0, Y0))));
		L.SetColor(Ink); L.LineThickness = 2.f; Canvas->DrawItem(L);
	}
	if (!bLabels) Text(TEXT("N"), X0 + S * 0.5f - 4 * Sc, Y0 + 2 * Sc, Ink, 0.8f * Sc, false);
}

void AErtHUD::DrawMap(float SW, float SH, float Sc)
{
	FCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.02f, 0.02f, 0.03f, 0.85f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	const float S = FMath::Min(SW, SH) - 80 * Sc;
	DrawMapArea((SW - S) * 0.5f, 40 * Sc, S, 0.f, 0.f, 1000.f, true, Sc);
	AErtMissionDirector* D = Director();
	if (D && D->GetObjectives().Num())
	{
		float Y = 44 * Sc;
		Text(D->GetPhaseTitle(), 24 * Sc, Y, FLinearColor(1.f, 0.85f, 0.35f), 1.1f * Sc, true, true); Y += 26 * Sc;
		for (const FErtObjective& O : D->GetObjectives()) { Text(O.Text(), 24 * Sc, Y, O.bDone ? FLinearColor(0.5f, 0.9f, 0.4f) : FLinearColor(0.95f, 0.95f, 0.9f), 0.95f * Sc); Y += 20 * Sc; }
	}
	{
		// Yon kvestlar jurnali (o'ng ustun)
		float QY = 44 * Sc; const float QX = SW - 300 * Sc;
		Text(FErtLoc::Get().Tr(TEXT("ui.hud.quests")), QX, QY, FLinearColor(1.f, 0.85f, 0.35f), 1.1f * Sc, true, true); QY += 26 * Sc;
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
			for (const AErtMissionDirector::FSideInfo& Sq : AErtMissionDirector::LoadSideQuests())
			{
				const bool bDone = GM->HasFlag(TEXT("sq_done_") + Sq.Id);
				const bool bActive = D && D->IsSideQuest() && D->GetEpisodeId() == Sq.Id && D->GetState() != EErtMissionState::Inactive;
				const FString Mark = bDone ? TEXT("[x] ") : (bActive ? TEXT("[>] ") : TEXT("[ ] "));
				Text(Mark + FErtLoc::Get().TrOr(Sq.TitleKey, Sq.Id), QX, QY, bDone ? FLinearColor(0.5f, 0.9f, 0.4f) : (bActive ? FLinearColor(1.f, 0.85f, 0.35f) : FLinearColor(0.95f, 0.95f, 0.9f)), 0.9f * Sc); QY += 19 * Sc;
			}
	}
	Text(TEXT("XARITA   (M yoki Esc: yopish)   yashil - siz, oltin - maqsad, qizil - dushman, ko'k - odamlar"), 24 * Sc, SH - 28 * Sc, FLinearColor(0.6f, 0.6f, 0.55f), 0.9f * Sc);
}

// ---------------- Inventar ----------------

void AErtHUD::DrawInventory(float SW, float SH, float Sc)
{
	AErtCharacter* H = Cast<AErtCharacter>(GetOwningPawn());
	if (!H) return;
	const FLinearColor Gold(1.f, 0.85f, 0.35f), White(0.95f, 0.95f, 0.9f), Grey(0.6f, 0.6f, 0.55f);
	FCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.02f, 0.02f, 0.03f, 0.85f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	Text(TEXT("INVENTAR"), 40 * Sc, 30 * Sc, Gold, 1.5f * Sc, true, true);
	float Y = 90 * Sc;
	auto Row = [&](const FString& K, const FString& V) { Text(K, 44 * Sc, Y, Grey, Sc); Text(V, 300 * Sc, Y, White, Sc); Y += 26 * Sc; };
	Row(TEXT("Daraja"), FString::Printf(TEXT("%d   (XP %d / %d)"), H->Level, H->XP, H->XPToNext()));
	Row(TEXT("Sog'liq"), FString::Printf(TEXT("%d / %d"), (int32)H->GetHealth(), (int32)H->GetMaxHealth()));
	Row(TEXT("Stamina"), FString::Printf(TEXT("%d / %d"), (int32)H->GetStamina(), (int32)H->StaminaMax));
	Row(TEXT("Oltin"), FString::Printf(TEXT("%d"), H->Gold));
	Row(TEXT("Dori (H)"), FString::Printf(TEXT("%d   (+45 sog'liq)"), H->Potions));
	Row(TEXT("Kiyik go'shti"), FString::Printf(TEXT("%d   (+25, dori bo'lmasa H bilan)"), H->Meat));
	Row(TEXT("O'qlar"), FString::Printf(TEXT("%d / %d"), H->GetArrows(), H->MaxArrows));
	Y += 10 * Sc;
	Text(TEXT("JIHOZ"), 44 * Sc, Y, Gold, 1.1f * Sc, true, true); Y += 30 * Sc;
	Row(TEXT("Qilich"), H->SwordTier >= 2 ? TEXT("Damashq po'lati (+12 zarar)") : TEXT("Oddiy qilich"));
	Row(TEXT("Qalqon"), H->bShield ? TEXT("Yog'och qalqon (blok 95%, kam stamina)") : TEXT("Yo'q"));
	Row(TEXT("Kamon"), H->BowTier >= 2 ? TEXT("Kompozit kamon (+20 zarar, 24 o'q)") : TEXT("Oddiy kamon"));
	Row(TEXT("Zarar"), FString::Printf(TEXT("qilich %d, o'q %d"), (int32)H->AttackDamage, (int32)H->ArrowDamage));
	Y += 16 * Sc;
	Text(TEXT("Savdogar Yusuf (shahar bozori): dori 15, 8 o'q 10, qalqon 60, kompozit kamon 90, Damashq qilichi 120 oltin"), 44 * Sc, Y, Grey, 0.9f * Sc);
	Text(TEXT("XP: dushman 20-80, epizod 150.  Oltin: dushmandan 4-14, epizod 40."), 44 * Sc, Y + 22 * Sc, Grey, 0.9f * Sc);
	Text(TEXT("I yoki Esc: yopish"), 40 * Sc, SH - 30 * Sc, Grey, 0.9f * Sc);
}
