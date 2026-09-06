# -*- coding: utf-8 -*-
# 10) Hunarmandchilik kengaytmasi (xom teri -> oshlash, kamon, egar)  11) Ulukayin daraxti (voha)
# 12) Qabila boshqaruvi (hadya -> qabila darajasi: ittifoqchilar, narxlar)  14) Skelet tana: qilich qinda/qo'lda (R), qalqon orqada/qo'lda
import io, os, sys, json
SRC = r"D:\Unreal_projects\Ertugrul\Source\Ertugrul"
DATA = r"D:\Unreal_projects\Ertugrul\Content\Ertugrul\Data"
def rd(p): return io.open(p, encoding="utf-8").read()
def wr(p, s): io.open(p, "w", encoding="utf-8", newline="\n").write(s)
def rep(path, old, new, root=SRC):
    p = os.path.join(root, path); s = rd(p)
    if new in s: print("  = allaqachon:", path); return
    if old not in s: print("!! topilmadi:", path, old[:70]); sys.exit(1)
    wr(p, s.replace(old, new, 1)); print("  + ", path)

# ---------- 10) Resurslar ----------
rep("ErtCharacter.h", "int32 ArrowTier = 1;", "int32 ArrowTier = 1; int32 RawHide = 0; bool bSaddle = false;   // xom teri (kiyik), egar (hunarmandchilik)")
rep("ErtCharacter.cpp", "\t\tCc->bLooted = true; Meat += 2; Leather += 1; AddXP(5);", "\t\tCc->bLooted = true; Meat += 2; RawHide += 1; AddXP(5);")
rep("ErtCharacter.cpp", "GM->ShopMsg = TEXT(\"Kiyik go'shti +2, teri +1 (H: yeyish yoki ot yonida boqish)\");", "GM->ShopMsg = TEXT(\"Kiyik go'shti +2, xom teri +1 (Deli Demir oshlaydi: 2 xom -> 3 charm)\");")
rep("ErtLoot.h", "int32 Gold = 0, Arrows = 0, Potions = 0, Meat = 0, Iron = 0;", "int32 Gold = 0, Arrows = 0, Potions = 0, Meat = 0, Iron = 0, Leather = 0;")
rep("ErtLoot.cpp", "H->Meat += Meat; H->Iron += Iron;", "H->Meat += Meat; H->Iron += Iron; H->Leather += Leather;")
rep("ErtLoot.cpp", "\tif (Iron > 0) S += FString::Printf(TEXT(\"%d temir  \"), Iron);", "\tif (Iron > 0) S += FString::Printf(TEXT(\"%d temir  \"), Iron);\n\tif (Leather > 0) S += FString::Printf(TEXT(\"%d charm  \"), Leather);")
rep("ErtEnemy.cpp", "\t\t\tL->bBoss = Kind == EErtEnemyKind::Boss;", "\t\t\tL->Leather = (Kind == EErtEnemyKind::Rider || Kind == EErtEnemyKind::Elite) && FMath::FRand() < 0.4f ? 1 : 0;\n\t\t\tL->bBoss = Kind == EErtEnemyKind::Boss;")
# Egar: otga minganda tezlik bonusi
rep("ErtHorse.h", "\tfloat CareSpeed() const { return 1.f + 0.12f * Care; }", "\tbool bSaddled = false;   // egar (o'yinchi hunarmandchiligi)\n\tfloat CareSpeed() const { return 1.f + 0.12f * Care + (bSaddled ? 0.08f : 0.f); }")
rep("ErtCharacter.cpp", "\tif (!H || Horse || bSwimming || bMantling) return;\n\tHorse = H;", "\tif (!H || Horse || bSwimming || bMantling) return;\n\tHorse = H; H->bSaddled = bSaddle;")
# Craft: oshlash, kamon, egar
rep("ErtGameMode.cpp",
    "\t\tCraft(TEXT(\"craft_shield\"), 2, 1, [&]() { H->bShield = true; }, TEXT(\"Temirchi: temir qoplamali qalqon\"));",
    "\t\tCraft(TEXT(\"craft_shield\"), 2, 1, [&]() { H->bShield = true; }, TEXT(\"Temirchi: temir qoplamali qalqon\"));\n"
    "\t\tCraft(TEXT(\"craft_bow\"), 1, 2, [&]() { H->BowTier = 2; }, TEXT(\"Temirchi: kompozit kamon (+20 zarar, 24 o'q)\"));\n"
    "\t\tCraft(TEXT(\"craft_saddle\"), 1, 3, [&]() { H->bSaddle = true; if (H->GetHorse()) H->GetHorse()->bSaddled = true; }, TEXT(\"Temirchi: egar - har qanday ot +8% tez\"));\n"
    "\t\tif (Flags.Contains(TEXT(\"craft_tan\"))) { Flags.Remove(TEXT(\"craft_tan\")); if (H->RawHide >= 2) { H->RawHide -= 2; H->Leather += 3; ShopMsg = TEXT(\"Teri oshlandi: 2 xom teri -> 3 charm\"); SaveGame(); } else ShopMsg = TEXT(\"Kamida 2 xom teri kerak\"); ShopMsgT = 4.f; }")
rep("ErtGameMode.cpp",
    "\tSet(TEXT(\"can_craft_shield\"), H->Iron >= 2 && H->Leather >= 1 && !H->bShield);",
    "\tSet(TEXT(\"can_craft_shield\"), H->Iron >= 2 && H->Leather >= 1 && !H->bShield);\n"
    "\tSet(TEXT(\"can_craft_tan\"), H->RawHide >= 2);\n\tSet(TEXT(\"can_craft_bow\"), H->Iron >= 1 && H->Leather >= 2 && H->BowTier < 2);\n\tSet(TEXT(\"can_craft_saddle\"), H->Iron >= 1 && H->Leather >= 3 && !H->bSaddle);\n"
    "\tSet(TEXT(\"can_donate_gold\"), H->Gold >= 100); Set(TEXT(\"can_donate_meat\"), H->Meat >= 5); Set(TEXT(\"can_donate_leather\"), H->Leather >= 3);")
# Saqlash
rep("ErtGameMode.cpp",
    "R->SetNumberField(TEXT(\"arrowTier\"), H->ArrowTier);",
    "R->SetNumberField(TEXT(\"arrowTier\"), H->ArrowTier); R->SetNumberField(TEXT(\"rawHide\"), H->RawHide); R->SetBoolField(TEXT(\"saddle\"), H->bSaddle); R->SetNumberField(TEXT(\"tribe\"), TribeScore);")
rep("ErtGameMode.cpp",
    "if (R->TryGetNumberField(TEXT(\"arrowTier\"), Dv)) H->ArrowTier = (int32)Dv; }",
    "if (R->TryGetNumberField(TEXT(\"arrowTier\"), Dv)) H->ArrowTier = (int32)Dv; if (R->TryGetNumberField(TEXT(\"rawHide\"), Dv)) H->RawHide = (int32)Dv; if (R->TryGetBoolField(TEXT(\"saddle\"), B)) H->bSaddle = B; if (R->TryGetNumberField(TEXT(\"tribe\"), Dv)) TribeScore = (int32)Dv; }")
# HUD inventar
rep("ErtHUD.cpp",
    "\tRow(TEXT(\"Temir / teri\"), FString::Printf(TEXT(\"%d / %d   (dushman o'ljasi / kiyik; Deli Demir temirxonasi - oba)\"), H->Iron, H->Leather));",
    "\tRow(TEXT(\"Temir / charm / xom teri\"), FString::Printf(TEXT(\"%d / %d / %d   (dushman o'ljasi / oshlangan / kiyik; Deli Demir - oba)\"), H->Iron, H->Leather, H->RawHide));\n"
    "\tif (AErtGameMode* GMq = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()))) Row(TEXT(\"Qabila\"), FString::Printf(TEXT(\"daraja %d (%d ball)  - Hayma Onaga hadya: oltin/go'sht/charm; +ittifoqchi, arzon narx\"), GMq->TribeLevel(), GMq->TribeScore));")
rep("ErtHUD.cpp",
    "\tRow(TEXT(\"Kamon\"), H->BowTier >= 2 ? TEXT(\"Kompozit kamon (+20 zarar, 24 o'q)\") : TEXT(\"Oddiy kamon\"));",
    "\tRow(TEXT(\"Kamon\"), H->BowTier >= 2 ? TEXT(\"Kompozit kamon (+20 zarar, 24 o'q)\") : TEXT(\"Oddiy kamon\"));\n\tRow(TEXT(\"Egar\"), H->bSaddle ? TEXT(\"Charm egar (ot +8% tez)\") : TEXT(\"Yo'q (3 charm + 1 temir)\"));")
rep("ErtHUD.cpp",
    "Damashq qilichi = 4 temir; qalqon = 2 temir + 1 teri\"), 44 * Sc, Y + 44 * Sc, Grey, 0.9f * Sc);",
    "Damashq qilichi = 4 temir; qalqon = 2 temir + 1 teri; oshlash 2 xom -> 3 charm; kamon 1 temir + 2 charm; egar 1 temir + 3 charm\"), 44 * Sc, Y + 44 * Sc, Grey, 0.9f * Sc);")

# ---------- 12) Qabila ----------
rep("ErtGameMode.h", "\tint32 Honor = 0;", "\tint32 Honor = 0;\n\tint32 TribeScore = 0;   // qabila boshqaruvi: hadyalar -> daraja 1..3 (ittifoqchi +, narx -5%/daraja)\n\tint32 TribeLevel() const { return TribeScore >= 120 ? 3 : (TribeScore >= 50 ? 2 : 1); }")
rep("ErtGameMode.cpp",
    "\t\tBuy(TEXT(\"buy_potion\"), 15, [&]() { H->Potions += 1; });",
    "\t\t// Qabila hadyalari (Hayma Ona): oltin 100 -> +10 ball, go'sht 5 -> +5, charm 3 -> +6; or +1\n"
    "\t\tauto Donate = [&](const TCHAR* Flag, TFunction<bool()> Take, int32 Pts, const TCHAR* Msg) { if (!Flags.Contains(Flag)) return; Flags.Remove(Flag); if (Take()) { const int32 L0 = TribeLevel(); TribeScore += Pts; AddHonor(1); ShopMsg = FString::Printf(TEXT(\"%s: qabila +%d ball (daraja %d)%s\"), Msg, Pts, TribeLevel(), TribeLevel() > L0 ? TEXT(\"  - DARAJA OSHDI!\") : TEXT(\"\")); ShopMsgT = 4.f; SaveGame(); } };\n"
    "\t\tDonate(TEXT(\"donate_gold\"), [&]() { if (H->Gold < 100) return false; H->AddGold(-100); return true; }, 10, TEXT(\"100 oltin hadya\"));\n"
    "\t\tDonate(TEXT(\"donate_meat\"), [&]() { if (H->Meat < 5) return false; H->Meat -= 5; return true; }, 5, TEXT(\"5 go'sht hadya\"));\n"
    "\t\tDonate(TEXT(\"donate_leather\"), [&]() { if (H->Leather < 3) return false; H->Leather -= 3; return true; }, 6, TEXT(\"3 charm hadya\"));\n"
    "\t\tBuy(TEXT(\"buy_potion\"), 15, [&]() { H->Potions += 1; });")
rep("ErtGameMode.cpp",
    "\t\t\tPrice = FMath::RoundToInt(Price * (Honor >= 10 ? 0.85f : (Honor <= -5 ? 1.3f : 1.f)));   // or/iymon narxga ta'sir qiladi",
    "\t\t\tPrice = FMath::RoundToInt(Price * (Honor >= 10 ? 0.85f : (Honor <= -5 ? 1.3f : 1.f)) * (1.f - 0.05f * (TribeLevel() - 1)));   // or/iymon va qabila darajasi narxga ta'sir qiladi")
rep("ErtMission.cpp",
    "\t\tconst int32 Want = MaxW >= 3 ? FMath::Clamp(MaxW + (bBig ? 3 : 0), 3, 8) : 0;",
    "\t\tint32 TribeBonus = 0; if (AErtGameMode* GMt = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) TribeBonus = GMt->TribeLevel() - 1;   // qabila darajasi: +1/+2 alp\n"
    "\t\tconst int32 Want = MaxW >= 3 ? FMath::Clamp(MaxW + (bBig ? 3 : 0) + TribeBonus, 3, 10) : 0;")

# ---------- 11) Ulukayin daraxti (voha, tush joyi) ----------
rep("ErtWorldBuilder.cpp",
    "void AErtWorldBuilder::BuildLandmarks()\n{\n\tFErtFabLib& Fab = FErtFabLib::Get();\n\tint32 N = 0;",
    "void AErtWorldBuilder::BuildLandmarks()\n{\n\tFErtFabLib& Fab = FErtFabLib::Get();\n\tint32 N = 0;\n"
    "\t// Ulukayin - hayot daraxti (voha, tush bosqichi joyi): ulkan tana, 6 shox, ko'k-oq nurli barg sharlari\n"
    "\t{\n"
    "\t\tconst float UE_ = OasisE + 40.f, UN_ = OasisN + 30.f, UZ = HeightAt(UE_, UN_);\n"
    "\t\tFErtMeshData M(100.f);\n"
    "\t\tconst FLinearColor Bark(0.30f, 0.22f, 0.16f), Leaf(0.62f, 0.82f, 1.0f), LeafD(0.40f, 0.62f, 0.95f);\n"
    "\t\tM.AddCylinder(W(UE_, UN_, UZ), 1.6f, 1.0f, 9.f, 14, Bark, false, FRotator::ZeroRotator, 0.08f, 7);\n"
    "\t\tfor (int32 i = 0; i < 6; ++i)\n\t\t{\n"
    "\t\t\tconst float A = i * 60.f + 20.f, Ar = FMath::DegreesToRadians(A);\n"
    "\t\t\tM.AddCone(W(UE_ + FMath::Cos(Ar) * 0.6f, UN_ + FMath::Sin(Ar) * 0.6f, UZ + 7.f + (i % 2) * 1.2f), 0.55f, 6.5f, 8, Bark, FRotator(-38.f, A, 0));\n"
    "\t\t\tM.AddSphere(W(UE_ + FMath::Cos(Ar) * 4.8f, UN_ + FMath::Sin(Ar) * 4.8f, UZ + 11.5f + (i % 2) * 1.2f), 3.2f, 10, (i % 2) ? Leaf : LeafD, FVector(1.f, 1.f, 0.8f), 0.18f, 11 + i);\n"
    "\t\t}\n"
    "\t\tM.AddSphere(W(UE_, UN_, UZ + 14.5f), 3.8f, 12, Leaf, FVector(1.f, 1.f, 0.85f), 0.15f, 5);\n"
    "\t\tfor (int32 r = 0; r < 8; ++r) { const float Ar = r * 0.785f; M.AddCone(W(UE_ + FMath::Cos(Ar) * 1.4f, UN_ + FMath::Sin(Ar) * 1.4f, UZ - 0.2f), 0.5f, 2.2f, 6, Bark, FRotator(-80.f, r * 45.f, 0)); }   // ildizlar\n"
    "\t\tM.Commit(NewPart(TEXT(\"Ulukayin\"), true), 0, true); ++N;\n"
    "\t}")

# ---------- 14) Skelet tana: qilich qinda (R), qalqon orqada/qo'lda ----------
rep("ErtHeroBody.h",
    "\tFVector SwordLoc = FVector::ZeroVector; FRotator SwordRot = FRotator::ZeroRotator; FName SwordSocket = TEXT(\"hand_r\");",
    "\tFVector SwordLoc = FVector::ZeroVector; FRotator SwordRot = FRotator::ZeroRotator; FName SwordSocket = TEXT(\"hand_r\");\n"
    "\t// Qin (chap son) va qalqon (orqa / chap qo'l): character.json sheath_socket/shield_socket bilan almashtiriladi\n"
    "\tFName SheathSocket = TEXT(\"thigh_l\"), ShieldBackSocket = TEXT(\"spine_03\"), ShieldHandSocket = TEXT(\"hand_l\");\n"
    "\tFVector SheathLoc = FVector(28.f, -6.f, 12.f); FRotator SheathRot = FRotator(0.f, 0.f, 95.f);\n"
    "\tbool bSheathed = false; UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> SkelShield;\n"
    "public:\n\tvoid SetSheathed(bool bOn); bool IsSheathed() const { return bSheathed; }\n\tvoid SkelBuildShield(bool bHas);\nprivate:")
rep("ErtHeroBody.cpp",
    "void UErtHeroBody::SkelBuildSword()\n{",
    "void UErtHeroBody::SetSheathed(bool bOn)\n{\n"
    "\tbSheathed = bOn;\n"
    "\tif (!Skel || !SkelSword) return;\n"
    "\tif (bOn) { SkelSword->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SheathSocket); SkelSword->SetRelativeLocation(SheathLoc); SkelSword->SetRelativeRotation(SheathRot); }\n"
    "\telse { SkelSword->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SwordSocket); SkelSword->SetRelativeLocation(SwordLoc); SkelSword->SetRelativeRotation(SwordRot); }\n"
    "}\n\n"
    "void UErtHeroBody::SkelBuildShield(bool bHas)\n{\n"
    "\tif (!Skel) return;\n"
    "\tif (!bHas) { if (SkelShield) { SkelShield->DestroyComponent(); SkelShield = nullptr; } return; }\n"
    "\tif (!SkelShield) { SkelShield = MakePart(TEXT(\"SkelShield\"), Skel, FVector::ZeroVector); }\n"
    "\tconst bool bHand = bBlock;\n"
    "\tSkelShield->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bHand ? ShieldHandSocket : ShieldBackSocket);\n"
    "\tSkelShield->SetRelativeLocation(bHand ? FVector(0, -6.f, 0) : FVector(-14.f, 0, 0)); SkelShield->SetRelativeRotation(bHand ? FRotator(0, 0, 0) : FRotator(0, 0, 90.f));\n"
    "\tif (SkelShield->GetNumSections() == 0)\n\t{\n"
    "\t\tFErtMeshData M;\n"
    "\t\tconst FLinearColor WoodS = ErtCol::Sty(FLinearColor(0.36f, 0.22f, 0.10f), ErtCol::StyleWood), IronS = ErtCol::Sty(FLinearColor(0.35f, 0.34f, 0.33f), ErtCol::StyleMetal);\n"
    "\t\tM.AddCylinder(FVector(0, 0, -1.5f), 30.f, 30.f, 3.f, 24, WoodS, true, FRotator(90.f, 0, 0));\n"
    "\t\tM.AddCylinder(FVector(0, 0, -2.f), 31.5f, 31.5f, 1.2f, 24, IronS, true, FRotator(90.f, 0, 0));\n"
    "\t\tM.AddSphere(FVector(0, 0, 1.5f), 6.f, 8, IronS, FVector(1, 1, 0.5f));\n"
    "\t\tM.Commit(SkelShield, 0, false);\n\t}\n"
    "}\n\n"
    "void UErtHeroBody::SkelBuildSword()\n{")
s = rd(os.path.join(SRC, "ErtHeroBody.cpp"))
if "sheath_socket" not in s:
    s = s.replace("\t\tFString Sock; if ((*Sw)->TryGetStringField(TEXT(\"socket\"), Sock)) SwordSocket = FName(*Sock);",
                  "\t\tFString Sock; if ((*Sw)->TryGetStringField(TEXT(\"socket\"), Sock)) SwordSocket = FName(*Sock);\n"
                  "\t\tFString Sh; if ((*Sw)->TryGetStringField(TEXT(\"sheath_socket\"), Sh)) SheathSocket = FName(*Sh); if ((*Sw)->TryGetStringField(TEXT(\"shield_back_socket\"), Sh)) ShieldBackSocket = FName(*Sh); if ((*Sw)->TryGetStringField(TEXT(\"shield_hand_socket\"), Sh)) ShieldHandSocket = FName(*Sh);", 1)
    wr(os.path.join(SRC, "ErtHeroBody.cpp"), s); print("  +  HeroBody sockets json")
# Blok holatida qalqon qo'lga: SetBlocking
rep("ErtHeroBody.h", "\tvoid SetBlocking(bool bOn) { bBlock = bOn; }", "\tvoid SetBlocking(bool bOn) { const bool bWas = bBlock; bBlock = bOn; if (Skel && SkelShield && bWas != bOn) SkelBuildShield(true); }")
# Personaj: R = qilich qinga / chiqarish; hujumda avtomatik chiqadi; ApplyEquipment qalqon
rep("ErtCharacter.h", "\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Whistle;", "\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Whistle;\n\tUPROPERTY(Transient) TObjectPtr<UInputAction> IA_Sheathe;\n\tvoid OnSheathe();")
rep("ErtCharacter.cpp", "\tIA_Whistle = MakeAction(TEXT(\"IA_ErtWhistle\"), EInputActionValueType::Boolean);", "\tIA_Whistle = MakeAction(TEXT(\"IA_ErtWhistle\"), EInputActionValueType::Boolean);\n\tIA_Sheathe = MakeAction(TEXT(\"IA_ErtSheathe\"), EInputActionValueType::Boolean);")
rep("ErtCharacter.cpp", "\tMap(IA_Whistle, EKeys::Z); Map(IA_Whistle, EKeys::Gamepad_DPad_Down);", "\tMap(IA_Whistle, EKeys::Z); Map(IA_Whistle, EKeys::Gamepad_DPad_Down);\n\tMap(IA_Sheathe, EKeys::R); Map(IA_Sheathe, EKeys::Gamepad_DPad_Left);")
rep("ErtCharacter.cpp", "if (N == TEXT(\"Whistle\")) return IA_Whistle;", "if (N == TEXT(\"Whistle\")) return IA_Whistle; if (N == TEXT(\"Sheathe\")) return IA_Sheathe;")
rep("ErtCharacter.cpp", "\tEIC->BindAction(IA_Whistle, ETriggerEvent::Started, this, &AErtCharacter::OnWhistle);", "\tEIC->BindAction(IA_Whistle, ETriggerEvent::Started, this, &AErtCharacter::OnWhistle);\n\tEIC->BindAction(IA_Sheathe, ETriggerEvent::Started, this, &AErtCharacter::OnSheathe);")
rep("ErtCharacter.cpp", "void AErtCharacter::OnPotion()\n{",
    "void AErtCharacter::OnSheathe()\n{\n\tif (!bInputEnabled || bDead || !Body || !Body->IsSkeletal()) return;\n\tBody->SetSheathed(!Body->IsSheathed());\n"
    "\tif (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->ShopMsg = Body->IsSheathed() ? TEXT(\"Qilich qinga solindi (R: chiqarish)\") : TEXT(\"Qilich chiqarildi\"); GM->ShopMsgT = 2.f; }\n}\n\n"
    "void AErtCharacter::OnPotion()\n{")
# Hujumda avtomatik chiqarish
rep("ErtCharacter.cpp", "void AErtCharacter::OnAttack()\n{\n\tif (!bInputEnabled || bMantling || bDead || AttackCD > 0.f || Boat) return;",
    "void AErtCharacter::OnAttack()\n{\n\tif (!bInputEnabled || bMantling || bDead || AttackCD > 0.f || Boat) return;\n\tif (Body && Body->IsSkeletal() && Body->IsSheathed()) Body->SetSheathed(false);")
# ApplyEquipment: qalqon skelet tanada
rep("ErtCharacter.cpp", "void AErtCharacter::ApplyEquipment()\n{", "void AErtCharacter::ApplyEquipment()\n{\n\tif (Body && Body->IsSkeletal()) Body->SkelBuildShield(bShield);")

# ---------- Dialoglar ----------
dp = os.path.join(DATA, "dialogue", "npc_deli_demir.json"); d = json.load(io.open(dp, encoding="utf-8"))
opts = d["nodes"]["craft"]["options"]
if not any(o.get("set_flag") == "craft_tan" for o in opts):
    back = opts.pop()
    opts += [{"text_key": "DLG_CRAFT_TAN", "next": "craft_done", "set_flag": "craft_tan", "requires_evidence": "can_craft_tan"},
             {"text_key": "DLG_CRAFT_BOW", "next": "craft_done", "set_flag": "craft_bow", "requires_evidence": "can_craft_bow"},
             {"text_key": "DLG_CRAFT_SADDLE", "next": "craft_done", "set_flag": "craft_saddle", "requires_evidence": "can_craft_saddle"}, back]
    io.open(dp, "w", encoding="utf-8", newline="\n").write(json.dumps(d, ensure_ascii=False, indent=1) + "\n"); print("  +  deli_demir craft")
hp = os.path.join(DATA, "dialogue", "npc_hayme_ana.json"); d = json.load(io.open(hp, encoding="utf-8"))
b = d["nodes"]["b"]
if not any(o.get("set_flag") == "donate_gold" for o in b["options"]):
    b["options"] += [{"text_key": "DLG_TRIBE_GOLD", "next": "t", "set_flag": "donate_gold", "requires_evidence": "can_donate_gold"},
                     {"text_key": "DLG_TRIBE_MEAT", "next": "t", "set_flag": "donate_meat", "requires_evidence": "can_donate_meat"},
                     {"text_key": "DLG_TRIBE_LEATHER", "next": "t", "set_flag": "donate_leather", "requires_evidence": "can_donate_leather"}]
    d["nodes"]["t"] = {"speaker": "Hayma Ona", "text_key": "DLG_TRIBE_THANKS", "next": "end"}
    io.open(hp, "w", encoding="utf-8", newline="\n").write(json.dumps(d, ensure_ascii=False, indent=1) + "\n"); print("  +  hayme donate")
loc = os.path.join(DATA, "ertugrul_loc.csv"); s = rd(loc); have = set(l.split(",", 1)[0] for l in s.splitlines())
rows = [("DLG_CRAFT_TAN", "Teri oshlash (2 xom teri -> 3 charm)", "Deri tabaklama (2 ham deri -> 3 deri)", "Tan hides (2 raw hides -> 3 leather)"),
        ("DLG_CRAFT_BOW", "Kompozit kamon (1 temir + 2 charm)", "Kompozit yay (1 demir + 2 deri)", "Composite bow (1 iron + 2 leather)"),
        ("DLG_CRAFT_SADDLE", "Egar (1 temir + 3 charm) - ot +8% tez", "Eyer (1 demir + 3 deri) - at +%8 hizli", "Saddle (1 iron + 3 leather) - horse +8% speed"),
        ("DLG_TRIBE_GOLD", "Obaga 100 oltin hadya qilaman", "Obaya 100 altin bagisliyorum", "I donate 100 gold to the tribe"),
        ("DLG_TRIBE_MEAT", "Obaga 5 go'sht hadya qilaman", "Obaya 5 et bagisliyorum", "I donate 5 meat to the tribe"),
        ("DLG_TRIBE_LEATHER", "Obaga 3 charm hadya qilaman", "Obaya 3 deri bagisliyorum", "I donate 3 leather to the tribe"),
        ("DLG_TRIBE_THANKS", "Alloh rozi bo'lsin, o'g'lim. Oba kuchayadi, alplar ko'payadi.", "Allah razi olsun oglum. Oba guclenir, alpler cogalir.", "May God be pleased with you, my son. The tribe grows stronger, the alps more numerous.")]
add = ['%s,"%s","%s","%s"' % r for r in rows if r[0] not in have]
if add:
    if not s.endswith("\n"): s += "\n"
    wr(loc, s + "\n".join(add) + "\n"); print("  +  loc", len(add))
print("OK")
