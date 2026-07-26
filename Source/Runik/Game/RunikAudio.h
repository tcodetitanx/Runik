// Loads the procedurally generated WAVs in Content/Audio and plays them
// through transient procedural sound waves. Music beds are retriggered by
// the UI on a timer so no looping asset support is required.
#pragma once

#include "CoreMinimal.h"

class UWorld;

class FRunikAudio
{
public:
	void LoadAll();

	// One-shot SFX. Volume is scaled by the user's sfx/master settings.
	void Play(UWorld* World, FName Clip, float Volume = 1.0f) const;
	// Ambient bed; returns the clip length in seconds (retrigger after this).
	float PlayMusic(UWorld* World, FName Clip, float Volume = 1.0f) const;

	float MasterVolume = 0.8f;
	float SfxVolume = 1.0f;
	float MusicVolume = 0.6f;

private:
	struct FClip
	{
		TArray<uint8> Pcm;      // 16-bit mono
		int32 SampleRate = 44100;
	};
	TMap<FName, FClip> Clips;

	float PlayInternal(UWorld* World, FName Clip, float Volume) const;
};
