#include "ErtLoot.h"
#include "ErtProcMesh.h"
#include "ErtCharacter.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"

AErtLoot::AErtLoot()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AErtLoot::BeginPlay()
{
	Super::BeginPlay();
	Mesh = NewObject<UProceduralMeshComponent>(this, TEXT("LootMesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->RegisterComponent();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"))) Mesh->SetMaterial(0, M);
	FErtMeshData D;
	const FLinearColor Cloth = bBoss ? FLinearColor(0.45f, 0.08f, 0.08f) : FLinearColor(0.42f, 0.30f, 0.16f);
	D.AddSphere(FVector(0, 0, 14.f), bBoss ? 20.f : 14.f, 8, Cloth, FVector(1.f, 1.f, 0.8f), 0.1f, 5);    // to'rva
	D.AddCylinder(FVector(0, 0, 24.f), 4.f, 3.f, 6.f, 6, FLinearColor(0.3f, 0.2f, 0.1f));                    // bog'ich
	if (Gold > 0) D.AddSphere(FVector(9.f, 0, 8.f), 5.f, 6, FLinearColor(1.f, 0.85f, 0.25f));                // tanga
	if (Arrows > 0) D.AddBox(FVector(-8.f, 6.f, 30.f), FVector(1.f, 1.f, 22.f), FLinearColor(0.45f, 0.32f, 0.15f), FRotator(0, 0, 20.f));
	if (Potions > 0) D.AddCylinder(FVector(-6.f, -10.f, 0.f), 4.f, 3.f, 14.f, 6, FLinearColor(0.2f, 0.6f, 0.3f));
	D.Commit(Mesh, 0, false);
	SetLifeSpan(90.f);
}

void AErtLoot::Tick(float Dt)
{
	Super::Tick(Dt);
	T += Dt;
	if (Mesh) { Mesh->SetRelativeLocation(FVector(0, 0, 4.f + FMath::Sin(T * 2.5f) * 4.f)); Mesh->SetRelativeRotation(FRotator(0, T * 40.f, 0)); }
}

FString AErtLoot::Describe() const
{
	FString S;
	if (Gold > 0) S += FString::Printf(TEXT("%d oltin  "), Gold);
	if (Arrows > 0) S += FString::Printf(TEXT("%d o'q  "), Arrows);
	if (Potions > 0) S += FString::Printf(TEXT("%d dori  "), Potions);
	if (Meat > 0) S += FString::Printf(TEXT("%d go'sht  "), Meat);
	return S.TrimEnd();
}

void AErtLoot::GiveTo(AErtCharacter* H)
{
	if (!H) return;
	H->AddGold(Gold); H->AddArrows(Arrows); H->Potions += Potions; H->Meat += Meat;
	Destroy();
}

// ---------------- Nishon ----------------

AErtTarget::AErtTarget()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AErtTarget::BeginPlay()
{
	Super::BeginPlay();
	Mesh = NewObject<UProceduralMeshComponent>(this, TEXT("TargetMesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->RegisterComponent();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"))) Mesh->SetMaterial(0, M);
	FErtMeshData D;
	D.AddBox(FVector(0, 0, 60.f), FVector(4.f, 4.f, 60.f), FLinearColor(0.4f, 0.28f, 0.14f));                        // ustun
	D.AddCylinder(FVector(0, 0, 130.f), 45.f, 45.f, 10.f, 16, FLinearColor(0.85f, 0.75f, 0.45f), true, FRotator(90.f, 0, 0));  // somon halqa
	D.AddCylinder(FVector(-6.f, 0, 130.f), 28.f, 28.f, 3.f, 16, FLinearColor(0.9f, 0.9f, 0.85f), true, FRotator(90.f, 0, 0));
	D.AddCylinder(FVector(-9.5f, 0, 130.f), 12.f, 12.f, 3.f, 12, FLinearColor(0.8f, 0.15f, 0.12f), true, FRotator(90.f, 0, 0)); // qizil markaz
	D.Commit(Mesh, 0, true);
}

void AErtTarget::Tick(float Dt)
{
	Super::Tick(Dt);
	HitFlash = FMath::Max(0.f, HitFlash - Dt);
	if (Mesh) Mesh->SetRelativeRotation(FRotator(0, 0, FMath::Sin(HitFlash * 20.f) * HitFlash * 12.f));
}
