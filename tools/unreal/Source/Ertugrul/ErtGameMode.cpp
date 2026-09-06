#include "ErtGameMode.h"
#include "ErtNav.h"
#include "ErtMap3D.h"
#include "Ertugrul.h"
#include "ErtCharacter.h"
#include "ErtCutscene.h"
#include "ErtEpisodeDb.h"
#include "ErtHUD.h"
#include "ErtHorse.h"
#include "ErtNpc.h"
#include "ErtLoot.h"
#include "ErtBoat.h"
#include "ErtWorldBuilder.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "ErtLoc.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/PostProcessVolume.h"
#include "ErtWeather.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "ErtMission.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

AErtGameMode::AErtGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	Visited.SetNumZeroed(VisN * VisN);
	DefaultPawnClass = AErtCharacter::StaticClass();
	HUDClass = AErtHUD::StaticClass();
}

void AErtGameMode::BeginPlay()
{
	Super::BeginPlay();
	Director = GetWorld()->SpawnActor<AErtMissionDirector>();
	// Oba darvozasi oldida ikkita ot
	{
		FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FLinearColor Coats[] = { FLinearColor(0.36f, 0.22f, 0.11f), FLinearColor(0.15f, 0.12f, 0.10f) };
		for (int32 i = 0; i < 2; ++i)
		{
			AErtHorse* Hs = GetWorld()->SpawnActor<AErtHorse>(AErtHorse::StaticClass(), FVector(44600.f + i * 350.f, -56000.f + i * 250.f, 2250.f), FRotator(0, 0.f, 0), SP);
			if (Hs) Hs->Init(Coats[i]);
		}
		// Qayiqlar: daryoda (oba yaqinida) ikkita, ko'lda bitta
		if (AErtWorldBuilder* Wb = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(this, AErtWorldBuilder::StaticClass())))
		{
			const float Ns[] = { 480.f, 300.f };
			for (float N : Ns) { const float E = Wb->RiverE(N) + 8.f; GetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector(N * 100.f, E * 100.f, ErtMap::WaterZ * 100.f + 10.f), FRotator(0, 0, 0), SP); }
			GetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector((ErtMap::LakeN - 20.f) * 100.f, (ErtMap::LakeE + 10.f) * 100.f, ErtMap::LakeZ * 100.f + 10.f), FRotator(0, 45.f, 0), SP);
			GetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector((ErtMap::AskN - ErtMap::AskR + 14.f) * 100.f, (ErtMap::AskE + 8.f) * 100.f, ErtMap::AskZ * 100.f + 10.f), FRotator(0, 0, 0), SP);
			GetWorld()->SpawnActor<AErtBoat>(AErtBoat::StaticClass(), FVector((ErtMap::AskN - 10.f) * 100.f, (ErtMap::AskE - 30.f) * 100.f, ErtMap::AskZ * 100.f + 10.f), FRotator(0, 120.f, 0), SP);
		}
		// Karvonsaroy oldida ikkita tuya
		for (int32 i = 0; i < 2; ++i)
		{
			AErtHorse* Cm = GetWorld()->SpawnActor<AErtHorse>(AErtHorse::StaticClass(), FVector((ErtMap::CaravanN + 30.f) * 100.f, (ErtMap::CaravanE - 10.f + i * 6.f) * 100.f, 1500.f), FRotator(0, 90.f, 0), SP);
			if (Cm) Cm->Init(FLinearColor(0.72f + i * 0.05f, 0.58f, 0.36f), true);
		}
	}
	Cutscene = GetWorld()->SpawnActor<AErtCutsceneDirector>();
	Weather = GetWorld()->SpawnActor<AErtWeather>();
	bUnlockAll = FParse::Param(FCommandLine::Get(), TEXT("ErtUnlockAll"));
	{
		FString Gfx; if (FParse::Value(FCommandLine::Get(), TEXT("-ErtGfx="), Gfx)) GfxPreset = Gfx == TEXT("ultra") ? 0 : (Gfx == TEXT("low") || Gfx == TEXT("s") ? 2 : 1);
		ApplyGfxPreset();
		if (FParse::Param(FCommandLine::Get(), TEXT("ErtNoGps"))) bGps = false;
	}
	SpawnNpcs();
	LoadGame();
	{ FTimerHandle Th2; GetWorldTimerManager().SetTimer(Th2, [this]() { LoadGame(); }, 0.5f, false); }   // o'yinchi paydo bo'lgach inventar/daraja
	Sun = Cast<ADirectionalLight>(UGameplayStatics::GetActorOfClass(this, ADirectionalLight::StaticClass()));
	Sky = Cast<ASkyLight>(UGameplayStatics::GetActorOfClass(this, ASkyLight::StaticClass()));
	// Quyosh nurlari (ekran-fazo light shaft: bulut va tog' orqasidan nur tolalari) - arzon, Intel GPU ga ham to'g'ri keladi
	if (Sun && Sun->GetComponent())
	{
		UDirectionalLightComponent* DL = Sun->GetComponent();
		DL->bEnableLightShaftBloom = true; DL->BloomScale = 0.35f; DL->BloomThreshold = 0.6f; DL->BloomMaxBrightness = 12.f; DL->BloomTint = FColor(255, 232, 200);
		DL->bEnableLightShaftOcclusion = true; DL->OcclusionMaskDarkness = 0.08f; DL->OcclusionDepthRange = 120000.f;
		DL->LightShaftOverrideDirection = FVector::ZeroVector;
		DL->MarkRenderStateDirty();
	}

	FString Ep;
	const bool bDirect = FParse::Value(FCommandLine::Get(), TEXT("-ErtEpisode="), Ep);
	const bool bFree = FParse::Param(FCommandLine::Get(), TEXT("ErtFreeRoam"));
	const bool bCut = FParse::Param(FCommandLine::Get(), TEXT("ErtCutscene"));
	if (bFree) return;
	FTimerHandle Th;
	if (bDirect) GetWorldTimerManager().SetTimer(Th, [this, Ep, bCut]() { BeginEpisode(Ep, bCut); }, 0.6f, false);
	else GetWorldTimerManager().SetTimer(Th, [this]() { OpenMenu(EErtMenu::Main); }, 0.3f, false);
}

void AErtGameMode::SetPlayerInput(bool bEnabled, bool bHide)
{
	if (AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		H->bInputEnabled = bEnabled;
		H->SetActorHiddenInGame(bHide);
	}
}

bool AErtGameMode::IsCompleted(const FString& Id) const { return Completed.Contains(Id); }

bool AErtGameMode::IsUnlocked(const FErtEpisode& E) const
{
	if (bUnlockAll || E.GlobalIndex == 0 || IsCompleted(E.Id)) return true;
	for (const FString& P : E.Prerequisites) if (!IsCompleted(P)) return false;
	return true;
}

void AErtGameMode::SpawnNpcs()
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/npcs.json")))) return;
	TSharedPtr<FJsonObject> R;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) || !R.IsValid()) return;
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (!R->TryGetArrayField(TEXT("npcs"), Arr)) return;
	int32 N = 0;
	for (const auto& V : *Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O.IsValid()) continue;
		FString Id, Name, Place, Dlg; double U = 0, Vv = 0, Yaw = 0; bool bWoman = false;
		O->TryGetStringField(TEXT("id"), Id); O->TryGetStringField(TEXT("name"), Name); O->TryGetStringField(TEXT("place"), Place); O->TryGetStringField(TEXT("dialog"), Dlg);
		O->TryGetNumberField(TEXT("u"), U); O->TryGetNumberField(TEXT("v"), Vv); O->TryGetNumberField(TEXT("yaw"), Yaw); O->TryGetBoolField(TEXT("woman"), bWoman);
		FLinearColor Kaftan(0.3f, 0.2f, 0.1f);
		const TArray<TSharedPtr<FJsonValue>>* K = nullptr;
		if (O->TryGetArrayField(TEXT("kaftan"), K) && K->Num() >= 3) Kaftan = FLinearColor((*K)[0]->AsNumber(), (*K)[1]->AsNumber(), (*K)[2]->AsNumber());
		float E = ErtMap::ObaE, Nn = ErtMap::ObaN;
		if (Place == TEXT("city")) { E = ErtMap::CityE; Nn = ErtMap::CityN; }
		else if (Place == TEXT("caravan")) { E = ErtMap::CaravanE; Nn = ErtMap::CaravanN; }
		else if (Place == TEXT("camp")) { E = ErtMap::CampE; Nn = ErtMap::CampN; }
		else if (Place == TEXT("forest")) { E = -330.f; Nn = 700.f; }
		else if (Place == TEXT("damascus")) { E = ErtMap::DamE; Nn = ErtMap::DamN; }
		else if (Place == TEXT("halab")) { E = ErtMap::HalabE; Nn = ErtMap::HalabN; }
		else if (Place == TEXT("konya")) { E = ErtMap::KonE; Nn = ErtMap::KonN; }
		else if (Place == TEXT("kayseri")) { E = ErtMap::KayE; Nn = ErtMap::KayN; }
		else if (Place == TEXT("sivas")) { E = ErtMap::SivE; Nn = ErtMap::SivN; }
		else if (Place == TEXT("erzurum")) { E = ErtMap::ErzE; Nn = ErtMap::ErzN; }
		else if (Place == TEXT("bursa")) { E = ErtMap::BurE; Nn = ErtMap::BurN; }
		else if (Place == TEXT("nikeya")) { E = ErtMap::NikE; Nn = ErtMap::NikN; }
		else if (Place == TEXT("karacahisar")) { E = ErtMap::KarE; Nn = ErtMap::KarN; }
		else if (Place == TEXT("sogut")) { E = ErtMap::SogE; Nn = ErtMap::SogN; }
		else if (Place == TEXT("domanic")) { E = ErtMap::DomE; Nn = ErtMap::DomN; }
		E += U; Nn += Vv;
		const float X = Nn * 100.f, Y = E * 100.f;
		FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtNpcGround), true);
		float Z = 2000.f;
		if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(X, Y, 60000.f), FVector(X, Y, -5000.f), ECC_Visibility, Q)) Z = Hit.ImpactPoint.Z;
		AErtNpc* Npc = GetWorld()->SpawnActor<AErtNpc>(AErtNpc::StaticClass(), FVector(X, Y, Z + 92.f), FRotator(0, Yaw, 0));
		if (Npc) { Npc->Setup(Id, Name, Dlg, bWoman, Kaftan, Yaw); ++N; }
	}
	UE_LOG(LogErtugrul, Log, TEXT("NPC: %d"), N);
}

void AErtGameMode::RefreshSideQuestFlags()
{
	for (const AErtMissionDirector::FSideInfo& S : AErtMissionDirector::LoadSideQuests())
	{
		if (Flags.Contains(TEXT("sq_done_") + S.Id) || Honor <= -10) Flags.Remove(TEXT("sq_avail_") + S.Id);   // or/iymon past bo'lsa hech kim kvest bermaydi
		else Flags.Add(TEXT("sq_avail_") + S.Id);
	// Dialog bayroqlari: honor_high / honor_low (graflarda requires_evidence sifatida)
	Flags.Remove(TEXT("honor_high")); Flags.Remove(TEXT("honor_low"));
	if (Honor >= 10) Flags.Add(TEXT("honor_high")); else if (Honor <= -5) Flags.Add(TEXT("honor_low"));
	}
}

void AErtGameMode::StartDialog(AErtNpc* Npc)
{
	if (!Npc || Dialog.IsActive() || (Cutscene && Cutscene->IsPlaying())) return;
	RefreshSideQuestFlags();
	if (!Dialog.Start(Npc->GetDialogId(), &Flags, &Honor)) return;
	Npc->SetTalking(true); TalkingNpc = Npc;
	SetPlayerInput(false, false);
}

void AErtGameMode::DialogChoose(int32 Index)
{
	if (!Dialog.IsActive()) return;
	Dialog.Choose(Index);
	if (!Dialog.IsActive()) EndDialog();
}

void AErtGameMode::EndDialog()
{
	LastDialogId = Dialog.GetId(); LastDuelPoints = Dialog.GetDuelPoints(); LastDuelThreshold = Dialog.GetDuelThreshold();
	LastDialogEndTime = GetWorld()->GetTimeSeconds();
	Dialog.End();
	if (TalkingNpc) { TalkingNpc->SetTalking(false); TalkingNpc = nullptr; }
	SaveGame();
	if (AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (Flags.Contains(TEXT("give_arrows"))) { Flags.Remove(TEXT("give_arrows")); H->AddArrows(8); }
		if (Flags.Contains(TEXT("sword_sharpened"))) { Flags.Remove(TEXT("sword_sharpened")); H->AttackDamage += 4.f; }
		auto Buy = [&](const TCHAR* Flag, int32 Price, TFunction<void()> Give)
		{
			if (!Flags.Contains(Flag)) return;
			Flags.Remove(Flag);
			Price = FMath::RoundToInt(Price * (Honor >= 10 ? 0.85f : (Honor <= -5 ? 1.3f : 1.f)));   // or/iymon narxga ta'sir qiladi
			if (H->Gold >= Price) { H->AddGold(-Price); Give(); ShopMsg = FString::Printf(TEXT("Sotib olindi (-%d oltin)"), Price); }
			else ShopMsg = TEXT("Oltin yetarli emas");
			ShopMsgT = 3.f;
		};
		// Yon kvestni boshlash
		TArray<FString> ToStart;
		for (const FString& F : Flags) if (F.StartsWith(TEXT("sq_start_"))) ToStart.Add(F);
		for (const FString& F : ToStart)
		{
			Flags.Remove(F);
			const FString Qid = F.Mid(9);
			if (Director && !Director->IsSideQuest() && (Director->GetState() == EErtMissionState::Inactive || Director->GetState() == EErtMissionState::Cleared))
			{
				if (Director->StartSideQuest(Qid)) { Flags.Remove(TEXT("sq_avail_") + Qid); bEpisodeStarted = true; }
			}
			else { ShopMsg = TEXT("Avval joriy missiyani tugating"); ShopMsgT = 3.f; }
		}
		if (Flags.Contains(TEXT("act_archery"))) { Flags.Remove(TEXT("act_archery")); StartActivity(1); }
		if (Flags.Contains(TEXT("act_wrestle"))) { Flags.Remove(TEXT("act_wrestle")); StartActivity(2); }
		if (Flags.Contains(TEXT("hayme_blessing"))) { Flags.Remove(TEXT("hayme_blessing")); H->Potions += 2; H->Heal(100.f); ShopMsg = TEXT("Onaning duosi: +2 dori, to'liq shifo"); ShopMsgT = 3.f; }
		Buy(TEXT("buy_potion"), 15, [&]() { H->Potions += 1; });
		Buy(TEXT("buy_arrows"), 10, [&]() { H->AddArrows(8); });
		Buy(TEXT("buy_shield"), 60, [&]() { H->bShield = true; H->ApplyEquipment(); });
		Buy(TEXT("buy_sword"), 120, [&]() { H->SwordTier = 2; H->ApplyEquipment(); });
		Buy(TEXT("buy_bow"), 90, [&]() { H->BowTier = 2; H->ApplyEquipment(); });
	}
	if (Menu == EErtMenu::None) SetPlayerInput(true, false);
	UE_LOG(LogErtugrul, Log, TEXT("Dialog tugadi, or/iymon: %d, bayroqlar: %d"), Honor, Flags.Num());
}

extern float GErtMouseSens; extern bool GErtInvertY;

void AErtGameMode::SaveGame()
{
	TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Comp; for (const FString& C : Completed) Comp.Add(MakeShared<FJsonValueString>(C));
	TArray<TSharedPtr<FJsonValue>> Fl; for (const FString& F : Flags) Fl.Add(MakeShared<FJsonValueString>(F));
	if (Director) for (const FString& C : Director->LoadProgressPublic()) if (!Completed.Contains(C)) { Completed.Add(C); Comp.Add(MakeShared<FJsonValueString>(C)); }
	R->SetArrayField(TEXT("completed"), Comp);
	R->SetArrayField(TEXT("flags"), Fl);
	R->SetNumberField(TEXT("honor"), Honor);
	R->SetNumberField(TEXT("language"), Language);
	R->SetNumberField(TEXT("mouse_sens"), MouseSens);
	R->SetBoolField(TEXT("invert_y"), bInvertY);
	R->SetNumberField(TEXT("gfx"), GfxPreset);
	{ FString Vs; Vs.Reserve(Visited.Num()); for (uint8 V : Visited) Vs.AppendChar(V ? TEXT('1') : TEXT('0')); R->SetStringField(TEXT("visited"), Vs); }
	{ TSharedPtr<FJsonObject> KO = MakeShared<FJsonObject>(); for (const auto& P : SavedKeys) KO->SetStringField(P.Key, P.Value); R->SetObjectField(TEXT("keys"), KO); }
	if (AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		R->SetNumberField(TEXT("meat"), H->Meat); R->SetBoolField(TEXT("pelt"), H->bPeltArmor);
		R->SetNumberField(TEXT("gold"), H->Gold); R->SetNumberField(TEXT("potions"), H->Potions); R->SetNumberField(TEXT("arrows"), H->GetArrows());
		R->SetNumberField(TEXT("level"), H->Level); R->SetNumberField(TEXT("xp"), H->XP);
		R->SetNumberField(TEXT("sword"), H->SwordTier); R->SetNumberField(TEXT("bow"), H->BowTier); R->SetBoolField(TEXT("shield"), H->bShield);
	}
	FString Out;
	const TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(R.ToSharedRef(), W);
	FFileHelper::SaveStringToFile(Out, *(FPaths::ProjectSavedDir() / TEXT("ert_save.json")));
}

void AErtGameMode::LoadGame()
{
	FString Text;
	TSharedPtr<FJsonObject> R;
	if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectSavedDir() / TEXT("ert_save.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) && R.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (R->TryGetArrayField(TEXT("completed"), A)) for (const auto& V : *A) Completed.AddUnique(V->AsString());
		if (R->TryGetArrayField(TEXT("flags"), A)) for (const auto& V : *A) Flags.Add(V->AsString());
		R->TryGetNumberField(TEXT("honor"), Honor);
		R->TryGetNumberField(TEXT("language"), Language);
		R->TryGetNumberField(TEXT("gfx"), GfxPreset);
		{ FString Vs; if (R->TryGetStringField(TEXT("visited"), Vs) && Vs.Len() == Visited.Num()) for (int32 i = 0; i < Vs.Len(); ++i) Visited[i] = Vs[i] == TEXT('1') ? 1 : 0; }
		double D = 1.0; if (R->TryGetNumberField(TEXT("mouse_sens"), D)) MouseSens = (float)D;
		R->TryGetBoolField(TEXT("invert_y"), bInvertY);
		{ const TSharedPtr<FJsonObject>* KO = nullptr; if (R->TryGetObjectField(TEXT("keys"), KO)) for (const auto& P : (*KO)->Values) SavedKeys.Add(FString(P.Key.ToView()), P.Value->AsString()); ApplySavedKeys(); }
		if (AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			int32 V = 0;
			if (R->TryGetNumberField(TEXT("gold"), V)) H->Gold = V;
			if (R->TryGetNumberField(TEXT("meat"), V)) H->Meat = V;
			{ bool Bp = false; if (R->TryGetBoolField(TEXT("pelt"), Bp)) H->bPeltArmor = Bp; }
			if (R->TryGetNumberField(TEXT("potions"), V)) H->Potions = V;
			if (R->TryGetNumberField(TEXT("arrows"), V)) H->AddArrows(V - H->GetArrows());
			if (R->TryGetNumberField(TEXT("level"), V)) { H->Level = FMath::Max(1, V); H->MaxHealth = 100.f + (H->Level - 1) * 10.f; H->StaminaMax = 100.f + (H->Level - 1) * 5.f; }
			if (R->TryGetNumberField(TEXT("xp"), V)) H->XP = V;
			if (R->TryGetNumberField(TEXT("sword"), V)) H->SwordTier = V;
			if (R->TryGetNumberField(TEXT("bow"), V)) H->BowTier = V;
			bool B = false; if (R->TryGetBoolField(TEXT("shield"), B)) H->bShield = B;
			H->ApplyEquipment();
		}
	}
	// Eski matn formati
	if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectSavedDir() / TEXT("ert_progress.txt")))) { TArray<FString> L; Text.ParseIntoArrayLines(L); for (const FString& S : L) Completed.AddUnique(S); }
	FErtLoc::Get().SetLanguage(Language);
	GErtMouseSens = MouseSens; GErtInvertY = bInvertY;
	RefreshSideQuestFlags();
}

void AErtGameMode::SetWeather(const FString& Name) { if (Weather) Weather->SetWeather(Name); }

void AErtGameMode::Rumble(float Intensity, float Seconds)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->PlayDynamicForceFeedback(Intensity, Seconds, true, true, true, true);
}

void AErtGameMode::StartActivity(int32 Kind)
{
	if (Activity != 0) return;
	Activity = Kind; ActScore = 0; WrestleP = 0.5f;
	AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Kind == 1)
	{
		ActivityT = 60.f; ActGoal = 5;
		if (H) H->AddArrows(10);
		// 3 nishon: mashq maydoni (oba u=46, v=-24) yonida, o'yinchidan 15-25 m
		for (AActor* A : ActTargets) if (A) A->Destroy();
		ActTargets.Reset();
		for (int32 i = 0; i < 3; ++i)
		{
			const float E = ErtMap::ObaE + 46.f + (i - 1) * 6.f, N = ErtMap::ObaN - 24.f + 18.f + i * 3.f;
			const float X = N * 100.f, Y = E * 100.f;
			FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtTargetGround), true);
			float Z = 2000.f; if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(X, Y, 60000.f), FVector(X, Y, -5000.f), ECC_Visibility, Q)) Z = Hit.ImpactPoint.Z;
			if (AActor* T = GetWorld()->SpawnActor<AErtTarget>(AErtTarget::StaticClass(), FVector(X, Y, Z), FRotator(0, 180.f, 0))) ActTargets.Add(T);
		}
		if (H) H->ResetAt(FVector((ErtMap::ObaN - 24.f) * 100.f, (ErtMap::ObaE + 46.f) * 100.f, 2100.f), 0.f);
	}
	else { ActivityT = 12.f; }
}

void AErtGameMode::WrestlePress()
{
	if (Activity != 2) return;
	WrestleP = FMath::Min(1.f, WrestleP + 0.06f);
	Rumble(0.3f, 0.08f);
}

void AErtGameMode::EndActivity(bool bWon)
{
	AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Activity == 1) { ActResult = bWon ? FString::Printf(TEXT("Kamon musobaqasi: %d/%d - g'alaba! +30 oltin, +60 XP, +1 or"), ActScore, ActGoal) : FString::Printf(TEXT("Kamon musobaqasi: %d/%d - keyingi safar"), ActScore, ActGoal); }
	else { ActResult = bWon ? TEXT("Kurash: Bamsini yiqitdingiz! +50 XP, +2 or") : TEXT("Kurash: Bamsi g'olib. Yana urinib ko'ring"); }
	if (bWon && H) { if (Activity == 1) { H->AddGold(30); H->AddXP(60); Honor += 1; } else { H->AddXP(50); Honor += 2; } SaveGame(); }
	ActResultT = 6.f;
	Activity = 0;
	FTimerHandle Th; GetWorldTimerManager().SetTimer(Th, [this]() { for (AActor* A : ActTargets) if (A) A->Destroy(); ActTargets.Reset(); }, 20.f, false);
}

void AErtGameMode::SetTimeOfDay(const FString& Name)
{
	if (Name == TEXT("dawn")) DayT = 0.24f;
	else if (Name == TEXT("dusk")) DayT = 0.74f;
	else if (Name == TEXT("night")) DayT = 0.95f;
	else DayT = 0.38f;
}

void AErtGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	if (Menu == EErtMenu::Map) MapInput(Dt);
	// Kashf qilingan hududlar (50 m hujayra, atrofi bilan)
	VisitT += Dt;
	if (VisitT > 0.5f)
	{
		VisitT = 0.f;
		if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const float E = P->GetActorLocation().Y / 100.f, N = P->GetActorLocation().X / 100.f;
			const int32 X = (int32)((E + 1000.f) / 50.f), Y = (int32)((N + 1000.f) / 50.f);
			for (int32 dy = -2; dy <= 2; ++dy) for (int32 dx = -2; dx <= 2; ++dx)
			{
				const int32 Xi = X + dx, Yi = Y + dy;
				if (Xi < 0 || Yi < 0 || Xi >= VisN || Yi >= VisN || dx * dx + dy * dy > 5) continue;
				if (!Visited[Yi * VisN + Xi]) { Visited[Yi * VisN + Xi] = 1; bFogDirty = true; }
			}
		}
	}
	if (bFogDirty && Menu == EErtMenu::Map) UpdateFogTex();
	if (bWaypoint) if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) if (FVector::Dist2D(P->GetActorLocation(), Waypoint) < 400.f) ClearWaypoint();   // yetib keldi
	// GPS: faol maqsadga yo'l
	if (AErtGps* G = AErtGps::Get(GetWorld()))
	{
		FVector T = FVector::ZeroVector;
		if (bWaypoint) T = Waypoint;
		else if (bGps && Director && Director->GetState() != EErtMissionState::Inactive)
		{
			// Yaqin tirik dushman bo'lsa yo'l kerak emas; aks holda birinchi marker
			TArray<FVector> Ms; Director->GetMarkers(Ms);
			if (Ms.Num()) T = Ms[0];
		}
		G->SetTarget(T);
	}
	if (bCapturing)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (PC && H)
		{
			TArray<FKey> All; EKeys::GetAllKeys(All);
			for (const FKey& K : All)
			{
				if (K.IsGamepadKey() || K.IsAxis1D() || K.IsAxis2D() || K.IsAxis3D() || K.IsTouch() || !PC->WasInputKeyJustPressed(K)) continue;
				if (K == EKeys::Escape) { bCapturing = false; break; }
				const FString Act = AErtCharacter::BindableActions()[KeyRow];
				H->SetBinding(Act, K); SavedKeys.Add(Act, K.GetFName().ToString());
				bCapturing = false; SaveGame();
				break;
			}
		}
	}
	ActResultT = FMath::Max(0.f, ActResultT - Dt);
	if (Activity != 0)
	{
		ActivityT -= Dt;
		if (Activity == 2) WrestleP = FMath::Max(0.f, WrestleP - 0.09f * Dt * (1.f + WrestleP));   // Bamsi qarshilik qiladi
		if (Activity == 1 && ActScore >= ActGoal) EndActivity(true);
		else if (Activity == 2 && WrestleP >= 1.f) EndActivity(true);
		else if (Activity == 2 && WrestleP <= 0.f) EndActivity(false);
		else if (ActivityT <= 0.f) EndActivity(false);
	}
	DayT = FMath::Fmod(DayT + Dt / DayLength, 1.f);
	ShopMsgT = FMath::Max(0.f, ShopMsgT - Dt);
	if (!Sun) return;
	// Quyosh balandligi: tongda ufqdan chiqadi, peshinda 62 gradus, shomda botadi; tunda ufq ostida (oy sifatida xira)
	const float Elev = FMath::Sin((DayT - 0.25f) * 2.f * PI) * 62.f;
	const float Yaw = 28.f + (DayT - 0.25f) * 180.f;
	Sun->SetActorRotation(FRotator(-FMath::Max(Elev, 14.f), Yaw, 0.f));   // tunda oy nuri (14 gradus)
	const float Day = FMath::Clamp((Elev + 4.f) / 16.f, 0.f, 1.f);
	const FLinearColor SunCol = FMath::Lerp(FLinearColor(0.45f, 0.55f, 0.9f), FMath::Lerp(FLinearColor(1.f, 0.55f, 0.28f), FLinearColor(1.f, 0.96f, 0.9f), FMath::Clamp(Elev / 25.f, 0.f, 1.f)), Day);
	if (UDirectionalLightComponent* DL = Sun->GetComponent())
	{
		DL->SetIntensity(FMath::Lerp(0.9f, 7.f, Day) * (1.f - 0.45f * (Weather ? (Weather->GetWeather() == TEXT("storm") || Weather->GetWeather() == TEXT("rain") ? 1.f : 0.f) : 0.f)));
		DL->SetLightColor(SunCol);
	}
	if (Weather) Weather->UpdateSky(Day, SunCol, Elev);
	if (Sky && Sky->GetLightComponent()) Sky->GetLightComponent()->SetIntensity(FMath::Lerp(0.4f, 1.f, Day));
	if (!PPV) PPV = Cast<APostProcessVolume>(UGameplayStatics::GetActorOfClass(this, APostProcessVolume::StaticClass()));
	if (PPV)
	{
		// Tunda ekspozitsiya pastroq, ranglar so'nik va ko'kimtir
		PPV->Settings.bOverride_AutoExposureBias = true; PPV->Settings.AutoExposureBias = FMath::Lerp(-0.9f, 0.3f, Day);
		PPV->Settings.bOverride_ColorSaturation = true; const float Sat = FMath::Lerp(0.55f, 1.12f, Day); PPV->Settings.ColorSaturation = FVector4(Sat, Sat, Sat, 1.f);
		PPV->Settings.bOverride_ColorGain = true; PPV->Settings.ColorGain = FVector4(FMath::Lerp(0.75f, 1.f, Day), FMath::Lerp(0.85f, 1.f, Day), 1.f, 1.f);
		if (bDream)
		{
			// Tush: binafsha-ko'k tus, past to'yinganlik, kuchli vinyetka
			PPV->Settings.ColorSaturation = FVector4(0.45f, 0.45f, 0.45f, 1.f);
			PPV->Settings.ColorGain = FVector4(0.85f, 0.65f, 1.25f, 1.f);
			PPV->Settings.bOverride_VignetteIntensity = true; PPV->Settings.VignetteIntensity = 0.8f;
			PPV->Settings.AutoExposureBias = -0.2f;
		}
		else { PPV->Settings.bOverride_VignetteIntensity = true; PPV->Settings.VignetteIntensity = 0.35f; }
	}
}

void AErtGameMode::ApplyDisplay()
{
	if (UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr) { GS->ApplySettings(false); GS->SaveSettings(); }
}

FString AErtGameMode::DisplayRow(int32 Row) const
{
	UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!GS) return TEXT("-");
	if (Row == 4)
	{
		const EWindowMode::Type M = GS->GetFullscreenMode();
		return M == EWindowMode::Fullscreen ? TEXT("To'liq ekran") : (M == EWindowMode::WindowedFullscreen ? TEXT("Oynali to'liq ekran") : TEXT("Oyna"));
	}
	if (Row == 5) { const FIntPoint R = GS->GetScreenResolution(); return FString::Printf(TEXT("%d x %d"), R.X, R.Y); }
	if (Row == 6) { static const TCHAR* Q[] = { TEXT("Past"), TEXT("O'rta"), TEXT("Yuqori"), TEXT("Epik") }; const int32 L = GS->GetOverallScalabilityLevel(); return L < 0 ? TEXT("Maxsus") : Q[FMath::Clamp(L, 0, 3)]; }
	if (Row == 7) return GS->IsVSyncEnabled() ? TEXT("Yoqilgan") : TEXT("O'chirilgan");
	return TEXT("-");
}

void AErtGameMode::SettingsToggle()
{
	if (Menu == EErtMenu::Settings) { SaveGame(); OpenMenu(Prev); }
	else if (Menu == EErtMenu::None || Menu == EErtMenu::Pause || Menu == EErtMenu::Main) { Prev = Menu == EErtMenu::None ? EErtMenu::Pause : Menu; OpenMenu(EErtMenu::Settings); }
}

void AErtGameMode::OpenMenu(EErtMenu M)
{
	Menu = M;
	const bool bPause = M != EErtMenu::None;
	UGameplayStatics::SetGamePaused(this, bPause);
	SetPlayerInput(!bPause, false);
	if (M == EErtMenu::Episodes)
	{
		const TArray<FErtEpisode>& All = UErtEpisodeDb::Get()->All();
		for (int32 i = 0; i < All.Num(); ++i) if (IsUnlocked(All[i]) && !IsCompleted(All[i].Id)) { MenuIndex = i; break; }
	}
}

void AErtGameMode::ToggleMap()
{
	if (Dialog.IsActive() || (Cutscene && Cutscene->IsPlaying())) return;
	if (Menu == EErtMenu::Map) OpenMenu(EErtMenu::None);
	else if (Menu == EErtMenu::None || Menu == EErtMenu::Pause) OpenMenu(EErtMenu::Map);
	if (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->SetActive(Menu == EErtMenu::Map);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		const bool bMap = Menu == EErtMenu::Map;
		PC->bShowMouseCursor = bMap; PC->bEnableClickEvents = bMap;
		if (bMap) { FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); PC->SetInputMode(Mode); }
		else PC->SetInputMode(FInputModeGameOnly());
		bDragging = false;
	}
}

void AErtGameMode::MapInput(float Dt)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	AErtMap3D* M3 = AErtMap3D::Get(GetWorld());
	if (!PC || !M3) return;
	const float S = MapRect.Z;
	float MX = 0.f, MY = 0.f; const bool bHasMouse = PC->GetMousePosition(MX, MY);
	MapMouse = FVector2D(-1, -1);
	if (bHasMouse && S > 1.f && MX >= MapRect.X && MX <= MapRect.X + S && MY >= MapRect.Y && MY <= MapRect.Y + S) MapMouse = FVector2D((MX - MapRect.X) / S, (MY - MapRect.Y) / S);
	// Surish: chap tugma bilan sudrash yoki WASD
	if (PC->IsInputKeyDown(EKeys::LeftMouseButton) && bHasMouse)
	{
		if (bDragging) M3->PanPixels(MX - LastMouse.X, MY - LastMouse.Y, S);
		bDragging = true; LastMouse = FVector2D(MX, MY);
	}
	else bDragging = false;
	const float PanPx = 420.f * Dt;
	if (PC->IsInputKeyDown(EKeys::W)) M3->PanPixels(0.f, PanPx, S);
	if (PC->IsInputKeyDown(EKeys::S)) M3->PanPixels(0.f, -PanPx, S);
	if (PC->IsInputKeyDown(EKeys::A)) M3->PanPixels(PanPx, 0.f, S);
	if (PC->IsInputKeyDown(EKeys::D)) M3->PanPixels(-PanPx, 0.f, S);
	if (PC->IsInputKeyDown(EKeys::Q)) M3->Rotate(-70.f * Dt);
	if (PC->IsInputKeyDown(EKeys::E)) M3->Rotate(70.f * Dt);
	if (PC->IsInputKeyDown(EKeys::Z)) M3->Tilt(-40.f * Dt);
	if (PC->IsInputKeyDown(EKeys::X)) M3->Tilt(40.f * Dt);
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp) || PC->WasInputKeyJustPressed(EKeys::Equals) || PC->WasInputKeyJustPressed(EKeys::Add)) M3->Zoom(0.8f);
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown) || PC->WasInputKeyJustPressed(EKeys::Hyphen) || PC->WasInputKeyJustPressed(EKeys::Subtract)) M3->Zoom(1.25f);
	if (PC->WasInputKeyJustPressed(EKeys::R)) { if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) M3->SetCenter(FVector2D(P->GetActorLocation().X, P->GetActorLocation().Y)); }
	// Yo'l nuqtasi: o'ng tugma (mavjud nuqta yonida - o'chirish), Delete - o'chirish
	if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton) && MapMouse.X >= 0.f)
	{
		const FVector Wp = M3->Unproject(MapMouse.X, MapMouse.Y);
		if (bWaypoint && FVector::Dist2D(Wp, Waypoint) < 3000.f) ClearWaypoint(); else SetWaypoint(Wp);
	}
	if (PC->WasInputKeyJustPressed(EKeys::Delete) || PC->WasInputKeyJustPressed(EKeys::BackSpace)) ClearWaypoint();
}

void AErtGameMode::SetDream(bool bOn)
{
	if (bOn == bDream) return;
	bDream = bOn;
	if (bOn) { SavedDayT = DayT; SavedWeather = GetWeatherName(); DayT = 0.97f; SetWeather(TEXT("fog")); }
	else { DayT = SavedDayT; SetWeather(SavedWeather.IsEmpty() ? TEXT("clear") : SavedWeather); }
	UE_LOG(LogErtugrul, Log, TEXT("Tush rejimi: %s"), bOn ? TEXT("yoqildi") : TEXT("o'chdi"));
}

const FString& AErtGameMode::GetWeatherName() const
{
	static FString Empty;
	return Weather ? Weather->GetWeather() : Empty;
}

void AErtGameMode::MapRotate(float DeltaYaw) { if (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->Rotate(DeltaYaw); }

void AErtGameMode::ApplyGfxPreset()
{
	// Konsol o'zgaruvchilari: preset jadvali (docs/UNREAL_PORT.md)
	auto Cmd = [&](const TCHAR* C) { if (GEngine) GEngine->Exec(GetWorld(), C); };
	switch (GfxPreset)
	{
	case 0:   // PC Ultra: Lumen (Hardware RT), Virtual Shadow Maps High, Lumen RT aks, 100% + TSR (DLSS plagin bo'lsa o'zi almashadi)
		Cmd(TEXT("r.DynamicGlobalIlluminationMethod 1")); Cmd(TEXT("r.Lumen.HardwareRayTracing 1")); Cmd(TEXT("r.ReflectionMethod 1")); Cmd(TEXT("r.Lumen.Reflections.HardwareRayTracing 1"));
		Cmd(TEXT("r.Shadow.Virtual.Enable 1")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional 0")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal 0"));
		Cmd(TEXT("r.AntiAliasingMethod 4")); Cmd(TEXT("r.DynamicRes.OperationMode 0")); Cmd(TEXT("r.ScreenPercentage 100")); Cmd(TEXT("sg.GlobalIlluminationQuality 3")); Cmd(TEXT("sg.ShadowQuality 3")); Cmd(TEXT("sg.ReflectionQuality 3")); Cmd(TEXT("sg.FoliageQuality 3")); Cmd(TEXT("sg.ViewDistanceQuality 3"));
		break;
	case 1:   // PS5 / Series X 60 FPS: Lumen Software, VSM Medium, Lumen SW aks, dinamik 50-70% TSR
		Cmd(TEXT("r.DynamicGlobalIlluminationMethod 1")); Cmd(TEXT("r.Lumen.HardwareRayTracing 0")); Cmd(TEXT("r.ReflectionMethod 1")); Cmd(TEXT("r.Lumen.Reflections.HardwareRayTracing 0"));
		Cmd(TEXT("r.Shadow.Virtual.Enable 1")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional 1")); Cmd(TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal 1"));
		Cmd(TEXT("r.AntiAliasingMethod 4")); Cmd(TEXT("r.DynamicRes.OperationMode 2")); Cmd(TEXT("r.DynamicRes.MinScreenPercentage 50")); Cmd(TEXT("r.DynamicRes.MaxScreenPercentage 70")); Cmd(TEXT("r.DynamicRes.FrameTimeBudget 16.6")); Cmd(TEXT("sg.GlobalIlluminationQuality 2")); Cmd(TEXT("sg.ShadowQuality 2")); Cmd(TEXT("sg.ReflectionQuality 2")); Cmd(TEXT("sg.FoliageQuality 2")); Cmd(TEXT("sg.ViewDistanceQuality 2"));
		break;
	default:  // Series S 60 FPS: SSGI (Distance Field/Screen Space), Cascaded Shadows, SSR, dinamik 50-60% TSR
		Cmd(TEXT("r.DynamicGlobalIlluminationMethod 2")); Cmd(TEXT("r.Lumen.HardwareRayTracing 0")); Cmd(TEXT("r.ReflectionMethod 2")); Cmd(TEXT("r.SSR.Quality 2"));
		Cmd(TEXT("r.Shadow.Virtual.Enable 0")); Cmd(TEXT("r.Shadow.CSM.MaxCascades 3"));
		Cmd(TEXT("r.AntiAliasingMethod 4")); Cmd(TEXT("r.DynamicRes.OperationMode 2")); Cmd(TEXT("r.DynamicRes.MinScreenPercentage 50")); Cmd(TEXT("r.DynamicRes.MaxScreenPercentage 60")); Cmd(TEXT("r.DynamicRes.FrameTimeBudget 16.6")); Cmd(TEXT("sg.GlobalIlluminationQuality 1")); Cmd(TEXT("sg.ShadowQuality 1")); Cmd(TEXT("sg.ReflectionQuality 1")); Cmd(TEXT("sg.FoliageQuality 1")); Cmd(TEXT("sg.ViewDistanceQuality 2"));
		break;
	}
	UE_LOG(LogErtugrul, Log, TEXT("Grafika preseti: %s"), GfxPresetName(GfxPreset));
}

void AErtGameMode::ToggleInventory()
{
	if (Dialog.IsActive() || (Cutscene && Cutscene->IsPlaying())) return;
	if (Menu == EErtMenu::Inventory) OpenMenu(EErtMenu::None);
	else if (Menu == EErtMenu::None || Menu == EErtMenu::Pause) OpenMenu(EErtMenu::Inventory);
}

void AErtGameMode::HitStop(float Seconds, float Dilation)
{
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
	FTimerHandle Th;
	GetWorldTimerManager().SetTimer(Th, [this]() { UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }, Seconds * Dilation, false);
}

void AErtGameMode::SettingsMove(int32 Delta)
{
	if (bCapturing) return;
	if (SettingsPage == 1) { const int32 N = AErtCharacter::BindableActions().Num() + 1; KeyRow = (KeyRow + Delta + N) % N; return; }
	SettingsRow = (SettingsRow + Delta + 9) % 9;
}

void AErtGameMode::ApplySavedKeys()
{
	AErtCharacter* H = Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!H) return;
	for (const auto& P : SavedKeys) { const FKey K(*P.Value); if (K.IsValid()) H->Bindings.Add(P.Key, K); }
	H->ApplyBindings();
}

void AErtGameMode::SettingsAdjust(int32 Delta)
{
	if (bCapturing) return;
	if (SettingsPage == 1)
	{
		const TArray<FString>& Acts = AErtCharacter::BindableActions();
		if (KeyRow >= Acts.Num()) { SettingsPage = 0; return; }   // "Orqaga"
		bCapturing = true;                                       // keyingi bosilgan tugma
		return;
	}
	if (SettingsRow == 3) { SettingsPage = 1; KeyRow = 0; return; }
	if (SettingsRow == 8) { GfxPreset = (GfxPreset + Delta + 3) % 3; ApplyGfxPreset(); return; }
	UGameUserSettings* GS = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (SettingsRow >= 4 && GS)
	{
		if (SettingsRow == 4)
		{
			const EWindowMode::Type M = GS->GetFullscreenMode();
			const int32 Cur = M == EWindowMode::Fullscreen ? 0 : (M == EWindowMode::WindowedFullscreen ? 1 : 2);
			const int32 N = (Cur + Delta + 3) % 3;
			GS->SetFullscreenMode(N == 0 ? EWindowMode::Fullscreen : (N == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed));
		}
		else if (SettingsRow == 5)
		{
			ResIndex = (ResIndex + Delta + 4) % 4;
			static const FIntPoint Res[] = { FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440) };
			GS->SetScreenResolution(Res[ResIndex]);
		}
		else if (SettingsRow == 6)
		{
			const int32 Q = FMath::Clamp(GS->GetOverallScalabilityLevel(), 0, 3);
			GS->SetOverallScalabilityLevel((Q + Delta + 4) % 4);
		}
		else if (SettingsRow == 7) GS->SetVSyncEnabled(!GS->IsVSyncEnabled());
		ApplyDisplay();
		return;
	}
	if (SettingsRow == 0) { Language = (Language + Delta + 3) % 3; FErtLoc::Get().SetLanguage(Language); }
	else if (SettingsRow == 1) { MouseSens = FMath::Clamp(MouseSens + Delta * 0.1f, 0.2f, 3.f); GErtMouseSens = MouseSens; }
	else { bInvertY = !bInvertY; GErtInvertY = bInvertY; }
}

void AErtGameMode::MenuToggle()
{
	if (Cutscene && Cutscene->IsPlaying()) return;
	if (Dialog.IsActive()) { EndDialog(); return; }
	switch (Menu)
	{
	case EErtMenu::None: OpenMenu(bEpisodeStarted ? EErtMenu::Pause : EErtMenu::Main); break;
	case EErtMenu::Main: break;
	case EErtMenu::Pause: OpenMenu(EErtMenu::None); break;
	case EErtMenu::Map: case EErtMenu::Inventory: OpenMenu(EErtMenu::None); break;
	case EErtMenu::Settings: if (bCapturing) { bCapturing = false; break; } if (SettingsPage == 1) { SettingsPage = 0; break; } SaveGame(); OpenMenu(Prev == EErtMenu::None ? EErtMenu::Pause : Prev); break;
	case EErtMenu::Episodes: OpenMenu(Prev == EErtMenu::None ? (bEpisodeStarted ? EErtMenu::Pause : EErtMenu::Main) : Prev); break;
	}
}

void AErtGameMode::MenuMove(int32 Delta)
{
	if (Dialog.IsActive()) { Dialog.MoveSelection(Delta); return; }
	if (Menu == EErtMenu::Map) { if (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->Zoom(Delta < 0 ? 0.8f : 1.25f); return; }
	if (Menu == EErtMenu::Settings) { SettingsMove(Delta); return; }
	if (Menu == EErtMenu::Main) { RowMain = (RowMain + Delta + 4) % 4; return; }
	if (Menu == EErtMenu::Pause) { RowPause = (RowPause + Delta + 5) % 5; return; }
	if (Menu != EErtMenu::Episodes) return;
	const int32 N = UErtEpisodeDb::Get()->All().Num();
	if (N == 0) return;
	MenuIndex = (MenuIndex + Delta + N) % N;
}

void AErtGameMode::MenuConfirm()
{
	if (Dialog.IsActive()) { OnAdvance(); return; }
	if (Menu == EErtMenu::Settings) { SettingsAdjust(1); return; }
	if (Menu == EErtMenu::Map || Menu == EErtMenu::Inventory) { OpenMenu(EErtMenu::None); return; }
	const TArray<FErtEpisode>& All = UErtEpisodeDb::Get()->All();
	if (Menu == EErtMenu::Main)
	{
		switch (RowMain)
		{
		case 0: // Boshlash / davom etish: birinchi ochiq va bajarilmagan epizod
			for (int32 i = 0; i < All.Num(); ++i) if (IsUnlocked(All[i]) && !IsCompleted(All[i].Id)) { OpenMenu(EErtMenu::None); BeginEpisode(All[i].Id, true); return; }
			if (All.Num()) { OpenMenu(EErtMenu::None); BeginEpisode(All[0].Id, true); }
			return;
		case 1: Prev = EErtMenu::Main; OpenMenu(EErtMenu::Episodes); return;
		case 2: Prev = EErtMenu::Main; OpenMenu(EErtMenu::Settings); return;
		default: UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false); return;
		}
	}
	if (Menu == EErtMenu::Pause)
	{
		switch (RowPause)
		{
		case 0: OpenMenu(EErtMenu::None); return;
		case 1: OpenMenu(EErtMenu::Map); return;
		case 2: Prev = EErtMenu::Pause; OpenMenu(EErtMenu::Episodes); return;
		case 3: Prev = EErtMenu::Pause; OpenMenu(EErtMenu::Settings); return;
		default: SaveGame(); UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false); return;
		}
	}
	if (Menu != EErtMenu::Episodes) return;
	if (!All.IsValidIndex(MenuIndex) || !IsUnlocked(All[MenuIndex])) return;
	OpenMenu(EErtMenu::None);
	BeginEpisode(All[MenuIndex].Id, true);
}

void AErtGameMode::BeginEpisode(const FString& Id, bool bWithCutscene)
{
	if (!Director) return;
	bEpisodeStarted = true;
	const FString EpId = Id;
	if (bWithCutscene && Cutscene)
	{
		Director->StopEpisode();
		SetPlayerInput(false, true);
		const FVector Origin = Director->GetAnchor(EpId);
		Cutscene->Play(EpId, Origin, [this, EpId]()
		{
			SetPlayerInput(true, false);
			if (Director) Director->StartEpisode(EpId);
		});
	}
	else
	{
		SetPlayerInput(true, false);
		Director->StartEpisode(EpId);
	}
}

void AErtGameMode::OnAdvance()
{
	if (Dialog.IsActive()) { Dialog.Advance(); if (!Dialog.IsActive()) EndDialog(); return; }
	if (Cutscene && Cutscene->IsPlaying()) { Cutscene->Advance(); return; }
	if (Director && Director->GetState() == EErtMissionState::Cleared && Director->GetStateTime() > 1.5f) Director->SkipCleared();
}

void AErtGameMode::OnSkip()
{
	if (Dialog.IsActive()) { EndDialog(); return; }
	if (Cutscene && Cutscene->IsPlaying()) { Cutscene->Skip(); return; }
	MenuToggle();
}


void AErtGameMode::UpdateFogTex()
{
	bFogDirty = false;
	if (!FogTex)
	{
		FogTex = UTexture2D::CreateTransient(VisN, VisN, PF_B8G8R8A8);
		FogTex->Filter = TF_Bilinear;
		FogTex->SRGB = false;
		FogTex->AddToRoot();
	}
	FTexture2DMipMap& Mip = FogTex->GetPlatformData()->Mips[0];
	uint8* Data = (uint8*)Mip.BulkData.Lock(LOCK_READ_WRITE);
	for (int32 y = 0; y < VisN; ++y)
		for (int32 x = 0; x < VisN; ++x)
		{
			// Yumshoq chekka: qo'shni kashf qilingan hujayralar bo'lsa alfa kamayadi
			float A = Visited[y * VisN + x] ? 0.f : 1.f;
			if (A > 0.f)
			{
				int32 Nb = 0;
				for (int32 dy = -1; dy <= 1; ++dy) for (int32 dx = -1; dx <= 1; ++dx) { const int32 X2 = x + dx, Y2 = y + dy; if (X2 >= 0 && Y2 >= 0 && X2 < VisN && Y2 < VisN && Visited[Y2 * VisN + X2]) ++Nb; }
				A = FMath::Max(0.35f, 1.f - Nb * 0.12f);
			}
			uint8* P = Data + (y * VisN + x) * 4;
			P[0] = 12; P[1] = 12; P[2] = 18; P[3] = (uint8)(A * 255.f);
		}
	Mip.BulkData.Unlock();
	FogTex->UpdateResource();
}
