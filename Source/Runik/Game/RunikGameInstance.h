#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RunikAssets.h"
#include "RunikAudio.h"
#include "RunikSave.h"
#include "RunikGameInstance.generated.h"

class UTexture2D;

UCLASS()
class URunikGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	FRunikAssets Assets;
	FRunikAudio Audio;
	FRunikSave SaveData;

	void SaveGame();
	void ApplyAudioSettings();
	void ApplyDisplaySettings();
	void PlaySfx(FName Clip, float Volume = 1.0f);

private:
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> KeepAliveTextures;
};
