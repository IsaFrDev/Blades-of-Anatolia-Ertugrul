// Fab/Quixel Megascans assetlari: loyihaga yuklangan statik meshlar avtomatik topilib (Asset Registry),
// nomiga qarab toifalanadi (daraxt, qarag'ay, qoya, buta, o't). Manifest (Content/Ertugrul/Data/fab_assets.json) ustunlik qiladi.
// Hech narsa topilmasa protsedural daraxt/qoyalar ishlatiladi.
#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

struct FErtFabLib
{
	TArray<UStaticMesh*> Trees, Pines, Rocks, Bushes, Grass, Props, Stumps;   // Props: bochka/yashik/chelak/fonar/stol; Stumps: to'nka/yotgan tana
	bool bScanned = false;

	/** Manifest + Asset Registry skaneri. Manifest: {"trees":[...], "pines":[...], "rocks":[...], "bushes":[...], "grass":[...], "scan_paths":["/Game/Fab", ...]} */
	void Scan();
	bool HasTrees() const { return Trees.Num() + Pines.Num() > 0; }
	bool HasRocks() const { return Rocks.Num() > 0; }
	/** Mesh balandligini (bounds Z) berilgan metrga keltiruvchi masshtab */
	static float ScaleToHeight(UStaticMesh* M, float TargetMeters);
	static float ScaleToRadius(UStaticMesh* M, float TargetMeters);
	static FErtFabLib& Get();
};
