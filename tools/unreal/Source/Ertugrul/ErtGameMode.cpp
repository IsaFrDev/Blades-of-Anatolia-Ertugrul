#include "ErtGameMode.h"
#include "ErtCharacter.h"

AErtGameMode::AErtGameMode()
{
	DefaultPawnClass = AErtCharacter::StaticClass();
}
