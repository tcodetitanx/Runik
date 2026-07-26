#include "RunikSave.h"
#include "Runik.h"
#include "../Core/RunikPuzzles.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString FRunikSave::FilePath()
{
	return FPaths::ProjectSavedDir() / TEXT("SaveGames") / TEXT("runik_save.json");
}

void FRunikSave::Load()
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath()))
	{
		return; // fresh profile
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogRunik, Warning, TEXT("Corrupt save file, starting fresh"));
		return;
	}

	Root->TryGetNumberField(TEXT("masterVolume"), MasterVolume);
	Root->TryGetNumberField(TEXT("sfxVolume"), SfxVolume);
	Root->TryGetNumberField(TEXT("musicVolume"), MusicVolume);
	Root->TryGetNumberField(TEXT("windowMode"), WindowMode);
	Root->TryGetNumberField(TEXT("simSpeed"), SimSpeed);
	Root->TryGetBoolField(TEXT("showBothPlanes"), bShowBothPlanes);

	const TSharedPtr<FJsonObject>* ProgressObj;
	if (Root->TryGetObjectField(TEXT("progress"), ProgressObj))
	{
		for (const auto& Pair : (*ProgressObj)->Values)
		{
			const TSharedPtr<FJsonObject> P = Pair.Value->AsObject();
			if (!P.IsValid()) continue;
			FRunikPuzzleProgress Prog;
			P->TryGetBoolField(TEXT("solved"), Prog.bSolved);
			P->TryGetNumberField(TEXT("bestCost"), Prog.BestCost);
			P->TryGetNumberField(TEXT("bestTicks"), Prog.BestTicks);
			P->TryGetNumberField(TEXT("bestRunes"), Prog.BestRunes);
			const TArray<TSharedPtr<FJsonValue>>* Arr;
			if (P->TryGetArrayField(TEXT("solution"), Arr))
			{
				for (const TSharedPtr<FJsonValue>& V : *Arr)
				{
					const TSharedPtr<FJsonObject> O = V->AsObject();
					if (!O.IsValid()) continue;
					rk::Placement Pl;
					Pl.Rune = static_cast<rk::ERune>(
						static_cast<int32>(O->GetNumberField(TEXT("rune"))));
					Pl.Pos.Q = static_cast<int32>(O->GetNumberField(TEXT("q")));
					Pl.Pos.R = static_cast<int32>(O->GetNumberField(TEXT("r")));
					Pl.Plane = O->GetBoolField(TEXT("ethereal"))
						? rk::EPlane::Ethereal : rk::EPlane::Material;
					Pl.Priority = static_cast<int32>(O->GetNumberField(TEXT("prio")));
					Prog.SavedSolution.push_back(Pl);
				}
			}
			Progress.Add(Pair.Key, MoveTemp(Prog));
		}
	}
}

void FRunikSave::Save() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("masterVolume"), MasterVolume);
	Root->SetNumberField(TEXT("sfxVolume"), SfxVolume);
	Root->SetNumberField(TEXT("musicVolume"), MusicVolume);
	Root->SetNumberField(TEXT("windowMode"), WindowMode);
	Root->SetNumberField(TEXT("simSpeed"), SimSpeed);
	Root->SetBoolField(TEXT("showBothPlanes"), bShowBothPlanes);

	TSharedRef<FJsonObject> ProgressObj = MakeShared<FJsonObject>();
	for (const auto& Pair : Progress)
	{
		TSharedRef<FJsonObject> P = MakeShared<FJsonObject>();
		P->SetBoolField(TEXT("solved"), Pair.Value.bSolved);
		P->SetNumberField(TEXT("bestCost"), Pair.Value.BestCost);
		P->SetNumberField(TEXT("bestTicks"), Pair.Value.BestTicks);
		P->SetNumberField(TEXT("bestRunes"), Pair.Value.BestRunes);
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const rk::Placement& Pl : Pair.Value.SavedSolution)
		{
			TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetNumberField(TEXT("rune"), static_cast<int32>(Pl.Rune));
			O->SetNumberField(TEXT("q"), Pl.Pos.Q);
			O->SetNumberField(TEXT("r"), Pl.Pos.R);
			O->SetBoolField(TEXT("ethereal"), Pl.Plane == rk::EPlane::Ethereal);
			O->SetNumberField(TEXT("prio"), Pl.Priority);
			Arr.Add(MakeShared<FJsonValueObject>(O));
		}
		P->SetArrayField(TEXT("solution"), Arr);
		ProgressObj->SetObjectField(Pair.Key, P);
	}
	Root->SetObjectField(TEXT("progress"), ProgressObj);

	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	FFileHelper::SaveStringToFile(Out, *FilePath());
}

bool FRunikSave::IsPuzzleUnlocked(int32 PuzzleIndex) const
{
	// Linear campaign: a puzzle unlocks once the previous one is solved.
	if (PuzzleIndex <= 0) return true;
	const std::vector<rk::Puzzle>& All = rk::GetPuzzles();
	if (PuzzleIndex >= static_cast<int32>(All.size())) return false;
	const FString PrevId(All[PuzzleIndex - 1].Id.c_str());
	const FRunikPuzzleProgress* Prev = Progress.Find(PrevId);
	return Prev && Prev->bSolved;
}

int32 FRunikSave::SolvedCount() const
{
	int32 N = 0;
	for (const auto& Pair : Progress)
	{
		if (Pair.Value.bSolved) ++N;
	}
	return N;
}
