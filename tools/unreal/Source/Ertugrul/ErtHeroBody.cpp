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
	// Tos + kamar + (qilich belda, agar qo'lda bo'lmasa)
	M.AddBox(FVector(0, 0, 4), FVector(12, 16, 9), Kaftan);
	M.AddBox(FVector(0, 0, 10), FVector(12.6f, 16.6f, 2.5f), Leather);
	M.AddBox(FVector(12.8f, 0, 10), FVector(1.5f, 3, 3), Trim);
	M.AddBox(FVector(-4, -19, -30), FVector(2.2f, 1.6f, 42), Leather, FRotator(12, 0, 8));
	if (!bSwordInHand) M.AddBox(FVector(4, -19, 14), FVector(1.5f, 4.5f, 1.5f), Trim, FRotator(12, 0, 8));
	M.Commit(Pelvis, 0, false);

	// Ko'krak: kaftan + ko'krak zirhi + oltin hoshiya + yelka bo'laklari + orqa plash
	M.Reset();
	M.AddBox(FVector(0, 0, 23), FVector(13, 19, 23), Kaftan);
	M.AddBox(FVector(10, 0, 24), FVector(4, 13, 18), Leather);
	M.AddBox(FVector(14.2f, 0, 24), FVector(0.6f, 1.6f, 18), Trim);
	M.AddBox(FVector(0, -21, 40), FVector(8, 4, 5), Fur);
	M.AddBox(FVector(0, 21, 40), FVector(8, 4, 5), Fur);
	M.AddBox(FVector(-13.5f, 0, 24), FVector(1.5f, 17, 22), Fur);
	M.Commit(Torso, 0, false);

	// Bosh: bo'yin, bosh, soqol, burun, bo'rk yoki dubulg'a
	M.Reset();
	M.AddCylinder(FVector(0, 0, 0), 5, 5, 7, 8, Skin);
	M.AddSphere(FVector(0, 0, 17), 11, 12, Skin, FVector(1.0f, 0.95f, 1.15f));
	M.AddBox(FVector(8, 0, 9), FVector(4, 7, 6), Beard);
	M.AddBox(FVector(11.5f, 0, 17), FVector(2, 2, 2.5f), Skin);
	if (bHelmet)
	{
		M.AddSphere(FVector(0, 0, 20), 12.5f, 12, Steel, FVector(1, 1, 0.9f));
		M.AddCone(FVector(0, 0, 30), 3.f, 8.f, 6, Steel);
		M.AddBox(FVector(11.8f, 0, 14), FVector(1.5f, 1.5f, 6), Steel); // burun himoyasi
	}
	else
	{
		M.AddCylinder(FVector(0, 0, 24), 12.5f, 12.5f, 9, 12, Fur);
		M.AddCylinder(FVector(0, 0, 33), 11.5f, 8, 9, 12, ErtCol::Vary(Kaftan, 0.15f, 3));
	}
	M.Commit(Head, 0, false);

	// Qo'llar
	for (int32 Side = 0; Side < 2; ++Side)
	{
		M.Reset();
		M.AddBox(FVector(0, 0, -15), FVector(5.5f, 5.5f, 15), Kaftan);
		M.Commit(Side ? UpperArmR : UpperArmL, 0, false);
		M.Reset();
		M.AddBox(FVector(0, 0, -13), FVector(5, 5, 13), Kaftan);
		M.AddBox(FVector(0, 0, -22), FVector(5.4f, 5.4f, 5), Leather);
		M.AddBox(FVector(0.5f, 0, -30), FVector(4, 3.5f, 5), Skin);
		if (Side == 1 && bSwordInHand)
		{
			M.AddBox(FVector(0.5f, 0, -30), FVector(1.2f, 6, 1.2f), Trim);               // qo'riqlovchi
			M.AddBox(FVector(42, 0, -30), FVector(40, 0.6f, 2.8f), Steel);               // tig'
		}
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
		M.AddBox(FVector(0, 0, -30), FVector(6.5f, 6.5f, 12), Leather);
		M.AddBox(FVector(6, 0, -43), FVector(13, 6, 3.5f), Leather);
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
	ThighL->SetRelativeRotation(FRotator(P.ThighL, 0, 0));
	ThighR->SetRelativeRotation(FRotator(P.ThighR, 0, 0));
	// Pitch < 0 = oldinga (pivot ostidagi bo'lak uchun). Tizza faqat orqaga bukiladi.
	ShinL->SetRelativeRotation(FRotator(P.KneeL, 0, 0));
	ShinR->SetRelativeRotation(FRotator(P.KneeR, 0, 0));
	UpperArmL->SetRelativeRotation(FRotator(P.ArmL, 0, P.ArmSpread));
	UpperArmR->SetRelativeRotation(FRotator(P.ArmR, 0, -P.ArmSpread));
	LowerArmL->SetRelativeRotation(FRotator(-P.ElbowL, 0, 0));
	LowerArmR->SetRelativeRotation(FRotator(-P.ElbowR, 0, 0));
}

void UErtHeroBody::TriggerAttack() { AttackT = 1.f; }
void UErtHeroBody::TriggerHurt() { HurtT = 1.f; }

void UErtHeroBody::SetDead(float HalfH)
{
	if (!IsBuilt() || bDead) return;
	bDead = true;
	Root->SetRelativeRotation(FRotator(-82.f, 0, 12.f));
	Root->SetRelativeLocation(FVector(30.f, 0, -HalfH + 14.f - 89.f));
	FPose P; P.ArmL = -40.f; P.ArmR = 30.f; P.ArmSpread = 25.f; P.KneeL = 20.f; P.ThighR = 15.f;
	Cur = P;
	Apply(Cur);
}

void UErtHeroBody::Animate(float Dt, float Speed, bool bInAir, bool bCrouched, float Lean, float SlopeDeg)
{
	if (!IsBuilt() || bDead) return;
	IdleT += Dt;
	AttackT = FMath::Max(0.f, AttackT - Dt / 0.45f);
	HurtT = FMath::Max(0.f, HurtT - Dt / 0.35f);
	const float S = FMath::Clamp(Speed / 330.f, 0.f, 1.9f);
	const float Freq = Speed > 15.f ? (1.3f + Speed / 260.f) : 0.f;
	Phase += Dt * Freq * 2.f * PI;
	if (Phase > 2.f * PI) Phase -= 2.f * PI;

	FPose T;
	const float Sw = FMath::Sin(Phase);
	const float Swing = 27.f * FMath::Min(S, 1.35f);
	if (bSwim)
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
	if (bSwordInHand && AttackT <= 0.f && !bInAir)
	{
		// Qilich tayyor holat: o'ng qo'l oldinda, tirsak bukilgan
		T.ArmR = -15.f; T.ElbowR = 70.f;
	}
	if (AttackT > 0.f)
	{
		// Zarba: qo'l orqadan-yuqoridan oldinga siltanadi, tana buriladi
		const float K = 1.f - AttackT;               // 0 -> 1
		const float Sweep = FMath::Sin(K * PI * 0.85f);
		T.ArmR = FMath::Lerp(115.f, -75.f, FMath::Clamp(K * 1.4f, 0.f, 1.f));
		T.ElbowR = 15.f + 30.f * (1.f - Sweep);
		T.ArmSpread = 12.f;
		T.TorsoYaw = FMath::Lerp(-28.f, 32.f, FMath::Clamp(K * 1.3f, 0.f, 1.f));
		T.TorsoPitch -= 10.f * Sweep;
	}
	if (HurtT > 0.f) { T.TorsoPitch += 18.f * HurtT; T.HeadPitch -= 10.f * HurtT; }

	const float A = FMath::Clamp(Dt * (AttackT > 0.f ? 22.f : 12.f), 0.f, 1.f);
	auto L = [A](float& C, float Tg) { C = FMath::Lerp(C, Tg, A); };
	L(Cur.PelvisZ, T.PelvisZ); L(Cur.PelvisPitch, T.PelvisPitch); L(Cur.TorsoPitch, T.TorsoPitch); L(Cur.TorsoRoll, T.TorsoRoll); L(Cur.TorsoYaw, T.TorsoYaw); L(Cur.HeadPitch, T.HeadPitch);
	L(Cur.ThighL, T.ThighL); L(Cur.ThighR, T.ThighR); L(Cur.KneeL, T.KneeL); L(Cur.KneeR, T.KneeR);
	L(Cur.ArmL, T.ArmL); L(Cur.ArmR, T.ArmR); L(Cur.ElbowL, T.ElbowL); L(Cur.ElbowR, T.ElbowR); L(Cur.ArmSpread, T.ArmSpread);
	Apply(Cur);
}
