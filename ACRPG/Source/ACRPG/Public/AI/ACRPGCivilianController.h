// ACRPGCivilianController.h — Ep.#29 "Civilian AI and Fixes"
//
// Tinch aholi hech kimga hujum qilmaydi. Uning butun mantiqi ikki holatda:
//   Passive  — bozorda yuradi, ishini qiladi
//   Fleeing  — jang yoki qilich ko'rsa, qochadi
//
// Ep.#66 (Market Towns) da shu AI shaharlarni "tirik" qiladi.
//
// Ep.#29 dagi "fixes" qismi: tinch aholi o'yinchini har ko'rganda qochib
// ketardi. Bu yerda faqat QUROL CHIQARILGAN bo'lsa yoki jang ketayotgan
// bo'lsa qochadi — shunda shaharda tinch yurish mumkin.

#pragma once

#include "CoreMinimal.h"
#include "AI/ACRPGAIControllerBase.h"
#include "ACRPGCivilianController.generated.h"

UCLASS()
class ACRPG_API AACRPGCivilianController : public AACRPGAIControllerBase
{
	GENERATED_BODY()

public:
	AACRPGCivilianController();

protected:
	virtual void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus) override;
	virtual void HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus) override;

	/** Tinch aholi hech kimni dushman deb bilmaydi. */
	virtual bool IsHostileTo(const AActor* Other) const override { return false; }

	/** Shu masofadan yaqin qurolli odam ko'rsa qochadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Tinch aholi", meta = (ClampMin = "100"))
	float PanicDistance = 600.f;

	/** Qochgandan keyin necha soniyada tinchlanadi. */
	UPROPERTY(EditDefaultsOnly, Category = "Tinch aholi", meta = (ClampMin = "1"))
	float CalmDownTime = 12.f;

private:
	void CalmDown();

	FTimerHandle CalmTimer;
};
