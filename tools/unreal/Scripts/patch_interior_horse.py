# -*- coding: utf-8 -*-
import io, re, json
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

# ================= (k) Ichki jihozlash =================
h = load('ErtWorldBuilder.h')
h = rep(h, "\tvoid BuildProps();", "\tvoid BuildProps();\n\t/** O'tov/uy ichkarisi: E, N, R (m), Z (pol balandligi) - BuildProps buyumlarni ichkariga qo'yadi */\n\tTArray<FVector4> Interiors;")
save('ErtWorldBuilder.h', h)
c = load('ErtWorldBuilder.cpp')
# AddYurt va AddHouse ichki joyni ro'yxatga oladi
c = rep(c, "void AErtWorldBuilder::AddYurt(FErtMeshData& M, float E, float N, float Z, float R, float WallH, float RoofH, const FLinearColor& Wall, const FLinearColor& Roof, float DoorYaw, int32 S)\n{",
        "void AErtWorldBuilder::AddYurt(FErtMeshData& M, float E, float N, float Z, float R, float WallH, float RoofH, const FLinearColor& Wall, const FLinearColor& Roof, float DoorYaw, int32 S)\n{\n\tif (R >= 2.3f) Interiors.Add(FVector4(E, N, R - 0.7f, Z));")
c = rep(c, "void AErtWorldBuilder::AddHouse(FErtMeshData& M, float E, float N, float Z, float HU, float HV, float H, float Yaw, const FLinearColor& C, int32 S)\n{",
        "void AErtWorldBuilder::AddHouse(FErtMeshData& M, float E, float N, float Z, float HU, float HV, float H, float Yaw, const FLinearColor& C, int32 S)\n{\n\tInteriors.Add(FVector4(E, N, FMath::Min(HU, HV) - 0.8f, Z));")
c = rep(c, "\tUE_LOG(LogErtugrul, Log, TEXT(\"Buyumlar (Fab): %d\"), Placed);",
        '''\t// Ichki jihozlar: har o'tov/uyga 1-3 buyum (trace tekshiruvisiz - tom ostida), devordan uzoqroq
\tint32 Inside = 0;
\tfor (const FVector4& In : Interiors)
\t{
\t\tconst int32 Cnt = RS.RandRange(1, 3);
\t\tfor (int32 i = 0; i < Cnt; ++i)
\t\t{
\t\t\tconst float A = RS.FRand() * 2.f * PI, R = RS.FRandRange(0.3f, 0.8f) * In.Z;
\t\t\tconst float E = In.X + FMath::Cos(A) * R, N = In.Y + FMath::Sin(A) * R;
\t\t\tUStaticMesh* Mesh = Fab.Props[RS.RandRange(0, Fab.Props.Num() - 1)];
\t\t\tFabComp(Mesh, false)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, In.W), FVector(FErtFabLib::ScaleToHeight(Mesh, RS.FRandRange(0.45f, 0.8f)))), true);
\t\t\t++Inside;
\t\t}
\t}
\tUE_LOG(LogErtugrul, Log, TEXT("Buyumlar (Fab): %d tashqarida, %d ichkarida (%d xona)"), Placed, Inside, Interiors.Num());''')
save('ErtWorldBuilder.cpp', c)

# ================= (l) Ot: skeletli rejim =================
h = load('ErtHorse.h')
h = rep(h, "\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> NeckMesh;",
        "\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> NeckMesh;\n\t// Skeletli rejim (character.json: \"horse\" / \"camel\"): SingleNode, tezlikka qarab idle/walk/trot/gallop/jump\n\tUPROPERTY(Transient) TObjectPtr<class USkeletalMeshComponent> Skel;\n\tTMap<FString, TArray<TObjectPtr<class UAnimSequence>>> SkelAnims;\n\tUPROPERTY(Transient) TObjectPtr<class UAnimSequence> CurAnim;\n\tfloat WalkRef = 250.f, TrotRef = 520.f, GallopRef = 950.f;\n\tbool TryBuildSkeletal();\n\tvoid SkelPlay(const FString& Key, float Rate, const TCHAR* Fallback = nullptr);")
save('ErtHorse.h', h)
c = load('ErtHorse.cpp')
if '#include "Components/SkeletalMeshComponent.h"' not in c:
    c = c.replace('#include "Materials/MaterialInterface.h"\n', '#include "Materials/MaterialInterface.h"\n#include "Components/SkeletalMeshComponent.h"\n#include "Engine/SkeletalMesh.h"\n#include "Animation/AnimSequence.h"\n#include "Misc/FileHelper.h"\n#include "Misc/Paths.h"\n#include "Dom/JsonObject.h"\n#include "Serialization/JsonReader.h"\n#include "Serialization/JsonSerializer.h"\n', 1)
c = rep(c, "void AErtHorse::Build()\n{\n\tif (bBuilt) return;\n\tbBuilt = true;", "void AErtHorse::Build()\n{\n\tif (bBuilt) return;\n\tbBuilt = true;\n\tif (TryBuildSkeletal()) return;")
c = rep(c, "void AErtHorse::Animate(float Dt)\n{\n\tconst float Sp = GetCharacterMovement()->Velocity.Size2D();",
        '''void AErtHorse::Animate(float Dt)
{
	if (Skel)
	{
		const float Sp2 = GetCharacterMovement()->Velocity.Size2D();
		if (GetCharacterMovement()->IsFalling()) SkelPlay(TEXT("jump"), 1.f, TEXT("gallop"));
		else if (Sp2 < 15.f) SkelPlay(TEXT("idle"), 1.f);
		else if (Sp2 < WalkRef * 1.4f) SkelPlay(TEXT("walk"), FMath::Clamp(Sp2 / WalkRef, 0.5f, 1.5f), TEXT("idle"));
		else if (Sp2 < TrotRef * 1.4f) SkelPlay(TEXT("trot"), FMath::Clamp(Sp2 / TrotRef, 0.6f, 1.5f), TEXT("walk"));
		else SkelPlay(TEXT("gallop"), FMath::Clamp(Sp2 / GallopRef, 0.6f, 1.4f), TEXT("trot"));
		return;
	}
	const float Sp = GetCharacterMovement()->Velocity.Size2D();''')
c = rep(c, "\tif (BodyMesh) { BodyMesh->SetRelativeRotation(FRotator(0, 0, 75.f)); BodyMesh->SetRelativeLocation(FVector(0, 0, -40.f)); }",
        "\tif (Skel) { SkelPlay(TEXT(\"death\"), 1.f, TEXT(\"idle\")); if (CurAnim) { Skel->PlayAnimation(CurAnim, false); } }\n\tif (BodyMesh) { BodyMesh->SetRelativeRotation(FRotator(0, 0, 75.f)); BodyMesh->SetRelativeLocation(FVector(0, 0, -40.f)); }")
c += r'''

// ---------------- Skeletli ot (character.json "horse"/"camel") ----------------

bool AErtHorse::TryBuildSkeletal()
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("Ertugrul/Data/character.json")))) return false;
	TSharedPtr<FJsonObject> Cfg;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Cfg) || !Cfg.IsValid()) return false;
	const TSharedPtr<FJsonObject>* Prof = nullptr;
	if (!Cfg->TryGetObjectField(bCamel ? TEXT("camel") : TEXT("horse"), Prof) || !Prof->IsValid()) return false;
	FString MeshPath; if (!(*Prof)->TryGetStringField(TEXT("mesh"), MeshPath) || MeshPath.IsEmpty()) return false;
	auto ObjPath = [](FString P) { if (!P.Contains(TEXT("."))) { const FString Nm = FPaths::GetBaseFilename(P); P = P + TEXT(".") + Nm; } return P; };
	USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, *ObjPath(MeshPath));
	if (!SM) { UE_LOG(LogErtugrul, Warning, TEXT("character.json: ot meshi topilmadi %s"), *MeshPath); return false; }
	Skel = NewObject<USkeletalMeshComponent>(this, TEXT("SkelHorse"));
	Skel->SetupAttachment(GetCapsuleComponent());
	const double Yaw = (*Prof)->HasField(TEXT("yaw")) ? (*Prof)->GetNumberField(TEXT("yaw")) : -90.0;
	const double Zo = (*Prof)->HasField(TEXT("z")) ? (*Prof)->GetNumberField(TEXT("z")) : 0.0;
	const double Sc = (*Prof)->HasField(TEXT("scale")) ? (*Prof)->GetNumberField(TEXT("scale")) : 1.0;
	Skel->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + Zo));
	Skel->SetRelativeRotation(FRotator(0, Yaw, 0));
	Skel->SetRelativeScale3D(FVector(Sc));
	Skel->SetSkeletalMesh(SM);
	Skel->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Skel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Skel->RegisterComponent();
	if ((*Prof)->HasField(TEXT("walk_ref"))) WalkRef = (*Prof)->GetNumberField(TEXT("walk_ref"));
	if ((*Prof)->HasField(TEXT("trot_ref"))) TrotRef = (*Prof)->GetNumberField(TEXT("trot_ref"));
	if ((*Prof)->HasField(TEXT("gallop_ref"))) GallopRef = (*Prof)->GetNumberField(TEXT("gallop_ref"));
	// Egar joyi: "saddle": [x, y, z] (mesh koordinatasida, sm) yoki "saddle_socket"
	const TArray<TSharedPtr<FJsonValue>>* Sd = nullptr;
	if ((*Prof)->TryGetArrayField(TEXT("saddle"), Sd) && Sd->Num() == 3) Saddle->SetRelativeLocation(FVector((*Sd)[0]->AsNumber(), (*Sd)[1]->AsNumber(), (*Sd)[2]->AsNumber()));
	FString SaddleSock;
	if ((*Prof)->TryGetStringField(TEXT("saddle_socket"), SaddleSock) && !SaddleSock.IsEmpty()) { Saddle->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(*SaddleSock)); }
	const TSharedPtr<FJsonObject>* Anims = nullptr;
	if ((*Prof)->TryGetObjectField(TEXT("anims"), Anims) && Anims->IsValid())
	{
		for (const auto& Pair : (*Anims)->Values)
		{
			TArray<TObjectPtr<UAnimSequence>> List;
			auto Add = [&](const FString& P) { if (UAnimSequence* A = LoadObject<UAnimSequence>(nullptr, *ObjPath(P))) List.Add(A); };
			if (Pair.Value->Type == EJson::String) Add(Pair.Value->AsString());
			else if (Pair.Value->Type == EJson::Array) for (const TSharedPtr<FJsonValue>& V : Pair.Value->AsArray()) Add(V->AsString());
			if (List.Num()) SkelAnims.Add(FString(Pair.Key.ToView()), List);
		}
	}
	SkelPlay(TEXT("idle"), 1.f);
	UE_LOG(LogErtugrul, Log, TEXT("Skeletli %s: %s, %d animatsiya turi"), bCamel ? TEXT("tuya") : TEXT("ot"), *SM->GetName(), SkelAnims.Num());
	return true;
}

void AErtHorse::SkelPlay(const FString& Key, float Rate, const TCHAR* Fallback)
{
	if (!Skel) return;
	const TArray<TObjectPtr<UAnimSequence>>* L = SkelAnims.Find(Key);
	if ((!L || !L->Num()) && Fallback) L = SkelAnims.Find(Fallback);
	if (!L || !L->Num()) L = SkelAnims.Find(TEXT("idle"));
	if (!L || !L->Num()) return;
	UAnimSequence* A = (*L)[0];
	if (A != CurAnim || !Skel->IsPlaying()) { CurAnim = A; Skel->PlayAnimation(A, Key != TEXT("death")); }
	Skel->SetPlayRate(Rate);
}
'''
save('ErtHorse.cpp', c)

# character.json: horse/camel profillari (bo'sh - Blender FBX kutiladi)
p = 'D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/character.json'
d = json.load(io.open(p, encoding='utf-8'))
if 'horse' not in d:
    d['horse'] = {"mesh": "", "yaw": -90, "z": 0, "scale": 1.0, "walk_ref": 250, "trot_ref": 520, "gallop_ref": 950, "saddle": [-8, 0, 66], "saddle_socket": "",
                  "anims": {"idle": "", "walk": "", "trot": "", "gallop": "", "jump": "", "death": ""},
                  "_izoh": "Blender'dan ot FBX: mesh yo'lini va animatsiyalarni yozing; bo'sh bo'lsa protsedural ot"}
    d['camel'] = {"mesh": "", "yaw": -90, "z": 0, "scale": 1.0, "anims": {}}
    io.open(p, 'w', encoding='utf-8', newline='\n').write(json.dumps(d, ensure_ascii=False, indent=1))
print('patched')
