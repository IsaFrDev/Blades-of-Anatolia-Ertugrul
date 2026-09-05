# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# ---------------- ErtProcMesh.h: to'rga yopishtirish ----------------
h = load('ErtProcMesh.h')
h = rep(h, "namespace ErtCol\n{", "/** Grid Snapping: qiymatni to'rga yopishtirish (Grid metr yoki sm - chaqiruvchi birligida) */\ninline float ErtSnap(float V, float Grid) { return Grid > 0.f ? FMath::RoundToFloat(V / Grid) * Grid : V; }\ninline FVector2D ErtSnap(const FVector2D& V, float Grid) { return FVector2D(ErtSnap(V.X, Grid), ErtSnap(V.Y, Grid)); }\n\nnamespace ErtCol\n{")
save('ErtProcMesh.h', h)

# ---------------- ErtWorldBuilder.h ----------------
h = load('ErtWorldBuilder.h')
h = rep(h, "\tUFUNCTION(BlueprintPure, Category = \"Ertugrul\") static bool IsDesert(float E, float N) { return N < -700.f + 60.f; }",
"""	UFUNCTION(BlueprintPure, Category = "Ertugrul") static bool IsDesert(float E, float N) { return N < -700.f + 60.f; }
	/** Relyef rangi (xarita uchun) */
	FLinearColor ColorAt(float E, float N) const;
	/** Gulxan/mash'ala joylari (dunyo sm, W = masshtab) - BeginPlay da olov effektlari spawn qilinadi */
	TArray<FVector4> FireSpots;
	/** Spline devor segmentlari (reja m): A, B, Z (poydevor), H - dekallar va o't-o'lan uchun */
	struct FWallSeg { FVector2D A, B; float Z, H; };
	TArray<FWallSeg> WallSegs;""")
h = rep(h, "\tvoid BuildProps();\n", "\tvoid BuildProps();\n\tvoid BuildSplineWalls();\n\tvoid BuildDecals();\n\tvoid BuildShoreFoliage();\n\tUPROPERTY(Transient) TArray<TObjectPtr<class UDecalComponent>> Decals;\n\tUPROPERTY(Transient) TObjectPtr<UMaterialInterface> DecalMat;\n")
h = rep(h, "\tvoid AddHouse(FErtMeshData& M, float E, float N, float Z, float HU, float HV, float H, float Yaw, const FLinearColor& C, int32 S);\n};",
"""	void AddHouse(FErtMeshData& M, float E, float N, float Z, float HU, float HV, float H, float Yaw, const FLinearColor& C, int32 S);
	/** Spline Mesh devor: nuqtalar (reja m, 1 m to'rga yopishtiriladi) bo'ylab 4 m li modullar takrorlanadi, burchaklarda minora, tepada tishlar */
	void AddWallSpline(FErtMeshData& M, const TArray<FVector2D>& Pts, float H, float Thick, const FLinearColor& Col, bool bBattlements, bool bTowers, int32 S);
};""")
save('ErtWorldBuilder.h', h)

# ---------------- ErtWorldBuilder.cpp ----------------
c = load('ErtWorldBuilder.cpp')
c = rep(c, '#include "Components/PointLightComponent.h"\n', '#include "Components/PointLightComponent.h"\n#include "Components/DecalComponent.h"\n#include "ErtFire.h"\n')
c = rep(c, "\tM.AddCone(W(E, N, Z + 0.2f), 0.45f, 1.1f, 6, Flame);\n\tM.AddCone(W(E + 0.15f, N - 0.1f, Z + 0.2f), 0.3f, 1.5f, 5, Ember);\n\tif (bLight)",
        "\tM.AddCone(W(E, N, Z + 0.2f), 0.35f, 0.5f, 6, Ember);   // cho'g' (olov tili protsedural effekt bilan)\n\tFireSpots.Add(FVector4(W(E, N, Z + 0.25f), bLight ? 1.f : 0.75f));\n\tif (bLight)")
c = rep(c, "void AErtWorldBuilder::BeginPlay()\n{\n\tSuper::BeginPlay();\n\tif (!bBuilt) Build();\n}",
"""void AErtWorldBuilder::BeginPlay()
{
	Super::BeginPlay();
	if (!bBuilt) Build();
	// Olov effektlari (olov tili + tutun + uchqun + miltillovchi nur)
	int32 NF = 0;
	for (const FVector4& F : FireSpots) if (AErtFireFx::Spawn(GetWorld(), FVector(F.X, F.Y, F.Z), F.W, true)) ++NF;
	UE_LOG(LogErtugrul, Log, TEXT("Olov effektlari: %d"), NF);
}

FLinearColor AErtWorldBuilder::ColorAt(float E, float N) const
{
	const float H = HeightAt(E, N);
	const FVector Nm = TerrainNormal(E, N);
	return TerrainColor(E, N, H, 1.f - Nm.Z);
}""")
c = rep(c, "\tif (bBuildForest) { BuildForest(); BuildRocks(); BuildGrass(); BuildProps(); }",
        "\tif (bBuildSettlements) BuildSplineWalls();\n\tif (bBuildForest) { BuildForest(); BuildRocks(); BuildGrass(); BuildShoreFoliage(); BuildProps(); }\n\tif (bBuildSettlements) BuildDecals();")
c += r'''

// ---------------- Spline devorlar (Grid Snapping + Spline Mesh uslubi), dekallar, qirg'oq o'simliklari ----------------

void AErtWorldBuilder::AddWallSpline(FErtMeshData& M, const TArray<FVector2D>& InPts, float H, float Thick, const FLinearColor& Col, bool bBattlements, bool bTowers, int32 S)
{
	if (InPts.Num() < 2) return;
	TArray<FVector2D> Pts; for (const FVector2D& P : InPts) Pts.Add(ErtSnap(P, 1.f));   // 1 m to'r
	const float Module = 4.f;   // takrorlanuvchi devor bo'lagi uzunligi (m)
	for (int32 i = 0; i + 1 < Pts.Num(); ++i)
	{
		const FVector2D A = Pts[i], B = Pts[i + 1];
		const float Len = FVector2D::Distance(A, B); if (Len < 0.5f) continue;
		const FVector2D Dir = (B - A) / Len;
		const int32 NMod = FMath::Max(1, FMath::RoundToInt(Len / Module));
		const float ML = Len / NMod;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
		for (int32 m = 0; m < NMod; ++m)
		{
			const FVector2D C0 = A + Dir * (m + 0.5f) * ML;
			const float Z0 = FMath::Min(HeightAt(A.X + Dir.X * m * ML, A.Y + Dir.Y * m * ML), HeightAt(A.X + Dir.X * (m + 1) * ML, A.Y + Dir.Y * (m + 1) * ML)) - 0.6f;
			const float Zt = HeightAt(C0.X, C0.Y) + H;
			const FLinearColor Cm = ErtCol::Vary(Col, 0.08f, S + i * 97 + m);
			// Modul: egilgan spline bo'ylab joylashgan quti (devorning o'zi)
			M.AddBox(W(C0.X, C0.Y, (Z0 + Zt) * 0.5f), FVector(ML * 0.5f * 100.f + 1.f, Thick * 50.f, (Zt - Z0) * 50.f), Cm, FRotator(0, Yaw, 0));
			if (bBattlements)
				for (int32 t = 0; t < 2; ++t)
				{
					const FVector2D Ct = C0 + Dir * ((t - 0.5f) * ML * 0.5f);
					M.AddBox(W(Ct.X, Ct.Y, Zt + 0.35f), FVector(ML * 0.2f * 100.f, Thick * 50.f, 35.f), Cm * 0.95f, FRotator(0, Yaw, 0));
				}
			WallSegs.Add({ A + Dir * m * ML, A + Dir * (m + 1) * ML, Z0 + 0.6f, Zt - Z0 });
		}
	}
	if (bTowers)
		for (int32 i = 0; i < Pts.Num(); ++i)
		{
			if (i > 0 && i + 1 < Pts.Num()) { const FVector2D D0 = (Pts[i] - Pts[i - 1]).GetSafeNormal(), D1 = (Pts[i + 1] - Pts[i]).GetSafeNormal(); if (FVector2D::DotProduct(D0, D1) > 0.9f) continue; }   // to'g'ri chiziqda minora yo'q
			const float Zb = HeightAt(Pts[i].X, Pts[i].Y) - 0.6f;
			M.AddCylinder(W(Pts[i].X, Pts[i].Y, Zb), Thick * 1.4f, Thick * 1.3f, H + 1.8f + 0.6f, 10, ErtCol::Vary(Col, 0.06f, S + i), true, FRotator::ZeroRotator, 0.03f, S + i);
			M.AddCone(W(Pts[i].X, Pts[i].Y, Zb + H + 2.4f), Thick * 1.5f, 1.2f, 10, ErtCol::Sty(FLinearColor(0.42f, 0.28f, 0.18f), ErtCol::StyleRoof));
		}
}

void AErtWorldBuilder::BuildSplineWalls()
{
	FErtMeshData M(100.f);
	const FLinearColor DryStone = ErtCol::Sty(FLinearColor(0.58f, 0.55f, 0.48f), ErtCol::StyleStone), Curtain = ErtCol::Sty(FLinearColor(0.50f, 0.48f, 0.45f), ErtCol::StyleStone), Quay = ErtCol::Sty(FLinearColor(0.55f, 0.56f, 0.55f), ErtCol::StyleStone);
	// So'g'ut: uzumzor devori (past quruq tosh, egri spline)
	AddWallSpline(M, { {SogE - 95.f, SogN + 20.f}, {SogE - 80.f, SogN + 45.f}, {SogE - 55.f, SogN + 62.f}, {SogE - 20.f, SogN + 70.f}, {SogE + 25.f, SogN + 66.f} }, 1.4f, 0.6f, DryStone, false, false, 3100);
	// Domaniç: qo'ra (halqa)
	{
		TArray<FVector2D> Ring; for (int32 i = 0; i <= 8; ++i) { const float A = 2.f * PI * i / 8; Ring.Add(FVector2D(DomE + 40.f + FMath::Cos(A) * 14.f, DomN - 30.f + FMath::Sin(A) * 14.f)); }
		AddWallSpline(M, Ring, 1.2f, 0.5f, DryStone, false, false, 3200);
	}
	// Bagras: yo'l bo'yidagi tashqi qo'rg'on devori (tishli, minorali)
	AddWallSpline(M, { {FortE - 150.f, FortN - 210.f}, {FortE - 120.f, FortN - 180.f}, {FortE - 95.f, FortN - 140.f}, {FortE - 80.f, FortN - 100.f} }, 4.5f, 1.4f, Curtain, true, true, 3300);
	// Nikeya: Askaniya ko'li qirg'og'idagi tosh qirg'oq devori (yoy)
	{
		TArray<FVector2D> Arc; for (int32 i = 0; i <= 10; ++i) { const float A = FMath::DegreesToRadians(200.f + 14.f * i); Arc.Add(FVector2D(AskE + FMath::Cos(A) * (AskR + 7.f), AskN + FMath::Sin(A) * (AskR + 7.f))); }
		AddWallSpline(M, Arc, 1.1f, 0.8f, Quay, false, false, 3400);
	}
	// Konya: shahar tashqarisidagi karvon yo'li devori
	AddWallSpline(M, { {KonE - 200.f, KonN - 40.f}, {KonE - 175.f, KonN - 10.f}, {KonE - 160.f, KonN + 30.f}, {KonE - 165.f, KonN + 70.f} }, 3.2f, 1.1f, Curtain, true, true, 3500);
	// Qayi obasi: janubiy chegara devori (yog'och ustunli tosh)
	AddWallSpline(M, { {ObaE - 130.f, ObaN - 140.f}, {ObaE - 90.f, ObaN - 150.f}, {ObaE - 40.f, ObaN - 152.f}, {ObaE + 10.f, ObaN - 148.f} }, 1.6f, 0.7f, DryStone, false, false, 3600);
	M.Commit(NewPart(TEXT("SplineWalls"), true), 0, true);
	UE_LOG(LogErtugrul, Log, TEXT("Spline devorlar: %d modul"), WallSegs.Num());
}

void AErtWorldBuilder::BuildDecals()
{
	DecalMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtDecal.M_ErtDecal"));
	if (!DecalMat) { UE_LOG(LogErtugrul, Warning, TEXT("M_ErtDecal yo'q - dekallar o'tkazib yuborildi (ert_make_decal.py)")); return; }
	FRandomStream RS(Seed + 41);
	int32 N = 0;
	auto Place = [&](const FVector& Pos, float Yaw, float Size, float Kind)
	{
		UDecalComponent* D = NewObject<UDecalComponent>(this, *FString::Printf(TEXT("Decal_%d"), N));
		D->SetupAttachment(RootComponent);
		D->SetDecalMaterial(DecalMat);
		D->DecalSize = FVector(40.f, Size, Size);
		D->SetWorldLocation(Pos);
		D->SetWorldRotation(FRotator(0.f, Yaw, RS.FRandRange(0.f, 360.f)));
		D->SetFadeScreenSize(0.005f);
		D->RegisterComponent();
		if (UMaterialInstanceDynamic* MID = D->CreateDynamicMaterialInstance()) { MID->SetScalarParameterValue(TEXT("Kind"), Kind); MID->SetScalarParameterValue(TEXT("Seed"), RS.FRandRange(0.f, 10.f)); }
		Decals.Add(D); ++N;
	};
	// Spline devorlar: har modulda 55% ehtimol bilan mog'or (pastda) yoki yoriq (o'rtada), ikki tomonda
	for (const FWallSeg& Sg : WallSegs)
	{
		if (RS.FRand() > 0.55f) continue;
		const FVector2D Dir = (Sg.B - Sg.A).GetSafeNormal(), Nrm(-Dir.Y, Dir.X);
		const FVector2D C = (Sg.A + Sg.B) * 0.5f + Dir * RS.FRandRange(-1.2f, 1.2f);
		const bool bMoss = RS.FRand() < 0.6f;
		const float Zc = Sg.Z + (bMoss ? 0.35f : Sg.H * RS.FRandRange(0.4f, 0.75f));
		const float Side = RS.FRand() < 0.5f ? 1.f : -1.f;
		const FVector2D P = C + Nrm * Side * 0.05f;
		// Dekal X o'qi devorga qarab (proyeksiya yo'nalishi): Nrm * -Side
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(-Side * Nrm.Y, -Side * Nrm.X)) ;
		Place(W(P.X, P.Y, Zc) + FVector(0, 0, 0), FMath::RadiansToDegrees(FMath::Atan2(-Side * Nrm.Y, -Side * Nrm.X)), RS.FRandRange(70.f, 140.f), bMoss ? 0.f : 1.f);
	}
	// Uylar/o'tovlar (Interiors): tashqi devor tagida mog'or/dog'
	for (const FVector4& In : Interiors)
	{
		if (RS.FRand() > 0.5f) continue;
		const float A = RS.FRand() * 2.f * PI;
		const float R = In.Z + 0.02f;
		const FVector2D P(In.X + FMath::Cos(A) * R, In.Y + FMath::Sin(A) * R);
		Place(W(P.X, P.Y, In.W + 0.4f), FMath::RadiansToDegrees(A) + 180.f, RS.FRandRange(60.f, 110.f), RS.FRand() < 0.7f ? 0.f : 2.f);
	}
	UE_LOG(LogErtugrul, Log, TEXT("Dekallar: %d"), N);
}

void AErtWorldBuilder::BuildShoreFoliage()
{
	FRandomStream RS(Seed + 53);
	FErtMeshData G(1.f), Pebbles(100.f);
	const FLinearColor Reed(0.38f, 0.52f, 0.20f), ReedD(0.30f, 0.42f, 0.16f), Pebble = ErtCol::Sty(FLinearColor(0.55f, 0.53f, 0.48f), ErtCol::StyleRock);
	int32 Clumps = 0, Stones = 0;
	auto Clump = [&](float E, float N, float Hgt, const FLinearColor& C)
	{
		const float H = HeightAt(E, N);
		float SurfZ; if (IsWater(E, N, SurfZ) && H < SurfZ - 0.1f) return;
		const FVector Base = W(E, N, H - 0.02f);
		const float Yaw0 = RS.FRandRange(0.f, 180.f);
		for (int32 b = 0; b < 4; ++b) ErtAddBlade(G, Base + FVector(RS.FRandRange(-14.f, 14.f), RS.FRandRange(-14.f, 14.f), 0), Yaw0 + b * 45.f, Hgt * RS.FRandRange(0.7f, 1.15f), RS.FRandRange(30.f, 55.f), ErtCol::Vary(C, 0.12f, Clumps), RS.FRandRange(-10.f, 10.f));
		++Clumps;
	};
	auto Stone = [&](float E, float N)
	{
		const float H = HeightAt(E, N);
		Pebbles.AddSphere(W(E, N, H - 0.05f), RS.FRandRange(0.08f, 0.22f), 5, ErtCol::Vary(Pebble, 0.15f, Stones), FVector(1.f, RS.FRandRange(0.7f, 1.f), 0.6f), 0.2f, Stones);
		++Stones;
	};
	// Ko'l va voha qirg'oqlari: qamish + mayda toshlar (halqa)
	struct FLk { float E, N, R; };
	for (const FLk& L : { FLk{LakeE, LakeN, LakeR}, FLk{AskE, AskN, AskR}, FLk{OasisE, OasisN, OasisR} })
		for (int32 i = 0; i < 260; ++i)
		{
			const float A = RS.FRand() * 2.f * PI, R = L.R + RS.FRandRange(4.5f, 12.f);
			const float E = L.E + FMath::Cos(A) * R, N = L.N + FMath::Sin(A) * R;
			if (!IsBuildable(E, N)) continue;
			if (RS.FRand() < 0.7f) Clump(E, N, RS.FRandRange(45.f, 80.f), RS.FRand() < 0.5f ? Reed : ReedD); else Stone(E, N);
		}
	// Daryo bo'ylari
	for (float N = -980.f; N < 980.f; N += 3.f)
	{
		if (N < DesertN + 20.f) continue;
		for (int32 s = -1; s <= 1; s += 2)
		{
			if (RS.FRand() > 0.55f) continue;
			const float E = RiverE(N) + s * RS.FRandRange(27.f, 36.f);
			float Wd = 0.f; if (RoadDist(E, N, &Wd) < Wd * 0.5f + 1.f) continue;
			if (RS.FRand() < 0.75f) Clump(E, N, RS.FRandRange(40.f, 75.f), Reed); else Stone(E, N);
		}
	}
	// Spline devor tagi: maysa va toshchalar (ikki tomon)
	for (const FWallSeg& Sg : WallSegs)
	{
		const FVector2D Dir = (Sg.B - Sg.A).GetSafeNormal(), Nrm(-Dir.Y, Dir.X);
		for (int32 k = 0; k < 3; ++k)
		{
			const FVector2D P = Sg.A + Dir * RS.FRandRange(0.f, FVector2D::Distance(Sg.A, Sg.B)) + Nrm * (RS.FRand() < 0.5f ? 1.f : -1.f) * RS.FRandRange(0.5f, 1.4f);
			if (RS.FRand() < 0.7f) Clump(P.X, P.Y, RS.FRandRange(25.f, 45.f), ReedD); else Stone(P.X, P.Y);
		}
	}
	if (G.Verts.Num()) { UProceduralMeshComponent* P = NewPart(TEXT("ShoreGrass"), false, GrassMat ? GrassMat : Mat); G.Commit(P, 0, false); P->SetCastShadow(false); }
	if (Pebbles.Verts.Num()) Pebbles.Commit(NewPart(TEXT("Pebbles"), false), 0, false);
	UE_LOG(LogErtugrul, Log, TEXT("Qirg'oq/devor o'simliklari: %d tup, %d tosh"), Clumps, Stones);
}
'''
save('ErtWorldBuilder.cpp', c)

# ---------------- ErtWeather.cpp: hajmli tuman + mahalliy tuman hajmlari ----------------
w = load('ErtWeather.cpp')
w = rep(w, '#include "Engine/ExponentialHeightFog.h"\n', '#include "Engine/ExponentialHeightFog.h"\n#include "Components/LocalFogVolumeComponent.h"\n#include "ErtWorldBuilder.h"\n')
w = rep(w, "\t\tF->SecondFogData.FogHeightOffset = -40000.f;\n\t\tF->MarkRenderStateDirty();\n\t}",
"""		F->SecondFogData.FogHeightOffset = -40000.f;
		// Volumetric Fog: nur shu'lalari va suv/o'rmon ustidagi hajmli tuman
		F->SetVolumetricFog(true);
		F->SetVolumetricFogScatteringDistribution(0.45f);
		F->SetVolumetricFogExtinctionScale(1.6f);
		F->VolumetricFogDistance = 14000.f;
		F->VolumetricFogAlbedo = FColor(235, 238, 245);
		F->MarkRenderStateDirty();
	}
	// Mahalliy tuman hajmlari: ko'llar, voha, o'rmon (radius = masshtab, sm)
	{
		using namespace ErtMap;
		struct FLv { float E, N, Z, R, Ext; };
		const FLv Vols[] = { {LakeE, LakeN, LakeZ, 9000.f, 0.6f}, {AskE, AskN, AskZ, 11000.f, 0.5f}, {OasisE, OasisN, OasisZ, 8000.f, 0.35f}, {-330.f, 700.f, 24.f, 16000.f, 0.35f}, {DomE, DomN, DomH, 12000.f, 0.25f} };
		int32 i = 0;
		for (const FLv& V : Vols)
		{
			ULocalFogVolumeComponent* L = NewObject<ULocalFogVolumeComponent>(this, *FString::Printf(TEXT("LocalFog_%d"), i++));
			L->SetupAttachment(RootComponent);
			L->RadialFogExtinction = V.Ext;
			L->HeightFogExtinction = V.Ext * 1.6f;
			L->HeightFogFalloff = 600.f;
			L->HeightFogOffset = 0.f;
			L->FogAlbedo = FLinearColor(0.85f, 0.9f, 0.97f);
			L->FogPhaseG = 0.3f;
			L->RegisterComponent();
			L->SetWorldLocation(AErtWorldBuilder::PlanToWorld(V.E, V.N, V.Z + 2.f));
			L->SetWorldScale3D(FVector(V.R, V.R, V.R * 0.25f));
		}
	}""")
save('ErtWeather.cpp', w)

# ---------------- ErtEnemy: jamoa (0 dushman, 1 ittifoqchi alp), umumiy nishon ----------------
h = load('ErtEnemy.h')
h = rep(h, "\tvoid Init(EErtEnemyKind InKind, const FVector& Home, float PatrolRadius);",
        "\tvoid Init(EErtEnemyKind InKind, const FVector& Home, float PatrolRadius);\n\t/** Jamoa: 0 dushman, 1 ittifoqchi (Qayi alplari) - Init dan oldin qo'yiladi */\n\tint32 Team = 0;\n\tbool IsAlly() const { return Team == 1; }\n\t/** Hozirgi raqib (o'yinchi yoki boshqa jamoa) */\n\tAActor* CurrentTarget() const { return TargetActor.Get(); }")
h = rep(h, "\tbool CanSee(const APawn* Player) const;", "\tbool CanSee(const AActor* Target) const;\n\tTWeakObjectPtr<AActor> TargetActor;\n\tfloat RetargetT = 0.f;\n\tAActor* PickTarget(APawn* Player);")
save('ErtEnemy.h', h)

e = load('ErtEnemy.cpp')
e = rep(e, "\tBody->bSwordInHand = Kind != EErtEnemyKind::Deer && Kind != EErtEnemyKind::Crossbow;\n\tHealth = MaxHealth;",
"""	if (Team == 1)
	{
		// Qayi alplari: oq-jigarrang kaftan, ko'k belbog', qalpoq/dubulg'a
		Body->Kaftan = FLinearColor(0.72f, 0.62f, 0.45f); Body->Trim = FLinearColor(0.15f, 0.25f, 0.5f); Body->Leather = FLinearColor(0.35f, 0.22f, 0.12f); Body->bCloak = false;
		bAlerted = true; MaxHealth *= 1.3f;
	}
	Body->bSwordInHand = Kind != EErtEnemyKind::Deer && Kind != EErtEnemyKind::Crossbow;
	Health = MaxHealth;""")
e = rep(e, "bool AErtEnemy::CanSee(const APawn* Player) const\n{\n\tFHitResult H;\n\tFCollisionQueryParams Q(SCENE_QUERY_STAT(ErtSee), false, this);\n\tQ.AddIgnoredActor(Player);\n\tconst FVector A = GetActorLocation() + FVector(0, 0, 60), B = Player->GetActorLocation() + FVector(0, 0, 40);",
        "bool AErtEnemy::CanSee(const AActor* Target) const\n{\n\tFHitResult H;\n\tFCollisionQueryParams Q(SCENE_QUERY_STAT(ErtSee), false, this);\n\tQ.AddIgnoredActor(Target);\n\tconst FVector A = GetActorLocation() + FVector(0, 0, 60), B = Target->GetActorLocation() + FVector(0, 0, 40);")
# TickGuard: nishon tanlash
e = rep(e, "\tAErtCharacter* Hero = Cast<AErtCharacter>(Player);\n\tconst bool bHeroAlive = Hero && !Hero->IsDead();\n\tconst float DP = bHeroAlive ? FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation()) : 1e9f;\n\n\t// Kechiktirilgan zarba (animatsiya o'rtasida tegadi)\n\tif (HitPending >= 0.f)\n\t{\n\t\tHitPending -= Dt;\n\t\tif (HitPending < 0.f && bHeroAlive && DP < AttackRange + 80.f) { Hero->ReceiveHit(bHeavyPending ? AttackDamage * 2.f : AttackDamage, GetActorLocation(), this, bHeavyPending); bHeavyPending = false; }\n\t}\n\n\tif (Mount && bAlerted) { TickRider(Dt, bHeroAlive ? Hero : nullptr, DP); return; }",
"""	// Nishon: o'yinchi (dushman uchun) yoki boshqa jamoa a'zosi (urush) - har 0.5 s qayta tanlanadi
	RetargetT -= Dt;
	if (RetargetT <= 0.f || !TargetActor.IsValid()) { RetargetT = 0.5f; TargetActor = PickTarget(Player); }
	AActor* Tgt = TargetActor.Get();
	AErtCharacter* Hero = Cast<AErtCharacter>(Tgt);
	AErtEnemy* Foe = Cast<AErtEnemy>(Tgt);
	const bool bHeroAlive = (Hero && !Hero->IsDead()) || (Foe && !Foe->IsDead());
	const float DP = bHeroAlive ? FVector::Dist2D(Tgt->GetActorLocation(), GetActorLocation()) : 1e9f;
	auto DealHit = [&](float Dmg, bool bHeavy) { if (Hero) Hero->ReceiveHit(Dmg, GetActorLocation(), this, bHeavy); else if (Foe) { Foe->ApplyHit(Dmg, this, bHeavy); AErtBurst::Blood(GetWorld(), Foe->GetActorLocation() + FVector(0, 0, 60.f), (Foe->GetActorLocation() - GetActorLocation()).GetSafeNormal2D(), 0.7f); } };

	// Kechiktirilgan zarba (animatsiya o'rtasida tegadi)
	if (HitPending >= 0.f)
	{
		HitPending -= Dt;
		if (HitPending < 0.f && bHeroAlive && DP < AttackRange + 80.f) { DealHit(bHeavyPending ? AttackDamage * 2.f : AttackDamage, bHeavyPending); bHeavyPending = false; }
	}

	if (Mount && bAlerted && Hero) { TickRider(Dt, bHeroAlive ? Hero : nullptr, DP); return; }
	if (Team == 1 && !bHeroAlive)
	{
		// Ittifoqchi: raqib yo'q - o'yinchiga ergashadi (4-7 m orqada)
		if (APawn* Pl = Player) { const float Dpl = FVector::Dist2D(Pl->GetActorLocation(), GetActorLocation()); if (Dpl > 700.f) MoveToward(Pl->GetActorLocation(), Dpl > 1800.f ? MoveSpeed * 1.25f : MoveSpeed); else if (Dpl > 420.f) MoveToward(Pl->GetActorLocation(), 220.f); }
		return;
	}
	if (Hero) Hero = bHeroAlive ? Hero : nullptr;""")
e = rep(e, "\tif (!bAlerted && bHeroAlive)\n\t{\n\t\tconst FVector To = (Hero->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();\n\t\tconst float Facing = FVector::DotProduct(GetActorForwardVector(), To);\n\t\tconst float SeeRange = Mount ? 2600.f : (Hero->bIsCrouched ? 900.f : 1400.f);\n\t\tif ((DP < SeeRange && Facing > 0.35f && CanSee(Hero)) || DP < 320.f) bAlerted = true;\n\t}",
"""	if (!bAlerted && bHeroAlive)
	{
		const FVector To = (Tgt->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float Facing = FVector::DotProduct(GetActorForwardVector(), To);
		const float SeeRange = Mount ? 2600.f : ((Hero && Hero->bIsCrouched) ? 900.f : 1400.f);
		if ((DP < SeeRange && Facing > 0.35f && CanSee(Tgt)) || DP < 320.f) bAlerted = true;
	}""")
e = rep(e, "\tif (bAlerted && bHeroAlive && Kind == EErtEnemyKind::Footman && Health < MaxHealth * 0.3f)\n\t{\n\t\t// Or/iymon yuqori bo'lsa oddiy askarlar qo'rqib qochadi\n\t\tif (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))\n\t\t\tif (GM->GetHonor() >= 15) { MoveToward(GetActorLocation() + (GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * 600.f, MoveSpeed); return; }\n\t}",
"""	if (bAlerted && Hero && Team == 0 && Kind == EErtEnemyKind::Footman && Health < MaxHealth * 0.3f)
	{
		// Or/iymon yuqori bo'lsa oddiy askarlar qo'rqib qochadi
		if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))
			if (GM->GetHonor() >= 15) { MoveToward(GetActorLocation() + (GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * 600.f, MoveSpeed); return; }
	}""")
# Crossbow va yaqin jang: Hero-> o'rniga Tgt->
e = rep(e, "\t\tif (Kind == EErtEnemyKind::Crossbow)\n\t\t{\n\t\t\tif (DP > AttackRange) MoveToward(Hero->GetActorLocation(), MoveSpeed);\n\t\t\telse if (DP < 500.f) MoveToward(GetActorLocation() + (GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * 400.f, MoveSpeed * 0.8f);\n\t\t\telse SetActorRotation(FRotator(0, (Hero->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));\n\t\t\tif (DP <= AttackRange && AttackCD <= 0.f && CanSee(Hero))",
        "\t\tif (Kind == EErtEnemyKind::Crossbow)\n\t\t{\n\t\t\tif (DP > AttackRange) MoveToward(Tgt->GetActorLocation(), MoveSpeed);\n\t\t\telse if (DP < 500.f) MoveToward(GetActorLocation() + (GetActorLocation() - Tgt->GetActorLocation()).GetSafeNormal2D() * 400.f, MoveSpeed * 0.8f);\n\t\t\telse SetActorRotation(FRotator(0, (Tgt->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));\n\t\t\tif (DP <= AttackRange && AttackCD <= 0.f && CanSee(Tgt))")
e = rep(e, "\t\t\t\tconst FVector Target = Hero->GetActorLocation() + Hero->GetVelocity() * 0.35f + FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 0);",
        "\t\t\t\tconst FVector Target = Tgt->GetActorLocation() + Tgt->GetVelocity() * 0.35f + FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 0);")
e = rep(e, "\t\tif (DP > AttackRange * 0.85f) MoveToward(Hero->GetActorLocation(), MoveSpeed);\n\t\telse SetActorRotation(FRotator(0, (Hero->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));",
        "\t\tif (DP > AttackRange * 0.85f) MoveToward(Tgt->GetActorLocation(), MoveSpeed);\n\t\telse SetActorRotation(FRotator(0, (Tgt->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));")
e += r'''

AActor* AErtEnemy::PickTarget(APawn* Player)
{
	AErtCharacter* Hero = Cast<AErtCharacter>(Player);
	const bool bHeroAlive = Hero && !Hero->IsDead();
	AActor* Best = nullptr; float BestD = Team == 1 ? 3200.f : 2600.f;
	// Boshqa jamoa a'zolari (urush)
	TArray<AActor*> All; UGameplayStatics::GetAllActorsOfClass(this, AErtEnemy::StaticClass(), All);
	for (AActor* A : All)
	{
		AErtEnemy* E = Cast<AErtEnemy>(A);
		if (!E || E == this || E->IsDead() || E->IsAnimal() || E->Team == Team) continue;
		const float D = FVector::Dist2D(E->GetActorLocation(), GetActorLocation());
		if (D < BestD) { BestD = D; Best = E; }
	}
	if (Team == 0 && bHeroAlive)
	{
		// Dushman: o'yinchi yaqinroq bo'lsa (yoki allaqachon sezgan bo'lsa) o'yinchini tanlaydi
		const float Dh = FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation());
		if (!Best || Dh < BestD * 1.1f || (TargetActor.Get() == Hero && Dh < 1500.f)) Best = Hero;
	}
	return Best;
}
'''
# ApplyHit: Killer/XP - ittifoqchi o'ldirsa o'yinchiga XP yo'q (Killer o'zi)
save('ErtEnemy.cpp', e)

# ---------------- ErtCharacter: ittifoqchilarga zarba/qulf yo'q; xarita aylantirish ----------------
c = load('ErtCharacter.cpp')
c = rep(c, "\t\t\tAErtEnemy* E = Cast<AErtEnemy>(R.GetActor());\n\t\t\tif (!E || Done.Contains(E)) continue;", "\t\t\tAErtEnemy* E = Cast<AErtEnemy>(R.GetActor());\n\t\t\tif (!E || Done.Contains(E) || E->IsAlly()) continue;")
c = rep(c, "\t\tif (!E || E->IsDead() || E->IsAnimal()) continue;\n\t\tconst FVector To = E->GetActorLocation() - GetActorLocation();\n\t\tconst float D = To.Size2D(); if (D > 1800.f) continue;",
        "\t\tif (!E || E->IsDead() || E->IsAnimal() || E->IsAlly()) continue;\n\t\tconst FVector To = E->GetActorLocation() - GetActorLocation();\n\t\tconst float D = To.Size2D(); if (D > 1800.f) continue;")
c = rep(c, "void AErtCharacter::OnMenuLeft() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (GM->IsSettingsOpen()) GM->SettingsAdjust(-1); } }\nvoid AErtCharacter::OnMenuRight() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (GM->IsSettingsOpen()) GM->SettingsAdjust(1); } }",
        "void AErtCharacter::OnMenuLeft() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (GM->IsSettingsOpen()) GM->SettingsAdjust(-1); else if (GM->GetMenu() == EErtMenu::Map) GM->MapRotate(-30.f); } }\nvoid AErtCharacter::OnMenuRight() { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { if (GM->IsSettingsOpen()) GM->SettingsAdjust(1); else if (GM->GetMenu() == EErtMenu::Map) GM->MapRotate(30.f); } }")
# Avtomatik o'yinchi: ittifoqchilarni nishon qilmasin
c = rep(c, "\t\tif (!E || E->IsDead() || E->GetKind() == EErtEnemyKind::Deer) continue;\n\t\tconst float Dd = FVector::Dist2D(E->GetActorLocation(), GetActorLocation());",
        "\t\tif (!E || E->IsDead() || E->GetKind() == EErtEnemyKind::Deer || E->IsAlly()) continue;\n\t\tconst float Dd = FVector::Dist2D(E->GetActorLocation(), GetActorLocation());")
save('ErtCharacter.cpp', c)

# ---------------- ErtArrow: o'yinchi o'qi ittifoqchiga tegmaydi ----------------
a = load('ErtArrow.cpp')
a = rep(a, "\t\t\tAErtEnemy* E = Cast<AErtEnemy>(A);\n\t\t\tif (!E || E->IsDead()) continue;\n\t\t\tconst FVector C = E->GetActorLocation();", "\t\t\tAErtEnemy* E = Cast<AErtEnemy>(A);\n\t\t\tif (!E || E->IsDead() || E->IsAlly()) continue;\n\t\t\tconst FVector C = E->GetActorLocation();")
save('ErtArrow.cpp', a)

# ---------------- ErtMission: ittifoqchilar (urush), kengash bo'sh dialog, yo'l bardoshliligi ----------------
h = load('ErtMission.h')
h = rep(h, "\tconst TArray<TObjectPtr<AErtEnemy>>& GetEnemies() const { return Enemies; }", "\tconst TArray<TObjectPtr<AErtEnemy>>& GetEnemies() const { return Enemies; }\n\tconst TArray<TObjectPtr<AErtEnemy>>& GetAllies() const { return Allies; }\n\tint32 AliveAllies() const;\n\t/** Urush: o'yinchi atrofida N ittifoqchi alp (bir bosqichda bir marta) */\n\tvoid SpawnAllies(int32 N);")
h = rep(h, "\tUPROPERTY(Transient) TArray<TObjectPtr<AErtEnemy>> Enemies;\n", "\tUPROPERTY(Transient) TArray<TObjectPtr<AErtEnemy>> Enemies;\n\tUPROPERTY(Transient) TArray<TObjectPtr<AErtEnemy>> Allies;\n\tvoid ClearAllies();\n")
save('ErtMission.h', h)
m = load('ErtMission.cpp')
m = rep(m, "void AErtMissionDirector::ClearEnemies()\n{", """int32 AErtMissionDirector::AliveAllies() const { int32 N = 0; for (const AErtEnemy* E : Allies) if (E && !E->IsDead()) ++N; return N; }

void AErtMissionDirector::ClearAllies()
{
	for (AErtEnemy* E : Allies) if (E) E->Destroy();
	Allies.Reset();
}

void AErtMissionDirector::SpawnAllies(int32 N)
{
	AErtCharacter* H = Hero();
	if (!H) return;
	FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	int32 Made = 0;
	for (int32 i = 0; i < N; ++i)
	{
		const float A = 2.f * PI * i / N + PI;   // o'yinchi orqasida yarim doira
		const FVector P = GroundAt(H->GetActorLocation().X + FMath::Cos(A) * 380.f, H->GetActorLocation().Y + FMath::Sin(A) * 380.f);
		AErtEnemy* E = GetWorld()->SpawnActor<AErtEnemy>(AErtEnemy::StaticClass(), P + FVector(0, 0, 100.f), FRotator(0, H->GetActorRotation().Yaw, 0), SP);
		if (!E) continue;
		E->Team = 1;
		E->Init(i % 3 == 0 ? EErtEnemyKind::Sergeant : EErtEnemyKind::Footman, P, 0.f);
		Allies.Add(E); ++Made;
	}
	UE_LOG(LogErtugrul, Log, TEXT("[Missiya] urush: %d ittifoqchi alp"), Made);
}

void AErtMissionDirector::ClearEnemies()
{""")
m = rep(m, "\tClearEnemies();\n\tClearPhaseNpcs();\n\tPhases.Reset(); Objectives.Reset(); Waves.Reset();\n\tState = EErtMissionState::Inactive;", "\tClearEnemies();\n\tClearAllies();\n\tClearPhaseNpcs();\n\tPhases.Reset(); Objectives.Reset(); Waves.Reset();\n\tState = EErtMissionState::Inactive;")
# Bosqich boshida: katta jang bosqichlarida (>=3 dushman to'lqinda) ittifoqchilar
m = rep(m, "\tWaveIdx = 0;\n\tSpawnWave(0);\n\tClearPhaseNpcs();", """	WaveIdx = 0;
	SpawnWave(0);
	// Urush: 3+ dushmanli to'lqin bo'lsa Qayi alplari yordamga keladi (SIEGE/DEFENSE da ko'proq)
	{
		int32 MaxW = 0; for (const FErtWave& Wv : Waves) MaxW = FMath::Max(MaxW, Wv.Spawns.Num());
		const bool bBig = EpisodeArchetype == TEXT("SIEGE") || EpisodeArchetype == TEXT("DEFENSE");
		const int32 Want = MaxW >= 3 ? FMath::Clamp(MaxW + (bBig ? 3 : 0), 3, 8) : 0;
		if (Want > AliveAllies()) { ClearAllies(); SpawnAllies(Want); }
		else if (Want == 0) ClearAllies();
	}
	ClearPhaseNpcs();""")
m = rep(m, "\tFString EpisodeId, EpisodeTitle, EpisodeDate, IntroText, PhaseTitle, NextEpisodeId, CpName, Cliffhanger;", "\tFString EpisodeId, EpisodeTitle, EpisodeDate, IntroText, PhaseTitle, NextEpisodeId, CpName, Cliffhanger, EpisodeArchetype;") if "EpisodeArchetype" not in m else m
# EpisodeArchetype ni to'ldirish: BuildPhases boshida
m = rep(m, "void AErtMissionDirector::BuildPhases(const FErtEpisode& E)\n{\n\tPhases.Reset();", "void AErtMissionDirector::BuildPhases(const FErtEpisode& E)\n{\n\tEpisodeArchetype = E.Archetype;\n\tPhases.Reset();")
# Kengash: dialog bo'sh bo'lsa - nuqtaga yaqinlashib 2 s turish kifoya
m = rep(m, "\t\tcase EErtObjKind::Council:\n\t\t\tif (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))\n\t\t\t\tif (GM->LastDialogId == O.DialogId",
"""		case EErtObjKind::Council:
			if (O.DialogId.IsEmpty())
			{
				// Dialog grafi yo'q: kengash nuqtasida 2 s turish kifoya
				if (FVector::Dist2D(PL, O.Point) < 450.f) O.Hold += Dt; else O.Hold = 0.f;
				if (O.Hold > 2.f) { O.bDone = true; UE_LOG(LogErtugrul, Log, TEXT("[Missiya] kengash (dialogsiz) o'tdi")); }
				break;
			}
			if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))
				if (GM->LastDialogId == O.DialogId""")
m = rep(m, "\t\t\t\tconst bool bNear = FVector::Dist2D(PL, T) < O.Radius && (!O.bNeedZ || FeetZ > T.Z - 120.f) && FMath::Abs(FeetZ - T.Z) < 400.f;",
        "\t\t\t\tconst bool bNear = FVector::Dist2D(PL, T) < O.Radius * 1.4f && (!O.bNeedZ || FeetZ > T.Z - 200.f) && FMath::Abs(FeetZ - T.Z) < 700.f;")
save('ErtMission.cpp', m)
# ErtMission.h da EpisodeArchetype maydoni
h = load('ErtMission.h')
h = rep(h, "\tFString EpisodeId, EpisodeTitle, EpisodeDate, IntroText, PhaseTitle, NextEpisodeId, CpName, Cliffhanger;", "\tFString EpisodeId, EpisodeTitle, EpisodeDate, IntroText, PhaseTitle, NextEpisodeId, CpName, Cliffhanger, EpisodeArchetype;")
save('ErtMission.h', h)

# ---------------- ErtDialog: bo'sh id ----------------
d = load('ErtDialog.cpp')
d = rep(d, "bool FErtDialog::Start(const FString& Id, TSet<FString>* InFlags, int32* InHonor)\n{\n\tEnd();", "bool FErtDialog::Start(const FString& Id, TSet<FString>* InFlags, int32* InHonor)\n{\n\tEnd();\n\tif (Id.IsEmpty()) return false;")
save('ErtDialog.cpp', d)

# ---------------- ErtGameMode: grafika presetlari, GPS, 3D xarita ----------------
h = load('ErtGameMode.h')
h = rep(h, "\tint32 Language = 0; float MouseSens = 1.f; bool bInvertY = false;",
"""	int32 Language = 0; float MouseSens = 1.f; bool bInvertY = false;
	/** Grafika preseti: 0 PC Ultra (Lumen HW, VSM High, TSR 100%), 1 PS5/XSX (Lumen SW, VSM Medium, TSR 50-70%), 2 Series S (SSGI, CSM, SSR, TSR 50-60%) */
	int32 GfxPreset = 1;
	void ApplyGfxPreset();
	static const TCHAR* GfxPresetName(int32 P) { return P == 0 ? TEXT("PC Ultra") : (P == 1 ? TEXT("PS5 / Xbox Series X (60 FPS)") : TEXT("Xbox Series S (60 FPS)")); }
	/** 3D xarita aylantirish (chap/o'ng) */
	void MapRotate(float DeltaYaw);
	bool bGps = true;""")
save('ErtGameMode.h', h)
g = load('ErtGameMode.cpp')
g = rep(g, '#include "ErtGameMode.h"\n', '#include "ErtGameMode.h"\n#include "ErtNav.h"\n#include "ErtMap3D.h"\n')
g = rep(g, "\tSettingsRow = (SettingsRow + Delta + 8) % 8;", "\tSettingsRow = (SettingsRow + Delta + 9) % 9;")
g = rep(g, "\tif (SettingsRow == 3) { SettingsPage = 1; KeyRow = 0; return; }\n\tUGameUserSettings* GS", "\tif (SettingsRow == 3) { SettingsPage = 1; KeyRow = 0; return; }\n\tif (SettingsRow == 8) { GfxPreset = (GfxPreset + Delta + 3) % 3; ApplyGfxPreset(); return; }\n\tUGameUserSettings* GS")
g = rep(g, "\tR->SetBoolField(TEXT(\"invert_y\"), bInvertY);", "\tR->SetBoolField(TEXT(\"invert_y\"), bInvertY);\n\tR->SetNumberField(TEXT(\"gfx\"), GfxPreset);")
g = rep(g, "\t\tR->TryGetNumberField(TEXT(\"language\"), Language);", "\t\tR->TryGetNumberField(TEXT(\"language\"), Language);\n\t\tR->TryGetNumberField(TEXT(\"gfx\"), GfxPreset);")
g = rep(g, "\tbUnlockAll = FParse::Param(FCommandLine::Get(), TEXT(\"ErtUnlockAll\"));",
"""	bUnlockAll = FParse::Param(FCommandLine::Get(), TEXT("ErtUnlockAll"));
	{
		FString Gfx; if (FParse::Value(FCommandLine::Get(), TEXT("-ErtGfx="), Gfx)) GfxPreset = Gfx == TEXT("ultra") ? 0 : (Gfx == TEXT("low") || Gfx == TEXT("s") ? 2 : 1);
		ApplyGfxPreset();
		if (FParse::Param(FCommandLine::Get(), TEXT("ErtNoGps"))) bGps = false;
	}""")
g = rep(g, "void AErtGameMode::MenuMove(int32 Delta)\n{\n\tif (Dialog.IsActive()) { Dialog.MoveSelection(Delta); return; }",
        "void AErtGameMode::MenuMove(int32 Delta)\n{\n\tif (Dialog.IsActive()) { Dialog.MoveSelection(Delta); return; }\n\tif (Menu == EErtMenu::Map) { if (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->Zoom(Delta < 0 ? 0.8f : 1.25f); return; }")
g = rep(g, "void AErtGameMode::ToggleMap()\n{\n\tif (Dialog.IsActive() || (Cutscene && Cutscene->IsPlaying())) return;\n\tif (Menu == EErtMenu::Map) OpenMenu(EErtMenu::None);\n\telse if (Menu == EErtMenu::None || Menu == EErtMenu::Pause) OpenMenu(EErtMenu::Map);\n}",
"""void AErtGameMode::ToggleMap()
{
	if (Dialog.IsActive() || (Cutscene && Cutscene->IsPlaying())) return;
	if (Menu == EErtMenu::Map) OpenMenu(EErtMenu::None);
	else if (Menu == EErtMenu::None || Menu == EErtMenu::Pause) OpenMenu(EErtMenu::Map);
	if (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->SetActive(Menu == EErtMenu::Map);
}

void AErtGameMode::MapRotate(float DeltaYaw) { if (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->Rotate(DeltaYaw); }

void AErtGameMode::ApplyGfxPreset()
{
	// Konsol o'zgaruvchilari: preset jadvali (docs/UNREAL_PORT.md)
	auto Cmd = [&](const TCHAR* C) { if (GEngine) GEngine->Exec(GetWorld(), C); };
	switch (GfxPreset)
	{
	case 0:   // PC Ultra: Lumen (Hardware RT), Virtual Shadow Maps High, Lumen RT aks, 100% + TSR (DLSS plagin bo'lsa o'zi almashadi)
		Cmd(TEXT("r.DynamicGlobalIlluminationMethod 1")); Cmd(TEXT("r.Lumen.HardwareRayTracing 1")); Cmd(TEXT("r.ReflectionMethod 1")); Cmd(TEXT("r.Lumen.Reflections.HardwareRayTracing 1"));
		Cmd(TEXT("r.Shadow.Virtual.Enable 1")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional 0")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal 0"));
		Cmd(TEXT("r.AntiAliasingMethod 4")); Cmd(TEXT("r.DynamicRes.OperationMode 0")); Cmd(TEXT("r.ScreenPercentage 100")); Cmd(TEXT("sg.GlobalIlluminationQuality 3")); Cmd(TEXT("sg.ShadowQuality 3")); Cmd(TEXT("sg.ReflectionQuality 3")); Cmd(TEXT("sg.FoliageQuality 3")); Cmd(TEXT("sg.ViewDistanceQuality 3"));
		break;
	case 1:   // PS5 / Series X 60 FPS: Lumen Software, VSM Medium, Lumen SW aks, dinamik 50-70% TSR
		Cmd(TEXT("r.DynamicGlobalIlluminationMethod 1")); Cmd(TEXT("r.Lumen.HardwareRayTracing 0")); Cmd(TEXT("r.ReflectionMethod 1")); Cmd(TEXT("r.Lumen.Reflections.HardwareRayTracing 0"));
		Cmd(TEXT("r.Shadow.Virtual.Enable 1")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional 1")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal 1"));
		Cmd(TEXT("r.AntiAliasingMethod 4")); Cmd(TEXT("r.DynamicRes.OperationMode 2")); Cmd(TEXT("r.DynamicRes.MinScreenPercentage 50")); Cmd(TEXT("r.DynamicRes.MaxScreenPercentage 70")); Cmd(TEXT("r.DynamicRes.FrameTimeBudget 16.6")); Cmd(TEXT("sg.GlobalIlluminationQuality 2")); Cmd(TEXT("sg.ShadowQuality 2")); Cmd(TEXT("sg.ReflectionQuality 2")); Cmd(TEXT("sg.FoliageQuality 2")); Cmd(TEXT("sg.ViewDistanceQuality 2"));
		break;
	default:  // Series S 60 FPS: SSGI (Distance Field/Screen Space), Cascaded Shadows, SSR, dinamik 50-60% TSR
		Cmd(TEXT("r.DynamicGlobalIlluminationMethod 2")); Cmd(TEXT("r.Lumen.HardwareRayTracing 0")); Cmd(TEXT("r.ReflectionMethod 2")); Cmd(TEXT("r.SSR.Quality 2"));
		Cmd(TEXT("r.Shadow.Virtual.Enable 0")); Cmd(TEXT("r.Shadow.CSM.MaxCascades 3"));
		Cmd(TEXT("r.AntiAliasingMethod 4")); Cmd(TEXT("r.DynamicRes.OperationMode 2")); Cmd(TEXT("r.DynamicRes.MinScreenPercentage 50")); Cmd(TEXT("r.DynamicRes.MaxScreenPercentage 60")); Cmd(TEXT("r.DynamicRes.FrameTimeBudget 16.6")); Cmd(TEXT("sg.GlobalIlluminationQuality 1")); Cmd(TEXT("sg.ShadowQuality 1")); Cmd(TEXT("sg.ReflectionQuality 1")); Cmd(TEXT("sg.FoliageQuality 1")); Cmd(TEXT("sg.ViewDistanceQuality 2"));
		break;
	}
	UE_LOG(LogErtugrul, Log, TEXT("Grafika preseti: %s"), GfxPresetName(GfxPreset));
}""")
# Tick: GPS maqsadi = birinchi marker
g = rep(g, "void AErtGameMode::Tick(float Dt)\n{\n\tSuper::Tick(Dt);", """void AErtGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	// GPS: faol maqsadga yo'l
	if (AErtGps* G = AErtGps::Get(GetWorld()))
	{
		FVector T = FVector::ZeroVector;
		if (bGps && Director && Director->GetState() != EErtMissionState::Inactive)
		{
			// Yaqin tirik dushman bo'lsa yo'l kerak emas; aks holda birinchi marker
			TArray<FVector> Ms; Director->GetMarkers(Ms);
			if (Ms.Num()) T = Ms[0];
		}
		G->SetTarget(T);
	}""")
save('ErtGameMode.cpp', g)

# ---------------- ErtHUD: 3D xarita, GPS chizig'i, ittifoqchilar, preset qatori ----------------
hd = load('ErtHUD.cpp')
hd = rep(hd, '#include "ErtHUD.h"\n', '#include "ErtHUD.h"\n#include "ErtNav.h"\n#include "ErtMap3D.h"\n#include "Engine/TextureRenderTarget2D.h"\n')
hd = rep(hd, "\t\tFString::Printf(TEXT(\"VSync:   < %s >\"), *GM->DisplayRow(7)) };\n\tfor (int32 i = 0; i < 8; ++i)",
        "\t\tFString::Printf(TEXT(\"VSync:   < %s >\"), *GM->DisplayRow(7)),\n\t\tFString::Printf(TEXT(\"Render preseti / Preset:   < %s >\"), AErtGameMode::GfxPresetName(GM->GfxPreset)) };\n\tfor (int32 i = 0; i < 9; ++i)")
hd = rep(hd, "\tText(FString::Printf(TEXT(\"Or/iymon: %d    Bajarilgan epizodlar: saqlangan (Saved/ert_save.json)\"), GM->GetHonor()), 44 * Sc, 400 * Sc, Grey, Sc);\n\tText(TEXT(\"Intel GPU uchun tavsiya: Grafika sifati Past yoki O'rta, VSync yoqilgan.\"), 44 * Sc, 424 * Sc, Grey, 0.9f * Sc);",
        "\tText(FString::Printf(TEXT(\"Or/iymon: %d    Bajarilgan epizodlar: saqlangan (Saved/ert_save.json)\"), GM->GetHonor()), 44 * Sc, 434 * Sc, Grey, Sc);\n\tText(TEXT(\"Preset: PC Ultra = Lumen (HW RT) + Virtual Shadows High + 100%;  PS5/XSX = Lumen (SW) + VSM Medium + TSR 50-70%;  Series S = SSGI + CSM + SSR + TSR 50-60%\"), 44 * Sc, 458 * Sc, Grey, 0.85f * Sc);")
# Minimap: GPS yo'li va ittifoqchilar
hd = rep(hd, "\t// NPClar\n\t{\n\t\tTArray<AActor*> Npcs; UGameplayStatics::GetAllActorsOfClass(GetWorld(), AErtNpc::StaticClass(), Npcs);",
"""	// GPS yo'li
	if (AErtGps* G = Cast<AErtGps>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtGps::StaticClass())))
	{
		const TArray<FVector>& Path = G->GetPath();
		for (int32 i = 0; i + 1 < Path.Num(); ++i)
		{
			const FVector2D A(PX(Path[i].Y / 100.f), PY(Path[i].X / 100.f)), B(PX(Path[i + 1].Y / 100.f), PY(Path[i + 1].X / 100.f));
			if (!Inside(A.X, A.Y) && !Inside(B.X, B.Y)) continue;
			FCanvasLineItem L(A, B); L.SetColor(FLinearColor(1.f, 0.8f, 0.2f)); L.LineThickness = 2.f; Canvas->DrawItem(L);
		}
	}
	// Ittifoqchilar (urush)
	if (D) for (const AErtEnemy* E : D->GetAllies())
	{
		if (!E || E->IsDead()) continue;
		const float X = PX(E->GetActorLocation().Y / 100.f), Y = PY(E->GetActorLocation().X / 100.f);
		if (Inside(X, Y)) { FCanvasTileItem T(FVector2D(X - 3 * Sc, Y - 3 * Sc), FVector2D(6 * Sc, 6 * Sc), FLinearColor(0.2f, 0.85f, 0.6f)); Canvas->DrawItem(T); }
	}
	// NPClar
	{
		TArray<AActor*> Npcs; UGameplayStatics::GetAllActorsOfClass(GetWorld(), AErtNpc::StaticClass(), Npcs);""")
# GPS masofa matni (maqsad markeri yonida)
hd = rep(hd, "\t\t\tconst FString DS = FString::Printf(TEXT(\"%.0f m\"), Dist);\n\t\t\tText(DS, S.X - TextWidth(DS, Sc, false) * 0.5f, S.Y + Sz + 2, Gold, Sc);",
        "\t\t\tFString DS = FString::Printf(TEXT(\"%.0f m\"), Dist);\n\t\t\tif (AErtGps* G = Cast<AErtGps>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtGps::StaticClass()))) if (G->HasPath() && &P == &Pts[0]) DS = FString::Printf(TEXT(\"GPS %.0f m\"), G->GetPathLengthM());\n\t\t\tText(DS, S.X - TextWidth(DS, Sc, false) * 0.5f, S.Y + Sz + 2, Gold, Sc);")
# 3D xarita ekrani
hd = rep(hd, "void AErtHUD::DrawMap(float SW, float SH, float Sc)\n{\n\tFCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.02f, 0.02f, 0.03f, 0.85f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);\n\tconst float S = FMath::Min(SW, SH) - 80 * Sc;\n\tDrawMapArea((SW - S) * 0.5f, 40 * Sc, S, 0.f, 0.f, 1000.f, true, Sc);",
"""void AErtHUD::DrawMap(float SW, float SH, float Sc)
{
	FCanvasTileItem Bg(FVector2D(0, 0), FVector2D(SW, SH), FLinearColor(0.02f, 0.02f, 0.03f, 0.85f)); Bg.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bg);
	const float S = FMath::Min(SW, SH) - 80 * Sc;
	AErtMap3D* M3 = Cast<AErtMap3D>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtMap3D::StaticClass()));
	if (M3 && M3->GetTexture())
	{
		// 3D relyef xaritasi (render-tekstura) + belgilar proyeksiyasi
		const float X0 = (SW - S) * 0.5f, Y0 = 40 * Sc;
		FCanvasTileItem T(FVector2D(X0, Y0), M3->GetTexture()->GetResource(), FVector2D(S, S), FLinearColor::White); T.BlendMode = SE_BLEND_Opaque; Canvas->DrawItem(T);
		using namespace ErtMap;
		AErtWorldBuilder* W = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtWorldBuilder::StaticClass()));
		const FLinearColor Ink(0.98f, 0.95f, 0.85f);
		auto Lbl = [&](float E, float N, const TCHAR* Name, const FLinearColor& C)
		{
			float U, V; if (!M3->Project(AErtWorldBuilder::PlanToWorld(E, N, W ? W->HeightAt(E, N) : 0.f), U, V)) return;
			Circle(X0 + U * S, Y0 + V * S, 3.f * Sc, C, 10);
			Text(Name, X0 + U * S + 5 * Sc, Y0 + V * S - 8 * Sc, C, 0.85f * Sc, true, false);
		};
		Lbl(ObaE, ObaN, TEXT("Qayi obasi"), Ink); Lbl(FortE, FortN, TEXT("Bagras"), Ink); Lbl(CityE, CityN, TEXT("Shahar"), Ink); Lbl(CampE, CampN, TEXT("Mo'g'ul lageri"), FLinearColor(1.f, 0.4f, 0.3f));
		Lbl(DamE, DamN, TEXT("Damashq"), Ink); Lbl(HalabE, HalabN, TEXT("Halab"), Ink); Lbl(KonE, KonN, TEXT("Konya"), Ink); Lbl(KayE, KayN, TEXT("Qayseri"), Ink); Lbl(SivE, SivN, TEXT("Sivas"), Ink); Lbl(ErzE, ErzN, TEXT("Erzurum"), Ink);
		Lbl(BurE, BurN, TEXT("Bursa"), Ink); Lbl(NikE, NikN, TEXT("Nikeya"), Ink); Lbl(KarE, KarN, TEXT("Karacahisar"), Ink); Lbl(SogE, SogN, TEXT("So'g'ut"), Ink); Lbl(DomE, DomN, TEXT("Domaniç"), FLinearColor(0.6f, 0.95f, 0.5f)); Lbl(AskE, AskN, TEXT("Askaniya"), FLinearColor(0.5f, 0.75f, 1.f)); Lbl(LakeE, LakeN, TEXT("Ko'l"), FLinearColor(0.5f, 0.75f, 1.f));
		if (AErtMissionDirector* Dm = Director()) { TArray<FVector> Pts; Dm->GetMarkers(Pts); for (const FVector& P : Pts) { float U, V; if (M3->Project(P, U, V)) { const float Sz = 6 * Sc; FCanvasTileItem Tm(FVector2D(X0 + U * S - Sz, Y0 + V * S - Sz), FVector2D(Sz * 2, Sz * 2), FLinearColor(1.f, 0.8f, 0.2f)); Tm.Rotation = FRotator(0, 45.f, 0); Tm.PivotPoint = FVector2D(0.5f, 0.5f); Canvas->DrawItem(Tm); } } }
		if (APawn* P = GetOwningPawn()) { float U, V; if (M3->Project(P->GetActorLocation(), U, V)) Text(TEXT("SIZ"), X0 + U * S + 6 * Sc, Y0 + V * S - 10 * Sc, FLinearColor(0.3f, 1.f, 0.4f), 0.9f * Sc, true, true); }
		if (AErtGps* G = Cast<AErtGps>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtGps::StaticClass()))) if (G->HasPath()) Text(FString::Printf(TEXT("GPS yo'li: %.0f m"), G->GetPathLengthM()), X0 + 10 * Sc, Y0 + S - 24 * Sc, FLinearColor(1.f, 0.85f, 0.3f), Sc);
		// Kichik 2D xarita (o'ng-past burchak)
		DrawMapArea(SW - 230 * Sc, SH - 250 * Sc, 200 * Sc, 0.f, 0.f, 1000.f, false, Sc);
	}
	else DrawMapArea((SW - S) * 0.5f, 40 * Sc, S, 0.f, 0.f, 1000.f, true, Sc);""")
hd = rep(hd, "\tText(TEXT(\"XARITA   (M yoki Esc: yopish)   yashil - siz, oltin - maqsad, qizil - dushman, ko'k - odamlar\"), 24 * Sc, SH - 28 * Sc, FLinearColor(0.6f, 0.6f, 0.55f), 0.9f * Sc);",
        "\tText(TEXT(\"3D XARITA   (M yoki Esc: yopish)   Chap/O'ng: aylantirish   Yuqori/Past: masshtab   yashil - siz, oltin - maqsad/GPS yo'li, qizil - dushman\"), 24 * Sc, SH - 28 * Sc, FLinearColor(0.6f, 0.6f, 0.55f), 0.9f * Sc);")
save('ErtHUD.cpp', hd)
print('patched')
