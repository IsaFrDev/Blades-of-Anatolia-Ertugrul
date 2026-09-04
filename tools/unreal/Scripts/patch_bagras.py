# -*- coding: utf-8 -*-
import io, re
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# Uslub yordamchilari (alfa kanali)
h = load('ErtProcMesh.h')
h = rep(h, "namespace ErtCol\n{", '''namespace ErtCol
{
	// Material uslubi (M_ErtVertexColor): vertex rangining alfa kanali naqshni tanlaydi
	constexpr float StyleGround = 0.0f, StyleStone = 0.2f, StyleWood = 0.4f, StyleRoof = 0.6f, StyleBrick = 0.8f, StylePlain = 1.0f;
	inline FLinearColor Sty(const FLinearColor& C, float Style) { return FLinearColor(C.R, C.G, C.B, Style); }''')
save('ErtProcMesh.h', h)

c = load('ErtWorldBuilder.cpp')
c = rep(c, "\t\t\t\t\tC.Add(TerrainColor(E, N, H, 1.f - Nm.Z));", "\t\t\t\t\tC.Add(ErtCol::Sty(TerrainColor(E, N, H, 1.f - Nm.Z), ErtCol::StyleGround));")
start = c.index("void AErtWorldBuilder::BuildFortress()")
end = c.index("// ---------------- Devorli shahar (janubi-g'arb, oltin gumbaz) ----------------")
new_fort = r'''void AErtWorldBuilder::BuildFortress()
{
	// Bagras qal'asi: tog' cho'qqisidagi salibchilar qal'asi. Blokli tosh devorlar (PBR uslub), mashikuli, kungura,
	// konus cherepitsa tomli dumaloq burjlar, arkli darvozaxona, panjara, ko'tarma ko'prik, donjon, cherkov, kazarma, otxona, temirchi.
	const float Z = FortZ;
	auto FE = [](float u) { return FortE + u; };
	auto FN = [](float v) { return FortN + v; };
	FErtMeshData M(100.f);
	const float Half = FortHalf, WallH = 12.f, Th = 2.2f;
	int32 S = 0;
	using namespace ErtCol;
	const FLinearColor StoneS = Sty(FLinearColor(0.58f, 0.55f, 0.50f), StyleStone), StoneD = Sty(FLinearColor(0.42f, 0.40f, 0.37f), StyleStone);
	const FLinearColor RoofT = Sty(FLinearColor(0.50f, 0.26f, 0.16f), StyleRoof), WoodP = Sty(FLinearColor(0.40f, 0.27f, 0.13f), StyleWood), WoodD = Sty(FLinearColor(0.26f, 0.17f, 0.08f), StyleWood);
	const FLinearColor Iron(0.25f, 0.26f, 0.28f), Slit(0.03f, 0.03f, 0.03f), Blue(0.1f, 0.15f, 0.45f), Straw2 = Sty(FLinearColor(0.80f, 0.70f, 0.40f), StylePlain), Slate = Sty(FLinearColor(0.30f, 0.32f, 0.36f), StyleRoof);
	auto Tower = [&](float u, float v, float R, float Hh, float RoofH, bool bBig)
	{
		M.AddCylinder(W(FE(u), FN(v), Z - 2.f), R * 1.12f, R, Hh + 2.f, 14, Vary(StoneS, 0.05f, ++S), false);          // glasisli tana
		for (float bz = 3.f; bz < Hh - 1.f; bz += 3.f) M.AddCylinder(W(FE(u), FN(v), Z + bz), R * 1.01f, R * 1.01f, 0.18f, 14, StoneD, false);   // qator choklari
		M.AddCylinder(W(FE(u), FN(v), Z + Hh - 0.6f), R * 1.22f, R * 1.22f, 0.9f, 14, Vary(StoneD, 0.04f, ++S), true); // mashikuli tokchasi
		for (int32 i = 0; i < 14; ++i) { const float A = i * 2.f * PI / 14; M.AddBox(W(FE(u + FMath::Cos(A) * R * 1.12f), FN(v + FMath::Sin(A) * R * 1.12f), Z + Hh - 1.4f), FVector(22, 22, 70), StoneD, FRotator(0, FMath::RadiansToDegrees(A), 0)); }   // konsollar
		M.AddCylinder(W(FE(u), FN(v), Z + Hh + 0.3f), R * 1.15f, R * 1.15f, 1.3f, 14, Vary(StoneS, 0.05f, ++S), false); // parapet
		for (int32 i = 0; i < 14; i += 2) { const float A = (i + 0.5f) * 2.f * PI / 14; M.AddBox(W(FE(u + FMath::Cos(A) * R * 1.1f), FN(v + FMath::Sin(A) * R * 1.1f), Z + Hh + 2.f), FVector(28, 45, 55), Vary(StoneS, 0.05f, ++S), FRotator(0, FMath::RadiansToDegrees(A), 0)); }   // kungura tishlari
		M.AddCone(W(FE(u), FN(v), Z + Hh + 1.6f), R * 1.2f, RoofH, 14, Vary(RoofT, 0.06f, ++S));
		M.AddSphere(W(FE(u), FN(v), Z + Hh + 1.6f + RoofH), 0.25f, 6, Iron);
		for (int32 i = 0; i < (bBig ? 8 : 4); ++i) { const float A = i * 2.f * PI / (bBig ? 8 : 4) + 0.4f; M.AddBox(W(FE(u + FMath::Cos(A) * R * 1.0f), FN(v + FMath::Sin(A) * R * 1.0f), Z + Hh * 0.55f), FVector(12, 14, 110), Slit, FRotator(0, FMath::RadiansToDegrees(A), 0)); }   // o'q tirqishlari
	};
	auto WallRun = [&](float u0, float v0, float u1, float v1, bool bGate)
	{
		const float Len = FVector2D::Distance(FVector2D(u0, v0), FVector2D(u1, v1));
		const int32 Blocks = FMath::CeilToInt(Len / 4.f);
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(u1 - u0, v1 - v0));
		const float cu0 = (u0 + u1) * 0.5f, cv0 = (v0 + v1) * 0.5f;
		const float InU = (cu0 != 0) ? -FMath::Sign(cu0) : 0.f, InV = (cv0 != 0) ? -FMath::Sign(cv0) : 0.f;   // ichkariga yo'nalish
		for (int32 i = 0; i < Blocks; ++i)
		{
			const float t0 = (float)i / Blocks, t1 = (float)(i + 1) / Blocks;
			const float cu = FMath::Lerp(u0, u1, (t0 + t1) * 0.5f), cv = FMath::Lerp(v0, v1, (t0 + t1) * 0.5f);
			const float BL = Len * (t1 - t0) * 0.5f;
			if (bGate && FMath::Abs(cu) < 3.5f) continue;   // darvozaxona alohida
			M.AddBox(W(FE(cu), FN(cv), Z + WallH * 0.5f), FVector(BL, Th, WallH * 0.5f) * 100.f, Vary(StoneS, 0.06f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(FE(cu), FN(cv), Z + 1.2f), FVector(BL, Th * 1.25f, 1.2f) * 100.f, Vary(StoneD, 0.05f, ++S), FRotator(0, Yaw, 0));   // poydevor qalinlashuvi
			// Mashikuli tokchasi (tashqi tomonda) va parapet, kungura tishlari
			M.AddBox(W(FE(cu - InU * (Th * 0.5f + 0.4f)), FN(cv - InV * (Th * 0.5f + 0.4f)), Z + WallH - 0.3f), FVector(BL, 0.5f, 0.35f) * 100.f, Vary(StoneD, 0.05f, ++S), FRotator(0, Yaw, 0));
			M.AddBox(W(FE(cu - InU * (Th * 0.5f + 0.35f)), FN(cv - InV * (Th * 0.5f + 0.35f)), Z + WallH + 0.7f), FVector(BL, 0.35f, 0.7f) * 100.f, Vary(StoneS, 0.06f, ++S), FRotator(0, Yaw, 0));
			const int32 Merlons = FMath::Max(1, FMath::RoundToInt(BL * 2.f / 1.6f));
			for (int32 k = 0; k < Merlons; ++k)
			{
				const float t = (k + 0.5f) / Merlons - 0.5f;
				const float du = (u1 - u0) / Len * t * BL * 2.f, dv = (v1 - v0) / Len * t * BL * 2.f;
				M.AddBox(W(FE(cu + du - InU * (Th * 0.5f + 0.35f)), FN(cv + dv - InV * (Th * 0.5f + 0.35f)), Z + WallH + 1.85f), FVector(0.45f, 0.35f, 0.5f) * 100.f, Vary(StoneS, 0.06f, ++S), FRotator(0, Yaw, 0));
				if (k % 2 == 0) M.AddBox(W(FE(cu + du - InU * (Th * 0.5f + 0.42f)), FN(cv + dv - InV * (Th * 0.5f + 0.42f)), Z + WallH * 0.6f), FVector(0.06f, 0.06f, 0.9f) * 100.f, Slit, FRotator(0, Yaw, 0));   // o'q tirqishi
			}
		}
		// Yo'lak (devor ichki tomonidagi yog'och supa) va tayanch ustunlar
		M.AddBox(W(FE(cu0 + InU * (Th * 0.5f + 0.8f)), FN(cv0 + InV * (Th * 0.5f + 0.8f)), Z + WallH - 1.6f), FVector(Len * 0.5f - 2.f, 0.85f, 0.12f) * 100.f, WoodP, FRotator(0, Yaw, 0));
		for (float t = -Len * 0.5f + 3.f; t < Len * 0.5f - 1.f; t += 4.f)
		{
			const float du = (u1 - u0) / Len * t, dv = (v1 - v0) / Len * t;
			M.AddBox(W(FE(cu0 + du + InU * (Th * 0.5f + 1.2f)), FN(cv0 + dv + InV * (Th * 0.5f + 1.2f)), Z + WallH * 0.5f - 0.8f), FVector(0.12f, 0.12f, WallH * 0.5f - 0.8f) * 100.f, WoodD, FRotator(0, Yaw, 0));
			M.AddBox(W(FE(cu0 + du + InU * (Th * 0.5f + 0.8f)), FN(cv0 + dv + InV * (Th * 0.5f + 0.8f)), Z + WallH - 1.05f), FVector(0.08f, 0.8f, 0.5f) * 100.f, WoodD, FRotator(0, Yaw, 0));   // panjara
		}
	};
	WallRun(-Half, -Half, Half, -Half, true);
	WallRun(-Half, Half, Half, Half, false);
	WallRun(-Half, -Half, -Half, Half, false);
	WallRun(Half, -Half, Half, Half, false);
	for (int32 i = 0; i < 4; ++i) Tower((i & 1) ? Half : -Half, (i & 2) ? Half : -Half, 5.2f, 17.f, 5.5f, true);
	Tower(0.f, Half, 3.6f, 15.f, 4.f, false);
	Tower(-Half, 0.f, 3.6f, 15.f, 4.f, false); Tower(Half, 0.f, 3.6f, 15.f, 4.f, false);
	// Darvozaxona: ikki dumaloq minora, ark, panjara, ko'tarma ko'prik, xandaq
	Tower(-6.f, -Half, 3.8f, 15.f, 4.5f, false); Tower(6.f, -Half, 3.8f, 15.f, 4.5f, false);
	M.AddBox(W(FE(0), FN(-Half), Z + 11.f), FVector(300, 260, 300), Vary(StoneS, 0.05f, ++S));                                 // ark usti
	M.AddCylinder(W(FE(0), FN(-Half + 1.4f), Z + 5.f), 3.f, 3.f, 2.8f, 12, Slit, true, FRotator(90.f, 0, 0));                 // ark (qorong'i yo'lak)
	for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(FE(k * 3.1f), FN(-Half), Z + 4.f), FVector(40, 260, 400), Vary(StoneD, 0.05f, ++S));   // ark tayanchlari
	for (int32 i = 0; i < 6; ++i) M.AddBox(W(FE(-2.5f + i * 1.f), FN(-Half - 1.35f), Z + 3.4f), FVector(5, 5, 340), Iron);        // panjara vertikal
	for (int32 i = 0; i < 5; ++i) M.AddBox(W(FE(0), FN(-Half - 1.35f), Z + 0.9f + i * 1.3f), FVector(300, 5, 5), Iron);           // panjara gorizontal
	M.AddBox(W(FE(0), FN(-Half - 5.5f), Z + 0.12f), FVector(300, 480, 10), Vary(WoodP, 0.06f, ++S));                            // ko'tarma ko'prik (tushirilgan)
	for (int32 i = 0; i < 7; ++i) M.AddBox(W(FE(0), FN(-Half - 1.8f - i * 1.3f), Z + 0.2f), FVector(300, 6, 6), WoodD);
	for (int32 k = -1; k <= 1; k += 2) { M.AddBox(W(FE(k * 1.7f), FN(-Half - 6.f), Z + 5.f), FVector(3, 800, 3), Iron, FRotator(-52.f, 0, 0)); }   // zanjirlar
	M.AddBox(W(FE(0), FN(-Half - 5.5f), Z - 1.6f), FVector(900, 480, 120), Sty(FLinearColor(0.30f, 0.28f, 0.22f), StyleGround));   // xandaq tubi
	for (int32 k = -1; k <= 1; k += 2) AddBanner(M, FE(k * 6.f), FN(-Half + 1.f), Z + 15.f + 1.6f + 4.5f, 3.5f, Blue, false);
	AddFire(M, FE(-3.6f), FN(-Half + 3.5f), Z, true); AddFire(M, FE(3.6f), FN(-Half + 3.5f), Z, true);
	// Donjon (qasr): kvadrat, burchak ustunlari, uch qavat derazalar, mashikuli, kungura, bayroq, ustki shiypon tom
	{
		const float DU = 0.f, DV = 8.f, DH = 26.f, HW = 6.f;
		M.AddBox(W(FE(DU), FN(DV), Z + DH * 0.5f), FVector(HW * 100.f, HW * 100.f, DH * 50.f), Vary(StoneS, 0.04f, ++S));
		M.AddBox(W(FE(DU), FN(DV), Z + 1.5f), FVector(HW * 100.f + 40.f, HW * 100.f + 40.f, 150), Vary(StoneD, 0.04f, ++S));   // poydevor
		for (int32 i = 0; i < 4; ++i)
		{
			const float u = DU + ((i & 1) ? HW : -HW), v = DV + ((i & 2) ? HW : -HW);
			M.AddBox(W(FE(u), FN(v), Z + DH * 0.5f + 1.f), FVector(110, 110, DH * 50.f + 100.f), Vary(StoneD, 0.04f, ++S));       // burchak ustunlari
			M.AddCone(W(FE(u), FN(v), Z + DH + 2.f), 1.2f, 2.2f, 8, Slate);
		}
		for (int32 f = 0; f < 3; ++f) for (int32 side = 0; side < 4; ++side)
		{
			const float zz = Z + 6.f + f * 7.f;
			const float su = (side == 0) ? -HW : (side == 1) ? HW : 0.f, sv = (side == 2) ? -HW : (side == 3) ? HW : 0.f;
			for (int32 k = -1; k <= 1; k += 2)
			{
				const float ou = (side < 2) ? 0.f : k * 1.8f, ov = (side < 2) ? k * 1.8f : 0.f;
				M.AddBox(W(FE(DU + su + ou), FN(DV + sv + ov), zz), FVector((side < 2) ? 14.f : 60.f, (side < 2) ? 60.f : 14.f, f == 2 ? 140.f : 90.f), Slit);   // deraza
				M.AddBox(W(FE(DU + su + ou), FN(DV + sv + ov), zz + (f == 2 ? 0.8f : 0.55f)), FVector((side < 2) ? 20.f : 80.f, (side < 2) ? 80.f : 20.f, 12.f), Vary(StoneD, 0.05f, ++S));   // deraza peshtoqi
			}
		}
		M.AddBox(W(FE(DU), FN(DV), Z + DH - 0.5f), FVector(HW * 100.f + 70.f, HW * 100.f + 70.f, 45.f), Vary(StoneD, 0.04f, ++S));   // mashikuli
		for (int32 i = 0; i < 12; ++i)
		{
			const float t = -HW + 0.5f + i * (2.f * HW - 1.f) / 11.f;
			M.AddBox(W(FE(DU + t), FN(DV - HW - 0.5f), Z + DH + 0.6f), FVector(40, 30, 60), Vary(StoneS, 0.05f, ++S)); M.AddBox(W(FE(DU + t), FN(DV + HW + 0.5f), Z + DH + 0.6f), FVector(40, 30, 60), Vary(StoneS, 0.05f, ++S));
			M.AddBox(W(FE(DU - HW - 0.5f), FN(DV + t), Z + DH + 0.6f), FVector(30, 40, 60), Vary(StoneS, 0.05f, ++S)); M.AddBox(W(FE(DU + HW + 0.5f), FN(DV + t), Z + DH + 0.6f), FVector(30, 40, 60), Vary(StoneS, 0.05f, ++S));
		}
		M.AddCone(W(FE(DU), FN(DV), Z + DH + 0.2f), HW * 0.75f, 4.5f, 4, Vary(Slate, 0.05f, ++S), FRotator(0, 45.f, 0));   // shiypon tom
		AddBanner(M, FE(DU), FN(DV), Z + DH + 4.6f, 4.f, Blue, false);
		M.AddBox(W(FE(DU), FN(DV - HW - 0.1f), Z + 2.2f), FVector(160, 20, 240), WoodD);   // katta eshik
		M.AddCylinder(W(FE(DU), FN(DV - HW - 0.15f), Z + 3.4f), 0.8f, 0.8f, 0.3f, 10, WoodD, true, FRotator(90.f, 0, 0));
		for (int32 i = 0; i < 6; ++i) M.AddBox(W(FE(DU - 3.f + i * 1.2f), FN(DV - HW - 1.2f - i * 0.0f), Z + 0.15f + i * 0.0f), FVector(50, 90, 12), Vary(StoneD, 0.05f, ++S));   // zinapoya toshlari
	}
	// Cherkov (g'arb): tosh nef, yarim dumaloq apsida, qiya tom, xoch; kazarma (sharq, cherepitsa tom); otxona (yog'och); temirchi; quduq; bochkalar; oshxona tutuni
	M.AddBox(W(FE(-20.f), FN(12.f), Z + 3.f), FVector(500, 1000, 300), Vary(StoneS, 0.05f, ++S));
	M.AddCylinder(W(FE(-20.f), FN(22.f), Z), 2.5f, 2.5f, 5.f, 10, Vary(StoneS, 0.05f, ++S), true);
	M.AddCone(W(FE(-20.f), FN(22.f), Z + 5.f), 2.7f, 1.6f, 10, Vary(RoofT, 0.06f, ++S));
	M.AddCone(W(FE(-20.f), FN(12.f), Z + 5.8f), 4.6f, 2.8f, 4, Vary(RoofT, 0.06f, ++S), FRotator(0, 45.f, 0));
	M.AddBox(W(FE(-20.f), FN(4.f), Z + 9.2f), FVector(8, 8, 90), FLinearColor(0.9f, 0.75f, 0.25f)); M.AddBox(W(FE(-20.f), FN(4.f), Z + 9.6f), FVector(8, 40, 8), FLinearColor(0.9f, 0.75f, 0.25f));
	for (int32 i = 0; i < 3; ++i) for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(FE(-20.f + k * 2.55f), FN(8.f + i * 3.5f), Z + 3.6f), FVector(10, 30, 100), Slit);   // uzun derazalar
	M.AddBox(W(FE(21.f), FN(14.f), Z + 2.5f), FVector(450, 900, 250), Vary(StoneS, 0.06f, ++S));                                    // kazarma
	M.AddCone(W(FE(21.f), FN(14.f), Z + 5.f), 5.6f, 2.6f, 4, Vary(RoofT, 0.06f, ++S), FRotator(0, 45.f, 0));
	for (int32 i = 0; i < 4; ++i) M.AddBox(W(FE(16.4f), FN(8.f + i * 4.f), Z + 2.8f), FVector(10, 50, 70), Slit);
	M.AddBox(W(FE(21.f), FN(-12.f), Z + 1.8f), FVector(400, 700, 180), Vary(WoodP, 0.08f, ++S));                                   // otxona
	M.AddCone(W(FE(21.f), FN(-12.f), Z + 3.6f), 5.f, 2.2f, 4, Straw2, FRotator(0, 45.f, 0));
	for (int32 i = 0; i < 3; ++i) M.AddBox(W(FE(16.9f), FN(-17.f + i * 5.f), Z + 1.2f), FVector(6, 140, 120), WoodD);
	AddHorse(M, FE(24.f), FN(-20.f), Z, 90.f, FLinearColor(0.36f, 0.22f, 0.11f));
	M.AddBox(W(FE(-20.f), FN(-14.f), Z + 1.6f), FVector(300, 350, 160), Vary(StoneD, 0.06f, ++S));                                  // temirchi
	M.AddCone(W(FE(-20.f), FN(-14.f), Z + 3.2f), 4.f, 1.8f, 4, Slate, FRotator(0, 45.f, 0));
	M.AddBox(W(FE(-20.f), FN(-14.f), Z + 4.6f), FVector(50, 50, 120), Vary(StoneD, 0.05f, ++S));                                    // mo'ri
	M.AddBox(W(FE(-16.f), FN(-14.f), Z + 0.7f), FVector(45, 25, 70), Iron);                                                         // sandon
	AddFire(M, FE(-17.5f), FN(-11.f), Z, true);
	M.AddCylinder(W(FE(12.f), FN(-6.f), Z), 1.2f, 1.2f, 1.f, 12, Vary(StoneS, 0.05f, ++S), false);                                   // quduq
	for (int32 k = -1; k <= 1; k += 2) M.AddBox(W(FE(12.f + k * 1.3f), FN(-6.f), Z + 1.3f), FVector(10, 10, 260), WoodD);
	M.AddCone(W(FE(12.f), FN(-6.f), Z + 2.6f), 1.8f, 1.f, 8, Vary(RoofT, 0.06f, ++S));
	for (int32 i = 0; i < 7; ++i) { const float u = -12.f + (i % 4) * 1.1f, v = -24.f + (i / 4) * 1.1f; M.AddCylinder(W(FE(u), FN(v), Z), 0.42f, 0.42f, 0.95f, 8, Vary(WoodP, 0.08f, ++S), true); M.AddCylinder(W(FE(u), FN(v), Z + 0.45f), 0.44f, 0.44f, 0.06f, 8, Iron, true); }   // bochkalar
	for (int32 i = 0; i < 3; ++i) M.AddBox(W(FE(6.f + i * 1.5f), FN(-24.f), Z + 0.5f), FVector(60, 60, 50), Vary(WoodP, 0.1f, ++S));   // yashiklar
	for (int32 i = 0; i < 6; ++i) { const float A = i * PI / 3; M.AddBox(W(FE(-6.f), FN(-24.f), Z + 1.6f), FVector(6, 6, 160), WoodD, FRotator(0, 0, 0)); M.AddBox(W(FE(-6.f + FMath::Cos(A) * 0.5f), FN(-24.f + FMath::Sin(A) * 0.5f), Z + 3.2f), FVector(10, 10, 12), Iron); }   // qurol ustuni
	for (int32 i = 0; i < 4; ++i) { const float u = -Half + 3.f + i * 20.f; M.AddBox(W(FE(u), FN(Half - 2.6f), Z + 1.8f), FVector(6, 6, 180), WoodD); M.AddSphere(W(FE(u), FN(Half - 2.6f), Z + 3.7f), 0.25f, 6, FLinearColor(1.0f, 0.55f, 0.1f)); }   // mash'alalar
	M.Commit(NewPart(TEXT("Fortress"), true), 0, true);
}

'''
c = c[:start] + new_fort + c[end:]
save('ErtWorldBuilder.cpp', c)

# Renderer: ekran-fazo GI, SSR, AO, kontakt soyalar (Intel GPU: Lumen/RT emas)
ini = 'D:/Unreal_projects/Ertugrul/Config/DefaultEngine.ini'
s = io.open(ini, encoding='utf-8').read()
s = rep(s, "r.DynamicGlobalIlluminationMethod=0", "r.DynamicGlobalIlluminationMethod=2")
if "r.AmbientOcclusionLevels" not in s:
    s = s.replace("[/Script/Engine.RendererSettings]\n", "[/Script/Engine.RendererSettings]\nr.AmbientOcclusionLevels=2\nr.AmbientOcclusionRadiusScale=1.5\nr.DefaultFeature.AmbientOcclusion=True\nr.DefaultFeature.AmbientOcclusionStaticFraction=True\nr.Shadow.MaxCSMResolution=4096\nr.Shadow.CSM.MaxCascades=4\nr.Shadow.DistanceScale=1.5\nr.ContactShadows=1\nr.SSR.Quality=3\nr.SSGI.Quality=3\n", 1)
io.open(ini, 'w', encoding='utf-8', newline='\n').write(s)
print('patched')
