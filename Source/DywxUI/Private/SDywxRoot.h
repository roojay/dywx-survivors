#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"
#include "DywxHUDState.h"
#include "DywxUIState.h"

class ADywxUIPlayerController;
class SConstraintCanvas;

class SDywxRoot : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDywxRoot) {} SLATE_ARGUMENT(TWeakObjectPtr<ADywxUIPlayerController>, Owner) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    void HandleEscape() { Back(); }
    void HandleDash() { Dash(); }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    void SetHUDState(const FDywxHUDState& State);
    void BeginLiveHUD(const FDywxHUDState& State);
    void ReturnToMenu();
    void SetEndlessUnlocked(bool bUnlocked);
private:
    TWeakObjectPtr<ADywxUIPlayerController> Owner;
    dywx::Navigation Nav;
    FDywxHUDState HUD;
    TMap<FString, TUniquePtr<FSlateBrush>> Brushes;
    TArray<TStrongObjectPtr<UTexture2D>> Textures;
    FButtonStyle NormalButton, PrimaryButton, FramedButton;
    FProgressBarStyle ProgressStyle;
    TSharedPtr<SConstraintCanvas> Canvas;
    float UIScale = 1.0f;
    FSlateFontInfo Font(int32 Size, bool bTitle = false) const;
    const FSlateBrush* Art(const FString& Filename);
    const FSlateBrush* Portrait(int32 Index);
    TSharedRef<SWidget> Label(const FString& Text, int32 Size = 24, FLinearColor Color = FLinearColor::White, bool bTitle = false) const;
    TSharedRef<SWidget> Paragraph(const FString& Text, float Width, int32 Size = 24, FLinearColor Color = FLinearColor::White, bool bTitle = false) const;
    TSharedRef<SWidget> Button(const FString& Text, TFunction<FReply()> Action, bool bPrimary = false, bool bEnabled = true, const FString& Tooltip = TEXT(""), bool bFramed = false);
    TSharedRef<SWidget> Panel(TSharedRef<SWidget> Content, FLinearColor Tint, float Padding = 24.0f) const;
    void At(float X, float Y, float W, float H, TSharedRef<SWidget> Widget);
    void Rebuild();
    void MainScreen();
    void CharacterScreen();
    void MapScreen();
    void BattleScreen();
    TSharedRef<SWidget> ModalContent();
    FReply Back();
    FReply Primary();
    FReply OpenModal(dywx::Modal Modal);
    FReply CloseModal();
    FReply Pause();
    FReply Dash();
    void SyncInput();
};
