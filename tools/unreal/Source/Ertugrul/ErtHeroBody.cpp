#include "ErtHeroBody.h"
#include "ErtProcMesh.h"
#include "Ertugrul.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"

UErtHeroBody::UErtHeroBody()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UProceduralMeshComponent* UErtHeroBody::MakePart(const FName& Name, USceneComponent* Parent, const FVector& RelLoc)
{
	AActor* Owner = GetOwner();
	UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(Owner, Name);
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

	// Bo'g'im pivotlari (sm). Tos suyagi yerdan 89 sm balandda.
	PelvisBase = FVector(0, 0, -HalfH + 89.f);
	Root = NewObject<USceneComponent>(GetOwner(), TEXT("BodyRoot"));
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
	// Tos + kamar + qin (chap yonda qilich)
	M.AddBox(FVector(0, 0, 4), FVector(12, 16, 9), Kaftan);
	M.AddBox(FVector(0, 0, 10), FVector(12.6f, 16.6f, 2.5f), Leather);
	M.AddBox(FVector(12.8f, 0, 10), FVector(1.5f, 3, 3), Trim); // to'qa
	M.AddBox(FVector(-4, -19, -30), FVector(2.2f, 1.6f, 42), Leather, FRotator(12, 0, 8)); // qin
	M.AddBox(FVector(4, -19, 14), FVector(1.5f, 4.5f, 1.5f), Trim, FRotator(12, 0, 8));   // dastak
	M.Commit(Pelvis, 0, false);

	// Ko'krak: kaftan + ko'krak zirhi (charm) + oltin hoshiya + yelka bo'laklari
	M.Reset();
	M.AddBox(FVector(0, 0, 23), FVector(13, 19, 23), Kaftan);
	M.AddBox(FVector(10, 0, 24), FVector(4, 13, 18), Leather);
	M.AddBox(FVector(14.2f, 0, 24), FVector(0.6f, 1.6f, 18), Trim);
	M.AddBox(FVector(0, -21, 40), FVector(8, 4, 5), Fur);
	M.AddBox(FVector(0, 21, 40), FVector(8, 4, 5), Fur);
	M.AddBox(FVector(-13.5f, 0, 24), FVector(1.5f, 17, 22), Fur); // orqa mo'yna plash
	M.Commit(Torso, 0, false);

	// Bosh: bo'yin, bosh, soqol, burun, bo'rk (mo'yna halqa + baland tepa)
	M.Reset();
	M.AddCylinder(FVector(0, 0, 0), 5, 5, 7, 8, Skin);
	M.AddSphere(FVector(0, 0, 17), 11, 12, Skin, FVector(1.0f, 0.95f, 1.15f));
	M.AddBox(FVector(8, 0, 9), FVector(4, 7, 6), Beard);
	M.AddBox(FVector(11.5f, 0, 17), FVector(2, 2, 2.5f), Skin);
	M.AddCylinder(FVector(0, 0, 24), 12.5f, 12.5f, 9, 12, Fur);
	M.AddCylinder(FVector(0, 0, 33), 11.5f, 8, 9, 12, ErtCol::Vary(Kaftan, 0.15f, 3));
	M.Commit(Head, 0, false);

	// Qo'llar
	for (int32 Side = 0; Side < 2; ++Side)
	{
		M.Reset();
		M.AddBox(FVector(0, 0, -15), FVector(5.5f, 5.5f, 15), Kaftan);
		M.Commit(Side ? UpperArmR : UpperArmL, 0, false);
		M.Reset();
		M.AddBox(FVector(0, 0, -13), FVector(5, 5, 13), Kaftan);
		M.AddBox(FVector(0, 0, -22), FVector(5.4f, 5.4f, 5), Leather); // bilaguzuk
		M.AddBox(FVector(0.5f, 0, -30), FVector(4, 3.5f, 5), Skin);   // panja
		M.Commit(Side ? LowerArmR : LowerArmL, 0, false);
	}
	// Oyoqlar
	for (int32 Side = 0; Side < 2; ++Side)
	{
		M.Reset();
		M.AddBox(FVector(0, 0, -22), FVector(7, 7, 22), Trousers);
		M.Commit(Side ? ThighR : ThighL, 0, false);
		M.Reset();
		M.AddBox(FVector(0, 0, -20), FVector(6, 6, 20), Trousers);
		M.AddBox(FVector(0, 0, -30), FVector(6.5f, 6.5f, 12), Leather); // etik qo'nji
		M.AddBox(FVector(6, 0, -43), FVector(13, 6, 3.5f), Leather);    // tovon
		M.Commit(Side ? ShinR : ShinL, 0, false);
	}
	Apply(Cur);
	UE_LOG(LogErtugrul, Log, TEXT("HeroBody qurildi (11 bo'g'im)"));
}

void UErtHeroBody::Apply(const FPose& P)
{
	Pelvis->SetRelativeLocation(PelvisBase + FVector(0, 0, P.PelvisZ));
	Torso->SetRelativeRotation(FRotator(P.TorsoPitch, 0, P.TorsoRoll));
	Head->SetRelativeRotation(FRotator(P.HeadPitch, 0, -P.TorsoRoll * 0.5f));
	ThighL->SetRelativeRotation(FRotator(P.ThighL, 0, 0));
	ThighR->SetRelativeRotation(FRotator(P.ThighR, 0, 0));
	ShinL->SetRelativeRotation(FRotator(-P.KneeL, 0, 0));
	ShinR->SetRelativeRotation(FRotator(-P.KneeR, 0, 0));
	UpperArmL->SetRelativeRotation(FRotator(P.ArmL, 0, P.ArmSpread));
	UpperArmR->SetRelativeRotation(FRotator(P.ArmR, 0, -P.ArmSpread));
	LowerArmL->SetRelativeRotation(FRotator(-P.ElbowL, 0, 0));
	LowerArmR->SetRelativeRotation(FRotator(-P.ElbowR, 0, 0));
}

void UErtHeroBody::Animate(float Dt, float Speed, bool bInAir, bool bCrouched, float Lean, float SlopeDeg)
{
	if (!IsBuilt()) return;
	IdleT += Dt;
	const float S = FMath::Clamp(Speed / 330.f, 0.f, 1.9f);
	const float Freq = Speed > 15.f ? (1.3f + Speed / 260.f) : 0.f;
	Phase += Dt * Freq * 2.f * PI;
	if (Phase > 2.f * PI) Phase -= 2.f * PI;

	FPose T;
	const float Sw = FMath::Sin(Phase);
	const float Swing = 27.f * FMath::Min(S, 1.35f);
	if (bInAir)
	{
		T.ThighL = 35.f; T.ThighR = 10.f; T.KneeL = 70.f; T.KneeR = 35.f;
		T.ArmL = 35.f; T.ArmR = 35.f; T.ElbowL = 30.f; T.ElbowR = 30.f; T.ArmSpread = 20.f;
		T.TorsoPitch = -8.f;
	}
	else if (bCrouched)
	{
		T.PelvisZ = -34.f; T.TorsoPitch = -22.f; T.HeadPitch = 14.f;
		T.ThighL = 62.f - Sw * Swing * 0.5f; T.ThighR = 62.f + Sw * Swing * 0.5f;
		T.KneeL = 85.f; T.KneeR = 85.f;
		T.ArmL = -10.f; T.ArmR = -10.f; T.ElbowL = 45.f; T.ElbowR = 45.f;
	}
	else if (S < 0.04f)
	{
		// Tik turish: nafas, engil tebranish
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

	const float A = FMath::Clamp(Dt * 12.f, 0.f, 1.f);
	auto L = [A](float& C, float Tg) { C = FMath::Lerp(C, Tg, A); };
	L(Cur.PelvisZ, T.PelvisZ); L(Cur.TorsoPitch, T.TorsoPitch); L(Cur.TorsoRoll, T.TorsoRoll); L(Cur.HeadPitch, T.HeadPitch);
	L(Cur.ThighL, T.ThighL); L(Cur.ThighR, T.ThighR); L(Cur.KneeL, T.KneeL); L(Cur.KneeR, T.KneeR);
	L(Cur.ArmL, T.ArmL); L(Cur.ArmR, T.ArmR); L(Cur.ElbowL, T.ElbowL); L(Cur.ElbowR, T.ElbowR); L(Cur.ArmSpread, T.ArmSpread);
	Apply(Cur);
}
