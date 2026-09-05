// Protsedural geometriya yordamchisi: qutilar, silindrlar, konuslar, sharlar — vertex rang bilan.
#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

struct ERTUGRUL_API FErtMeshData
{
	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	/** Silindr/konus/shar o'lchamlari (radius, balandlik) uchun birlik: 1 = sm, 100 = metr. Pozitsiyalar doim sm. */
	float Unit = 1.f;

	FErtMeshData() = default;
	explicit FErtMeshData(float InUnit) : Unit(InUnit) {}

	void Reset();
	int32 NumTris() const { return Tris.Num() / 3; }

	// Uchburchak: Outward - tashqi tomon; kerak bo'lsa aylanish tartibi almashtiriladi.
	void AddTri(const FVector& A, const FVector& B, const FVector& C, const FVector& Outward, const FLinearColor& Col);
	// To'rtburchak A,B,C,D (halqa tartibida).
	void AddQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Outward, const FLinearColor& Col);
	// To'rtburchak to'g'ri UV bilan (0,0)-(1,0)-(1,1)-(0,1): barg kartochkalari (alfa niqobli material) uchun.
	void AddQuadUV(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Outward, const FLinearColor& Col);
	// Quti: markaz, yarim o'lchamlar, rang, aylanish.
	void AddBox(const FVector& Center, const FVector& Extent, const FLinearColor& Col, const FRotator& Rot = FRotator::ZeroRotator);
	// Silindr/konus: Base - pastki markaz, o'q +Z (Rot bilan buriladi).
	void AddCylinder(const FVector& Base, float RBottom, float RTop, float Height, int32 Segs, const FLinearColor& Col,
	                 bool bCaps = true, const FRotator& Rot = FRotator::ZeroRotator, float Jitter = 0.f, int32 Seed = 0);
	void AddCone(const FVector& Base, float R, float Height, int32 Segs, const FLinearColor& Col, const FRotator& Rot = FRotator::ZeroRotator)
	{
		AddCylinder(Base, R, 0.f, Height, Segs, Col, true, Rot);
	}
	// Shar (Scale bilan ellipsoid, Jitter bilan tosh).
	void AddSphere(const FVector& Center, float R, int32 Segs, const FLinearColor& Col, const FVector& Scale = FVector::OneVector, float Jitter = 0.f, int32 Seed = 0);
	// Yassi to'r (relyef uchun): nuqtalar va ranglar tashqaridan beriladi, indekslar/normallar hisoblanadi.
	void AddGrid(const TArray<FVector>& GridVerts, const TArray<FLinearColor>& GridColors, int32 W, int32 H);

	void Commit(UProceduralMeshComponent* Comp, int32 Section, bool bCollision) const;
};

/** Grid Snapping: qiymatni to'rga yopishtirish (Grid metr yoki sm - chaqiruvchi birligida) */
inline float ErtSnap(float V, float Grid) { return Grid > 0.f ? FMath::RoundToFloat(V / Grid) * Grid : V; }
inline FVector2D ErtSnap(const FVector2D& V, float Grid) { return FVector2D(ErtSnap(V.X, Grid), ErtSnap(V.Y, Grid)); }

namespace ErtCol
{
	// Material uslubi (M_ErtVertexColor): vertex rangining alfa kanali naqshni tanlaydi
	constexpr float StyleGround = 0.0f, StyleStone = 0.2f, StyleWood = 0.4f, StyleRoof = 0.6f, StyleBrick = 0.8f, StylePlain = 1.0f;
	constexpr float StyleCloth = 0.10f, StyleLeather = 0.32f, StyleMetal = 0.50f, StyleFur = 0.70f, StyleSkin = 0.90f;   // personaj/ot: to'qima, charm, metall/zanjir, jun, badan
	constexpr float StyleFelt = 0.12f, StyleRock = 0.27f, StyleBark = 0.64f, StyleLeaf = 0.97f;   // tabiat: kigiz, qoya, po'stloq, barg
	constexpr float StyleMudRoof = 0.92f, StyleCobble = 0.95f;   // tekis tuproq tom, ko'cha tosh yotqizmasi
	inline FLinearColor Sty(const FLinearColor& C, float Style) { return FLinearColor(C.R, C.G, C.B, Style); }
	inline FLinearColor Vary(const FLinearColor& C, float Amount, int32 Seed)
	{
		FRandomStream RS(Seed);
		const float F = 1.f + RS.FRandRange(-Amount, Amount);
		return FLinearColor(FMath::Clamp(C.R * F, 0.f, 1.f), FMath::Clamp(C.G * F, 0.f, 1.f), FMath::Clamp(C.B * F, 0.f, 1.f), C.A);
	}
}
