# -*- coding: utf-8 -*-
# -ErtFly: ko'p nuqtali kamera yo'li (Blender oba videosi kabi), har kadr skrinshot; oba markazida Ertug'rul modeli
import io, os, sys
SRC = r"D:\Unreal_projects\Ertugrul\Source\Ertugrul"
def rd(p): return io.open(p, encoding="utf-8").read()
def wr(p, s): io.open(p, "w", encoding="utf-8", newline="\n").write(s)
def rep(path, old, new):
    p = os.path.join(SRC, path); s = rd(p)
    if new in s: print("  = allaqachon:", path); return
    if old not in s: print("!! topilmadi:", path, old[:70]); sys.exit(1)
    wr(p, s.replace(old, new, 1)); print("  + ", path)

rep("ErtCharacter.h", "\tTArray<float> CamShot; float CamShotT = -1.f;",
    "\tTArray<float> CamShot; float CamShotT = -1.f;\n"
    "\t// -ErtFly=E,N,Z,P,Y;E,N,Z,P,Y;... -ErtFlyFrames=72 -ErtFlyStep=0.35: Catmull-Rom yo'l, har kadr skrinshot (fly_NNN), oxirida chiqish\n"
    "\tTArray<FVector> FlyPos; TArray<FVector2D> FlyRot; int32 FlyFrames = 0, FlyIdx = -1; float FlyStep = 0.35f, FlyT = -1.f;")
rep("ErtCharacter.cpp",
    "\t\tFString CamArg; if (FParse::Value(FCommandLine::Get(), TEXT(\"-ErtCam=\"), CamArg, false))",
    "\t\tFString FlyArg; if (FParse::Value(FCommandLine::Get(), TEXT(\"-ErtFly=\"), FlyArg, false))\n"
    "\t\t{\n\t\t\tTArray<FString> Keys; FlyArg.TrimQuotes().ParseIntoArray(Keys, TEXT(\";\"));\n"
    "\t\t\tfor (const FString& K : Keys) { TArray<FString> P; K.ParseIntoArray(P, TEXT(\",\")); if (P.Num() >= 5) { FlyPos.Add(FVector(FCString::Atof(*P[0]), FCString::Atof(*P[1]), FCString::Atof(*P[2]))); FlyRot.Add(FVector2D(FCString::Atof(*P[3]), FCString::Atof(*P[4]))); } }\n"
    "\t\t\tFlyFrames = 72; FParse::Value(FCommandLine::Get(), TEXT(\"-ErtFlyFrames=\"), FlyFrames); FParse::Value(FCommandLine::Get(), TEXT(\"-ErtFlyStep=\"), FlyStep);\n"
    "\t\t\tif (FlyPos.Num() >= 2) { FlyT = 0.f; FlyIdx = -1; if (ShotDir.IsEmpty()) ShotDir = TEXT(\"D:/temp/claude/flyshot\"); }\n\t\t}\n"
    "\t\tFString CamArg; if (FParse::Value(FCommandLine::Get(), TEXT(\"-ErtCam=\"), CamArg, false))")
rep("ErtCharacter.cpp",
    "\tif (CamShotT >= 0.f)\n\t{\n\t\tconst float Prev = CamShotT; CamShotT += Dt;",
    "\tif (FlyT >= 0.f)\n\t{\n"
    "\t\tFlyT += Dt;\n"
    "\t\tconst float Start = 8.f;\n"
    "\t\tif (FlyT >= Start)\n\t\t{\n"
    "\t\t\tconst int32 Want = FMath::Min(FlyFrames - 1, (int32)((FlyT - Start) / FlyStep));\n"
    "\t\t\tif (Want > FlyIdx)\n\t\t\t{\n"
    "\t\t\t\t// Oldingi kadr: skrinshot (kamera bir tick oldin joylashgan)\n"
    "\t\t\t\tif (FlyIdx >= 0) { const FString File = FString::Printf(TEXT(\"%s/fly_%03d.png\"), *ShotDir, FlyIdx); FScreenshotRequest::RequestScreenshot(File, false, false); }\n"
    "\t\t\t\tif (FlyIdx >= FlyFrames - 1) { FlyT = -1.f; UE_LOG(LogErtugrul, Log, TEXT(\"[Fly] %d kadr tayyor\"), FlyFrames); FPlatformMisc::RequestExit(false); return; }\n"
    "\t\t\t\tFlyIdx = Want;\n"
    "\t\t\t\tconst float U = FlyFrames > 1 ? (float)FlyIdx / (float)(FlyFrames - 1) : 0.f;\n"
    "\t\t\t\tconst int32 Segs = FlyPos.Num() - 1; const float S = U * Segs; const int32 I = FMath::Clamp((int32)S, 0, Segs - 1); const float T = S - I;\n"
    "\t\t\t\tauto At = [&](int32 k) { return FlyPos[FMath::Clamp(k, 0, FlyPos.Num() - 1)]; };\n"
    "\t\t\t\tauto RotAt = [&](int32 k) { return FlyRot[FMath::Clamp(k, 0, FlyRot.Num() - 1)]; };\n"
    "\t\t\t\tconst FVector P = FMath::CubicCRSplineInterp(At(I - 1), At(I), At(I + 1), At(I + 2), 0.f, 1.f, 2.f, 3.f, 1.f + T);\n"
    "\t\t\t\tconst float Sm = FMath::SmoothStep(0.f, 1.f, T);\n"
    "\t\t\t\tconst FVector2D R0 = RotAt(I), R1 = RotAt(I + 1);\n"
    "\t\t\t\tconst float Pitch = FMath::Lerp(R0.X, R1.X, Sm), Yaw = R0.Y + FMath::FindDeltaAngleDegrees(R0.Y, R1.Y) * Sm;\n"
    "\t\t\t\tTeleport(P.X, P.Y, P.Z, Pitch, Yaw); TargetArm = 0.f; Boom->TargetArmLength = 0.f;\n"
    "\t\t\t}\n\t\t}\n\t}\n"
    "\tif (CamShotT >= 0.f)\n\t{\n\t\tconst float Prev = CamShotT; CamShotT += Dt;")

# Oba markazida Ertug'rul modeli (olov yonida)
rep("ErtWorldBuilder.cpp",
    "void AErtWorldBuilder::BuildLandmarks()\n{\n\tFErtFabLib& Fab = FErtFabLib::Get();\n\tint32 N = 0;",
    "void AErtWorldBuilder::BuildLandmarks()\n{\n\tFErtFabLib& Fab = FErtFabLib::Get();\n\tint32 N = 0;\n"
    "\t// Ertug'rul Bey modeli (AssetHub/Blender, statik): oba markazi olovi yonida, keyinchalik skeletli versiya bilan almashadi\n"
    "\tif (UStaticMesh* Hero = LoadObject<UStaticMesh>(nullptr, TEXT(\"/Game/ErtAssets/Chars/Hero/SM_ErtugrulHero\")))\n"
    "\t{ const float HE = ObaE + 3.f, HN = ObaN - 6.f; FabPlace(Hero, HE, HN, HeightAt(HE, HN), 90.f, 1.85f, true, true); ++N; UE_LOG(LogErtugrul, Log, TEXT(\"Ertug'rul modeli obada (%.0f, %.0f)\"), HE, HN); }")
print("OK")
