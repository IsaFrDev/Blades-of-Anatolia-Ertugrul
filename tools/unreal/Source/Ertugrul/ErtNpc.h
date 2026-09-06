// NPC: turgan personaj (protsedural tana), yaqinlashganda o'yinchiga qaraydi, [E] bilan dialog boshlanadi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ErtNpc.generated.h"

class UErtHeroBody;

UCLASS()
class ERTUGRUL_API AErtNpc : public AActor
{
	GENERATED_BODY()
public:
	AErtNpc();
	void Setup(const FString& InId, const FString& NameKey, const FString& InDialogId, bool bWoman, const FLinearColor& Kaftan, float Yaw);
	const FString& GetNpcId() const { return Id; }
	const FString& GetDialogId() const { return DialogId; }
	FString GetDisplayName() const;

protected:
	virtual void Tick(float Dt) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UErtHeroBody> Body;
	FString Id, LocName, DialogId;
	float HomeYaw = 0.f;
	FVector HomePos = FVector::ZeroVector, Target = FVector::ZeroVector;
	// Kundalik vazifa: 1 suv tashish (quduq <-> o'tov), 2 o't o'rish (egilib turadi), 3 bola (yugurib o'ynaydi)
	int32 Chore = 0; FVector ChoreA = FVector::ZeroVector, ChoreB = FVector::ZeroVector; float ChoreT = 0.f; bool bAtB = false, bBend = false;
public:
	void SetChore(int32 Kind, const FVector& A, const FVector& B) { Chore = Kind; ChoreA = A; ChoreB = B; ChoreT = FMath::FRandRange(0.f, 3.f); if (Kind == 3) SetActorScale3D(FVector(0.62f)); }
private:
	float WanderT = 3.f;
	bool bTalking = false;
	float GreetCD = 0.f;
	FString GreetText; float GreetT = 0.f;
	bool bWomanNpc = false;
public:
	const FString& GetGreetText() const { return GreetText; }
	float GetGreetT() const { return GreetT; }
private:
public:
	void SetTalking(bool b) { bTalking = b; }
};
