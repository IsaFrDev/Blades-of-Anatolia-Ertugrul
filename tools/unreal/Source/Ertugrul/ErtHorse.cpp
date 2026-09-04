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
	Saddle->SetRelativeLocation(FVector(-5.f, 0.f, 62.f));
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
	WanderT = FMath::FRandRange(2.f, 6.f);
	Build();
}

void AErtHorse::Build()
{
	if (bBuilt) return;
	bBuilt = true;
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
		const FLinearColor Sand = Coat, Dark = Coat * 0.6f, Leather(0.30f, 0.18f, 0.09f), Cloth(0.55f, 0.15f, 0.12f), Tassel(0.85f, 0.7f, 0.2f);
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
	const FLinearColor Dark = Coat * 0.55f, Mane(0.12f, 0.09f, 0.06f), Leather(0.30f, 0.18f, 0.09f), Cloth(0.55f, 0.12f, 0.10f);
	// Kapsula markazi yerdan 100 sm balandda. Tana markazi yerdan 125 sm.
	BodyMesh = Make(TEXT("HorseBody"), GetCapsuleComponent(), FVector(0, 0, 25.f));
	FErtMeshData M;
	M.AddBox(FVector(0, 0, 0), FVector(80, 26, 30), Coat);
	M.AddBox(FVector(-70, 0, 6), FVector(22, 24, 26), ErtCol::Vary(Coat, 0.05f, 2));         // sag'ri
	M.AddBox(FVector(78, 0, 8), FVector(18, 22, 26), ErtCol::Vary(Coat, 0.05f, 3));          // ko'krak
	M.AddBox(FVector(-92, 0, -8), FVector(6, 5, 45), Mane, FRotator(-20, 0, 0));             // dum
	M.AddBox(FVector(-12, 0, 24), FVector(34, 22, 5), Cloth);                                 // egar ostidagi mato
	M.AddBox(FVector(-8, 0, 33), FVector(28, 18, 6), Leather);                                // egar
	M.AddBox(FVector(-34, 0, 44), FVector(5, 12, 8), Leather);                                // egar orqasi
	M.AddBox(FVector(16, 0, 42), FVector(4, 8, 6), Leather);                                  // egar oldi
	M.AddBox(FVector(-6, -26, 18), FVector(3, 2, 40), Leather);                               // uzangi tasmasi
	M.AddBox(FVector(-6, 26, 18), FVector(3, 2, 40), Leather);
	M.AddBox(FVector(-6, -27, -22), FVector(8, 3, 3), FLinearColor(0.6f, 0.6f, 0.62f));      // uzangi
	M.AddBox(FVector(-6, 27, -22), FVector(8, 3, 3), FLinearColor(0.6f, 0.6f, 0.62f));
	M.Commit(BodyMesh, 0, false);

	HeadMesh = Make(TEXT("HorseHead"), BodyMesh, FVector(88.f, 0, 22.f));
	M.Reset();
	M.AddBox(FVector(18, 0, 30), FVector(20, 12, 36), Coat, FRotator(-35, 0, 0));           // bo'yin
	M.AddBox(FVector(10, 0, 44), FVector(16, 3, 30), Mane, FRotator(-35, 0, 0));            // yol
	M.AddBox(FVector(48, 0, 58), FVector(28, 10, 12), ErtCol::Vary(Coat, 0.06f, 4), FRotator(-8, 0, 0)); // bosh
	M.AddBox(FVector(74, 0, 52), FVector(6, 8, 7), Dark);                                    // tumshuq
	M.AddBox(FVector(34, -8, 72), FVector(3, 2, 7), Coat, FRotator(0, 0, -20));             // quloqlar
	M.AddBox(FVector(34, 8, 72), FVector(3, 2, 7), Coat, FRotator(0, 0, 20));
	M.AddBox(FVector(50, 0, 50), FVector(24, 11, 2), Leather);                               // yugan
	M.AddBox(FVector(30, 0, 40), FVector(30, 0.8f, 0.8f), Leather, FRotator(30, 0, 0));      // jilov
	M.Commit(HeadMesh, 0, false);

	for (int32 i = 0; i < 4; ++i)
	{
		const float X = (i < 2) ? 58.f : -58.f, Y = (i & 1) ? 19.f : -19.f;
		UProceduralMeshComponent* Leg = Make(TEXT("HorseLeg"), GetCapsuleComponent(), FVector(X, Y, -2.f));
		M.Reset();
		M.AddBox(FVector(0, 0, -22), FVector(8, 8, 24), ErtCol::Vary(Coat, 0.05f, 10 + i));
		M.AddBox(FVector(0, 0, -68), FVector(6, 6, 26), ErtCol::Vary(Coat * 0.9f, 0.05f, 20 + i));
		M.AddBox(FVector(2, 0, -96), FVector(8, 7, 5), Dark);                                 // tuyoq
		M.Commit(Leg, 0, false);
		Legs.Add(Leg);
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
	if (BodyMesh) { BodyMesh->SetRelativeRotation(FRotator(0, 0, 75.f)); BodyMesh->SetRelativeLocation(FVector(0, 0, -40.f)); }
	for (UProceduralMeshComponent* L : Legs) if (L) { L->SetRelativeRotation(FRotator(0, 0, 75.f)); L->AddRelativeLocation(FVector(0, 0, -60.f)); }
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
	if (!Rider && Health < MaxHealth) Health = FMath::Min(MaxHealth, Health + 3.f * Dt);
	UCharacterMovementComponent* CM = GetCharacterMovement();
	if (Rider)
	{
		// Maqsad tezligi
		float Target = 0.f;
		if (Input.Y > 0.2f) Target = bGallopIn ? GallopSpeed : (Input.Y > 0.7f ? TrotSpeed : WalkSpeed);
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
	const float Sp = GetCharacterMovement()->Velocity.Size2D();
	const float Stride = bCamel ? 340.f : (Sp > 600.f ? 300.f : 190.f);
	if (Sp > 10.f) Phase += Dt * (Sp / Stride) * 2.f * PI;
	const float Amp = FMath::Clamp(Sp / 500.f, 0.f, 1.3f) * 28.f;
	const bool bInAir = GetCharacterMovement()->IsFalling();
	for (int32 i = 0; i < Legs.Num(); ++i)
	{
		// Yo'rtish: diagonal juftlar (FL+BR, FR+BL); chopishda oldingi/orqa juftlar
		float Ph = Phase;
		if (bCamel) Ph += ((i & 1) ? 0.f : PI);   // tuya: bir tomon oyoqlari birga (yo'rg'a)
		else if (Sp > 600.f) Ph += (i < 2) ? 0.f : PI * 0.6f; else Ph += ((i == 0 || i == 3) ? 0.f : PI);
		float Pitch = FMath::Sin(Ph) * Amp;
		if (bInAir) Pitch = (i < 2) ? -35.f : 30.f;
		Legs[i]->SetRelativeRotation(FRotator(Pitch, 0, 0));
	}
	HeadBob = FMath::Sin(Phase) * FMath::Min(Sp / 300.f, 1.f) * 6.f;
	if (HeadMesh) HeadMesh->SetRelativeRotation(FRotator(HeadBob - (Sp > 600.f ? 10.f : 0.f), 0, 0));
	if (BodyMesh) BodyMesh->SetRelativeLocation(FVector(0, 0, (bCamel ? 45.f : 25.f) + FMath::Abs(FMath::Sin(Phase)) * FMath::Min(Sp / 400.f, 1.f) * (bCamel ? 8.f : 5.f)));
	// Chavandozni egarda ushlab turamiz (pozitsiya egar bilan birga yuradi - attach)
}
