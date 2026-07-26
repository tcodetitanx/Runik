#include "RunikGameMode.h"
#include "../Runik.h"
#include "RunikGameInstance.h"
#include "../UI/SRunikRoot.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SViewport.h"

ARunikGameMode::ARunikGameMode()
{
	DefaultPawnClass = nullptr;
}

void ARunikGameMode::BeginPlay()
{
	Super::BeginPlay();

	URunikGameInstance* GI = Cast<URunikGameInstance>(GetGameInstance());
	if (!GI || !GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogRunik, Error, TEXT("Runik boot failed: no game instance/viewport"));
		return;
	}

	RootWidget = SNew(SRunikRoot, GI);
	GEngine->GameViewport->AddViewportWidgetContent(RootWidget.ToSharedRef(), 10);

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = true;
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(RootWidget);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void ARunikGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (RootWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(RootWidget.ToSharedRef());
	}
	RootWidget.Reset();
	Super::EndPlay(Reason);
}
