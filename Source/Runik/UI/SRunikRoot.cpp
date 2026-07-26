#include "SRunikRoot.h"
#include "RunikStyle.h"
#include "SPuzzleBoard.h"
#include "../Runik.h"
#include "../Game/RunikGameInstance.h"
#include "../Core/RunikPuzzles.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

using namespace RunikStyle;

void SRunikRoot::Construct(const FArguments& InArgs, URunikGameInstance* InGI)
{
	GI = InGI;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(GI->Assets.GetBrush(TEXT("bg_menu")))
		]
		+ SOverlay::Slot()
		[
			SAssignNew(ContentBox, SBox)
		]
	];

	RegisterActiveTimer(0.5f,
		FWidgetActiveTimerDelegate::CreateSP(this, &SRunikRoot::MusicTick));

	ShowMainMenu();
}

EActiveTimerReturnType SRunikRoot::MusicTick(double CurrentTime, float)
{
	if (!MusicClip.IsNone() && CurrentTime >= MusicEndTime)
	{
		const float Len = GI->Audio.PlayMusic(GI->GetWorld(), MusicClip);
		MusicEndTime = CurrentTime + FMath::Max(4.0f, Len - 0.25f);
	}
	return EActiveTimerReturnType::Continue;
}

void SRunikRoot::SetMusic(FName Clip)
{
	if (MusicClip != Clip)
	{
		MusicClip = Clip;
		MusicEndTime = -1.0; // retrigger on next tick
	}
}

FReply SRunikRoot::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		switch (Screen)
		{
		case ERunikScreen::MainMenu: break; // ignore; quitting is a button
		case ERunikScreen::Puzzle:   ShowCampaign(); return FReply::Handled();
		case ERunikScreen::Campaign: ShowMainMenu(); return FReply::Handled();
		default:                     ShowMainMenu(); return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

// ------------------------------------------------------------------ screens
void SRunikRoot::ShowMainMenu()
{
	Screen = ERunikScreen::MainMenu;
	SetMusic(TEXT("amb_menu"));
	ContentBox->SetContent(BuildMainMenu());
}

void SRunikRoot::ShowCampaign()
{
	Screen = ERunikScreen::Campaign;
	SetMusic(TEXT("amb_menu"));
	GI->SaveGame();
	ContentBox->SetContent(BuildCampaign());
}

void SRunikRoot::ShowStory(int32 PuzzleIndex)
{
	Screen = ERunikScreen::Story;
	PendingPuzzle = PuzzleIndex;
	ContentBox->SetContent(BuildStory(PuzzleIndex));
}

void SRunikRoot::ShowPuzzle(int32 PuzzleIndex)
{
	Screen = ERunikScreen::Puzzle;
	SetMusic(TEXT("amb_forge"));
	ContentBox->SetContent(
		SNew(SPuzzleBoard, GI, PuzzleIndex)
		.OnExit_Lambda([this]() { ShowCampaign(); }));
}

void SRunikRoot::ShowSettings()
{
	Screen = ERunikScreen::Settings;
	ContentBox->SetContent(BuildSettings());
}

void SRunikRoot::ShowCredits()
{
	Screen = ERunikScreen::Credits;
	ContentBox->SetContent(BuildCredits());
}

// ------------------------------------------------------------------ builders
TSharedRef<SWidget> SRunikRoot::BigButton(const FText& Label, FOnClicked OnClicked,
                                          bool bEnabled)
{
	return SNew(SBox)
		.WidthOverride(340)
		.HeightOverride(52)
		.Padding(FMargin(0, 4))
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.IsEnabled(bEnabled)
			.OnClicked(FOnClicked::CreateLambda([this, OnClicked]()
			{
				GI->PlaySfx(TEXT("sfx_click"));
				return OnClicked.IsBound() ? OnClicked.Execute() : FReply::Handled();
			}))
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(Font(18, true))
				.ColorAndOpacity(Parchment)
			]
		];
}

TSharedRef<SWidget> SRunikRoot::BuildMainMenu()
{
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().FillHeight(0.32f).VAlign(VAlign_Bottom).HAlign(HAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("Runik", "Title", "R U N I K"))
			.Font(Font(64, true))
			.ColorAndOpacity(Gold)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("Runik", "Subtitle",
				"an inscription in two worlds"))
			.Font(Font(16))
			.ColorAndOpacity(Azure)
		]
	]
	+ SVerticalBox::Slot().FillHeight(0.68f).VAlign(VAlign_Center).HAlign(HAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BigButton(NSLOCTEXT("Runik", "Play", "Campaign"),
				FOnClicked::CreateLambda([this]() { ShowCampaign(); return FReply::Handled(); }))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BigButton(NSLOCTEXT("Runik", "Settings", "Settings"),
				FOnClicked::CreateLambda([this]() { ShowSettings(); return FReply::Handled(); }))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BigButton(NSLOCTEXT("Runik", "Credits", "Credits"),
				FOnClicked::CreateLambda([this]() { ShowCredits(); return FReply::Handled(); }))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			BigButton(NSLOCTEXT("Runik", "Quit", "Quit"),
				FOnClicked::CreateLambda([this]()
				{
					if (APlayerController* PC = GI->GetFirstLocalPlayerController())
					{
						PC->ConsoleCommand(TEXT("quit"));
					}
					return FReply::Handled();
				}))
		]
	];
}

TSharedRef<SWidget> SRunikRoot::BuildCampaign()
{
	const std::vector<rk::Chapter>& Chapters = rk::GetChapters();
	const std::vector<rk::Puzzle>& Puzzles = rk::GetPuzzles();

	TSharedRef<SScrollBox> Scroll = SNew(SScrollBox);

	for (int32 C = 0; C < static_cast<int32>(Chapters.size()); ++C)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		for (int32 I = 0; I < static_cast<int32>(Puzzles.size()); ++I)
		{
			const rk::Puzzle& P = Puzzles[I];
			if (P.Chapter != C) continue;
			const FString Id(P.Id.c_str());
			const FRunikPuzzleProgress* Prog = GI->SaveData.Progress.Find(Id);
			const bool bSolved = Prog && Prog->bSolved;
			const bool bUnlocked = GI->SaveData.IsPuzzleUnlocked(I);

			FText Status;
			if (bSolved)
			{
				Status = FText::FromString(FString::Printf(
					TEXT("Solved  |  cost %d - ticks %d - runes %d"),
					Prog->BestCost, Prog->BestTicks, Prog->BestRunes));
			}
			else
			{
				Status = bUnlocked
					? NSLOCTEXT("Runik", "Unsolved", "Unsolved")
					: NSLOCTEXT("Runik", "Locked", "Locked");
			}

			Row->AddSlot().AutoWidth().Padding(8)
			[
				SNew(SBox).WidthOverride(280).HeightOverride(120)
				[
					SNew(SButton)
					.IsEnabled(bUnlocked)
					.OnClicked(FOnClicked::CreateLambda([this, I]()
					{
						GI->PlaySfx(TEXT("sfx_click"));
						ShowStory(I);
						return FReply::Handled();
					}))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4)
						[
							SNew(STextBlock)
							.Text(FText::FromString(UTF8_TO_TCHAR(P.Name.c_str())))
							.Font(Font(16, true))
							.ColorAndOpacity(bSolved ? Ok : (bUnlocked ? Gold : Dim))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(4, 0)
						[
							SNew(STextBlock)
							.Text(Status)
							.Font(Font(11))
							.ColorAndOpacity(bUnlocked ? Parchment : Dim)
						]
						+ SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Bottom).Padding(4)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("Par: cost %d - ticks %d - runes %d"),
								P.ParCost, P.ParTicks, P.ParRunes)))
							.Font(Font(10))
							.ColorAndOpacity(Dim)
						]
					]
				]
			];
		}

		Scroll->AddSlot().Padding(12, 18, 12, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Chapter %d - %s"),
				C + 1, UTF8_TO_TCHAR(Chapters[C].Name.c_str()))))
			.Font(Font(24, true))
			.ColorAndOpacity(Gold)
		];
		Scroll->AddSlot().Padding(12, 0)[Row];
	}

	return SNew(SBorder)
	.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
	.BorderBackgroundColor(FLinearColor(0, 0, 0, 0.55f))
	.Padding(24)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("Runik", "CampaignTitle", "The Journals of Alviss"))
				.Font(Font(30, true))
				.ColorAndOpacity(Parchment)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				BigButton(NSLOCTEXT("Runik", "Back", "Back"),
					FOnClicked::CreateLambda([this]() { ShowMainMenu(); return FReply::Handled(); }))
			]
		]
		+ SVerticalBox::Slot().FillHeight(1)[Scroll]
	];
}

TSharedRef<SWidget> SRunikRoot::BuildStory(int32 PuzzleIndex)
{
	const std::vector<rk::Puzzle>& Puzzles = rk::GetPuzzles();
	const rk::Puzzle& P = Puzzles[PuzzleIndex];
	const std::vector<rk::Chapter>& Chapters = rk::GetChapters();

	// Show the chapter prologue when entering its first puzzle.
	bool bFirstOfChapter = true;
	for (int32 I = 0; I < PuzzleIndex; ++I)
	{
		if (Puzzles[I].Chapter == P.Chapter) { bFirstOfChapter = false; break; }
	}

	FString Text;
	if (bFirstOfChapter)
	{
		Text += FString(UTF8_TO_TCHAR(Chapters[P.Chapter].Lore.c_str())) + TEXT("\n\n");
	}
	Text += UTF8_TO_TCHAR(P.Lore.c_str());
	Text += TEXT("\n\n- - -\n\n");
	Text += UTF8_TO_TCHAR(P.Brief.c_str());

	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(760)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(PanelBg)
			.Padding(36)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(UTF8_TO_TCHAR(P.Name.c_str())))
					.Font(Font(28, true))
					.ColorAndOpacity(Gold)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 18)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Text))
					.Font(Font(14))
					.ColorAndOpacity(Parchment)
					.AutoWrapText(true)
					.LineHeightPercentage(1.35f)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 12, 0, 0)
				[
					BigButton(NSLOCTEXT("Runik", "BeginWork", "Begin the Work"),
						FOnClicked::CreateLambda([this]()
						{
							ShowPuzzle(PendingPuzzle);
							return FReply::Handled();
						}))
				]
			]
		]
	];
}

TSharedRef<SWidget> SRunikRoot::BuildSettings()
{
	auto Row = [](const FText& Label, TSharedRef<SWidget> Control) -> TSharedRef<SWidget>
	{
		return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(Label).Font(Font(15)).ColorAndOpacity(Parchment)
		]
		+ SHorizontalBox::Slot().FillWidth(0.6f).VAlign(VAlign_Center)
		[
			Control
		];
	};

	FRunikSave& Save = GI->SaveData;

	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(640)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(PanelBg)
			.Padding(36)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 20)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("Runik", "SettingsTitle", "Settings"))
					.Font(Font(28, true)).ColorAndOpacity(Gold)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
				[
					Row(NSLOCTEXT("Runik", "Master", "Master volume"),
						SNew(SSlider)
						.Value(Save.MasterVolume)
						.OnValueChanged_Lambda([this](float V)
						{
							GI->SaveData.MasterVolume = V;
							GI->ApplyAudioSettings();
						})
						.OnMouseCaptureEnd_Lambda([this]() { GI->PlaySfx(TEXT("sfx_click")); }))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
				[
					Row(NSLOCTEXT("Runik", "Sfx", "Effects volume"),
						SNew(SSlider)
						.Value(Save.SfxVolume)
						.OnValueChanged_Lambda([this](float V)
						{
							GI->SaveData.SfxVolume = V;
							GI->ApplyAudioSettings();
						})
						.OnMouseCaptureEnd_Lambda([this]() { GI->PlaySfx(TEXT("sfx_fuse")); }))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
				[
					Row(NSLOCTEXT("Runik", "Music", "Music volume"),
						SNew(SSlider)
						.Value(Save.MusicVolume)
						.OnValueChanged_Lambda([this](float V)
						{
							GI->SaveData.MusicVolume = V;
							GI->ApplyAudioSettings();
						}))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
				[
					Row(NSLOCTEXT("Runik", "Speed", "Machine speed"),
						SNew(SSlider)
						.MinValue(0.25f).MaxValue(4.0f)
						.Value(Save.SimSpeed)
						.OnValueChanged_Lambda([this](float V)
						{
							GI->SaveData.SimSpeed = V;
						}))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
				[
					Row(NSLOCTEXT("Runik", "Window", "Window mode"),
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Runik", "Borderless", "Borderless"))
							.OnClicked_Lambda([this]()
							{
								GI->SaveData.WindowMode = 0;
								GI->ApplyDisplaySettings();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Runik", "Windowed", "Windowed"))
							.OnClicked_Lambda([this]()
							{
								GI->SaveData.WindowMode = 1;
								GI->ApplyDisplaySettings();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("Runik", "Fullscreen", "Fullscreen"))
							.OnClicked_Lambda([this]()
							{
								GI->SaveData.WindowMode = 2;
								GI->ApplyDisplaySettings();
								return FReply::Handled();
							})
						])
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 24, 0, 0)
				[
					BigButton(NSLOCTEXT("Runik", "BackSave", "Back"),
						FOnClicked::CreateLambda([this]()
						{
							GI->SaveGame();
							ShowMainMenu();
							return FReply::Handled();
						}))
				]
			]
		]
	];
}

TSharedRef<SWidget> SRunikRoot::BuildCredits()
{
	const FText CreditsText = NSLOCTEXT("Runik", "CreditsBody",
		"RUNIK\n\n"
		"A transmutation machine puzzle in two worlds.\n\n"
		"Design, code, art pipeline and sound - Bull Axiom\n"
		"Built with Unreal Engine\n\n"
		"Gem and board illustrations processed from the studio's asset "
		"library; rune glyphs drawn procedurally from the Elder Futhark.\n\n"
		"For Alviss, wherever he is.");

	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(620)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(PanelBg)
			.Padding(36)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(CreditsText)
					.Font(Font(15))
					.ColorAndOpacity(Parchment)
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
					.LineHeightPercentage(1.4f)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 24, 0, 0)
				[
					BigButton(NSLOCTEXT("Runik", "Back2", "Back"),
						FOnClicked::CreateLambda([this]() { ShowMainMenu(); return FReply::Handled(); }))
				]
			]
		]
	];
}
