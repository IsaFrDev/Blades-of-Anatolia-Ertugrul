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
	Trees.Reset(); Pines.Reset(); Rocks.Reset(); Bushes.Reset(); Grass.Reset(); Props.Reset(); Stumps.Reset();
	Yurts.Reset(); Houses.Reset(); Gates.Reset(); Wells.Reset(); Carts.Reset(); Stalls.Reset(); Tents.Reset();
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
				for (const TSharedPtr<FJsonValue>& V : *Arr) if (UStaticMesh* M = ErtLoadMesh(V->AsString())) { M->AddToRoot(); Out.Add(M); }
			};
			Fill(TEXT("trees"), Trees); Fill(TEXT("pines"), Pines); Fill(TEXT("rocks"), Rocks); Fill(TEXT("bushes"), Bushes); Fill(TEXT("grass"), Grass); Fill(TEXT("props"), Props); Fill(TEXT("stumps"), Stumps);
			const TArray<TSharedPtr<FJsonValue>>* Paths = nullptr;
			if (Root->TryGetArrayField(TEXT("scan_paths"), Paths)) for (const TSharedPtr<FJsonValue>& V : *Paths) ScanPaths.AddUnique(V->AsString());
		}
	}
	// Asset Registry: statik meshlar nomi bo'yicha
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AR = ARM.Get();
	// O'yin rejimida registr asinxron yuklanadi - bu yo'llarni sinxron skanerlaymiz
	AR.ScanPathsSynchronous(ScanPaths, true);
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
			if (Name.Contains(TEXT("yurt")) || Name.Contains(TEXT("ger_"))) Target = &Yurts;
			else if (Name.Contains(TEXT("tent"))) Target = &Tents;
			else if (Name.Contains(TEXT("house")) || Name.Contains(TEXT("hut")) || Name.Contains(TEXT("cottage"))) Target = &Houses;
			else if (Name.Contains(TEXT("gate"))) Target = &Gates;
			else if (Name.Contains(TEXT("well"))) Target = &Wells;
			else if (Name.Contains(TEXT("cart")) || Name.Contains(TEXT("wagon"))) Target = &Carts;
			else if (Name.Contains(TEXT("stall")) || Name.Contains(TEXT("market")) || Name.Contains(TEXT("bazaar"))) Target = &Stalls;
			else if (Name.Contains(TEXT("stump")) || Name.Contains(TEXT("trunk")) || Name.Contains(TEXT("log"))) Target = &Stumps;
			else if (Name.Contains(TEXT("barrel")) || Name.Contains(TEXT("crate")) || Name.Contains(TEXT("bucket")) || Name.Contains(TEXT("lantern")) || Name.Contains(TEXT("table")) || Name.Contains(TEXT("stool")) || Name.Contains(TEXT("fire_pit")) || Name.Contains(TEXT("firepit")) || Name.Contains(TEXT("shield")) || Name.Contains(TEXT("tether")) || Name.Contains(TEXT("trough")) || Name.Contains(TEXT("hay")) || Name.Contains(TEXT("woodpile")) || Name.Contains(TEXT("banner")) || Name.Contains(TEXT("rack"))) Target = &Props;
			else if (Name.Contains(TEXT("pine")) || Name.Contains(TEXT("fir")) || Name.Contains(TEXT("spruce")) || Name.Contains(TEXT("cedar")) || Name.Contains(TEXT("conifer"))) Target = &Pines;
			else if (Name.Contains(TEXT("tree")) || Name.Contains(TEXT("oak")) || Name.Contains(TEXT("beech")) || Name.Contains(TEXT("birch")) || Name.Contains(TEXT("maple")) || Name.Contains(TEXT("poplar")) || Name.Contains(TEXT("willow")) || Name.Contains(TEXT("olive"))) Target = &Trees;
			else if (Name.Contains(TEXT("rock")) || Name.Contains(TEXT("boulder")) || Name.Contains(TEXT("cliff")) || Name.Contains(TEXT("stone"))) Target = &Rocks;
			else if (Name.Contains(TEXT("bush")) || Name.Contains(TEXT("shrub"))) Target = &Bushes;
			else if (Name.Contains(TEXT("grass")) || Name.Contains(TEXT("fern")) || Name.Contains(TEXT("plant")) || Name.Contains(TEXT("clover"))) Target = &Grass;
			if (!Target) continue;
			if (Name.Contains(TEXT("_lod")) && !Name.EndsWith(TEXT("lod0"))) continue;
			// Ko'p qismli importlarning qism-meshlari (SM_PH_x_1, _2 ...): buyumlar uchun faqat asosiy mesh
			if (Target == &Props) { int32 Us = -1; if (Name.FindLastChar(TEXT('_'), Us) && Us + 1 < Name.Len() && FChar::IsDigit(Name[Us + 1]) && Name.Mid(Us + 1).IsNumeric() && Name.Mid(Us + 1).Len() <= 2 && !Name.Contains(TEXT("_0"))) continue; }
			if (UStaticMesh* M = Cast<UStaticMesh>(A.GetAsset())) { M->AddToRoot(); Target->AddUnique(M); ++Found; }   // GC himoyasi
		}
	}
	UE_LOG(LogErtugrul, Log, TEXT("Fab kutubxonasi: daraxt %d, qarag'ay %d, qoya %d, buta %d, o't %d, buyum %d, to'nka %d; o'tov %d, uy %d, darvoza %d, quduq %d, arava %d, rasta %d, chodir %d (registrdan %d)"), Trees.Num(), Pines.Num(), Rocks.Num(), Bushes.Num(), Grass.Num(), Props.Num(), Stumps.Num(), Yurts.Num(), Houses.Num(), Gates.Num(), Wells.Num(), Carts.Num(), Stalls.Num(), Tents.Num(), Found);
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
