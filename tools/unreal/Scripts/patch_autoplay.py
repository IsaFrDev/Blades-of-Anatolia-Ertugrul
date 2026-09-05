# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:80])
    return s.replace(a, b)

h = load('ErtCharacter.h')
h = rep(h, "\tFVector2D MoveInput = FVector2D::ZeroVector;",
        "\tFVector2D MoveInput = FVector2D::ZeroVector;\n\t// Avtomatik o'yinchi (-ErtAutoPlay): epizodni o'zi o'ynaydi (sinov)\n\tbool bAutoPlay = false;\n\tfloat AutoT = 0.f, AutoTotalT = 0.f, AutoStuckT = 0.f, AutoPhaseT = 0.f, AutoIdleT = 0.f, AutoPotT = 0.f;\n\tint32 AutoLastPhase = -1, AutoTeleports = 0;\n\tFVector AutoLastPos = FVector::ZeroVector;\n\tvoid UpdateAutoPlay(float Dt);")
save('ErtCharacter.h', h)

c = load('ErtCharacter.cpp')
c = rep(c, "\tif (ShotT >= 0.f) UpdateShotScript(Dt);", "\tif (ShotT >= 0.f) UpdateShotScript(Dt);\n\tif (bAutoPlay) UpdateAutoPlay(Dt);")
c = rep(c, "\tif (FParse::Value(FCommandLine::Get(), TEXT(\"-ErtShot=\"), ShotDir))",
        "\tbAutoPlay = FParse::Param(FCommandLine::Get(), TEXT(\"ErtAutoPlay\"));\n\tif (bAutoPlay) UE_LOG(LogErtugrul, Log, TEXT(\"[AutoPlay] yoqildi\"));\n\tif (FParse::Value(FCommandLine::Get(), TEXT(\"-ErtShot=\"), ShotDir))")
c += r'''

// ---------------- Avtomatik o'yinchi (sinov): kat-sahna o'tkaziladi, dialog tanlanadi, maqsadga boriladi, dushmanga hujum ----------------

void AErtCharacter::UpdateAutoPlay(float Dt)
{
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;
	AutoT += Dt; AutoTotalT += Dt; AutoPotT = FMath::Max(0.f, AutoPotT - Dt);
	if (AutoTotalT > 480.f) { UE_LOG(LogErtugrul, Warning, TEXT("[AutoPlay] umumiy vaqt tugadi (480 s) - chiqish")); FPlatformMisc::RequestExit(false); return; }
	if (GM->GetCutscene() && GM->GetCutscene()->IsPlaying()) { if (AutoT > 1.2f) { AutoT = 0.f; GM->OnSkip(); UE_LOG(LogErtugrul, Log, TEXT("[AutoPlay] kat-sahna o'tkazildi")); } DebugMove = FVector2D::ZeroVector; return; }
	if (GM->IsDialogActive())
	{
		if (AutoT > 0.35f) { AutoT = 0.f; if (GM->GetDialog().HasOptions()) GM->DialogChoose(0); else GM->OnAdvance(); }
		DebugMove = FVector2D::ZeroVector; return;
	}
	if (GM->GetMenu() != EErtMenu::None) { if (AutoT > 0.6f) { AutoT = 0.f; OnConfirm(); } return; }
	AErtMissionDirector* D = GM->GetDirector();
	if (!D) return;
	const EErtMissionState St = D->GetState();
	if (St == EErtMissionState::Cleared) { if (D->GetStateTime() > 1.6f && AutoT > 0.5f) { AutoT = 0.f; GM->OnAdvance(); } DebugMove = FVector2D::ZeroVector; return; }
	if (St == EErtMissionState::Inactive)
	{
		AutoIdleT += Dt;
		if (AutoIdleT > 6.f) { UE_LOG(LogErtugrul, Log, TEXT("[AutoPlay] epizod tugadi (%.0f s, %d teleport)"), AutoTotalT, AutoTeleports); FPlatformMisc::RequestExit(false); }
		return;
	}
	AutoIdleT = 0.f;
	if (bDead || St == EErtMissionState::Failed || St == EErtMissionState::Briefing) { DebugMove = FVector2D::ZeroVector; return; }
	// Bosqich vaqti: 150 s dan oshsa dushmanlar yo'q qilinadi (tiqilib qolgan bosqichni o'tkazish)
	if (D->GetPhaseIndex() != AutoLastPhase) { AutoLastPhase = D->GetPhaseIndex(); AutoPhaseT = 0.f; UE_LOG(LogErtugrul, Log, TEXT("[AutoPlay] bosqich %d/%d"), AutoLastPhase + 1, D->GetPhaseCount()); }
	else AutoPhaseT += Dt;
	// Maqsad: eng yaqin tirik dushman, bo'lmasa bajarilmagan maqsad nuqtasi, bo'lmasa marker
	FVector Target = FVector::ZeroVector; bool bHas = false; AErtEnemy* Foe = nullptr; float BestD = 3500.f;
	for (AErtEnemy* E : D->GetEnemies())
	{
		if (!E || E->IsDead() || E->GetKind() == EErtEnemyKind::Deer) continue;
		const float Dd = FVector::Dist2D(E->GetActorLocation(), GetActorLocation());
		if (Dd < BestD) { BestD = Dd; Foe = E; }
	}
	if (Foe) { Target = Foe->GetActorLocation(); bHas = true; }
	else
	{
		for (const FErtObjective& O : D->GetObjectives())
		{
			if (O.bDone || O.bFailed || O.bOptional) continue;
			if (O.Kind == EErtObjKind::Route && O.Points.Num()) { Target = O.Points[FMath::Clamp(O.Progress, 0, O.Points.Num() - 1)]; bHas = true; break; }
			if (O.Kind == EErtObjKind::Hunt) { for (AErtEnemy* E : D->GetEnemies()) if (E && !E->IsDead()) { Target = E->GetActorLocation(); Foe = E; bHas = true; break; } if (bHas) break; }
			if (!O.Point.IsNearlyZero()) { Target = O.Point; bHas = true; break; }
		}
	}
	if (!bHas) { TArray<FVector> Ms; D->GetMarkers(Ms); if (Ms.Num()) { Target = Ms[0]; bHas = true; } }
	if (AutoPhaseT > 150.f)
	{
		UE_LOG(LogErtugrul, Warning, TEXT("[AutoPlay] bosqich %d 150 s da bajarilmadi - dushmanlar yo'q qilinadi"), AutoLastPhase + 1);
		for (AErtEnemy* E : D->GetEnemies()) if (E && !E->IsDead()) E->ApplyHit(99999.f, this, true);
		AutoPhaseT = 0.f;
	}
	if (!bHas) { DebugMove = FVector2D::ZeroVector; return; }
	FVector To = Target - GetActorLocation(); To.Z = 0.f;
	const float Dist = To.Size();
	if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-8.f, To.Rotation().Yaw, 0.f));
	if (Foe && Dist < 230.f)
	{
		DebugMove = FVector2D::ZeroVector;
		if (AutoT > 0.45f) { AutoT = 0.f; if (FMath::FRand() < 0.15f) OnDodge(); else { OnAttackPressed(); OnAttackReleased(); } }
	}
	else if (Dist > 160.f) { DebugMove = FVector2D(0.f, 1.f); bWantSprint = Dist > 900.f && Stamina > 20.f; if (bIsCrouched) UnCrouch(); }
	else
	{
		DebugMove = FVector2D::ZeroVector;
		if (AutoT > 1.f) { AutoT = 0.f; OnInteract(); }   // NPC bilan gaplashish / narsa olish
	}
	if (Health < MaxHealth * 0.35f && Potions > 0 && AutoPotT <= 0.f) { OnPotion(); AutoPotT = 15.f; }
	// Harakat (kat-sahna skriptidagi kabi)
	if (!DebugMove.IsNearlyZero())
	{
		MoveInput = DebugMove;
		if (!Horse)
		{
			const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
			AddMovementInput(FRotationMatrix(YawRot).GetUnitAxis(EAxis::X), DebugMove.Y);
		}
	}
	// Tiqilib qolish: 3 s harakatsiz -> sakrash, 7 s -> maqsad yoniga teleport
	if (FVector::Dist2D(GetActorLocation(), AutoLastPos) < 25.f && !DebugMove.IsNearlyZero()) AutoStuckT += Dt; else AutoStuckT = 0.f;
	AutoLastPos = GetActorLocation();
	if (AutoStuckT > 3.f && AutoStuckT < 3.2f) Jump();
	if (AutoStuckT > 7.f)
	{
		AutoStuckT = 0.f; ++AutoTeleports;
		if (Horse) DismountHorse();
		const FVector Dir = (GetActorLocation() - Target).GetSafeNormal2D();
		const FVector P = Target + Dir * 260.f;
		const float Zg = WorldRef ? WorldRef->HeightAt(P.Y / 100.f, P.X / 100.f) : P.Z / 100.f;
		SetActorLocation(FVector(P.X, P.Y, Zg * 100.f + 120.f), false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogErtugrul, Warning, TEXT("[AutoPlay] tiqilib qoldi - teleport (%d)"), AutoTeleports);
	}
}
'''
save('ErtCharacter.cpp', c)
print('patched')
