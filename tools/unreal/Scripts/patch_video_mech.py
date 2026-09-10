# -*- coding: utf-8 -*-
# Video mexanikalari: ot bilan bosib o'tish (trample), orqadan yashirin takedown, dinamik plash (4 bo'g'inli), baland devorga tirmashish
import io, os, sys
SRC = r"D:\Unreal_projects\Ertugrul\Source\Ertugrul"
def rd(p): return io.open(p, encoding="utf-8").read()
def wr(p, s): io.open(p, "w", encoding="utf-8", newline="\n").write(s)
def rep(path, old, new):
    p = os.path.join(SRC, path); s = rd(p)
    if new in s: print("  = allaqachon:", path); return
    if old not in s: print("!! topilmadi:", path, old[:70]); sys.exit(1)
    wr(p, s.replace(old, new, 1)); print("  + ", path)

# ---------- 1) Ot bilan bosib o'tish ----------
rep("ErtHorse.h", "\tbool bSaddled = false;   // egar (o'yinchi hunarmandchiligi)",
    "\tbool bSaddled = false;   // egar (o'yinchi hunarmandchiligi)\n\tfloat TrampleT = 0.f;   // chopib dushmanni bosib o'tish tekshiruvi (0.12 s)")
rep("ErtHorse.cpp", "#include \"ErtCharacter.h\"", "#include \"ErtCharacter.h\"\n#include \"ErtEnemy.h\"\n#include \"ErtFx.h\"\n#include \"ErtAudio.h\"\n#include \"Kismet/GameplayStatics.h\"")
rep("ErtHorse.cpp", "\tCare = FMath::Max(0.f, Care - Dt / 900.f); CareFxT = FMath::Max(0.f, CareFxT - Dt);",
    "\tCare = FMath::Max(0.f, Care - Dt / 900.f); CareFxT = FMath::Max(0.f, CareFxT - Dt);\n"
    "\t// Chopib bosib o'tish: chavandoz bilan 550+ sm/s da oldindagi dushmanlar zarar oladi va uloqtiriladi\n"
    "\tTrampleT -= Dt;\n"
    "\tif (Rider && CurSpeed > 550.f && TrampleT <= 0.f)\n\t{\n"
    "\t\tTrampleT = 0.12f;\n"
    "\t\tconst FVector Ahead = GetActorLocation() + GetActorForwardVector() * 150.f;\n"
    "\t\tTArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtEnemy::StaticClass(), All);\n"
    "\t\tfor (AActor* A : All)\n\t\t{\n"
    "\t\t\tAErtEnemy* E = Cast<AErtEnemy>(A); if (!E || E->IsDead() || E->IsAnimal()) continue;\n"
    "\t\t\tif (AErtCharacter* Rc = Cast<AErtCharacter>(Rider)) if (E->Team == 1) continue;   // ittifoqchilarni bosmaydi\n"
    "\t\t\tif (FVector::Dist2D(E->GetActorLocation(), Ahead) > 170.f) continue;\n"
    "\t\t\tconst float Dmg = 18.f + CurSpeed * 0.03f;\n"
    "\t\t\tE->ApplyHit(Dmg, Rider, true);\n"
    "\t\t\tif (!E->IsDead()) E->LaunchCharacter(((E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * 0.7f + GetActorForwardVector() * 0.5f) * 700.f + FVector(0, 0, 320.f), true, true);\n"
    "\t\t\tAErtBurst::Blood(GetWorld(), E->GetActorLocation() + FVector(0, 0, 60.f), GetActorForwardVector() + FVector(0, 0, 0.4f), 1.2f);\n"
    "\t\t\tFErtAudio::PlaySfx(GetWorld(), TEXT(\"hit\"), E->GetActorLocation(), 0.9f, 0.7f);\n"
    "\t\t\tCurSpeed *= 0.85f;   // urilishdan sekinlashadi\n"
    "\t\t}\n\t}")

# ---------- 2) Orqadan yashirin takedown ----------
rep("ErtCharacter.cpp",
    "\t\t\tconst bool bExecute = Kind != 3 && ((E->IsStaggered() && !E->IsBoss()) || E->GetHealth() < E->GetMaxHealth() * E->ExecuteThreshold());",
    "\t\t\t// Yashirin zarba: dushman sezmagan va orqasidan (oldinga yo'nalishi bizga qarama-qarshi) bo'lsa - bir zarbada\n"
    "\t\t\tconst FVector ToMe = (GetActorLocation() - E->GetActorLocation()).GetSafeNormal2D();\n"
    "\t\t\tconst bool bStealthKill = Kind != 3 && !E->IsBoss() && !E->IsAlerted() && FVector::DotProduct(E->GetActorForwardVector(), ToMe) < -0.25f && FVector::Dist2D(E->GetActorLocation(), GetActorLocation()) < 210.f;\n"
    "\t\t\tif (bStealthKill) { StealthKills++; if (AErtGameMode* GMs = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GMs->ShopMsg = TEXT(\"Yashirin zarba!\"); GMs->ShopMsgT = 1.5f; } }\n"
    "\t\t\tconst bool bExecute = bStealthKill || (Kind != 3 && ((E->IsStaggered() && !E->IsBoss()) || E->GetHealth() < E->GetMaxHealth() * E->ExecuteThreshold()));")
rep("ErtCharacter.h", "\tint32 Meat = 0, Iron = 0, Leather = 0;", "\tint32 StealthKills = 0;   // orqadan yashirin takedownlar\n\tint32 Meat = 0, Iron = 0, Leather = 0;")

# ---------- 3) Dinamik plash: 4 bo'g'in, tezlik/tezlanish/shamol bilan tebranadi ----------
rep("ErtHeroBody.h", "\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Torso;",
    "\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Torso;\n"
    "\tUPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> CloakSegs;   // dinamik plash bo'g'inlari (yelkadan pastga)\n"
    "\tTArray<float> CloakPitch; float CloakPrevSpeed = 0.f;")
rep("ErtHeroBody.cpp",
    "\tif (bCloak)\n\t{\n\t\tM.AddBox(FVector(-16.5f, 0, 12), FVector(1.2f, 18, 36), CloakS, FRotator(-5, 0, 0));\n\t\tM.AddBox(FVector(-15.5f, 0, 45), FVector(3.f, 16, 3.f), CloakS);\n\t\tM.AddBox(FVector(-16.f, 0, -18), FVector(1.5f, 19, 8), ErtCol::Sty(Cloak * 0.85f, ErtCol::StyleCloth), FRotator(-8, 0, 0));\n\t\tM.AddSphere(FVector(6, -16, 44), 1.8f, 6, TrimS); M.AddSphere(FVector(6, 16, 44), 1.8f, 6, TrimS);   // to'g'nog'ichlar\n\t}",
    "\tif (bCloak)\n\t{\n"
    "\t\tM.AddBox(FVector(-15.5f, 0, 45), FVector(3.f, 16, 3.f), CloakS);   // yelka bandi\n"
    "\t\tM.AddSphere(FVector(6, -16, 44), 1.8f, 6, TrimS); M.AddSphere(FVector(6, 16, 44), 1.8f, 6, TrimS);   // to'g'nog'ichlar\n"
    "\t\t// Dinamik plash: 4 ta bo'g'in zanjiri (har biri oldingisiga bog'langan, Animate da tebranadi)\n"
    "\t\tfor (UProceduralMeshComponent* C : CloakSegs) if (C) C->DestroyComponent();\n"
    "\t\tCloakSegs.Reset(); CloakPitch.Reset();\n"
    "\t\tUSceneComponent* Par = Torso; FVector Base(-16.f, 0, 44.f);\n"
    "\t\tfor (int32 i = 0; i < 4; ++i)\n\t\t{\n"
    "\t\t\tUProceduralMeshComponent* Seg = MakePart(*FString::Printf(TEXT(\"Cloak%d\"), i), Par, Base);\n"
    "\t\t\tFErtMeshData Cm; const float Wd = 18.f + i * 1.5f, Ln = 17.f;\n"
    "\t\t\tCm.AddBox(FVector(0, 0, -Ln * 0.5f), FVector(1.1f, Wd, Ln * 0.5f), i == 3 ? ErtCol::Sty(Cloak * 0.85f, ErtCol::StyleCloth) : CloakS, FRotator::ZeroRotator);\n"
    "\t\t\tCm.Commit(Seg, 0, false);\n"
    "\t\t\tCloakSegs.Add(Seg); CloakPitch.Add(0.f); Par = Seg; Base = FVector(0, 0, -Ln);\n"
    "\t\t}\n\t}")
rep("ErtHeroBody.cpp",
    "\tif (!IsBuilt() || bDead) return;\n\tif (Skel) { SkelAnimate(Dt, Speed, bInAir, bCrouched); return; }",
    "\tif (!IsBuilt() || bDead) return;\n"
    "\t// Dinamik plash: tezlik orqaga ko'taradi, tezlanish/tormoz silkitadi, shamol va yurish ritmi tebratadi; bo'g'inlar kechikib ergashadi\n"
    "\tif (CloakSegs.Num())\n\t{\n"
    "\t\tconst float Acc = FMath::Clamp((Speed - CloakPrevSpeed) / FMath::Max(Dt, 0.001f), -3000.f, 3000.f); CloakPrevSpeed = Speed;\n"
    "\t\tconst float Tm = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;\n"
    "\t\tfloat Target = -8.f - FMath::Clamp(Speed / 600.f, 0.f, 1.3f) * 55.f - Acc * 0.006f + (bInAir ? -25.f : 0.f) + FMath::Sin(Tm * 2.3f) * 3.f + FMath::Sin(Tm * 5.1f + 1.f) * 1.5f * (0.3f + Speed / 400.f);\n"
    "\t\tfor (int32 i = 0; i < CloakSegs.Num(); ++i)\n\t\t{\n"
    "\t\t\tconst float Want = (i == 0) ? Target : CloakPitch[i - 1] * 0.55f + FMath::Sin(Tm * 3.7f + i * 1.3f) * (2.f + Speed / 150.f);\n"
    "\t\t\tCloakPitch[i] = FMath::FInterpTo(CloakPitch[i], Want, Dt, 9.f - i * 1.5f);\n"
    "\t\t\tif (CloakSegs[i]) CloakSegs[i]->SetRelativeRotation(FRotator(CloakPitch[i], 0, FMath::Sin(Tm * 1.9f + i) * (1.f + Speed / 300.f)));\n"
    "\t\t}\n\t}\n"
    "\tif (Skel) { SkelAnimate(Dt, Speed, bInAir, bCrouched); return; }")

# ---------- 4) Baland devorga tirmashish ----------
rep("ErtCharacter.h", "float MantleMaxHeight = 185.f;", "float MantleMaxHeight = 265.f;   // baland devor/to'siqlar (parkour)")
print("OK")
