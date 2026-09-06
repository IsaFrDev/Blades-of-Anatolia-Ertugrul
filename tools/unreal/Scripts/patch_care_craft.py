# -*- coding: utf-8 -*-
# Ot parvarishi (boqish/tarash), hunarmandchilik (temirchi Deli Demir), tush jumboq dialoglari, yuz animatsiyasi (morph/jag')
import io, os, json, csv, sys
SRC = r"D:\Unreal_projects\Ertugrul\Source\Ertugrul"
DATA = r"D:\Unreal_projects\Ertugrul\Content\Ertugrul\Data"

def rd(p): return io.open(p, encoding="utf-8").read()
def wr(p, s): io.open(p, "w", encoding="utf-8", newline="\n").write(s)
def rep(path, old, new, must=True):
    p = os.path.join(SRC, path); s = rd(p)
    if new in s and old not in s: print("  = allaqachon:", path); return
    if old not in s:
        if must: print("!! topilmadi:", path, old[:60]); sys.exit(1)
        return
    wr(p, s.replace(old, new, 1)); print("  + ", path)

# ---------------- Ot: parvarish ----------------
rep("ErtHorse.h",
    "\tvoid Summon(const FVector& To) { SummonTo = To; SummonT = 25.f; }",
    "\tvoid Summon(const FVector& To) { SummonTo = To; SummonT = 25.f; }\n"
    "\t// Parvarish: boqish (go'sht/olma) va tarash - Care 0..1, tezlik +12% * Care, sog'liq tiklanishi tezroq, chaqirilganda tezroq keladi\n"
    "\tfloat Care = 0.f, CareFxT = 0.f;\n"
    "\tvoid Feed() { Health = MaxHealth; Care = FMath::Min(1.f, Care + 0.35f); CareFxT = 2.5f; }\n"
    "\tvoid Groom() { Care = FMath::Min(1.f, Care + 0.5f); CareFxT = 2.5f; }\n"
    "\tfloat CareSpeed() const { return 1.f + 0.12f * Care; }\n"
    "\tfloat GetHealth() const { return Health; } float GetMaxHealth() const { return MaxHealth; }")
rep("ErtHorse.cpp",
    "\tif (!Rider && Health < MaxHealth) Health = FMath::Min(MaxHealth, Health + 3.f * Dt);",
    "\tif (!Rider && Health < MaxHealth) Health = FMath::Min(MaxHealth, Health + (3.f + 4.f * Care) * Dt);\n"
    "\tCare = FMath::Max(0.f, Care - Dt / 900.f); CareFxT = FMath::Max(0.f, CareFxT - Dt);")
rep("ErtHorse.cpp",
    "\t\tif (Input.Y > 0.2f) Target = bGallopIn ? GallopSpeed : (Input.Y > 0.7f ? TrotSpeed : WalkSpeed);",
    "\t\tif (Input.Y > 0.2f) Target = (bGallopIn ? GallopSpeed : (Input.Y > 0.7f ? TrotSpeed : WalkSpeed)) * CareSpeed();")
rep("ErtHorse.cpp",
    "CurSpeed = FMath::FInterpConstantTo(CurSpeed, Dist > 1500.f ? GallopSpeed * 0.8f : TrotSpeed, Dt, 400.f);",
    "CurSpeed = FMath::FInterpConstantTo(CurSpeed, (Dist > 1500.f ? GallopSpeed * 0.8f : TrotSpeed) * CareSpeed(), Dt, 400.f);")

# Personaj: V (tepish) ot yonida = tarash; H (dori) ot yonida go'sht bilan = boqish
rep("ErtCharacter.cpp",
    "\tif (!bInputEnabled || bMantling || bDead || AttackCD > 0.f || Horse) return;\n\tAttackCD = 0.9f;\n\tStamina = FMath::Max(0.f, Stamina - 8.f);\n\tDoAttack(3, 0.3f, true, 0.9f, 420.f);",
    "\tif (!bInputEnabled || bMantling || bDead || AttackCD > 0.f || Horse) return;\n"
    "\tif (AErtHorse* Hh = NearestHorse(300.f)) { if (!Hh->IsMounted()) { AttackCD = 1.2f; Hh->Groom(); if (Body) Body->TriggerAttack(3); FErtAudio::PlaySfx(GetWorld(), TEXT(\"block\"), GetActorLocation(), 0.35f, 0.7f);\n"
    "\t\tif (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->ShopMsg = FString::Printf(TEXT(\"Ot tarandi: parvarish %d%% (tezlik +%d%%)\"), (int32)(Hh->Care * 100.f), (int32)(Hh->Care * 12.f)); GM->ShopMsgT = 3.f; } return; } }\n"
    "\tAttackCD = 0.9f;\n\tStamina = FMath::Max(0.f, Stamina - 8.f);\n\tDoAttack(3, 0.3f, true, 0.9f, 420.f);")
rep("ErtCharacter.cpp",
    "void AErtCharacter::OnPotion() { if (bInputEnabled && !bDead) UsePotion(); }",
    "void AErtCharacter::OnPotion()\n{\n\tif (!bInputEnabled || bDead) return;\n"
    "\tif (!Horse) if (AErtHorse* Hh = NearestHorse(300.f)) if (!Hh->IsMounted() && Meat > 0)\n"
    "\t{\n\t\tMeat--; Hh->Feed(); FErtAudio::PlaySfx(GetWorld(), TEXT(\"block\"), GetActorLocation(), 0.35f, 0.6f);\n"
    "\t\tif (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->ShopMsg = FString::Printf(TEXT(\"Ot boqildi: sog'liq to'liq, parvarish %d%%\"), (int32)(Hh->Care * 100.f)); GM->ShopMsgT = 3.f; }\n\t\treturn;\n\t}\n"
    "\tUsePotion();\n}")

# ---------------- Resurslar: temir (dushman o'ljasi), teri (kiyik) ----------------
rep("ErtCharacter.h", "\tint32 Meat = 0;", "\tint32 Meat = 0, Iron = 0, Leather = 0; bool bIronArmor = false; int32 ArrowTier = 1;")
rep("ErtLoot.h", "\tint32 Gold = 0, Arrows = 0, Potions = 0, Meat = 0;", "\tint32 Gold = 0, Arrows = 0, Potions = 0, Meat = 0, Iron = 0;")
rep("ErtLoot.cpp", "H->Potions += Potions; H->Meat += Meat;", "H->Potions += Potions; H->Meat += Meat; H->Iron += Iron;")
rep("ErtEnemy.cpp",
    "\t\t\tL->bBoss = Kind == EErtEnemyKind::Boss;",
    "\t\t\tL->Iron = Kind == EErtEnemyKind::Boss ? 3 : (FMath::FRand() < (bRich ? 0.6f : 0.3f) ? 1 : 0);\n\t\t\tL->bBoss = Kind == EErtEnemyKind::Boss;")
rep("ErtCharacter.cpp",
    "\t\tCc->bLooted = true; Meat += 2; AddXP(5);",
    "\t\tCc->bLooted = true; Meat += 2; Leather += 1; AddXP(5);")
rep("ErtCharacter.cpp",
    "GM->ShopMsg = TEXT(\"Kiyik go'shti +2 (H bilan yeyiladi, +25)\");",
    "GM->ShopMsg = TEXT(\"Kiyik go'shti +2, teri +1 (H: yeyish yoki ot yonida boqish)\");")
# Describe: temir
rep("ErtLoot.cpp", "\tif (Meat > 0) S += FString::Printf(TEXT(\"%d go'sht  \"), Meat);", "\tif (Meat > 0) S += FString::Printf(TEXT(\"%d go'sht  \"), Meat);\n\tif (Iron > 0) S += FString::Printf(TEXT(\"%d temir  \"), Iron);")
if False:
    import re
    m = re.search(r"FString AErtLoot::Describe\(\) const\n\{\n", s)
    if not m: print("!! Describe topilmadi"); sys.exit(1)
    s = s[:m.end()] + "\tFString IronS = Iron > 0 ? FString::Printf(TEXT(\", temir %d\"), Iron) : FString();\n" + s[m.end():]
    # append IronS to the returned string: find 'return ' in Describe
    i = s.find("return ", m.end()); j = s.find(";", i)
    s = s[:i] + "return (" + s[i+7:j] + ") + IronS;" + s[j+1:]
    wr(p, s); print("  +  ErtLoot.cpp Describe")

# ---------------- Zarar/zirh: temir zirh, po'lat o'q ----------------
p = os.path.join(SRC, "ErtCharacter.cpp"); s = rd(p)
if "bIronArmor" not in s:
    # damage reduction: find bPeltArmor usage in TakeDamage-like code
    idx = s.find("bPeltArmor ?")
    if idx < 0: idx = s.find("bPeltArmor)")
    print("  pelt usage:", s[max(0, idx-120):idx+80].replace("\n", " | "))
rep("ErtCharacter.cpp", "	if (bPeltArmor) Damage *= 0.85f;   // bo'ri terisi zirhi", "	if (bIronArmor) Damage *= 0.8f; else if (bPeltArmor) Damage *= 0.85f;   // temir zirh / bo'ri terisi zirhi")
rep("ErtCharacter.cpp", "	ArrowDamage = 45.f + (Level - 1) * 3.f + (BowTier >= 2 ? 20.f : 0.f);", "	ArrowDamage = 45.f + (Level - 1) * 3.f + (BowTier >= 2 ? 20.f : 0.f) + (ArrowTier >= 2 ? 8.f : 0.f);")
# ---------------- GameMode: craft bayroqlari ----------------
rep("ErtGameMode.cpp",
    "\t\tBuy(TEXT(\"buy_potion\"), 15, [&]() { H->Potions += 1; });",
    "\t\t// Hunarmandchilik (Deli Demir temirxonasi): teri/temir -> po'lat o'q, temir zirh, Damashq qilichi, dori\n"
    "\t\tauto Craft = [&](const TCHAR* Flag, int32 NeedIron, int32 NeedLeather, TFunction<void()> Give, const TCHAR* Msg)\n"
    "\t\t{\n\t\t\tif (!Flags.Contains(Flag)) return; Flags.Remove(Flag);\n"
    "\t\t\tif (H->Iron < NeedIron || H->Leather < NeedLeather) { ShopMsg = FString::Printf(TEXT(\"Yetarli emas: kerak temir %d, teri %d (sizda %d / %d)\"), NeedIron, NeedLeather, H->Iron, H->Leather); ShopMsgT = 4.f; return; }\n"
    "\t\t\tH->Iron -= NeedIron; H->Leather -= NeedLeather; Give(); H->ApplyEquipment(); ShopMsg = Msg; ShopMsgT = 4.f; SaveGame();\n\t\t};\n"
    "\t\tCraft(TEXT(\"craft_arrows\"), 1, 1, [&]() { H->AddArrows(12); H->ArrowTier = 2; }, TEXT(\"Temirchi: 12 po'lat o'q yasaldi (+8 zarar)\"));\n"
    "\t\tCraft(TEXT(\"craft_armor\"), 3, 2, [&]() { H->bIronArmor = true; }, TEXT(\"Temirchi: temir zirh (zarar -20%)\"));\n"
    "\t\tCraft(TEXT(\"craft_sword\"), 4, 0, [&]() { H->SwordTier = 2; }, TEXT(\"Temirchi: Damashq qilichi (+12 zarar)\"));\n"
    "\t\tCraft(TEXT(\"craft_shield\"), 2, 1, [&]() { H->bShield = true; }, TEXT(\"Temirchi: temir qoplamali qalqon\"));\n"
    "\t\t// Tush jumboqlari: to'g'ri javob - donolik (or +5, keyingi bosqichda to'liq shifo); noto'g'ri - or -2\n"
    "\t\tif (Flags.Contains(TEXT(\"dream_wise\"))) { Flags.Remove(TEXT(\"dream_wise\")); Flags.Add(TEXT(\"dream_gift\")); AddHonor(5); H->Heal(100.f); ShopMsg = TEXT(\"Tush jumboqi yechildi: or +5, Ulukayin ne'mati (shifo)\"); ShopMsgT = 4.f; }\n"
    "\t\tif (Flags.Contains(TEXT(\"dream_lost\"))) { Flags.Remove(TEXT(\"dream_lost\")); AddHonor(-2); ShopMsg = TEXT(\"Tush jumboqi yechilmadi: or -2\"); ShopMsgT = 4.f; }\n"
    "\t\tBuy(TEXT(\"buy_potion\"), 15, [&]() { H->Potions += 1; });")
# Refresh craft flags before dialog (visible options)
rep("ErtGameMode.cpp",
    "\tif (!Dialog.Start(Npc->GetDialogId(), &Flags, &Honor)) return;\n\tNpc->SetTalking(true); TalkingNpc = Npc;",
    "\tRefreshCraftFlags();\n\tif (!Dialog.Start(Npc->GetDialogId(), &Flags, &Honor)) return;\n\tNpc->SetTalking(true); TalkingNpc = Npc;")
rep("ErtGameMode.cpp",
    "void AErtGameMode::RefreshSideQuestFlags()\n{",
    "void AErtGameMode::RefreshCraftFlags()\n{\n"
    "\tAErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)); if (!H) return;\n"
    "\tauto Set = [&](const TCHAR* F, bool b) { if (b) Flags.Add(F); else Flags.Remove(F); };\n"
    "\tSet(TEXT(\"can_craft_arrows\"), H->Iron >= 1 && H->Leather >= 1);\n"
    "\tSet(TEXT(\"can_craft_armor\"), H->Iron >= 3 && H->Leather >= 2 && !H->bIronArmor);\n"
    "\tSet(TEXT(\"can_craft_sword\"), H->Iron >= 4 && H->SwordTier < 2);\n"
    "\tSet(TEXT(\"can_craft_shield\"), H->Iron >= 2 && H->Leather >= 1 && !H->bShield);\n"
    "}\n\n"
    "bool AErtGameMode::StartDialogId(const FString& Id)\n{\n"
    "\tif (Dialog.IsActive() || (Cutscene && Cutscene->IsPlaying())) return false;\n"
    "\tif (!Dialog.Start(Id, &Flags, &Honor)) return false;\n"
    "\tTalkingNpc = nullptr; SetPlayerInput(false, false); return true;\n}\n\n"
    "void AErtGameMode::RefreshSideQuestFlags()\n{")
rep("ErtGameMode.h",
    "\tvoid AddFlag(const FString& F) { Flags.Add(F); }",
    "\tvoid AddFlag(const FString& F) { Flags.Add(F); }\n\tvoid RefreshCraftFlags();\n\tbool StartDialogId(const FString& Id); // NPCsiz dialog (tush jumboqi)")

# Save/load Iron/Leather/IronArmor/ArrowTier
p = os.path.join(SRC, "ErtGameMode.cpp"); s = rd(p)
if "\"iron\"" not in s:
    a = s.find("R->SetNumberField(TEXT(\"meat\")")
    if a < 0:
        a = s.find("SetNumberField(TEXT(\"potions\")")
    if a < 0: print("!! save meat topilmadi"); sys.exit(1)
    e = s.find("\n", a)
    s = s[:e+1] + "\tR->SetNumberField(TEXT(\"iron\"), H->Iron); R->SetNumberField(TEXT(\"leather\"), H->Leather); R->SetBoolField(TEXT(\"ironArmor\"), H->bIronArmor); R->SetNumberField(TEXT(\"arrowTier\"), H->ArrowTier);\n" + s[e+1:]
    # load
    b = s.find("TEXT(\"meat\")", e)
    if b < 0: b = s.find("TEXT(\"potions\")", e)
    e2 = s.find("\n", b)
    s = s[:e2+1] + "\t\t{ double D = 0; if (R->TryGetNumberField(TEXT(\"iron\"), D)) H->Iron = (int32)D; if (R->TryGetNumberField(TEXT(\"leather\"), D)) H->Leather = (int32)D; bool B = false; if (R->TryGetBoolField(TEXT(\"ironArmor\"), B)) H->bIronArmor = B; if (R->TryGetNumberField(TEXT(\"arrowTier\"), D)) H->ArrowTier = (int32)D; }\n" + s[e2+1:]
    wr(p, s); print("  +  ErtGameMode.cpp save/load")

# ---------------- Missiya: tush tugaganda jumboq dialogi ----------------
rep("ErtMission.cpp",
    "\t\tif (bAllDone && bWavesDone)\n\t\t{\n\t\t\tH->AddArrows(4);\n\t\t\tH->Heal(25.f);",
    "\t\tif (bAllDone && bWavesDone && Phases.IsValidIndex(PhaseIdx) && Phases[PhaseIdx].bDream && !bDreamRiddleDone)\n"
    "\t\t{\n\t\t\t// Tush jumboqi: Ulukayin savol beradi, javob real dunyoga bayroq beradi\n"
    "\t\t\tbDreamRiddleDone = true;\n"
    "\t\t\tif (AErtGameMode* GMr = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { const int32 Ri = FMath::Abs(GlobalIdx) % 3 + 1; if (GMr->StartDialogId(FString::Printf(TEXT(\"dream_riddle_%d\"), Ri))) break; }\n"
    "\t\t}\n"
    "\t\tif (bAllDone && bWavesDone)\n\t\t{\n\t\t\tH->AddArrows(4);\n\t\t\tH->Heal(25.f);")
rep("ErtMission.h", "\tFVector DreamReturn", "\tbool bDreamRiddleDone = false; int32 GlobalIdx = 0;\n\tFVector DreamReturn", must=False)
s = rd(os.path.join(SRC, "ErtMission.h"))
if "bDreamRiddleDone" not in s:
    print("  DreamReturn e'loni:", [l for l in s.splitlines() if "DreamReturn" in l][:2])
    sys.exit(1)
# GlobalIdx set at episode start; reset riddle flag at StartPhase
rep("ErtMission.cpp",
    "\tPhaseIdx = Idx; PhaseT = 0.f;",
    "\tPhaseIdx = Idx; PhaseT = 0.f; bDreamRiddleDone = false;")
s = rd(os.path.join(SRC, "ErtMission.cpp"))
if "GlobalIdx = E.GlobalIndex" not in s:
    i = s.find("E.GlobalIndex % 4 == 2")
    # find the start of that line's function: put assignment before 'if (Phases.Num() >= 2 && E.GlobalIndex % 4 == 2)' first occurrence
    j = s.rfind("\n", 0, i)
    s = s[:j+1] + "\t\tGlobalIdx = E.GlobalIndex;\n" + s[j+1:]
    wr(os.path.join(SRC, "ErtMission.cpp"), s); print("  +  ErtMission.cpp GlobalIdx")

# ---------------- Yuz animatsiyasi (morph target / jag' suyagi) ----------------
rep("ErtHeroBody.h",
    "\tvoid SetRiding(bool bOn) { bRide = bOn; }",
    "\tvoid SetRiding(bool bOn) { bRide = bOn; }\n"
    "\t// Yuz: gapirganda og'iz morph-targetlari (MetaHuman/Paragon/Daz nomlari), davriy ko'z yumish; suyakli yuzda jag' suyagi\n"
    "\tvoid SetTalk(bool bOn) { bTalk = bOn; }\n\tvoid FaceAnimate(float Dt);\n\tbool bTalk = false; float TalkT = 0.f, BlinkT = 2.f, BlinkV = 0.f; int32 MouthMorph = -2, BlinkMorph = -2; FName MouthName, BlinkName, BlinkName2; float FaceScan = 0.f;")
rep("ErtHeroBody.cpp",
    "void UErtHeroBody::SkelAnimate(float Dt, float Speed, bool bInAir, bool bCrouched)\n{\n\tIdleT += Dt;",
    "void UErtHeroBody::FaceAnimate(float Dt)\n{\n"
    "\tif (!Skel || !Skel->GetSkeletalMeshAsset()) return;\n"
    "\tif (MouthMorph == -2 || FaceScan > 0.f) { FaceScan -= Dt; if (MouthMorph == -2 || FaceScan <= 0.f) {\n"
    "\t\tFaceScan = 5.f; MouthMorph = -1; BlinkMorph = -1;\n"
    "\t\tstatic const TCHAR* Mouths[] = { TEXT(\"MouthOpen\"), TEXT(\"Mouth_Open\"), TEXT(\"mouthOpen\"), TEXT(\"JawOpen\"), TEXT(\"jawOpen\"), TEXT(\"Jaw_Open\"), TEXT(\"CTRL_expressions_jawOpen\"), TEXT(\"viseme_aa\"), TEXT(\"AA\"), TEXT(\"Talk\") };\n"
    "\t\tstatic const TCHAR* Blinks[] = { TEXT(\"EyeBlink\"), TEXT(\"Blink\"), TEXT(\"eyesClosed\"), TEXT(\"EyesClosed\"), TEXT(\"eyeBlinkLeft\"), TEXT(\"Blink_L\"), TEXT(\"CTRL_expressions_eyeBlinkL\") };\n"
    "\t\tstatic const TCHAR* Blinks2[] = { TEXT(\"\"), TEXT(\"\"), TEXT(\"\"), TEXT(\"\"), TEXT(\"eyeBlinkRight\"), TEXT(\"Blink_R\"), TEXT(\"CTRL_expressions_eyeBlinkR\") };\n"
    "\t\tUSkeletalMesh* SM = Skel->GetSkeletalMeshAsset();\n"
    "\t\tfor (const TCHAR* Nm : Mouths) if (SM->FindMorphTarget(FName(Nm))) { MouthMorph = 1; MouthName = FName(Nm); break; }\n"
    "\t\tfor (int32 i = 0; i < UE_ARRAY_COUNT(Blinks); ++i) if (SM->FindMorphTarget(FName(Blinks[i]))) { BlinkMorph = 1; BlinkName = FName(Blinks[i]); BlinkName2 = FName(Blinks2[i]); break; }\n"
    "\t\tif (MouthMorph < 0) { static const TCHAR* Jaws[] = { TEXT(\"jaw\"), TEXT(\"Jaw\"), TEXT(\"FACIAL_C_Jaw\"), TEXT(\"jaw_01\"), TEXT(\"head_jaw\") }; for (const TCHAR* Nm : Jaws) if (Skel->GetBoneIndex(FName(Nm)) != INDEX_NONE) { MouthMorph = 2; MouthName = FName(Nm); break; } }\n"
    "\t} }\n"
    "\tif (MouthMorph == 1 || MouthMorph == 2)\n\t{\n"
    "\t\tTalkT = bTalk ? TalkT + Dt : 0.f;\n"
    "\t\tconst float Open = bTalk ? FMath::Clamp(0.25f + 0.35f * FMath::Sin(TalkT * 11.f) + 0.25f * FMath::Sin(TalkT * 17.3f + 1.f), 0.f, 0.8f) : 0.f;\n"
    "\t\tif (MouthMorph == 1) Skel->SetMorphTarget(MouthName, Open);\n"
    "\t}\n"
    "\tif (BlinkMorph == 1)\n\t{\n"
    "\t\tBlinkT -= Dt; if (BlinkT <= 0.f) { BlinkT = FMath::FRandRange(2.f, 5.f); BlinkV = 1.f; }\n"
    "\t\tif (BlinkV > 0.f) { BlinkV = FMath::Max(0.f, BlinkV - Dt * 7.f); const float B = FMath::Sin(BlinkV * PI); Skel->SetMorphTarget(BlinkName, B); if (!BlinkName2.IsNone() && BlinkName2 != NAME_None && BlinkName2.ToString().Len() > 0) Skel->SetMorphTarget(BlinkName2, B); }\n"
    "\t}\n}\n\n"
    "void UErtHeroBody::SkelAnimate(float Dt, float Speed, bool bInAir, bool bCrouched)\n{\n\tIdleT += Dt;\n\tFaceAnimate(Dt);")
s = rd(os.path.join(SRC, "ErtHeroBody.cpp"))
if "#include \"Engine/SkeletalMesh.h\"" not in s:
    s = s.replace("#include \"ErtHeroBody.h\"", "#include \"ErtHeroBody.h\"\n#include \"Engine/SkeletalMesh.h\"\n#include \"Components/SkeletalMeshComponent.h\"", 1); wr(os.path.join(SRC, "ErtHeroBody.cpp"), s); print("  +  ErtHeroBody.cpp include")
# NPC va qahramon: kim gapirsa o'sha og'zini qimirlatadi
rep("ErtNpc.cpp",
    "\tif (!Body || !Body->IsBuilt()) return;",
    "\tif (!Body || !Body->IsBuilt()) return;\n"
    "\tif (AErtGameMode* GMt = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) Body->SetTalk(bTalking && GMt->IsDialogActive() && GMt->DialogSpeaker() != TEXT(\"Ertugrul\"));")
rep("ErtGameMode.h",
    "\tvoid RefreshCraftFlags();",
    "\tvoid RefreshCraftFlags();\n\tFString DialogSpeaker() const { return Dialog.IsActive() ? Dialog.Speaker() : FString(); }")
# Hero talk: in character Tick where Body->Animate is called
p = os.path.join(SRC, "ErtCharacter.cpp"); s = rd(p)
if "Body->SetTalk(" not in s:
    i = s.find("Body->Animate(")
    j = s.rfind("\n", 0, i)
    s = s[:j+1] + "\tif (Body) if (AErtGameMode* GMt = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) Body->SetTalk(GMt->IsDialogActive() && GMt->DialogSpeaker() == TEXT(\"Ertugrul\"));\n" + s[j+1:]
    wr(p, s); print("  +  ErtCharacter.cpp SetTalk")

# ---------------- HUD: inventar qatorlari ----------------
rep("ErtHUD.cpp",
    "\tRow(TEXT(\"O'qlar\"), FString::Printf(TEXT(\"%d / %d\"), H->GetArrows(), H->MaxArrows));",
    "\tRow(TEXT(\"O'qlar\"), FString::Printf(TEXT(\"%d / %d%s\"), H->GetArrows(), H->MaxArrows, H->ArrowTier >= 2 ? TEXT(\"  (po'lat, +8)\") : TEXT(\"\")));\n"
    "\tRow(TEXT(\"Temir / teri\"), FString::Printf(TEXT(\"%d / %d   (dushman o'ljasi / kiyik; Deli Demir temirxonasi - oba)\"), H->Iron, H->Leather));")
rep("ErtHUD.cpp",
    "\tRow(TEXT(\"Zirh\"), H->bPeltArmor ? TEXT(\"Bo'ri terisi (zarar -15%)\") : TEXT(\"Charm ko'krak zirhi\"));",
    "\tRow(TEXT(\"Zirh\"), H->bIronArmor ? TEXT(\"Temir zirh (zarar -20%)\") : H->bPeltArmor ? TEXT(\"Bo'ri terisi (zarar -15%)\") : TEXT(\"Charm ko'krak zirhi\"));")
rep("ErtHUD.cpp",
    "\tText(TEXT(\"XP: dushman 20-80, epizod 150.  Oltin: dushmandan 4-14, epizod 40.\"), 44 * Sc, Y + 22 * Sc, Grey, 0.9f * Sc);",
    "\tText(TEXT(\"XP: dushman 20-80, epizod 150.  Oltin: dushmandan 4-14, epizod 40.\"), 44 * Sc, Y + 22 * Sc, Grey, 0.9f * Sc);\n"
    "\tText(TEXT(\"Temirchi Deli Demir: 12 po'lat o'q = 1 temir + 1 teri; temir zirh = 3 temir + 2 teri; Damashq qilichi = 4 temir; qalqon = 2 temir + 1 teri\"), 44 * Sc, Y + 44 * Sc, Grey, 0.9f * Sc);\n"
    "\tText(TEXT(\"Ot: yonida V - tarash, H (go'sht bilan) - boqish: sog'liq to'liq, tezlik +12% gacha\"), 44 * Sc, Y + 66 * Sc, Grey, 0.9f * Sc);")

# ---------------- Data: Deli Demir dialogi (hunarmandchilik) ----------------
dp = os.path.join(DATA, "dialogue", "npc_deli_demir.json")
d = json.load(io.open(dp, encoding="utf-8"))
n0 = d["nodes"]["n0"]
if not any(o.get("set_flag") == "craft_arrows" for o in n0["options"]):
    n0["options"] = [o for o in n0["options"] if o.get("next") != "craft"]
    n0["options"].insert(0, {"text_key": "DLG_CRAFT_OPT", "next": "craft"})
    d["nodes"]["craft"] = {"speaker": "Deli Demir", "text_key": "DLG_CRAFT_MENU", "options": [
        {"text_key": "DLG_CRAFT_ARROWS", "next": "craft_done", "set_flag": "craft_arrows", "requires_evidence": "can_craft_arrows"},
        {"text_key": "DLG_CRAFT_ARMOR", "next": "craft_done", "set_flag": "craft_armor", "requires_evidence": "can_craft_armor"},
        {"text_key": "DLG_CRAFT_SWORD", "next": "craft_done", "set_flag": "craft_sword", "requires_evidence": "can_craft_sword"},
        {"text_key": "DLG_CRAFT_SHIELD", "next": "craft_done", "set_flag": "craft_shield", "requires_evidence": "can_craft_shield"},
        {"text_key": "DLG_CRAFT_BACK", "next": "end"}]}
    d["nodes"]["craft_done"] = {"speaker": "Deli Demir", "text_key": "DLG_CRAFT_DONE", "next": "end"}
    io.open(dp, "w", encoding="utf-8", newline="\n").write(json.dumps(d, ensure_ascii=False, indent=1) + "\n"); print("  +  npc_deli_demir.json")

# Tush jumboqlari (3 ta)
riddles = {
 1: [("DLG_DREAM_R1_Q", "Ulukayin"), [("DLG_DREAM_R1_A", "dream_lost"), ("DLG_DREAM_R1_B", "dream_wise"), ("DLG_DREAM_R1_C", "dream_lost")]],
 2: [("DLG_DREAM_R2_Q", "Ulukayin"), [("DLG_DREAM_R2_A", "dream_wise"), ("DLG_DREAM_R2_B", "dream_lost"), ("DLG_DREAM_R2_C", "dream_lost")]],
 3: [("DLG_DREAM_R3_Q", "Ulukayin"), [("DLG_DREAM_R3_A", "dream_lost"), ("DLG_DREAM_R3_B", "dream_lost"), ("DLG_DREAM_R3_C", "dream_wise")]],
}
for k, (q, opts) in riddles.items():
    fp = os.path.join(DATA, "dialogue", "dream_riddle_%d.json" % k)
    if os.path.exists(fp): continue
    nodes = {"q": {"speaker": q[1], "text_key": "DLG_DREAM_INTRO", "next": "r"},
             "r": {"speaker": q[1], "text_key": q[0], "options": [{"text_key": t, "next": "good" if f == "dream_wise" else "bad", "set_flag": f} for t, f in opts]},
             "good": {"speaker": q[1], "text_key": "DLG_DREAM_GOOD", "next": "end"},
             "bad": {"speaker": q[1], "text_key": "DLG_DREAM_BAD", "next": "end"}}
    json.dump({"id": "dream_riddle_%d" % k, "start": "q", "nodes": nodes}, io.open(fp, "w", encoding="utf-8", newline="\n"), ensure_ascii=False, indent=1); print("  +  dream_riddle_%d.json" % k)

# Lokalizatsiya
loc = os.path.join(DATA, "ertugrul_loc.csv")
rows = [
 ("DLG_CRAFT_OPT", "Temirxona: nimadir yasab ber (temir/teri bilan)", "Demirhane: bir sey yap (demir/deri ile)", "Forge: craft something (iron/leather)"),
 ("DLG_CRAFT_MENU", "Temir keltirdingmi? Ayt, nima yasay: o'q, zirh, qilich, qalqon.", "Demir getirdin mi? Soyle, ne yapayim: ok, zirh, kilic, kalkan.", "Did you bring iron? Say what I should forge: arrows, armor, sword, shield."),
 ("DLG_CRAFT_ARROWS", "12 po'lat o'q (1 temir + 1 teri)", "12 celik ok (1 demir + 1 deri)", "12 steel arrows (1 iron + 1 leather)"),
 ("DLG_CRAFT_ARMOR", "Temir zirh (3 temir + 2 teri)", "Demir zirh (3 demir + 2 deri)", "Iron armor (3 iron + 2 leather)"),
 ("DLG_CRAFT_SWORD", "Damashq qilichi (4 temir)", "Sam kilici (4 demir)", "Damascus sword (4 iron)"),
 ("DLG_CRAFT_SHIELD", "Temir qoplamali qalqon (2 temir + 1 teri)", "Demir kapli kalkan (2 demir + 1 deri)", "Iron-bound shield (2 iron + 1 leather)"),
 ("DLG_CRAFT_BACK", "Hozircha kerak emas", "Simdilik gerek yok", "Not now"),
 ("DLG_CRAFT_DONE", "Bo'ldi, olov sovumasidan ol. Alloh yordamching bo'lsin, Ertug'rul.", "Tamam, ates sogumadan al. Allah yardimcin olsun Ertugrul.", "Done. Take it before the fire cools. May God be your helper, Ertugrul."),
 ("DLG_DREAM_INTRO", "Ertug'rul... Men Ulukayin, hayot daraxtiman. Uch belgini topding. Endi jumboqimga javob ber.", "Ertugrul... Ben Ulukayin, hayat agaciyim. Uc isareti buldun. Simdi bilmeceme cevap ver.", "Ertugrul... I am Ulukayin, the tree of life. You found the three signs. Now answer my riddle."),
 ("DLG_DREAM_R1_Q", "Qilichdan o'tkir, oltindan qimmat, yo'qotsang qaytmas. Bu nima?", "Kilictan keskin, altindan degerli, kaybedersen geri gelmez. Nedir?", "Sharper than a sword, dearer than gold, lost and never returned. What is it?"),
 ("DLG_DREAM_R1_A", "Kuch", "Guc", "Strength"),
 ("DLG_DREAM_R1_B", "Or-nomus (so'z)", "Namus (soz)", "Honor (a given word)"),
 ("DLG_DREAM_R1_C", "Oltin", "Altin", "Gold"),
 ("DLG_DREAM_R2_Q", "Qanoti yo'q, lekin uchadi; og'zi yo'q, lekin butun obani uyg'otadi. Bu nima?", "Kanadi yok ama ucar; agzi yok ama butun obayi uyandirir. Nedir?", "No wings yet it flies; no mouth yet it wakes the whole tribe. What is it?"),
 ("DLG_DREAM_R2_A", "Xabar (ovoza)", "Haber", "News (a rumor)"),
 ("DLG_DREAM_R2_B", "Burgut", "Kartal", "The eagle"),
 ("DLG_DREAM_R2_C", "Shamol", "Ruzgar", "The wind"),
 ("DLG_DREAM_R3_Q", "Bir ota - ko'p o'g'il; o'g'illari bir bo'lsa tog'ni ko'chiradi, ayrilsa qamishdek sinadi. Bu nima?", "Bir baba - cok ogul; ogullar bir olsa dagi tasir, ayrilsa kamis gibi kirilir. Nedir?", "One father, many sons; united they move mountains, divided they snap like reeds. What is it?"),
 ("DLG_DREAM_R3_A", "Qo'shin", "Ordu", "An army"),
 ("DLG_DREAM_R3_B", "Oila", "Aile", "A family"),
 ("DLG_DREAM_R3_C", "O'q dastasi (qabila birligi)", "Ok demeti (boy birligi)", "A bundle of arrows (the tribe's unity)"),
 ("DLG_DREAM_GOOD", "To'g'ri. Uyg'onganingda kuchingni to'liq topasan. Yo'ling ochiq bo'lsin.", "Dogru. Uyandiginda gucunu tam bulacaksin. Yolun acik olsun.", "Correct. When you wake, your strength will be whole. May your road be open."),
 ("DLG_DREAM_BAD", "Yo'q... Hali o'rganishing kerak. Tush tugadi, uyg'on.", "Hayir... Daha ogrenmen gerek. Ruya bitti, uyan.", "No... You still have much to learn. The dream ends. Wake."),
]
s = rd(loc)
have = set(l.split(",", 1)[0] for l in s.splitlines())
add = []
for k, uz, tr, en in rows:
    if k not in have: add.append('%s,"%s","%s","%s"' % (k, uz, tr, en))
if add:
    if not s.endswith("\n"): s += "\n"
    wr(loc, s + "\n".join(add) + "\n"); print("  +  loc %d" % len(add))
print("OK")
