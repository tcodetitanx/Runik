#include "RunikAudio.h"
#include "../Runik.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{

// Minimal RIFF/WAVE parser for the 16-bit mono PCM files we generate.
bool ParseWav(const TArray<uint8>& Data, TArray<uint8>& OutPcm, int32& OutRate)
{
	if (Data.Num() < 44 || FMemory::Memcmp(Data.GetData(), "RIFF", 4) != 0 ||
	    FMemory::Memcmp(Data.GetData() + 8, "WAVE", 4) != 0)
	{
		return false;
	}
	int32 Pos = 12;
	OutRate = 44100;
	while (Pos + 8 <= Data.Num())
	{
		const uint8* P = Data.GetData() + Pos;
		const uint32 ChunkSize = P[4] | (P[5] << 8) | (P[6] << 16) | (P[7] << 24);
		if (FMemory::Memcmp(P, "fmt ", 4) == 0 && ChunkSize >= 16)
		{
			const uint8* F = P + 8;
			OutRate = F[4] | (F[5] << 8) | (F[6] << 16) | (F[7] << 24);
		}
		else if (FMemory::Memcmp(P, "data", 4) == 0)
		{
			const int32 Avail = FMath::Min<int32>(ChunkSize, Data.Num() - Pos - 8);
			OutPcm.Append(P + 8, Avail);
			return Avail > 0;
		}
		Pos += 8 + ChunkSize + (ChunkSize & 1);
	}
	return false;
}

} // namespace

void FRunikAudio::LoadAll()
{
	const FString Dir = FPaths::ProjectContentDir() / TEXT("Audio");
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.wav")), true, false);
	for (const FString& File : Files)
	{
		TArray<uint8> Data;
		if (!FFileHelper::LoadFileToArray(Data, *(Dir / File)))
		{
			continue;
		}
		FClip Clip;
		if (ParseWav(Data, Clip.Pcm, Clip.SampleRate))
		{
			Clips.Add(FName(*FPaths::GetBaseFilename(File)), MoveTemp(Clip));
		}
		else
		{
			UE_LOG(LogRunik, Warning, TEXT("Unsupported wav: %s"), *File);
		}
	}
	UE_LOG(LogRunik, Log, TEXT("Loaded %d audio clips"), Clips.Num());
}

float FRunikAudio::PlayInternal(UWorld* World, FName Name, float Volume) const
{
	const FClip* Clip = Clips.Find(Name);
	if (!Clip || !World || Volume <= 0.01f)
	{
		return 0.0f;
	}
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>();
	Wave->SetSampleRate(Clip->SampleRate);
	Wave->NumChannels = 1;
	const float Duration = Clip->Pcm.Num() / 2.0f / Clip->SampleRate;
	Wave->Duration = Duration;
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->bLooping = false;
	Wave->QueueAudio(Clip->Pcm.GetData(), Clip->Pcm.Num());
	UGameplayStatics::PlaySound2D(World, Wave, Volume);
	return Duration;
}

void FRunikAudio::Play(UWorld* World, FName Clip, float Volume) const
{
	PlayInternal(World, Clip, Volume * SfxVolume * MasterVolume);
}

float FRunikAudio::PlayMusic(UWorld* World, FName Clip, float Volume) const
{
	return PlayInternal(World, Clip, Volume * MusicVolume * MasterVolume);
}
