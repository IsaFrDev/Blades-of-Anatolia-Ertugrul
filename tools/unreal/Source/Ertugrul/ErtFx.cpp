#include "ErtFx.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"

AErtBurst::AErtBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->bUseAsyncCooking = false;
}

static UMaterialInterface* ErtFxMat(bool bTranslucent)
{
	static UMaterialInterface* Dust = nullptr; static UMaterialInterface* Lit = nullptr;
	if (bTranslucent) { if (!Dust) Dust = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtDust.M_ErtDust")); return Dust; }
	if (!Lit) Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	return Lit;
}

void AErtBurst::InitParticles(const FVector& Pos, const FVector& Dir, int32 Kind, int32 Count)
{
	Mode = 0;
	SetActorLocation(Pos);
	const FVector D = Dir.GetSafeNormal();
	const FLinearColor Base = Kind == 0 ? FLinearColor(0.45f, 0.02f, 0.01f) : Kind == 1 ? FLinearColor(1.f, 0.85f, 0.35f) : FLinearColor(0.62f, 0.55f, 0.42f);
	Duration = Kind == 0 ? 1.6f : Kind == 1 ? 0.45f : 0.9f;
	for (int32 i = 0; i < Count; ++i)
	{
		FP P;
		const FVector R(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-0.3f, 1.f));
		const float Sp = Kind == 0 ? FMath::FRandRange(180.f, 520.f) : Kind == 1 ? FMath::FRandRange(300.f, 900.f) : FMath::FRandRange(40.f, 160.f);
		P.P = FVector::ZeroVector; P.V = (D * 1.2f + R.GetSafeNormal() * 0.9f).GetSafeNormal() * Sp + FVector(0, 0, Kind == 2 ? 60.f : 120.f);
		P.Life = Duration * FMath::FRandRange(0.6f, 1.f);
		P.Size = Kind == 0 ? FMath::FRandRange(1.8f, 4.5f) : Kind == 1 ? FMath::FRandRange(1.5f, 3.5f) : FMath::FRandRange(8.f, 20.f);
		P.C = Base * FMath::FRandRange(0.8f, 1.2f); P.C.A = 1.f;
		P.bStuck = false;
		Ps.Add(P);
	}
	Mesh->SetMaterial(0, ErtFxMat(Kind != 0));
	Mesh->SetTranslucentSortPriority(Kind == 0 ? 0 : 1);
	Rebuild();
}

void AErtBurst::InitArc(const FVector& Center, float Yaw, float StartAng, float EndAng, float Radius, float Tilt, bool bHeavy)
{
	Mode = 1;
	SetActorLocation(Center);
	ArcCenter = FVector::ZeroVector; ArcYaw = Yaw; ArcA0 = StartAng; ArcA1 = EndAng; ArcR = Radius; ArcTilt = Tilt; bArcHeavy = bHeavy;
	Duration = bHeavy ? 0.28f : 0.18f;
	Mesh->SetMaterial(0, ErtFxMat(true));
	Rebuild();
}

void AErtBurst::Rebuild()
{
	FErtMeshData M;
	if (Mode == 0)
	{
		for (const FP& P : Ps)
		{
			if (P.Life <= 0.f) continue;
			const float A = FMath::Clamp(P.Life / (Duration * 0.5f), 0.f, 1.f);
			FLinearColor C = P.C; C.A = A;
			const float S = P.Size;
			// Ikki kesishgan kvadrat (kameraga bog'liq emas - arzon)
			M.AddQuad(P.P + FVector(-S, 0, -S), P.P + FVector(S, 0, -S), P.P + FVector(S, 0, S), P.P + FVector(-S, 0, S), FVector::RightVector, C);
			M.AddQuad(P.P + FVector(0, -S, -S), P.P + FVector(0, S, -S), P.P + FVector(0, S, S), P.P + FVector(0, -S, S), FVector::ForwardVector, C);
		}
	}
	else
	{
		// Yoy: T bo'yicha kengayadi va so'nadi; ichki radius 0.55R, tashqi R; oxiri (yangi qism) yorqinroq
		const float K = FMath::Clamp(T / Duration, 0.f, 1.f);
		const float Reach = FMath::Lerp(ArcA0, ArcA1, FMath::Min(1.f, K * 1.6f));
		const int32 Segs = 14;
		const FQuat Q = FRotator(ArcTilt, ArcYaw, 0).Quaternion();
		const float Alpha = (1.f - K) * (bArcHeavy ? 0.7f : 0.5f);
		for (int32 i = 0; i < Segs; ++i)
		{
			const float a0 = FMath::DegreesToRadians(FMath::Lerp(ArcA0, Reach, (float)i / Segs)), a1 = FMath::DegreesToRadians(FMath::Lerp(ArcA0, Reach, (float)(i + 1) / Segs));
			const float Fade0 = (float)i / Segs, Fade1 = (float)(i + 1) / Segs;
			const FVector In0 = Q.RotateVector(FVector(FMath::Cos(a0), FMath::Sin(a0), 0) * ArcR * 0.55f), Out0 = Q.RotateVector(FVector(FMath::Cos(a0), FMath::Sin(a0), 0) * ArcR);
			const FVector In1 = Q.RotateVector(FVector(FMath::Cos(a1), FMath::Sin(a1), 0) * ArcR * 0.55f), Out1 = Q.RotateVector(FVector(FMath::Cos(a1), FMath::Sin(a1), 0) * ArcR);
			const FLinearColor C(0.95f, 0.95f, 1.f, Alpha * (0.25f + 0.75f * Fade1));
			(void)Fade0;
			M.AddQuad(In0, Out0, Out1, In1, Q.RotateVector(FVector::UpVector), C);
		}
	}
	if (M.Verts.Num()) Mesh->CreateMeshSection_LinearColor(0, M.Verts, M.Tris, M.Normals, M.UVs, M.Colors, M.Tangents, false, false);
	else Mesh->ClearMeshSection(0);
}

void AErtBurst::Tick(float Dt)
{
	Super::Tick(Dt);
	T += Dt;
	if (Mode == 0)
	{
		bool bAny = false;
		for (FP& P : Ps)
		{
			if (P.Life <= 0.f) continue;
			P.Life -= Dt; bAny = true;
			if (P.bStuck) continue;
			P.V.Z -= 980.f * Dt * 1.2f;
			P.V *= FMath::Max(0.f, 1.f - 1.8f * Dt);
			P.P += P.V * Dt;
			if (P.P.Z < -GetActorLocation().Z + 2.f)
			{
				P.P.Z = -GetActorLocation().Z + 2.f; P.bStuck = true; P.Size *= 1.6f;
				// Yerga tekkan qon: doimiy dog' (yer balandligi: trace)
				if (!bSplatDone && P.C.R > 0.3f && P.C.G < 0.1f)
				{
					bSplatDone = true;
					FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtSplat), true, this);
					const FVector Wp = GetActorLocation() + FVector(P.P.X, P.P.Y, 0);
					if (GetWorld()->LineTraceSingleByChannel(Hit, Wp + FVector(0, 0, 150.f), Wp - FVector(0, 0, 300.f), ECC_Visibility, Q) && Hit.ImpactNormal.Z > 0.7f)
						if (AErtSplats* S = AErtSplats::Get(GetWorld())) S->AddSplat(Hit.ImpactPoint, FMath::FRandRange(28.f, 55.f) * FMath::Sqrt((float)Ps.Num() / 14.f), FLinearColor(0.30f, 0.02f, 0.01f));
				}
			}
		}
		if (!bAny || T > Duration + 0.2f) { Destroy(); return; }
	}
	else if (T > Duration) { Destroy(); return; }
	Rebuild();
}

void AErtBurst::Blood(UWorld* W, const FVector& Pos, const FVector& Dir, float Strength)
{
	if (!W) return;
	FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AErtBurst* B = W->SpawnActor<AErtBurst>(AErtBurst::StaticClass(), Pos, FRotator::ZeroRotator, SP)) B->InitParticles(Pos, Dir, 0, FMath::RoundToInt(22 * Strength));
}

void AErtBurst::Sparks(UWorld* W, const FVector& Pos, const FVector& Dir)
{
	if (!W) return;
	FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AErtBurst* B = W->SpawnActor<AErtBurst>(AErtBurst::StaticClass(), Pos, FRotator::ZeroRotator, SP)) B->InitParticles(Pos, Dir, 1, 12);
}

void AErtBurst::Dust(UWorld* W, const FVector& Pos, float Strength)
{
	if (!W) return;
	FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AErtBurst* B = W->SpawnActor<AErtBurst>(AErtBurst::StaticClass(), Pos, FRotator::ZeroRotator, SP)) B->InitParticles(Pos, FVector::UpVector, 2, FMath::RoundToInt(10 * Strength));
}

void AErtBurst::SwordArc(UWorld* W, const FVector& Center, float Yaw, int32 AttackKind)
{
	if (!W || AttackKind == 3) return;
	FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AErtBurst* B = W->SpawnActor<AErtBurst>(AErtBurst::StaticClass(), Center, FRotator::ZeroRotator, SP);
	if (!B) return;
	// 0: o'ngdan chapga gorizontal, 1: chapdan o'ngga pastdan yuqoriga, 2: og'ir vertikal (yuqoridan pastga)
	if (AttackKind == 0) B->InitArc(Center, Yaw, 70.f, -70.f, 150.f, 10.f, false);
	else if (AttackKind == 1) B->InitArc(Center, Yaw, -70.f, 70.f, 145.f, -25.f, false);
	else B->InitArc(Center, Yaw, 110.f, -20.f, 170.f, 80.f, true);
}


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
