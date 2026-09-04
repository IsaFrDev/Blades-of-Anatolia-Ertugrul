#include "ErtAudio.h"
#include "Ertugrul.h"
#include "ErtLoc.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"

TWeakObjectPtr<UAudioComponent> FErtAudio::CurrentVo;
float FErtAudio::LastVoDuration = 0.f;

static const TCHAR* SfxFile(const FString& Name)
{
	if (Name == TEXT("hit")) return TEXT("00_hit.wav");
	if (Name == TEXT("block")) return TEXT("01_block.wav");
	if (Name == TEXT("parry")) return TEXT("02_parry.wav");
	if (Name == TEXT("kill")) return TEXT("03_kill.wav");
	if (Name == TEXT("swing")) return TEXT("04_swing.wav");
	if (Name == TEXT("bowshot")) return TEXT("05_bowshot.wav");
	if (Name == TEXT("arrow_hit")) return TEXT("06_arrow_hit.wav");
	if (Name == TEXT("arrow_wall")) return TEXT("07_arrow_wall.wav");
	if (Name == TEXT("death")) return TEXT("08_death.wav");
	return nullptr;
}

UAudioComponent* FErtAudio::PlayWav(UWorld* World, const FString& Path, const FVector* Pos, float Volume, float Pitch)
{
	if (!World) return nullptr;
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() < 44) { UE_LOG(LogErtugrul, Verbose, TEXT("WAV topilmadi: %s"), *Path); return nullptr; }
	// RIFF: fmt va data bo'laklarini topamiz
	int32 Ch = 1, Rate = 22050, Bits = 16, DataOff = -1, DataLen = 0;
	int32 P = 12;
	while (P + 8 <= Bytes.Num())
	{
		const FString Id = FString::ConstructFromPtrSize((const ANSICHAR*)&Bytes[P], 4);
		const uint32 Len = *(const uint32*)&Bytes[P + 4];
		if (Id == TEXT("fmt "))
		{
			Ch = *(const uint16*)&Bytes[P + 10]; Rate = *(const uint32*)&Bytes[P + 12]; Bits = *(const uint16*)&Bytes[P + 22];
		}
		else if (Id == TEXT("data")) { DataOff = P + 8; DataLen = FMath::Min((int32)Len, Bytes.Num() - DataOff); break; }
		P += 8 + (int32)Len + ((int32)Len & 1);
	}
	if (DataOff < 0 || Bits != 16) { UE_LOG(LogErtugrul, Warning, TEXT("WAV formati qo'llanmaydi: %s"), *Path); return nullptr; }
	USoundWaveProcedural* W = NewObject<USoundWaveProcedural>(World);
	W->SetSampleRate(Rate);
	W->NumChannels = Ch;
	W->Duration = INDEFINITELY_LOOPING_DURATION;
	W->SoundGroup = SOUNDGROUP_Default;
	W->bLooping = false;
	W->Pitch = Pitch;
	W->QueueAudio(&Bytes[DataOff], DataLen);
	LastVoDuration = (float)DataLen / (float)(Rate * Ch * 2);
	UAudioComponent* AC = Pos ? UGameplayStatics::SpawnSoundAtLocation(World, W, *Pos, FRotator::ZeroRotator, Volume, Pitch, 0.f, nullptr, nullptr, true)
	                          : UGameplayStatics::SpawnSound2D(World, W, Volume, Pitch, 0.f, nullptr, false, true);
	return AC;
}

UAudioComponent* FErtAudio::PlaySfx(UWorld* World, const FString& Name, const FVector& Pos, float Volume, float Pitch)
{
	const TCHAR* F = SfxFile(Name);
	if (!F) return nullptr;
	return PlayWav(World, FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/audio/sfx") / F, &Pos, Volume, Pitch);
}

UAudioComponent* FErtAudio::PlayVo(UWorld* World, const FString& VoId, float Volume)
{
	StopVo();
	static const TCHAR* Langs[] = { TEXT("uz"), TEXT("tr"), TEXT("en") };
	const int32 L = FMath::Clamp(FErtLoc::Get().GetLanguage(), 0, 2);
	FString Path = FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/audio/vo") / Langs[L] / (VoId + TEXT(".wav"));
	if (!FPaths::FileExists(Path)) Path = FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/audio/vo/uz") / (VoId + TEXT(".wav"));
	UAudioComponent* AC = PlayWav(World, Path, nullptr, Volume, 1.f);
	CurrentVo = AC;
	return AC;
}

void FErtAudio::StopVo()
{
	if (CurrentVo.IsValid()) { CurrentVo->Stop(); CurrentVo = nullptr; }
}
