# -*- coding: utf-8 -*-
import io, json
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

b = load('Ertugrul.Build.cs')
b = rep(b, '"Json", "JsonUtilities", "ProceduralMeshComponent"', '"Json", "JsonUtilities", "ProceduralMeshComponent", "AssetRegistry"')
save('Ertugrul.Build.cs', b)

h = load('ErtWorldBuilder.h')
h = rep(h, "\tUPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Parts;",
        "\tUPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Parts;\n\t// Fab/Megascans meshlari uchun instanslangan komponentlar (mesh -> HISM)\n\tUPROPERTY(Transient) TMap<TObjectPtr<UStaticMesh>, TObjectPtr<class UHierarchicalInstancedStaticMeshComponent>> FabInst;\n\tclass UHierarchicalInstancedStaticMeshComponent* FabComp(UStaticMesh* M, bool bCollision);\n\tint32 FabTreesPlaced = 0, FabRocksPlaced = 0;")
if "class UStaticMesh;" not in h:
    h = h.replace("class UProceduralMeshComponent;", "class UProceduralMeshComponent;\nclass UStaticMesh;", 1)
save('ErtWorldBuilder.h', h)

c = load('ErtWorldBuilder.cpp')
if '#include "ErtFab.h"' not in c:
    c = c.replace('#include "ErtProcMesh.h"\n', '#include "ErtProcMesh.h"\n#include "ErtFab.h"\n#include "Engine/StaticMesh.h"\n#include "Components/HierarchicalInstancedStaticMeshComponent.h"\n', 1)
c = rep(c, "\tif (bBuildForest) { BuildForest(); BuildRocks(); }", "\tif (bBuildForest) { BuildForest(); BuildRocks(); }\n\tif (FabTreesPlaced + FabRocksPlaced > 0) UE_LOG(LogErtugrul, Log, TEXT(\"Fab meshlari: %d daraxt, %d qoya (instans)\"), FabTreesPlaced, FabRocksPlaced);")
# FabComp
c = rep(c, "UProceduralMeshComponent* AErtWorldBuilder::NewPart(const FString& Name, bool bCollision, UMaterialInterface* M)",
        '''UHierarchicalInstancedStaticMeshComponent* AErtWorldBuilder::FabComp(UStaticMesh* M, bool bCollision)
{
	if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Found = FabInst.Find(M)) return Found->Get();
	UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("Fab")));
	H->SetupAttachment(RootComponent);
	H->SetMobility(EComponentMobility::Static);
	H->SetStaticMesh(M);
	H->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bCollision) { H->SetCollisionObjectType(ECC_WorldStatic); H->SetCollisionResponseToAllChannels(ECR_Block); }
	H->SetCastShadow(true);
	H->bAffectDistanceFieldLighting = false;
	H->RegisterComponent();
	FabInst.Add(M, H);
	return H;
}

UProceduralMeshComponent* AErtWorldBuilder::NewPart(const FString& Name, bool bCollision, UMaterialInterface* M)''')
# BuildForest: Fab daraxtlari
c = rep(c, "\t\tconst bool bPine = H > 60.f || RS.FRand() < 0.55f;\n\t\tconst int32 cx = FMath::Clamp((int32)((E + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);",
        '''\t\tconst bool bPine = H > 60.f || RS.FRand() < 0.55f;
\t\t{
\t\t\tFErtFabLib& Fab = FErtFabLib::Get();
\t\t\tconst TArray<UStaticMesh*>& Pool = (bPine && Fab.Pines.Num()) ? Fab.Pines : (Fab.Trees.Num() ? Fab.Trees : Fab.Pines);
\t\t\tif (Pool.Num())
\t\t\t{
\t\t\t\tUStaticMesh* Mesh = Pool[RS.RandRange(0, Pool.Num() - 1)];
\t\t\t\tconst float Sc = FErtFabLib::ScaleToHeight(Mesh, RS.FRandRange(0.8f, 1.5f) * (bPine ? 11.f : 8.f));
\t\t\t\tFabComp(Mesh, true)->AddInstance(FTransform(FRotator(0, RS.FRandRange(0.f, 360.f), 0), W(E, N, H - 0.05f), FVector(Sc)), true);
\t\t\t\t++Placed; ++FabTreesPlaced;
\t\t\t\tcontinue;
\t\t\t}
\t\t}
\t\tconst int32 cx = FMath::Clamp((int32)((E + Half) / WorldSizeM * CellsPerSide), 0, CellsPerSide - 1);''')
# BuildRocks: Fab qoyalari
c = rep(c, "\t\tconst float R = RS.FRandRange(1.0f, 4.5f) * (H > 100.f ? 1.6f : 1.f);\n\t\tM.AddSphere(",
        '''\t\tconst float R = RS.FRandRange(1.0f, 4.5f) * (H > 100.f ? 1.6f : 1.f);
\t\t{
\t\t\tFErtFabLib& Fab = FErtFabLib::Get();
\t\t\tif (Fab.Rocks.Num())
\t\t\t{
\t\t\t\tUStaticMesh* Mesh = Fab.Rocks[RS.RandRange(0, Fab.Rocks.Num() - 1)];
\t\t\t\tconst float Sc = FErtFabLib::ScaleToRadius(Mesh, R);
\t\t\t\tFabComp(Mesh, true)->AddInstance(FTransform(FRotator(RS.FRandRange(-8.f, 8.f), RS.FRandRange(0.f, 360.f), RS.FRandRange(-8.f, 8.f)), W(E, N, H - R * 0.15f), FVector(Sc)), true);
\t\t\t\t++Placed; ++FabRocksPlaced;
\t\t\t\tcontinue;
\t\t\t}
\t\t}
\t\tM.AddSphere(''')
save('ErtWorldBuilder.cpp', c)

# uproject: Fab plaginini yoqish
up = 'D:/Unreal_projects/Ertugrul/Ertugrul.uproject'
u = io.open(up, encoding='utf-8').read()
if '"Name": "Fab"' not in u:
    u = u.replace('\t\t{\n\t\t\t"Name": "PythonScriptPlugin",\n\t\t\t"Enabled": true\n\t\t}', '\t\t{\n\t\t\t"Name": "PythonScriptPlugin",\n\t\t\t"Enabled": true\n\t\t},\n\t\t{\n\t\t\t"Name": "Fab",\n\t\t\t"Enabled": true\n\t\t}', 1)
    io.open(up, 'w', encoding='utf-8', newline='\n').write(u)

# Manifest: sinov uchun dvigatel shakllari (Cone = qarag'ay, Sphere = qoya) - keyin bo'shatiladi
man = {
    "_izoh": "Fab/Megascans meshlari: /Game/Fab, /Game/Megascans, /Game/Quixel, /Game/ErtAssets avtomatik skanerlanadi (nomida tree/pine/rock/bush/grass). Bu ro'yxatlar qo'lda qo'shish uchun.",
    "trees": [], "pines": [], "rocks": [], "bushes": [], "grass": [],
    "scan_paths": []
}
io.open('D:/Unreal_projects/Ertugrul/Content/Ertugrul/Data/fab_assets.json', 'w', encoding='utf-8', newline='\n').write(json.dumps(man, ensure_ascii=False, indent=1))
print('patched')
