#include "ErtMission.h"
#include "Ertugrul.h"
#include "ErtCharacter.h"
#include "ErtEpisodeDb.h"
#include "ErtLoc.h"
#include "ErtProcMesh.h"
#include "ErtWorldBuilder.h"
#include "ErtHorse.h"
#include "ErtNpc.h"
#include "ErtGameMode.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

using namespace ErtMap;

// ---------------- Objective ----------------

FString FErtObjective::Text() const
{
	FString Base = FErtLoc::Get().Tr(LocKey);
	switch (Kind)
	{
	case EErtObjKind::DefeatAll: if (Target > 0) Base += FString::Printf(TEXT("  %d/%d"), Progress, Target); break;
	case EErtObjKind::SurviveTime: case EErtObjKind::HoldPoint: case EErtObjKind::Timer:
		Base += FString::Printf(TEXT("  %d s"), FMath::Max(0, Target - Progress)); break;
	case EErtObjKind::Route: Base += FString::Printf(TEXT("  %d/%d"), Progress, Points.Num()); break;
	case EErtObjKind::Hunt: case EErtObjKind::Collect: Base += FString::Printf(TEXT("  %d/%d"), Progress, Target); break;
	case EErtObjKind::Council: break;
	default: break;
	}
	if (bFailed) Base += TEXT("  (") + FErtLoc::Get().Tr(TEXT("ui.obj.failed")) + TEXT(")");
	return Base;
}

// ---------------- Director ----------------

AErtMissionDirector::AErtMissionDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AErtMissionDirector::BeginPlay()
{
	Super::BeginPlay();
	World = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(this, AErtWorldBuilder::StaticClass()));
	MarkerMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ertugrul/Materials/M_ErtVertexColor.M_ErtVertexColor"));
	MarkerMesh = NewObject<UProceduralMeshComponent>(this, TEXT("Markers"));
	MarkerMesh->SetupAttachment(RootComponent);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetCastShadow(false);
	MarkerMesh->RegisterComponent();
	if (MarkerMat) MarkerMesh->SetMaterial(0, MarkerMat);
}

AErtCharacter* AErtMissionDirector::Hero() const
{
	return Cast<AErtCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

int32 AErtMissionDirector::AliveEnemies() const
{
	int32 N = 0;
	for (const AErtEnemy* E : Enemies) if (E && !E->IsDead() && !E->IsAnimal()) ++N;
	return N;
}

void AErtMissionDirector::GetMarkers(TArray<FVector>& Out) const
{
	Out.Reset();
	if (State != EErtMissionState::Fighting && State != EErtMissionState::WaveCleared && State != EErtMissionState::Briefing) return;
	for (const FErtObjective& O : Objectives)
	{
		if (O.bDone) continue;
		switch (O.Kind)
		{
		case EErtObjKind::Route: if (O.Progress < O.Points.Num()) Out.Add(O.Points[O.Progress]); break;
		case EErtObjKind::Collect: for (int32 i = 0; i < O.Points.Num(); ++i) if (!O.Collected[i]) Out.Add(O.Points[i]); break;
		case EErtObjKind::HoldPoint: case EErtObjKind::Council: Out.Add(O.Point); break;
		default: break;
		}
	}
}

// ---------------- Joy tanlash (kolliziya bilan: tomlar, qoyalar) ----------------

FVector AErtMissionDirector::GroundAt(float X, float Y, bool* bOnProp) const
{
	FHitResult H;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtGround), true);
	const FVector A(X, Y, 60000.f), B(X, Y, -5000.f);
	if (GetWorld()->LineTraceSingleByChannel(H, A, B, ECC_Visibility, Q))
	{
		if (bOnProp && World)
		{
			const float TerrainZ = World->HeightAt(Y / 100.f, X / 100.f) * 100.f;
			*bOnProp = (H.ImpactPoint.Z > TerrainZ + 150.f) && H.ImpactNormal.Z > 0.75f;
		}
		return H.ImpactPoint;
	}
	if (bOnProp) *bOnProp = false;
	const float Z = World ? World->HeightAt(Y / 100.f, X / 100.f) * 100.f : 0.f;
	return FVector(X, Y, Z);
}

FVector AErtMissionDirector::FindSpot(const FVector& Around, float MinR, float MaxR) const
{
	FVector Best = Around;
	for (int32 i = 0; i < 24; ++i)
	{
		const float A = Rng.FRandRange(0.f, 2.f * PI), R = Rng.FRandRange(MinR, MaxR);
		const FVector P2(Around.X + FMath::Cos(A) * R, Around.Y + FMath::Sin(A) * R, 0);
		if (FMath::Abs(P2.X) > 95000.f || FMath::Abs(P2.Y) > 95000.f) continue;
		FHitResult H;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ErtSpot), true);
		if (!GetWorld()->LineTraceSingleByChannel(H, FVector(P2.X, P2.Y, 60000.f), FVector(P2.X, P2.Y, -5000.f), ECC_Visibility, Q)) continue;
		if (H.ImpactNormal.Z < 0.72f) continue;                       // tik qiyalik
		if (H.ImpactPoint.Z < WaterZ * 100.f + 80.f) continue;        // suv
		// Kapsula uchun bo'sh joy
		if (GetWorld()->OverlapBlockingTestByChannel(H.ImpactPoint + FVector(0, 0, 100.f), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(40.f, 90.f), Q)) continue;
		return H.ImpactPoint;
	}
	return GroundAt(Best.X, Best.Y);
}

FVector AErtMissionDirector::FindElevated(const FVector& Around, float MinR, float MaxR, bool& bElevated) const
{
	bElevated = false;
	for (int32 i = 0; i < 40; ++i)
	{
		const float A = Rng.FRandRange(0.f, 2.f * PI), R = Rng.FRandRange(MinR, MaxR);
		const float X = Around.X + FMath::Cos(A) * R, Y = Around.Y + FMath::Sin(A) * R;
		bool bProp = false;
		const FVector P = GroundAt(X, Y, &bProp);
		if (bProp) { bElevated = true; return P; }
	}
	return FindSpot(Around, MinR, MaxR);
}

FVector AErtMissionDirector::AnchorFor(const FErtEpisode& E) const
{
	// Reja koordinatalari (E, N) metrda -> dunyo
	const FString& A = E.Archetype;
	float PE = ObaE, PN = ObaN - 210.f;                       // oba janubidagi maydon
	if (A == TEXT("SIEGE"))              { PE = FortE - 90.f;  PN = FortN - 170.f; }   // qal'a yonbag'ri
	else if (A == TEXT("DEFENSE"))       { PE = ObaE + 40.f;   PN = ObaN - 200.f; }
	else if (A == TEXT("SURVIVAL"))      { PE = -330.f;        PN = 700.f; }           // o'rmon
	else if (A == TEXT("INFILTRATION"))  { PE = CampE - 240.f; PN = CampN + 20.f; }    // lager g'arbi
	else if (A == TEXT("CHASE"))         { PE = CrossE;        PN = CrossN + 30.f; }
	else if (A == TEXT("ESCORT") && E.bCamel) { PE = CaravanE - 60.f; PN = CaravanN + 40.f; }   // karvonsaroy (tuya)
	else if (A == TEXT("ESCORT"))        { PE = -640.f;        PN = 120.f; }           // daryo yo'li
	else if (A == TEXT("RITUAL"))        { PE = -250.f;        PN = 520.f; }
	else if (A == TEXT("COURT"))         { PE = CityE;         PN = CityN + 250.f; }   // shahar shimoli
	// Sham / Halab / Damashq epizodlari - Damashq shimoliy darvozasi oldi
	if (E.Region.Contains(TEXT("Damashq")) || E.Region.Contains(TEXT("Sham"))) { PE = DamE; PN = DamN - DamHalfN - 40.f; }
	else if (E.Region.Contains(TEXT("Halab"))) { PE = HalabE + HalabR + 45.f; PN = HalabN; }   // Halab sharqiy darvozasi oldi
	else if (E.Region.Contains(TEXT("Konya")) || E.Region.Contains(TEXT("Kubadabad"))) { PE = KonE + KonR + 45.f; PN = KonN; }   // Konya sharqiy darvozasi oldi
	else if (E.Region.Contains(TEXT("Kayseri"))) { PE = KayE - KayR - 45.f; PN = KayN; }   // Qayseri g'arbiy darvozasi oldi
	else if (E.Region.Contains(TEXT("Sivas"))) { PE = SivE; PN = SivN - SivR - 45.f; }   // Sivas janubiy darvozasi oldi
	else if (E.Region.Contains(TEXT("Erzurum")) || E.Region.Contains(TEXT("Erzincan"))) { PE = ErzE; PN = ErzN - ErzR - 45.f; }   // Erzurum janubiy darvozasi oldi
	else if (E.Region.Contains(TEXT("Bursa"))) { PE = BurE + BurR + 45.f; PN = BurN; }   // Bursa sharqiy kirishi
	else if (E.Region.Contains(TEXT("Nikeya"))) { PE = NikE + NikR + 55.f; PN = NikN; }   // Nikeya sharqiy darvozasi oldi
	else if (E.Region.Contains(TEXT("Karacahisar"))) { PE = KarE; PN = KarN - KarR - 60.f; }   // Karacahisar etagi (janub)
	else if (E.Region.Contains(TEXT("So'g'ut")) || E.Region.Contains(TEXT("Sogut"))) { PE = SogE + SogR + 20.f; PN = SogN; }   // So'g'ut sharqiy kirishi
	else if (E.Region.Contains(TEXT("Domani"))) { PE = DomE + 40.f; PN = DomN + 50.f; }   // Domaniç yaylovi (yo'l boshi)
	// Epizod indeksiga qarab biroz siljitamiz - har epizod boshqa joyda
	const float Off = (E.GlobalIndex % 5) * 35.f;
	return GroundAt(PN * 100.f + Off * 100.f, PE * 100.f - Off * 60.f);
}

FVector AErtMissionDirector::GetAnchor(const FString& InEpisodeId) const
{
	const FErtEpisode* E = UErtEpisodeDb::Get()->ById(InEpisodeId);
	return E ? AnchorFor(*E) : GroundAt(0.f, 0.f);
}

// ---------------- Epizod ----------------

bool AErtMissionDirector::StartEpisode(const FString& Id)
{
	const FErtEpisode* E = UErtEpisodeDb::Get()->ById(Id);
	if (!E) { UE_LOG(LogErtugrul, Warning, TEXT("Epizod topilmadi: %s"), *Id); return false; }
	return StartEpisodeData(*E, AnchorFor(*E), nullptr);
}

TArray<AErtMissionDirector::FSideInfo> AErtMissionDirector::LoadSideQuests()
{
	TArray<FSideInfo> Out;
	FString Text; TSharedPtr<FJsonObject> R;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/sidequests.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) || !R.IsValid()) return Out;
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (!R->TryGetArrayField(TEXT("quests"), Arr)) return Out;
	for (const auto& V : *Arr)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject(); if (!O.IsValid()) continue;
		FSideInfo S;
		O->TryGetStringField(TEXT("id"), S.Id); O->TryGetStringField(TEXT("giver"), S.Giver); O->TryGetStringField(TEXT("title"), S.TitleKey); O->TryGetStringField(TEXT("done_key"), S.DoneKey);
		O->TryGetNumberField(TEXT("xp"), S.XP); O->TryGetNumberField(TEXT("gold"), S.Gold); O->TryGetNumberField(TEXT("honor"), S.Honor);
		O->TryGetStringField(TEXT("reward"), S.Reward);
		Out.Add(S);
	}
	return Out;
}

bool AErtMissionDirector::StartSideQuest(const FString& QuestId)
{
	FString Text; TSharedPtr<FJsonObject> R;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/sidequests.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) || !R.IsValid()) return false;
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (!R->TryGetArrayField(TEXT("quests"), Arr)) return false;
	TSharedPtr<FJsonObject> Found;
	for (const auto& V : *Arr) { const TSharedPtr<FJsonObject> O = V->AsObject(); FString Id; if (O.IsValid() && O->TryGetStringField(TEXT("id"), Id) && Id == QuestId) { Found = O; break; } }
	if (!Found.IsValid()) return false;
	AErtCharacter* H = Hero(); if (!H) return false;
	FErtEpisode E; E.Id = QuestId; E.Archetype = TEXT("SIDE"); E.DifficultyTier = 2; E.MaxSimultaneous = 3;
	Found->TryGetStringField(TEXT("title"), E.LocTitle);
	Side = FSideInfo(); Side.Id = QuestId;
	Found->TryGetStringField(TEXT("giver"), Side.Giver); Found->TryGetStringField(TEXT("done_key"), Side.DoneKey); Side.TitleKey = E.LocTitle;
	Found->TryGetNumberField(TEXT("xp"), Side.XP); Found->TryGetNumberField(TEXT("gold"), Side.Gold); Found->TryGetNumberField(TEXT("honor"), Side.Honor);
	Found->TryGetStringField(TEXT("reward"), Side.Reward);
	const bool bOk = StartEpisodeData(E, GroundAt(H->GetActorLocation().X, H->GetActorLocation().Y), Found);
	bSideQuest = bOk;
	if (bOk) EpisodeTitle = FErtLoc::Get().Tr(TEXT("ui.hud.sidequest")) + TEXT(": ") + FErtLoc::Get().TrOr(E.LocTitle, QuestId);
	return bOk;
}

bool AErtMissionDirector::StartEpisodeData(const FErtEpisode& EData, const FVector& Start, const TSharedPtr<FJsonObject>& PhaseOverride)
{
	const FErtEpisode* E = &EData;
	AErtCharacter* H = Hero();
	if (!H) return false;
	StopEpisode();
	bSideQuest = false;
	PhaseOverrideObj = PhaseOverride;
	EpisodeId = E->Id;
	EpisodeTitle = UErtEpisodeDb::Get()->Title(*E);
	EpisodeDate = E->Gregorian;
	IntroText = FErtLoc::Get().TrOr(E->LocIntro, TEXT(""));
	Cliffhanger = FErtLoc::Get().TrOr(E->LocCliffhanger, TEXT(""));
	if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->SetTimeOfDay(E->TimeOfDay); GM->SetWeather(E->Weather); }
	NextEpisodeId = E->NextId;
	Kills = 0; Deaths = 0;
	Rng.Initialize(GetTypeHash(E->Id));

	if (!PhaseOverride.IsValid()) H->ResetAt(Start + FVector(0, 0, 100.f), 0.f);
	Cursor = Start;
	if (E->bHorse)
	{
		// Ot minish epizodi: ot boshlanish nuqtasi yonida kutadi
		FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FVector HP = FindSpot(Start, 350.f, 600.f);
		if (Horse) Horse->Destroy();
		Horse = GetWorld()->SpawnActor<AErtHorse>(AErtHorse::StaticClass(), HP + FVector(0, 0, 130.f), FRotator(0, (Start - HP).Rotation().Yaw, 0), SP);
		if (Horse) Horse->Init(E->bCamel ? FLinearColor(0.72f, 0.58f, 0.36f) : FLinearColor(0.36f, 0.22f, 0.11f), E->bCamel);
	}
	BuildPhases(*E);
	int32 StartIdx = 0;
	FParse::Value(FCommandLine::Get(), TEXT("-ErtPhase="), StartIdx);   // sinov: bosqichdan boshlash
	StartPhase(FMath::Clamp(StartIdx, 0, Phases.Num() - 1));
	State = EErtMissionState::Briefing;
	StateT = 0.f;
	UE_LOG(LogErtugrul, Log, TEXT("[Missiya] %s '%s' (%s, tier %d): %d bosqich, boshlanish %s"),
		*E->Id, *EpisodeTitle, *E->Archetype, E->DifficultyTier, Phases.Num(), *Start.ToCompactString());
	return true;
}

void AErtMissionDirector::StopEpisode()
{
	ClearEnemies();
	ClearPhaseNpcs();
	Phases.Reset(); Objectives.Reset(); Waves.Reset();
	State = EErtMissionState::Inactive;
	Cp.bValid = false;
	RebuildMarkers();
}

void AErtMissionDirector::ClearPhaseNpcs()
{
	for (AErtNpc* N : PhaseNpcs) if (N) N->Destroy();
	PhaseNpcs.Reset();
}

void AErtMissionDirector::ClearEnemies()
{
	for (AErtEnemy* E : Enemies) if (E) { if (AErtHorse* Hs = E->GetMount()) Hs->Destroy(); E->Destroy(); }
	Enemies.Reset();
}

void AErtMissionDirector::BuildPhases(const FErtEpisode& E)
{
	Phases.Reset();
	const int32 Tier = FMath::Clamp(E.DifficultyTier, 1, 5);
	const int32 SeasonN = (E.SeasonId.Len() >= 2) ? FMath::Clamp(E.SeasonId[1] - '0', 1, 4) : 1;
	const FString A_Arch = E.Archetype;
	int32 PerWave = E.MaxSimultaneous > 0 ? E.MaxSimultaneous : 3;
	PerWave = FMath::Clamp(PerWave, 2, 6);

	auto PickKind = [&]() -> EErtEnemyKind
	{
		const float R = Rng.FRand();
		if (SeasonN >= 3 && Tier >= 3 && R > 0.85f) return EErtEnemyKind::Elite;
		if (Tier >= 2 && R > 0.86f) return EErtEnemyKind::Rider;
		if ((A_Arch == TEXT("CHASE") || A_Arch == TEXT("ESCORT")) && R > 0.70f) return EErtEnemyKind::Rider;
		if (Tier >= 2 && R > 0.80f) return EErtEnemyKind::Crossbow;
		if (R > 0.62f) return EErtEnemyKind::Sergeant;
		return EErtEnemyKind::Footman;
	};
	auto MakeWave = [&](int32 N, float Delay, bool bPatrol)
	{
		FErtWave W; W.Delay = Delay;
		for (int32 i = 0; i < N; ++i)
		{
			FErtSpawn S; S.Kind = PickKind();
			const float MinR = S.Kind == EErtEnemyKind::Crossbow ? 1800.f : (S.Kind == EErtEnemyKind::Rider ? 2600.f : 1300.f);
			S.Pos = FindSpot(Cursor, MinR, MinR + 1300.f);
			S.Yaw = (Cursor - S.Pos).Rotation().Yaw;
			S.Patrol = bPatrol ? Rng.FRandRange(400.f, 800.f) : 0.f;
			W.Spawns.Add(S);
		}
		return W;
	};
	auto AddObj = [](TArray<FErtObjective>& L, EErtObjKind K, const TCHAR* Key, int32 Target, bool bOpt = false)
	{
		FErtObjective O; O.Kind = K; O.LocKey = Key; O.Target = Target; O.bOptional = bOpt; L.Add(O);
	};
	auto AddTravel = [&](int32 N, bool bTimed)
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.travel");
		FErtObjective O; O.Kind = EErtObjKind::Route; O.LocKey = TEXT("ui.obj.route"); O.Radius = 300.f;
		FVector C = Cursor; bool bAnyElev = false;
		for (int32 k = 0; k < N; ++k)
		{
			bool bEl = false;
			const FVector Pt = (k % 2 == 1) ? FindElevated(C, 1600.f, 3000.f, bEl) : FindSpot(C, 1800.f, 3000.f);
			bAnyElev |= bEl;
			O.Points.Add(Pt); C = Pt;
		}
		O.bNeedZ = bAnyElev; O.Target = N;
		P.Objectives.Add(O);
		if (bTimed) AddObj(P.Objectives, EErtObjKind::Timer, TEXT("ui.obj.timer"), 16 * N, true);
		Cursor = C;
		Phases.Add(P);
	};
	auto AddHunt = [&](int32 N)
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.hunt");
		FErtWave W;
		for (int32 i = 0; i < N; ++i)
		{
			FErtSpawn S; S.Kind = EErtEnemyKind::Deer;
			S.Pos = FindSpot(Cursor, 2600.f, 4400.f); S.Yaw = Rng.FRandRange(0.f, 360.f); S.Patrol = Rng.FRandRange(500.f, 900.f);
			W.Spawns.Add(S);
		}
		P.Waves.Add(W);
		AddObj(P.Objectives, EErtObjKind::Hunt, TEXT("ui.obj.hunt"), N);
		Phases.Add(P);
	};
	auto AddStealth = [&](int32 Guards)
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.stealth");
		// Bosqinchilik: qorovullar mo'g'ul lageri ichida - o'yinchi lagerga kirib boradi
		if (A_Arch == TEXT("INFILTRATION")) Cursor = GroundAt((CampN + Rng.FRandRange(-60.f, 60.f)) * 100.f, (CampE + Rng.FRandRange(-60.f, 60.f)) * 100.f);
		P.Waves.Add(MakeWave(Guards, 0.f, true));
		P.Reinforce = MakeWave(FMath::Max(2, PerWave - 1), 0.f, false);
		AddObj(P.Objectives, EErtObjKind::StayUndetected, TEXT("ui.obj.undetected"), 0, true);
		AddObj(P.Objectives, EErtObjKind::DefeatAll, TEXT("ui.obj.guards"), 0);
		Phases.Add(P);
	};
	auto AddDefend = [&](int32 Seconds, int32 NWaves)
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.defend");
		const FVector Pt = FindSpot(Cursor, 600.f, 1200.f);
		FErtObjective O; O.Kind = EErtObjKind::HoldPoint; O.LocKey = TEXT("ui.obj.hold"); O.Point = Pt; O.Radius = 700.f; O.Target = Seconds;
		P.Objectives.Add(O);
		Cursor = Pt;
		for (int32 w = 0; w < NWaves; ++w) P.Waves.Add(MakeWave(PerWave + (w == NWaves - 1 ? 1 : 0), 1.2f, false));
		Phases.Add(P);
	};
	auto AddCollect = [&](int32 N)
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.collect");
		FErtObjective O; O.Kind = EErtObjKind::Collect; O.LocKey = TEXT("ui.obj.collect"); O.Radius = 220.f; O.Target = N;
		for (int32 k = 0; k < N; ++k) { O.Points.Add(FindSpot(Cursor, 1000.f, 2800.f)); O.Collected.Add(false); }
		P.Objectives.Add(O);
		Phases.Add(P);
	};
	auto AddDuel = [&]()
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.duel");
		FErtWave W; FErtSpawn S;
		S.Kind = Tier >= 4 ? EErtEnemyKind::Elite : (Tier >= 2 ? EErtEnemyKind::Sergeant : EErtEnemyKind::Footman);
		S.Pos = FindSpot(Cursor, 1000.f, 1600.f); S.Yaw = (Cursor - S.Pos).Rotation().Yaw;
		W.Spawns.Add(S); P.Waves.Add(W);
		AddObj(P.Objectives, EErtObjKind::DefeatAll, TEXT("ui.obj.duel"), 0);
		Phases.Add(P);
	};
	auto AddFight = [&](int32 NWaves, bool bSurvive)
	{
		FErtPhase P; P.TitleKey = TEXT("ui.phase.fight");
		for (int32 w = 0; w < NWaves; ++w) P.Waves.Add(MakeWave(PerWave + (w == NWaves - 1 ? 1 : 0), 1.2f, w == 0));
		AddObj(P.Objectives, EErtObjKind::DefeatAll, TEXT("ui.obj.defeat_all"), 0);
		if (bSurvive) AddObj(P.Objectives, EErtObjKind::SurviveTime, TEXT("ui.obj.survive"), 30 + Tier * 6, true);
		Phases.Add(P);
	};

	// Qo'lda belgilangan ssenariy (missions.json) bo'lsa - shu
	struct FOv { FString Type, Dialog, Loc; int32 N = 2, Waves = 1, Guards = 2, Threshold = 0; bool bTimed = false; TArray<FString> Flags; TArray<FErtCouncilNpc> Npcs; };
	TArray<FOv> Ov;
	{
		FString Text;
		TSharedPtr<FJsonObject> R;
		if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/missions.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), R) && R.IsValid())
		{
			const TSharedPtr<FJsonObject>* EpO = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* Ph = nullptr;
			if ((PhaseOverrideObj.IsValid() && PhaseOverrideObj->TryGetArrayField(TEXT("phases"), Ph)) || (R->TryGetObjectField(E.Id, EpO) && (*EpO)->TryGetArrayField(TEXT("phases"), Ph)))
				for (const auto& PV : *Ph)
				{
					const TSharedPtr<FJsonObject> O = PV->AsObject(); if (!O.IsValid()) continue;
					FOv P;
					O->TryGetStringField(TEXT("type"), P.Type); O->TryGetStringField(TEXT("dialog"), P.Dialog); O->TryGetStringField(TEXT("loc"), P.Loc);
					O->TryGetNumberField(TEXT("n"), P.N); O->TryGetNumberField(TEXT("waves"), P.Waves); O->TryGetNumberField(TEXT("guards"), P.Guards); O->TryGetNumberField(TEXT("threshold"), P.Threshold);
					O->TryGetBoolField(TEXT("timed"), P.bTimed);
					const TArray<TSharedPtr<FJsonValue>>* Fl = nullptr;
					if (O->TryGetArrayField(TEXT("flags"), Fl)) for (const auto& F : *Fl) P.Flags.Add(F->AsString());
					const TArray<TSharedPtr<FJsonValue>>* Np = nullptr;
					if (O->TryGetArrayField(TEXT("npcs"), Np))
						for (const auto& NV : *Np)
						{
							const TSharedPtr<FJsonObject> NO = NV->AsObject(); if (!NO.IsValid()) continue;
							FErtCouncilNpc C;
							NO->TryGetStringField(TEXT("id"), C.Id); NO->TryGetStringField(TEXT("name"), C.NameKey);
							double D = 0; if (NO->TryGetNumberField(TEXT("u"), D)) C.U = D; if (NO->TryGetNumberField(TEXT("v"), D)) C.V = D; if (NO->TryGetNumberField(TEXT("yaw"), D)) C.Yaw = D;
							NO->TryGetBoolField(TEXT("woman"), C.bWoman);
							const TArray<TSharedPtr<FJsonValue>>* K = nullptr;
							if (NO->TryGetArrayField(TEXT("kaftan"), K) && K->Num() >= 3) C.Kaftan = FLinearColor((*K)[0]->AsNumber(), (*K)[1]->AsNumber(), (*K)[2]->AsNumber());
							P.Npcs.Add(C);
						}
					Ov.Add(P);
				}
		}
	}
	if (Ov.Num() > 0)
	{
		for (const FOv& P : Ov)
		{
			if (P.Type == TEXT("travel")) AddTravel(P.N, P.bTimed);
			else if (P.Type == TEXT("hunt")) AddHunt(P.N);
			else if (P.Type == TEXT("stealth")) AddStealth(P.Guards);
			else if (P.Type == TEXT("defend")) AddDefend(30 + Tier * 6, P.Waves);
			else if (P.Type == TEXT("duel")) AddDuel();
			else if (P.Type == TEXT("boss"))
			{
				// No'yon bilan yakuniy duel
				FErtPhase Ph; Ph.TitleKey = TEXT("ui.phase.boss");
				FErtWave W; FErtSpawn S; S.Kind = EErtEnemyKind::Boss; S.Pos = FindSpot(Cursor, 1200.f, 1800.f); S.Yaw = (Cursor - S.Pos).Rotation().Yaw;
				W.Spawns.Add(S);
				for (int32 k = 0; k < FMath::Max(0, P.Guards - 2); ++k) { FErtSpawn G; G.Kind = EErtEnemyKind::Elite; G.Pos = FindSpot(Cursor, 1400.f, 2200.f); G.Yaw = S.Yaw; W.Spawns.Add(G); }
				Ph.Waves.Add(W);
				AddObj(Ph.Objectives, EErtObjKind::DefeatAll, TEXT("ui.obj.boss"), 0);
				Phases.Add(Ph);
			}
			else if (P.Type == TEXT("fight")) AddFight(P.Waves, false);
			else if (P.Type == TEXT("collect"))
			{
				AddCollect(P.N);
				FErtObjective& O = Phases.Last().Objectives[0];
				O.PointFlags = P.Flags;
				if (!P.Loc.IsEmpty()) O.LocKey = P.Loc;
			}
			else if (P.Type == TEXT("council"))
			{
				// Kengash: Bey chodiri oldida NPClar, dialog tugashi = maqsad
				FErtPhase Ph; Ph.TitleKey = TEXT("ui.phase.council");
				FErtObjective O; O.Kind = EErtObjKind::Council; O.LocKey = TEXT("ui.obj.council"); O.DialogId = P.Dialog; O.Threshold = P.Threshold;
				const FErtCouncilNpc* First = P.Npcs.Num() ? &P.Npcs[0] : nullptr;
				O.Point = GroundAt((ObaN + (First ? First->V : -7.f)) * 100.f, (ObaE + (First ? First->U : -6.f)) * 100.f);
				Ph.Objectives.Add(O);
				Ph.Npcs = P.Npcs;
				Phases.Add(Ph);
				Cursor = O.Point;
			}
		}
		return;
	}
	const FString& A = E.Archetype;
	const int32 FightWaves = (Tier <= 1) ? 1 : (Tier >= 4 ? 3 : 2);
	if (A == TEXT("SIEGE") || A == TEXT("DEFENSE")) { AddTravel(2, true); AddDefend(36 + Tier * 8, 2); AddTravel(2, false); AddFight(FightWaves, true); }
	else if (A == TEXT("SURVIVAL"))     { AddHunt(2); AddTravel(3, false); AddFight(FightWaves, true); }
	else if (A == TEXT("INFILTRATION")) { AddTravel(2, false); AddStealth(FMath::Clamp(PerWave, 2, 4)); AddCollect(2); AddTravel(2, true); }
	else if (A == TEXT("CHASE"))        { AddTravel(4, true); AddFight(1, false); AddTravel(2, true); AddDuel(); }
	else if (A == TEXT("ESCORT"))       { AddTravel(2, false); AddDefend(30 + Tier * 6, 2); AddTravel(2, false); AddFight(1, false); }
	else if (A == TEXT("RITUAL"))       { AddCollect(3); AddHunt(1); AddTravel(2, false); AddDuel(); }
	else if (A == TEXT("COURT"))        { AddCollect(2); AddTravel(2, false); AddStealth(2); AddDuel(); }
	else                                { AddTravel(2, false); AddCollect(3); AddStealth(2); AddFight(1, false); }
}

void AErtMissionDirector::StartPhase(int32 Idx)
{
	if (!Phases.IsValidIndex(Idx)) return;
	FErtPhase& P = Phases[Idx];
	PhaseIdx = Idx; PhaseT = 0.f;
	Objectives = P.Objectives;
	Waves = P.Waves;
	WaveIdx = 0;
	SpawnWave(0);
	ClearPhaseNpcs();
	for (const FErtCouncilNpc& C : P.Npcs)
	{
		const FVector G = GroundAt((ObaN + C.V) * 100.f, (ObaE + C.U) * 100.f);
		AErtNpc* Npc = GetWorld()->SpawnActor<AErtNpc>(AErtNpc::StaticClass(), G + FVector(0, 0, 92.f), FRotator(0, C.Yaw, 0));
		if (!Npc) continue;
		const FString Dlg = (P.Objectives.Num() && &C == &P.Npcs[0]) ? P.Objectives[0].DialogId : FString();
		Npc->Setup(C.Id, C.NameKey, Dlg, C.bWoman, C.Kaftan, C.Yaw);
		PhaseNpcs.Add(Npc);
	}
	PhaseTitle = FErtLoc::Get().Tr(P.TitleKey);
	SaveCheckpoint();
	RebuildMarkers();
	UE_LOG(LogErtugrul, Log, TEXT("[Missiya] %s bosqich %d/%d: %s (maqsad %d, to'lqin %d)"), *EpisodeId, Idx + 1, Phases.Num(), *P.TitleKey, P.Objectives.Num(), P.Waves.Num());
}

void AErtMissionDirector::SpawnWave(int32 Idx)
{
	ClearEnemies();
	if (!Waves.IsValidIndex(Idx)) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (const FErtSpawn& S : Waves[Idx].Spawns)
	{
		AErtEnemy* E = GetWorld()->SpawnActor<AErtEnemy>(AErtEnemy::StaticClass(), S.Pos + FVector(0, 0, 100.f), FRotator(0, S.Yaw, 0), SP);
		if (!E) continue;
		E->Init(S.Kind, S.Pos, S.Patrol);
		if (S.Kind == EErtEnemyKind::Rider)
		{
			AErtHorse* Hs = GetWorld()->SpawnActor<AErtHorse>(AErtHorse::StaticClass(), S.Pos + FVector(0, 0, 110.f), FRotator(0, S.Yaw, 0), SP);
			if (Hs) { Hs->Init(FLinearColor(0.18f, 0.13f, 0.10f)); E->MountHorse(Hs); }
		}
		Enemies.Add(E);
	}
	int32 Total = 0, Deer = 0;
	for (const AErtEnemy* E : Enemies) { if (E->IsAnimal()) ++Deer; else ++Total; }
	for (FErtObjective& O : Objectives)
	{
		if (O.Kind == EErtObjKind::DefeatAll) { O.Target = Total; O.Progress = 0; }
		if (O.Kind == EErtObjKind::Hunt && Deer > 0) O.Target = Deer;
	}
}

void AErtMissionDirector::SaveCheckpoint()
{
	AErtCharacter* H = Hero();
	Cp.bValid = true; Cp.Phase = PhaseIdx;
	Cp.Pos = H ? H->GetActorLocation() : Cursor;
	Cp.Yaw = H ? H->GetActorRotation().Yaw : 0.f;
	CpName = FString::Printf(TEXT("CP_%s_P%d"), *EpisodeId, PhaseIdx + 1);
	CpFlash = 1.f;
}

void AErtMissionDirector::RestartFromCheckpoint()
{
	if (!Cp.bValid) return;
	AErtCharacter* H = Hero();
	if (H) H->ResetAt(Cp.Pos + FVector(0, 0, 20.f), Cp.Yaw);
	StartPhase(Cp.Phase);
	State = EErtMissionState::Briefing;
	StateT = 0.f;
}

void AErtMissionDirector::RebuildMarkers()
{
	if (!MarkerMesh) return;
	MarkerMesh->ClearAllMeshSections();
	TArray<FVector> Pts; GetMarkers(Pts);
	if (Pts.Num() == 0) return;
	FErtMeshData M;
	const FLinearColor Gold(1.f, 0.85f, 0.25f, 1.f);
	for (const FVector& P : Pts)
	{
		M.AddCylinder(P + FVector(0, 0, -50), 22.f, 12.f, 900.f, 6, Gold, false);
		M.AddCone(P + FVector(0, 0, 260), 60.f, 60.f, 4, Gold * 1.2f, FRotator(180.f, 0, 0));
	}
	M.Commit(MarkerMesh, 0, false);
}

void AErtMissionDirector::UpdateObjectives(float Dt)
{
	AErtCharacter* H = Hero();
	if (!H) return;
	const FVector PL = H->GetActorLocation();
	const float FeetZ = PL.Z - 92.f;
	bool bMarkersDirty = false;
	int32 Alive = 0, Dead = 0, DeerDead = 0;
	bool bDetected = false;
	for (AErtEnemy* E : Enemies)
	{
		if (!E) continue;
		if (E->IsAnimal()) { if (E->IsDead()) ++DeerDead; continue; }
		if (E->IsDead()) ++Dead; else { ++Alive; if (E->IsAlerted()) bDetected = true; }
	}
	for (FErtObjective& O : Objectives)
	{
		if (O.bDone) continue;
		switch (O.Kind)
		{
		case EErtObjKind::DefeatAll:
			O.Progress = Dead;
			if (O.Target > 0 && Alive == 0) O.bDone = true;
			break;
		case EErtObjKind::SurviveTime:
			O.Progress = (int32)PhaseT;
			if (O.Progress >= O.Target) O.bDone = true;
			break;
		case EErtObjKind::StayUndetected:
			if (bDetected && !O.bFailed)
			{
				O.bFailed = true;
				FErtPhase& P = Phases[PhaseIdx];
				if (!P.bReinforced && P.Reinforce.Spawns.Num())
				{
					P.bReinforced = true;
					FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
					for (const FErtSpawn& S : P.Reinforce.Spawns)
					{
						AErtEnemy* E = GetWorld()->SpawnActor<AErtEnemy>(AErtEnemy::StaticClass(), S.Pos + FVector(0, 0, 100.f), FRotator(0, S.Yaw, 0), SP);
						if (E) { E->Init(S.Kind, S.Pos, 0.f); Enemies.Add(E); }
					}
					for (FErtObjective& O2 : Objectives) if (O2.Kind == EErtObjKind::DefeatAll) O2.Target += P.Reinforce.Spawns.Num();
					UE_LOG(LogErtugrul, Log, TEXT("[Missiya] sezildi - qo'shimcha kuch (%d)"), P.Reinforce.Spawns.Num());
				}
			}
			if (Alive == 0 && !O.bFailed && Dead > 0) O.bDone = true;
			break;
		case EErtObjKind::Route:
			if (O.Progress < O.Points.Num())
			{
				const FVector& T = O.Points[O.Progress];
				const bool bNear = FVector::Dist2D(PL, T) < O.Radius && (!O.bNeedZ || FeetZ > T.Z - 120.f) && FMath::Abs(FeetZ - T.Z) < 400.f;
				if (bNear) { ++O.Progress; bMarkersDirty = true; }
			}
			if (O.Progress >= O.Points.Num()) O.bDone = true;
			break;
		case EErtObjKind::Collect:
			for (int32 i = 0; i < O.Points.Num(); ++i)
				if (!O.Collected[i] && FVector::Dist(PL, O.Points[i] + FVector(0, 0, 90)) < O.Radius)
				{
					O.Collected[i] = true; ++O.Progress; bMarkersDirty = true;
					if (O.PointFlags.IsValidIndex(i)) if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->AddFlag(O.PointFlags[i]); CouncilResult = FErtLoc::Get().Tr(TEXT("ui.hud.evidence")) + TEXT(": ") + O.PointFlags[i]; CouncilResultT = 3.f; }
				}
			if (O.Progress >= O.Target) O.bDone = true;
			break;
		case EErtObjKind::Hunt:
			O.Progress = DeerDead;
			if (O.Progress >= O.Target) O.bDone = true;
			break;
		case EErtObjKind::HoldPoint:
			if (FVector::Dist2D(PL, O.Point) < O.Radius) O.Hold += Dt;
			O.Progress = (int32)O.Hold;
			if (O.Progress >= O.Target) O.bDone = true;
			break;
		case EErtObjKind::Timer:
			O.Progress = (int32)PhaseT;
			if (O.Progress > O.Target) O.bFailed = true;
			break;
		case EErtObjKind::Council:
			if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this)))
				if (GM->LastDialogId == O.DialogId && GM->LastDialogEndTime > GetWorld()->GetTimeSeconds() - PhaseT - 1.f)
				{
					O.bDone = true;
					const bool bWon = O.Threshold <= 0 || GM->LastDuelPoints >= O.Threshold;
					CouncilResult = FErtLoc::Get().Tr(bWon ? TEXT("ui.hud.duel_won") : TEXT("ui.hud.duel_lost")) + FString::Printf(TEXT("  (%d/%d)"), GM->LastDuelPoints, O.Threshold);
					CouncilResultT = 5.f;
					if (bWon) GM->AddFlag(O.DialogId + TEXT("_won"));
					UE_LOG(LogErtugrul, Log, TEXT("[Missiya] kengash: %d/%d ball, %s"), GM->LastDuelPoints, O.Threshold, bWon ? TEXT("yutdi") : TEXT("yutqazdi"));
				}
			break;
		}
	}
	if (bMarkersDirty) RebuildMarkers();
}

void AErtMissionDirector::Tick(float Dt)
{
	Super::Tick(Dt);
	if (State == EErtMissionState::Inactive) return;
	StateT += Dt;
	CpFlash = FMath::Max(0.f, CpFlash - Dt * 0.5f);
	CouncilResultT = FMath::Max(0.f, CouncilResultT - Dt);
	AErtCharacter* H = Hero();
	if (!H) return;

	// O'ldirilganlarni hisoblash
	for (AErtEnemy* E : Enemies)
		if (E && E->IsDead() && E->Killer == H && !E->IsAnimal() && !E->Tags.Contains(TEXT("counted"))) { E->Tags.Add(TEXT("counted")); ++Kills; }

	switch (State)
	{
	case EErtMissionState::Briefing:
		if (StateT >= 2.5f) { State = EErtMissionState::Fighting; StateT = 0.f; }
		break;
	case EErtMissionState::Fighting:
	{
		PhaseT += Dt;
		UpdateObjectives(Dt);
		if (H->IsDead())
		{
			++Deaths;
			State = EErtMissionState::Failed; StateT = 0.f;
			break;
		}
		const int32 Alive = AliveEnemies();
		int32 Total = 0; for (const AErtEnemy* E : Enemies) if (E && !E->IsAnimal()) ++Total;
		if (Alive == 0 && Total > 0 && WaveIdx + 1 < Waves.Num())
		{
			State = EErtMissionState::WaveCleared; StateT = 0.f;
			H->AddArrows(4);
			break;
		}
		bool bAllDone = true;
		for (const FErtObjective& O : Objectives) if (!O.bOptional && !O.bDone) { bAllDone = false; break; }
		const bool bWavesDone = Waves.Num() == 0 || (WaveIdx + 1 >= Waves.Num() && Alive == 0);
		if (bAllDone && bWavesDone)
		{
			H->AddArrows(4);
			H->Heal(25.f);
			if (PhaseIdx + 1 < Phases.Num()) { StartPhase(PhaseIdx + 1); State = EErtMissionState::Fighting; StateT = 0.f; }
			else if (bSideQuest) { State = EErtMissionState::Cleared; StateT = 0.f; H->AddXP(Side.XP); H->AddGold(Side.Gold); Cliffhanger = FErtLoc::Get().TrOr(Side.DoneKey, TEXT(""));
				// Maxsus mukofot
				FString RewardText;
				if (Side.Reward == TEXT("pelt")) { H->bPeltArmor = true; RewardText = TEXT("Bo'ri terisi zirhi: zarar -15%"); }
				else if (Side.Reward == TEXT("potions3")) { H->Potions += 3; RewardText = TEXT("+3 dori"); }
				else if (Side.Reward == TEXT("arrows12")) { H->AddArrows(12); RewardText = TEXT("+12 o'q"); }
				else if (Side.Reward == TEXT("xp100")) { H->AddXP(100); RewardText = TEXT("+100 XP (ustozlik)"); }
				else if (Side.Reward == TEXT("shield")) { H->bShield = true; H->ApplyEquipment(); RewardText = TEXT("Yog'och qalqon"); }
				else if (Side.Reward == TEXT("bow")) { H->BowTier = 2; H->ApplyEquipment(); RewardText = TEXT("Kompozit kamon"); }
				else if (Side.Reward == TEXT("meat6")) { H->Meat += 6; RewardText = TEXT("+6 go'sht"); }
				if (!RewardText.IsEmpty()) Cliffhanger += TEXT("\n") + RewardText; if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->AddFlag(TEXT("sq_done_") + Side.Id); GM->RemoveFlag(TEXT("sq_avail_") + Side.Id); GM->AddHonor(Side.Honor); GM->SaveGame(); } NextEpisodeId.Reset(); }
			else { State = EErtMissionState::Cleared; StateT = 0.f; H->AddXP(150); H->AddGold(40); SaveProgress(); UE_LOG(LogErtugrul, Log, TEXT("[Missiya] %s bajarildi: %d o'ldirildi, %d o'lim"), *EpisodeId, Kills, Deaths); }
		}
		break;
	}
	case EErtMissionState::WaveCleared:
		if (StateT >= 1.5f)
		{
			++WaveIdx;
			SpawnWave(WaveIdx);
			State = EErtMissionState::Fighting; StateT = 0.f;
		}
		break;
	case EErtMissionState::Failed:
		if (StateT >= 3.5f) RestartFromCheckpoint();
		break;
	case EErtMissionState::Cleared:
		if (StateT >= (bSideQuest ? 6.f : 16.f))
		{
			if (!NextEpisodeId.IsEmpty()) StartEpisode(NextEpisodeId);
			else StopEpisode();
		}
		break;
	default: break;
	}
}

// ---------------- Taraqqiyot (Saved/ert_progress.json) ----------------

TArray<FString> AErtMissionDirector::LoadProgress() const
{
	TArray<FString> Done;
	FString Text;
	if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectSavedDir() / TEXT("ert_progress.txt")))) Text.ParseIntoArrayLines(Done);
	return Done;
}

void AErtMissionDirector::SaveProgress()
{
	TArray<FString> Done = LoadProgress();
	Done.AddUnique(EpisodeId);
	FFileHelper::SaveStringToFile(FString::Join(Done, TEXT("\n")), *(FPaths::ProjectSavedDir() / TEXT("ert_progress.txt")));
	if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->SaveGame();
}
