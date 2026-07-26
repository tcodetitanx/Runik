// JSON save file: settings, campaign progress, best scores and saved
// machines per puzzle. Lives in Saved/SaveGames/runik_save.json.
#pragma once

#include "CoreMinimal.h"
#include "../Core/RunikCore.h"

struct FRunikPuzzleProgress
{
	bool bSolved = false;
	int32 BestCost = 0;
	int32 BestTicks = 0;
	int32 BestRunes = 0;
	std::vector<rk::Placement> SavedSolution;
};

class FRunikSave
{
public:
	// settings
	float MasterVolume = 0.8f;
	float SfxVolume = 1.0f;
	float MusicVolume = 0.6f;
	int32 WindowMode = 0;        // 0 fullscreen-windowed, 1 windowed, 2 fullscreen
	float SimSpeed = 1.0f;
	bool bShowBothPlanes = true;

	TMap<FString, FRunikPuzzleProgress> Progress;

	void Load();
	void Save() const;

	FRunikPuzzleProgress& For(const FString& PuzzleId)
	{
		return Progress.FindOrAdd(PuzzleId);
	}

	bool IsPuzzleUnlocked(int32 PuzzleIndex) const;
	int32 SolvedCount() const;

private:
	static FString FilePath();
};
