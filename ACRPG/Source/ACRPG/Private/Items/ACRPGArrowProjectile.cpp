#include "Items/ACRPGArrowProjectile.h"

#include "Core/ACRPG.h"
#include "Character/ACRPGCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"

AACRPGArrowProjectile::AACRPGArrowProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	SetRootComponent(CollisionCapsule);
	CollisionCapsule->InitCapsuleSize(3.f, 30.f);
	// O'q uzunligi bo'ylab yotgan kapsula — shuning uchun 90 gradus buramiz.
	CollisionCapsule->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionCapsule->SetNotifyRigidBodyCollision(true);

	ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	ArrowMesh->SetupAttachment(CollisionCapsule);
	ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 4000.f;
	ProjectileMovement->MaxSpeed = 4000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;	// o'q uchi oldinga qaraydi
	ProjectileMovement->ProjectileGravityScale = 0.35f;		// biroz pastga tushadi — realistik
	ProjectileMovement->bShouldBounce = false;

	InitialLifeSpan = 10.f;	// tegmasa 10 soniyada yo'qoladi
}

void AACRPGArrowProjectile::BeginPlay()
{
	Super::BeginPlay();
	SpawnLocation = GetActorLocation();
	CollisionCapsule->OnComponentHit.AddDynamic(this, &AACRPGArrowProjectile::OnArrowHit);
}

void AACRPGArrowProjectile::InitializeArrow(const FVector& InVelocity, float InDamage, AACRPGCharacterBase* InShooter)
{
	Damage = InDamage;
	Shooter = InShooter;

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = InVelocity;
	}

	// O'zimizni otmaymiz.
	if (Shooter)
	{
		CollisionCapsule->IgnoreActorWhenMoving(Shooter, true);
	}
}

void AACRPGArrowProjectile::OnArrowHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit || OtherActor == Shooter)
	{
		return;
	}
	bHasHit = true;

	// --- Urishni hisoblaymiz ---
	float FinalDamage = Damage;

	// Ep.#50 — masofa bo'yicha kamayish.
	if (bUseDistanceFalloff)
	{
		const float Distance = FVector::Dist(SpawnLocation, GetActorLocation());
		const float Falloff = FMath::Clamp(1.f - (Distance - FullDamageRange) / FullDamageRange, 0.3f, 1.f);
		FinalDamage *= Falloff;
	}

	// Boshga tekkanmi?
	if (Hit.BoneName == HeadBoneName)
	{
		FinalDamage *= HeadshotMultiplier;
		UE_LOG(LogACRPG, Log, TEXT("Boshga tegdi! x%.1f"), HeadshotMultiplier);
	}

	if (AACRPGCharacterBase* Victim = Cast<AACRPGCharacterBase>(OtherActor))
	{
		FPointDamageEvent DamageEvent;
		DamageEvent.Damage = FinalDamage;
		DamageEvent.HitInfo = Hit;
		DamageEvent.ShotDirection = GetActorForwardVector();

		Victim->TakeDamage(FinalDamage, DamageEvent,
			Shooter ? Shooter->GetController() : nullptr, Shooter);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
	}

	// --- Ep.#49: o'qni tekkan yerga "yopishtirib" qo'yamiz ---
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->ProjectileGravityScale = 0.f;
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Harakatlanuvchi obyektga tekkan bo'lsa, unga ulanamiz — dushman bilan birga qimirlaydi.
	if (OtherComp && OtherActor)
	{
		AttachToComponent(OtherComp,
			FAttachmentTransformRules::KeepWorldTransform, Hit.BoneName);
	}

	SetLifeSpan(StuckLifeSpan);
}
