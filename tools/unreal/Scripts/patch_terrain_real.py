# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def rep(p, a, b):
    s = io.open(SRC + p, encoding='utf-8').read()
    if b in s: return
    assert a in s, ("MISSING in " + p + ": " + a[:80])
    io.open(SRC + p, 'w', encoding='utf-8', newline='\n').write(s.replace(a, b))

# 1) Relyef: tizmali tog'lar, qatlamli qoya, balandroq massiv
rep('ErtWorldBuilder.cpp', "\tH += FMath::Exp(-FMath::Square(DF / 330.f)) * 190.f;\n\t// Qal'a tekisligi",
"""	H += FMath::Exp(-FMath::Square(DF / 330.f)) * 190.f;
	// Tizmalar va qirralar (ridged multifractal): tog' massivi va shimoliy tizmada keskin qirlar, etaklarda yumshoq
	{
		const float MountainMask = FMath::Clamp(Massif + Smooth01((N - 800.f) / 150.f) + FMath::Exp(-FMath::Square(DF / 330.f)) * 0.8f, 0.f, 1.f);
		const float R1 = 1.f - FMath::Abs(Noise(E, N, 0.009f)), R2 = 1.f - FMath::Abs(Noise(E + 500.f, N - 300.f, 0.022f)), R3 = 1.f - FMath::Abs(Noise(E - 200.f, N + 700.f, 0.06f));
		const float Ridge = R1 * R1 * 45.f + R2 * R2 * 18.f + R3 * 4.f;
		H += Ridge * MountainMask * (0.6f + 0.4f * Smooth01((H - 40.f) / 80.f));
		// Qatlamli qoya (strata): baland yonbag'irlarda zinapoya ko'rinishi
		H += MountainMask * 2.2f * FMath::Sin(H * 0.30f + Noise(E, N, 0.03f) * 2.5f);
		// Etak jarliklari: o'rta chastotali chuqurliklar (soylar)
		H -= MountainMask * 6.f * FMath::Pow(FMath::Abs(Noise(E + 900.f, N + 100.f, 0.015f)), 1.5f);
	}
	// Qal'a tekisligi""")
# Uludog' va Erciyes: tizmali detal
rep('ErtWorldBuilder.cpp', "\t\tH += UluH * T * T + Noise(E, N, 0.025f) * 7.f * T;", "\t\tH += UluH * T * T + Noise(E, N, 0.025f) * 7.f * T + FMath::Square(1.f - FMath::Abs(Noise(E, N, 0.03f))) * 14.f * T;")
rep('ErtWorldBuilder.cpp', "\t\tH += ErcH * T * T + Noise(E, N, 0.03f) * 6.f * T;", "\t\tH += ErcH * T * T + Noise(E, N, 0.03f) * 6.f * T + FMath::Square(1.f - FMath::Abs(Noise(E + 77.f, N, 0.04f))) * 10.f * T * (1.f - T);")
# 2) Relyef to'ri 5 m
rep('ErtWorldBuilder.h', "\tUPROPERTY(EditAnywhere, Category = \"Ertugrul|Dunyo\") float CellM = 10.f;", "\tUPROPERTY(EditAnywhere, Category = \"Ertugrul|Dunyo\") float CellM = 5.f;")
# 3) Qoyalar: tog' yonbag'irlarida katta qoya meshlari (cliff), qiyalikka yotqizilgan
rep('ErtWorldBuilder.cpp', "\tfor (int32 i = 0; i < 4000 && Placed < 320; ++i)\n\t{\n\t\tconst float E = RS.FRandRange(-Half + 10.f, Half - 10.f), N = RS.FRandRange(-Half + 10.f, Half - 10.f);\n\t\tconst float H = HeightAt(E, N);\n\t\tconst FVector Nm = TerrainNormal(E, N);\n\t\tconst float DF = FVector2D::Distance(FVector2D(E, N), FVector2D(FortE, FortN));\n\t\tfloat P = 0.02f;\n\t\tif (H > 60.f) P = 0.5f;\n\t\tif (Nm.Z < 0.8f) P += 0.3f;",
    "\tfor (int32 i = 0; i < 9000 && Placed < 700; ++i)\n\t{\n\t\tconst float E = RS.FRandRange(-Half + 10.f, Half - 10.f), N = RS.FRandRange(-Half + 10.f, Half - 10.f);\n\t\tconst float H = HeightAt(E, N);\n\t\tconst FVector Nm = TerrainNormal(E, N);\n\t\tconst float DF = FVector2D::Distance(FVector2D(E, N), FVector2D(FortE, FortN));\n\t\tfloat P = 0.02f;\n\t\tif (H > 60.f) P = 0.5f;\n\t\tif (Nm.Z < 0.8f) P += 0.3f;\n\t\tif (Nm.Z < 0.6f) P += 0.4f;   // tik yonbag'ir: qoya devorlari")
rep('ErtWorldBuilder.cpp', "\t\tconst float R = RS.FRandRange(1.0f, 4.5f) * (H > 100.f ? 1.6f : 1.f);\n\t\t{\n\t\t\tFErtFabLib& Fab = FErtFabLib::Get();\n\t\t\tif (Fab.Rocks.Num())\n\t\t\t{\n\t\t\t\tUStaticMesh* Mesh = Fab.Rocks[RS.RandRange(0, Fab.Rocks.Num() - 1)];\n\t\t\t\tconst float Sc = FErtFabLib::ScaleToRadius(Mesh, R);\n\t\t\t\tFabComp(Mesh, true)->AddInstance(FTransform(FRotator(RS.FRandRange(-8.f, 8.f), RS.FRandRange(0.f, 360.f), RS.FRandRange(-8.f, 8.f)), W(E, N, H - R * 0.15f), FVector(Sc)), true);",
    "\t\tconst bool bCliff = Nm.Z < 0.7f && H > 30.f;\n\t\tconst float R = (bCliff ? RS.FRandRange(4.f, 11.f) : RS.FRandRange(1.0f, 4.5f)) * (H > 100.f ? 1.4f : 1.f);\n\t\t{\n\t\t\tFErtFabLib& Fab = FErtFabLib::Get();\n\t\t\tif (Fab.Rocks.Num())\n\t\t\t{\n\t\t\t\t// Tik yonbag'irda 'cliff' nomli meshlar afzal\n\t\t\t\tUStaticMesh* Mesh = Fab.Rocks[RS.RandRange(0, Fab.Rocks.Num() - 1)];\n\t\t\t\tif (bCliff) for (int32 t = 0; t < 4; ++t) { UStaticMesh* C = Fab.Rocks[RS.RandRange(0, Fab.Rocks.Num() - 1)]; if (C->GetName().Contains(TEXT(\"Cliff\"))) { Mesh = C; break; } }\n\t\t\t\tconst float Sc = FErtFabLib::ScaleToRadius(Mesh, R);\n\t\t\t\t// Qiyalikka yotqizish: mesh yuqorisi relyef normaliga qaraydi (biroz), tasodifiy burilish\n\t\t\t\tconst FRotator Tilt = bCliff ? FRotationMatrix::MakeFromZ(FMath::Lerp(FVector::UpVector, Nm, 0.6f).GetSafeNormal()).Rotator() : FRotator(RS.FRandRange(-8.f, 8.f), 0.f, RS.FRandRange(-8.f, 8.f));\n\t\t\t\tFabComp(Mesh, true)->AddInstance(FTransform(FRotator(Tilt.Pitch, RS.FRandRange(0.f, 360.f), Tilt.Roll), W(E, N, H - R * (bCliff ? 0.35f : 0.15f)), FVector(Sc)), true);")
print('cpp ok')

# 4) Relyef shaderi: qor chizig'i 150 m, baland tog'da qoya rangi kulrang-jigarrang
p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
n0 = m.count('75')
m = m.replace("saturate((h - 75.0)", "saturate((h - 150.0)").replace("(WP.z / 100.0 - 75.0)", "(WP.z / 100.0 - 150.0)").replace("h > 75.0", "h > 150.0")
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('pbr 75 ->', m.count('150.0'))
