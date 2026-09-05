# -*- coding: utf-8 -*-
import io, re
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# ---------- Tana: o'lim variantlari, yo'nalishli zarba reaksiyasi ----------
h = load('ErtHeroBody.h')
h = rep(h, "\tvoid TriggerHurt();", "\tvoid TriggerHurt(float SideSign = 0.f);   // SideSign: -1 chapdan, +1 o'ngdan, 0 old/orqa")
h = rep(h, "\tvoid SetDead(float CapsuleHalfHeight);", "\tvoid SetDead(float CapsuleHalfHeight, int32 Variant = -1);   // -1 tasodifiy: 0 orqaga yiqilish, 1 yuztuban, 2 tiz cho'kib yonboshga")
h = rep(h, "\tfloat HurtT = 0.f;", "\tfloat HurtT = 0.f, HurtDir = 0.f;")
save('ErtHeroBody.h', h)
c = load('ErtHeroBody.cpp')
c = rep(c, "void UErtHeroBody::TriggerHurt() { HurtT = 1.f; }", "void UErtHeroBody::TriggerHurt(float SideSign) { HurtT = 1.f; HurtDir = SideSign; }")
c = rep(c, "\tif (HurtT > 0.f) { T.TorsoPitch += 18.f * HurtT; T.HeadPitch -= 10.f * HurtT; }",
        "\tif (HurtT > 0.f) { T.TorsoPitch += (HurtDir == 0.f ? 18.f : 8.f) * HurtT; T.HeadPitch -= 10.f * HurtT; T.TorsoYaw += 22.f * HurtDir * HurtT; T.TorsoRoll += 12.f * HurtDir * HurtT; T.ArmL += 25.f * HurtDir * HurtT; T.ArmR -= 25.f * HurtDir * HurtT; }")
c = rep(c, '''void UErtHeroBody::SetDead(float HalfH)
{
	if (!IsBuilt() || bDead) return;
	bDead = true;
	Root->SetRelativeRotation(FRotator(-82.f, 0, 12.f));
	Root->SetRelativeLocation(FVector(30.f, 0, -HalfH + 14.f - 89.f));
	FPose P; P.ArmL = -40.f; P.ArmR = 30.f; P.ArmSpread = 25.f; P.KneeL = 20.f; P.ThighR = 15.f;
	Cur = P;
	Apply(Cur);
}''', '''void UErtHeroBody::SetDead(float HalfH, int32 Variant)
{
	if (!IsBuilt() || bDead) return;
	bDead = true;
	if (Variant < 0) Variant = FMath::RandRange(0, 2);
	FPose P;
	if (Variant == 1)
	{
		// Yuztuban: oldinga yiqiladi, qo'llar bosh ustida
		Root->SetRelativeRotation(FRotator(84.f, 0, -8.f));
		Root->SetRelativeLocation(FVector(-30.f, 0, -HalfH + 16.f - 89.f));
		P.ArmL = -150.f; P.ArmR = -120.f; P.ArmSpread = 30.f; P.ElbowL = 30.f; P.KneeR = 25.f; P.ThighL = 10.f; P.HeadPitch = 20.f;
	}
	else if (Variant == 2)
	{
		// Tiz cho'kib yonboshga: tana bukilgan, tizzalar bukilgan
		Root->SetRelativeRotation(FRotator(-30.f, 25.f, 88.f));
		Root->SetRelativeLocation(FVector(10.f, 20.f, -HalfH + 24.f - 89.f));
		P.ThighL = -80.f; P.ThighR = -60.f; P.KneeL = 110.f; P.KneeR = 95.f; P.TorsoPitch = -35.f; P.ArmL = -20.f; P.ArmR = 40.f; P.ElbowR = 60.f; P.HeadPitch = 30.f;
	}
	else
	{
		Root->SetRelativeRotation(FRotator(-82.f, 0, 12.f));
		Root->SetRelativeLocation(FVector(30.f, 0, -HalfH + 14.f - 89.f));
		P.ArmL = -40.f; P.ArmR = 30.f; P.ArmSpread = 25.f; P.KneeL = 20.f; P.ThighR = 15.f;
	}
	Cur = P;
	Apply(Cur);
}''')
save('ErtHeroBody.cpp', c)

# ---------- Personaj: qilich izi, qon, uchqun, yo'nalishli reaksiya ----------
c = load('ErtCharacter.cpp')
if '#include "ErtFx.h"' not in c:
    c = c.replace('#include "ErtEnemy.h"\n', '#include "ErtEnemy.h"\n#include "ErtFx.h"\n', 1)
c = rep(c, "\tFErtAudio::PlaySfx(GetWorld(), TEXT(\"swing\"), GetActorLocation(), Kind == 2 ? 1.f : 0.8f, Kind == 2 ? 0.8f : FMath::FRandRange(0.9f, 1.1f));\n\tconst FVector C = GetActorLocation() + GetActorForwardVector() * 130.f + FVector(0, 0, Horse ? 0.f : 45.f);",
        "\tFErtAudio::PlaySfx(GetWorld(), TEXT(\"swing\"), GetActorLocation(), Kind == 2 ? 1.f : 0.8f, Kind == 2 ? 0.8f : FMath::FRandRange(0.9f, 1.1f));\n\tAErtBurst::SwordArc(GetWorld(), GetActorLocation() + GetActorForwardVector() * 40.f + FVector(0, 0, Horse ? 60.f : 30.f), GetActorRotation().Yaw, Kind);\n\tconst FVector C = GetActorLocation() + GetActorForwardVector() * 130.f + FVector(0, 0, Horse ? 0.f : 45.f);")
c = rep(c, "\t\t\tif (!bHit) { ShakeT = FMath::Max(ShakeT, 0.08f); continue; }   // to'sildi",
        "\t\t\tif (!bHit) { ShakeT = FMath::Max(ShakeT, 0.08f); AErtBurst::Sparks(GetWorld(), E->GetActorLocation() + FVector(0, 0, 50.f) + (GetActorLocation() - E->GetActorLocation()).GetSafeNormal2D() * 35.f, (GetActorLocation() - E->GetActorLocation()).GetSafeNormal2D()); continue; }   // to'sildi\n\t\t\tAErtBurst::Blood(GetWorld(), E->GetActorLocation() + FVector(0, 0, 60.f), (E->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() + FVector(0, 0, 0.3f), bExecute ? 2.2f : (Kind == 2 ? 1.5f : 1.f));")
c = rep(c, "\tif (bBlocking && bFacing && Stamina > 5.f && !bUnblockable) { Damage *= bShield ? 0.05f : 0.2f;",
        "\tif (bBlocking && bFacing && Stamina > 5.f && !bUnblockable) { AErtBurst::Sparks(GetWorld(), GetActorLocation() + To * 50.f + FVector(0, 0, 40.f), -To); Damage *= bShield ? 0.05f : 0.2f;")
c = rep(c, "\tHealth -= Damage;\n\tHurtFlash = 1.f;", "\tHealth -= Damage;\n\tHurtFlash = 1.f;\n\tAErtBurst::Blood(GetWorld(), GetActorLocation() + FVector(0, 0, 40.f), -To + FVector(0, 0, 0.4f), FMath::Clamp(Damage / 15.f, 0.6f, 1.6f));")
c = rep(c, "\tNoDamageT = 0.f;\n\tif (Body) Body->TriggerHurt();", "\tNoDamageT = 0.f;\n\tif (Body) Body->TriggerHurt(FMath::Sign(FVector::DotProduct(GetActorRightVector(), To)) * (FMath::Abs(FVector::DotProduct(GetActorRightVector(), To)) > 0.4f ? 1.f : 0.f));")
save('ErtCharacter.cpp', c)

# ---------- Dushman: yo'nalishli reaksiya, o'limda chang ----------
c = load('ErtEnemy.cpp')
if '#include "ErtFx.h"' not in c:
    c = c.replace('#include "ErtEnemy.h"\n', '#include "ErtEnemy.h"\n#include "ErtFx.h"\n', 1)
c = rep(c, "\tHealth -= Damage;\n\tbAlerted = true;\n\tif (Body && Body->IsBuilt()) Body->TriggerHurt();",
        "\tHealth -= Damage;\n\tbAlerted = true;\n\tif (Body && Body->IsBuilt())\n\t{\n\t\tfloat Side = 0.f;\n\t\tif (Source) { const float D = FVector::DotProduct(GetActorRightVector(), (Source->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()); Side = FMath::Abs(D) > 0.4f ? FMath::Sign(D) : 0.f; }\n\t\tBody->TriggerHurt(Side);\n\t}")
c = rep(c, "\tif (Body && Body->IsBuilt()) Body->SetDead(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());\n\tfor (UProceduralMeshComponent* P : DeerParts)",
        "\tif (Body && Body->IsBuilt()) Body->SetDead(GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());\n\tAErtBurst::Dust(GetWorld(), GetActorLocation() - FVector(0, 0, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - 10.f), Kind == EErtEnemyKind::Boss ? 2.f : 1.f);\n\tfor (UProceduralMeshComponent* P : DeerParts)")
save('ErtEnemy.cpp', c)

# ---------- O't-o'lan folyaji ----------
h = load('ErtWorldBuilder.h')
h = rep(h, "\tvoid BuildRocks();", "\tvoid BuildRocks();\n\tvoid BuildGrass();")
h = rep(h, "\tint32 FabTreesPlaced = 0, FabRocksPlaced = 0;", "\tint32 FabTreesPlaced = 0, FabRocksPlaced = 0;\n\tUPROPERTY(Transient) TObjectPtr<UMaterialInterface> GrassMat;\n\tUPROPERTY(EditAnywhere, Category = \"Ertugrul|Dunyo\") int32 GrassClumps = 26000;")
save('ErtWorldBuilder.h', h)
c = load('ErtWorldBuilder.cpp')
c = rep(c, "\tif (bBuildForest) { BuildForest(); BuildRocks(); }", "\tif (bBuildForest) { BuildForest(); BuildRocks(); BuildGrass(); }")
c = rep(c, "\tWaterMat = LoadObject<UMaterialInterface>(nullptr, TEXT(\"/Game/Ertugrul/Materials/M_ErtWater.M_ErtWater\"));",
        "\tWaterMat = LoadObject<UMaterialInterface>(nullptr, TEXT(\"/Game/Ertugrul/Materials/M_ErtWater.M_ErtWater\"));\n\tGrassMat = LoadObject<UMaterialInterface>(nullptr, TEXT(\"/Game/Ertugrul/Materials/M_ErtGrass.M_ErtGrass\"));")
c += r'''
// ---------------- O't-o'lan: kesishgan tolalar, shamol materiali (alfa = tebranish og'irligi) ----------------

static void ErtAddBlade(FErtMeshData& M, const FVector& Base, float Yaw, float Hgt, float Wd, const FLinearColor& Col, float Bend)
{
	const FQuat Q = FRotator(0, Yaw, 0).Quaternion();
	const FVector R = Q.RotateVector(FVector(0, Wd * 0.5f, 0)), Fw = Q.RotateVector(FVector(Bend, 0, 0));
	const int32 B = M.Verts.Num();
	FLinearColor Bot = Col; Bot.A = 0.f; FLinearColor Top = Col; Top.A = 1.f;
	M.Verts.Add(Base - R); M.Verts.Add(Base + R); M.Verts.Add(Base + R * 0.35f + Fw + FVector(0, 0, Hgt)); M.Verts.Add(Base - R * 0.35f + Fw + FVector(0, 0, Hgt));
	M.Colors.Add(Bot); M.Colors.Add(Bot); M.Colors.Add(Top); M.Colors.Add(Top);
	const FVector Nm = Q.RotateVector(FVector(1, 0, 0));
	for (int32 i = 0; i < 4; ++i) { M.Normals.Add(Nm); M.UVs.Add(FVector2D(i & 1, i >> 1)); M.Tangents.Add(FProcMeshTangent(0, 1, 0)); }
	M.Tris.Append({ B, B + 2, B + 1, B, B + 3, B + 2 });
}

void AErtWorldBuilder::BuildGrass()
{
	if (!GrassMat) return;
	FRandomStream RS(Seed + 17);
	const float Half = WorldSizeM * 0.5f;
	const int32 CellsPerSide = 6;
	TArray<FErtMeshData> Cells; Cells.Init(FErtMeshData(1.f), CellsPerSide * CellsPerSide);
	int32 Placed = 0;
	const FLinearColor G1(0.30f, 0.46f, 0.12f), G2(0.42f, 0.55f, 0.16f), G3(0.55f, 0.58f, 0.22f);
	for (int32 i = 0; i < GrassClumps * 4 && Placed < GrassClumps; ++i)
	{
		const float E = RS.FRandRange(-Half + 5.f, Half - 5.f), N = RS.FRandRange(-Half + 5.f, Half - 5.f);
		if (N < DesertN + 30.f) continue;
		const float H = HeightAt(E, N);
		const FVector Nm = TerrainNormal(E, N);
		if (H < WaterZ + 1.2f || H > 70.f || Nm.Z < 0.86f) continue;
		float Wd = 0.f; if (RoadDist(E, N, &Wd) < Wd * 0.5f + 1.f) continue;
		// Zichlik: yashil o'tloqlar (shimol) zich, quruq janub siyrak; oba, So'g'ut, yaylov atrofi zichroq
		float Dens = 0.35f + 0.5f * Smooth01((N + 100.f) / 500.f);
		Dens *= 0.6f + 0.4f * Noise(E, N, 0.02f);
		for (const FVector2D& Hot : { FVector2D(ObaE, ObaN), FVector2D(SogE, SogN), FVector2D(DomE, DomN), FVector2D(BurE, BurN) })
			Dens = FMath::Max(Dens, 0.95f * (1.f - Smooth01((FVector2D::Distance(FVector2D(E, N), Hot) - 120.f) / 200.f)));
		if (RS.FRand() > Dens) continue;
		if (!IsBuildable(E, N)) continue;
		const int32 cx = FMath::Clamp((int32)((E + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		const int32 cy = FMath::Clamp((int32)((N + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);
		FErtMeshData& M = Cells[cy * CellsPerSide + cx];
		const FVector Base = W(E, N, H - 0.02f);
		const float Hgt = RS.FRandRange(28.f, 55.f) * (1.f + 0.4f * Smooth01((N + 100.f) / 500.f));
		const FLinearColor Col = ErtCol::Vary(RS.FRand() < 0.6f ? G1 : (RS.FRand() < 0.5f ? G2 : G3), 0.12f, i);
		const float Yaw0 = RS.FRandRange(0.f, 180.f);
		for (int32 b = 0; b < 3; ++b) ErtAddBlade(M, Base + FVector(RS.FRandRange(-10.f, 10.f), RS.FRandRange(-10.f, 10.f), 0), Yaw0 + b * 60.f, Hgt * RS.FRandRange(0.8f, 1.1f), RS.FRandRange(14.f, 26.f), Col, RS.FRandRange(-8.f, 8.f));
		++Placed;
	}
	for (int32 cidx = 0; cidx < Cells.Num(); ++cidx)
		if (Cells[cidx].Verts.Num())
		{
			UProceduralMeshComponent* P = NewPart(FString::Printf(TEXT("Grass_%d"), cidx), false, GrassMat);
			Cells[cidx].Commit(P, 0, false);
			P->SetCastShadow(false);
		}
	UE_LOG(LogErtugrul, Log, TEXT("O't-o'lan: %d tup"), Placed);
}
'''
save('ErtWorldBuilder.cpp', c)

# ---------- Yer auto-materiali: qiyalikda qoya, balandda qor, tekis pastlikda ko'lmak, o't tolalari ----------
p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
m = rep(m, "#define HGROUND(p) (VN((p) * 3.0) * 0.5 + VN((p) * 11.0) * 0.3 + VN((p) * 40.0) * 0.2)",
        "#define HGROUND(p) (VN((p) * 3.0) * 0.45 + VN((p) * 11.0) * 0.25 + VN((p) * 40.0) * 0.15 + VN(float2((p).x * 90.0, (p).y * 9.0)) * 0.15)")
m = rep(m, "if (st == 0) { col *= 0.72 + 0.55 * h; }",
        '''if (st == 0)
{
	// Auto-qatlamlar: qiyalikda qoya, 75 m dan yuqorida qor, tekis pastlikda ko'lmak
	float slope = 1.0 - saturate((N.z - 0.62) / 0.28);
	float rockH = HROCK(uv * 0.5);
	float3 rockC = float3(0.42, 0.40, 0.37) * (0.62 + 0.55 * saturate(rockH + 0.25));
	float3 grassC = col * (0.72 + 0.55 * h);
	float snow = saturate((Pm.z - 75.0) / 18.0) * saturate((N.z - 0.55) / 0.3);
	float puddle = (N.z > 0.992 && Pm.z < 24.0) ? smoothstep(0.66, 0.74, VN(uv * 0.35 + 7.3)) : 0.0;
	col = lerp(grassC, rockC, slope);
	col = lerp(col, float3(0.92, 0.94, 0.98) * (0.85 + 0.15 * h), snow);
	col = lerp(col, float3(0.10, 0.13, 0.16), puddle * 0.85);
	h = lerp(h, rockH, slope);
	if (puddle > 0.5) h = 1.5;   // roughness tuguni uchun ko'lmak belgisi
}''')
m = rep(m, "float bump = (st == 0) ? 0.06 :", "float bump = (st == 0) ? (1.0 - saturate((N.z - 0.62) / 0.28)) * 0.1 + 0.06 :")
m = rep(m, "if (s >= 0.66 && s < 0.76) return 0.92;                 // jun", "if (s >= 0.66 && s < 0.76) return 0.92;                 // jun\nif (s < 0.05 && H > 1.2) return 0.12;                   // ko'lmak")
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('patched')
