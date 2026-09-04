// Ish vaqtida WAV o'ynatish (import qilinmagan fayllar): Content/Ertugrul/Data/audio/{sfx,vo/<til>}/*.wav
// PCM 16-bit mono/stereo -> USoundWaveProcedural -> 2D yoki joylashuvli ovoz.
#pragma once

#include "CoreMinimal.h"

class UWorld;
class UAudioComponent;

class ERTUGRUL_API FErtAudio
{
public:
	/** SFX: nom (hit, block, parry, kill, swing, bowshot, arrow_hit, arrow_wall, death) */
	static UAudioComponent* PlaySfx(UWorld* World, const FString& Name, const FVector& Pos, float Volume = 1.f, float Pitch = 1.f);
	/** VO: id (masalan ep001_intro_01), joriy til papkasidan; topilmasa uz */
	static UAudioComponent* PlayVo(UWorld* World, const FString& VoId, float Volume = 1.f);
	static void StopVo();
	static float LastVoDuration;

private:
	static UAudioComponent* PlayWav(UWorld* World, const FString& Path, const FVector* Pos, float Volume, float Pitch);
	static TWeakObjectPtr<UAudioComponent> CurrentVo;
};
