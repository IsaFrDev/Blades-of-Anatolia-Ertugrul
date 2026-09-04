#include "Character/ACRPGPlayerCharacter.h"

#include "Core/ACRPG.h"
#include "Core/ACRPGPlayerController.h"
#include "Components/ACRPGStatsComponent.h"
#include "Components/ACRPGCombatComponent.h"
#include "Components/ACRPGVaultingComponent.h"
#include "Components/ACRPGAssassinationComponent.h"
#include "Components/ACRPGTargetingComponent.h"
#include "Components/ACRPGEquipmentComponent.h"
#include "Components/ACRPGInventoryComponent.h"
#include "Components/ACRPGClimbingComponent.h"
#include "Components/ACRPGQuestComponent.h"
#include "Components/ACRPGSwimmingComponent.h"
#include "Components/ACRPGRidingComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

AACRPGPlayerCharacter::AACRPGPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- Ep.#2: kamera ---
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->bUsePawnControlRotation = true;		// sichqoncha boom'ni aylantiradi
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 70.f);	// yelka ustidan ko'rinish
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// --- Komponentlar ---
	VaultingComponent		= CreateDefaultSubobject<UACRPGVaultingComponent>(TEXT("Vaulting"));
	AssassinationComponent	= CreateDefaultSubobject<UACRPGAssassinationComponent>(TEXT("Assassination"));
	TargetingComponent		= CreateDefaultSubobject<UACRPGTargetingComponent>(TEXT("Targeting"));
	InventoryComponent		= CreateDefaultSubobject<UACRPGInventoryComponent>(TEXT("Inventory"));
	EquipmentComponent		= CreateDefaultSubobject<UACRPGEquipmentComponent>(TEXT("Equipment"));
	ClimbingComponent		= CreateDefaultSubobject<UACRPGClimbingComponent>(TEXT("Climbing"));
	QuestComponent			= CreateDefaultSubobject<UACRPGQuestComponent>(TEXT("Quests"));
	SwimmingComponent		= CreateDefaultSubobject<UACRPGSwimmingComponent>(TEXT("Swimming"));
	RidingComponent			= CreateDefaultSubobject<UACRPGRidingComponent>(TEXT("Riding"));

	CharacterTag = TEXT("Player");
}

void AACRPGPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AACRPGPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateLockOnCamera(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// INPUT BOG'LASH (Ep.#2, #78)
// ---------------------------------------------------------------------------

void AACRPGPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogACRPG, Error,
			TEXT("Enhanced Input yoqilmagan! Project Settings > Input > Default Classes ni tekshiring."));
		return;
	}

	// Nomlar: Triggered = har kadr, Started = bosilganda, Completed = qo'yib yuborilganda.
	if (IA_Move)		EIC->BindAction(IA_Move,   ETriggerEvent::Triggered, this, &AACRPGPlayerCharacter::Input_Move);
	if (IA_Look)		EIC->BindAction(IA_Look,   ETriggerEvent::Triggered, this, &AACRPGPlayerCharacter::Input_Look);

	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started,   this, &AACRPGPlayerCharacter::Input_JumpStarted);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AACRPGPlayerCharacter::Input_JumpCompleted);
	}
	if (IA_Sprint)
	{
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started,   this, &AACRPGPlayerCharacter::Input_SprintStarted);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AACRPGPlayerCharacter::Input_SprintCompleted);
	}
	if (IA_Block)
	{
		EIC->BindAction(IA_Block, ETriggerEvent::Started,   this, &AACRPGPlayerCharacter::Input_BlockStarted);
		EIC->BindAction(IA_Block, ETriggerEvent::Completed, this, &AACRPGPlayerCharacter::Input_BlockCompleted);
	}
	if (IA_Aim)
	{
		EIC->BindAction(IA_Aim, ETriggerEvent::Started,   this, &AACRPGPlayerCharacter::Input_AimStarted);
		EIC->BindAction(IA_Aim, ETriggerEvent::Completed, this, &AACRPGPlayerCharacter::Input_AimCompleted);
	}

	if (IA_Crouch)		EIC->BindAction(IA_Crouch,     ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_CrouchToggle);
	if (IA_Attack)		EIC->BindAction(IA_Attack,     ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_Attack);
	if (IA_Dodge)		EIC->BindAction(IA_Dodge,      ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_Dodge);
	if (IA_LockOn)		EIC->BindAction(IA_LockOn,     ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_LockOn);
	if (IA_Interact)	EIC->BindAction(IA_Interact,   ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_Interact);
	if (IA_ToggleMenu)	EIC->BindAction(IA_ToggleMenu, ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_ToggleMenu);
	if (IA_Pause)		EIC->BindAction(IA_Pause,      ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_Pause);
	if (IA_Throw)		EIC->BindAction(IA_Throw,      ETriggerEvent::Started, this, &AACRPGPlayerCharacter::Input_Throw);
}

// ---------------------------------------------------------------------------
// HARAKAT (Ep.#2)
// ---------------------------------------------------------------------------

FVector AACRPGPlayerCharacter::GetMovementDirection(bool bForward) const
{
	// Kamera qayerga qarasa, "oldinga" o'sha yer. Faqat Yaw olinadi —
	// aks holda kameraga qarab yerga yoki osmonga yurib ketasiz.
	const FRotator ControlRot = Controller ? Controller->GetControlRotation() : GetActorRotation();
	const FRotator YawOnly(0.f, ControlRot.Yaw, 0.f);

	return bForward
		? FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X)
		: FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);
}

void AACRPGPlayerCharacter::Input_Move(const FInputActionValue& Value)
{
	if (!CanMove())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();	// X = o'ng/chap, Y = old/orqa

	// Ep.#30-32 — tirmashayotgan bo'lsa, harakatni tirmashish komponenti boshqaradi.
	if (ClimbingComponent && ClimbingComponent->IsClimbing())
	{
		ClimbingComponent->AddClimbInput(Axis);
		return;
	}

	// Ep.#63 — hayvon minib turgan bo'lsa, boshqaruv hayvonga uzatiladi.
	if (RidingComponent && RidingComponent->IsRiding())
	{
		RidingComponent->AddRideInput(Axis);
		return;
	}

	AddMovementInput(GetMovementDirection(true),  Axis.Y);
	AddMovementInput(GetMovementDirection(false), Axis.X);
}

void AACRPGPlayerCharacter::Input_Look(const FInputActionValue& Value)
{
	// Ep.#12 — lock-on paytida kamerani o'yinchi emas, tizim boshqaradi.
	if (TargetingComponent && TargetingComponent->HasTarget())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

// ---------------------------------------------------------------------------
// SAKRASH VA VAULT (Ep.#2, #3)
// ---------------------------------------------------------------------------

void AACRPGPlayerCharacter::Input_JumpStarted()
{
	if (!CanStartAction())
	{
		return;
	}

	// Ep.#3: avval "sakrab o'tsa bo'ladigan to'siq bormi?" deb tekshiramiz.
	// Bo'lsa — vault; bo'lmasa — oddiy sakrash.
	// Bu ustuvorlik tartibi muhim: aks holda har doim sakrab, to'siqqa urilasiz.
	if (VaultingComponent && VaultingComponent->TryVault())
	{
		return;
	}

	// Ep.#32: yuqoridagi qirraga chiqish (mantling).
	if (ClimbingComponent && ClimbingComponent->TryStartClimb())
	{
		return;
	}

	Jump();
}

void AACRPGPlayerCharacter::Input_JumpCompleted()
{
	StopJumping();
}

// ---------------------------------------------------------------------------
// YUGURISH VA CHO'KKALASH (Ep.#2, #6)
// ---------------------------------------------------------------------------

void AACRPGPlayerCharacter::Input_SprintStarted()
{
	bWantsToSprint = true;

	// Ep.#6 — chidamlilik komponenti tugagach avtomatik o'chiradi.
	if (StatsComponent)
	{
		StatsComponent->SetSprinting(true);
	}
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AACRPGPlayerCharacter::Input_SprintCompleted()
{
	bWantsToSprint = false;
	if (StatsComponent)
	{
		StatsComponent->SetSprinting(false);
	}
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AACRPGPlayerCharacter::Input_CrouchToggle()
{
	// Ep.#25 — turishdan oldin tepada joy bormi tekshiriladi.
	// UE ning CanUnCrouch tekshiruvi buni o'zi bajaradi (bCrouchMaintainsBaseLocation).
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

// ---------------------------------------------------------------------------
// JANG (Ep.#4, #9, #12, #69)
// ---------------------------------------------------------------------------

void AACRPGPlayerCharacter::Input_Attack()
{
	if (!CanStartAction())
	{
		return;
	}

	// Ep.#4 — orqadan yaqinlashgan bo'lsa, oddiy hujum o'rniga assassination.
	// Bu tekshiruv hujumdan OLDIN bo'lishi shart.
	if (AssassinationComponent && AssassinationComponent->TryAssassinate())
	{
		return;
	}

	// Ep.#48 — kamon tortilgan bo'lsa, o'q otiladi.
	if (EquipmentComponent && EquipmentComponent->IsAiming())
	{
		EquipmentComponent->FireArrow();
		return;
	}

	// Ep.#9 — oddiy kombo hujumi.
	if (CombatComponent)
	{
		CombatComponent->RequestAttack();
	}
}

void AACRPGPlayerCharacter::Input_BlockStarted()
{
	if (CombatComponent) CombatComponent->StartBlocking();	// Ep.#69
}

void AACRPGPlayerCharacter::Input_BlockCompleted()
{
	if (CombatComponent) CombatComponent->StopBlocking();
}

void AACRPGPlayerCharacter::Input_Dodge()
{
	if (TargetingComponent) TargetingComponent->PerformDodge();	// Ep.#12
}

void AACRPGPlayerCharacter::Input_LockOn()
{
	if (TargetingComponent) TargetingComponent->ToggleLockOn();	// Ep.#12
}

// ---------------------------------------------------------------------------
// KAMON (Ep.#47-48)
// ---------------------------------------------------------------------------

void AACRPGPlayerCharacter::Input_AimStarted()
{
	if (!EquipmentComponent || !EquipmentComponent->HasBowEquipped())
	{
		return;
	}
	EquipmentComponent->SetAiming(true);

	// Nishonga olayotganda personaj kameraga qarab turadi (Ep.#47).
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = AimSpeed;
}

void AACRPGPlayerCharacter::Input_AimCompleted()
{
	if (EquipmentComponent) EquipmentComponent->SetAiming(false);

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = bWantsToSprint ? SprintSpeed : WalkSpeed;
}

// ---------------------------------------------------------------------------
// BOSHQA
// ---------------------------------------------------------------------------

void AACRPGPlayerCharacter::Input_Interact()
{
	// Ep.#38, #42 — oldimizdagi interaktiv obyekt bilan gaplashamiz/olamiz.
	if (QuestComponent) QuestComponent->TryInteract();
}

void AACRPGPlayerCharacter::Input_Throw()
{
	// Ep.#27 — tosh otib AI ni chalg'itish.
	if (CombatComponent) CombatComponent->ThrowDistraction();
}

void AACRPGPlayerCharacter::Input_ToggleMenu()
{
	if (EquipmentComponent) EquipmentComponent->ToggleEquipmentMenu();	// Ep.#15
}

void AACRPGPlayerCharacter::Input_Pause()
{
	if (AACRPGPlayerController* PC = Cast<AACRPGPlayerController>(GetController()))
	{
		PC->TogglePauseMenu();	// Ep.#64
	}
}

// ---------------------------------------------------------------------------
// LOCK-ON KAMERA (Ep.#12)
// ---------------------------------------------------------------------------

void AACRPGPlayerCharacter::UpdateLockOnCamera(float DeltaSeconds)
{
	if (!TargetingComponent || !TargetingComponent->HasTarget() || !Controller)
	{
		return;
	}

	AActor* Target = TargetingComponent->GetCurrentTarget();
	if (!Target)
	{
		return;
	}

	// Kamerani nishon bilan personaj orasidagi yo'nalishga silliq buramiz.
	const FRotator Desired = UKismetMathLibrary::FindLookAtRotation(
		GetActorLocation(), Target->GetActorLocation());

	const FRotator Smooth = FMath::RInterpTo(
		Controller->GetControlRotation(), Desired, DeltaSeconds, LockOnInterpSpeed);

	// Pitch'ni biroz pastga cheklaymiz — kamera osmonga qarab ketmasin.
	Controller->SetControlRotation(FRotator(FMath::Clamp(Smooth.Pitch, -40.f, 10.f), Smooth.Yaw, 0.f));
}

bool AACRPGPlayerCharacter::CanMove() const
{
	if (!Super::CanMove())
	{
		return false;
	}
	// Ep.#38 — dialog paytida harakat qotadi.
	return true;
}

void AACRPGPlayerCharacter::OnCombatStateChanged(ECombatState OldState, ECombatState NewState)
{
	Super::OnCombatStateChanged(OldState, NewState);

	// Ep.#12 — dodge paytida personaj yo'nalishini o'zgartirmaydi.
	if (NewState == ECombatState::Dodging)
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else if (OldState == ECombatState::Dodging)
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}
