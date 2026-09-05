# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# ---- Yerdagi qon dog'lari: bitta umumiy aktor, FIFO 400 ta dog' ----
h = load('ErtFx.h')
h = rep(h, "protected:\n\tvirtual void Tick(float Dt) override;", '''protected:
\tvirtual void Tick(float Dt) override;
\tbool bSplatDone = false;''')
h += '''
/** Yerdagi doimiy qon dog'lari (bitta aktor, 400 tagacha, eskilari o'chadi) */
UCLASS()
class ERTUGRUL_API AErtSplats : public AActor
{
	GENERATED_BODY()
public:
	AErtSplats();
	static AErtSplats* Get(UWorld* W);
	void AddSplat(const FVector& Pos, float Size, const FLinearColor& Col);
private:
	struct FS { FVector P; float Size; FLinearColor C; float Yaw; };
	TArray<FS> Splats;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Mesh;
	void Rebuild();
};
'''
save('ErtFx.h', h)

c = load('ErtFx.cpp')
c = rep(c, "\t\t\tif (P.P.Z < -GetActorLocation().Z + 2.f) { P.P.Z = -GetActorLocation().Z + 2.f; P.bStuck = true; P.Size *= 1.6f; }",
        '''\t\t\tif (P.P.Z < -GetActorLocation().Z + 2.f)
\t\t\t{
\t\t\t\tP.P.Z = -GetActorLocation().Z + 2.f; P.bStuck = true; P.Size *= 1.6f;
\t\t\t\t// Yerga tekkan qon: doimiy dog' (yer balandligi: trace)
\t\t\t\tif (!bSplatDone && P.C.R > 0.3f && P.C.G < 0.1f)
\t\t\t\t{
\t\t\t\t\tbSplatDone = true;
\t\t\t\t\tFHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtSplat), true, this);
\t\t\t\t\tconst FVector Wp = GetActorLocation() + FVector(P.P.X, P.P.Y, 0);
\t\t\t\t\tif (GetWorld()->LineTraceSingleByChannel(Hit, Wp + FVector(0, 0, 150.f), Wp - FVector(0, 0, 300.f), ECC_Visibility, Q) && Hit.ImpactNormal.Z > 0.7f)
\t\t\t\t\t\tif (AErtSplats* S = AErtSplats::Get(GetWorld())) S->AddSplat(Hit.ImpactPoint, FMath::FRandRange(28.f, 55.f) * FMath::Sqrt((float)Ps.Num() / 14.f), FLinearColor(0.30f, 0.02f, 0.01f));
\t\t\t\t}
\t\t\t}''')
c += r'''

// ---------------- Yerdagi doimiy qon dog'lari ----------------

AErtSplats::AErtSplats()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->bUseAsyncCooking = false;
}

AErtSplats* AErtSplats::Get(UWorld* W)
{
	if (!W) return nullptr;
	if (AErtSplats* S = Cast<AErtSplats>(UGameplayStatics::GetActorOfClass(W, AErtSplats::StaticClass()))) return S;
	AErtSplats* S = W->SpawnActor<AErtSplats>(AErtSplats::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (S && S->Mesh) S->Mesh->SetMaterial(0, ErtFxMat(false));
	return S;
}

void AErtSplats::AddSplat(const FVector& Pos, float Size, const FLinearColor& Col)
{
	FS S; S.P = Pos + FVector(0, 0, 1.5f); S.Size = Size; S.C = Col; S.Yaw = FMath::FRandRange(0.f, 360.f);
	Splats.Add(S);
	if (Splats.Num() > 400) Splats.RemoveAt(0, Splats.Num() - 400);
	Rebuild();
}

void AErtSplats::Rebuild()
{
	FErtMeshData M;
	int32 Seed = 0;
	for (const FS& S : Splats)
	{
		// Notekis dog': 9 nurli yulduzsimon ko'pburchak, har nur uzunligi tasodifiy; qirralarda mayda tomchilar
		FRandomStream RS(++Seed * 7919);
		const int32 N = 9;
		TArray<FVector> Ring; Ring.Reserve(N);
		for (int32 i = 0; i < N; ++i)
		{
			const float A = FMath::DegreesToRadians(S.Yaw + i * 360.f / N), R = S.Size * RS.FRandRange(0.45f, 1.f);
			Ring.Add(S.P + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0));
		}
		for (int32 i = 0; i < N; ++i) M.AddTri(S.P, Ring[i], Ring[(i + 1) % N], FVector::UpVector, ErtCol::Sty(S.C * RS.FRandRange(0.8f, 1.1f), ErtCol::StylePlain));
		for (int32 i = 0; i < 4; ++i)
		{
			const float A = FMath::DegreesToRadians(RS.FRandRange(0.f, 360.f)), R = S.Size * RS.FRandRange(1.05f, 1.6f), r = S.Size * RS.FRandRange(0.08f, 0.18f);
			const FVector Cn = S.P + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0);
			M.AddQuad(Cn + FVector(-r, -r, 0), Cn + FVector(r, -r, 0), Cn + FVector(r, r, 0), Cn + FVector(-r, r, 0), FVector::UpVector, ErtCol::Sty(S.C * 0.9f, ErtCol::StylePlain));
		}
	}
	if (M.Verts.Num()) Mesh->CreateMeshSection_LinearColor(0, M.Verts, M.Tris, M.Normals, M.UVs, M.Colors, M.Tangents, false, false);
}
'''
save('ErtFx.cpp', c)

# ---- Tanadagi jarohat izlari ----
h = load('ErtHeroBody.h')
h = rep(h, "\tvoid TriggerHurt(float SideSign = 0.f);", "\tvoid TriggerHurt(float SideSign = 0.f);\n\t/** Jarohat izi: ko'krak/orqa/yon yuzasida qora-qizil dog' (12 tagacha) */\n\tvoid AddWound(float SideSign, float Strength);")
h = rep(h, "\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Shield;", "\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Shield;\n\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Wounds;\n\tFErtMeshDataHolder* WoundData = nullptr;\n\tint32 WoundCount = 0;")
save('ErtHeroBody.h', h)
# FErtMeshDataHolder kerak emas - oddiy: har safar Wounds meshini qayta quramiz, dog'larni saqlab
h = load('ErtHeroBody.h')
h = h.replace("\tFErtMeshDataHolder* WoundData = nullptr;\n", "\tTArray<FVector4> WoundList;   // x,y,z markaz (Torso koordinatasi), w o'lcham\n")
save('ErtHeroBody.h', h)
c = load('ErtHeroBody.cpp')
c = rep(c, "void UErtHeroBody::TriggerAttack(int32 Kind) { AttackT = 1.f; AttackKind = Kind; }", '''void UErtHeroBody::TriggerAttack(int32 Kind) { AttackT = 1.f; AttackKind = Kind; }

void UErtHeroBody::AddWound(float SideSign, float Strength)
{
	if (!IsBuilt() || bDead) return;
	if (!Wounds) Wounds = MakePart(TEXT("Wounds"), Torso, FVector::ZeroVector);
	const int32 Count = FMath::Clamp(FMath::RoundToInt(Strength * 1.5f), 1, 3);
	for (int32 i = 0; i < Count; ++i)
	{
		FVector4 Wd;
		const float Z = FMath::FRandRange(8.f, 42.f), Sz = FMath::FRandRange(2.5f, 5.f) * FMath::Clamp(Strength, 0.6f, 1.6f);
		if (SideSign == 0.f) Wd = FVector4(FMath::FRand() < 0.7f ? 13.7f : -13.7f, FMath::FRandRange(-12.f, 12.f), Z, Sz);
		else Wd = FVector4(FMath::FRandRange(-8.f, 8.f), SideSign * 19.6f, Z, Sz);
		WoundList.Add(Wd);
	}
	if (WoundList.Num() > 12) WoundList.RemoveAt(0, WoundList.Num() - 12);
	FErtMeshData M;
	const FLinearColor Dark = ErtCol::Sty(FLinearColor(0.28f, 0.02f, 0.01f), ErtCol::StyleLeather);
	for (const FVector4& Wd : WoundList)
	{
		const bool bSide = FMath::Abs(Wd.Y) > 19.f;
		// Yuzaga yopishgan yassi dog' (0.4 sm qalin) + pastga oqqan iz
		if (bSide) { M.AddBox(FVector(Wd.X, Wd.Y, Wd.Z), FVector(Wd.W, 0.4f, Wd.W * 0.8f), Dark); M.AddBox(FVector(Wd.X, Wd.Y, Wd.Z - Wd.W * 1.4f), FVector(Wd.W * 0.25f, 0.4f, Wd.W * 0.9f), Dark); }
		else { M.AddBox(FVector(Wd.X, Wd.Y, Wd.Z), FVector(0.4f, Wd.W, Wd.W * 0.8f), Dark); M.AddBox(FVector(Wd.X, Wd.Y, Wd.Z - Wd.W * 1.4f), FVector(0.4f, Wd.W * 0.25f, Wd.W * 0.9f), Dark); }
	}
	M.Commit(Wounds, 0, false);
}''')
save('ErtHeroBody.cpp', c)

# Ilgaklar
c = load('ErtEnemy.cpp')
c = rep(c, "\t\tBody->TriggerHurt(Side);\n\t}", "\t\tBody->TriggerHurt(Side);\n\t\tBody->AddWound(Side, Damage / 15.f);\n\t}")
save('ErtEnemy.cpp', c)
c = load('ErtCharacter.cpp')
c = rep(c, "\tif (Body) Body->TriggerHurt(FMath::Sign(FVector::DotProduct(GetActorRightVector(), To)) * (FMath::Abs(FVector::DotProduct(GetActorRightVector(), To)) > 0.4f ? 1.f : 0.f));",
        "\tif (Body)\n\t{\n\t\tconst float SideW = FMath::Sign(FVector::DotProduct(GetActorRightVector(), To)) * (FMath::Abs(FVector::DotProduct(GetActorRightVector(), To)) > 0.4f ? 1.f : 0.f);\n\t\tBody->TriggerHurt(SideW);\n\t\tif (Damage > 2.f) Body->AddWound(SideW, Damage / 15.f);\n\t}")
save('ErtCharacter.cpp', c)

# Ko'lmaklar: yana kamroq va ochroq
p = 'D:/temp/claude/ert_make_pbr.py'
m = io.open(p, encoding='utf-8').read()
m = rep(m, "\tfloat puddle = (N.z > 0.995 && Pm.z < 22.0) ? smoothstep(0.80, 0.86, VN(uv * 0.35 + 7.3)) * smoothstep(0.55, 0.7, VN(uv * 0.06 + 3.1)) : 0.0;",
        "\tfloat puddle = (N.z > 0.996 && Pm.z < 20.0) ? smoothstep(0.86, 0.90, VN(uv * 0.35 + 7.3)) * smoothstep(0.62, 0.74, VN(uv * 0.06 + 3.1)) : 0.0;")
m = rep(m, "\tcol = lerp(col, float3(0.10, 0.13, 0.16), puddle * 0.85);", "\tcol = lerp(col, float3(0.20, 0.24, 0.27), puddle * 0.75);")
io.open(p, 'w', encoding='utf-8', newline='\n').write(m)
print('patched')
