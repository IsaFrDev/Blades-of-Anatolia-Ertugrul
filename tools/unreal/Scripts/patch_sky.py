# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def load(n): return io.open(SRC + n, encoding='utf-8').read()
def save(n, s): io.open(SRC + n, 'w', encoding='utf-8', newline='\n').write(s)
def rep(s, a, b):
    if b in s: return s
    assert a in s, ("MISSING: " + a[:90])
    return s.replace(a, b)

h = load('ErtWeather.h')
h = rep(h, "\tconst FString& GetWeather() const { return Current; }", "\tconst FString& GetWeather() const { return Current; }\n\t/** Osmon: kun ulushi (0 tun .. 1 kun), quyosh rangi, quyosh balandligi (gradus) - bulut tusi, yulduzlar, tuman rangi */\n\tvoid UpdateSky(float Day, const FLinearColor& SunColor, float ElevDeg);")
h = rep(h, "\tUPROPERTY(Transient) TObjectPtr<AExponentialHeightFog> Fog;", "\tUPROPERTY(Transient) TObjectPtr<AExponentialHeightFog> Fog;\n\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> CloudMesh;\n\tUPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> StarMesh;\n\tUPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> CloudMID;\n\tUPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> StarMID;\n\tfloat Coverage = 0.55f, CloudDark = 0.f;")
save('ErtWeather.h', h)

c = load('ErtWeather.cpp')
if '#include "Materials/MaterialInstanceDynamic.h"' not in c:
    c = c.replace('#include "ErtWeather.h"\n', '#include "ErtWeather.h"\n#include "Materials/MaterialInstanceDynamic.h"\n#include "Components/ExponentialHeightFogComponent.h"\n', 1)
c = rep(c, "\tFog = Cast<AExponentialHeightFog>(UGameplayStatics::GetActorOfClass(this, AExponentialHeightFog::StaticClass()));\n}", '''\tFog = Cast<AExponentialHeightFog>(UGameplayStatics::GetActorOfClass(this, AExponentialHeightFog::StaticClass()));
	// Bulut qatlami: 2 km balandlikda 36 km kenglikdagi tekislik (ikki qatlam), yulduz gumbazi: 60 km radiusli sfera (ichkaridan ko'rinadi)
	auto MakeSky = [&](const TCHAR* Name, const TCHAR* MatPath, UMaterialInstanceDynamic*& OutMID)
	{
		UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(this, Name);
		P->SetupAttachment(RootComponent);
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetCastShadow(false);
		P->bUseAsyncCooking = false;
		P->RegisterComponent();
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, MatPath)) { OutMID = UMaterialInstanceDynamic::Create(M, this); P->SetMaterial(0, OutMID); }
		return P;
	};
	{
		CloudMesh = MakeSky(TEXT("Clouds"), TEXT("/Game/Ertugrul/Materials/M_ErtClouds.M_ErtClouds"), CloudMID);
		FErtMeshData D;
		const float Hs = 1800000.f;
		for (int32 L = 0; L < 2; ++L)
		{
			const float Z = 200000.f + L * 70000.f;
			const int32 N = 12;
			for (int32 i = 0; i < N; ++i) for (int32 j = 0; j < N; ++j)
			{
				const float x0 = -Hs + i * 2.f * Hs / N, x1 = x0 + 2.f * Hs / N, y0 = -Hs + j * 2.f * Hs / N, y1 = y0 + 2.f * Hs / N;
				D.AddQuad(FVector(x0, y0, Z), FVector(x1, y0, Z), FVector(x1, y1, Z), FVector(x0, y1, Z), -FVector::UpVector, FLinearColor::White);
			}
		}
		D.Commit(CloudMesh, 0, false);
		CloudMesh->SetBoundsScale(50.f);
	}
	{
		StarMesh = MakeSky(TEXT("Stars"), TEXT("/Game/Ertugrul/Materials/M_ErtStars.M_ErtStars"), StarMID);
		FErtMeshData D;
		D.AddSphere(FVector(0, 0, -200000.f), 6000000.f, 28, FLinearColor::White);
		D.Commit(StarMesh, 0, false);
		StarMesh->SetBoundsScale(50.f);
	}
}

void AErtWeather::UpdateSky(float Day, const FLinearColor& SunColor, float ElevDeg)
{
	const float Twilight = 1.f - FMath::Clamp(FMath::Abs(ElevDeg) / 12.f, 0.f, 1.f);   // ufq yaqinida
	if (CloudMID)
	{
		// Kunduzi oq, tong/shomda quyosh rangida (zarhal), tunda ko'kimtir-qorong'i; bo'ronda qoraytiriladi
		FLinearColor Tint = FMath::Lerp(FLinearColor(0.06f, 0.07f, 0.12f), FLinearColor(0.95f, 0.96f, 1.0f), Day);
		Tint = FMath::Lerp(Tint, SunColor * 1.05f, Twilight * Day);
		Tint *= (1.f - 0.65f * CloudDark);
		CloudMID->SetVectorParameterValue(TEXT("Tint"), Tint);
		CloudMID->SetScalarParameterValue(TEXT("Coverage"), Coverage);
		CloudMID->SetScalarParameterValue(TEXT("Density"), 0.85f + 0.15f * CloudDark);
	}
	if (StarMID) StarMID->SetScalarParameterValue(TEXT("Vis"), (1.f - Day) * (1.f - Coverage * 0.6f));
	if (Fog && Fog->GetComponent())
	{
		FLinearColor FogCol = FMath::Lerp(FLinearColor(0.02f, 0.03f, 0.06f), FLinearColor(0.62f, 0.72f, 0.86f), Day);
		FogCol = FMath::Lerp(FogCol, FLinearColor(0.95f, 0.55f, 0.32f), Twilight * Day * 0.8f);
		if (CloudDark > 0.f) FogCol = FMath::Lerp(FogCol, FLinearColor(0.35f, 0.37f, 0.4f), CloudDark * 0.7f);
		Fog->GetComponent()->SetFogInscatteringColor(FogCol);
	}
}''')
# Ob-havo -> bulut qoplami
c = rep(c, "\tfloat FogDensity = 0.0006f;", "\tfloat FogDensity = 0.0006f;\n\tCoverage = 0.55f; CloudDark = 0.f;\n\tif (Name == TEXT(\"rain\")) { Coverage = 0.25f; CloudDark = 0.55f; }\n\telse if (Name == TEXT(\"storm\") || Name == TEXT(\"blizzard\")) { Coverage = 0.12f; CloudDark = 0.85f; }\n\telse if (Name == TEXT(\"snow\") || Name == TEXT(\"fog\")) { Coverage = 0.3f; CloudDark = 0.3f; }\n\telse if (Name == TEXT(\"dust\")) { Coverage = 0.6f; CloudDark = 0.4f; }\n\telse if (Name == TEXT(\"wind\")) { Coverage = 0.45f; }")
save('ErtWeather.cpp', c)

g = load('ErtGameMode.cpp')
g = rep(g, "\tif (UDirectionalLightComponent* DL = Sun->GetComponent())\n\t{\n\t\tDL->SetIntensity(FMath::Lerp(0.9f, 7.f, Day));\n\t\tDL->SetLightColor(FMath::Lerp(FLinearColor(0.45f, 0.55f, 0.9f), FMath::Lerp(FLinearColor(1.f, 0.62f, 0.35f), FLinearColor(1.f, 0.96f, 0.9f), FMath::Clamp(Elev / 25.f, 0.f, 1.f)), Day));\n\t}",
        "\tconst FLinearColor SunCol = FMath::Lerp(FLinearColor(0.45f, 0.55f, 0.9f), FMath::Lerp(FLinearColor(1.f, 0.55f, 0.28f), FLinearColor(1.f, 0.96f, 0.9f), FMath::Clamp(Elev / 25.f, 0.f, 1.f)), Day);\n\tif (UDirectionalLightComponent* DL = Sun->GetComponent())\n\t{\n\t\tDL->SetIntensity(FMath::Lerp(0.9f, 7.f, Day) * (1.f - 0.45f * (Weather ? (Weather->GetWeather() == TEXT(\"storm\") || Weather->GetWeather() == TEXT(\"rain\") ? 1.f : 0.f) : 0.f)));\n\t\tDL->SetLightColor(SunCol);\n\t}\n\tif (Weather) Weather->UpdateSky(Day, SunCol, Elev);")
if '#include "ErtWeather.h"' not in g:
    g = g.replace('#include "ErtGameMode.h"\n', '#include "ErtGameMode.h"\n#include "ErtWeather.h"\n', 1)
save('ErtGameMode.cpp', g)

# Sinov: shom va tun skrinshotlari
ch = load('ErtCharacter.cpp')
ch = rep(ch, '\tif (At(48.9f)) TakeShot(TEXT("domanic"));\n\tif (At(49.4f))', '\tif (At(48.9f)) TakeShot(TEXT("domanic"));\n\tif (At(49.0f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->SetTimeOfDay(TEXT("dusk")); Teleport(-560.f, 380.f, 12.f + 30.f, -8.f, -20.f); }\n\tif (At(50.2f)) TakeShot(TEXT("dusk"));\n\tif (At(50.3f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) GM->SetTimeOfDay(TEXT("night")); }\n\tif (At(51.5f)) TakeShot(TEXT("night"));\n\tif (At(51.6f)) { if (AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(this))) { GM->SetTimeOfDay(TEXT("day")); GM->SetWeather(TEXT("storm")); } }\n\tif (At(52.8f)) TakeShot(TEXT("storm"));\n\tif (At(53.3f))')
save('ErtCharacter.cpp', ch)
print('patched')
