#include "ErtHorse.h"
#include "Ertugrul.h"
#include "ErtCharacter.h"
#include "ErtEnemy.h"
#include "ErtWorldBuilder.h"
#include "ErtProcMesh.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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

AErtHorse::AErtHorse()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(58.f, 100.f);
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* CM = GetCharacterMovement();
	CM->bOrientRotationToMovement = false;
	CM->MaxWalkSpeed = 250.f;
	CM->bRunPhysicsWithNoController = true;   // kontrollersiz ham harakat/gravitatsiya
	CM->MaxAcceleration = 700.f;
	CM->BrakingDecelerationWalking = 900.f;
	CM->GravityScale = 1.5f;
	CM->JumpZVelocity = 520.f;
	CM->SetWalkableFloorAngle(46.f);
	CM->MaxStepHeight = 55.f;
	CM->GroundFriction = 6.f;
	if (USkeletalMeshComponent* SK = GetMesh()) SK->SetVisibility(false);
	Saddle = CreateDefaultSubobject<USceneComponent>(TEXT("Saddle"));
	Saddle->SetupAttachment(GetCapsuleComponent());
	Saddle->SetRelativeLocation(FVector(-8.f, 0.f, 66.f));
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void AErtHorse::Init(const FLinearColor& InCoat, bool bInCamel)
{
	Coat = InCoat; bCamel = bInCamel;
	if (bCamel) { WalkSpeed = 200.f; TrotSpeed = 420.f; GallopSpeed = 760.f; MaxHealth = 260.f; Health = 260.f; GetCapsuleComponent()->SetCapsuleSize(62.f, 118.f); Saddle->SetRelativeLocation(FVector(-10.f, 0.f, 90.f)); }
}

void AErtHorse::BeginPlay()
{
	Super::BeginPlay();
	HomePos = GetActorLocation();
	WanderTarget = HomePos;
	{ static const TCHAR* Names[] = { TEXT("Bo'ra"), TEXT("Tulpor"), TEXT("Qorabayir"), TEXT("Chaqmoq"), TEXT("Bo'z"), TEXT("Yulduz"), TEXT("Shamol"), TEXT("Qizil"), TEXT("Oqtosh"), TEXT("Burgut") };
	  const int32 Hn = FMath::Abs((int32)(HomePos.X * 0.013f) + (int32)(HomePos.Y * 0.031f)); HorseName = bCamel ? TEXT("Tuya") : Names[Hn % 10]; }
	WanderT = FMath::FRandRange(2.f, 6.f);
	Build();
}

void AErtHorse::Build()
{
	if (bBuilt) return;
	bBuilt = true;
	if (TryBuildSkeletal()) return;
	if (bCamel)
	{
		// Tuya: uzun oyoqlar, o'rkach, uzun bo'yin, kichik bosh; egar o'rkach ustida
		UMaterialInterface* MatC = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
		auto MakeC = [&](const TCHAR* Name, USceneComponent* Parent, const FVector& Rel)
		{
			UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, MakeUniqueObjectName(this, UProceduralMeshComponent::StaticClass(), Name));
			P->SetupAttachment(Parent); P->SetRelativeLocation(Rel); P->SetCollisionEnabled(ECollisionEnabled::NoCollision); P->RegisterComponent();
			if (MatC) P->SetMaterial(0, MatC); return P;
		};
		const FLinearColor Sand = ErtCol::Sty(Coat, ErtCol::StyleFur), Dark = ErtCol::Sty(Coat * 0.6f, ErtCol::StyleFur), Leather = ErtCol::Sty(FLinearColor(0.30f, 0.18f, 0.09f), ErtCol::StyleLeather), Cloth = ErtCol::Sty(FLinearColor(0.55f, 0.15f, 0.12f), ErtCol::StyleCloth), Tassel(0.85f, 0.7f, 0.2f);
		BodyMesh = MakeC(TEXT("CamelBody"), GetCapsuleComponent(), FVector(0, 0, 45.f));
		FErtMeshData M;
		M.AddBox(FVector(0, 0, 0), FVector(85, 24, 28), Sand);
		M.AddSphere(FVector(-5, 0, 34.f), 26.f, 10, ErtCol::Vary(Sand, 0.05f, 2), FVector(1.1f, 0.9f, 1.f));             // o'rkach
		M.AddBox(FVector(-88, 0, -6), FVector(4, 4, 32), Dark, FRotator(-25, 0, 0));                                    // dum
		M.AddBox(FVector(-5, 0, 52.f), FVector(30, 22, 5), Cloth);                                                        // gilam
		M.AddBox(FVector(-5, 0, 60.f), FVector(22, 16, 6), Leather);                                                      // egar
		for (int32 s = -1; s <= 1; s += 2) for (int32 k = 0; k < 5; ++k) M.AddBox(FVector(-30 + k * 12, s * 25.f, 30.f - k * 0), FVector(2, 1.5f, 8), Tassel);
		M.Commit(BodyMesh, 0, false);
		HeadMesh = MakeC(TEXT("CamelHead"), BodyMesh, FVector(80.f, 0, 10.f));
		M.Reset();
		M.AddBox(FVector(25, 0, 40), FVector(14, 11, 55), Sand, FRotator(-28, 0, 0));                                    // bo'yin (uzun, egilgan)
		M.AddBox(FVector(52, 0, 96), FVector(16, 10, 12), Sand, FRotator(15, 0, 0));                                     // bosh
		M.AddBox(FVector(70, 0, 92), FVector(8, 8, 8), Dark);                                                             // tumshuq
		M.AddBox(FVector(44, -7, 108), FVector(3, 2, 6), Sand, FRotator(0, 0, -25)); M.AddBox(FVector(44, 7, 108), FVector(3, 2, 6), Sand, FRotator(0, 0, 25));
		M.AddBox(FVector(56, 0, 88), FVector(20, 11, 1.5f), Leather);                                                     // yugan
		M.Commit(HeadMesh, 0, false);
		for (int32 i = 0; i < 4; ++i)
		{
			const float X = (i < 2) ? 60.f : -60.f, Y = (i & 1) ? 18.f : -18.f;
			UProceduralMeshComponent* Leg = MakeC(TEXT("CamelLeg"), GetCapsuleComponent(), FVector(X, Y, 18.f));
			M.Reset();
			M.AddBox(FVector(0, 0, -30), FVector(7, 7, 32), ErtCol::Vary(Sand, 0.05f, 10 + i));
			M.AddBox(FVector(0, 0, -95), FVector(5.5f, 5.5f, 36), ErtCol::Vary(Sand * 0.92f, 0.05f, 20 + i));
			M.AddBox(FVector(2, 0, -134), FVector(10, 9, 4), Dark);                                                        // keng tovon
			M.Commit(Leg, 0, false);
			Legs.Add(Leg);
		}
		return;
	}
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	auto Make = [&](const TCHAR* Name, USceneComponent* Parent, const FVector& Rel)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, MakeUniqueObjectName(this, UProceduralMeshComponent::StaticClass(), Name));
		P->SetupAttachment(Parent);
		P->SetRelativeLocation(Rel);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->RegisterComponent();
		if (Mat) P->SetMaterial(0, Mat);
		return P;
	};
	// ---- Realistik ot: ellipsoid tana, egilgan bo'yin, tumshuqli bosh, yol, dum, ikki bo'g'inli oyoqlar, egar-jabduq ----
	FRandomStream RS(GetUniqueID());
	const bool bDarkPoints = RS.FRand() < 0.55f;                          // qora oyoq/tumshuq (bay)
	const bool bBlaze = RS.FRand() < 0.35f;                               // peshonada oq dog'
	const int32 Socks = RS.RandRange(0, 3);                               // oq paypoqlar soni
	const FLinearColor CoatF = ErtCol::Sty(Coat, ErtCol::StyleFur);
	const FLinearColor Dark = ErtCol::Sty(bDarkPoints ? FLinearColor(0.08f, 0.06f, 0.05f) : Coat * 0.6f, ErtCol::StyleFur);
	const FLinearColor Mane = ErtCol::Sty(bDarkPoints ? FLinearColor(0.07f, 0.05f, 0.04f) : Coat * 0.45f, ErtCol::StyleFur);
	const FLinearColor Hoof(0.16f, 0.13f, 0.11f), Leather = ErtCol::Sty(FLinearColor(0.30f, 0.18f, 0.09f), ErtCol::StyleLeather), LeatherD = ErtCol::Sty(FLinearColor(0.20f, 0.12f, 0.06f), ErtCol::StyleLeather), Cloth = ErtCol::Sty(FLinearColor(0.55f, 0.12f, 0.10f), ErtCol::StyleCloth), ClothTrim = ErtCol::Sty(FLinearColor(0.85f, 0.7f, 0.25f), ErtCol::StyleCloth), Iron = ErtCol::Sty(FLinearColor(0.6f, 0.6f, 0.62f), ErtCol::StyleMetal), Eye(0.05f, 0.04f, 0.04f), WhiteM = ErtCol::Sty(FLinearColor(0.92f, 0.9f, 0.86f), ErtCol::StyleFur);
	auto Sph = [&](FErtMeshData& D, const FVector& C, float R, const FLinearColor& Col, const FVector& Sc, int32 Sd) { D.AddSphere(C, R, 12, ErtCol::Vary(Col, 0.04f, Sd), Sc); };
	// Kapsula markazi yerdan 100 sm balandda. Tana markazi yerdan ~130 sm.
	BodyMesh = Make(TEXT("HorseBody"), GetCapsuleComponent(), FVector(0, 0, 25.f));
	FErtMeshData M;
	Sph(M, FVector(0, 0, 2), 30.f, CoatF, FVector(2.6f, 1.0f, 1.05f), 1);                         // tana (bochka)
	Sph(M, FVector(-62, 0, 10), 30.f, CoatF, FVector(1.15f, 0.95f, 1.0f), 2);                     // sag'ri
	Sph(M, FVector(-70, 0, -8), 22.f, CoatF, FVector(0.9f, 1.05f, 1.1f), 3);                      // son
	Sph(M, FVector(66, 0, 6), 28.f, CoatF, FVector(1.05f, 0.95f, 1.05f), 4);                      // ko'krak/yelka
	Sph(M, FVector(40, 0, 32), 18.f, CoatF, FVector(1.4f, 0.8f, 0.6f), 5);                        // yag'rin (withers)
	Sph(M, FVector(74, 0, -6), 16.f, CoatF, FVector(1.1f, 0.9f, 1.0f), 6);                        // ko'krak oldi
	// Egar ostidagi gilam, egar (o'rindiq, oldingi qosh, orqa qosh), ayil, uzangi tasmalari va uzangilar, ko'krak tasmasi
	M.AddBox(FVector(-8, 0, 30), FVector(36, 30, 3), Cloth);
	M.AddBox(FVector(-8, 0, 30.5f), FVector(37, 31, 1.5f), ClothTrim);
	M.AddBox(FVector(-8, 0, 33), FVector(36, 28, 2), Cloth);
	Sph(M, FVector(-8, 0, 36), 14.f, Leather, FVector(2.0f, 1.25f, 0.45f), 7);                  // o'rindiq
	Sph(M, FVector(-34, 0, 44), 8.f, LeatherD, FVector(0.5f, 1.4f, 1.0f), 8);                   // orqa qosh
	Sph(M, FVector(18, 0, 45), 6.f, LeatherD, FVector(0.6f, 1.0f, 1.2f), 9);                    // oldingi qosh
	for (int32 s = -1; s <= 1; s += 2)
	{
		M.AddBox(FVector(-8, s * 27.f, 10), FVector(3, 1.5f, 38), Leather);                     // uzangi tasmasi
		M.AddBox(FVector(-8, s * 28.f, -30), FVector(7, 2.5f, 1.5f), Iron);                     // uzangi (tagi)
		M.AddBox(FVector(-8, s * 28.f, -25), FVector(1.5f, 2.5f, 6), Iron);                     // uzangi (yon)
		M.AddBox(FVector(-8, s * 25.f, 33), FVector(20, 4, 4), LeatherD);                       // egar qanoti
	}
	for (float a = -1.2f; a <= 1.2f; a += 0.3f) M.AddBox(FVector(-4, FMath::Sin(a) * 31.f, 8 + FMath::Cos(a) * 30.f - 30.f), FVector(4, 3, 3), LeatherD);   // ayil
	M.AddBox(FVector(70, 0, 8), FVector(3, 26, 2.5f), LeatherD);                                  // ko'krak tasmasi
	M.Commit(BodyMesh, 0, false);

	// Bo'yin: yag'rindan yuqoriga egilgan; ustida yol tolalari
	NeckMesh = Make(TEXT("HorseNeck"), BodyMesh, FVector(52.f, 0, 22.f));
	M.Reset();
	M.AddCylinder(FVector(0, 0, 0), 20.f, 13.f, 74.f, 12, CoatF, true, FRotator(-52.f, 0, 0));
	Sph(M, FVector(0, 0, 0), 20.f, CoatF, FVector(1.0f, 1.0f, 1.0f), 10);
	for (int32 i = 0; i < 9; ++i)
	{
		const float t = i / 8.f, X = 0.f + t * 46.f - 10.f, Zz = 12.f + t * 58.f;
		M.AddBox(FVector(X - 6, 0, Zz + 6), FVector(4, 1.6f, 9 + RS.FRand() * 3.f), Mane, FRotator(-52.f + RS.FRandRange(-8.f, 8.f), 0, RS.FRandRange(-10.f, 10.f)));
	}
	M.Commit(NeckMesh, 0, false);

	// Bosh: kalla + peshona + tumshuq, burun teshiklari, ko'zlar, quloqlar, yugan, jilov
	HeadMesh = Make(TEXT("HorseHead"), NeckMesh, FVector(44.f, 0, 62.f));
	M.Reset();
	Sph(M, FVector(6, 0, 4), 13.f, CoatF, FVector(1.3f, 1.0f, 1.15f), 11);                        // kalla
	M.AddCylinder(FVector(6, 0, 4), 11.f, 7.5f, 34.f, 10, CoatF, true, FRotator(-100.f, 0, 0));    // yuz (pastga-oldinga)
	Sph(M, FVector(40, 0, -2), 8.f, Dark, FVector(1.2f, 1.0f, 0.9f), 12);                        // tumshuq
	if (bBlaze) M.AddBox(FVector(18, 0, 6), FVector(14, 1.5f, 3), WhiteM, FRotator(-100.f + 90.f, 0, 0));
	for (int32 s = -1; s <= 1; s += 2)
	{
		Sph(M, FVector(43, s * 4.5f, 1), 1.6f, Eye, FVector(1, 1, 1), 13);                       // burun teshigi
		Sph(M, FVector(12, s * 10.5f, 7), 2.6f, Eye, FVector(0.7f, 1, 1), 14);                    // ko'z
		M.AddCylinder(FVector(2, s * 6.f, 14), 3.2f, 0.6f, 12.f, 6, CoatF, true, FRotator(-10.f, 0, s * 22.f));   // quloq
		M.AddBox(FVector(22, s * 8.5f, 3), FVector(1.2f, 1.f, 5.f), Leather, FRotator(-10.f, 0, 0));           // yugan yon tasma
		M.AddBox(FVector(30, s * 6.5f, -3), FVector(1.f, 1.f, 22.f), LeatherD, FRotator(85.f, 0, 0));          // jilov (orqaga)
	}
	M.AddBox(FVector(36, 0, -6), FVector(1.5f, 9, 1.5f), Leather);                               // burun tasmasi
	M.AddBox(FVector(-2, 0, 12), FVector(1.5f, 12, 1.5f), Leather);                               // peshona tasmasi
	M.AddBox(FVector(38, 0, -8), FVector(1, 8, 1), Iron);                                        // suvliq
	M.Commit(HeadMesh, 0, false);

	// Dum: sag'ridan tushuvchi qalin tola dastasi
	TailMesh = Make(TEXT("HorseTail"), BodyMesh, FVector(-92.f, 0, 22.f));
	M.Reset();
	M.AddCylinder(FVector(0, 0, 0), 5.f, 3.f, 22.f, 8, Mane, true, FRotator(-150.f, 0, 0));
	for (int32 i = 0; i < 7; ++i) M.AddBox(FVector(-12 + RS.FRandRange(-3.f, 3.f), RS.FRandRange(-4.f, 4.f), -26.f - i * 2.f), FVector(2.5f, 2.f, 30.f), ErtCol::Vary(Mane, 0.06f, 30 + i), FRotator(RS.FRandRange(-12.f, 4.f), 0, RS.FRandRange(-8.f, 8.f)));
	M.Commit(TailMesh, 0, false);

	// Oyoqlar: yuqori qism (son/yelka) kapsulaga, pastki qism (bilak+tuyoq) yuqori qismga bog'langan (tirsak/hock bo'g'ini)
	for (int32 i = 0; i < 4; ++i)
	{
		const bool bFront = i < 2;
		const float X = bFront ? 56.f : -60.f, Y = (i & 1) ? 19.f : -19.f;
		const bool bSock = i < Socks;
		UProceduralMeshComponent* Leg = Make(TEXT("HorseLeg"), GetCapsuleComponent(), FVector(X, Y, 4.f));
		M.Reset();
		Sph(M, FVector(0, 0, 0), 12.f, CoatF, FVector(1.1f, 0.8f, 1.0f), 40 + i);                                 // bo'g'in (yelka/son)
		M.AddCylinder(FVector(0, 0, 0), 9.f, 6.5f, 44.f, 10, CoatF, true, FRotator(180.f + (bFront ? 6.f : -10.f), 0, 0));   // yuqori suyak
		M.Commit(Leg, 0, false);
		Legs.Add(Leg);
		UProceduralMeshComponent* Low = Make(TEXT("HorseLowerLeg"), Leg, FVector(bFront ? 4.5f : -7.6f, 0, -43.5f));
		M.Reset();
		Sph(M, FVector(0, 0, 0), 7.f, CoatF, FVector(1.2f, 0.9f, 1.0f), 50 + i);                                    // tizza/hock
		M.AddCylinder(FVector(0, 0, 0), 5.5f, 4.2f, 40.f, 8, bSock ? WhiteM : (bDarkPoints ? Dark : ErtCol::Sty(CoatF * 0.92f, ErtCol::StyleFur)), true, FRotator(180.f, 0, 0));   // bilak (cannon)
		Sph(M, FVector(1, 0, -42), 5.2f, bSock ? WhiteM : Dark, FVector(1.1f, 1.0f, 0.8f), 60 + i);                  // to'piq (fetlock)
		M.AddCylinder(FVector(3, 0, -55), 6.5f, 5.f, 9.f, 8, Hoof, true);                                          // tuyoq
		M.Commit(Low, 0, false);
		LowerLegs.Add(Low);
	}
}

void AErtHorse::ApplyDamage(float D)
{
	if (bDead) return;
	Health -= D;
	if (Health > 0.f) return;
	Health = 0.f; bDead = true;
	// Ot yiqiladi: chavandoz uloqtiriladi
	if (AErtCharacter* R = Cast<AErtCharacter>(Rider))
	{
		R->DismountHorse();
		R->LaunchCharacter(GetActorForwardVector() * 450.f + FVector(0, 0, 320.f), true, true);
		R->ReceiveHit(15.f, GetActorLocation(), nullptr, true);
	}
	else if (Rider) { if (AErtEnemy* En = Cast<AErtEnemy>(Rider)) En->ApplyHit(30.f, nullptr, true); }
	Rider = nullptr;
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Skel) { SkelPlay(TEXT("death"), 1.f, TEXT("idle")); if (CurAnim) { Skel->PlayAnimation(CurAnim, false); } }
	if (BodyMesh) { BodyMesh->SetRelativeRotation(FRotator(0, 0, 75.f)); BodyMesh->SetRelativeLocation(FVector(0, 0, -40.f)); }
	for (UProceduralMeshComponent* L : Legs) if (L) { L->SetRelativeRotation(FRotator(0, 0, 75.f)); L->AddRelativeLocation(FVector(0, 0, -60.f)); }
	for (UProceduralMeshComponent* L : LowerLegs) if (L) L->SetRelativeRotation(FRotator(0, 0, 0));
	SetLifeSpan(25.f);
}

void AErtHorse::Mount(AActor* InRider)
{
	if (!InRider || Rider || bDead) return;
	Rider = InRider;
	Input = FVector2D::ZeroVector;
	UE_LOG(LogErtugrul, Log, TEXT("Otga minildi"));
}

void AErtHorse::Dismount()
{
	Rider = nullptr;
	Input = FVector2D::ZeroVector;
	bGallopIn = false;
}

void AErtHorse::SetRiderInput(const FVector2D& In, bool bGallop) { Input = In; bGallopIn = bGallop; }

void AErtHorse::RiderJump()
{
	if (CurSpeed > 150.f) Jump();
}

void AErtHorse::Tick(float Dt)
{
	Super::Tick(Dt);
	if (bDead) return;
	if (!Rider && Health < MaxHealth) Health = FMath::Min(MaxHealth, Health + (3.f + 4.f * Care) * Dt);
	Care = FMath::Max(0.f, Care - Dt / 900.f); CareFxT = FMath::Max(0.f, CareFxT - Dt);
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (Rider)
	{
		// Maqsad tezligi
		float Target = 0.f;
		if (Input.Y > 0.2f) Target = (bGallopIn ? GallopSpeed : (Input.Y > 0.7f ? TrotSpeed : WalkSpeed)) * CareSpeed();
		else if (Input.Y < -0.2f) Target = -140.f;
		const float Rate = (FMath::Abs(Target) > FMath::Abs(CurSpeed)) ? 380.f : 650.f;
		CurSpeed = FMath::FInterpConstantTo(CurSpeed, Target, Dt, Rate);
		// Burilish: sekinda tez, chopishda keng radius
		const float TurnRate = FMath::Lerp(120.f, 55.f, FMath::Clamp(FMath::Abs(CurSpeed) / GallopSpeed, 0.f, 1.f));
		if (FMath::Abs(CurSpeed) > 20.f || FMath::Abs(Input.X) > 0.2f)
			AddActorWorldRotation(FRotator(0, Input.X * TurnRate * Dt * (CurSpeed < 0 ? -1.f : 1.f), 0));
		const bool bSand = AErtWorldBuilder::IsDesert(GetActorLocation().Y / 100.f, GetActorLocation().X / 100.f);
		CM->MaxWalkSpeed = FMath::Max(50.f, FMath::Abs(CurSpeed) * ((bSand && !bCamel) ? 0.75f : 1.f));
		if (FMath::Abs(CurSpeed) > 5.f) AddMovementInput(GetActorForwardVector(), CurSpeed > 0 ? 1.f : -1.f);
	}
	else if (SummonT > 0.f)
	{
		// Hushtak: egasi tomon yo'rtish
		SummonT -= Dt;
		const FVector D = (SummonTo - GetActorLocation()).GetSafeNormal2D();
		const float Dist = FVector::Dist2D(SummonTo, GetActorLocation());
		if (Dist > 260.f && !D.IsNearlyZero())
		{
			CurSpeed = FMath::FInterpConstantTo(CurSpeed, (Dist > 1500.f ? GallopSpeed * 0.8f : TrotSpeed) * CareSpeed(), Dt, 400.f);
			CM->MaxWalkSpeed = FMath::Max(80.f, CurSpeed);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, D.Rotation().Yaw, 0), Dt, 4.f));
			AddMovementInput(GetActorForwardVector(), 1.f);
		}
		else { SummonT = 0.f; HomePos = GetActorLocation(); WanderTarget = HomePos; CurSpeed = 0.f; }
	}
	else
	{
		CurSpeed = FMath::FInterpConstantTo(CurSpeed, 0.f, Dt, 600.f);
		WanderT -= Dt;
		if (WanderT <= 0.f)
		{
			WanderT = FMath::FRandRange(5.f, 12.f);
			const float A = FMath::FRandRange(0.f, 2.f * PI), R = FMath::FRandRange(200.f, 700.f);
			WanderTarget = HomePos + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0);
		}
		const FVector D = (WanderTarget - GetActorLocation()).GetSafeNormal2D();
		if (FVector::Dist2D(WanderTarget, GetActorLocation()) > 120.f && !D.IsNearlyZero())
		{
			CM->MaxWalkSpeed = 110.f;
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, D.Rotation().Yaw, 0), Dt, 2.f));
			AddMovementInput(GetActorForwardVector(), 1.f);
		}
	}
	Animate(Dt);
}

void AErtHorse::Animate(float Dt)
{
	if (Skel)
	{
		const float Sp2 = GetCharacterMovement()->Velocity.Size2D();
		if (GetCharacterMovement()->IsFalling()) SkelPlay(TEXT("jump"), 1.f, TEXT("gallop"));
		else if (Sp2 < 15.f) SkelPlay(TEXT("idle"), 1.f);
		else if (Sp2 < WalkRef * 1.4f) SkelPlay(TEXT("walk"), FMath::Clamp(Sp2 / WalkRef, 0.5f, 1.5f), TEXT("idle"));
		else if (Sp2 < TrotRef * 1.4f) SkelPlay(TEXT("trot"), FMath::Clamp(Sp2 / TrotRef, 0.6f, 1.5f), TEXT("walk"));
		else SkelPlay(TEXT("gallop"), FMath::Clamp(Sp2 / GallopRef, 0.6f, 1.4f), TEXT("trot"));
		return;
	}
	const float Sp = GetCharacterMovement()->Velocity.Size2D();
	const float Stride = bCamel ? 340.f : (Sp > 600.f ? 300.f : 190.f);
	if (Sp > 10.f) Phase += Dt * (Sp / Stride) * 2.f * PI;
	const float Amp = FMath::Clamp(Sp / 500.f, 0.f, 1.3f) * 28.f;
	const bool bInAir = GetCharacterMovement()->IsFalling();
	const float T = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < Legs.Num(); ++i)
	{
		// Yo'rtish: diagonal juftlar (FL+BR, FR+BL); chopishda oldingi/orqa juftlar
		float Ph = Phase;
		if (bCamel) Ph += ((i & 1) ? 0.f : PI);   // tuya: bir tomon oyoqlari birga (yo'rg'a)
		else if (Sp > 600.f) Ph += (i < 2) ? 0.f : PI * 0.6f; else Ph += ((i == 0 || i == 3) ? 0.f : PI);
		float Pitch = FMath::Sin(Ph) * Amp;
		if (bInAir) Pitch = (i < 2) ? -35.f : 30.f;
		Legs[i]->SetRelativeRotation(FRotator(Pitch, 0, 0));
		if (LowerLegs.IsValidIndex(i) && LowerLegs[i])
		{
			// Pastki bo'g'in: oyoq oldinga uchganda bukiladi (oldingi oyoq orqaga, orqa oyoq oldinga buklanadi)
			const float Bend = FMath::Max(0.f, FMath::Sin(Ph + PI * 0.5f)) * Amp * 1.6f;
			float LP = (i < 2) ? -Bend : Bend * 0.8f;
			if (bInAir) LP = (i < 2) ? -50.f : 25.f;
			else if (Sp < 10.f) LP = 0.f;
			LowerLegs[i]->SetRelativeRotation(FRotator(LP, 0, 0));
		}
	}
	HeadBob = FMath::Sin(Phase) * FMath::Min(Sp / 300.f, 1.f) * 6.f;
	if (NeckMesh)
	{
		// Chopishda bo'yin cho'ziladi, turganda bosh biroz pastga (o'tlash)
		const float Stretch = FMath::Clamp((Sp - 300.f) / 600.f, 0.f, 1.f) * 22.f;
		const float Graze = (Sp < 5.f && !Rider) ? (FMath::Sin(T * 0.35f) > 0.3f ? 38.f : 0.f) : 0.f;
		NeckMesh->SetRelativeRotation(FMath::RInterpTo(NeckMesh->GetRelativeRotation(), FRotator(HeadBob * 0.6f + Stretch + Graze, 0, 0), Dt, 4.f));
	}
	if (HeadMesh) HeadMesh->SetRelativeRotation(FRotator(HeadBob - (Sp > 600.f ? 8.f : 0.f) + FMath::Sin(T * 1.7f) * 2.f, FMath::Sin(T * 0.9f) * 3.f, 0));
	if (TailMesh) TailMesh->SetRelativeRotation(FRotator(FMath::Clamp(Sp / 900.f, 0.f, 1.f) * 25.f, FMath::Sin(T * 2.3f) * 12.f + FMath::Sin(Phase) * 4.f, 0));
	if (BodyMesh) BodyMesh->SetRelativeLocation(FVector(0, 0, (bCamel ? 45.f : 25.f) + FMath::Abs(FMath::Sin(Phase)) * FMath::Min(Sp / 400.f, 1.f) * (bCamel ? 8.f : 5.f)));
	if (BodyMesh && !bCamel && !bDead) BodyMesh->SetRelativeRotation(FRotator(FMath::Sin(Phase) * FMath::Min(Sp / 700.f, 1.f) * 3.f, 0, 0));
}


// ---------------- Skeletli ot (character.json "horse"/"camel") ----------------

bool AErtHorse::TryBuildSkeletal()
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/character.json")))) return false;
	TSharedPtr<FJsonObject> Cfg;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Cfg) || !Cfg.IsValid()) return false;
	const TSharedPtr<FJsonObject>* Prof = nullptr;
	if (!Cfg->TryGetObjectField(bCamel ? TEXT("camel") : TEXT("horse"), Prof) || !Prof->IsValid()) return false;
	FString MeshPath; if (!(*Prof)->TryGetStringField(TEXT("mesh"), MeshPath) || MeshPath.IsEmpty()) return false;
	auto ObjPath = [](FString P) { if (!P.Contains(TEXT("."))) { const FString Nm = FPaths::GetBaseFilename(P); P = P + TEXT(".") + Nm; } return P; };
	USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, *ObjPath(MeshPath));
	if (!SM) { UE_LOG(LogErtugrul, Warning, TEXT("character.json: ot meshi topilmadi %s"), *MeshPath); return false; }
	Skel = NewObject<USkeletalMeshComponent>(this, TEXT("SkelHorse"));
	Skel->SetupAttachment(GetCapsuleComponent());
	const double Yaw = (*Prof)->HasField(TEXT("yaw")) ? (*Prof)->GetNumberField(TEXT("yaw")) : -90.0;
	const double Zo = (*Prof)->HasField(TEXT("z")) ? (*Prof)->GetNumberField(TEXT("z")) : 0.0;
	const double Sc = (*Prof)->HasField(TEXT("scale")) ? (*Prof)->GetNumberField(TEXT("scale")) : 1.0;
	Skel->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + Zo));
	Skel->SetRelativeRotation(FRotator(0, Yaw, 0));
	Skel->SetRelativeScale3D(FVector(Sc));
	SkelMeshRef = SM;
	Skel->SetSkeletalMesh(SM);
	Skel->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skel->RegisterComponent();
	if ((*Prof)->HasField(TEXT("walk_ref"))) WalkRef = (*Prof)->GetNumberField(TEXT("walk_ref"));
	if ((*Prof)->HasField(TEXT("trot_ref"))) TrotRef = (*Prof)->GetNumberField(TEXT("trot_ref"));
	if ((*Prof)->HasField(TEXT("gallop_ref"))) GallopRef = (*Prof)->GetNumberField(TEXT("gallop_ref"));
	// Egar joyi: "saddle": [x, y, z] (mesh koordinatasida, sm) yoki "saddle_socket"
	const TArray<TSharedPtr<FJsonValue>>* Sd = nullptr;
	if ((*Prof)->TryGetArrayField(TEXT("saddle"), Sd) && Sd->Num() == 3) Saddle->SetRelativeLocation(FVector((*Sd)[0]->AsNumber(), (*Sd)[1]->AsNumber(), (*Sd)[2]->AsNumber()));
	FString SaddleSock;
	if ((*Prof)->TryGetStringField(TEXT("saddle_socket"), SaddleSock) && !SaddleSock.IsEmpty()) { Saddle->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(*SaddleSock)); }
	const TSharedPtr<FJsonObject>* Anims = nullptr;
	if ((*Prof)->TryGetObjectField(TEXT("anims"), Anims) && Anims->IsValid())
	{
		for (const auto& Pair : (*Anims)->Values)
		{
			TArray<TObjectPtr<UAnimSequence>> List;
			auto Add = [&](const FString& P) { if (UAnimSequence* A = LoadObject<UAnimSequence>(nullptr, *ObjPath(P))) { if (!A->IsValidAdditive()) { List.Add(A); SkelAnimRefs.Add(A); } } };
			if (Pair.Value->Type == EJson::String) Add(Pair.Value->AsString());
			else if (Pair.Value->Type == EJson::Array) for (const TSharedPtr<FJsonValue>& V : Pair.Value->AsArray()) Add(V->AsString());
			if (List.Num()) SkelAnims.Add(FString(Pair.Key.ToView()), List);
		}
	}
	SkelPlay(TEXT("idle"), 1.f);
	UE_LOG(LogErtugrul, Log, TEXT("Skeletli %s: %s, %d animatsiya turi"), bCamel ? TEXT("tuya") : TEXT("ot"), *SM->GetName(), SkelAnims.Num());
	return true;
}

void AErtHorse::SkelPlay(const FString& Key, float Rate, const TCHAR* Fallback)
{
	if (!Skel || !IsValid(Skel)) return;
	const TArray<TObjectPtr<UAnimSequence>>* L = SkelAnims.Find(Key);
	if ((!L || !L->Num()) && Fallback) L = SkelAnims.Find(Fallback);
	if (!L || !L->Num()) L = SkelAnims.Find(TEXT("idle"));
	if (!L || !L->Num()) return;
	UAnimSequence* A = (*L)[0];
	if (A != CurAnim || !Skel->IsPlaying()) { CurAnim = A; Skel->PlayAnimation(A, Key != TEXT("death")); }
	Skel->SetPlayRate(Rate);
}
