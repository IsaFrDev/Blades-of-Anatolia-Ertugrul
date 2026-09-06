# -*- coding: utf-8 -*-
# 6) Oba hayoti: vazifali NPClar (suv tashish, o't o'rish, bolalar), qozon-ochoq, kigiz gilamlar
# 7) Suv: daryo oqimi (material)   8) Ot nomi + parvarish HUD   9) Kat-sahna kamerasi: Catmull-Rom + intro uchish
import io, os, sys, json, glob
SRC = r"D:\Unreal_projects\Ertugrul\Source\Ertugrul"
def rd(p): return io.open(p, encoding="utf-8").read()
def wr(p, s): io.open(p, "w", encoding="utf-8", newline="\n").write(s)
def rep(path, old, new, root=SRC):
    p = os.path.join(root, path); s = rd(p)
    if new in s: print("  = allaqachon:", path); return
    if old not in s: print("!! topilmadi:", path, old[:70]); sys.exit(1)
    wr(p, s.replace(old, new, 1)); print("  + ", path)

# ---------- 6a) NPC vazifalari ----------
rep("ErtNpc.h", "\tFVector HomePos = FVector::ZeroVector, Target = FVector::ZeroVector;",
    "\tFVector HomePos = FVector::ZeroVector, Target = FVector::ZeroVector;\n"
    "\t// Kundalik vazifa: 1 suv tashish (quduq <-> o'tov), 2 o't o'rish (egilib turadi), 3 bola (yugurib o'ynaydi)\n"
    "\tint32 Chore = 0; FVector ChoreA = FVector::ZeroVector, ChoreB = FVector::ZeroVector; float ChoreT = 0.f; bool bAtB = false, bBend = false;\n"
    "public:\n\tvoid SetChore(int32 Kind, const FVector& A, const FVector& B) { Chore = Kind; ChoreA = A; ChoreB = B; ChoreT = FMath::FRandRange(0.f, 3.f); if (Kind == 3) SetActorScale3D(FVector(0.62f)); }\nprivate:")
rep("ErtNpc.cpp",
    "\tif (!bTalking && DPl > 350.f)\n\t{\n\t\tWanderT -= Dt;",
    "\tif (Chore && !bTalking)\n\t{\n"
    "\t\tChoreT -= Dt; const FVector Goal = bAtB ? ChoreB : ChoreA; FVector To = Goal - GetActorLocation(); To.Z = 0;\n"
    "\t\tif (To.Size() > 60.f)\n\t\t{\n"
    "\t\t\tSpeed = Chore == 3 ? 230.f : 105.f; bBend = false;\n"
    "\t\t\tFVector NewPos = GetActorLocation() + To.GetSafeNormal() * Speed * Dt;\n"
    "\t\t\tFHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtNpcChore), true, this);\n"
    "\t\t\tif (GetWorld()->LineTraceSingleByChannel(Hit, NewPos + FVector(0, 0, 300), NewPos - FVector(0, 0, 300), ECC_Visibility, Q)) NewPos.Z = Hit.ImpactPoint.Z + 92.f * GetActorScale3D().Z;\n"
    "\t\t\tSetActorLocation(NewPos); SetActorRotation(FRotator(0, To.Rotation().Yaw, 0));\n"
    "\t\t}\n"
    "\t\telse if (ChoreT <= 0.f) { bAtB = !bAtB; ChoreT = Chore == 3 ? FMath::FRandRange(0.3f, 1.2f) : FMath::FRandRange(3.f, 7.f); if (Chore == 3) { const float A = FMath::FRandRange(0.f, 2.f * PI); (bAtB ? ChoreB : ChoreA) = HomePos + FVector(FMath::Cos(A), FMath::Sin(A), 0) * FMath::FRandRange(300.f, 900.f); } }\n"
    "\t\telse { Speed = 0.f; bBend = (Chore == 2); }\n"
    "\t}\n"
    "\telse if (!bTalking && DPl > 350.f)\n\t{\n\t\tWanderT -= Dt;")
rep("ErtNpc.cpp", "\tBody->Animate(Dt, Speed, false, false, 0.f, 0.f);", "\tBody->Animate(Dt, Speed, false, bBend, 0.f, 0.f);")

# 6b) Vazifali NPClarni yaratish (SpawnNpcs oxirida)
rep("ErtGameMode.cpp",
    "\tUE_LOG(LogErtugrul, Log, TEXT(\"NPC: %d\"), N);\n}",
    "\t// Oba hayoti: ayollar suv tashiydi (quduq <-> o'tov), o't o'radi; bolalar yugurib o'ynaydi\n"
    "\t{\n"
    "\t\tauto Gnd = [&](float E, float Nn) { const float X = Nn * 100.f, Y = E * 100.f; FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtChoreGround), true); float Z = 2000.f; if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(X, Y, 60000.f), FVector(X, Y, -5000.f), ECC_Visibility, Q)) Z = Hit.ImpactPoint.Z; return FVector(X, Y, Z + 92.f); };\n"
    "\t\tstruct FCh { int32 Kind; float AE, AN, BE, BN; float R, G, B; };\n"
    "\t\tconst float OE = ErtMap::ObaE, ON = ErtMap::ObaN;\n"
    "\t\tconst FCh Ch[] = {\n"
    "\t\t\t{1, OE + 13, ON + 13, OE - 22, ON + 36, 0.45f, 0.15f, 0.25f}, {1, OE + 13, ON + 13, OE + 40, ON - 12, 0.20f, 0.30f, 0.45f}, {1, OE + 13, ON + 13, OE - 42, ON - 30, 0.50f, 0.32f, 0.12f}, {1, OE + 13, ON + 13, OE + 26, ON + 42, 0.30f, 0.16f, 0.36f},\n"
    "\t\t\t{2, OE - 88, ON + 66, OE - 80, ON + 74, 0.36f, 0.30f, 0.14f}, {2, OE - 92, ON + 40, OE - 84, ON + 46, 0.26f, 0.36f, 0.20f}, {2, OE + 78, ON + 70, OE + 86, ON + 62, 0.44f, 0.24f, 0.18f},\n"
    "\t\t\t{3, OE + 8, ON - 38, OE + 24, ON - 44, 0.55f, 0.40f, 0.15f}, {3, OE - 30, ON - 50, OE - 12, ON - 58, 0.20f, 0.42f, 0.50f}, {3, OE + 44, ON + 20, OE + 56, ON + 30, 0.60f, 0.25f, 0.20f} };\n"
    "\t\tint32 K = 0;\n"
    "\t\tfor (const FCh& C : Ch)\n"
    "\t\t{\n"
    "\t\t\tconst FVector A = Gnd(C.AE, C.AN), B = Gnd(C.BE, C.BN);\n"
    "\t\t\tAErtNpc* Npc = GetWorld()->SpawnActor<AErtNpc>(AErtNpc::StaticClass(), B, FRotator(0, 0, 0));\n"
    "\t\t\tif (!Npc) continue;\n"
    "\t\t\tconst bool bWoman = C.Kind != 3 || (K % 2 == 0);\n"
    "\t\t\tNpc->Setup(C.Kind == 3 ? TEXT(\"Bola\") : (C.Kind == 1 ? TEXT(\"Oba ayoli\") : TEXT(\"O'roqchi\")), TEXT(\"\"), TEXT(\"\"), bWoman, FLinearColor(C.R, C.G, C.B), 0.f);\n"
    "\t\t\tNpc->SetChore(C.Kind, A, B); ++K; ++N;\n"
    "\t\t}\n"
    "\t}\n"
    "\tUE_LOG(LogErtugrul, Log, TEXT(\"NPC: %d\"), N);\n}")
s = rd(os.path.join(SRC, "ErtGameMode.cpp"))
if "#include \"ErtWorldBuilder.h\"" not in s:
    s = s.replace("#include \"ErtNpc.h\"", "#include \"ErtNpc.h\"\n#include \"ErtWorldBuilder.h\"", 1); wr(os.path.join(SRC, "ErtGameMode.cpp"), s); print("  +  GameMode include")

# 6c) Qozon-ochoq va kigiz gilamlar (oba)
rep("ErtWorldBuilder.cpp",
    "\t\t// Gulxanlar\n\t\tAddFire(M, OE(-16), ON(16), Z, true);",
    "\t\t// Qozon-ochoq: uch tosh ustida qora qozon (katta o'tov yonida)\n"
    "\t\tAddFire(M, OE(7), ON(-7), Z, true);\n"
    "\t\tfor (int32 q = 0; q < 3; ++q) { const float Aq = q * 2.094f; M.AddSphere(W(OE(7) + FMath::Cos(Aq) * 0.55f, ON(-7) + FMath::Sin(Aq) * 0.55f, Z + 0.15f), 0.2f, 6, Stone); }\n"
    "\t\tM.AddCylinder(W(OE(7), ON(-7), Z + 0.35f), 0.34f, 0.44f, 0.42f, 12, FLinearColor(0.07f, 0.07f, 0.08f), false);\n"
    "\t\tM.AddCylinder(W(OE(7), ON(-7), Z + 0.77f), 0.46f, 0.46f, 0.06f, 12, FLinearColor(0.16f, 0.16f, 0.17f), false);\n"
    "\t\t// Gulxanlar\n\t\tAddFire(M, OE(-16), ON(16), Z, true);")
rep("ErtWorldBuilder.cpp",
    "\t\t\t\tAddYurt(M, OE(u), ON(v), Z, 2.5f, 1.8f, 1.7f, Felt, ErtCol::Sty(FLinearColor(0.72f, 0.66f, 0.55f), ErtCol::StyleFelt), DoorYaw, ++K);",
    "\t\t\t\tAddYurt(M, OE(u), ON(v), Z, 2.5f, 1.8f, 1.7f, Felt, ErtCol::Sty(FLinearColor(0.72f, 0.66f, 0.55f), ErtCol::StyleFelt), DoorYaw, ++K);\n"
    "\t\t\t\t// Kigiz gilam eshik oldida (har 3-o'tovda, qizil yoki ko'k naqsh)\n"
    "\t\t\t\tif (K % 3 == 0) { const float Ry = FMath::DegreesToRadians(DoorYaw); M.AddBox(W(OE(u) + FMath::Cos(Ry) * 3.3f, ON(v) + FMath::Sin(Ry) * 3.3f, Z + 0.03f), FVector(65, 45, 1.5f), (K % 2) ? FLinearColor(0.55f, 0.12f, 0.10f) : FLinearColor(0.18f, 0.24f, 0.48f), FRotator(0, DoorYaw, 0)); }")

# ---------- 7) Daryo oqimi ----------
rep("ert_make_water.py", "float2 p = WP.xy / 100.0;\nfloat t = T;", "float2 p = WP.xy / 100.0;\nfloat t = T;\np.x -= t * 0.9;   // daryo oqimi: janubga (X = N o'qi)\np.y += sin(t * 0.3) * 0.15;", root=r"D:\temp\claude")

# ---------- 8) Ot nomi + parvarish HUD ----------
rep("ErtHorse.h", "\tfloat Care = 0.f, CareFxT = 0.f;", "\tfloat Care = 0.f, CareFxT = 0.f;\n\tFString HorseName;   // nom (Bo'ra, Tulpor...) - HUD da")
rep("ErtHorse.cpp", "\tHomePos = GetActorLocation();\n\tWanderTarget = HomePos;",
    "\tHomePos = GetActorLocation();\n\tWanderTarget = HomePos;\n"
    "\t{ static const TCHAR* Names[] = { TEXT(\"Bo'ra\"), TEXT(\"Tulpor\"), TEXT(\"Qorabayir\"), TEXT(\"Chaqmoq\"), TEXT(\"Bo'z\"), TEXT(\"Yulduz\"), TEXT(\"Shamol\"), TEXT(\"Qizil\"), TEXT(\"Oqtosh\"), TEXT(\"Burgut\") };\n"
    "\t  const int32 Hn = FMath::Abs((int32)(HomePos.X * 0.013f) + (int32)(HomePos.Y * 0.031f)); HorseName = bCamel ? TEXT(\"Tuya\") : Names[Hn % 10]; }")
rep("ErtHUD.cpp",
    "Text(FString::Printf(TEXT(\"%s %d\"), H->GetHorse()->IsCamel() ? TEXT(\"Tuya\") : TEXT(\"Ot\"), (int32)H->GetHorse()->Health), X + 166 * Sc, Y - 44 * Sc, White, 0.85f * Sc); }",
    "Text(FString::Printf(TEXT(\"%s %d  |  parvarish %d%%  tezlik +%d%%\"), *H->GetHorse()->HorseName, (int32)H->GetHorse()->Health, (int32)(H->GetHorse()->Care * 100.f), (int32)(H->GetHorse()->Care * 12.f)), X + 166 * Sc, Y - 44 * Sc, White, 0.85f * Sc); }")
rep("ErtHUD.cpp",
    "\t\telse if (AErtHorse* Nh = H->NearestHorse(320.f)) Text(Nh->IsCamel() ? TEXT(\"[E] Tuyaga minish\") : TEXT(\"[E] Otga minish\"), X, Y - 22 * Sc, FLinearColor(1.f, 0.85f, 0.35f), Sc);",
    "\t\telse if (AErtHorse* Nh = H->NearestHorse(320.f)) Text(FString::Printf(TEXT(\"[E] %s: minish   V: tarash   H: boqish (go'sht %d)   parvarish %d%%\"), *Nh->HorseName, H->Meat, (int32)(Nh->Care * 100.f)), X, Y - 22 * Sc, FLinearColor(1.f, 0.85f, 0.35f), Sc);")

# ---------- 9) Kat-sahna: silliq kamera (Catmull-Rom) + intro uchish ----------
rep("ErtCutscene.cpp",
    "\t\t\tP = FMath::Lerp(P, C[j + 1].Pos, U); L = FMath::Lerp(L, C[j + 1].Look, U); Fov = FMath::Lerp(Fov, C[j + 1].Fov, U);",
    "\t\t\t// Silliq kamera: Catmull-Rom (pozitsiya va qarash nuqtasi), FOV smoothstep\n"
    "\t\t\tauto At = [&](int32 k) -> const FErtCutCamKey& { return C[FMath::Clamp(k, 0, C.Num() - 1)]; };\n"
    "\t\t\tP = FMath::CubicCRSplineInterp(At(j - 1).Pos, At(j).Pos, At(j + 1).Pos, At(j + 2).Pos, 0.f, 1.f, 2.f, 3.f, 1.f + U);\n"
    "\t\t\tL = FMath::CubicCRSplineInterp(At(j - 1).Look, At(j).Look, At(j + 1).Look, At(j + 2).Look, 0.f, 1.f, 2.f, 3.f, 1.f + U);\n"
    "\t\t\tFov = FMath::Lerp(Fov, C[j + 1].Fov, FMath::SmoothStep(0.f, 1.f, U));")
rep("ErtCutscene.cpp",
    "\tif (R->TryGetNumberField(TEXT(\"duration\"), D)) Scene.Duration = D;",
    "\tif (R->TryGetNumberField(TEXT(\"duration\"), D)) Scene.Duration = D;\n\tdouble IntroFly = 0; R->TryGetNumberField(TEXT(\"intro_fly\"), IntroFly);")
rep("ErtCutscene.cpp",
    "\treturn Scene.Camera.Num() > 0 || Scene.Actors.Num() > 0;",
    "\t// Intro uchish: birinchi kamera kalitidan balanddan/uzoqdan tushib keladi (-ErtFly uslubi), hamma vaqtlar suriladi\n"
    "\tif (IntroFly > 0.0 && Scene.Camera.Num())\n\t{\n"
    "\t\tfor (FErtCutCamKey& C : Scene.Camera) C.T += IntroFly;\n"
    "\t\tfor (FErtCutActorDef& A : Scene.Actors) for (FErtCutKey& K : A.Keys) K.T += IntroFly;\n"
    "\t\tfor (FErtCutLine& Ln : Scene.Lines) Ln.T += IntroFly;\n"
    "\t\tScene.Duration += IntroFly;\n"
    "\t\tFErtCutCamKey F0 = Scene.Camera[0]; const FVector Dir = (F0.Pos - F0.Look).GetSafeNormal2D();\n"
    "\t\tF0.T = 0.f; F0.Pos = F0.Pos + Dir * 22.f + FVector(0, 26.f, 0); F0.Fov = FMath::Max(35.f, F0.Fov - 8.f);   // y-up dvijok koordinatasi: Y = balandlik\n"
    "\t\tFErtCutCamKey F1 = Scene.Camera[0]; F1.T = IntroFly * 0.55f; F1.Pos = Scene.Camera[0].Pos + Dir * 8.f + FVector(0, 9.f, 0);\n"
    "\t\tScene.Camera.Insert(F1, 0); Scene.Camera.Insert(F0, 0);\n"
    "\t}\n"
    "\treturn Scene.Camera.Num() > 0 || Scene.Actors.Num() > 0;")
# Epizod kirish kat-sahnalariga intro_fly qo'shish
n = 0
for f in glob.glob(r"D:\Unreal_projects\Ertugrul\Content\Ertugrul\Data\cutscenes\ep*_intro.json"):
    d = json.load(io.open(f, encoding="utf-8"))
    if d.get("intro_fly"): continue
    d["intro_fly"] = 5.0
    io.open(f, "w", encoding="utf-8", newline="\n").write(json.dumps(d, ensure_ascii=False, indent=1) + "\n"); n += 1
print("intro_fly qo'shildi:", n)
print("OK")
