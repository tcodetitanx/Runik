#include "SPuzzleBoard.h"
#include "RunikStyle.h"
#include "../Runik.h"
#include "../Game/RunikGameInstance.h"
#include "../Game/RunikGif.h"
#include "../Core/RunikPuzzles.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

using namespace RunikStyle;

namespace
{

constexpr float SidePanelWidth = 250.0f;
constexpr float BottomBarHeight = 118.0f;
constexpr float TopMargin = 44.0f;

FLinearColor PlaneGlow(rk::EPlane Plane)
{
	return Plane == rk::EPlane::Material ? Gold : Azure;
}

// ---------------------------------------------------------------- histogram
// Pseudo community distribution around par, drawn as bars with the player's
// score marked. Deterministic per puzzle+metric.
class SRunikHistogram : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SRunikHistogram)
		: _Value(0), _Par(1), _Seed(1), _Label() {}
		SLATE_ARGUMENT(int32, Value)
		SLATE_ARGUMENT(int32, Par)
		SLATE_ARGUMENT(int32, Seed)
		SLATE_ARGUMENT(FText, Label)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Value = InArgs._Value;
		Par = FMath::Max(1, InArgs._Par);
		Seed = InArgs._Seed;
		Label = InArgs._Label;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(360, 74);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
	                      const FSlateRect& Rect, FSlateWindowElementList& Out,
	                      int32 LayerId, const FWidgetStyle& Style,
	                      bool bEnabled) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		constexpr int32 Buckets = 24;
		const float BarW = Size.X / Buckets;
		const float ChartH = Size.Y - 22.0f;

		// smooth deterministic "community" curve peaked a little above par
		FRandomStream Rng(Seed);
		float Heights[Buckets];
		float MaxH = 0.01f;
		const float Peak = Buckets * 0.35f;
		for (int32 i = 0; i < Buckets; ++i)
		{
			const float D = (i - Peak) / (Buckets * 0.28f);
			Heights[i] = FMath::Exp(-D * D) * (0.7f + 0.3f * Rng.FRand());
			MaxH = FMath::Max(MaxH, Heights[i]);
		}
		// map a metric value onto a bucket: par lands on the peak bucket
		auto BucketOf = [&](int32 V) -> int32
		{
			const float Rel = static_cast<float>(V) / static_cast<float>(Par);
			return FMath::Clamp(static_cast<int32>(Peak * Rel), 0, Buckets - 1);
		};
		const int32 PlayerBucket = BucketOf(Value);
		const int32 ParBucket = BucketOf(Par);

		for (int32 i = 0; i < Buckets; ++i)
		{
			const float H = ChartH * Heights[i] / MaxH;
			FLinearColor Color(0.35f, 0.32f, 0.28f, 1.0f);
			if (i == ParBucket) Color = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
			if (i == PlayerBucket) Color = Value <= Par ? Ok : Azure;
			FSlateDrawElement::MakeBox(Out, LayerId,
				Geometry.ToPaintGeometry(
					FVector2f(BarW - 2.0f, H),
					FSlateLayoutTransform(FVector2f(i * BarW + 1.0f, ChartH - H))),
				White, ESlateDrawEffect::None, Color);
		}

		const FString Caption = FString::Printf(TEXT("%s   you: %d   par: %d"),
			*Label.ToString(), Value, Par);
		FSlateDrawElement::MakeText(Out, LayerId + 1,
			Geometry.ToPaintGeometry(
				FVector2f(Size.X, 18.0f),
				FSlateLayoutTransform(FVector2f(0.0f, ChartH + 4.0f))),
			Caption, Font(11), ESlateDrawEffect::None, Parchment);
		return LayerId + 1;
	}

private:
	int32 Value = 0, Par = 1, Seed = 1;
	FText Label;
};

} // namespace

// ------------------------------------------------------------------ set up
void SPuzzleBoard::Construct(const FArguments& InArgs, URunikGameInstance* InGI,
                             int32 InPuzzleIndex)
{
	GI = InGI;
	PuzzleIndex = InPuzzleIndex;
	OnExitDelegate = InArgs._OnExit;

	const std::vector<rk::Puzzle>& All = rk::GetPuzzles();
	Puzzle = &All[FMath::Clamp(PuzzleIndex, 0, static_cast<int32>(All.size()) - 1)];

	// restore the saved machine for this puzzle, if any
	const FString Id(Puzzle->Id.c_str());
	if (const FRunikPuzzleProgress* Prog = GI->SaveData.Progress.Find(Id))
	{
		Placements = Prog->SavedSolution;
	}
	// default to the first allowed rune
	for (int32 i = 0; i < static_cast<int32>(rk::ERune::COUNT); ++i)
	{
		if (RuneAllowed(static_cast<rk::ERune>(i)))
		{
			SelectedRune = static_cast<rk::ERune>(i);
			break;
		}
	}

	RebuildPreview();

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(SidePanelWidth)[BuildSidePanel()]
			]
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SNew(SSpacer) // board painted behind by OnPaint
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(BottomBarHeight)[BuildBottomBar()]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SAssignNew(WinOverlayBox, SBox)
			.Visibility_Lambda([this]()
			{
				return bWinOverlayVisible ? EVisibility::Visible : EVisibility::Collapsed;
			})
		]
	];
}

void SPuzzleBoard::RebuildPreview()
{
	Sim.Setup(*Puzzle, Placements);
	Anim.Reset();
}

// ------------------------------------------------------------- sim control
void SPuzzleBoard::StartOrPause()
{
	if (Mode == EMode::Edit)
	{
		RebuildPreview();
		Mode = EMode::Run;
		GI->PlaySfx(TEXT("sfx_inscribe"));
	}
	else if (Mode == EMode::Run)
	{
		Mode = EMode::Pause;
	}
	else if (Mode == EMode::Pause)
	{
		Mode = EMode::Run;
	}
}

void SPuzzleBoard::StepOnce()
{
	if (Mode == EMode::Edit)
	{
		RebuildPreview();
		Mode = EMode::Pause;
	}
	if (Mode == EMode::Pause)
	{
		DoStep();
	}
}

void SPuzzleBoard::StopMachine()
{
	Mode = EMode::Edit;
	bWinOverlayVisible = false;
	RebuildPreview();
	PersistSolution();
}

void SPuzzleBoard::DoStep()
{
	Sim.Tick();
	TimeSinceStep = 0.0f;
	HandleStepEvents();
	if (Sim.IsWon() && Mode != EMode::Won)
	{
		RecordWin();
	}
}

void SPuzzleBoard::HandleStepEvents()
{
	Anim.Reset();
	bool bFuse = false, bDetach = false, bShift = false, bOutput = false, bSpawn = false;
	for (const rk::SimEvent& E : Sim.GetLastEvents())
	{
		switch (E.Type)
		{
		case rk::EEventType::Moved:
		case rk::EEventType::Rotated:
			Anim.Add(E.EntityId, TPair<rk::Hex, rk::Hex>(E.From, E.To));
			break;
		case rk::EEventType::Fused:    bFuse = true; break;
		case rk::EEventType::Detached: bDetach = true; break;
		case rk::EEventType::Shifted:  bShift = true; break;
		case rk::EEventType::Output:   bOutput = true; break;
		case rk::EEventType::Spawned:  bSpawn = true; break;
		default: break;
		}
	}
	UWorld* World = GI->GetWorld();
	if (bOutput) GI->Audio.Play(World, TEXT("sfx_output"));
	if (bFuse)   GI->Audio.Play(World, TEXT("sfx_fuse"), 0.8f);
	if (bDetach) GI->Audio.Play(World, TEXT("sfx_detach"), 0.8f);
	if (bShift)  GI->Audio.Play(World, TEXT("sfx_shift"), 0.7f);
	if (!bOutput && !bFuse && !bDetach && !bShift)
	{
		GI->Audio.Play(World, TEXT("sfx_tick"), 0.35f);
	}
}

void SPuzzleBoard::RecordWin()
{
	Mode = EMode::Won;
	WinMetrics = Sim.GetMetrics();
	bWinOverlayVisible = true;
	GifStatus.Empty();
	if (WinOverlayBox.IsValid())
	{
		WinOverlayBox->SetContent(BuildWinOverlay());
	}
	GI->Audio.Play(GI->GetWorld(), TEXT("sfx_win"));

	const FString Id(Puzzle->Id.c_str());
	FRunikPuzzleProgress& Prog = GI->SaveData.For(Id);
	if (!Prog.bSolved)
	{
		Prog.bSolved = true;
		Prog.BestCost = WinMetrics.Cost;
		Prog.BestTicks = WinMetrics.Ticks;
		Prog.BestRunes = WinMetrics.Runes;
	}
	else
	{
		Prog.BestCost = FMath::Min(Prog.BestCost, WinMetrics.Cost);
		Prog.BestTicks = FMath::Min(Prog.BestTicks, WinMetrics.Ticks);
		Prog.BestRunes = FMath::Min(Prog.BestRunes, WinMetrics.Runes);
	}
	Prog.SavedSolution = Placements;
	GI->SaveGame();
}

void SPuzzleBoard::PersistSolution()
{
	const FString Id(Puzzle->Id.c_str());
	GI->SaveData.For(Id).SavedSolution = Placements;
}

FReply SPuzzleBoard::ExportGif()
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Gifs");
	IFileManager::Get().MakeDirectory(*Dir, true);
	const FString Path = Dir / FString::Printf(TEXT("runik_%s.gif"),
		UTF8_TO_TCHAR(Puzzle->Id.c_str()));
	FRunikBoardRenderer Renderer(GI->Assets);
	const bool bOk = Renderer.ExportGif(*Puzzle, Placements, 80, Path);
	GifStatus = bOk
		? FString::Printf(TEXT("Saved: %s"), *FPaths::ConvertRelativePathToFull(Path))
		: TEXT("GIF export failed");
	GI->PlaySfx(TEXT("sfx_click"));
	return FReply::Handled();
}

// ---------------------------------------------------------------- editing
bool SPuzzleBoard::RuneAllowed(rk::ERune R) const
{
	return (Puzzle->AllowedRunes & rk::RuneBit(R)) != 0;
}

bool SPuzzleBoard::CanPlaceAt(const rk::Hex& H, rk::EPlane Plane) const
{
	if (rk::HexLen(H) > Puzzle->Radius) return false;
	for (const rk::FixedRune& F : Puzzle->FixedRunes)
	{
		if (F.Pos == H && F.Plane == Plane) return false;
	}
	for (const rk::Wall& W : Puzzle->Walls)
	{
		if (W.Pos == H && W.Plane == Plane) return false;
	}
	for (const rk::Spawner& S : Puzzle->Spawners)
	{
		for (const rk::SpawnCell& C : S.Cells)
		{
			if (S.Pos + C.Offset == H && S.Plane == Plane) return false;
		}
	}
	for (const rk::OutputZone& Z : Puzzle->Outputs)
	{
		for (const rk::OutputCell& C : Z.Cells)
		{
			if (C.Pos == H && Z.Plane == Plane) return false;
		}
	}
	return FindPlacementAt(H, Plane) < 0;
}

int32 SPuzzleBoard::FindPlacementAt(const rk::Hex& H, rk::EPlane Plane) const
{
	for (size_t i = 0; i < Placements.size(); ++i)
	{
		if (Placements[i].Pos == H && Placements[i].Plane == Plane)
		{
			return static_cast<int32>(i);
		}
	}
	return -1;
}

// --------------------------------------------------------------- geometry
float SPuzzleBoard::BoardHexSize(const FGeometry& Geometry) const
{
	const FVector2D Size = Geometry.GetLocalSize();
	const float AvailW = Size.X - SidePanelWidth - 40.0f;
	const float AvailH = Size.Y - BottomBarHeight - TopMargin - 16.0f;
	const float Cells = Puzzle->Radius * 2.0f + 2.0f;
	// pointy-top: a column of tiles advances 1.5*Size, a row RootThree*Size
	return FMath::Max(6.0f, FMath::Min(AvailW / (Cells * RootThree),
	                                   AvailH / (Cells * 1.5f)));
}

FVector2D SPuzzleBoard::BoardOrigin(const FGeometry& Geometry) const
{
	const FVector2D Size = Geometry.GetLocalSize();
	return FVector2D(SidePanelWidth + (Size.X - SidePanelWidth) * 0.5f,
	                 TopMargin + (Size.Y - BottomBarHeight - TopMargin) * 0.5f);
}

bool SPuzzleBoard::LocateHex(const FGeometry& Geometry, const FVector2D& Local,
                             rk::Hex& OutHex, rk::EPlane& OutPlane) const
{
	const float HexSize = BoardHexSize(Geometry);
	int32 Q, R;
	RunikStyle::PixelToHex(Local - BoardOrigin(Geometry), HexSize, Q, R);
	if (rk::HexLen(rk::Hex(Q, R)) <= Puzzle->Radius)
	{
		OutHex = rk::Hex(Q, R);
		OutPlane = CurrentPlane; // only the visible plane is interactive
		return true;
	}
	return false;
}

// ------------------------------------------------------------------- input
FReply SPuzzleBoard::OnMouseMove(const FGeometry& MyGeometry,
                                 const FPointerEvent& MouseEvent)
{
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	bHover = LocateHex(MyGeometry, Local, HoverHex, HoverPlane);
	return FReply::Unhandled();
}

FReply SPuzzleBoard::OnMouseButtonDown(const FGeometry& MyGeometry,
                                       const FPointerEvent& MouseEvent)
{
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
	if (!bHover || Mode == EMode::Run || Mode == EMode::Pause)
	{
		return FReply::Handled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const int32 Existing = FindPlacementAt(HoverHex, HoverPlane);
		if (Existing >= 0)
		{
			// cycle the rune's rank
			rk::Placement& P = Placements[Existing];
			P.Priority = P.Priority % 9 + 1;
			GI->PlaySfx(TEXT("sfx_click"));
			RebuildPreview();
		}
		else if (RuneAllowed(SelectedRune) && CanPlaceAt(HoverHex, HoverPlane))
		{
			rk::Placement P;
			P.Rune = SelectedRune;
			P.Pos = HoverHex;
			P.Plane = HoverPlane;
			P.Priority = CurrentPriority;
			Placements.push_back(P);
			GI->PlaySfx(TEXT("sfx_place"));
			RebuildPreview();
		}
		else
		{
			GI->PlaySfx(TEXT("sfx_error"), 0.5f);
		}
		return FReply::Handled();
	}
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const int32 Existing = FindPlacementAt(HoverHex, HoverPlane);
		if (Existing >= 0)
		{
			Placements.erase(Placements.begin() + Existing);
			GI->PlaySfx(TEXT("sfx_detach"), 0.5f);
			RebuildPreview();
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SPuzzleBoard::OnMouseWheel(const FGeometry& MyGeometry,
                                  const FPointerEvent& MouseEvent)
{
	if (bHover && Mode == EMode::Edit)
	{
		const int32 Existing = FindPlacementAt(HoverHex, HoverPlane);
		if (Existing >= 0)
		{
			rk::Placement& P = Placements[Existing];
			P.Priority = FMath::Clamp(
				P.Priority + (MouseEvent.GetWheelDelta() > 0 ? 1 : -1), 1, 9);
			GI->PlaySfx(TEXT("sfx_tick"), 0.5f);
			RebuildPreview();
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

FReply SPuzzleBoard::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	const FKey Key = KeyEvent.GetKey();
	if (Key == EKeys::SpaceBar) { StartOrPause(); return FReply::Handled(); }
	if (Key == EKeys::S)        { StepOnce(); return FReply::Handled(); }
	if (Key == EKeys::R)        { StopMachine(); return FReply::Handled(); }
	if (Key == EKeys::Tab)
	{
		CurrentPlane = rk::OtherPlane(CurrentPlane);
		GI->PlaySfx(TEXT("sfx_click"), 0.6f);
		return FReply::Handled();
	}
	static const FKey DigitKeys[9] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	for (int32 N = 0; N < 9; ++N)
	{
		if (Key == DigitKeys[N])
		{
			CurrentPriority = N + 1;
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

// ------------------------------------------------------------------- tick
void SPuzzleBoard::Tick(const FGeometry& AllottedGeometry,
                        const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (Mode == EMode::Run)
	{
		TimeSinceStep += InDeltaTime;
		const float Interval = 0.6f / FMath::Max(0.25f, GI->SaveData.SimSpeed);
		if (TimeSinceStep >= Interval)
		{
			DoStep();
		}
	}
}

// ------------------------------------------------------------------ paint
int32 SPuzzleBoard::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
                            const FSlateRect& MyCullingRect,
                            FSlateWindowElementList& Out, int32 LayerId,
                            const FWidgetStyle& InWidgetStyle,
                            bool bParentEnabled) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
	const float HexSize = BoardHexSize(Geometry);
	const FVector2D LocalSize = Geometry.GetLocalSize();

	// backdrop
	FSlateDrawElement::MakeBox(Out, LayerId,
		Geometry.ToPaintGeometry(FVector2f(LocalSize),
			FSlateLayoutTransform(FVector2f(0, 0))),
		White, ESlateDrawEffect::None, BoardBg);
	++LayerId;

	const float Interval = 0.6f / FMath::Max(0.25f, GI->SaveData.SimSpeed);
	const float LerpAlpha = (Mode == EMode::Run)
		? FMath::Clamp(TimeSinceStep / Interval, 0.0f, 1.0f) : 1.0f;

	// Aspect-preserving sprite draw: Width is the on-screen width, height
	// follows the sprite's own proportions, centered on the hex center.
	auto DrawSprite = [&](FName Sprite, const FVector2D& Center, float Width,
	                      const FLinearColor& Tint, int32 Layer)
	{
		const FRunikSprite* S = GI->Assets.Get(Sprite);
		if (!S || S->Width <= 0) return;
		const float H = Width * static_cast<float>(S->Height) / S->Width;
		FSlateDrawElement::MakeBox(Out, Layer,
			Geometry.ToPaintGeometry(
				FVector2f(Width, H),
				FSlateLayoutTransform(
					FVector2f(Center - FVector2D(Width * 0.5f, H * 0.5f)))),
			&S->Brush, ESlateDrawEffect::None, Tint);
	};

	// Only one plane is visible at a time; Tab / the plane button crosses over.
	const rk::EPlane Plane = CurrentPlane;
	const FVector2D Origin = BoardOrigin(Geometry);
	const float TileW = HexSize * RootThree; // pointy-top tile width
	const float RuneW = TileW * 0.72f * 0.66f;

	// plane label
	{
		const FString LabelText = Plane == rk::EPlane::Material
			? TEXT("M A T E R I A L   -   sunwise          [Tab] cross the veil")
			: TEXT("E T H E R E A L   -   widdershins          [Tab] cross the veil");
		FSlateDrawElement::MakeText(Out, LayerId,
			Geometry.ToPaintGeometry(FVector2f(600, 24),
				FSlateLayoutTransform(FVector2f(Origin.X - 220.0f, 12.0f))),
			LabelText, Font(13, true), ESlateDrawEffect::None, PlaneGlow(Plane));
	}

	// board tiles
	for (int32 Q = -Puzzle->Radius; Q <= Puzzle->Radius; ++Q)
	{
		for (int32 R = -Puzzle->Radius; R <= Puzzle->Radius; ++R)
		{
			if (rk::HexLen(rk::Hex(Q, R)) > Puzzle->Radius) continue;
			const FVector2D C = Origin + RunikStyle::HexToPixel(Q, R, HexSize);
			DrawSprite(TEXT("hex_base"), C, TileW * 1.01f,
				Plane == rk::EPlane::Material
					? FLinearColor(1, 1, 1, 1)
					: FLinearColor(0.75f, 0.85f, 1.1f, 1.0f),
				LayerId);
		}
	}
	++LayerId;

	// spawners (ghost gems)
	for (const rk::Spawner& S : Puzzle->Spawners)
	{
		if (S.Plane != Plane) continue;
		for (const rk::SpawnCell& Cell : S.Cells)
		{
			const rk::Hex H = S.Pos + Cell.Offset;
			const FVector2D C = Origin + RunikStyle::HexToPixel(H.Q, H.R, HexSize);
			DrawSprite(FRunikAssets::GemSpriteName(static_cast<int32>(Cell.Gem)),
				C, TileW * 0.8f, FLinearColor(1, 1, 1, 0.22f), LayerId);
		}
	}
	// output zones (ghost gems, green tint)
	for (const rk::OutputZone& Z : Puzzle->Outputs)
	{
		if (Z.Plane != Plane) continue;
		for (const rk::OutputCell& Cell : Z.Cells)
		{
			const FVector2D C = Origin +
				RunikStyle::HexToPixel(Cell.Pos.Q, Cell.Pos.R, HexSize);
			DrawSprite(TEXT("hex_base"), C, TileW * 0.95f,
				FLinearColor(0.5f, 1.0f, 0.55f, 0.30f), LayerId);
			DrawSprite(FRunikAssets::GemSpriteName(static_cast<int32>(Cell.Gem)),
				C, TileW * 0.8f, FLinearColor(0.7f, 1.0f, 0.7f, 0.35f), LayerId);
		}
	}
	++LayerId;

	// bonds
	{
		TArray<FVector2f> Line;
		for (const auto& Bond : Sim.GetBonds())
		{
			const std::vector<rk::Entity>& Ents = Sim.GetEntities();
			const rk::Entity& A = Ents[Bond.first];
			const rk::Entity& B = Ents[Bond.second];
			if (!A.bAlive || !B.bAlive || A.Plane != Plane) continue;
			Line.Reset();
			Line.Add(FVector2f(Origin + RunikStyle::HexToPixel(A.Pos.Q, A.Pos.R, HexSize)));
			Line.Add(FVector2f(Origin + RunikStyle::HexToPixel(B.Pos.Q, B.Pos.R, HexSize)));
			FSlateDrawElement::MakeLines(Out, LayerId,
				Geometry.ToPaintGeometry(), Line, ESlateDrawEffect::None,
				FLinearColor(0.95f, 0.9f, 0.75f, 0.7f), true, HexSize * 0.18f);
		}
	}
	++LayerId;

	// entities
	for (const rk::Entity& E : Sim.GetEntities())
	{
		if (!E.bAlive || E.Plane != Plane) continue;
		FVector2D Pos = RunikStyle::HexToPixel(E.Pos.Q, E.Pos.R, HexSize);
		if (const TPair<rk::Hex, rk::Hex>* Move = Anim.Find(E.Id))
		{
			const FVector2D From =
				RunikStyle::HexToPixel(Move->Key.Q, Move->Key.R, HexSize);
			const FVector2D To =
				RunikStyle::HexToPixel(Move->Value.Q, Move->Value.R, HexSize);
			Pos = FMath::Lerp(From, To, FMath::SmoothStep(0.0f, 1.0f, LerpAlpha));
		}
		const FVector2D C = Origin + Pos;
		if (E.Kind == rk::EKind::Gem)
		{
			DrawSprite(FRunikAssets::GemSpriteName(static_cast<int32>(E.Gem)),
				C, TileW * 0.84f,
				E.bStatic ? FLinearColor(0.5f, 0.5f, 0.55f, 1.0f)
				          : FLinearColor::White, LayerId);
		}
		else
		{
			const FLinearColor Glow = PlaneGlow(E.Plane);
			DrawSprite(FRunikAssets::RuneSpriteName(static_cast<int32>(E.Rune)),
				C, RuneW, Glow, LayerId);
			// rank digit
			FSlateDrawElement::MakeText(Out, LayerId + 1,
				Geometry.ToPaintGeometry(FVector2f(24, 16),
					FSlateLayoutTransform(FVector2f(
						C + FVector2D(HexSize * 0.34f, HexSize * 0.30f)))),
				FString::FromInt(E.Priority), Font(11, true),
				ESlateDrawEffect::None,
				E.bStatic ? Dim : Glow);
		}
	}
	LayerId += 2;

	// hover ghost
	if (bHover && Mode == EMode::Edit && HoverPlane == Plane)
	{
		const FVector2D C = Origin +
			RunikStyle::HexToPixel(HoverHex.Q, HoverHex.R, HexSize);
		const bool bOk = CanPlaceAt(HoverHex, HoverPlane) && RuneAllowed(SelectedRune);
		DrawSprite(FRunikAssets::RuneSpriteName(static_cast<int32>(SelectedRune)),
			C, RuneW,
			(bOk ? PlaneGlow(Plane) : Danger).CopyWithNewOpacity(0.5f), LayerId);
	}
	++LayerId;

	return SCompoundWidget::OnPaint(Args, Geometry, MyCullingRect, Out,
	                                LayerId + 1, InWidgetStyle, bParentEnabled);
}

// ------------------------------------------------------------------ panels
FText SPuzzleBoard::StatusText() const
{
	switch (Mode)
	{
	case EMode::Run:   return NSLOCTEXT("Runik", "StRun", "The machine turns...");
	case EMode::Pause: return NSLOCTEXT("Runik", "StPause", "Held between breaths");
	case EMode::Won:   return NSLOCTEXT("Runik", "StWon", "The work is complete");
	default:           return NSLOCTEXT("Runik", "StEdit", "Inscribing");
	}
}

TSharedRef<SWidget> SPuzzleBoard::IconButton(FName Icon, const FText& Tooltip,
                                             FOnClicked OnClicked)
{
	return SNew(SBox).WidthOverride(46).HeightOverride(46).Padding(2)
	[
		SNew(SButton)
		.ToolTipText(Tooltip)
		.OnClicked(OnClicked)
		.ContentPadding(6)
		[
			SNew(SImage).Image(GI->Assets.GetBrush(Icon))
		]
	];
}

TSharedRef<SWidget> SPuzzleBoard::RuneButton(rk::ERune R)
{
	const bool bAllowed = RuneAllowed(R);
	return SNew(SBox).WidthOverride(74).HeightOverride(92).Padding(3)
	[
		SNew(SButton)
		.IsEnabled(bAllowed)
		.OnClicked_Lambda([this, R]()
		{
			SelectedRune = R;
			GI->PlaySfx(TEXT("sfx_click"), 0.7f);
			return FReply::Handled();
		})
		.ButtonColorAndOpacity_Lambda([this, R]()
		{
			return SelectedRune == R ? FLinearColor(1.0f, 0.9f, 0.6f, 1.0f)
			                         : FLinearColor::White;
		})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1)
			[
				SNew(SImage)
				.Image(GI->Assets.GetBrush(
					FRunikAssets::RuneSpriteName(static_cast<int32>(R))))
				.ColorAndOpacity_Lambda([this]() { return PlaneGlow(CurrentPlane); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(
					FString(UTF8_TO_TCHAR(rk::RuneName(R)))
						.Replace(TEXT(" ("), TEXT("\n("))))
				.Font(Font(9))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(bAllowed ? Parchment : Dim)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), rk::RuneCost(R))))
				.Font(Font(10, true))
				.ColorAndOpacity(Gold)
			]
		]
	];
}

TSharedRef<SWidget> SPuzzleBoard::BuildSidePanel()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	Box->AddSlot().AutoHeight().Padding(14, 14, 14, 4)
	[
		SNew(STextBlock)
		.Text(FText::FromString(UTF8_TO_TCHAR(Puzzle->Name.c_str())))
		.Font(Font(19, true)).ColorAndOpacity(Gold).AutoWrapText(true)
	];
	Box->AddSlot().AutoHeight().Padding(14, 4)
	[
		SNew(STextBlock)
		.Text(FText::FromString(UTF8_TO_TCHAR(Puzzle->Brief.c_str())))
		.Font(Font(11)).ColorAndOpacity(Parchment).AutoWrapText(true)
		.LineHeightPercentage(1.3f)
	];
	Box->AddSlot().AutoHeight().Padding(14, 12, 14, 2)
	[
		SNew(STextBlock)
		.Text_Lambda([this]()
		{
			return FText::FromString(FString::Printf(TEXT("Deliveries: %d / %d"),
				Sim.GetOutputsDone(), Puzzle->RequiredOutputs));
		})
		.Font(Font(14, true)).ColorAndOpacity(Ok)
	];
	Box->AddSlot().AutoHeight().Padding(14, 2)
	[
		SNew(STextBlock)
		.Text_Lambda([this]()
		{
			const rk::Metrics M = Sim.GetMetrics();
			return FText::FromString(FString::Printf(
				TEXT("Tick %d   Cost %d   Runes %d"),
				Sim.GetTicks(), M.Cost, M.Runes));
		})
		.Font(Font(11)).ColorAndOpacity(Dim)
	];
	Box->AddSlot().AutoHeight().Padding(14, 10, 14, 2)
	[
		SNew(STextBlock)
		.Text_Lambda([this]() { return StatusText(); })
		.Font(Font(12)).ColorAndOpacity(Azure)
	];
	Box->AddSlot().FillHeight(1)[SNew(SSpacer)];
	Box->AddSlot().AutoHeight().Padding(14, 6)
	[
		SNew(STextBlock)
		.Text(NSLOCTEXT("Runik", "Help",
			"Left click: inscribe / cycle rank\n"
			"Right click: erase\n"
			"Wheel: adjust rank\n"
			"Tab: switch plane   1-9: rank\n"
			"Space: run/pause   S: step   R: reset"))
		.Font(Font(10)).ColorAndOpacity(Dim).LineHeightPercentage(1.35f)
	];
	Box->AddSlot().AutoHeight().Padding(14, 8, 14, 14)
	[
		SNew(SButton)
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([this]()
		{
			PersistSolution();
			GI->SaveGame();
			OnExitDelegate.ExecuteIfBound();
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("Runik", "LeaveBoard", "Back to the Journals"))
			.Font(Font(13, true)).ColorAndOpacity(Parchment)
		]
	];

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(PanelBg)
		[
			Box
		];
}

TSharedRef<SWidget> SPuzzleBoard::BuildBottomBar()
{
	TSharedRef<SHorizontalBox> Palette = SNew(SHorizontalBox);
	for (int32 i = 0; i < static_cast<int32>(rk::ERune::COUNT); ++i)
	{
		Palette->AddSlot().AutoWidth()[RuneButton(static_cast<rk::ERune>(i))];
	}

	return SNew(SBorder)
	.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
	.BorderBackgroundColor(PanelBg)
	.Padding(FMargin(10, 6))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Palette
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0, 8, 0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("Rank %d"),
						CurrentPriority));
				})
				.Font(Font(13, true))
				.ColorAndOpacity_Lambda([this]() { return PlaneGlow(CurrentPlane); })
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("-")))
					.OnClicked_Lambda([this]()
					{
						CurrentPriority = FMath::Max(1, CurrentPriority - 1);
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("+")))
					.OnClicked_Lambda([this]()
					{
						CurrentPriority = FMath::Min(9, CurrentPriority + 1);
						return FReply::Handled();
					})
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
		[
			SNew(SButton)
			.ToolTipText(NSLOCTEXT("Runik", "PlaneTip",
				"Which plane new runes are inscribed on (Tab)"))
			.OnClicked_Lambda([this]()
			{
				CurrentPlane = rk::OtherPlane(CurrentPlane);
				GI->PlaySfx(TEXT("sfx_shift"), 0.4f);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return CurrentPlane == rk::EPlane::Material
						? NSLOCTEXT("Runik", "PlaneM", "Material plane")
						: NSLOCTEXT("Runik", "PlaneE", "Ethereal plane");
				})
				.Font(Font(13, true))
				.ColorAndOpacity_Lambda([this]() { return PlaneGlow(CurrentPlane); })
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1)[SNew(SSpacer)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				IconButton(TEXT("icon_play"),
					NSLOCTEXT("Runik", "RunTip", "Run / pause the machine (Space)"),
					FOnClicked::CreateLambda([this]()
					{
						StartOrPause();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				IconButton(TEXT("icon_step"),
					NSLOCTEXT("Runik", "StepTip", "Advance one tick (S)"),
					FOnClicked::CreateLambda([this]()
					{
						StepOnce();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				IconButton(TEXT("icon_stop"),
					NSLOCTEXT("Runik", "StopTip", "Stop and return to inscribing (R)"),
					FOnClicked::CreateLambda([this]()
					{
						StopMachine();
						return FReply::Handled();
					}))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
			[
				IconButton(TEXT("icon_gif"),
					NSLOCTEXT("Runik", "GifTip", "Export the running machine as a GIF"),
					FOnClicked::CreateLambda([this]() { return ExportGif(); }))
			]
		]
	];
}

TSharedRef<SWidget> SPuzzleBoard::BuildWinOverlay()
{
	const int32 SeedBase = static_cast<int32>(GetTypeHash(FString(Puzzle->Id.c_str())));
	return SNew(SBox).WidthOverride(460)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(PanelBg)
		.Padding(28)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("Runik", "WinTitle", "The Work Is Complete"))
				.Font(Font(24, true)).ColorAndOpacity(Gold)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 2)
			[
				SNew(SRunikHistogram)
				.Value(WinMetrics.Cost).Par(Puzzle->ParCost)
				.Seed(SeedBase + 1)
				.Label(NSLOCTEXT("Runik", "HCost", "COST"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 2)
			[
				SNew(SRunikHistogram)
				.Value(WinMetrics.Ticks).Par(Puzzle->ParTicks)
				.Seed(SeedBase + 2)
				.Label(NSLOCTEXT("Runik", "HTicks", "TICKS"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 2)
			[
				SNew(SRunikHistogram)
				.Value(WinMetrics.Runes).Par(Puzzle->ParRunes)
				.Seed(SeedBase + 3)
				.Label(NSLOCTEXT("Runik", "HRunes", "RUNES"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 12)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(GifStatus); })
				.Font(Font(10)).ColorAndOpacity(Azure).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("Runik", "ExportGif", "Export GIF"))
					.OnClicked_Lambda([this]() { return ExportGif(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("Runik", "Tinker", "Keep Tinkering"))
					.OnClicked_Lambda([this]()
					{
						bWinOverlayVisible = false;
						StopMachine();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("Runik", "Continue", "Continue"))
					.OnClicked_Lambda([this]()
					{
						OnExitDelegate.ExecuteIfBound();
						return FReply::Handled();
					})
				]
			]
		]
	];
}
