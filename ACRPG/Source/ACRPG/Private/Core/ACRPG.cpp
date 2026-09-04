#include "Core/ACRPG.h"

DEFINE_LOG_CATEGORY(LogACRPG);

void FACRPGModule::StartupModule()
{
	UE_LOG(LogACRPG, Log, TEXT("ACRPG moduli ishga tushdi."));
}

void FACRPGModule::ShutdownModule()
{
	UE_LOG(LogACRPG, Log, TEXT("ACRPG moduli to'xtadi."));
}

// IMPLEMENT_PRIMARY_GAME_MODULE — loyihada faqat BITTA marta bo'lishi kerak.
IMPLEMENT_PRIMARY_GAME_MODULE(FACRPGModule, ACRPG, "ACRPG");
