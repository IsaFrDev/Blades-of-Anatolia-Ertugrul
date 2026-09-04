// HUD: epizod/bosqich sarlavhasi, maqsadlar, sog'liq/stamina/o'q, markerlar, holat xabarlari.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ErtHUD.generated.h"

class AErtMissionDirector;

UCLASS()
class ERTUGRUL_API AErtHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;

private:
	AErtMissionDirector* Director() const;
	void DrawMenu(float SW, float SH, float Sc);
	void DrawCutscene(float SW, float SH, float Sc);
	void DrawDialog(float SW, float SH, float Sc);
	void Wrap(const FString& S, float MaxW, float Scale, TArray<FString>& Out) const;
	void Text(const FString& S, float X, float Y, const FLinearColor& C, float Scale = 1.f, bool bShadow = true, bool bLarge = false);
	float TextWidth(const FString& S, float Scale, bool bLarge) const;
	void Bar(float X, float Y, float W, float H, float Frac, const FLinearColor& Fill);
};
