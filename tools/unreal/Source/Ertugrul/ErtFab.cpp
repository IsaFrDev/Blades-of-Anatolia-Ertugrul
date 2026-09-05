#include "ErtFab.h"
#include "Ertugrul.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FErtFabLib& FErtFabLib::Get()
{
	static FErtFabLib Lib;
	if (!Lib.bScanned) Lib.Scan();
	return Lib;
}

static UStaticMesh* ErtLoadMesh(const FString& Path)
{
	if (Path.IsEmpty()) return nullptr;
	FString P = Path;
	if (!P.Contains(TEXT("."))) { FString Name = FPaths::GetBaseFilename(P); P = P + TEXT(".") + Name; }
	return LoadObject<UStaticMesh>(nullptr, *P);
}

void FErtFabLib::Scan()
{
	bScanned = true;
	Trees.Reset(); Pines.Reset(); Rocks.Reset(); Bushes.Reset(); Grass.Reset();
	TArray<FString> ScanPaths = { TEXT("/Game/Fab"), TEXT("/Game/Megascans"), TEXT("/Game/Quixel"), TEXT("/Game/MegascansLibrary"), TEXT("/Game/ErtAssets") };
	// Manifest
	const FString ManifestPath = FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/fab_assets.json");
	FString Json;
	if (FFileHelper::LoadFileToString(Json, *ManifestPath))
	{
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
		{
			auto Fill = [&](const TCHAR* Key, TArray<UStaticMesh*>& Out)
			{
				const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
				if (!Root->TryGetArrayField(Key, Arr)) return;
				for (const TSharedPtr<FJsonValue>& V : *Arr) if (UStaticMesh* M = ErtLoadMesh(V->AsString())) Out.Add(M);
			};
			Fill(TEXT("trees"), Trees); Fill(TEXT("pines"), Pines); Fill(TEXT("rocks"), Rocks); Fill(TEXT("bushes"), Bushes); Fill(TEXT("grass"), Grass);
			const TArray<TSharedPtr<FJsonValue>>* Paths = nullptr;
			if (Root->TryGetArrayField(TEXT("scan_paths"), Paths)) for (const TSharedPtr<FJsonValue>& V : *Paths) ScanPaths.AddUnique(V->AsString());
		}
	}
	// Asset Registry: statik meshlar nomi bo'yicha
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AR = ARM.Get();
	int32 Found = 0;
	for (const FString& Root : ScanPaths)
	{
		TArray<FAssetData> Assets;
		AR.GetAssetsByPath(FName(*Root), Assets, true);
		for (const FAssetData& A : Assets)
		{
			if (A.AssetClassPath.GetAssetName() != TEXT("StaticMesh")) continue;
			const FString Name = A.AssetName.ToString().ToLower();
			TArray<UStaticMesh*>* Target = nullptr;
			if (Name.Contains(TEXT("pine")) || Name.Contains(TEXT("fir")) || Name.Contains(TEXT("spruce")) || Name.Contains(TEXT("cedar")) || Name.Contains(TEXT("conifer"))) Target = &Pines;
			else if (Name.Contains(TEXT("tree")) || Name.Contains(TEXT("oak")) || Name.Contains(TEXT("beech")) || Name.Contains(TEXT("birch")) || Name.Contains(TEXT("maple")) || Name.Contains(TEXT("poplar")) || Name.Contains(TEXT("willow")) || Name.Contains(TEXT("olive"))) Target = &Trees;
			else if (Name.Contains(TEXT("rock")) || Name.Contains(TEXT("boulder")) || Name.Contains(TEXT("cliff")) || Name.Contains(TEXT("stone"))) Target = &Rocks;
			else if (Name.Contains(TEXT("bush")) || Name.Contains(TEXT("shrub"))) Target = &Bushes;
			else if (Name.Contains(TEXT("grass")) || Name.Contains(TEXT("fern")) || Name.Contains(TEXT("plant")) || Name.Contains(TEXT("clover"))) Target = &Grass;
			if (!Target) continue;
			if (Name.Contains(TEXT("_lod")) && !Name.EndsWith(TEXT("lod0"))) continue;
			if (UStaticMesh* M = Cast<UStaticMesh>(A.GetAsset())) { Target->AddUnique(M); ++Found; }
		}
	}
	UE_LOG(LogErtugrul, Log, TEXT("Fab kutubxonasi: daraxt %d, qarag'ay %d, qoya %d, buta %d, o't %d (registrdan %d)"), Trees.Num(), Pines.Num(), Rocks.Num(), Bushes.Num(), Grass.Num(), Found);
}

float FErtFabLib::ScaleToHeight(UStaticMesh* M, float TargetMeters)
{
	if (!M) return 1.f;
	const float H = M->GetBounds().BoxExtent.Z * 2.f;
	return H > 1.f ? TargetMeters * 100.f / H : 1.f;
}

float FErtFabLib::ScaleToRadius(UStaticMesh* M, float TargetMeters)
{
	if (!M) return 1.f;
	const FVector Ext = M->GetBounds().BoxExtent;
	const float R = FMath::Max3(Ext.X, Ext.Y, Ext.Z);
	return R > 1.f ? TargetMeters * 100.f / R : 1.f;
}
