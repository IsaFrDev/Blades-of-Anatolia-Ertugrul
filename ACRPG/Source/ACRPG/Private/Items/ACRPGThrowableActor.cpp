#include "Items/ACRPGThrowableActor.h"

#include "Core/ACRPG.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"

AACRPGThrowableActor::AACRPGThrowableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetNotifyRigidBodyCollision(true);	// OnComponentHit ishlashi uchun SHART

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->ProjectileGravityScale = 1.f;

	InitialLifeSpan = 20.f;
}

void AACRPGThrowableActor::BeginPlay()
{
	Super::BeginPlay();
	Mesh->OnComponentHit.AddDynamic(this, &AACRPGThrowableActor::OnImpact);
}

void AACRPGThrowableActor::Launch(const FVector& InVelocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = InVelocity;
	}
}

void AACRPGThrowableActor::OnImpact(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Faqat BIRINCHI urilishda shovqin chiqaramiz — aks holda har sakrashda
	// AI ni chaqirib, butun qarorgohni boshimizga to'playmiz.
	if (bHasImpacted)
	{
		return;
	}
	bHasImpacted = true;

	const FVector NoiseLocation = Hit.ImpactPoint;

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, NoiseLocation);
	}

	// --- ASOSIY QISM: AI ga shovqin haqida xabar beramiz ---
	//
	// Instigator = nullptr: AI shovqinni eshitadi, lekin uni O'YINCHIGA bog'lamaydi.
	// Agar bu yerga o'yinchini bersak, AI to'g'ridan-to'g'ri o'yinchiga yuguradi —
	// bu esa chalg'itishning butun ma'nosini yo'qotadi.
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		NoiseLocation,
		NoiseLoudness,
		nullptr,		// Instigator — ataylab bo'sh
		0.f,			// MaxRange (0 = sezgi sozlamasidagi radius)
		NoiseTag);

	UE_LOG(LogACRPG, Log, TEXT("Chalg'itish shovqini: %s"), *NoiseLocation.ToCompactString());

	SetLifeSpan(LifeAfterImpact);
}
