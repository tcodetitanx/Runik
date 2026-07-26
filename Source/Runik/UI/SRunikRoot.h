// Root Slate widget: screen switching between main menu, campaign, story
// interstitials, the puzzle board, settings and credits.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class URunikGameInstance;
class SBox;

enum class ERunikScreen : uint8
{
	MainMenu, Campaign, Story, Puzzle, Settings, Credits
};

class SRunikRoot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRunikRoot) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, URunikGameInstance* InGI);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& Geometry,
	                         const FKeyEvent& KeyEvent) override;

	void ShowMainMenu();
	void ShowCampaign();
	void ShowStory(int32 PuzzleIndex);
	void ShowPuzzle(int32 PuzzleIndex);
	void ShowSettings();
	void ShowCredits();

private:
	URunikGameInstance* GI = nullptr;
	TSharedPtr<SBox> ContentBox;
	ERunikScreen Screen = ERunikScreen::MainMenu;
	int32 PendingPuzzle = -1;
	double MusicEndTime = -1.0;
	FName MusicClip;

	EActiveTimerReturnType MusicTick(double CurrentTime, float DeltaTime);
	void SetMusic(FName Clip);

	TSharedRef<SWidget> BuildMainMenu();
	TSharedRef<SWidget> BuildCampaign();
	TSharedRef<SWidget> BuildStory(int32 PuzzleIndex);
	TSharedRef<SWidget> BuildSettings();
	TSharedRef<SWidget> BuildCredits();
	TSharedRef<SWidget> BigButton(const FText& Label, FOnClicked OnClicked,
	                              bool bEnabled = true);
};
