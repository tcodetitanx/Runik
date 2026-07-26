#include "RunikGameInstance.h"
#include "Runik.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"

void URunikGameInstance::Init()
{
	Super::Init();
	SaveData.Load();
	Assets.LoadAll(KeepAliveTextures);
	Audio.LoadAll();
	ApplyAudioSettings();
	ApplyDisplaySettings();
	UE_LOG(LogRunik, Log, TEXT("Runik initialized: %d puzzles solved"),
	       SaveData.SolvedCount());
}

void URunikGameInstance::Shutdown()
{
	SaveData.Save();
	Super::Shutdown();
}

void URunikGameInstance::SaveGame()
{
	SaveData.Save();
}

void URunikGameInstance::ApplyAudioSettings()
{
	Audio.MasterVolume = SaveData.MasterVolume;
	Audio.SfxVolume = SaveData.SfxVolume;
	Audio.MusicVolume = SaveData.MusicVolume;
}

void URunikGameInstance::ApplyDisplaySettings()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		switch (SaveData.WindowMode)
		{
		case 1:  Settings->SetFullscreenMode(EWindowMode::Windowed); break;
		case 2:  Settings->SetFullscreenMode(EWindowMode::Fullscreen); break;
		default: Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen); break;
		}
		Settings->ApplySettings(false);
	}
}

void URunikGameInstance::PlaySfx(FName Clip, float Volume)
{
	Audio.Play(GetWorld(), Clip, Volume);
}
