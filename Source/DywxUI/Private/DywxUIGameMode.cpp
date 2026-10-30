#include "DywxUIGameMode.h"
#include "DywxUIPlayerController.h"
ADywxUIGameMode::ADywxUIGameMode()
{
    PlayerControllerClass = ADywxUIPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    HUDClass = nullptr;
}
