#include "ErtProcMesh.h"

void FErtMeshData::Reset()
{
	Verts.Reset(); Tris.Reset(); Normals.Reset(); UVs.Reset(); Colors.Reset(); Tangents.Reset();
}

void FErtMeshData::AddTri(const FVector& A, const FVector& B, const FVector& C, const FVector& Outward, const FLinearColor& Col)
{
	// UE (chap qo'l tizimi): A,B,C tartibidagi uchburchakning old yuzi -Cross(B-A, C-A) tomonga qaraydi
	FVector N = -FVector::CrossProduct(B - A, C - A);
	if (N.IsNearlyZero()) return;
	N.Normalize();
	const bool bFlip = FVector::DotProduct(N, Outward) < 0.f;
	if (bFlip) N = -N;
	const int32 Base = Verts.Num();
	Verts.Add(A); Verts.Add(bFlip ? C : B); Verts.Add(bFlip ? B : C);
	for (int32 i = 0; i < 3; ++i)
	{
		Normals.Add(N);
		Colors.Add(Col);
		Tangents.Add(FProcMeshTangent(FVector::CrossProduct(N, FVector::UpVector).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector), false));
	}
	UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(0, 1));
	Tris.Add(Base); Tris.Add(Base + 1); Tris.Add(Base + 2);
}

void FErtMeshData::AddQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Outward, const FLinearColor& Col)
{
	AddTri(A, B, C, Outward, Col);
	AddTri(A, C, D, Outward, Col);
}

void FErtMeshData::AddQuadUV(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Outward, const FLinearColor& Col)
{
	FVector N = -FVector::CrossProduct(B - A, C - A);
	if (N.IsNearlyZero()) return;
	N.Normalize();
	const bool bFlip = FVector::DotProduct(N, Outward) < 0.f;
	if (bFlip) N = -N;
	const int32 Base = Verts.Num();
	Verts.Add(A); Verts.Add(B); Verts.Add(C); Verts.Add(D);
	UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
	for (int32 i = 0; i < 4; ++i)
	{
		Normals.Add(N); Colors.Add(Col);
		Tangents.Add(FProcMeshTangent((B - A).GetSafeNormal(), false));
	}
	if (bFlip) { Tris.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 }); }
	else { Tris.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 }); }
}

void FErtMeshData::AddBox(const FVector& Center, const FVector& E, const FLinearColor& Col, const FRotator& Rot)
{
	const FQuat Q = Rot.Quaternion();
	auto P = [&](float X, float Y, float Z) { return Center + Q.RotateVector(FVector(X * E.X, Y * E.Y, Z * E.Z)); };
	const FVector V000 = P(-1, -1, -1), V100 = P(1, -1, -1), V110 = P(1, 1, -1), V010 = P(-1, 1, -1);
	const FVector V001 = P(-1, -1, 1), V101 = P(1, -1, 1), V111 = P(1, 1, 1), V011 = P(-1, 1, 1);
	AddQuad(V001, V101, V111, V011, Q.RotateVector(FVector::UpVector), Col);
	AddQuad(V000, V010, V110, V100, Q.RotateVector(-FVector::UpVector), Col);
	AddQuad(V100, V110, V111, V101, Q.RotateVector(FVector::ForwardVector), Col);
	AddQuad(V000, V001, V011, V010, Q.RotateVector(-FVector::ForwardVector), Col);
	AddQuad(V010, V011, V111, V110, Q.RotateVector(FVector::RightVector), Col);
	AddQuad(V000, V100, V101, V001, Q.RotateVector(-FVector::RightVector), Col);
}

void FErtMeshData::AddCylinder(const FVector& Base, float RBottom, float RTop, float Height, int32 Segs, const FLinearColor& Col,
                               bool bCaps, const FRotator& Rot, float Jitter, int32 Seed)
{
	Segs = FMath::Max(3, Segs);
	RBottom *= Unit; RTop *= Unit; Height *= Unit;
	const FQuat Q = Rot.Quaternion();
	FRandomStream RS(Seed);
	TArray<FVector> Bot, Top;
	Bot.Reserve(Segs); Top.Reserve(Segs);
	for (int32 i = 0; i < Segs; ++i)
	{
		const float Ang = 2.f * PI * i / Segs;
		const float JB = 1.f + (Jitter > 0.f ? RS.FRandRange(-Jitter, Jitter) : 0.f);
		const float JT = 1.f + (Jitter > 0.f ? RS.FRandRange(-Jitter, Jitter) : 0.f);
		Bot.Add(Base + Q.RotateVector(FVector(FMath::Cos(Ang) * RBottom * JB, FMath::Sin(Ang) * RBottom * JB, 0.f)));
		Top.Add(Base + Q.RotateVector(FVector(FMath::Cos(Ang) * RTop * JT, FMath::Sin(Ang) * RTop * JT, Height)));
	}
	const FVector Axis = Q.RotateVector(FVector::UpVector);
	const FVector TopC = Base + Axis * Height;
	for (int32 i = 0; i < Segs; ++i)
	{
		const int32 j = (i + 1) % Segs;
		const FVector Mid = (Bot[i] + Bot[j] + Top[i] + Top[j]) * 0.25f;
		const FVector Out = (Mid - (Base + Axis * (Height * 0.5f))).GetSafeNormal();
		if (RTop <= KINDA_SMALL_NUMBER)
			AddTri(Bot[i], Bot[j], TopC, Out, Col);
		else
			AddQuad(Bot[i], Bot[j], Top[j], Top[i], Out, Col);
	}
	if (bCaps)
	{
		for (int32 i = 0; i < Segs; ++i)
		{
			const int32 j = (i + 1) % Segs;
			AddTri(Base, Bot[j], Bot[i], -Axis, Col);
			if (RTop > KINDA_SMALL_NUMBER) AddTri(TopC, Top[i], Top[j], Axis, Col);
		}
	}
}

void FErtMeshData::AddSphere(const FVector& Center, float R, int32 Segs, const FLinearColor& Col, const FVector& Scale, float Jitter, int32 Seed)
{
	Segs = FMath::Max(4, Segs);
	R *= Unit;
	const int32 Rings = FMath::Max(3, Segs / 2);
	FRandomStream RS(Seed);
	TArray<FVector> P;
	P.SetNum((Rings + 1) * Segs);
	for (int32 r = 0; r <= Rings; ++r)
	{
		const float Phi = PI * r / Rings;
		for (int32 s = 0; s < Segs; ++s)
		{
			const float Th = 2.f * PI * s / Segs;
			const float J = 1.f + (Jitter > 0.f ? RS.FRandRange(-Jitter, Jitter) : 0.f);
			const FVector D(FMath::Sin(Phi) * FMath::Cos(Th), FMath::Sin(Phi) * FMath::Sin(Th), FMath::Cos(Phi));
			P[r * Segs + s] = Center + D * Scale * R * J;
		}
	}
	for (int32 r = 0; r < Rings; ++r)
		for (int32 s = 0; s < Segs; ++s)
		{
			const int32 s2 = (s + 1) % Segs;
			const FVector& A = P[r * Segs + s];
			const FVector& B = P[r * Segs + s2];
			const FVector& C = P[(r + 1) * Segs + s2];
			const FVector& D = P[(r + 1) * Segs + s];
			const FVector Out = ((A + B + C + D) * 0.25f - Center).GetSafeNormal();
			if (r == 0) AddTri(A, B, C, Out, Col);
			else if (r == Rings - 1) AddTri(A, B, D, Out, Col);
			else AddQuad(A, B, C, D, Out, Col);
		}
}

void FErtMeshData::AddGrid(const TArray<FVector>& GV, const TArray<FLinearColor>& GC, int32 W, int32 H)
{
	const int32 Base = Verts.Num();
	Verts.Append(GV);
	Colors.Append(GC);
	Normals.AddZeroed(GV.Num());
	UVs.AddUninitialized(GV.Num());
	Tangents.AddUninitialized(GV.Num());
	for (int32 y = 0; y < H; ++y)
		for (int32 x = 0; x < W; ++x)
		{
			UVs[Base + y * W + x] = FVector2D((float)x / (W - 1), (float)y / (H - 1));
			Tangents[Base + y * W + x] = FProcMeshTangent(1, 0, 0);
		}
	for (int32 y = 0; y < H - 1; ++y)
		for (int32 x = 0; x < W - 1; ++x)
		{
			const int32 I00 = Base + y * W + x, I10 = I00 + 1, I01 = I00 + W, I11 = I01 + 1;
			auto Emit = [&](int32 A, int32 B, int32 C)
			{
				FVector N = -FVector::CrossProduct(Verts[B] - Verts[A], Verts[C] - Verts[A]);
				if (N.Z < 0.f) { Swap(B, C); N = -N; }
				Tris.Add(A); Tris.Add(B); Tris.Add(C);
				Normals[A] += N; Normals[B] += N; Normals[C] += N;
			};
			Emit(I00, I10, I11);
			Emit(I00, I11, I01);
		}
	for (int32 i = Base; i < Verts.Num(); ++i) Normals[i] = Normals[i].GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
}

void FErtMeshData::Commit(UProceduralMeshComponent* Comp, int32 Section, bool bCollision) const
{
	if (!Comp || Verts.Num() == 0) return;
	Comp->CreateMeshSection_LinearColor(Section, Verts, Tris, Normals, UVs, Colors, Tangents, bCollision, /*bSRGBConversion*/ false);
}
