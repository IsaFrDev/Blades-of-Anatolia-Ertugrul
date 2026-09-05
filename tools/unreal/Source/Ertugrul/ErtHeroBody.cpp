#include "ErtHeroBody.h"
#include "ErtProcMesh.h"
#include "Ertugrul.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UErtHeroBody::UErtHeroBody()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UProceduralMeshComponent* UErtHeroBody::MakePart(const FName& Name, USceneComponent* Parent, const FVector& RelLoc)
{
	AActor* Owner = GetOwner();
	UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(Owner, MakeUniqueObjectName(Owner, UProceduralMeshComponent::StaticClass(), Name));
	P->SetupAttachment(Parent);
	P->SetRelativeLocation(RelLoc);
	P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	P->bUseAsyncCooking = false;
	P->SetCastShadow(true);
	P->RegisterComponent();
	if (Mat) P->SetMaterial(0, Mat);
	return P;
}

void UErtHeroBody::Build(USceneComponent* Parent, float HalfH)
{
	if (IsBuilt() || !Parent) return;
	Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	if (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
	if (TryBuildSkeletal(Parent, HalfH)) return;

	// Bo'g'im pivotlari (sm). Tos suyagi yerdan 89 sm balandda.
	PelvisBase = FVector(0, 0, -HalfH + 89.f);
	Root = NewObject<USceneComponent>(GetOwner(), MakeUniqueObjectName(GetOwner(), USceneComponent::StaticClass(), TEXT("BodyRoot")));
	Root->SetupAttachment(Parent);
	Root->RegisterComponent();

	Pelvis = MakePart(TEXT("Pelvis"), Root, PelvisBase);
	Torso = MakePart(TEXT("Torso"), Pelvis, FVector(0, 0, 12));
	Head = MakePart(TEXT("Head"), Torso, FVector(0, 0, 47));
	UpperArmL = MakePart(TEXT("UpperArmL"), Torso, FVector(0, -21, 41));
	UpperArmR = MakePart(TEXT("UpperArmR"), Torso, FVector(0, 21, 41));
	LowerArmL = MakePart(TEXT("LowerArmL"), UpperArmL, FVector(0, 0, -30));
	LowerArmR = MakePart(TEXT("LowerArmR"), UpperArmR, FVector(0, 0, -30));
	ThighL = MakePart(TEXT("ThighL"), Pelvis, FVector(0, -9, -6));
	ThighR = MakePart(TEXT("ThighR"), Pelvis, FVector(0, 9, -6));
	ShinL = MakePart(TEXT("ShinL"), ThighL, FVector(0, 0, -43));
	ShinR = MakePart(TEXT("ShinR"), ThighR, FVector(0, 0, -43));

	FErtMeshData M;
	const FLinearColor KaftanS = ErtCol::Sty(Kaftan, ErtCol::StyleCloth), TrousersS = ErtCol::Sty(Trousers, ErtCol::StyleCloth), LeatherS = ErtCol::Sty(Leather, ErtCol::StyleLeather), SkinS = ErtCol::Sty(Skin, ErtCol::StyleSkin), SteelS = ErtCol::Sty(Steel, ErtCol::StyleMetal), FurS = ErtCol::Sty(Fur, ErtCol::StyleFur), BeardS = ErtCol::Sty(Beard, ErtCol::StyleFur), TrimS = ErtCol::Sty(Trim, ErtCol::StyleMetal);
	// Tos + kamar + (qilich belda, agar qo'lda bo'lmasa)
	M.AddBox(FVector(0, 0, 4), FVector(12, 16, 9), KaftanS);
	M.AddBox(FVector(0, 0, 10), FVector(12.6f, 16.6f, 2.5f), LeatherS);
	M.AddBox(FVector(12.8f, 0, 10), FVector(1.5f, 3, 3), TrimS);
	if (!bWoman) M.AddBox(FVector(-4, -19, -30), FVector(2.2f, 1.6f, 42), LeatherS, FRotator(12, 0, 8));
	if (!bSwordInHand && !bWoman) M.AddBox(FVector(4, -19, 14), FVector(1.5f, 4.5f, 1.5f), TrimS, FRotator(12, 0, 8));
	if (bWoman) { M.AddBox(FVector(0, 0, -34), FVector(14, 18, 42), KaftanS); M.AddBox(FVector(0, 0, -74), FVector(15, 19, 4), TrimS); } // uzun ko'ylak
	M.Commit(Pelvis, 0, false);

	// Ko'krak: kaftan + ko'krak zirhi + oltin hoshiya + yelka bo'laklari + orqa plash
	M.Reset();
	M.AddBox(FVector(0, 0, 23), FVector(13, 19, 23), KaftanS);
	M.AddBox(FVector(10, 0, 24), FVector(4, 13, 18), LeatherS);
	M.AddBox(FVector(14.2f, 0, 24), FVector(0.6f, 1.6f, 18), TrimS);
	M.AddBox(FVector(0, -21, 40), FVector(8, 4, 5), FurS);
	M.AddBox(FVector(0, 21, 40), FVector(8, 4, 5), FurS);
	M.AddBox(FVector(-13.5f, 0, 24), FVector(1.5f, 17, 22), FurS);
	const FLinearColor CloakS = ErtCol::Sty(Cloak, ErtCol::StyleCloth), MailS = ErtCol::Sty(Steel * 0.8f, ErtCol::StyleMetal), PlateS = ErtCol::Sty(Steel * 0.92f, ErtCol::StyleMetal);
	if (bMail) { M.AddBox(FVector(0, 0, 22), FVector(13.4f, 19.4f, 21), MailS); M.AddBox(FVector(0, 0, 44), FVector(11, 17, 3), MailS); }
	if (bLamellar)
	{
		for (int32 r = 0; r < 5; ++r) for (int32 cc = -2; cc <= 2; ++cc)
		{
			M.AddBox(FVector(13.6f, cc * 6.2f, 6 + r * 7.2f), FVector(0.7f, 2.8f, 3.4f), ErtCol::Vary(PlateS, 0.06f, r * 5 + cc), FRotator(-6, 0, 0));    // oldingi qatorlar
			M.AddBox(FVector(-13.6f, cc * 6.2f, 6 + r * 7.2f), FVector(0.7f, 2.8f, 3.4f), ErtCol::Vary(PlateS, 0.06f, 30 + r * 5 + cc), FRotator(6, 0, 0));  // orqa qatorlar
		}
		for (int32 r = 0; r < 5; ++r) M.AddBox(FVector(0, 0, 6 + r * 7.2f), FVector(13.9f, 19.6f, 0.5f), LeatherS);   // bog'lovchi tasmalar
	}
	if (bPauldrons)
	{
		for (int32 s = -1; s <= 1; s += 2)
		{
			M.AddSphere(FVector(0, s * 22.f, 42), 8.f, 10, PlateS, FVector(1.0f, 1.1f, 0.65f));
			M.AddBox(FVector(0, s * 24.f, 37), FVector(6.5f, 3.5f, 2.f), ErtCol::Vary(PlateS, 0.05f, 7 + s));
			M.AddBox(FVector(0, s * 26.f, 33), FVector(6.f, 3.f, 1.8f), ErtCol::Vary(PlateS, 0.05f, 9 + s));
			M.AddSphere(FVector(0, s * 22.f, 47), 1.6f, 6, TrimS);
		}
	}
	if (bCloak)
	{
		M.AddBox(FVector(-16.5f, 0, 12), FVector(1.2f, 18, 36), CloakS, FRotator(-5, 0, 0));
		M.AddBox(FVector(-15.5f, 0, 45), FVector(3.f, 16, 3.f), CloakS);
		M.AddBox(FVector(-16.f, 0, -18), FVector(1.5f, 19, 8), ErtCol::Sty(Cloak * 0.85f, ErtCol::StyleCloth), FRotator(-8, 0, 0));
		M.AddSphere(FVector(6, -16, 44), 1.8f, 6, TrimS); M.AddSphere(FVector(6, 16, 44), 1.8f, 6, TrimS);   // to'g'nog'ichlar
	}
	if (bQuiver)
	{
		M.AddCylinder(FVector(-14, 8, 8), 3.6f, 3.2f, 34, 8, LeatherS, true, FRotator(-18, 0, 12));
		M.AddCylinder(FVector(-13, 8, 8), 3.8f, 3.8f, 2, 8, TrimS, true, FRotator(-18, 0, 12));
		for (int32 i = 0; i < 6; ++i) M.AddBox(FVector(-20 + (i % 3) * 1.6f, 6 + (i / 3) * 2.5f, 44), FVector(0.4f, 0.4f, 4), ErtCol::Sty(FLinearColor(0.85f, 0.85f, 0.8f), ErtCol::StylePlain), FRotator(-18, 0, 12));
		M.AddBox(FVector(0, 0, 30), FVector(13.3f, 2.f, 1.2f), LeatherS, FRotator(0, 0, -30));   // sadoq tasmasi
	}
	if (bBackShield)
	{
		M.AddCylinder(FVector(-16.5f, 0, 24), 20.f, 20.f, 2.5f, 12, ErtCol::Sty(FLinearColor(0.35f, 0.22f, 0.10f), ErtCol::StyleWood), true, FRotator(0, 0, 90.f));
		M.AddCylinder(FVector(-19.f, 0, 24), 6.f, 5.f, 3.f, 8, PlateS, true, FRotator(0, 0, 90.f));
		M.AddCylinder(FVector(-19.f, 0, 24), 20.5f, 20.5f, 1.f, 12, TrimS, false, FRotator(0, 0, 90.f));
	}
	M.Commit(Torso, 0, false);

	// Bosh: bo'yin, bosh, soqol, burun, bo'rk yoki dubulg'a
	M.Reset();
	M.AddCylinder(FVector(0, 0, 0), 5, 5, 7, 8, SkinS);
	M.AddSphere(FVector(0, 0, 17), 11, 12, SkinS, FVector(1.0f, 0.95f, 1.15f));
	if (!bWoman) M.AddBox(FVector(8, 0, 9), FVector(4, 7, 6), BeardS);
	M.AddBox(FVector(11.5f, 0, 17), FVector(2, 2, 2.5f), SkinS);
	if (bWoman)
	{
		// Ro'mol: bosh atrofida va orqaga tushgan
		M.AddSphere(FVector(-1, 0, 19), 12.8f, 12, FurS, FVector(1, 1, 1.05f));
		M.AddBox(FVector(-9, 0, 2), FVector(4, 12, 16), FurS);
		M.AddCylinder(FVector(0, 0, 26), 12.f, 12.f, 3.f, 12, TrimS);
	}
	else if (bTurban)
	{
		const FLinearColor TurbS = ErtCol::Sty(Turban, ErtCol::StyleCloth);
		M.AddCylinder(FVector(0, 0, 24), 12.6f, 12.6f, 4.f, 12, ErtCol::Vary(TurbS, 0.05f, 11), true, FRotator(0, 0, 6.f));
		M.AddCylinder(FVector(0, 0, 27.5f), 13.2f, 12.4f, 4.f, 12, ErtCol::Vary(TurbS, 0.05f, 12), true, FRotator(5.f, 40.f, -6.f));
		M.AddCylinder(FVector(0, 0, 31), 12.2f, 10.5f, 3.5f, 12, ErtCol::Vary(TurbS, 0.05f, 13), true, FRotator(-4.f, 80.f, 5.f));
		M.AddSphere(FVector(0, 0, 34), 9.5f, 10, ErtCol::Sty(Kaftan * 0.9f, ErtCol::StyleCloth), FVector(1, 1, 0.55f));
		M.AddBox(FVector(10.5f, 0, 30), FVector(2.5f, 3.5f, 2.f), TrimS);   // salla bezagi
	}
	else if (bHelmet)
	{
		M.AddSphere(FVector(0, 0, 20), 12.5f, 12, SteelS, FVector(1, 1, 0.9f));
		M.AddCone(FVector(0, 0, 30), 3.f, 8.f, 6, SteelS);
		M.AddBox(FVector(11.8f, 0, 14), FVector(1.5f, 1.5f, 6), SteelS); // burun himoyasi
	}
	else
	{
		M.AddCylinder(FVector(0, 0, 24), 12.5f, 12.5f, 9, 12, FurS);
		M.AddCylinder(FVector(0, 0, 33), 11.5f, 8, 9, 12, ErtCol::Vary(KaftanS, 0.15f, 3));
	}
	M.Commit(Head, 0, false);

	// Qo'llar
	for (int32 Side = 0; Side < 2; ++Side)
	{
		M.Reset();
		M.AddBox(FVector(0, 0, -15), FVector(5.5f, 5.5f, 15), KaftanS);
		M.Commit(Side ? UpperArmR : UpperArmL, 0, false);
		M.Reset();
		M.AddBox(FVector(0, 0, -13), FVector(5, 5, 13), KaftanS);
		M.AddBox(FVector(0, 0, -22), FVector(5.4f, 5.4f, 5), LeatherS);
		M.AddBox(FVector(0.5f, 0, -30), FVector(4, 3.5f, 5), SkinS);
		if (Side == 1 && bSwordInHand)
		{
			M.AddBox(FVector(0.5f, 0, -30), FVector(1.2f, 6, 1.2f), TrimS);               // qo'riqlovchi
			M.AddBox(FVector(42, 0, -30), FVector(40, 0.6f, 2.8f), SteelS);               // tig'
		}
		M.Commit(Side ? LowerArmR : LowerArmL, 0, false);
	}
	// Oyoqlar
	for (int32 Side = 0; Side < 2; ++Side)
	{
		M.Reset();
		M.AddBox(FVector(0, 0, -22), FVector(7, 7, 22), TrousersS);
		M.Commit(Side ? ThighR : ThighL, 0, false);
		M.Reset();
		M.AddBox(FVector(0, 0, -20), FVector(6, 6, 20), TrousersS);
		if (bBoots)
		{
			M.AddBox(FVector(0, 0, -25), FVector(6.7f, 6.7f, 17), ErtCol::Sty(Leather * 0.9f, ErtCol::StyleLeather));
			M.AddBox(FVector(0, 0, -9), FVector(7.0f, 7.0f, 1.5f), ErtCol::Sty(Leather * 0.7f, ErtCol::StyleLeather));   // etik og'zi
			M.AddBox(FVector(0, 0, -30), FVector(7.1f, 7.1f, 1.f), TrimS);                                                // to'qa tasma
		}
		else M.AddBox(FVector(0, 0, -30), FVector(6.5f, 6.5f, 12), LeatherS);
		M.AddBox(FVector(6, 0, -43), FVector(13, 6, 3.5f), LeatherS);
		M.Commit(Side ? ShinR : ShinL, 0, false);
	}
	Apply(Cur);
}

void UErtHeroBody::Apply(const FPose& P)
{
	Pelvis->SetRelativeLocation(PelvisBase + FVector(0, 0, P.PelvisZ));
	Pelvis->SetRelativeRotation(FRotator(P.PelvisPitch, 0, 0));
	Torso->SetRelativeRotation(FRotator(P.TorsoPitch, P.TorsoYaw, P.TorsoRoll));
	Head->SetRelativeRotation(FRotator(P.HeadPitch, -P.TorsoYaw * 0.6f, -P.TorsoRoll * 0.5f));
	ThighL->SetRelativeRotation(FRotator(P.ThighL, 0, -P.LegSpread));
	ThighR->SetRelativeRotation(FRotator(P.ThighR, 0, P.LegSpread));
	// Pitch < 0 = oldinga (pivot ostidagi bo'lak uchun). Tizza faqat orqaga bukiladi.
	ShinL->SetRelativeRotation(FRotator(P.KneeL, 0, 0));
	ShinR->SetRelativeRotation(FRotator(P.KneeR, 0, 0));
	UpperArmL->SetRelativeRotation(FRotator(P.ArmL, 0, P.ArmSpread));
	UpperArmR->SetRelativeRotation(FRotator(P.ArmR, 0, -P.ArmSpread));
	LowerArmL->SetRelativeRotation(FRotator(-P.ElbowL, 0, 0));
	LowerArmR->SetRelativeRotation(FRotator(-P.ElbowR, 0, 0));
}

void UErtHeroBody::SetShield(bool bOn)
{
	if (!IsBuilt() || Skel) return;
	if (!bOn) { if (Shield) { Shield->DestroyComponent(); Shield = nullptr; } return; }
	if (Shield) return;
	Shield = MakePart(TEXT("Shield"), LowerArmL, FVector(0, -7, -18));
	FErtMeshData M;
	const FLinearColor KaftanS = ErtCol::Sty(Kaftan, ErtCol::StyleCloth), TrousersS = ErtCol::Sty(Trousers, ErtCol::StyleCloth), LeatherS = ErtCol::Sty(Leather, ErtCol::StyleLeather), SkinS = ErtCol::Sty(Skin, ErtCol::StyleSkin), SteelS = ErtCol::Sty(Steel, ErtCol::StyleMetal), FurS = ErtCol::Sty(Fur, ErtCol::StyleFur), BeardS = ErtCol::Sty(Beard, ErtCol::StyleFur), TrimS = ErtCol::Sty(Trim, ErtCol::StyleMetal);
	M.AddCylinder(FVector(0, 0, 0), 22.f, 22.f, 2.5f, 12, ErtCol::Sty(FLinearColor(0.35f, 0.22f, 0.10f), ErtCol::StyleWood), true, FRotator(0, 0, 90.f));
	M.AddCylinder(FVector(0, -2.5f, 0), 6.f, 5.f, 3.f, 8, SteelS, true, FRotator(0, 0, 90.f));
	M.AddCylinder(FVector(0, -2.6f, 0), 22.5f, 22.5f, 1.f, 12, TrimS, false, FRotator(0, 0, 90.f));
	M.Commit(Shield, 0, false);
}

void UErtHeroBody::SetSwordTier(int32 Tier)
{
	if (!IsBuilt() || !bSwordInHand) return;
	Steel = Tier >= 2 ? FLinearColor(0.55f, 0.62f, 0.75f) : FLinearColor(0.75f, 0.77f, 0.80f);
	if (Skel) { SkelBuildSword(); return; }
	FErtMeshData M;
	const FLinearColor KaftanS = ErtCol::Sty(Kaftan, ErtCol::StyleCloth), TrousersS = ErtCol::Sty(Trousers, ErtCol::StyleCloth), LeatherS = ErtCol::Sty(Leather, ErtCol::StyleLeather), SkinS = ErtCol::Sty(Skin, ErtCol::StyleSkin), SteelS = ErtCol::Sty(Steel, ErtCol::StyleMetal), FurS = ErtCol::Sty(Fur, ErtCol::StyleFur), BeardS = ErtCol::Sty(Beard, ErtCol::StyleFur), TrimS = ErtCol::Sty(Trim, ErtCol::StyleMetal);
	M.AddBox(FVector(0, 0, -13), FVector(5, 5, 13), KaftanS);
	M.AddBox(FVector(0, 0, -22), FVector(5.4f, 5.4f, 5), LeatherS);
	M.AddBox(FVector(0.5f, 0, -30), FVector(4, 3.5f, 5), SkinS);
	M.AddBox(FVector(0.5f, 0, -30), FVector(1.2f, 6, 1.2f), TrimS);
	M.AddBox(FVector(Tier >= 2 ? 46.f : 42.f, 0, -30), FVector(Tier >= 2 ? 44.f : 40.f, 0.6f, 2.8f), SteelS);
	M.Commit(LowerArmR, 0, false);
}

void UErtHeroBody::TriggerAttack(int32 Kind)
{
	AttackT = 1.f; AttackKind = Kind;
	if (Skel)
	{
		const TCHAR* Key = Kind == 2 ? TEXT("heavy") : Kind == 3 ? TEXT("kick") : TEXT("attack");
		SkelPlay(Key, false, Kind == 2 ? 1.f : 1.25f, -1, TEXT("attack"));
		if (CurAnim) OneShotT = CurAnim->GetPlayLength() / (Kind == 2 ? 1.f : 1.25f) * 0.9f;
	}
}

void UErtHeroBody::AddWound(float SideSign, float Strength)
{
	if (!IsBuilt() || bDead || Skel) return;
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
}
void UErtHeroBody::TriggerHurt(float SideSign)
{
	HurtT = 1.f; HurtDir = SideSign;
	if (Skel && OneShotT <= 0.15f)
	{
		SkelPlay(TEXT("hurt"), false, 1.3f, -1, TEXT("idle"));
		if (CurAnim) OneShotT = FMath::Min(0.6f, CurAnim->GetPlayLength() / 1.3f);
	}
}

void UErtHeroBody::SetDead(float HalfH, int32 Variant)
{
	if (!IsBuilt() || bDead) return;
	bDead = true;
	if (Skel) { OneShotT = 0.f; SkelPlay(TEXT("death"), false, 1.f, -1, TEXT("idle")); return; }
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
}

void UErtHeroBody::Animate(float Dt, float Speed, bool bInAir, bool bCrouched, float Lean, float SlopeDeg)
{
	if (!IsBuilt() || bDead) return;
	if (Skel) { SkelAnimate(Dt, Speed, bInAir, bCrouched); return; }
	IdleT += Dt;
	AttackT = FMath::Max(0.f, AttackT - Dt / (AttackKind == 2 ? 0.75f : 0.45f));
	HurtT = FMath::Max(0.f, HurtT - Dt / 0.35f);
	ParryT = FMath::Max(0.f, ParryT - Dt / 0.3f);
	const float S = FMath::Clamp(Speed / 330.f, 0.f, 1.9f);
	const float Freq = Speed > 15.f ? (1.3f + Speed / 260.f) : 0.f;
	Phase += Dt * Freq * 2.f * PI;
	if (Phase > 2.f * PI) Phase -= 2.f * PI;

	FPose T;
	const float Sw = FMath::Sin(Phase);
	const float Swing = 27.f * FMath::Min(S, 1.35f);
	if (bRide)
	{
		// Egarda: sonlar oldinga-yonga, tizza bukilgan, qo'llar jilovda, tana engil oldinga
		RideBob = FMath::Sin(IdleT * 6.f) * FMath::Min(Speed / 400.f, 1.f);
		T.PelvisZ = -2.f + RideBob * 2.f; T.TorsoPitch = -6.f - 8.f * FMath::Min(Speed / 900.f, 1.f); T.HeadPitch = 4.f;
		T.ThighL = -72.f; T.ThighR = -72.f; T.LegSpread = 32.f; T.KneeL = 88.f; T.KneeR = 88.f;
		T.ArmL = -42.f; T.ArmR = -42.f; T.ElbowL = 48.f; T.ElbowR = 48.f; T.ArmSpread = 6.f;
	}
	else if (bSwim)
	{
		// Suzish: tana gorizontal, oyoqlar navbatma-navbat tepadi, qo'llar oldinga cho'ziladi (brass)
		const float Ph = IdleT * 4.5f + Phase;
		T.PelvisPitch = -78.f; T.PelvisZ = -30.f; T.TorsoPitch = 6.f; T.HeadPitch = 45.f;
		T.ThighL = 12.f + 18.f * FMath::Sin(Ph); T.ThighR = 12.f - 18.f * FMath::Sin(Ph);
		T.KneeL = 20.f + 15.f * FMath::Max(0.f, -FMath::Sin(Ph)); T.KneeR = 20.f + 15.f * FMath::Max(0.f, FMath::Sin(Ph));
		const float St = FMath::Sin(Ph * 0.5f);
		T.ArmL = -120.f + 50.f * St; T.ArmR = -120.f + 50.f * St;
		T.ElbowL = 10.f + 25.f * (1.f - St); T.ElbowR = T.ElbowL; T.ArmSpread = 30.f + 25.f * St;
	}
	else if (bInAir)
	{
		T.ThighL = -35.f; T.ThighR = -10.f; T.KneeL = 70.f; T.KneeR = 35.f;
		T.ArmL = -35.f; T.ArmR = -35.f; T.ElbowL = 30.f; T.ElbowR = 30.f; T.ArmSpread = 20.f;
		T.TorsoPitch = -8.f;
	}
	else if (bCrouched)
	{
		// Cho'kkalash: tos pastga, sonlar oldinga, tizza qattiq bukilgan - oyoq tos ostida qoladi
		T.PelvisZ = -42.f; T.TorsoPitch = -24.f; T.HeadPitch = 16.f;
		T.ThighL = -62.f - Sw * Swing * 0.4f; T.ThighR = -62.f + Sw * Swing * 0.4f;
		T.KneeL = 118.f; T.KneeR = 118.f;
		T.ArmL = -25.f; T.ArmR = -25.f; T.ElbowL = 55.f; T.ElbowR = 55.f; T.ArmSpread = 8.f;
	}
	else if (S < 0.04f)
	{
		const float Br = FMath::Sin(IdleT * 1.7f);
		T.PelvisZ = Br * 0.6f; T.TorsoPitch = -2.f + Br * 0.8f; T.HeadPitch = 2.f;
		T.ArmL = 4.f; T.ArmR = 4.f; T.ElbowL = 12.f; T.ElbowR = 12.f; T.ArmSpread = 6.f;
	}
	else
	{
		T.ThighL = -Sw * Swing; T.ThighR = Sw * Swing;
		const float Bend = 25.f + 40.f * FMath::Min(S, 1.2f);
		T.KneeL = FMath::Max(0.f, Sw) * Bend;
		T.KneeR = FMath::Max(0.f, -Sw) * Bend;
		const float ArmSw = Swing * (S > 1.05f ? 1.2f : 0.8f);
		T.ArmL = Sw * ArmSw; T.ArmR = -Sw * ArmSw;
		T.ElbowL = 15.f + 55.f * FMath::Clamp(S - 0.9f, 0.f, 1.f); T.ElbowR = T.ElbowL;
		T.ArmSpread = 4.f + 6.f * FMath::Min(S, 1.f);
		T.TorsoPitch = -(3.f + 9.f * FMath::Min(S, 1.6f)) + SlopeDeg * 0.25f;
		T.TorsoRoll = Lean * 6.f;
		T.HeadPitch = 3.f + 6.f * FMath::Min(S, 1.6f);
		T.PelvisZ = -FMath::Abs(Sw) * (2.f + 3.f * FMath::Min(S, 1.5f));
	}
	if (bBlock && AttackT <= 0.f)
	{
		// Blok: qilich ko'krak oldida ko'ndalang, chap qo'l yuzni yopadi
		T.ArmR = -70.f; T.ElbowR = 95.f; T.ArmL = -55.f; T.ElbowL = 80.f; T.ArmSpread = 14.f; T.TorsoPitch -= 4.f;
	}
	if (ParryT > 0.f)
	{
		T.ArmR = -95.f + 30.f * ParryT; T.ElbowR = 60.f; T.TorsoYaw = -18.f * ParryT; T.TorsoPitch -= 6.f * ParryT;
	}
	if (bSwordInHand && AttackT <= 0.f && !bInAir && !bBlock)
	{
		// Qilich tayyor holat: o'ng qo'l oldinda, tirsak bukilgan
		T.ArmR = -15.f; T.ElbowR = 70.f;
	}
	if (AttackT > 0.f)
	{
		const float K = 1.f - AttackT;               // 0 -> 1
		const float Sweep = FMath::Sin(K * PI * 0.85f);
		if (AttackKind == 1)
		{
			// Chapdan qaytish zarbasi: tana teskari buriladi, qo'l pastdan yuqoriga
			T.ArmR = FMath::Lerp(-95.f, 60.f, FMath::Clamp(K * 1.4f, 0.f, 1.f));
			T.ElbowR = 25.f; T.ArmSpread = 40.f * (1.f - K);
			T.TorsoYaw = FMath::Lerp(32.f, -30.f, FMath::Clamp(K * 1.3f, 0.f, 1.f));
		}
		else if (AttackKind == 2)
		{
			// Og'ir zarba: ikki qo'l tepaga (kutish), keyin qattiq pastga
			const float Up = FMath::Clamp(K / 0.45f, 0.f, 1.f), Down = FMath::Clamp((K - 0.45f) / 0.3f, 0.f, 1.f);
			T.ArmR = FMath::Lerp(-40.f, -175.f, Up) + 200.f * Down; T.ArmL = T.ArmR + 10.f;
			T.ElbowR = 20.f; T.ElbowL = 20.f; T.ArmSpread = 6.f;
			T.TorsoPitch += 12.f * Up - 30.f * Down; T.PelvisZ -= 8.f * Down;
		}
		else if (AttackKind == 3)
		{
			// Tepki: o'ng oyoq oldinga
			T.ThighR = FMath::Lerp(20.f, -95.f, FMath::Clamp(K * 1.6f, 0.f, 1.f)) * (K < 0.8f ? 1.f : (1.f - K) * 5.f);
			T.KneeR = 30.f * (1.f - Sweep); T.TorsoPitch += 8.f; T.ArmL = -30.f; T.ArmR = 40.f;
		}
		else
		{
			// O'ngdan: qo'l orqadan-yuqoridan oldinga siltanadi, tana buriladi
			T.ArmR = FMath::Lerp(115.f, -75.f, FMath::Clamp(K * 1.4f, 0.f, 1.f));
			T.ElbowR = 15.f + 30.f * (1.f - Sweep);
			T.ArmSpread = 12.f;
			T.TorsoYaw = FMath::Lerp(-28.f, 32.f, FMath::Clamp(K * 1.3f, 0.f, 1.f));
			T.TorsoPitch -= 10.f * Sweep;
		}
	}
	if (HurtT > 0.f) { T.TorsoPitch += (HurtDir == 0.f ? 18.f : 8.f) * HurtT; T.HeadPitch -= 10.f * HurtT; T.TorsoYaw += 22.f * HurtDir * HurtT; T.TorsoRoll += 12.f * HurtDir * HurtT; T.ArmL += 25.f * HurtDir * HurtT; T.ArmR -= 25.f * HurtDir * HurtT; }

	const float A = FMath::Clamp(Dt * (AttackT > 0.f ? 22.f : 12.f), 0.f, 1.f);
	auto L = [A](float& C, float Tg) { C = FMath::Lerp(C, Tg, A); };
	L(Cur.PelvisZ, T.PelvisZ); L(Cur.PelvisPitch, T.PelvisPitch); L(Cur.TorsoPitch, T.TorsoPitch); L(Cur.TorsoRoll, T.TorsoRoll); L(Cur.TorsoYaw, T.TorsoYaw); L(Cur.HeadPitch, T.HeadPitch);
	L(Cur.ThighL, T.ThighL); L(Cur.ThighR, T.ThighR); L(Cur.KneeL, T.KneeL); L(Cur.KneeR, T.KneeR);
	L(Cur.ArmL, T.ArmL); L(Cur.ArmR, T.ArmR); L(Cur.ElbowL, T.ElbowL); L(Cur.ElbowR, T.ElbowR); L(Cur.ArmSpread, T.ArmSpread); L(Cur.LegSpread, T.LegSpread);
	Apply(Cur);
}


// ---------------- Skeletli rejim (character.json) ----------------

namespace
{
	TSharedPtr<FJsonObject> ErtCharacterJson()
	{
		static TSharedPtr<FJsonObject> Root; static bool bTried = false;
		if (bTried) return Root;
		bTried = true;
		FString Json;
		if (FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/character.json"))))
		{
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
			if (!FJsonSerializer::Deserialize(Reader, Root)) Root.Reset();
		}
		return Root;
	}
	FString ErtObjPath(FString P)
	{
		if (!P.Contains(TEXT("."))) { const FString Name = FPaths::GetBaseFilename(P); P = P + TEXT(".") + Name; }
		return P;
	}
}

bool UErtHeroBody::TryBuildSkeletal(USceneComponent* Parent, float HalfH)
{
	TSharedPtr<FJsonObject> Cfg = ErtCharacterJson();
	if (!Cfg.IsValid()) return false;
	const TSharedPtr<FJsonObject>* Prof = nullptr;
	if (!Cfg->TryGetObjectField(Profile, Prof) || !Prof->IsValid()) return false;
	FString MeshPath; if (!(*Prof)->TryGetStringField(TEXT("mesh"), MeshPath) || MeshPath.IsEmpty()) return false;
	USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, *ErtObjPath(MeshPath));
	if (!SM) { UE_LOG(LogErtugrul, Warning, TEXT("character.json: mesh topilmadi %s"), *MeshPath); return false; }
	AActor* Owner = GetOwner();
	Skel = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("SkelBody")));
	Skel->SetupAttachment(Parent);
	const double Yaw = (*Prof)->HasField(TEXT("yaw")) ? (*Prof)->GetNumberField(TEXT("yaw")) : -90.0;
	const double Zo = (*Prof)->HasField(TEXT("z")) ? (*Prof)->GetNumberField(TEXT("z")) : 0.0;
	const double Sc = (*Prof)->HasField(TEXT("scale")) ? (*Prof)->GetNumberField(TEXT("scale")) : 1.0;
	Skel->SetRelativeLocation(FVector(0, 0, -HalfH + Zo));
	Skel->SetRelativeRotation(FRotator(0, Yaw, 0));
	Skel->SetRelativeScale3D(FVector(Sc));
	Skel->SetSkeletalMesh(SM);
	Skel->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skel->SetCastShadow(true);
	Skel->RegisterComponent();
	if ((*Prof)->HasField(TEXT("walk_ref"))) WalkRef = (*Prof)->GetNumberField(TEXT("walk_ref"));
	if ((*Prof)->HasField(TEXT("run_ref"))) RunRef = (*Prof)->GetNumberField(TEXT("run_ref"));
	const TSharedPtr<FJsonObject>* Anims = nullptr;
	if ((*Prof)->TryGetObjectField(TEXT("anims"), Anims) && Anims->IsValid())
	{
		for (const auto& Pair : (*Anims)->Values)
		{
			TArray<TObjectPtr<UAnimSequence>> List;
			auto Add = [&](const FString& P) { if (UAnimSequence* A = LoadObject<UAnimSequence>(nullptr, *ErtObjPath(P))) List.Add(A); else UE_LOG(LogErtugrul, Warning, TEXT("character.json: animatsiya topilmadi %s"), *P); };
			if (Pair.Value->Type == EJson::String) Add(Pair.Value->AsString());
			else if (Pair.Value->Type == EJson::Array) for (const TSharedPtr<FJsonValue>& V : Pair.Value->AsArray()) Add(V->AsString());
			if (List.Num()) SkelAnims.Add(FString(Pair.Key.ToView()), List);
		}
	}
	const TSharedPtr<FJsonObject>* Sw = nullptr;
	if ((*Prof)->TryGetObjectField(TEXT("sword"), Sw) && Sw->IsValid())
	{
		FString Sock; if ((*Sw)->TryGetStringField(TEXT("socket"), Sock)) SwordSocket = FName(*Sock);
		const TArray<TSharedPtr<FJsonValue>>* L = nullptr;
		if ((*Sw)->TryGetArrayField(TEXT("loc"), L) && L->Num() == 3) SwordLoc = FVector((*L)[0]->AsNumber(), (*L)[1]->AsNumber(), (*L)[2]->AsNumber());
		if ((*Sw)->TryGetArrayField(TEXT("rot"), L) && L->Num() == 3) SwordRot = FRotator((*L)[0]->AsNumber(), (*L)[1]->AsNumber(), (*L)[2]->AsNumber());
	}
	if (bSwordInHand) SkelBuildSword();
	SkelPlay(TEXT("idle"), true);
	UE_LOG(LogErtugrul, Log, TEXT("Skeletli tana (%s): %s, %d animatsiya turi"), *Profile, *SM->GetName(), SkelAnims.Num());
	return true;
}

void UErtHeroBody::SkelBuildSword()
{
	if (!Skel) return;
	if (!SkelSword)
	{
		SkelSword = MakePart(TEXT("SkelSword"), Skel, FVector::ZeroVector);
		SkelSword->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SwordSocket);
		SkelSword->SetRelativeLocation(SwordLoc); SkelSword->SetRelativeRotation(SwordRot);
	}
	FErtMeshData M;
	const FLinearColor SteelS = ErtCol::Sty(Steel, ErtCol::StyleMetal), TrimS = ErtCol::Sty(Trim, ErtCol::StyleMetal), LeatherS = ErtCol::Sty(Leather, ErtCol::StyleLeather);
	// Mannequin suyaklarida X o'qi barmoqlar tomon: tig' +X bo'ylab (mushtdan tashqariga), dasta -X
	M.AddBox(FVector(-10, 0, 0), FVector(10, 1.6f, 1.6f), LeatherS);            // dasta (kaftda)
	M.AddBox(FVector(2, 0, 0), FVector(1.5f, 1.5f, 7), TrimS);                   // qo'riqlovchi
	M.AddBox(FVector(46, 0, 0), FVector(42, 0.6f, 3.2f), SteelS);                // tig'
	M.AddSphere(FVector(-21, 0, 0), 2.f, 6, TrimS);                              // soqqa
	M.Commit(SkelSword, 0, false);
}

UAnimSequence* UErtHeroBody::SkelPick(const FString& Key, int32 Index) const
{
	const TArray<TObjectPtr<UAnimSequence>>* L = SkelAnims.Find(Key);
	if (!L || L->Num() == 0) return nullptr;
	return (*L)[Index >= 0 ? Index % L->Num() : FMath::RandRange(0, L->Num() - 1)];
}

void UErtHeroBody::SkelPlay(const FString& Key, bool bLoop, float Rate, int32 Index, const TCHAR* Fallback)
{
	if (!Skel) return;
	UAnimSequence* A = SkelPick(Key, Index);
	if (!A && Fallback) A = SkelPick(Fallback, Index);
	if (!A) A = SkelPick(TEXT("idle"), 0);
	if (!A) return;
	if (A != CurAnim || !Skel->IsPlaying())
	{
		CurAnim = A;
		Skel->PlayAnimation(A, bLoop);
	}
	Skel->SetPlayRate(Rate);
}

void UErtHeroBody::SkelAnimate(float Dt, float Speed, bool bInAir, bool bCrouched)
{
	IdleT += Dt;
	AttackT = FMath::Max(0.f, AttackT - Dt / (AttackKind == 2 ? 0.75f : 0.45f));
	HurtT = FMath::Max(0.f, HurtT - Dt / 0.35f);
	ParryT = FMath::Max(0.f, ParryT - Dt / 0.3f);
	if (OneShotT > 0.f) { OneShotT -= Dt; if (OneShotT > 0.f) return; }
	if (bSwim) { SkelPlay(TEXT("swim"), true, 1.f, 0, TEXT("fall")); return; }
	if (bRide) { SkelPlay(TEXT("ride"), true, 1.f, 0, TEXT("idle")); return; }
	if (bInAir) { SkelPlay(TEXT("fall"), true, 1.f, 0, TEXT("jump")); return; }
	if (Speed < 15.f) { SkelPlay(bBlock ? TEXT("block") : (bCrouched ? TEXT("crouch") : TEXT("idle")), true, 1.f, 0, TEXT("idle")); return; }
	if (bCrouched) { SkelPlay(TEXT("crouchwalk"), true, FMath::Clamp(Speed / WalkRef, 0.5f, 1.4f), 0, TEXT("walk")); return; }
	if (Speed < 420.f) SkelPlay(TEXT("walk"), true, FMath::Clamp(Speed / WalkRef, 0.5f, 1.6f), 0, TEXT("idle"));
	else SkelPlay(TEXT("run"), true, FMath::Clamp(Speed / RunRef, 0.6f, 1.6f), 0, TEXT("walk"));
}
