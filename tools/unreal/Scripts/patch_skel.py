# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtHeroBody.h')
h = rep(h, "\t/** Tanani quradi (bo'g'imlar + geometriya). Parent - kapsula yoki ildiz komponent. */\n\tvoid Build(USceneComponent* Parent, float CapsuleHalfHeight);",
        '''\t/** character.json dagi profil (hero / enemy / npc). Profilda skeletli mesh bo'lsa protsedural tana o'rniga u ishlatiladi. */
\tUPROPERTY(EditAnywhere, Category = "Ertugrul|Skelet") FString Profile = TEXT("hero");
\tbool IsSkeletal() const { return Skel != nullptr; }
\t/** Tanani quradi (bo'g'imlar + geometriya). Parent - kapsula yoki ildiz komponent. */
\tvoid Build(USceneComponent* Parent, float CapsuleHalfHeight);''')
h = rep(h, "\tbool IsBuilt() const { return Pelvis != nullptr; }", "\tbool IsBuilt() const { return Pelvis != nullptr || Skel != nullptr; }")
h = rep(h, "\tUPROPERTY(Transient) TObjectPtr<UMaterialInterface> Mat;", '''\tUPROPERTY(Transient) TObjectPtr<UMaterialInterface> Mat;
\t// Skeletli rejim (character.json): SingleNode animatsiya, holatga qarab almashtiriladi
\tUPROPERTY(Transient) TObjectPtr<class USkeletalMeshComponent> Skel;
\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> SkelSword;
\tTMap<FString, TArray<TObjectPtr<class UAnimSequence>>> SkelAnims;
\tUPROPERTY(Transient) TObjectPtr<class UAnimSequence> CurAnim;
\tfloat OneShotT = 0.f, WalkRef = 200.f, RunRef = 500.f;
\tFVector SwordLoc = FVector::ZeroVector; FRotator SwordRot = FRotator::ZeroRotator; FName SwordSocket = TEXT("hand_r");
\tbool TryBuildSkeletal(USceneComponent* Parent, float HalfH);
\tclass UAnimSequence* SkelPick(const FString& Key, int32 Index = -1) const;
\tvoid SkelPlay(const FString& Key, bool bLoop, float Rate = 1.f, int32 Index = -1, const TCHAR* Fallback = nullptr);
\tvoid SkelAnimate(float Dt, float Speed, bool bInAir, bool bCrouched);
\tvoid SkelBuildSword();''')
save('ErtHeroBody.h', h)

c = load('ErtHeroBody.cpp')
if '#include "Components/SkeletalMeshComponent.h"' not in c:
    c = c.replace('#include "Materials/MaterialInterface.h"\n', '#include "Materials/MaterialInterface.h"\n#include "Components/SkeletalMeshComponent.h"\n#include "Engine/SkeletalMesh.h"\n#include "Animation/AnimSequence.h"\n#include "Misc/FileHelper.h"\n#include "Misc/Paths.h"\n#include "Dom/JsonObject.h"\n#include "Serialization/JsonReader.h"\n#include "Serialization/JsonSerializer.h"\n', 1)
c = rep(c, "\tif (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT(\"/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial\"));\n\n\t// Bo'g'im pivotlari (sm). Tos suyagi yerdan 89 sm balandda.",
        "\tif (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT(\"/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial\"));\n\tif (TryBuildSkeletal(Parent, HalfH)) return;\n\n\t// Bo'g'im pivotlari (sm). Tos suyagi yerdan 89 sm balandda.")
# Animate: skelet bo'lsa alohida yo'l
c = rep(c, "void UErtHeroBody::Animate(float Dt, float Speed, bool bInAir, bool bCrouched, float Lean, float SlopeDeg)\n{\n\tif (!IsBuilt() || bDead) return;",
        "void UErtHeroBody::Animate(float Dt, float Speed, bool bInAir, bool bCrouched, float Lean, float SlopeDeg)\n{\n\tif (!IsBuilt() || bDead) return;\n\tif (Skel) { SkelAnimate(Dt, Speed, bInAir, bCrouched); return; }")
c = rep(c, "void UErtHeroBody::TriggerAttack(int32 Kind) { AttackT = 1.f; AttackKind = Kind; }",
        '''void UErtHeroBody::TriggerAttack(int32 Kind)
{
	AttackT = 1.f; AttackKind = Kind;
	if (Skel)
	{
		const TCHAR* Key = Kind == 2 ? TEXT("heavy") : Kind == 3 ? TEXT("kick") : TEXT("attack");
		SkelPlay(Key, false, Kind == 2 ? 1.f : 1.25f, -1, TEXT("attack"));
		if (CurAnim) OneShotT = CurAnim->GetPlayLength() / (Kind == 2 ? 1.f : 1.25f) * 0.9f;
	}
}''')
c = rep(c, "void UErtHeroBody::TriggerHurt(float SideSign) { HurtT = 1.f; HurtDir = SideSign; }",
        '''void UErtHeroBody::TriggerHurt(float SideSign)
{
	HurtT = 1.f; HurtDir = SideSign;
	if (Skel && OneShotT <= 0.15f)
	{
		SkelPlay(TEXT("hurt"), false, 1.3f, -1, TEXT("idle"));
		if (CurAnim) OneShotT = FMath::Min(0.6f, CurAnim->GetPlayLength() / 1.3f);
	}
}''')
c = rep(c, "void UErtHeroBody::SetDead(float HalfH, int32 Variant)\n{\n\tif (!IsBuilt() || bDead) return;\n\tbDead = true;",
        "void UErtHeroBody::SetDead(float HalfH, int32 Variant)\n{\n\tif (!IsBuilt() || bDead) return;\n\tbDead = true;\n\tif (Skel) { OneShotT = 0.f; SkelPlay(TEXT(\"death\"), false, 1.f, -1, TEXT(\"idle\")); return; }")
c = rep(c, "void UErtHeroBody::SetShield(bool bOn)\n{\n\tif (!IsBuilt()) return;", "void UErtHeroBody::SetShield(bool bOn)\n{\n\tif (!IsBuilt() || Skel) return;")
c = rep(c, "void UErtHeroBody::SetSwordTier(int32 Tier)\n{\n\tif (!IsBuilt() || !bSwordInHand) return;\n\tSteel = Tier >= 2 ? FLinearColor(0.55f, 0.62f, 0.75f) : FLinearColor(0.75f, 0.77f, 0.80f);",
        "void UErtHeroBody::SetSwordTier(int32 Tier)\n{\n\tif (!IsBuilt() || !bSwordInHand) return;\n\tSteel = Tier >= 2 ? FLinearColor(0.55f, 0.62f, 0.75f) : FLinearColor(0.75f, 0.77f, 0.80f);\n\tif (Skel) { SkelBuildSword(); return; }")
c = rep(c, "void UErtHeroBody::AddWound(float SideSign, float Strength)\n{\n\tif (!IsBuilt() || bDead) return;", "void UErtHeroBody::AddWound(float SideSign, float Strength)\n{\n\tif (!IsBuilt() || bDead || Skel) return;")
c += r'''

// ---------------- Skeletli rejim (character.json) ----------------

namespace
{
	TSharedPtr<FJsonObject> ErtCharacterJson()
	{
		static TSharedPtr<FJsonObject> Root; static bool bTried = false;
		if (bTried) return Root;
		bTried = true;
		FString Json;
		if (FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/character.json"))))
		{
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
			if (!FJsonSerializer::Deserialize(Reader, Root)) Root.Reset();
		}
		return Root;
	}
	FString ErtObjPath(FString P)
	{
		if (!P.Contains(TEXT("."))) { const FString Name = FPaths::GetBaseFilename(P); P = P + TEXT(".") + Name; }
		return P;
	}
}

bool UErtHeroBody::TryBuildSkeletal(USceneComponent* Parent, float HalfH)
{
	TSharedPtr<FJsonObject> Root = ErtCharacterJson();
	if (!Root.IsValid()) return false;
	const TSharedPtr<FJsonObject>* Prof = nullptr;
	if (!Root->TryGetObjectField(Profile, Prof) || !Prof->IsValid()) return false;
	FString MeshPath; if (!(*Prof)->TryGetStringField(TEXT("mesh"), MeshPath) || MeshPath.IsEmpty()) return false;
	USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, *ErtObjPath(MeshPath));
	if (!SM) { UE_LOG(LogErtugrul, Warning, TEXT("character.json: mesh topilmadi %s"), *MeshPath); return false; }
	AActor* Owner = GetOwner();
	Skel = NewObject<USkeletalMeshComponent>(Owner, MakeUniqueObjectName(Owner, USkeletalMeshComponent::StaticClass(), TEXT("SkelBody")));
	Skel->SetupAttachment(Parent);
	const double Yaw = (*Prof)->HasField(TEXT("yaw")) ? (*Prof)->GetNumberField(TEXT("yaw")) : -90.0;
	const double Zo = (*Prof)->HasField(TEXT("z")) ? (*Prof)->GetNumberField(TEXT("z")) : 0.0;
	const double Sc = (*Prof)->HasField(TEXT("scale")) ? (*Prof)->GetNumberField(TEXT("scale")) : 1.0;
	Skel->SetRelativeLocation(FVector(0, 0, -HalfH + Zo));
	Skel->SetRelativeRotation(FRotator(0, Yaw, 0));
	Skel->SetRelativeScale3D(FVector(Sc));
	Skel->SetSkeletalMesh(SM);
	Skel->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skel->SetCastShadow(true);
	Skel->RegisterComponent();
	if ((*Prof)->HasField(TEXT("walk_ref"))) WalkRef = (*Prof)->GetNumberField(TEXT("walk_ref"));
	if ((*Prof)->HasField(TEXT("run_ref"))) RunRef = (*Prof)->GetNumberField(TEXT("run_ref"));
	const TSharedPtr<FJsonObject>* Anims = nullptr;
	if ((*Prof)->TryGetObjectField(TEXT("anims"), Anims) && Anims->IsValid())
	{
		for (const auto& Pair : (*Anims)->Values)
		{
			TArray<TObjectPtr<UAnimSequence>> List;
			auto Add = [&](const FString& P) { if (UAnimSequence* A = LoadObject<UAnimSequence>(nullptr, *ErtObjPath(P))) List.Add(A); else UE_LOG(LogErtugrul, Warning, TEXT("character.json: animatsiya topilmadi %s"), *P); };
			if (Pair.Value->Type == EJson::String) Add(Pair.Value->AsString());
			else if (Pair.Value->Type == EJson::Array) for (const TSharedPtr<FJsonValue>& V : Pair.Value->AsArray()) Add(V->AsString());
			if (List.Num()) SkelAnims.Add(FString(Pair.Key.ToView()), List);
		}
	}
	const TSharedPtr<FJsonObject>* Sw = nullptr;
	if ((*Prof)->TryGetObjectField(TEXT("sword"), Sw) && Sw->IsValid())
	{
		FString Sock; if ((*Sw)->TryGetStringField(TEXT("socket"), Sock)) SwordSocket = FName(*Sock);
		const TArray<TSharedPtr<FJsonValue>>* L = nullptr;
		if ((*Sw)->TryGetArrayField(TEXT("loc"), L) && L->Num() == 3) SwordLoc = FVector((*L)[0]->AsNumber(), (*L)[1]->AsNumber(), (*L)[2]->AsNumber());
		if ((*Sw)->TryGetArrayField(TEXT("rot"), L) && L->Num() == 3) SwordRot = FRotator((*L)[0]->AsNumber(), (*L)[1]->AsNumber(), (*L)[2]->AsNumber());
	}
	if (bSwordInHand) SkelBuildSword();
	SkelPlay(TEXT("idle"), true);
	UE_LOG(LogErtugrul, Log, TEXT("Skeletli tana (%s): %s, %d animatsiya turi"), *Profile, *SM->GetName(), SkelAnims.Num());
	return true;
}

void UErtHeroBody::SkelBuildSword()
{
	if (!Skel) return;
	if (!SkelSword)
	{
		SkelSword = MakePart(TEXT("SkelSword"), Skel, FVector::ZeroVector);
		SkelSword->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SwordSocket);
		SkelSword->SetRelativeLocation(SwordLoc); SkelSword->SetRelativeRotation(SwordRot);
	}
	FErtMeshData M;
	const FLinearColor SteelS = ErtCol::Sty(Steel, ErtCol::StyleMetal), TrimS = ErtCol::Sty(Trim, ErtCol::StyleMetal), LeatherS = ErtCol::Sty(Leather, ErtCol::StyleLeather);
	M.AddBox(FVector(0, 0, -12), FVector(1.6f, 1.6f, 12), LeatherS);            // dasta (pastga)
	M.AddBox(FVector(0, 0, 0), FVector(1.5f, 7, 1.5f), TrimS);                   // qo'riqlovchi
	M.AddBox(FVector(0, 0, 42), FVector(0.6f, 3.2f, 42), SteelS);                // tig' (yuqoriga)
	M.AddSphere(FVector(0, 0, -25), 2.f, 6, TrimS);                              // soqqa
	M.Commit(SkelSword, 0, false);
}

UAnimSequence* UErtHeroBody::SkelPick(const FString& Key, int32 Index) const
{
	const TArray<TObjectPtr<UAnimSequence>>* L = SkelAnims.Find(Key);
	if (!L || L->Num() == 0) return nullptr;
	return (*L)[Index >= 0 ? Index % L->Num() : FMath::RandRange(0, L->Num() - 1)];
}

void UErtHeroBody::SkelPlay(const FString& Key, bool bLoop, float Rate, int32 Index, const TCHAR* Fallback)
{
	if (!Skel) return;
	UAnimSequence* A = SkelPick(Key, Index);
	if (!A && Fallback) A = SkelPick(Fallback, Index);
	if (!A) A = SkelPick(TEXT("idle"), 0);
	if (!A) return;
	if (A != CurAnim || !Skel->IsPlaying())
	{
		CurAnim = A;
		Skel->PlayAnimation(A, bLoop);
	}
	Skel->SetPlayRate(Rate);
}

void UErtHeroBody::SkelAnimate(float Dt, float Speed, bool bInAir, bool bCrouched)
{
	IdleT += Dt;
	AttackT = FMath::Max(0.f, AttackT - Dt / (AttackKind == 2 ? 0.75f : 0.45f));
	HurtT = FMath::Max(0.f, HurtT - Dt / 0.35f);
	ParryT = FMath::Max(0.f, ParryT - Dt / 0.3f);
	if (OneShotT > 0.f) { OneShotT -= Dt; if (OneShotT > 0.f) return; }
	if (bSwim) { SkelPlay(TEXT("swim"), true, 1.f, 0, TEXT("fall")); return; }
	if (bRide) { SkelPlay(TEXT("ride"), true, 1.f, 0, TEXT("idle")); return; }
	if (bInAir) { SkelPlay(TEXT("fall"), true, 1.f, 0, TEXT("jump")); return; }
	if (Speed < 15.f) { SkelPlay(bBlock ? TEXT("block") : (bCrouched ? TEXT("crouch") : TEXT("idle")), true, 1.f, 0, TEXT("idle")); return; }
	if (bCrouched) { SkelPlay(TEXT("crouchwalk"), true, FMath::Clamp(Speed / WalkRef, 0.5f, 1.4f), 0, TEXT("walk")); return; }
	if (Speed < 420.f) SkelPlay(TEXT("walk"), true, FMath::Clamp(Speed / WalkRef, 0.5f, 1.6f), 0, TEXT("idle"));
	else SkelPlay(TEXT("run"), true, FMath::Clamp(Speed / RunRef, 0.6f, 1.6f), 0, TEXT("walk"));
}
'''
save('ErtHeroBody.cpp', c)

# Dushman va NPC profillari
e = load('ErtEnemy.cpp')
e = rep(e, "\tBody = CreateDefaultSubobject<UErtHeroBody>(TEXT(\"Body\"));\n\tAutoPossessAI", "\tBody = CreateDefaultSubobject<UErtHeroBody>(TEXT(\"Body\"));\n\tBody->Profile = TEXT(\"enemy\");\n\tAutoPossessAI")
save('ErtEnemy.cpp', e)
n = load('ErtNpc.cpp')
if 'Profile = TEXT("npc")' not in n:
    import re
    m = re.search(r"Body = CreateDefaultSubobject<UErtHeroBody>\(TEXT\(\"Body\"\)\);", n)
    if m: n = n[:m.end()] + "\n\tBody->Profile = TEXT(\"npc\");" + n[m.end():]
save('ErtNpc.cpp', n)
print('patched')
