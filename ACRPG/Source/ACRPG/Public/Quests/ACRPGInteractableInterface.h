// ACRPGInteractableInterface.h — Ep.#38, #42
//
// Interfeys — "shu narsa bilan gaplashish/ishlatish mumkin" degan belgi.
//
// Nega interfeys, meros emas?
// Chunki o'zaro ta'sir qiladigan narsalar juda har xil: NPC (Character),
// sandiq (Actor), eshik, o't. Ularni bitta bazaviy klassga tiqib bo'lmaydi.
// Interfeys esa istalgan klassga qo'shiladi.
//
// BlueprintNativeEvent — funksiyani C++ da ham, Blueprint'da ham yozish mumkin.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ACRPGInteractableInterface.generated.h"

class AACRPGCharacterBase;

UINTERFACE(MinimalAPI, Blueprintable)
class UACRPGInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class ACRPG_API IACRPGInteractableInterface
{
	GENERATED_BODY()

public:
	/** O'yinchi "E" bosganda. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "O'zaro ta'sir")
	void OnInteract(AACRPGCharacterBase* Interactor);

	/** Ekranda ko'rsatiladigan matn: "E — Gaplashish". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "O'zaro ta'sir")
	FText GetInteractionPrompt() const;
};
