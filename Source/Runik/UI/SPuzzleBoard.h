// The Runik workbench: hex board renderer for both planes, rune inscription
// editor, machine simulation controls, win report with histogram comparison
// and animated GIF export.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "../Core/RunikSim.h"

class URunikGameInstance;

class SPuzzleBoard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPuzzleBoard) {}
		SLATE_EVENT(FSimpleDelegate, OnExit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, URunikGameInstance* InGI,
	               int32 InPuzzleIndex);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
	                  const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements, int32 LayerId,
	                      const FWidgetStyle& InWidgetStyle,
	                      bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
	                                 const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry,
	                           const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry,
	                            const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry,
	                         const FKeyEvent& KeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	enum class EMode : uint8 { Edit, Run, Pause, Won };

	URunikGameInstance* GI = nullptr;
	int32 PuzzleIndex = 0;
	const rk::Puzzle* Puzzle = nullptr;
	std::vector<rk::Placement> Placements;
	rk::Sim Sim;
	EMode Mode = EMode::Edit;
	FSimpleDelegate OnExitDelegate;

	rk::ERune SelectedRune = rk::ERune::Pull;
	int32 CurrentPriority = 1;
	rk::EPlane CurrentPlane = rk::EPlane::Material;

	float TimeSinceStep = 0.0f;
	// entity id -> (from,to) hex of the last step, for movement tweening
	TMap<int32, TPair<rk::Hex, rk::Hex>> Anim;

	bool bHover = false;
	rk::Hex HoverHex;
	rk::EPlane HoverPlane = rk::EPlane::Material;

	bool bWinOverlayVisible = false;
	rk::Metrics WinMetrics;
	FString GifStatus;
	TSharedPtr<class SBox> WinOverlayBox;

	// ---- machine control
	void RebuildPreview();
	void StartOrPause();
	void StepOnce();
	void StopMachine();
	void DoStep();
	void HandleStepEvents();
	void RecordWin();
	void PersistSolution();
	FReply ExportGif();

	// ---- editing
	bool RuneAllowed(rk::ERune R) const;
	bool CanPlaceAt(const rk::Hex& H, rk::EPlane Plane) const;
	int32 FindPlacementAt(const rk::Hex& H, rk::EPlane Plane) const;

	// ---- geometry
	float BoardHexSize(const FGeometry& Geometry) const;
	FVector2D PlaneOrigin(int32 PlaneIdx, const FGeometry& Geometry) const;
	bool LocateHex(const FGeometry& Geometry, const FVector2D& Local,
	               rk::Hex& OutHex, rk::EPlane& OutPlane) const;

	// ---- widget construction
	TSharedRef<SWidget> BuildSidePanel();
	TSharedRef<SWidget> BuildBottomBar();
	TSharedRef<SWidget> BuildWinOverlay();
	TSharedRef<SWidget> RuneButton(rk::ERune R);
	TSharedRef<SWidget> IconButton(FName Icon, const FText& Tooltip,
	                               FOnClicked OnClicked);

	FText StatusText() const;
};
