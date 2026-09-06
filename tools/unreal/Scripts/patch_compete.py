# -*- coding: utf-8 -*-
# Raqobatchi (Ertugrul of Ulukayin) tahlili asosida: 3 jangchi, Alp mahorat shkalasi, rangli hujum ko'rsatkichi, ot hushtagi, tush bosqichi
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
DATA = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(p, a, b):
    s = load(p)
    if b in s: return
    assert a in s, ("MISSING in " + p + ": " + a[:90])
    save(p, s.replace(a, b))

# ---------------- Ot: hushtak bilan chaqirish ----------------
rep('ErtHorse.h', "\tvoid ApplyDamage(float D);", "\tvoid ApplyDamage(float D);\n\t/** Hushtak: ot o'yinchi tomon yo'rtib keladi (25 s) */\n\tvoid Summon(const FVector& To) { SummonTo = To; SummonT = 25.f; }\n\tbool IsSummoned() const { return SummonT > 0.f; }")
rep('ErtHorse.h', "\tFVector HomePos = FVector::ZeroVector, WanderTarget = FVector::ZeroVector;", "\tFVector HomePos = FVector::ZeroVector, WanderTarget = FVector::ZeroVector, SummonTo = FVector::ZeroVector;\n\tfloat SummonT = 0.f;")
rep('ErtHorse.cpp', "\telse\n\t{\n\t\tCurSpeed = FMath::FInterpConstantTo(CurSpeed, 0.f, Dt, 600.f);\n\t\tWanderT -= Dt;",
"""	else if (SummonT > 0.f)
	{
		// Hushtak: egasi tomon yo'rtish
		SummonT -= Dt;
		const FVector D = (SummonTo - GetActorLocation()).GetSafeNormal2D();
		const float Dist = FVector::Dist2D(SummonTo, GetActorLocation());
		if (Dist > 260.f && !D.IsNearlyZero())
		{
			CurSpeed = FMath::FInterpConstantTo(CurSpeed, Dist > 1500.f ? GallopSpeed * 0.8f : TrotSpeed, Dt, 400.f);
			CM->MaxWalkSpeed = FMath::Max(80.f, CurSpeed);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, D.Rotation().Yaw, 0), Dt, 4.f));
			AddMovementInput(GetActorForwardVector(), 1.f);
		}
		else { SummonT = 0.f; HomePos = GetActorLocation(); WanderTarget = HomePos; CurSpeed = 0.f; }
	}
	else
	{
		CurSpeed = FMath::FInterpConstantTo(CurSpeed, 0.f, Dt, 600.f);
		WanderT -= Dt;""")

# ---------------- Dushman: og'ir zarba ko'rsatkichi ----------------
rep('ErtEnemy.h', "\tbool IsWindingUp() const { return HitPending > 0.f; }", "\tbool IsWindingUp() const { return HitPending > 0.f; }\n\t/** Kutilayotgan zarba og'irmi (to'sib bo'lmaydi - qochish kerak) */\n\tbool IsHeavyPending() const { return bHeavyPending; }")
# Meryem yashirinish: cho'kkan holda ko'rish masofasi 60%
rep('ErtEnemy.cpp', "\t\tconst float SeeRange = Mount ? 2600.f : ((Hero && Hero->bIsCrouched) ? 900.f : 1400.f);", "\t\tfloat SeeRange = Mount ? 2600.f : ((Hero && Hero->bIsCrouched) ? 900.f : 1400.f);\n\t\tif (Hero && Hero->Warrior == 2 && Hero->bIsCrouched) SeeRange *= 0.6f;   // Meryem: yashirinish ustasi")

# ---------------- Personaj: jangchilar, Alp shkalasi, mahorat, hushtak ----------------
h = load('ErtCharacter.h')
h = h.replace("\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Kick;", "\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Kick;\n\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Skill;\n\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Whistle;\n\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Warrior1;\n\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Warrior2;\n\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Warrior3;")
h = h.replace("\tvoid OnAttackPressed(); void OnAttackReleased(); void OnKick();", """	void OnAttackPressed(); void OnAttackReleased(); void OnKick();
	// Jangchilar (raqobatchi tahlili): 0 Ertug'rul (qilich, muvozanat), 1 Turg'ut (bolta: sekin, kuchli, gangitadi), 2 Meryem (kamon, yashirinish, tez)
	int32 Warrior = 0;
	void SetWarrior(int32 W);
	const TCHAR* WarriorName() const { return Warrior == 1 ? TEXT("Turg'ut Alp") : (Warrior == 2 ? TEXT("Meryem") : TEXT("Ertug'rul Bey")); }
	float WarriorMelee = 1.f, WarriorArrow = 1.f, WarriorSpeed = 1.f, WarriorStagger = 0.f, WarriorKnock = 1.f;
	/** Alp mahorat shkalasi (0..100): zarba +7, parry +20; F - maxsus zarba */
	float AlpBar = 0.f;
	void AddAlp(float V) { AlpBar = FMath::Clamp(AlpBar + V, 0.f, 100.f); }
	float GetAlpBar() const { return AlpBar; }
	const TCHAR* SkillName() const { return Warrior == 1 ? TEXT("Yer zarbasi") : (Warrior == 2 ? TEXT("Uch o'q") : TEXT("Bo'ron qilichi")); }
	void OnSkill(); void OnWhistle(); void OnWarrior1() { SetWarrior(0); } void OnWarrior2() { SetWarrior(1); } void OnWarrior3() { SetWarrior(2); }
	float SkillFlash = 0.f;""")
save('ErtCharacter.h', h)

c = load('ErtCharacter.cpp')
# Inputlar
c = c.replace("\tIA_Kick = MakeAction(TEXT(\"IA_ErtKick\"), EInputActionValueType::Boolean);", "\tIA_Kick = MakeAction(TEXT(\"IA_ErtKick\"), EInputActionValueType::Boolean);\n\tIA_Skill = MakeAction(TEXT(\"IA_ErtSkill\"), EInputActionValueType::Boolean);\n\tIA_Whistle = MakeAction(TEXT(\"IA_ErtWhistle\"), EInputActionValueType::Boolean);\n\tIA_Warrior1 = MakeAction(TEXT(\"IA_ErtWarrior1\"), EInputActionValueType::Boolean);\n\tIA_Warrior2 = MakeAction(TEXT(\"IA_ErtWarrior2\"), EInputActionValueType::Boolean);\n\tIA_Warrior3 = MakeAction(TEXT(\"IA_ErtWarrior3\"), EInputActionValueType::Boolean);")
c = c.replace("\tMap(IA_Kick, EKeys::V); Map(IA_Kick, EKeys::Gamepad_RightTrigger);\n}", "\tMap(IA_Kick, EKeys::V); Map(IA_Kick, EKeys::Gamepad_RightTrigger);\n\tMap(IA_Skill, EKeys::F); Map(IA_Skill, EKeys::Gamepad_LeftShoulder);\n\tMap(IA_Whistle, EKeys::Z); Map(IA_Whistle, EKeys::Gamepad_DPad_Down);\n\tMap(IA_Warrior1, EKeys::F1); Map(IA_Warrior2, EKeys::F2); Map(IA_Warrior3, EKeys::F3);\n}")
c = c.replace("\tEIC->BindAction(IA_Potion, ETriggerEvent::Started, this, &AErtCharacter::OnPotion);", "\tEIC->BindAction(IA_Potion, ETriggerEvent::Started, this, &AErtCharacter::OnPotion);\n\tEIC->BindAction(IA_Skill, ETriggerEvent::Started, this, &AErtCharacter::OnSkill);\n\tEIC->BindAction(IA_Whistle, ETriggerEvent::Started, this, &AErtCharacter::OnWhistle);\n\tEIC->BindAction(IA_Warrior1, ETriggerEvent::Started, this, &AErtCharacter::OnWarrior1);\n\tEIC->BindAction(IA_Warrior2, ETriggerEvent::Started, this, &AErtCharacter::OnWarrior2);\n\tEIC->BindAction(IA_Warrior3, ETriggerEvent::Started, this, &AErtCharacter::OnWarrior3);")
c = c.replace('TEXT("Kick"), TEXT("Inventory"), TEXT("Potion"), TEXT("Map"), TEXT("Settings") };', 'TEXT("Kick"), TEXT("Inventory"), TEXT("Potion"), TEXT("Map"), TEXT("Settings"), TEXT("Skill"), TEXT("Whistle") };')
c = c.replace('if (N == TEXT("Dodge")) return IA_Dodge; if (N == TEXT("Lock")) return IA_Lock; if (N == TEXT("Kick")) return IA_Kick; if (N == TEXT("Inventory")) return IA_Inventory;', 'if (N == TEXT("Dodge")) return IA_Dodge; if (N == TEXT("Lock")) return IA_Lock; if (N == TEXT("Kick")) return IA_Kick; if (N == TEXT("Inventory")) return IA_Inventory; if (N == TEXT("Skill")) return IA_Skill; if (N == TEXT("Whistle")) return IA_Whistle;')
# Zarar ko'paytirgichlari
c = c.replace("\t\t\tconst float Dmg = bExecute ? 999.f : AttackDamage * DamageMul * (RiposteT > 0.f ? 2.f : 1.f);", "\t\t\tconst float Dmg = bExecute ? 999.f : AttackDamage * WarriorMelee * DamageMul * (RiposteT > 0.f ? 2.f : 1.f);")
c = c.replace("\t\t\tAErtBurst::Blood(GetWorld(), E->GetActorLocation() + FVector(0, 0, 60.f), (E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() + FVector(0, 0, 0.3f), bExecute ? 2.2f : (Kind == 2 ? 1.5f : 1.f));\n\t\t\tif (StaggerSec > 0.f && !E->IsDead()) E->Stagger(StaggerSec);\n\t\t\tif (Knock > 0.f && !E->IsDead()) E->LaunchCharacter((E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * Knock + FVector(0, 0, 120.f), true, true);",
              "\t\t\tAErtBurst::Blood(GetWorld(), E->GetActorLocation() + FVector(0, 0, 60.f), (E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() + FVector(0, 0, 0.3f), bExecute ? 2.2f : (Kind == 2 ? 1.5f : 1.f));\n\t\t\tAddAlp(7.f);\n\t\t\tif (StaggerSec + WarriorStagger > 0.f && !E->IsDead()) E->Stagger(StaggerSec + WarriorStagger);\n\t\t\tif (Knock * WarriorKnock > 0.f && !E->IsDead()) E->LaunchCharacter((E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * Knock * WarriorKnock + FVector(0, 0, 120.f), true, true);")
c = c.replace("\t\tParryFlash = 1.f; RiposteT = 1.6f;", "\t\tParryFlash = 1.f; RiposteT = 1.6f; AddAlp(20.f);")
c = c.replace("Ar->Launch(D2, 4200.f, ArrowDamage, true, this);", "Ar->Launch(D2, 4200.f, ArrowDamage * WarriorArrow, true, this);")
c = c.replace("\tfloat Speed = Gait == EErtGait::Sprint ? SprintSpeed : (Gait == EErtGait::Walk ? WalkSpeed : JogSpeed);", "\tfloat Speed = (Gait == EErtGait::Sprint ? SprintSpeed : (Gait == EErtGait::Walk ? WalkSpeed : JogSpeed)) * WarriorSpeed;")
c += r'''

// ---------------- Jangchilar, Alp mahorati, hushtak (raqobatchi tahlili: Ertugrul of Ulukayin) ----------------

void AErtCharacter::SetWarrior(int32 W)
{
	if (!bInputEnabled || bDead || Horse) return;
	Warrior = FMath::Clamp(W, 0, 2);
	switch (Warrior)
	{
	case 1: WarriorMelee = 1.6f; WarriorArrow = 0.8f; WarriorSpeed = 0.92f; WarriorStagger = 0.45f; WarriorKnock = 1.6f; break;   // Turg'ut: bolta
	case 2: WarriorMelee = 0.7f; WarriorArrow = 1.45f; WarriorSpeed = 1.12f; WarriorStagger = 0.f; WarriorKnock = 0.8f; break;   // Meryem: kamon
	default: WarriorMelee = 1.f; WarriorArrow = 1.f; WarriorSpeed = 1.f; WarriorStagger = 0.f; WarriorKnock = 1.f; break;
	}
	if (Body)
	{
		Body->bAxe = Warrior == 1;
		if (Warrior == 1) { Body->Kaftan = FLinearColor(0.20f, 0.16f, 0.12f); Body->Trim = FLinearColor(0.55f, 0.45f, 0.25f); Body->Cloak = FLinearColor(0.25f, 0.12f, 0.08f); }
		else if (Warrior == 2) { Body->Kaftan = FLinearColor(0.18f, 0.28f, 0.38f); Body->Trim = FLinearColor(0.85f, 0.75f, 0.45f); Body->Cloak = FLinearColor(0.15f, 0.22f, 0.32f); }
		else { Body->Kaftan = FLinearColor(0.35f, 0.08f, 0.07f); Body->Trim = FLinearColor(0.85f, 0.70f, 0.25f); Body->Cloak = FLinearColor(0.35f, 0.08f, 0.07f); }
		Body->RefreshWeapon();
	}
	if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->ShopMsg = FString::Printf(TEXT("%s: %s"), WarriorName(), Warrior == 1 ? TEXT("bolta - sekin, kuchli, gangituvchi") : (Warrior == 2 ? TEXT("kamon +45%, tez, yashirinish") : TEXT("qilich - muvozanat"))); GM->ShopMsgT = 2.5f; }
	UE_LOG(LogErtugrul, Log, TEXT("Jangchi: %s"), WarriorName());
}

void AErtCharacter::OnSkill()
{
	if (!bInputEnabled || bDead) return;
	if (AlpBar < 100.f) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->ShopMsg = FString::Printf(TEXT("Alp shkalasi %.0f%% - zarba va parry bilan to'ldiring"), AlpBar); GM->ShopMsgT = 1.5f; } return; }
	AlpBar = 0.f; SkillFlash = 1.f;
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this));
	if (Warrior == 2)
	{
		// Meryem: uch o'q (o'q sarflanmaydi)
		const FVector Start = GetActorLocation() + FVector(0, 0, 60.f) + GetActorForwardVector() * 40.f;
		const FVector Base = Cam ? Cam->GetForwardVector() : GetActorForwardVector();
		for (int32 i = -1; i <= 1; ++i)
		{
			const FVector Dir = FRotator(0, i * 7.f, 0).RotateVector(Base) + FVector(0, 0, 0.03f);
			if (AErtArrow* Ar = GetWorld()->SpawnActor<AErtArrow>(AErtArrow::StaticClass(), Start, Dir.Rotation())) Ar->Launch(Dir.GetSafeNormal(), 4400.f, ArrowDamage * WarriorArrow * 1.2f, true, this);
		}
		FErtAudio::PlaySfx(GetWorld(), TEXT("bowshot"), GetActorLocation(), 1.f, 0.9f);
		if (Body) Body->TriggerAttack(1);
		return;
	}
	const bool bSlam = Warrior == 1;
	if (Body) Body->TriggerAttack(2);
	AErtBurst::SwordArc(GetWorld(), GetActorLocation() + FVector(0, 0, 30.f), GetActorRotation().Yaw, 2);
	AErtBurst::SwordArc(GetWorld(), GetActorLocation() + FVector(0, 0, 30.f), GetActorRotation().Yaw + 180.f, 2);
	AErtBurst::Dust(GetWorld(), GetActorLocation(), bSlam ? 3.f : 1.5f);
	FErtAudio::PlaySfx(GetWorld(), TEXT("swing"), GetActorLocation(), 1.f, 0.6f);
	TArray<FOverlapResult> Hits;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtSkill), false, this);
	const float R = bSlam ? 420.f : 280.f;
	if (GetWorld()->OverlapMultiByChannel(Hits, GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(R), Q))
	{
		TSet<AActor*> Done;
		for (const FOverlapResult& Hr : Hits)
		{
			AErtEnemy* E = Cast<AErtEnemy>(Hr.GetActor());
			if (!E || Done.Contains(E) || E->IsAlly() || E->IsDead()) continue;
			Done.Add(E);
			E->ApplyHit(AttackDamage * WarriorMelee * (bSlam ? 1.3f : 1.9f), this, true);
			if (E->IsDead()) { AddXP(E->XPValue()); continue; }
			E->Stagger(bSlam ? 1.8f : 0.9f);
			const FVector Away = (E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			E->LaunchCharacter(Away * (bSlam ? 700.f : 350.f) + FVector(0, 0, bSlam ? 320.f : 140.f), true, true);
			AErtBurst::Blood(GetWorld(), E->GetActorLocation() + FVector(0, 0, 60.f), Away + FVector(0, 0, 0.3f), 1.4f);
		}
	}
	ShakeT = FMath::Max(ShakeT, bSlam ? 0.45f : 0.25f);
	if (GM) { GM->Rumble(bSlam ? 1.f : 0.7f, 0.4f); GM->HitStop(0.12f, 0.2f); }
}

void AErtCharacter::OnWhistle()
{
	if (!bInputEnabled || bDead || Horse) return;
	AErtHorse* Best = nullptr; float BestD = 30000.f;
	TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtHorse::StaticClass(), All);
	for (AActor* A : All)
	{
		AErtHorse* Hs = Cast<AErtHorse>(A);
		if (!Hs || Hs->IsDead() || Hs->IsMounted() || Hs->IsCamel()) continue;
		const float D = FVector::Dist2D(Hs->GetActorLocation(), GetActorLocation());
		if (D < BestD) { BestD = D; Best = Hs; }
	}
	AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this));
	if (Best) { Best->Summon(GetActorLocation()); if (GM) { GM->ShopMsg = FString::Printf(TEXT("Hushtak: ot kelmoqda (%.0f m)"), BestD / 100.f); GM->ShopMsgT = 2.f; } FErtAudio::PlaySfx(GetWorld(), TEXT("bowshot"), GetActorLocation(), 0.6f, 1.6f); }
	else if (GM) { GM->ShopMsg = TEXT("Yaqinda ot yo'q (300 m)"); GM->ShopMsgT = 1.5f; }
}
'''
save('ErtCharacter.cpp', c)
# Tick: SkillFlash so'nishi
rep('ErtCharacter.cpp', "\tDodgeT = FMath::Max(0.f, DodgeT - Dt);\n\tComboWindowT = FMath::Max(0.f, ComboWindowT - Dt);", "\tDodgeT = FMath::Max(0.f, DodgeT - Dt);\n\tComboWindowT = FMath::Max(0.f, ComboWindowT - Dt);\n\tSkillFlash = FMath::Max(0.f, SkillFlash - Dt * 1.5f);")

# ---------------- Tana: bolta ----------------
rep('ErtHeroBody.h', "\tUPROPERTY(EditAnywhere, Category = \"Ertugrul|Kiyim\") bool bMail = false;", "\tUPROPERTY(EditAnywhere, Category = \"Ertugrul|Kiyim\") bool bMail = false;\n\t/** Turg'ut: qilich o'rniga bolta */\n\tUPROPERTY(EditAnywhere, Category = \"Ertugrul|Kiyim\") bool bAxe = false;\n\t/** Qurolni qayta qurish (jangchi almashganda) */\n\tvoid RefreshWeapon() { if (Skel) SkelBuildSword(); }")
rep('ErtHeroBody.cpp', "\tM.AddBox(FVector(46, 0, 0), FVector(42, 0.6f, 3.2f), SteelS);                // tig'\n\tM.AddSphere(FVector(-21, 0, 0), 2.f, 6, TrimS);                              // soqqa\n\tM.Commit(SkelSword, 0, false);",
"""	if (bAxe)
	{
		// Bolta: uzun dasta, keng tig' (Turg'ut Alp)
		M.AddCylinder(FVector(-12, 0, 0), 1.6f, 1.6f, 70.f, 8, ErtCol::Sty(FLinearColor(0.30f, 0.20f, 0.11f), ErtCol::StyleWood), true, FRotator(-90, 0, 0));
		M.AddBox(FVector(52, 0, 9), FVector(6, 1.0f, 12), SteelS);                 // tig' (ikki tomonlama)
		M.AddBox(FVector(52, 0, -9), FVector(6, 1.0f, 12), SteelS);
		M.AddBox(FVector(52, 0, 0), FVector(4, 2.2f, 4), TrimS);                   // bog'lam
	}
	else
	{
		M.AddBox(FVector(46, 0, 0), FVector(42, 0.6f, 3.2f), SteelS);                // tig'
		M.AddSphere(FVector(-21, 0, 0), 2.f, 6, TrimS);                              // soqqa
	}
	M.Commit(SkelSword, 0, false);""")

# ---------------- HUD: jangchi nomi, Alp shkalasi, rangli ko'rsatkich, tush belgisi ----------------
hd = load('ErtHUD.cpp')
hd = hd.replace("\t\t\tif (E->IsWindingUp()) Text(TEXT(\"!\"), S.X - 5 * Sc, S.Y - 30 * Sc, FLinearColor(1.f, 0.25f, 0.1f), 1.6f * Sc, true, true);",
"""			if (E->IsWindingUp())
			{
				// Rangli ko'rsatkich: sariq - yengil (parry/blok), qizil - og'ir (to'sib bo'lmaydi, dodge)
				const bool bHeavy = E->IsHeavyPending();
				const FLinearColor Cw = bHeavy ? FLinearColor(1.f, 0.2f, 0.1f) : FLinearColor(1.f, 0.85f, 0.2f);
				Circle(S.X, S.Y - 34 * Sc, 10 * Sc, Cw, 16); Circle(S.X, S.Y - 34 * Sc, 5 * Sc, FLinearColor(0.1f, 0.05f, 0.02f), 10);
				Text(bHeavy ? TEXT("DODGE") : TEXT("PARRY"), S.X - 18 * Sc, S.Y - 62 * Sc, Cw, 0.8f * Sc, true, true);
			}""")
hd = hd.replace("\t\tText(FString::Printf(TEXT(\"%s %d   %s %d   Dori %d  Go'sht %d   Oltin %d   Daraja %d\"), *L.Tr(TEXT(\"ui.hud.health\")), (int32)H->GetHealth(), *L.Tr(TEXT(\"ui.hud.arrows\")), H->GetArrows(), H->Potions, H->Meat, H->Gold, H->Level), X, Y + 40 * Sc, White, Sc);",
"""		Text(FString::Printf(TEXT("%s %d   %s %d   Dori %d  Go'sht %d   Oltin %d   Daraja %d"), *L.Tr(TEXT("ui.hud.health")), (int32)H->GetHealth(), *L.Tr(TEXT("ui.hud.arrows")), H->GetArrows(), H->Potions, H->Meat, H->Gold, H->Level), X, Y + 40 * Sc, White, Sc);
		// Jangchi va Alp mahorat shkalasi (F)
		{
			const float AX = X + 290 * Sc, AY = Y;
			Text(FString::Printf(TEXT("%s   [F1/F2/F3]"), H->WarriorName()), AX, AY - 18 * Sc, Gold, 0.9f * Sc);
			const bool bReady = H->GetAlpBar() >= 100.f;
			Bar(AX, AY, 180 * Sc, 10 * Sc, H->GetAlpBar() / 100.f, bReady ? FLinearColor(0.4f, 0.9f, 1.f) : FLinearColor(0.2f, 0.55f, 0.85f));
			Text(FString::Printf(TEXT("ALP %s: %s %s"), bReady ? TEXT("TAYYOR") : TEXT(""), H->SkillName(), bReady ? TEXT("[F]") : TEXT("")), AX, AY + 12 * Sc, bReady ? FLinearColor(0.5f, 0.95f, 1.f) : Grey, 0.85f * Sc);
			if (H->SkillFlash > 0.f) Text(H->SkillName(), SW * 0.5f - TextWidth(H->SkillName(), 1.7f * Sc, true) * 0.5f, SH * 0.34f, FLinearColor(0.5f, 0.95f, 1.f, H->SkillFlash), 1.7f * Sc, true, true);
		}
		if (GM && GM->bDream) { Text(TEXT("TUSH"), SW * 0.5f - TextWidth(TEXT("TUSH"), 1.2f * Sc, true) * 0.5f, 18 * Sc, FLinearColor(0.8f, 0.6f, 1.f, 0.8f), 1.2f * Sc, true, true); }""")
save('ErtHUD.cpp', hd)

# ---------------- GameMode: tush rejimi ----------------
rep('ErtGameMode.h', "\tbool bGps = true;", "\tbool bGps = true;\n\t/** Tush (Ulukayin mifi) rejimi: binafsha tuslash, tuman, tun */\n\tbool bDream = false; float SavedDayT = 0.38f; FString SavedWeather;\n\tvoid SetDream(bool bOn);\n\tconst FString& GetWeatherName() const;")
rep('ErtGameMode.cpp', "void AErtGameMode::MapRotate(float DeltaYaw)", """void AErtGameMode::SetDream(bool bOn)
{
	if (bOn == bDream) return;
	bDream = bOn;
	if (bOn) { SavedDayT = DayT; SavedWeather = GetWeatherName(); DayT = 0.97f; SetWeather(TEXT("fog")); }
	else { DayT = SavedDayT; SetWeather(SavedWeather.IsEmpty() ? TEXT("clear") : SavedWeather); }
	UE_LOG(LogErtugrul, Log, TEXT("Tush rejimi: %s"), bOn ? TEXT("yoqildi") : TEXT("o'chdi"));
}

const FString& AErtGameMode::GetWeatherName() const
{
	static FString Empty;
	return Weather ? Weather->GetWeather() : Empty;
}

void AErtGameMode::MapRotate(float DeltaYaw)""")
rep('ErtGameMode.cpp', "\t\tPPV->Settings.bOverride_ColorGain = true; PPV->Settings.ColorGain = FVector4(FMath::Lerp(0.75f, 1.f, Day), FMath::Lerp(0.85f, 1.f, Day), 1.f, 1.f);",
"""		PPV->Settings.bOverride_ColorGain = true; PPV->Settings.ColorGain = FVector4(FMath::Lerp(0.75f, 1.f, Day), FMath::Lerp(0.85f, 1.f, Day), 1.f, 1.f);
		if (bDream)
		{
			// Tush: binafsha-ko'k tus, past to'yinganlik, kuchli vinyetka
			PPV->Settings.ColorSaturation = FVector4(0.45f, 0.45f, 0.45f, 1.f);
			PPV->Settings.ColorGain = FVector4(0.85f, 0.65f, 1.25f, 1.f);
			PPV->Settings.bOverride_VignetteIntensity = true; PPV->Settings.VignetteIntensity = 0.8f;
			PPV->Settings.AutoExposureBias = -0.2f;
		}
		else { PPV->Settings.bOverride_VignetteIntensity = true; PPV->Settings.VignetteIntensity = 0.35f; }""")

# ---------------- Missiya: tush bosqichi ----------------
rep('ErtMission.h', "struct FErtPhase { FString TitleKey; TArray<FErtObjective> Objectives; TArray<FErtWave> Waves; FErtWave Reinforce; bool bReinforced = false; TArray<FErtCouncilNpc> Npcs; };",
    "struct FErtPhase { FString TitleKey; TArray<FErtObjective> Objectives; TArray<FErtWave> Waves; FErtWave Reinforce; bool bReinforced = false; TArray<FErtCouncilNpc> Npcs; bool bDream = false; FVector DreamSpot = FVector::ZeroVector; };")
rep('ErtMission.h', "\tFVector2D CouncilBase = FVector2D::ZeroVector;", "\tFVector2D CouncilBase = FVector2D::ZeroVector;\n\tFVector DreamReturn = FVector::ZeroVector; float DreamReturnYaw = 0.f;")
m = load('ErtMission.cpp')
m = m.replace("\tauto AddDuel = [&]()\n\t{\n\t\tFErtPhase P; P.TitleKey = TEXT(\"ui.phase.duel\");",
"""	// Tush (Ulukayin mifi): Uludog' cho'qqisida 3 belgi (jumboq) yig'iladi, dunyo binafsha tumanda; tugagach qaytish
	auto AddDream = [&]()
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.dream"); P.bDream = true;
		const float SE = UluE, SN = UluN;
		P.DreamSpot = FVector(SN * 100.f, SE * 100.f, (World ? World->HeightAt(SE, SN) : 100.f) * 100.f);
		FErtObjective O; O.Kind = EErtObjKind::Collect; O.LocKey = TEXT("ui.obj.dream"); O.Radius = 260.f; O.Target = 3;
		for (int32 k = 0; k < 3; ++k)
		{
			const float A = 2.f * PI * k / 3.f + Rng.FRandRange(-0.4f, 0.4f), R = Rng.FRandRange(22.f, 48.f);
			const float E = SE + FMath::Cos(A) * R, N = SN + FMath::Sin(A) * R;
			O.Points.Add(FVector(N * 100.f, E * 100.f, (World ? World->HeightAt(E, N) : 100.f) * 100.f)); O.Collected.Add(false);
		}
		P.Objectives.Add(O);
		Phases.Add(P);
	};
	auto AddDuel = [&]()
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.duel");""")
m = m.replace("\telse if (A == TEXT(\"RITUAL\"))       { AddCollect(3); AddHunt(1); AddTravel(2, false); AddDuel(); }",
              "\telse if (A == TEXT(\"RITUAL\"))       { AddDream(); AddCollect(2); AddHunt(1); AddTravel(2, false); AddDuel(); }")
m = m.replace("\telse                                { AddTravel(2, false); AddCollect(3); AddStealth(2); AddFight(1, false); }\n}",
              "\telse                                { AddTravel(2, false); AddCollect(3); AddStealth(2); AddFight(1, false); }\n\t// Har 4-epizodda (global indeks % 4 == 2) ikkinchi bosqich - tush\n\tif (Phases.Num() >= 2 && E.GlobalIndex % 4 == 2 && !Phases[1].bDream) { AddDream(); FErtPhase D = Phases.Last(); Phases.RemoveAt(Phases.Num() - 1); Phases.Insert(D, 1); }\n}")
# StartPhase: tushga kirish
m = m.replace("\tPhaseIdx = Idx; PhaseT = 0.f;\n\tObjectives = P.Objectives;", """	PhaseIdx = Idx; PhaseT = 0.f;
	if (AErtGameMode* GMd = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		AErtCharacter* Hd = Hero();
		if (P.bDream && Hd) { DreamReturn = Hd->GetActorLocation(); DreamReturnYaw = Hd->GetActorRotation().Yaw; Hd->ResetAt(P.DreamSpot + FVector(0, 0, 100.f), 0.f); GMd->SetDream(true); }
		else if (GMd->bDream) { GMd->SetDream(false); if (Hd && !DreamReturn.IsNearlyZero()) Hd->ResetAt(DreamReturn + FVector(0, 0, 100.f), DreamReturnYaw); }
	}
	Objectives = P.Objectives;""")
# Epizod tugashi/to'xtashi: tushdan chiqish
m = m.replace("\tClearEnemies();\n\tClearAllies();\n\tClearPhaseNpcs();\n\tPhases.Reset(); Objectives.Reset(); Waves.Reset();\n\tState = EErtMissionState::Inactive;",
              "\tClearEnemies();\n\tClearAllies();\n\tClearPhaseNpcs();\n\tif (AErtGameMode* GMd = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) if (GMd->bDream) { GMd->SetDream(false); if (AErtCharacter* Hd = Hero()) if (!DreamReturn.IsNearlyZero()) Hd->ResetAt(DreamReturn + FVector(0, 0, 100.f), DreamReturnYaw); }\n\tPhases.Reset(); Objectives.Reset(); Waves.Reset();\n\tState = EErtMissionState::Inactive;")
save('ErtMission.cpp', m)
# GlobalIndex maydoni bormi?
import re
ep = load('ErtEpisodeDb.h')
print('GlobalIndex' in ep)

# ---------------- Lokalizatsiya ----------------
loc = io.open(DATA + 'ui_loc.csv', encoding='utf-8').read()
if 'ui.phase.dream' not in loc:
    loc = loc.rstrip('\n') + '\n"ui.phase.dream","Tush: Ulukayin","Rüya: Ulukayın","Dream: Ulukayin"\n"ui.obj.dream","Tushdagi belgilarni toping (jumboq)","Rüyadaki işaretleri bulun (bilmece)","Find the signs in the dream (riddle)"\n'
    io.open(DATA + 'ui_loc.csv', 'w', encoding='utf-8', newline='\n').write(loc)
print('patched')
