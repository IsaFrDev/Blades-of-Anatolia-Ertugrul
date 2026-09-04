#include "ErtBoat.h"
#include "Ertugrul.h"
#include "ErtProcMesh.h"
#include "ErtCharacter.h"
#include "ErtWorldBuilder.h"
#include "ErtAudio.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AErtBoat::AErtBoat()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Seat = CreateDefaultSubobject<USceneComponent>(TEXT("Seat"));
	Seat->SetupAttachment(RootComponent);
	Seat->SetRelativeLocation(FVector(-20.f, 0, 30.f));
}

void AErtBoat::BeginPlay()
{
	Super::BeginPlay();
	World = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(this, AErtWorldBuilder::StaticClass()));
	Yaw = GetActorRotation().Yaw;
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	auto Make = [&](const TCHAR* Name, const FVector& Rel, bool bCollide)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, Name);
		P->SetupAttachment(RootComponent); P->SetRelativeLocation(Rel);
		P->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		if (bCollide) { P->SetCollisionObjectType(ECC_WorldDynamic); P->SetCollisionResponseToAllChannels(ECR_Block); P->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); }
		P->RegisterComponent(); if (Mat) P->SetMaterial(0, Mat); return P;
	};
	const FLinearColor Wood(0.42f, 0.28f, 0.14f), Dark(0.28f, 0.18f, 0.09f), Rope(0.7f, 0.6f, 0.4f);
	Hull = Make(TEXT("Hull"), FVector::ZeroVector, true);
	FErtMeshData M;
	// Tub, bortlar (qiya), tumshuq va orqa
	M.AddBox(FVector(0, 0, 0), FVector(180, 42, 5), Dark);
	M.AddBox(FVector(0, -48, 22), FVector(170, 6, 24), Wood, FRotator(0, 0, -14.f));
	M.AddBox(FVector(0, 48, 22), FVector(170, 6, 24), Wood, FRotator(0, 0, 14.f));
	M.AddBox(FVector(192, 0, 24), FVector(24, 12, 26), ErtCol::Vary(Wood, 0.06f, 3), FRotator(0, 0, 0));
	M.AddBox(FVector(-182, 0, 22), FVector(8, 40, 24), ErtCol::Vary(Wood, 0.06f, 4));
	for (int32 i = -1; i <= 1; ++i) M.AddBox(FVector(i * 90.f, 0, 26), FVector(10, 44, 4), ErtCol::Vary(Wood, 0.08f, 5 + i)); // o'rindiqlar
	M.AddBox(FVector(60, 0, 40), FVector(4, 4, 4), Rope);
	M.Commit(Hull, 0, true);
	OarL = Make(TEXT("OarL"), FVector(20, -48, 40), false);
	OarR = Make(TEXT("OarR"), FVector(20, 48, 40), false);
	M.Reset(); M.AddBox(FVector(0, -70, -20), FVector(3, 70, 3), Wood, FRotator(0, 0, -20.f)); M.AddBox(FVector(0, -135, -44), FVector(6, 16, 2), Dark, FRotator(0, 0, -20.f)); M.Commit(OarL, 0, false);
	M.Reset(); M.AddBox(FVector(0, 70, -20), FVector(3, 70, 3), Wood, FRotator(0, 0, 20.f)); M.AddBox(FVector(0, 135, -44), FVector(6, 16, 2), Dark, FRotator(0, 0, 20.f)); M.Commit(OarR, 0, false);
}

bool AErtBoat::WaterAt(const FVector& P, float& SurfZ) const
{
	if (!World) { SurfZ = 0.f; return false; }
	return World->IsWater(P.Y / 100.f, P.X / 100.f, SurfZ);
}

void AErtBoat::Board(AErtCharacter* H)
{
	if (!H || Rider) return;
	Rider = H;
	H->GetCharacterMovement()->StopMovementImmediately();
	H->GetCharacterMovement()->SetMovementMode(MOVE_None);
	H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	H->AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	H->SetActorRelativeLocation(FVector(0, 0, 60.f));
	H->SetActorRelativeRotation(FRotator::ZeroRotator);
	UE_LOG(LogErtugrul, Log, TEXT("Qayiqqa o'tirildi"));
}

void AErtBoat::Leave()
{
	if (!Rider) return;
	AErtCharacter* H = Rider; Rider = nullptr; Input = FVector2D::ZeroVector;
	H->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	// Eng yaqin qirg'oq: 8 yo'nalishda 3-30 m
	FVector Best = GetActorLocation() + GetActorRightVector() * 250.f; float BestD = 1e9f; bool bFound = false;
	for (int32 a = 0; a < 16 && !bFound; ++a)
		for (float R = 300.f; R <= 3000.f; R += 150.f)
		{
			const float Ang = a * PI / 8.f;
			const FVector P = GetActorLocation() + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 0);
			float S; if (WaterAt(P, S)) continue;
			if (R < BestD) { BestD = R; Best = P; bFound = true; }
			break;
		}
	FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtShore), true, this); Q.AddIgnoredActor(H);
	float Z = GetActorLocation().Z + 200.f;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Best + FVector(0, 0, 5000.f), Best - FVector(0, 0, 5000.f), ECC_Visibility, Q)) Z = Hit.ImpactPoint.Z;
	H->SetActorLocation(FVector(Best.X, Best.Y, Z + 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	H->SetActorRotation(FRotator(0, GetActorRotation().Yaw, 0));
	H->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	H->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void AErtBoat::Tick(float Dt)
{
	Super::Tick(Dt);
	T += Dt;
	// Tezlik va burilish
	const float Target = Input.Y > 0.2f ? 380.f : (Input.Y < -0.2f ? -120.f : 0.f);
	Speed = FMath::FInterpConstantTo(Speed, Target, Dt, Target != 0.f ? 160.f : 220.f);
	if (FMath::Abs(Speed) > 10.f || FMath::Abs(Input.X) > 0.2f) Yaw += Input.X * 60.f * Dt * (Speed < 0 ? -1.f : 1.f);
	const FVector Fwd = FRotator(0, Yaw, 0).Vector();
	FVector Pos = GetActorLocation();
	const FVector Next = Pos + Fwd * Speed * Dt;
	float Surf = 0.f;
	if (WaterAt(Next + Fwd * 220.f, Surf) && WaterAt(Next, Surf)) Pos = Next;
	else { if (FMath::Abs(Speed) > 60.f) FErtAudio::PlaySfx(GetWorld(), TEXT("arrow_wall"), Pos, 0.5f, 0.6f); Speed *= 0.2f; }
	if (!WaterAt(Pos, Surf)) WaterAt(GetActorLocation(), Surf);
	// Suzuvchanlik va to'lqin
	const float Bob = FMath::Sin(T * 1.6f) * 3.f + FMath::Sin(T * 2.7f + 1.f) * 2.f;
	Pos.Z = Surf * 100.f + 6.f + Bob;
	SetActorLocation(Pos);
	SetActorRotation(FRotator(FMath::Sin(T * 1.3f) * 1.5f - Speed * 0.004f, Yaw, FMath::Sin(T * 1.9f) * 2.f + Input.X * 3.f));
	// Eshkaklar
	if (FMath::Abs(Speed) > 20.f) RowPhase += Dt * 2.2f * FMath::Sign(Speed);
	const float Sw = FMath::Sin(RowPhase), Lift = FMath::Max(0.f, FMath::Cos(RowPhase)) * 18.f;
	if (OarL) OarL->SetRelativeRotation(FRotator(0, -Sw * 28.f, -Lift));
	if (OarR) OarR->SetRelativeRotation(FRotator(0, Sw * 28.f, Lift));
	if (Rider && Rider->IsDead()) Leave();
}
