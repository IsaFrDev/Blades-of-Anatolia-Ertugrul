# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtHeroBody.h')
h = rep(h, "\tUPROPERTY(EditAnywhere, Category = \"Ertugrul|Ranglar\") bool bSwordInHand = false;",
        '''\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Ranglar") bool bSwordInHand = false;
\t// Kiyim detallari (dushman/NPC): salla, lamellar zirh, yelka himoyasi, plash, sadoq, orqadagi qalqon, etik, zanjir ko'ylak
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bTurban = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bLamellar = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bPauldrons = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bCloak = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bQuiver = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bBackShield = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bBoots = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") bool bMail = false;
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") FLinearColor Cloak = FLinearColor(0.35f, 0.08f, 0.07f);
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Kiyim") FLinearColor Turban = FLinearColor(0.9f, 0.88f, 0.8f);''')
save('ErtHeroBody.h', h)

c = load('ErtHeroBody.cpp')
# Ko'krak: zanjir ko'ylak, lamellar qatorlar, yelka himoyasi, plash, sadoq, orqadagi qalqon
c = rep(c, "\tM.AddBox(FVector(-13.5f, 0, 24), FVector(1.5f, 17, 22), FurS);\n\tM.Commit(Torso, 0, false);",
        '''\tM.AddBox(FVector(-13.5f, 0, 24), FVector(1.5f, 17, 22), FurS);
\tconst FLinearColor CloakS = ErtCol::Sty(Cloak, ErtCol::StyleCloth), MailS = ErtCol::Sty(Steel * 0.8f, ErtCol::StyleMetal), PlateS = ErtCol::Sty(Steel * 0.92f, ErtCol::StyleMetal);
\tif (bMail) { M.AddBox(FVector(0, 0, 22), FVector(13.4f, 19.4f, 21), MailS); M.AddBox(FVector(0, 0, 44), FVector(11, 17, 3), MailS); }
\tif (bLamellar)
\t{
\t\tfor (int32 r = 0; r < 5; ++r) for (int32 cc = -2; cc <= 2; ++cc)
\t\t{
\t\t\tM.AddBox(FVector(13.6f, cc * 6.2f, 6 + r * 7.2f), FVector(0.7f, 2.8f, 3.4f), ErtCol::Vary(PlateS, 0.06f, r * 5 + cc), FRotator(-6, 0, 0));    // oldingi qatorlar
\t\t\tM.AddBox(FVector(-13.6f, cc * 6.2f, 6 + r * 7.2f), FVector(0.7f, 2.8f, 3.4f), ErtCol::Vary(PlateS, 0.06f, 30 + r * 5 + cc), FRotator(6, 0, 0));  // orqa qatorlar
\t\t}
\t\tfor (int32 r = 0; r < 5; ++r) M.AddBox(FVector(0, 0, 6 + r * 7.2f), FVector(13.9f, 19.6f, 0.5f), LeatherS);   // bog'lovchi tasmalar
\t}
\tif (bPauldrons)
\t{
\t\tfor (int32 s = -1; s <= 1; s += 2)
\t\t{
\t\t\tM.AddSphere(FVector(0, s * 22.f, 42), 8.f, 10, PlateS, FVector(1.0f, 1.1f, 0.65f));
\t\t\tM.AddBox(FVector(0, s * 24.f, 37), FVector(6.5f, 3.5f, 2.f), ErtCol::Vary(PlateS, 0.05f, 7 + s));
\t\t\tM.AddBox(FVector(0, s * 26.f, 33), FVector(6.f, 3.f, 1.8f), ErtCol::Vary(PlateS, 0.05f, 9 + s));
\t\t\tM.AddSphere(FVector(0, s * 22.f, 47), 1.6f, 6, TrimS);
\t\t}
\t}
\tif (bCloak)
\t{
\t\tM.AddBox(FVector(-16.5f, 0, 12), FVector(1.2f, 18, 36), CloakS, FRotator(-5, 0, 0));
\t\tM.AddBox(FVector(-15.5f, 0, 45), FVector(3.f, 16, 3.f), CloakS);
\t\tM.AddBox(FVector(-16.f, 0, -18), FVector(1.5f, 19, 8), ErtCol::Sty(Cloak * 0.85f, ErtCol::StyleCloth), FRotator(-8, 0, 0));
\t\tM.AddSphere(FVector(6, -16, 44), 1.8f, 6, TrimS); M.AddSphere(FVector(6, 16, 44), 1.8f, 6, TrimS);   // to'g'nog'ichlar
\t}
\tif (bQuiver)
\t{
\t\tM.AddCylinder(FVector(-14, 8, 8), 3.6f, 3.2f, 34, 8, LeatherS, true, FRotator(-18, 0, 12));
\t\tM.AddCylinder(FVector(-13, 8, 8), 3.8f, 3.8f, 2, 8, TrimS, true, FRotator(-18, 0, 12));
\t\tfor (int32 i = 0; i < 6; ++i) M.AddBox(FVector(-20 + (i % 3) * 1.6f, 6 + (i / 3) * 2.5f, 44), FVector(0.4f, 0.4f, 4), ErtCol::Sty(FLinearColor(0.85f, 0.85f, 0.8f), ErtCol::StylePlain), FRotator(-18, 0, 12));
\t\tM.AddBox(FVector(0, 0, 30), FVector(13.3f, 2.f, 1.2f), LeatherS, FRotator(0, 0, -30));   // sadoq tasmasi
\t}
\tif (bBackShield)
\t{
\t\tM.AddCylinder(FVector(-16.5f, 0, 24), 20.f, 20.f, 2.5f, 12, ErtCol::Sty(FLinearColor(0.35f, 0.22f, 0.10f), ErtCol::StyleWood), true, FRotator(0, 0, 90.f));
\t\tM.AddCylinder(FVector(-19.f, 0, 24), 6.f, 5.f, 3.f, 8, PlateS, true, FRotator(0, 0, 90.f));
\t\tM.AddCylinder(FVector(-19.f, 0, 24), 20.5f, 20.5f, 1.f, 12, TrimS, false, FRotator(0, 0, 90.f));
\t}
\tM.Commit(Torso, 0, false);''')
# Bosh: salla (dubulg'a va bo'rk o'rniga, erkak uchun)
c = rep(c, "\telse if (bHelmet)\n\t{\n\t\tM.AddSphere(FVector(0, 0, 20), 12.5f, 12, SteelS, FVector(1, 1, 0.9f));",
        '''\telse if (bTurban)
\t{
\t\tconst FLinearColor TurbS = ErtCol::Sty(Turban, ErtCol::StyleCloth);
\t\tM.AddCylinder(FVector(0, 0, 24), 12.6f, 12.6f, 4.f, 12, ErtCol::Vary(TurbS, 0.05f, 11), true, FRotator(0, 0, 6.f));
\t\tM.AddCylinder(FVector(0, 0, 27.5f), 13.2f, 12.4f, 4.f, 12, ErtCol::Vary(TurbS, 0.05f, 12), true, FRotator(5.f, 40.f, -6.f));
\t\tM.AddCylinder(FVector(0, 0, 31), 12.2f, 10.5f, 3.5f, 12, ErtCol::Vary(TurbS, 0.05f, 13), true, FRotator(-4.f, 80.f, 5.f));
\t\tM.AddSphere(FVector(0, 0, 34), 9.5f, 10, ErtCol::Sty(Kaftan * 0.9f, ErtCol::StyleCloth), FVector(1, 1, 0.55f));
\t\tM.AddBox(FVector(10.5f, 0, 30), FVector(2.5f, 3.5f, 2.f), TrimS);   // salla bezagi
\t}
\telse if (bHelmet)
\t{
\t\tM.AddSphere(FVector(0, 0, 20), 12.5f, 12, SteelS, FVector(1, 1, 0.9f));''')
# Oyoqlar: baland etik
c = rep(c, "\t\tM.AddBox(FVector(0, 0, -30), FVector(6.5f, 6.5f, 12), LeatherS);\n\t\tM.AddBox(FVector(6, 0, -43), FVector(13, 6, 3.5f), LeatherS);\n\t\tM.Commit(Side ? ShinR : ShinL, 0, false);",
        '''\t\tif (bBoots)
\t\t{
\t\t\tM.AddBox(FVector(0, 0, -25), FVector(6.7f, 6.7f, 17), ErtCol::Sty(Leather * 0.9f, ErtCol::StyleLeather));
\t\t\tM.AddBox(FVector(0, 0, -9), FVector(7.0f, 7.0f, 1.5f), ErtCol::Sty(Leather * 0.7f, ErtCol::StyleLeather));   // etik og'zi
\t\t\tM.AddBox(FVector(0, 0, -30), FVector(7.1f, 7.1f, 1.f), TrimS);                                                // to'qa tasma
\t\t}
\t\telse M.AddBox(FVector(0, 0, -30), FVector(6.5f, 6.5f, 12), LeatherS);
\t\tM.AddBox(FVector(6, 0, -43), FVector(13, 6, 3.5f), LeatherS);
\t\tM.Commit(Side ? ShinR : ShinL, 0, false);''')
save('ErtHeroBody.cpp', c)

# Dushmanlar
e = load('ErtEnemy.cpp')
e = rep(e, "\t\tBody->Kaftan = FLinearColor(0.12f, 0.22f, 0.12f); Body->Trim = FLinearColor(0.5f, 0.5f, 0.45f); Body->bHelmet = true; break;",
        "\t\tBody->Kaftan = FLinearColor(0.12f, 0.22f, 0.12f); Body->Trim = FLinearColor(0.5f, 0.5f, 0.45f); Body->bHelmet = true; Body->bBoots = true; Body->bBackShield = true; break;")
e = rep(e, "\t\tBody->Kaftan = FLinearColor(0.10f, 0.13f, 0.32f); Body->Leather = FLinearColor(0.5f, 0.5f, 0.52f); Body->bHelmet = true; break;",
        "\t\tBody->Kaftan = FLinearColor(0.10f, 0.13f, 0.32f); Body->Leather = FLinearColor(0.5f, 0.5f, 0.52f); Body->bHelmet = true; Body->bLamellar = true; Body->bPauldrons = true; Body->bBoots = true; break;")
e = rep(e, "\t\tBody->Kaftan = FLinearColor(0.30f, 0.22f, 0.12f); Body->bHelmet = false; Body->Fur = FLinearColor(0.35f, 0.3f, 0.25f); break;",
        "\t\tBody->Kaftan = FLinearColor(0.30f, 0.22f, 0.12f); Body->bHelmet = false; Body->Fur = FLinearColor(0.35f, 0.3f, 0.25f); Body->bQuiver = true; Body->bBoots = true; break;")
e = rep(e, "Body->Trim = FLinearColor(0.9f, 0.75f, 0.2f); Body->bHelmet = true; break;",
        "Body->Trim = FLinearColor(0.9f, 0.75f, 0.2f); Body->bHelmet = true; Body->bLamellar = true; Body->bPauldrons = true; Body->bCloak = true; Body->Cloak = FLinearColor(0.4f, 0.06f, 0.05f); Body->bBoots = true; break;")
e = rep(e, "Body->Fur = FLinearColor(0.2f, 0.18f, 0.16f); Body->bHelmet = true; break;",
        "Body->Fur = FLinearColor(0.2f, 0.18f, 0.16f); Body->bHelmet = true; Body->bLamellar = true; Body->bPauldrons = true; Body->bCloak = true; Body->Cloak = FLinearColor(0.08f, 0.07f, 0.09f); Body->bBoots = true; Body->bMail = true; break;")
e = rep(e, "\t\tBody->Kaftan = FLinearColor(0.38f, 0.10f, 0.08f); Body->Leather = FLinearColor(0.35f, 0.33f, 0.30f); Body->bHelmet = true; break;",
        "\t\tBody->Kaftan = FLinearColor(0.38f, 0.10f, 0.08f); Body->Leather = FLinearColor(0.35f, 0.33f, 0.30f); Body->bHelmet = true; Body->bMail = true; Body->bCloak = true; Body->Cloak = FLinearColor(0.25f, 0.2f, 0.12f); Body->bBoots = true; break;")
save('ErtEnemy.cpp', e)

# NPClar: erkaklarga salla (55%), beklarga plash, savdogar/qozi/imom/mudarris sallali
n = load('ErtNpc.cpp')
n = rep(n, "\tBody->Beard = RS.FRand() < 0.35f ? FLinearColor(0.7f, 0.68f, 0.64f) : FLinearColor(0.16f, 0.11f, 0.06f);\n\tBody->Build(RootComponent, 92.f);",
        '''\tBody->Beard = RS.FRand() < 0.35f ? FLinearColor(0.7f, 0.68f, 0.64f) : FLinearColor(0.16f, 0.11f, 0.06f);
\tif (!bWoman)
\t{
\t\tconst bool bScholar = InId.Contains(TEXT("imom")) || InId.Contains(TEXT("mudarris")) || InId.Contains(TEXT("qozi")) || InId.Contains(TEXT("arabi")) || InId.Contains(TEXT("savdogar")) || InId.Contains(TEXT("bozorchi"));
\t\tconst bool bBey = InId.Contains(TEXT("bey")) || InId.Contains(TEXT("hokim")) || InId.Contains(TEXT("tekfur")) || InId.Contains(TEXT("kopek")) || InId.Contains(TEXT("beyi"));
\t\tBody->bTurban = bScholar || (!bBey && RS.FRand() < 0.5f);
\t\tBody->Turban = bScholar ? FLinearColor(0.92f, 0.9f, 0.84f) : FLinearColor(0.3f + RS.FRand() * 0.5f, 0.2f + RS.FRand() * 0.4f, 0.15f + RS.FRand() * 0.4f);
\t\tBody->bCloak = bBey || RS.FRand() < 0.2f;
\t\tBody->Cloak = bBey ? FLinearColor(0.35f, 0.08f, 0.07f) : FLinearColor(0.25f + RS.FRand() * 0.2f, 0.18f + RS.FRand() * 0.15f, 0.1f + RS.FRand() * 0.1f);
\t\tBody->bBoots = bBey || RS.FRand() < 0.4f;
\t\tif (bBey) { Body->bPauldrons = InId.Contains(TEXT("tekfur")); Body->Trim = FLinearColor(0.9f, 0.75f, 0.25f); }
\t}
\tBody->Build(RootComponent, 92.f);''')
save('ErtNpc.cpp', n)

# Sinov: dushman yaqinidan kadr
ch = load('ErtCharacter.cpp')
if 'TakeShot(TEXT("enemy"))' not in ch:
    ch = rep(ch, '\tif (At(52.8f)) TakeShot(TEXT("storm"));\n\tif (At(53.3f))',
        '''\tif (At(52.8f)) TakeShot(TEXT("storm"));
\tif (At(52.9f))
\t{
\t\tif (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->SetWeather(TEXT("clear"));
\t\tTArray<AActor*> Es; UGameplayStatics::GetAllActorsOfClass(this, AErtEnemy::StaticClass(), Es);
\t\tAErtEnemy* Best = nullptr;
\t\tfor (AActor* A : Es) { AErtEnemy* En = Cast<AErtEnemy>(A); if (En && !En->IsDead() && En->GetKind() != EErtEnemyKind::Deer) { Best = En; break; } }
\t\tif (Best) { const FVector L = Best->GetActorLocation(), F = Best->GetActorForwardVector(); SetActorLocation(L + F * 320.f + FVector(0, 0, 10.f), false, nullptr, ETeleportType::TeleportPhysics); if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-6.f, (-F).Rotation().Yaw, 0.f)); TargetArm = 260.f; }
\t}
\tif (At(54.0f)) TakeShot(TEXT("enemy"));
\tif (At(54.5f))''')
save('ErtCharacter.cpp', ch)
print('patched')
