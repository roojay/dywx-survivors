#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/Texture2D.h"
#include "DywxHUDState.h"
#include "DywxUIState.h"
#include "MVP/Types.h"

class ADywxUIPlayerController;
class SConstraintCanvas;

class SDywxRoot : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDywxRoot) {} SLATE_ARGUMENT(TWeakObjectPtr<ADywxUIPlayerController>, Owner) SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override;
    void HandleEscape() { Back(); }
    // Idempotent: never toggle/close an existing modal on focus loss.
    void HandleFocusLoss();
    void HandleDash() { Dash(); }
    void HandleUpgradeSelection(int32 Index);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    void SetSnapshot(const dywx::mvp::Snapshot& State);
    void RefreshRunUI();
    void SetHUDState(const FDywxHUDState& State);
    void BeginLiveHUD(const FDywxHUDState& State);
    void ReturnToMenu();
    void ShowNotice(const FString& Text) { RunNotice=Text;Rebuild(); }
    void SetEndlessUnlocked(bool bUnlocked);
private:
    TWeakObjectPtr<ADywxUIPlayerController> Owner;
    dywx::Navigation Nav;
    FDywxHUDState HUD;
    dywx::mvp::Snapshot Run;
    bool bBeastVisible = false;
    dywx::mvp::Vec2 BeastOffset;
    float BossChangeUntil = 0;
    float BossHP = 0, BossMaxHP = 1;
    uint64 UIEpoch = 0;
    uint64 SelectedBag = 0;
    int32 SelectedSlot = 0;
    int32 AspectFilter = -1, AffixFilter = -1;
    int32 RarityFilter = -1;
    bool bRefinePreview = false;
    bool bRefineStones = false;
    bool bModalPausedRun = false;
    FString RunNotice;
    TSharedRef<SWidget> LiveText(TFunction<FString()> Value, int32 Size = 22, FLinearColor Color = FLinearColor::White);
    bool bFusionOpen = false;
    int32 SelectedUpgrade = -1;
    dywx::mvp::UpgradeOffer SelectedOffer{};
    uint64 SelectedUpgradeGeneration = 0;
    int32 SelectedUpgradePending = 0;
    FReply ConfirmUpgrade();
    void UpgradePanel();
    void RestPanel();
    void ResultPanel();
    FReply RunCommand(TFunction<void()> Command);
    TMap<FString, TUniquePtr<FSlateBrush>> Brushes;
    TArray<TStrongObjectPtr<UTexture2D>> Textures;
    FButtonStyle NormalButton, PrimaryButton, FramedButton, RowNormalButton, RowPrimaryButton;
    FProgressBarStyle ProgressStyle;
    TSharedPtr<SConstraintCanvas> Canvas;
    float UIScale = 1.0f;
    double ResolutionScale = 1.0;
    FSlateFontInfo Font(int32 Size, bool bTitle = false) const;
    const FSlateBrush* Art(const FString& Filename);
    const FSlateBrush* Portrait(int32 Index);
    const FSlateBrush* ConceptCell(const FString& Filename,int32 Index);
    const FSlateBrush* FacePortrait(int32 Index);
    TSharedRef<SWidget> Framed(TSharedRef<SWidget> Content,bool bSelected=false);
    const FSlateBrush* CombatIcon(int32 Spell);
    TSharedRef<SWidget> CombatButton(const FString& Text, TFunction<FReply()> Action);
    TSharedRef<SWidget> Label(const FString& Text, int32 Size = 24, FLinearColor Color = FLinearColor::White, bool bTitle = false) const;
    TSharedRef<SWidget> Paragraph(const FString& Text, float Width, int32 Size = 24, FLinearColor Color = FLinearColor::White, bool bTitle = false) const;
    TSharedRef<SWidget> Button(const FString& Text, TFunction<FReply()> Action, bool bPrimary = false, bool bEnabled = true, const FString& Tooltip = TEXT(""), bool bFramed = false, TSharedPtr<SWidget> TooltipContent = nullptr);
    TSharedRef<SWidget> Comparison(const dywx::mvp::Stats& Before,const dywx::mvp::Stats& After,const FString& Description=TEXT("")) const;
    TSharedRef<SWidget> Panel(TSharedRef<SWidget> Content, FLinearColor Tint, float Padding = 24.0f) const;
    void At(float X, float Y, float W, float H, TSharedRef<SWidget> Widget);
    void Rebuild();
    void SceneSidebar();
    bool bAdvancedSettings = false;
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
