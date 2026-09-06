# -*- coding: utf-8 -*-
import io
SRC = 'D:/Unreal_projects/Ertugrul/Source/Ertugrul/'
def rep(p, a, b):
    s = io.open(SRC + p, encoding='utf-8').read()
    if b in s: return
    assert a in s, ("MISSING in " + p + ": " + a[:80])
    io.open(SRC + p, 'w', encoding='utf-8', newline='\n').write(s.replace(a, b))

# ---------------- ErtMap3D: surish, egish, unproject ----------------
rep('ErtMap3D.h', "\tvoid Rotate(float DeltaYaw) { Yaw += DeltaYaw; UpdateCamera(); }\n\tvoid Zoom(float Factor);\n\tfloat GetYaw() const { return Yaw; }",
"""	void Rotate(float DeltaYaw) { Yaw += DeltaYaw; UpdateCamera(); }
	void Zoom(float Factor);
	float GetYaw() const { return Yaw; }
	/** Xarita markazi (dunyo XY, sm) - surish */
	void SetCenter(const FVector2D& WorldXY) { Center = WorldXY; UpdateCamera(); }
	const FVector2D& GetCenter() const { return Center; }
	/** Ekran piksellari bo'yicha surish (S - xarita kvadrati piksel o'lchami) */
	void PanPixels(float Dx, float Dy, float S);
	void Tilt(float DeltaPitch) { Pitch = FMath::Clamp(Pitch + DeltaPitch, -89.f, -30.f); UpdateCamera(); }
	float GetOrtho() const { return Ortho; }
	/** Xarita teksturasi [0..1] -> dunyo XY (sm), relyef sathi bilan */
	FVector Unproject(float U, float V) const;""")
rep('ErtMap3D.h', "\tfloat Yaw = 35.f, Pitch = -52.f, Ortho = 4600.f;", "\tfloat Yaw = 35.f, Pitch = -62.f, Ortho = 4600.f;\n\tFVector2D Center = FVector2D::ZeroVector;")
rep('ErtMap3D.cpp', "\tPrimaryActorTick.bCanEverTick = true;\n\tRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT(\"Root\"));\n}\n\nAErtMap3D* AErtMap3D::Get",
    "\tPrimaryActorTick.bCanEverTick = true;\n\tPrimaryActorTick.bTickEvenWhenPaused = true;\n\tRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT(\"Root\"));\n}\n\nAErtMap3D* AErtMap3D::Get")
rep('ErtMap3D.cpp', "\tconst FRotator R(Pitch, Yaw, 0.f);\n\tconst FVector Center = Origin + FVector(0, 0, 400.f);\n\tCapture->SetWorldLocation(Center - R.Vector() * 60000.f);",
    "\tconst FRotator R(Pitch, Yaw, 0.f);\n\tconst FVector Cn = Origin + FVector(Center.X * K, Center.Y * K, 400.f);\n\tCapture->SetWorldLocation(Cn - R.Vector() * 60000.f);")
rep('ErtMap3D.cpp', "void AErtMap3D::Zoom(float Factor) { Ortho = FMath::Clamp(Ortho * Factor, 1200.f, 5200.f); UpdateCamera(); }",
"""void AErtMap3D::Zoom(float Factor) { Ortho = FMath::Clamp(Ortho * Factor, 500.f, 5200.f); UpdateCamera(); }

void AErtMap3D::PanPixels(float Dx, float Dy, float S)
{
	if (S <= 1.f) return;
	const float Yr = FMath::DegreesToRadians(Yaw);
	const FVector2D Fwd(FMath::Cos(Yr), FMath::Sin(Yr)), Right(-FMath::Sin(Yr), FMath::Cos(Yr));
	const float PerPx = Ortho / S / K;   // dunyo sm / piksel
	Center += (-Right * Dx + Fwd * Dy / FMath::Max(0.3f, FMath::Abs(FMath::Sin(FMath::DegreesToRadians(Pitch))))) * PerPx;
	const float Lim = 105000.f;
	Center.X = FMath::Clamp(Center.X, -Lim, Lim); Center.Y = FMath::Clamp(Center.Y, -Lim, Lim);
	UpdateCamera();
}

FVector AErtMap3D::Unproject(float U, float V) const
{
	if (!Capture) return FVector::ZeroVector;
	const FRotator R = Capture->GetComponentRotation();
	const FVector Right = FRotationMatrix(R).GetUnitAxis(EAxis::Y), Up = FRotationMatrix(R).GetUnitAxis(EAxis::Z), Fwd = R.Vector();
	// Ortografik nur: kamera tekisligidagi nuqta + yo'nalish; miniatyura o'rtacha sathi (Origin.Z + 20 m * KZ) bilan kesishuv
	const FVector P0 = Capture->GetComponentLocation() + Right * (U - 0.5f) * Ortho + Up * (0.5f - V) * Ortho;
	const float PlaneZ = Origin.Z + 2000.f * KZ;
	const float T = FMath::Abs(Fwd.Z) > 0.01f ? (PlaneZ - P0.Z) / Fwd.Z : 0.f;
	const FVector Mini = P0 + Fwd * T;
	FVector Wp((Mini.X - Origin.X) / K, (Mini.Y - Origin.Y) / K, 0.f);
	if (World) Wp.Z = World->HeightAt(Wp.Y / 100.f, Wp.X / 100.f) * 100.f;
	return Wp;
}""")
# Yo'l nuqtasi konusi va kashf qilinmagan hududlar miniatyurada emas - HUD da chiziladi

# ---------------- GameMode: xarita boshqaruvi, yo'l nuqtasi, kashfiyot ----------------
rep('ErtGameMode.h', "\t/** 3D xarita aylantirish (chap/o'ng) */\n\tvoid MapRotate(float DeltaYaw);\n\tbool bGps = true;",
"""	/** 3D xarita aylantirish (chap/o'ng) */
	void MapRotate(float DeltaYaw);
	bool bGps = true;
	// AC/GTA uslubidagi xarita: yo'l nuqtasi, kashf qilingan hududlar, sichqoncha boshqaruvi
	FVector Waypoint = FVector::ZeroVector; bool bWaypoint = false;
	void SetWaypoint(const FVector& W) { Waypoint = W; bWaypoint = true; }
	void ClearWaypoint() { bWaypoint = false; }
	static constexpr int32 VisN = 40;   // 50 m hujayralar (2000 m)
	TArray<uint8> Visited;
	bool IsVisited(float E, float N) const { const int32 X = FMath::Clamp((int32)((E + 1000.f) / 50.f), 0, VisN - 1), Y = FMath::Clamp((int32)((N + 1000.f) / 50.f), 0, VisN - 1); return Visited.IsValidIndex(Y * VisN + X) && Visited[Y * VisN + X] != 0; }
	float VisitT = 0.f;
	/** HUD xarita kvadrati (X0, Y0, S) - sichqoncha koordinatalari uchun */
	FVector MapRect = FVector::ZeroVector;
	FVector2D MapMouse = FVector2D(-1, -1);   // xarita ichidagi [0..1] sichqoncha, tashqarida -1
	FVector2D LastMouse = FVector2D::ZeroVector; bool bDragging = false;
	void MapInput(float Dt);""")
rep('ErtGameMode.cpp', "\tPrimaryActorTick.bCanEverTick = true;", "\tPrimaryActorTick.bCanEverTick = true;\n\tPrimaryActorTick.bTickEvenWhenPaused = true;\n\tVisited.SetNumZeroed(VisN * VisN);")
# Sichqoncha kursori xarita ochilganda
rep('ErtGameMode.cpp', "\tif (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->SetActive(Menu == EErtMenu::Map);\n}",
"""	if (AErtMap3D* M3 = AErtMap3D::Get(GetWorld())) M3->SetActive(Menu == EErtMenu::Map);
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
}""")
# Tick: kashfiyot va xarita boshqaruvi; GPS maqsadi: yo'l nuqtasi ustun
rep('ErtGameMode.cpp', "\t// GPS: faol maqsadga yo'l\n\tif (AErtGps* G = AErtGps::Get(GetWorld()))\n\t{\n\t\tFVector T = FVector::ZeroVector;\n\t\tif (bGps && Director && Director->GetState() != EErtMissionState::Inactive)",
"""	if (Menu == EErtMenu::Map) MapInput(Dt);
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
				Visited[Yi * VisN + Xi] = 1;
			}
		}
	}
	if (bWaypoint) if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) if (FVector::Dist2D(P->GetActorLocation(), Waypoint) < 400.f) ClearWaypoint();   // yetib keldi
	// GPS: faol maqsadga yo'l
	if (AErtGps* G = AErtGps::Get(GetWorld()))
	{
		FVector T = FVector::ZeroVector;
		if (bWaypoint) T = Waypoint;
		else if (bGps && Director && Director->GetState() != EErtMissionState::Inactive)""")
# Saqlash/yuklash
rep('ErtGameMode.cpp', "\tR->SetNumberField(TEXT(\"gfx\"), GfxPreset);", "\tR->SetNumberField(TEXT(\"gfx\"), GfxPreset);\n\t{ FString Vs; Vs.Reserve(Visited.Num()); for (uint8 V : Visited) Vs.AppendChar(V ? TEXT('1') : TEXT('0')); R->SetStringField(TEXT(\"visited\"), Vs); }")
rep('ErtGameMode.cpp', "\t\tR->TryGetNumberField(TEXT(\"gfx\"), GfxPreset);", "\t\tR->TryGetNumberField(TEXT(\"gfx\"), GfxPreset);\n\t\t{ FString Vs; if (R->TryGetStringField(TEXT(\"visited\"), Vs) && Vs.Len() == Visited.Num()) for (int32 i = 0; i < Vs.Len(); ++i) Visited[i] = Vs[i] == TEXT('1') ? 1 : 0; }")

# ---------------- HUD: AC uslubidagi xarita ----------------
hd = io.open(SRC + 'ErtHUD.cpp', encoding='utf-8').read()
a = hd.find("\tif (M3 && M3->GetTexture())\n\t{\n\t\t// 3D relyef xaritasi")
b = hd.find("\telse DrawMapArea((SW - S) * 0.5f, 40 * Sc, S, 0.f, 0.f, 1000.f, true, Sc);", a)
assert a > 0 and b > 0
new_block = r'''	if (M3 && M3->GetTexture())
	{
		// AC/GTA uslubi: 3D relyef (render-tekstura), kashf qilinmagan hududlar qorong'i, joy belgilari, viloyat nomlari, yo'l nuqtasi
		const float X0 = (SW - S) * 0.5f, Y0 = 40 * Sc;
		AErtGameMode* GM = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
		if (GM) GM->MapRect = FVector(X0, Y0, S);
		FCanvasTileItem T(FVector2D(X0, Y0), M3->GetTexture()->GetResource(), FVector2D(S, S), FLinearColor::White); T.BlendMode = SE_BLEND_Opaque; Canvas->DrawItem(T);
		using namespace ErtMap;
		AErtWorldBuilder* W = Cast<AErtWorldBuilder>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtWorldBuilder::StaticClass()));
		auto Hz = [&](float E, float N) { return W ? W->HeightAt(E, N) : 0.f; };
		auto Proj = [&](float E, float N, float& X, float& Y) { float U, V; const bool bIn = M3->Project(AErtWorldBuilder::PlanToWorld(E, N, Hz(E, N)), U, V); X = X0 + U * S; Y = Y0 + V * S; return bIn; };
		// Kashf qilinmagan hududlar (fog of war): 50 m hujayralar
		if (GM)
		{
			const float CellPx = 5000.f * 0.02f / M3->GetOrtho() * S;   // hujayra piksel o'lchami (miniatyura K=0.02)
			for (int32 y = 0; y < AErtGameMode::VisN; ++y)
				for (int32 x = 0; x < AErtGameMode::VisN; ++x)
				{
					if (GM->Visited[y * AErtGameMode::VisN + x]) continue;
					const float E = -1000.f + (x + 0.5f) * 50.f, N = -1000.f + (y + 0.5f) * 50.f;
					float X, Y; if (!Proj(E, N, X, Y)) continue;
					FCanvasTileItem F(FVector2D(X - CellPx * 0.55f, Y - CellPx * 0.55f), FVector2D(CellPx * 1.1f, CellPx * 1.1f), FLinearColor(0.05f, 0.05f, 0.08f, 0.62f)); F.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(F);
				}
		}
		// Viloyat nomlari (xira, katta)
		const FLinearColor Region(0.95f, 0.9f, 0.75f, 0.35f);
		auto Rg = [&](float E, float N, const TCHAR* Name) { float X, Y; if (Proj(E, N, X, Y)) Text(Name, X - TextWidth(Name, 1.6f * Sc, true) * 0.5f, Y, Region, 1.6f * Sc, false, true); };
		Rg(-450.f, 450.f, TEXT("BITINIYA")); Rg(150.f, 150.f, TEXT("RUM SULTONLIGI")); Rg(400.f, -800.f, TEXT("SHOM")); Rg(600.f, -450.f, TEXT("MO'G'UL YERLARI")); Rg(400.f, 600.f, TEXT("ARMAN QIROLLIGI"));
		// Joy belgilari: 0 shahar (gumbaz), 1 qal'a (tishli), 2 qishloq/oba (uy), 3 lager (chodir), 4 ko'l (to'lqin), 5 tog'
		struct FPl { float E, N; int32 Kind; const TCHAR* Name; };
		const FPl Places[] = { {ObaE, ObaN, 2, TEXT("Qayi obasi")}, {FortE, FortN, 1, TEXT("Bagras")}, {CityE, CityN, 0, TEXT("Shahar")}, {CampE, CampN, 3, TEXT("Mo'g'ul lageri")}, {DamE, DamN, 0, TEXT("Damashq")}, {HalabE, HalabN, 0, TEXT("Halab")}, {KonE, KonN, 0, TEXT("Konya")}, {KayE, KayN, 0, TEXT("Qayseri")}, {SivE, SivN, 0, TEXT("Sivas")}, {ErzE, ErzN, 0, TEXT("Erzurum")}, {BurE, BurN, 0, TEXT("Bursa")}, {NikE, NikN, 0, TEXT("Nikeya")}, {KarE, KarN, 1, TEXT("Karacahisar")}, {SogE, SogN, 2, TEXT("So'g'ut")}, {DomE, DomN, 2, TEXT("Domaniç")}, {AskE, AskN, 4, TEXT("Askaniya")}, {LakeE, LakeN, 4, TEXT("Ko'l")}, {OasisE, OasisN, 4, TEXT("Voha")}, {CaravanE, CaravanN, 2, TEXT("Karvonsaroy")}, {UluE, UluN, 5, TEXT("Uludog'")}, {ErcE, ErcN, 5, TEXT("Erciyes")} };
		const FLinearColor Ink(0.98f, 0.95f, 0.85f), Unknown(0.7f, 0.7f, 0.65f);
		for (const FPl& P : Places)
		{
			float X, Y; if (!Proj(P.E, P.N, X, Y)) continue;
			const bool bKnown = !GM || GM->IsVisited(P.E, P.N);
			const FLinearColor C = bKnown ? Ink : Unknown;
			const float R = 7.f * Sc;
			auto Ln = [&](float ax, float ay, float bx, float by) { FCanvasLineItem L(FVector2D(X + ax * R, Y + ay * R), FVector2D(X + bx * R, Y + by * R)); L.SetColor(C); L.LineThickness = 2.f; Canvas->DrawItem(L); };
			// Fon doirasi
			Circle(X, Y, R * 1.35f, FLinearColor(0.08f, 0.07f, 0.06f, 0.75f), 16);
			switch (P.Kind)
			{
			case 0: Ln(-1, 0.8f, 1, 0.8f); Ln(-0.8f, 0.8f, -0.8f, 0); Ln(0.8f, 0.8f, 0.8f, 0); for (int32 k = 0; k < 6; ++k) { const float a0 = PI * k / 6, a1 = PI * (k + 1) / 6; Ln(-0.8f * FMath::Cos(a0), -0.8f * FMath::Sin(a0), -0.8f * FMath::Cos(a1), -0.8f * FMath::Sin(a1)); } Ln(0.9f, 0, 0.9f, -1.2f); break;   // gumbaz + minora
			case 1: Ln(-1, 0.9f, 1, 0.9f); Ln(-1, 0.9f, -1, -0.5f); Ln(1, 0.9f, 1, -0.5f); Ln(-1, -0.5f, -0.6f, -0.5f); Ln(-0.6f, -0.5f, -0.6f, -1); Ln(-0.6f, -1, -0.2f, -1); Ln(-0.2f, -1, -0.2f, -0.5f); Ln(-0.2f, -0.5f, 0.2f, -0.5f); Ln(0.2f, -0.5f, 0.2f, -1); Ln(0.2f, -1, 0.6f, -1); Ln(0.6f, -1, 0.6f, -0.5f); Ln(0.6f, -0.5f, 1, -0.5f); break;   // tishli qal'a
			case 2: Ln(-0.9f, 0.9f, 0.9f, 0.9f); Ln(-0.9f, 0.9f, -0.9f, -0.1f); Ln(0.9f, 0.9f, 0.9f, -0.1f); Ln(-1.1f, -0.1f, 0, -1); Ln(0, -1, 1.1f, -0.1f); break;   // uy
			case 3: Ln(-1, 0.9f, 1, 0.9f); Ln(-1, 0.9f, 0, -1); Ln(0, -1, 1, 0.9f); break;   // chodir
			case 4: for (int32 k = 0; k < 4; ++k) { Ln(-1 + k * 0.5f, (k & 1) ? -0.2f : 0.2f, -0.5f + k * 0.5f, (k & 1) ? 0.2f : -0.2f); } break;   // to'lqin
			default: Ln(-1, 0.9f, -0.3f, -0.6f); Ln(-0.3f, -0.6f, 0.1f, 0.1f); Ln(0.1f, 0.1f, 0.5f, -0.9f); Ln(0.5f, -0.9f, 1, 0.9f); break;   // tog'
			}
			Text(bKnown ? P.Name : TEXT("?"), X + R * 1.8f, Y - 8 * Sc, C, 0.85f * Sc, true, false);
		}
		// Maqsad markerlari, ittifoqchilar/dushmanlar, GPS
		if (AErtMissionDirector* Dm = Director())
		{
			TArray<FVector> Pts; Dm->GetMarkers(Pts);
			for (const FVector& P : Pts) { float U, V; if (M3->Project(P, U, V)) { const float Sz = 7 * Sc; FCanvasTileItem Tm(FVector2D(X0 + U * S - Sz, Y0 + V * S - Sz), FVector2D(Sz * 2, Sz * 2), FLinearColor(1.f, 0.8f, 0.2f)); Tm.Rotation = FRotator(0, 45.f, 0); Tm.PivotPoint = FVector2D(0.5f, 0.5f); Canvas->DrawItem(Tm); } }
			for (const AErtEnemy* E : Dm->GetEnemies()) if (E && !E->IsDead() && !E->IsAnimal()) { float U, V; if (M3->Project(E->GetActorLocation(), U, V)) Circle(X0 + U * S, Y0 + V * S, 3.f * Sc, FLinearColor(0.9f, 0.15f, 0.1f), 8); }
		}
		if (AErtGps* G = Cast<AErtGps>(UGameplayStatics::GetActorOfClass(GetWorld(), AErtGps::StaticClass())))
		{
			const TArray<FVector>& Path = G->GetPath();
			for (int32 i = 0; i + 1 < Path.Num(); ++i)
			{
				float U0, V0, U1, V1; const bool b0 = M3->Project(Path[i], U0, V0), b1 = M3->Project(Path[i + 1], U1, V1);
				if (!b0 && !b1) continue;
				FCanvasLineItem L(FVector2D(X0 + U0 * S, Y0 + V0 * S), FVector2D(X0 + U1 * S, Y0 + V1 * S)); L.SetColor(FLinearColor(0.3f, 0.85f, 1.f)); L.LineThickness = 3.f; Canvas->DrawItem(L);
			}
			if (G->HasPath()) Text(FString::Printf(TEXT("Yo'l: %.0f m"), G->GetPathLengthM()), X0 + 10 * Sc, Y0 + 10 * Sc, FLinearColor(0.3f, 0.85f, 1.f), Sc);
		}
		// Yo'l nuqtasi (AC uslubi: ko'k marker)
		if (GM && GM->bWaypoint) { float U, V; if (M3->Project(GM->Waypoint, U, V)) { const float X = X0 + U * S, Y = Y0 + V * S; Circle(X, Y, 8 * Sc, FLinearColor(0.2f, 0.7f, 1.f), 16); Circle(X, Y, 3 * Sc, FLinearColor(1, 1, 1), 8); FCanvasLineItem L(FVector2D(X, Y - 8 * Sc), FVector2D(X, Y - 24 * Sc)); L.SetColor(FLinearColor(0.2f, 0.7f, 1.f)); L.LineThickness = 3.f; Canvas->DrawItem(L); } }
		// O'yinchi (yashil uchburchak, yo'nalish bilan)
		if (APawn* P = GetOwningPawn()) { float U, V; if (M3->Project(P->GetActorLocation(), U, V)) { const float X = X0 + U * S, Y = Y0 + V * S, Yaw = FMath::DegreesToRadians(P->GetActorRotation().Yaw - M3->GetYaw() - 90.f); const float Sz = 9 * Sc; FCanvasTileItem Tp(FVector2D(X - Sz, Y - Sz * 1.6f), FVector2D(Sz * 2, Sz * 3.2f), FLinearColor(0.15f, 0.95f, 0.35f)); Tp.Rotation = FRotator(0, FMath::RadiansToDegrees(Yaw), 0); Tp.PivotPoint = FVector2D(0.5f, 0.5f); Canvas->DrawItem(Tp); Circle(X, Y, 3.5f * Sc, FLinearColor(0.02f, 0.3f, 0.1f), 10); } }
		// Kursor ostidagi koordinata
		if (GM && GM->MapMouse.X >= 0.f) { const FVector Wp = M3->Unproject(GM->MapMouse.X, GM->MapMouse.Y); Text(FString::Printf(TEXT("%.0f E  %.0f N   balandlik %.0f m"), Wp.Y / 100.f, Wp.X / 100.f, Wp.Z / 100.f), X0 + S - 260 * Sc, Y0 + 10 * Sc, FLinearColor(0.85f, 0.85f, 0.8f), 0.9f * Sc); }
		// Legenda
		{
			const float LX = X0 + 10 * Sc, LY = Y0 + S - 30 * Sc;
			FCanvasTileItem Bgl(FVector2D(LX - 6 * Sc, LY - 6 * Sc), FVector2D(560 * Sc, 26 * Sc), FLinearColor(0.02f, 0.02f, 0.03f, 0.6f)); Bgl.BlendMode = SE_BLEND_Translucent; Canvas->DrawItem(Bgl);
			Text(TEXT("yashil - siz   oltin - maqsad   ko'k - yo'l nuqtasi/yo'l   qizil - dushman   qorong'i - kashf qilinmagan   ? - noma'lum joy"), LX, LY, FLinearColor(0.8f, 0.8f, 0.75f), 0.85f * Sc);
		}
		// Kichik 2D xarita (o'ng-past burchak)
		DrawMapArea(SW - 230 * Sc, SH - 250 * Sc, 200 * Sc, 0.f, 0.f, 1000.f, false, Sc);
	}
'''
hd = hd[:a] + new_block + hd[b:]
hd = hd.replace("\tText(TEXT(\"3D XARITA   (M yoki Esc: yopish)   Chap/O'ng: aylantirish   Yuqori/Past: masshtab   yashil - siz, oltin - maqsad/GPS yo'li, qizil - dushman\"), 24 * Sc, SH - 28 * Sc, FLinearColor(0.6f, 0.6f, 0.55f), 0.9f * Sc);",
                "\tText(TEXT(\"XARITA   sichqoncha: sudrash - surish, g'ildirak - masshtab, o'ng tugma - yo'l nuqtasi (GPS)   WASD surish  Q/E aylantirish  Z/X egish  R - o'zimga   M/Esc yopish\"), 24 * Sc, SH - 28 * Sc, FLinearColor(0.6f, 0.6f, 0.55f), 0.9f * Sc);")
# Minimap va dunyo markeri: yo'l nuqtasi
hd = hd.replace("\t// Ittifoqchilar (urush)\n\tif (D) for (const AErtEnemy* E : D->GetAllies())",
"""	// Yo'l nuqtasi (minimap)
	if (AErtGameMode* GMw = Cast<AErtGameMode>(UGameplayStatics::GetGameMode(GetWorld()))) if (GMw->bWaypoint) { const float X = PX(GMw->Waypoint.Y / 100.f), Y = PY(GMw->Waypoint.X / 100.f); if (Inside(X, Y)) Circle(X, Y, 4 * Sc, FLinearColor(0.2f, 0.7f, 1.f), 10); }
	// Ittifoqchilar (urush)
	if (D) for (const AErtEnemy* E : D->GetAllies())""")
io.open(SRC + 'ErtHUD.cpp', 'w', encoding='utf-8', newline='\n').write(hd)
# Dunyo markeri (HUD asosiy): yo'l nuqtasi ko'k romb + masofa
rep('ErtHUD.cpp', "\t// --- markerlar (dunyo -> ekran) ---\n\tTArray<FVector> Pts; D->GetMarkers(Pts);\n\tif (H)\n\t{",
"""	// --- markerlar (dunyo -> ekran) ---
	TArray<FVector> Pts; D->GetMarkers(Pts);
	if (H && GM && GM->bWaypoint)
	{
		const FVector S = Project(GM->Waypoint + FVector(0, 0, 250.f));
		if (S.Z > 0.f) { const float Sz = 8 * Sc; FCanvasTileItem T(FVector2D(S.X - Sz, S.Y - Sz), FVector2D(Sz * 2, Sz * 2), FLinearColor(0.2f, 0.7f, 1.f)); T.Rotation = FRotator(0, 45.f, 0); T.PivotPoint = FVector2D(0.5f, 0.5f); Canvas->DrawItem(T); const FString DS = FString::Printf(TEXT("%.0f m"), FVector::Dist(GM->Waypoint, H->GetActorLocation()) / 100.f); Text(DS, S.X - TextWidth(DS, Sc, false) * 0.5f, S.Y + Sz + 2, FLinearColor(0.2f, 0.7f, 1.f), Sc); }
	}
	if (H)
	{""")
print('patched')
