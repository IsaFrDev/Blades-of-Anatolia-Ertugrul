// ACRPGTypes.h — butun loyihaning "lug'ati".
//
// Blueprint seriyasida bu ma'lumotlar Enum asset'lari (E_EquipSlot, E_ItemCategory...)
// va Struct asset'lari (S_ItemData, S_QuestData...) sifatida yaratilgan edi.
// C++ da ularni bitta joyda e'lon qilamiz — shunda barcha klasslar bir xil tildan foydalanadi.
//
// Qamrab olingan epizodlar: #5-7 (stats), #14/#16/#18 (equipment), #34-35 (quest),
// #44 (armor), #51 (ammo), #55 (footsteps), #57 (animal AI).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ACRPGTypes.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;
class AACRPGItemBase;
class UAnimMontage;
class USoundBase;

// ---------------------------------------------------------------------------
// EKIPIROVKA (Ep. #14, #16, #17, #18, #19, #44, #45)
// ---------------------------------------------------------------------------

/** Ep.#16-18: personajda nechta "slot" bor va har biri qaysi turdagi buyumni qabul qiladi. */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None		UMETA(DisplayName = "Yo'q"),
	MainHand	UMETA(DisplayName = "Asosiy qo'l (qilich)"),
	OffHand		UMETA(DisplayName = "Yordamchi qo'l (qalqon)"),
	Ranged		UMETA(DisplayName = "Uzoq masofa (kamon)"),
	Head		UMETA(DisplayName = "Bosh"),
	Chest		UMETA(DisplayName = "Ko'krak"),
	Hands		UMETA(DisplayName = "Qo'llar"),
	Legs		UMETA(DisplayName = "Oyoqlar"),
	Quiver		UMETA(DisplayName = "O'qdon"),

	MAX			UMETA(Hidden)
};

/** Ep.#18: buyum kategoriyasi — menyuda filtrlash va slot mosligini tekshirish uchun. */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None		UMETA(DisplayName = "Yo'q"),
	Sword		UMETA(DisplayName = "Qilich"),
	Shield		UMETA(DisplayName = "Qalqon"),
	Bow			UMETA(DisplayName = "Kamon"),
	Armor		UMETA(DisplayName = "Zirh"),
	Consumable	UMETA(DisplayName = "Iste'mol qilinadigan"),
	QuestItem	UMETA(DisplayName = "Kvest buyumi"),
	Ammo		UMETA(DisplayName = "O'q-dori")
};

// ---------------------------------------------------------------------------
// JANG (Ep. #9, #10, #12, #69, #75)
// ---------------------------------------------------------------------------

/** Personaj ayni damda nima qilayotgani. Bir vaqtda faqat bittasi bo'ladi. */
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	Idle			UMETA(DisplayName = "Bo'sh"),
	Attacking		UMETA(DisplayName = "Hujum qilmoqda"),
	Blocking		UMETA(DisplayName = "Bloklamoqda"),		// Ep.#69
	Dodging			UMETA(DisplayName = "Chetga sakramoqda"),	// Ep.#12
	HitReacting		UMETA(DisplayName = "Zarbadan chayqalmoqda"),// Ep.#10
	Assassinating	UMETA(DisplayName = "Assassination"),		// Ep.#4
	Climbing		UMETA(DisplayName = "Tirmashmoqda"),		// Ep.#30
	Vaulting		UMETA(DisplayName = "Sakrab o'tmoqda"),		// Ep.#3
	Swimming		UMETA(DisplayName = "Suzmoqda"),			// Ep.#56
	Riding			UMETA(DisplayName = "Minmoqda"),			// Ep.#63
	Dead			UMETA(DisplayName = "O'lgan")
};

/** Ep.#10, #11: qaysi tomondan zarba yegani — hit reaction montajini tanlash uchun. */
UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front	UMETA(DisplayName = "Old"),
	Back	UMETA(DisplayName = "Orqa"),
	Left	UMETA(DisplayName = "Chap"),
	Right	UMETA(DisplayName = "O'ng")
};

// ---------------------------------------------------------------------------
// AI (Ep. #20-24, #28, #29, #57)
// ---------------------------------------------------------------------------

/** Blackboard'dagi "State" kalitining C++ ko'rinishi. */
UENUM(BlueprintType)
enum class EAIState : uint8
{
	Passive			UMETA(DisplayName = "Passiv"),
	Patrolling		UMETA(DisplayName = "Patrul"),		// Ep.#20
	Investigating	UMETA(DisplayName = "Tekshirmoqda"),	// Ep.#24 (shovqin eshitdi)
	Chasing			UMETA(DisplayName = "Quvmoqda"),		// Ep.#21
	Attacking		UMETA(DisplayName = "Hujum"),		// Ep.#22-23
	Fleeing			UMETA(DisplayName = "Qochmoqda"),		// Ep.#29 (tinch aholi)
	Dead			UMETA(DisplayName = "O'lgan")
};

/** Ep.#57-59: hayvon turi — ov qiladimi, qochadimi, minsa bo'ladimi. */
UENUM(BlueprintType)
enum class EAnimalBehavior : uint8
{
	Passive		UMETA(DisplayName = "Beozor (kiyik)"),
	Aggressive	UMETA(DisplayName = "Yirtqich (sirtlon)"),
	Mountable	UMETA(DisplayName = "Minsa bo'ladi (tuya/ot)")	// Ep.#63
};

// ---------------------------------------------------------------------------
// KVEST (Ep. #34-43)
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EQuestState : uint8
{
	Unavailable	UMETA(DisplayName = "Ochilmagan"),
	Available	UMETA(DisplayName = "Olish mumkin"),
	Active		UMETA(DisplayName = "Faol"),			// Ep.#36
	Completed	UMETA(DisplayName = "Bajarilgan"),		// Ep.#43
	Failed		UMETA(DisplayName = "Muvaffaqiyatsiz")
};

/** Ep.#37, #41, #42: maqsad turi — qanday hodisa uni sanaydi. */
UENUM(BlueprintType)
enum class EQuestObjectiveType : uint8
{
	Kill		UMETA(DisplayName = "Dushmanni yo'q qilish"),	// Ep.#41
	Collect		UMETA(DisplayName = "Buyum yig'ish"),			// Ep.#42
	Reach		UMETA(DisplayName = "Joyga yetib borish"),
	TalkTo		UMETA(DisplayName = "Suhbatlashish"),			// Ep.#38
	Interact	UMETA(DisplayName = "O'zaro ta'sir")
};

// ---------------------------------------------------------------------------
// STRUKTURALAR
// ---------------------------------------------------------------------------

/**
 * Ep.#14, #18, #44, #73 — bitta buyumning barcha ma'lumoti.
 * Blueprint'da bu DT_Items nomli Data Table edi; C++ da FTableRowBase'dan meros olamiz,
 * shunda editor'da xuddi shu Data Table'ni yasay olasiz.
 */
USTRUCT(BlueprintType)
struct ACRPG_API FACRPGItemData : public FTableRowBase
{
	GENERATED_BODY()

	/** Data Table qatorining nomi bilan bir xil bo'lishi shart emas, lekin qulay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Umumiy")
	FName ItemID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Umumiy")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Umumiy", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Umumiy")
	EItemCategory Category = EItemCategory::None;

	/** Ep.#17: bu buyum qaysi slotga tushadi. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Umumiy")
	EEquipmentSlot Slot = EEquipmentSlot::None;

	/** Ep.#73: menyudagi ikonka. Soft ref — kerak bo'lgandagina yuklanadi. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ko'rinish")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Qurol/qalqon uchun dunyoda spawn bo'ladigan aktyor (Ep.#19). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ko'rinish")
	TSoftClassPtr<AACRPGItemBase> ItemActorClass;

	/** Ep.#44-45: zirh uchun — personajga kiyiladigan skeletal mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ko'rinish")
	TSoftObjectPtr<USkeletalMesh> ArmorMesh;

	// --- Sonli xususiyatlar ---

	/** Ep.#10: qilich zarbasining bazaviy urishi. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Statistika")
	float BaseDamage = 0.f;

	/** Ep.#44: zirh bergan himoya (kelgan urishdan ayiriladi). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Statistika")
	float ArmorValue = 0.f;

	/** Ep.#51: bitta qatorda nechta saqlash mumkin (o'q uchun 30, qilich uchun 1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Statistika", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Statistika")
	int32 Value = 0;
};

/** Inventardagi bitta yozuv: qaysi buyum va nechta. */
USTRUCT(BlueprintType)
struct ACRPG_API FACRPGInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventar")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventar", meta = (ClampMin = "0"))
	int32 Quantity = 0;

	bool IsValid() const { return ItemID != NAME_None && Quantity > 0; }
};

/**
 * Ep.#37, #39 — kvestning bitta maqsadi.
 * "3 ta dushmanni o'ldir" => Type=Kill, TargetTag="Bandit", RequiredCount=3.
 */
USTRUCT(BlueprintType)
struct ACRPG_API FACRPGQuestObjective
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Maqsad")
	FText ObjectiveText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Maqsad")
	EQuestObjectiveType Type = EQuestObjectiveType::Kill;

	/** Kimni/nimani. Kill uchun dushman tegi, Collect uchun ItemID, TalkTo uchun NPC tegi. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Maqsad")
	FName TargetTag = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Maqsad", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

	/** O'yin davomida o'zgaradi — Data Table'da 0 bo'lib turadi. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Maqsad")
	int32 CurrentCount = 0;

	/** Ep.#37: xaritada marker qo'yish uchun. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Maqsad")
	FVector WorldLocation = FVector::ZeroVector;

	bool IsComplete() const { return CurrentCount >= RequiredCount; }
};

/**
 * Ep.#34-35, #43 — kvestning to'liq ta'rifi. DT_Quests Data Table qatori.
 */
USTRUCT(BlueprintType)
struct ACRPG_API FACRPGQuestData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kvest")
	FName QuestID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kvest")
	FText QuestName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kvest", meta = (MultiLine = true))
	FText QuestDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kvest")
	TArray<FACRPGQuestObjective> Objectives;

	/** Ep.#38: kvest beruvchi NPC aytadigan gaplar. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialog")
	TArray<FText> DialogueLines;

	/** Ep.#65: bajargani uchun tajriba. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mukofot")
	int32 XPReward = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mukofot")
	TArray<FACRPGInventoryEntry> ItemRewards;

	/** Zanjir kvestlar: shu kvest tugagach keyingisi ochiladi. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kvest")
	FName NextQuestID = NAME_None;
};

/** O'yinchining jurnalidagi kvest holati (saqlanadi — Ep.#81). */
USTRUCT(BlueprintType)
struct ACRPG_API FACRPGQuestProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Kvest")
	FName QuestID = NAME_None;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Kvest")
	EQuestState State = EQuestState::Unavailable;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Kvest")
	TArray<FACRPGQuestObjective> Objectives;
};

/**
 * Ep.#55 — qadam tovushi: sirt turiga qarab boshqa ovoz va boshqa chang effekti.
 * Physical Material'dagi SurfaceType bilan mos keladi.
 */
USTRUCT(BlueprintType)
struct ACRPG_API FACRPGFootstepEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Qadam")
	TEnumAsByte<EPhysicalSurface> Surface = SurfaceType_Default;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Qadam")
	TObjectPtr<USoundBase> Sound = nullptr;

	/** Qumda yurganda chang chiqishi uchun (Ep.#55). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Qadam")
	TObjectPtr<class UNiagaraSystem> Effect = nullptr;
};

/**
 * Ep.#9, #23 — bitta kombo qadami.
 * Blueprint'da bu "Array of Montage" edi; bu yerda har bir qadamga o'z sozlamasini beramiz.
 */
USTRUCT(BlueprintType)
struct ACRPG_API FACRPGComboStep
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kombo")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** Bazaviy urishga ko'paytiriladi (oxirgi zarba kuchliroq bo'lsin). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kombo", meta = (ClampMin = "0.1"))
	float DamageMultiplier = 1.f;

	/** Ep.#6: shu zarba yeydigan chidamlilik. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kombo", meta = (ClampMin = "0"))
	float StaminaCost = 10.f;
};
