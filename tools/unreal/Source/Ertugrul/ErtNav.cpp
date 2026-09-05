#include "ErtNav.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ErtWorldBuilder.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

AErtGps::AErtGps()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AErtGps* AErtGps::Get(UWorld* W)
{
	if (!W) return nullptr;
	if (AErtGps* G = Cast<AErtGps>(UGameplayStatics::GetActorOfClass(W, AErtGps::StaticClass()))) return G;
	return W->SpawnActor<AErtGps>(AErtGps::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
}

void AErtGps::BeginPlay()
{
	Super::BeginPlay();
	World = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(this, AErtWorldBuilder::StaticClass()));
	Ribbon = NewObject<UProceduralMeshComponent>(this, TEXT("Ribbon"));
	Ribbon->SetupAttachment(RootComponent);
	Ribbon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ribbon->SetCastShadow(false);
	Ribbon->bUseAsyncCooking = false;
	Ribbon->RegisterComponent();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtDust.M_ErtDust"))) Ribbon->SetMaterial(0, M);
	BuildCostGrid();
}

void AErtGps::BuildCostGrid()
{
	if (!World) return;
	const float Half = World->WorldSizeM * 0.5f;
	Cells = FMath::Max(10, FMath::RoundToInt(World->WorldSizeM / CellM));
	CostGrid.SetNum(Cells * Cells);
	for (int32 y = 0; y < Cells; ++y)
		for (int32 x = 0; x < Cells; ++x)
		{
			const float E = -Half + (x + 0.5f) * CellM, N = -Half + (y + 0.5f) * CellM;
			const float H = World->HeightAt(E, N);
			const float Hx = World->HeightAt(E + CellM, N), Hy = World->HeightAt(E, N + CellM);
			const float Slope = FMath::Max(FMath::Abs(Hx - H), FMath::Abs(Hy - H)) / CellM;   // m/m
			float SurfZ; const bool bWater = World->IsWater(E, N, SurfZ) && H < SurfZ - 0.3f;
			float C = 1.f + Slope * 6.f;
			if (Slope > 0.9f) C += 40.f;
			if (bWater) C += 25.f;
			CostGrid[y * Cells + x] = C;
		}
	UE_LOG(LogErtugrul, Log, TEXT("GPS: narx to'ri %dx%d"), Cells, Cells);
}

void AErtGps::SetTarget(const FVector& T)
{
	Target = T;
	if (T.IsNearlyZero()) { Path.Reset(); PathLenM = 0.f; if (Ribbon) Ribbon->ClearAllMeshSections(); }
}

bool AErtGps::FindPath(const FVector& From, const FVector& To)
{
	if (!World || CostGrid.Num() == 0) return false;
	const float Half = World->WorldSizeM * 0.5f;
	auto Idx = [&](const FVector& P, int32& X, int32& Y) { X = FMath::Clamp((int32)((P.Y / 100.f + Half) / CellM), 0, Cells - 1); Y = FMath::Clamp((int32)((P.X / 100.f + Half) / CellM), 0, Cells - 1); };
	int32 Sx, Sy, Tx, Ty; Idx(From, Sx, Sy); Idx(To, Tx, Ty);
	const int32 N = Cells * Cells;
	TArray<float> G; G.Init(1e30f, N);
	TArray<int32> Parent; Parent.Init(-1, N);
	TArray<uint8> Closed; Closed.Init(0, N);
	struct FNode { float F; int32 I; bool operator<(const FNode& O) const { return F < O.F; } };
	TArray<FNode> Open; Open.Heapify();
	const int32 S = Sy * Cells + Sx, Tg = Ty * Cells + Tx;
	G[S] = 0.f; Open.HeapPush({ 0.f, S });
	auto Heur = [&](int32 I) { const int32 X = I % Cells, Y = I / Cells; return FMath::Sqrt((float)((X - Tx) * (X - Tx) + (Y - Ty) * (Y - Ty))); };
	int32 Expanded = 0;
	while (Open.Num())
	{
		FNode Cur; Open.HeapPop(Cur);
		if (Closed[Cur.I]) continue;
		Closed[Cur.I] = 1;
		if (Cur.I == Tg) break;
		if (++Expanded > 60000) break;
		const int32 X = Cur.I % Cells, Y = Cur.I / Cells;
		for (int32 dy = -1; dy <= 1; ++dy)
			for (int32 dx = -1; dx <= 1; ++dx)
			{
				if (!dx && !dy) continue;
				const int32 Nx = X + dx, Ny = Y + dy;
				if (Nx < 0 || Ny < 0 || Nx >= Cells || Ny >= Cells) continue;
				const int32 Ni = Ny * Cells + Nx;
				if (Closed[Ni]) continue;
				const float Step = (dx && dy) ? 1.41421f : 1.f;
				const float Ng = G[Cur.I] + Step * CostGrid[Ni];
				if (Ng < G[Ni]) { G[Ni] = Ng; Parent[Ni] = Cur.I; Open.HeapPush({ Ng + Heur(Ni), Ni }); }
			}
	}
	if (Parent[Tg] < 0 && Tg != S) return false;
	TArray<FVector> Rev;
	for (int32 I = Tg; I >= 0; I = Parent[I]) { const int32 X = I % Cells, Y = I / Cells; const float E = -Half + (X + 0.5f) * CellM, Nn = -Half + (Y + 0.5f) * CellM; Rev.Add(AErtWorldBuilder::PlanToWorld(E, Nn, World->HeightAt(E, Nn))); if (I == S) break; }
	Path.Reset();
	Path.Add(FVector(From.X, From.Y, World->HeightAt(From.Y / 100.f, From.X / 100.f) * 100.f));
	for (int32 i = Rev.Num() - 2; i >= 1; --i) Path.Add(Rev[i]);   // boshlang'ich/oxirgi hujayra o'rniga aniq nuqtalar
	Path.Add(FVector(To.X, To.Y, World->HeightAt(To.Y / 100.f, To.X / 100.f) * 100.f));
	// Silliqlash: ketma-ket 3 nuqta o'rtacha (2 marta)
	for (int32 pass = 0; pass < 2; ++pass)
		for (int32 i = 1; i + 1 < Path.Num(); ++i) { FVector P = (Path[i - 1] + Path[i] * 2.f + Path[i + 1]) * 0.25f; P.Z = World->HeightAt(P.Y / 100.f, P.X / 100.f) * 100.f; Path[i] = P; }
	PathLenM = 0.f;
	for (int32 i = 1; i < Path.Num(); ++i) PathLenM += FVector::Dist2D(Path[i - 1], Path[i]) / 100.f;
	return true;
}

void AErtGps::Tick(float Dt)
{
	Super::Tick(Dt);
	Anim += Dt;
	APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!P || Target.IsNearlyZero() || !bEnabled) { if (Path.Num()) { Path.Reset(); if (Ribbon) Ribbon->ClearAllMeshSections(); } return; }
	RecalcT -= Dt;
	const FVector From = P->GetActorLocation();
	const bool bMoved = FVector::Dist2D(From, LastFrom) > 1500.f || FVector::Dist2D(Target, LastTarget) > 800.f;
	if (RecalcT <= 0.f || bMoved || Path.Num() == 0)
	{
		RecalcT = 2.5f; LastFrom = From; LastTarget = Target;
		if (FVector::Dist2D(From, Target) < 600.f) { Path.Reset(); Path.Add(From); Path.Add(Target); PathLenM = FVector::Dist2D(From, Target) / 100.f; }
		else FindPath(From, Target);
		RebuildRibbon();
	}
	else if (Path.Num() > 1)
	{
		// O'tilgan qismni kesib tashlash (o'yinchi yo'l bo'ylab yurganda lenta orqada qolmasin)
		while (Path.Num() > 2 && FVector::Dist2D(From, Path[1]) < FVector::Dist2D(Path[0], Path[1])) { Path.RemoveAt(0); RebuildRibbon(); }
	}
}

void AErtGps::RebuildRibbon()
{
	if (!Ribbon) return;
	if (Path.Num() < 2) { Ribbon->ClearAllMeshSections(); return; }
	FErtMeshData M;
	const float Wd = 22.f;
	float Dist = 0.f;
	for (int32 i = 0; i + 1 < Path.Num(); ++i)
	{
		const FVector A = Path[i] + FVector(0, 0, 9.f), B = Path[i + 1] + FVector(0, 0, 9.f);
		const FVector Dir = (B - A).GetSafeNormal2D();
		const FVector R = FVector::CrossProduct(FVector::UpVector, Dir) * Wd;
		const float SegL = FVector::Dist2D(A, B) / 100.f;
		// Strelkasimon naqsh: har 4 m da yorqinroq bo'lak (alfa bilan) - uzoqda xira
		const float Fade = FMath::Clamp(1.f - Dist / 900.f, 0.25f, 1.f);
		const FLinearColor C(1.f, 0.82f, 0.25f, 0.55f * Fade);
		M.AddQuad(A - R, A + R, B + R, B - R, FVector::UpVector, C);
		Dist += SegL;
	}
	// Maqsad ustuni (yorug')
	const FVector T = Path.Last();
	for (int32 k = 0; k < 2; ++k)
	{
		const FVector R = (k == 0 ? FVector(1, 0, 0) : FVector(0, 1, 0)) * 18.f;
		M.AddQuad(T - R, T + R, T + R + FVector(0, 0, 900.f), T - R + FVector(0, 0, 900.f), FVector(0, 0, 1), FLinearColor(1.f, 0.85f, 0.3f, 0.3f));
		M.AddQuad(T + R, T - R, T - R + FVector(0, 0, 900.f), T + R + FVector(0, 0, 900.f), FVector(0, 0, 1), FLinearColor(1.f, 0.85f, 0.3f, 0.3f));
	}
	M.Commit(Ribbon, 0, false);
}
