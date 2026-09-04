// Modul sarlavhasi. UE har bir game module'dan shu ikki funksiyani kutadi.
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

ACRPG_API DECLARE_LOG_CATEGORY_EXTERN(LogACRPG, Log, All);

class FACRPGModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
