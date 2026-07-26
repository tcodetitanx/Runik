#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RunikGameMode.generated.h"

// Runik is a pure UI game: the game mode only bootstraps the Slate root
// widget on top of an empty engine map.
UCLASS()
class ARunikGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARunikGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	TSharedPtr<class SRunikRoot> RootWidget;
};
