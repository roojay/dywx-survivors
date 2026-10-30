#include "SDywxRoot.h"
#include "DywxUIPlayerController.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FLinearColor Gold(0.88f, 0.64f, 0.30f, 1.0f);
const FLinearColor Paper(0.94f, 0.90f, 0.80f, 1.0f);
const FLinearColor Muted(0.60f, 0.66f, 0.69f, 1.0f);
const FLinearColor Ink(0.015f, 0.026f, 0.045f, 0.94f);
const TCHAR* Names[] = {TEXT("断岳行者"), TEXT("巡星羽客"), TEXT("司霆方士"), TEXT("归灵祝师")};
const TCHAR* Roles[] = {TEXT("近身挥斩 · 积势清场"), TEXT("远程穿行 · 引星破阵"), TEXT("雷法连锁 · 镇伏妖邪"), TEXT("灵契相随 · 生息不绝")};
const TCHAR* SkillNames[] = {TEXT("断岳三叠"), TEXT("回风刃环"), TEXT("穿云重矢")};
const TCHAR* SkillArt[] = {TEXT("icon-slash.png"), TEXT("icon-orbit.png"), TEXT("icon-arrow.png")};
}

void SDywxRoot::Construct(const FArguments& Args)
{
    Owner = Args._Owner;
    NormalButton = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
    NormalButton.SetNormal(FSlateColorBrush(FLinearColor(0.01f, 0.02f, 0.035f, 0.32f)));
    NormalButton.SetHovered(FSlateColorBrush(FLinearColor(0.16f, 0.12f, 0.065f, 0.95f)));
    NormalButton.SetPressed(FSlateColorBrush(FLinearColor(0.24f, 0.16f, 0.06f, 1.0f)));
    NormalButton.SetDisabled(FSlateColorBrush(FLinearColor(0.02f, 0.025f, 0.035f, 0.8f)));
    NormalButton.SetNormalPadding(FMargin(16, 4));
    NormalButton.SetPressedPadding(FMargin(16, 5, 16, 3));
    PrimaryButton = NormalButton;
    PrimaryButton.SetNormal(FSlateColorBrush(FLinearColor(0.10f, 0.065f, 0.025f, 0.90f)));
    FramedButton = PrimaryButton;
    FSlateBrush Frame = *Art(TEXT("button-gold.png"));
    Frame.SetUVRegion(FBox2f(FVector2f(0,0.21f),FVector2f(1,0.77f)));
    // This asset contains a complete frame plus transparent top/bottom gutters,
    // not a nine-slice texture. Image draws the cropped frame across the full slot;
    // Box slicing used texture-space borders and compressed the centre/text area.
    Frame.DrawAs = ESlateBrushDrawType::Image;
    Frame.ImageSize = FVector2D(490,90);
    Frame.Margin = FMargin(0);
    FramedButton.SetNormal(Frame);
    Frame.TintColor = FSlateColor(FLinearColor(1.2f,1.12f,0.85f));
    FramedButton.SetHovered(Frame);
    Frame.TintColor = FSlateColor(FLinearColor(0.80f,0.68f,0.45f));
    FramedButton.SetPressed(Frame);
    Frame.TintColor = FSlateColor(FLinearColor(0.45f,0.42f,0.36f));
    FramedButton.SetDisabled(Frame);
    ProgressStyle = FCoreStyle::Get().GetWidgetStyle<FProgressBarStyle>("ProgressBar");
    ProgressStyle.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.015f, 0.025f, 0.04f, 0.95f)));
    ProgressStyle.SetFillImage(FSlateColorBrush(FLinearColor::White));
    Rebuild();
}
FSlateFontInfo SDywxRoot::Font(int32 Size, bool bTitle) const
{
    const FString File = FPaths::ProjectContentDir() / TEXT("UI/Fonts") /
        (bTitle ? TEXT("SarasaUiSC-Bold.ttf") : TEXT("SarasaUiSC-Regular.ttf"));
    return FSlateFontInfo(File, FMath::RoundToInt(Size * UIScale), EFontHinting::Default, FFontOutlineSettings());
}
const FSlateBrush* SDywxRoot::Art(const FString& Filename)
{
    if (auto* Existing = Brushes.Find(Filename)) return Existing->Get();
    auto Brush = MakeUnique<FSlateBrush>();
    const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("UI/Images") / Filename);
    if (UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(File))
    {
        Texture->SRGB = true;
        Texture->Filter = TF_Bilinear;
        Texture->UpdateResource();
        Brush->SetResourceObject(Texture);
        Brush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
        Brush->DrawAs = ESlateBrushDrawType::Image;
        Textures.Emplace(Texture); // Strong references prevent GC while Slate holds the brush.
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DYWX UI image unavailable: %s"), *File);
        *Brush = FSlateColorBrush(FLinearColor(0.035f, 0.045f, 0.065f));
    }
    const FSlateBrush* Result = Brush.Get();
    Brushes.Add(Filename, MoveTemp(Brush));
    return Result;
}
const FSlateBrush* SDywxRoot::Portrait(int32 Index)
{
    const FString Key = FString::Printf(TEXT("portrait-%d"), Index);
    if (auto* Existing = Brushes.Find(Key)) return Existing->Get();
    auto Brush = MakeUnique<FSlateBrush>(*Art(TEXT("character-portraits.png")));
    Brush->SetUVRegion(FBox2f(FVector2f(Index * 0.25f, 0), FVector2f((Index + 1) * 0.25f, 1)));
    Brush->ImageSize = FVector2D(443.5f, 887.0f);
    const FSlateBrush* Result = Brush.Get();
    Brushes.Add(Key, MoveTemp(Brush));
    return Result;
}
TSharedRef<SWidget> SDywxRoot::Label(const FString& Text, int32 Size, FLinearColor Color, bool bTitle) const
{
    return SNew(STextBlock).Text(FText::FromString(Text)).Font(Font(Size, bTitle))
        .ColorAndOpacity(Color).AutoWrapText(false).ShadowColorAndOpacity(FLinearColor(0,0,0,0.8f)).ShadowOffset(FVector2D(1,2));
}
TSharedRef<SWidget> SDywxRoot::Paragraph(const FString& Text, float Width, int32 Size, FLinearColor Color, bool bTitle) const
{
    // Explicit reference-canvas width avoids the one-frame AutoWrap desired-size
    // feedback loop. Only prose wraps; titles, controls and HUD values never do.
    return SNew(STextBlock).Text(FText::FromString(Text)).Font(Font(Size, bTitle))
        .ColorAndOpacity(Color).AutoWrapText(false).WrapTextAt(Width)
        .ShadowColorAndOpacity(FLinearColor(0,0,0,0.8f)).ShadowOffset(FVector2D(1,2));
}
TSharedRef<SWidget> SDywxRoot::Panel(TSharedRef<SWidget> Content, FLinearColor Tint, float Padding) const
{
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(Tint).Padding(Padding)[Content];
}
TSharedRef<SWidget> SDywxRoot::Button(const FString& Text, TFunction<FReply()> Action, bool bPrimary, bool bEnabled, const FString& Tooltip, bool bFramed)
{
    // Thin gold outline for compact selected controls; secondary controls have only
    // a bottom rule, not the opaque grey toolkit rectangles from the first build.
    const FMargin Rule = bFramed ? FMargin(0) : (bPrimary ? FMargin(1) : FMargin(0,0,0,1));
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(bFramed ? FLinearColor::Transparent : FLinearColor(0.44f,0.35f,0.21f,bPrimary?0.9f:0.4f))
        .Padding(Rule).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
        [SNew(SButton).ButtonStyle(bFramed ? &FramedButton : (bPrimary ? &PrimaryButton : &NormalButton))
            .ButtonColorAndOpacity(FLinearColor::White).ForegroundColor(Paper)
            .IsEnabled(bEnabled).HAlign(HAlign_Fill).VAlign(VAlign_Center)
            .ToolTipText(FText::FromString(Tooltip))
            .OnClicked_Lambda([Fn = MoveTemp(Action)]() mutable { return Fn(); })
            [SNew(STextBlock).Text(FText::FromString(Text)).Font(Font(bPrimary ? 30 : 24,true))
                .AutoWrapText(false).Justification(ETextJustify::Center)
                .ColorAndOpacity(bEnabled ? Paper : Muted)
                .ShadowColorAndOpacity(FLinearColor(0,0,0,0.8f)).ShadowOffset(FVector2D(1,2))]];
}
void SDywxRoot::At(float X, float Y, float W, float H, TSharedRef<SWidget> Widget)
{
    Canvas->AddSlot().Anchors(FAnchors(0,0)).Alignment(FVector2D::ZeroVector)
        .Offset(FMargin(X,Y,W,H))[Widget];
}
void SDywxRoot::Rebuild()
{
    SAssignNew(Canvas, SConstraintCanvas);
    const TCHAR* Background = TEXT("main-background.png");
    if (Nav.screen == dywx::Screen::Character) Background = TEXT("character-background.png");
    if (Nav.screen == dywx::Screen::Map) Background = TEXT("map-background.png");
    if (Nav.screen == dywx::Screen::Battle) Background = TEXT("battle-background.png");
    if (!(Nav.screen == dywx::Screen::Battle && Nav.liveBattle))
        At(0,0,1920,1080,SNew(SImage).Image(Art(Background)).Visibility(EVisibility::HitTestInvisible));
    switch (Nav.screen) {
    case dywx::Screen::Main: MainScreen(); break;
    case dywx::Screen::Character: CharacterScreen(); break;
    case dywx::Screen::Map: MapScreen(); break;
    case dywx::Screen::Battle: BattleScreen(); break;
    }
    if (Nav.screen != dywx::Screen::Battle)
        At(1450,1020,420,35, Label(TEXT("道衍万象 · 界面原型"),18,Muted));
    if (Nav.screen == dywx::Screen::Character || Nav.screen == dywx::Screen::Map)
        At(60,1005,190,60,Button(TEXT("Esc  返回"),[this]{ return Back(); }));
    auto Overlay = SNew(SOverlay)
        + SOverlay::Slot()[Canvas.ToSharedRef()];
    if (Nav.modal != dywx::Modal::None)
    {
        Canvas->SetEnabled(false);
        Overlay->AddSlot()[Panel(SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[ModalContent()],FLinearColor(0,0,0,0.72f),0)];
    }
    ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::Both)
        [SNew(SBox).WidthOverride(1920).HeightOverride(1080)[Overlay]]];
    if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::SetDirectly);
}
void SDywxRoot::MainScreen()
{
    At(60,90,660,870,Panel(SNew(SBox),FLinearColor(0.01f,0.02f,0.035f,0.22f),0));
    At(110,155,610,155,Label(TEXT("道衍万象"),82,Paper,true));
    At(125,320,530,55,Label(TEXT("神话为表 · 万象待解"),24,Gold,true));
    At(120,455,490,90,Button(TEXT("开始修行"),[this]{ return Primary(); },true,true,TEXT(""),true));
    At(145,580,440,70,Button(TEXT("道典"),[this]{ return OpenModal(dywx::Modal::Codex); }));
    At(145,685,440,70,Button(TEXT("设置"),[this]{ return OpenModal(dywx::Modal::Settings); }));
    At(145,790,440,70,Button(TEXT("退出游戏"),[this]{ return OpenModal(dywx::Modal::Quit); }));
    At(80,1020,950,40,Label(TEXT("Enter 确认   ·   Tab 切换焦点   ·   Esc 返回"),20,Muted));
}
void SDywxRoot::CharacterScreen()
{
    At(55,45,800,130,Label(TEXT("选择修士"),65,Paper,true));
    At(70,160,500,45,Label(TEXT("角色预览"),23,Gold));
    for (int32 Index=0; Index<4; ++Index)
    {
        At(65,265+Index*160,465,130,
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Nav.character==Index?Gold:FLinearColor(0.2f,0.22f,0.25f)).Padding(2)
            [SNew(SButton).ButtonStyle(Nav.character==Index?&PrimaryButton:&NormalButton).ContentPadding(0).ButtonColorAndOpacity(FLinearColor::White).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
                .OnClicked_Lambda([this,Index]{Nav.SelectCharacter(Index);Rebuild();return FReply::Handled();})
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(105)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Portrait(Index))]]]
                    +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(16,0)
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight()[Label(Names[Index],24,Paper,true)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)
                            [Label(Index==0?TEXT("当前可用"):TEXT("构想 · 暂未开放"),18,Muted)]]]]);
    }
    At(1370,300,505,650,Panel(SNew(SBox),Ink,0));
    At(1400,340,450,100,Label(Names[Nav.character],48,Paper,true));
    At(1400,450,440,80,Label(Roles[Nav.character],24,Gold));
    if (Nav.character == 0)
    {
        At(1410,560,85,85,SNew(SImage).Image(Art(TEXT("icon-slash.png"))));
        At(1515,580,300,65,Label(TEXT("断岳三叠"),28,Paper,true));
        At(1410,680,410,90,Label(TEXT("自动挥斩 · 走位与遁术"),24,Paper));
    }
    else
    {
        At(655,220,560,735,Panel(SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Portrait(Nav.character))],Ink,12));
        At(1400,560,430,190,Paragraph(TEXT("外观与能力尚在设计。当前仅开放断岳行者的界面预览。"),430,24,Muted));
    }
    At(1395,820,450,85,Button(TEXT("选择洞天"),[this]{return Primary();},true,Nav.character==0,TEXT("仅断岳行者可进入当前预览"),true));
    At(670,975,570,42,Label(Nav.character == 0 ? TEXT("美术占位 · 非最终角色模型") : TEXT("构想角色 · 暂无对应模型"),20,Muted));
}
void SDywxRoot::MapScreen()
{
    At(55,55,800,130,Label(TEXT("选择洞天"),65,Paper,true));
    At(1360,0,560,1080,Panel(SNew(SBox),Ink,0));
    At(1410,190,460,110,Label(TEXT("栖雷古坛"),56,Paper,true));
    At(1420,345,430,120,Paragraph(TEXT("地纹先亮，雷霆后至。\n引敌入雷，借势破劫。"),430,26,Paper,true));
    const TCHAR* Modes[] = {TEXT("快速"),TEXT("传统"),TEXT("无尽")};
    for (int32 Index=0; Index<3; ++Index)
    {
        const auto Mode = static_cast<dywx::Mode>(Index);
        At(1395+Index*160,530,145,95,Button(Modes[Index],[this,Mode]{Nav.SelectMode(Mode);Rebuild();return FReply::Handled();},Nav.mode==Mode,Index!=2||Nav.endlessUnlocked));
    }
    At(1410,655,460,70,Paragraph(Nav.endlessUnlocked ? TEXT("模式只影响开局请求，未接入玩法") : TEXT("无尽 · 待解锁（预览状态）"),460,21,Muted));
    At(1400,750,465,80,Button(FString(Names[Nav.character])+TEXT("  ›"),[this]{Nav.Back();Rebuild();return FReply::Handled();}));
    At(1390,875,480,100,Button(TEXT("进入洞天"),[this]{return Primary();},true,true,TEXT(""),true));
    const TCHAR* Maps[] = {TEXT("栖雷古坛"),TEXT("回春玉圃"),TEXT("归声石窟")};
    for (int32 Index=0; Index<3; ++Index)
    {
        const float X = 60.0f + Index * 425.0f;
        At(X,755,395,165,Panel(Index==0 ? StaticCastSharedRef<SWidget>(SNew(SImage).Image(Art(TEXT("map-background.png")))) : Label(TEXT("后续内容"),30,Muted,true),FLinearColor(0.02f,0.03f,0.04f,0.90f),6));
        At(X,930,395,70,Button(Maps[Index],[this,Index]{Nav.SelectMap(Index);Rebuild();return FReply::Handled();},Index==Nav.map,Index==0,TEXT("当前仅开放栖雷古坛")));
    }
}
void SDywxRoot::BattleScreen()
{
    At(45,30,470,92,Panel(SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
        [SNew(STextBlock).Font(Font(24)).ColorAndOpacity(Paper).Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("心元  %.0f / %.0f"),HUD.Health,HUD.MaxHealth));})]
        +SVerticalBox::Slot().FillHeight(1)
        [SNew(SProgressBar).Style(&ProgressStyle).FillColorAndOpacity(FLinearColor(0.85f,0.12f,0.035f)).Percent_Lambda([this]{return TOptional<float>(dywx::SafeFraction(HUD.Health,HUD.MaxHealth));})],FLinearColor(0.01f,0.02f,0.035f,0.8f),12));
    At(825,25,280,110,Panel(SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(Font(25,true)).ColorAndOpacity(Paper).Justification(ETextJustify::Center).Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("第 %d 波"),HUD.Wave));})]
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(Font(25)).ColorAndOpacity(Gold).Justification(ETextJustify::Center).Text_Lambda([this]{const int32 S=FMath::FloorToInt(HUD.ElapsedSeconds);return FText::FromString(FString::Printf(TEXT("%02d:%02d"),S/60,S%60));})],FLinearColor(0.01f,0.02f,0.035f,0.8f),8));
    At(1640,35,230,65,Button(TEXT("暂停  Esc"),[this]{return Pause();}));
    if (!Nav.liveBattle) At(660,145,650,58,Panel(Label(TEXT("HUD 预览 · 静态示例数据 · 非实机战斗"),18,Paper),FLinearColor(0.02f,0.025f,0.04f,0.85f),8));
    for (int32 Index=0; Index<3; ++Index)
    {
        const float X=680.0f+Index*175.0f;
        At(X,885,110,110,Panel(SNew(SImage).Image(Art(SkillArt[Index])),FLinearColor(0.015f,0.025f,0.04f,0.9f),8));
        At(X-24,1000,175,40,SNew(STextBlock).Font(Font(19)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)
            .Text_Lambda([this,Index]{const int32 Level=HUD.SkillLevels.IsValidIndex(Index)?HUD.SkillLevels[Index]:0;return FText::FromString(FString::Printf(TEXT("%s %d"),SkillNames[Index],Level));}));
    }
    At(1240,880,120,105,SNew(SImage).Image(Art(TEXT("icon-dash.png"))));
    At(1195,985,220,60,Button(TEXT("遁术  空格"),[this]{return Dash();},false,true,TEXT("实时模式向玩法发送遁术请求；预览不模拟技能")));
    At(1200,850,215,35,SNew(STextBlock).Font(Font(18)).ColorAndOpacity(Paper).Justification(ETextJustify::Center).Text_Lambda([this]{return FText::FromString(HUD.DashCooldownRemaining>0?FString::Printf(TEXT("冷却 %.1f 秒"),HUD.DashCooldownRemaining):TEXT("遁术就绪"));}));
    At(650,1050,760,12,SNew(SProgressBar).Style(&ProgressStyle).FillColorAndOpacity(FLinearColor(0.1f,0.5f,0.9f))
        .Percent_Lambda([this]{return TOptional<float>(dywx::SafeFraction(HUD.Experience,HUD.ExperienceToNext));}));
    At(525,1040,125,35,Label(TEXT("悟法进度"),18,Paper));
    At(1420,1040,200,35,SNew(STextBlock).Font(Font(18)).ColorAndOpacity(Paper).Text_Lambda([this]{return FText::FromString(FString::Printf(TEXT("%.0f/%.0f"),HUD.Experience,HUD.ExperienceToNext));}));
    At(50,925,80,80,SNew(SImage).Image(Art(TEXT("icon-stone.png"))).Visibility_Lambda([this]{return HUD.bHasStone?EVisibility::HitTestInvisible:EVisibility::Collapsed;}));
    At(145,925,80,80,SNew(SImage).Image(Art(TEXT("icon-mirror.png"))).Visibility_Lambda([this]{return HUD.bHasMirror?EVisibility::HitTestInvisible:EVisibility::Collapsed;}));
    At(45,1010,95,35,SNew(STextBlock).Font(Font(18)).ColorAndOpacity(Paper).Text(FText::FromString(TEXT("磨砺石"))).Visibility_Lambda([this]{return HUD.bHasStone?EVisibility::HitTestInvisible:EVisibility::Collapsed;}));
    At(145,1010,95,35,SNew(STextBlock).Font(Font(18)).ColorAndOpacity(Paper).Text(FText::FromString(TEXT("须弥镜"))).Visibility_Lambda([this]{return HUD.bHasMirror?EVisibility::HitTestInvisible:EVisibility::Collapsed;}));
}
TSharedRef<SWidget> SDywxRoot::ModalContent()
{
    auto Body = SNew(SVerticalBox);
    auto Add = [&Body](TSharedRef<SWidget> Widget, float Height) {
        Body->AddSlot().AutoHeight().Padding(0,10)[SNew(SBox).HeightOverride(Height)[Widget]];
    };
    switch (Nav.modal)
    {
    case dywx::Modal::Quit:
        Add(Label(TEXT("离开道衍万象？"),36,Paper,true),75);
        Add(Label(TEXT("当前界面预览进度不会保存。"),23,Muted),55);
        Add(Button(TEXT("取消"),[this]{return CloseModal();},true),72);
        Add(Button(TEXT("确认退出"),[this]{if (Owner.IsValid()) Owner->ConfirmQuit();return FReply::Handled();}),65);
        break;
    case dywx::Modal::Pause:
        Add(Label(TEXT("暂停菜单"),40,Paper,true),80);
        Add(Label(Nav.liveBattle?TEXT("战斗暂停由玩法层处理。") : TEXT("界面预览 · 无战斗模拟"),22,Muted),55);
        Add(Button(TEXT("继续"),[this]{return CloseModal();},true),72);
        Add(Button(TEXT("返回主菜单"),[this]{if(Owner.IsValid()) Owner->ShowMainMenu();else ReturnToMenu();return FReply::Handled();}),70);
        break;
    case dywx::Modal::Codex:
        Add(Label(TEXT("道典"),42,Paper,true),85);
        Add(Paragraph(TEXT("神话为表，科技为里；探索为引，认知为赏。"),596,26,Gold,true),100);
        Add(Paragraph(TEXT("自动施展术法，移动与遁术避险。\n当前演示只包含界面，未接入战斗、存档与内容解锁。"),596,23,Paper),150);
        Add(Button(TEXT("返回"),[this]{return CloseModal();},true),70);
        break;
    case dywx::Modal::Settings:
        Add(Label(TEXT("界面设置"),40,Paper,true),80);
        Add(Label(TEXT("字号 · 本次运行生效"),24,Paper),50);
        Add(Button(TEXT("标准字号"),[this]{UIScale=1.0f;Rebuild();return FReply::Handled();},UIScale<1.05f),65);
        Add(Button(TEXT("大字号"),[this]{UIScale=1.1f;Rebuild();return FReply::Handled();},UIScale>=1.05f),65);
        Add(Button(TEXT("返回"),[this]{return CloseModal();}),65);
        break;
    default: break;
    }
    return SNew(SBox).WidthOverride(680)[Panel(Body,Ink,42)];
}
FReply SDywxRoot::Primary()
{
    if (Nav.modal != dywx::Modal::None) return CloseModal(); // Never confirm quitting with a stray Enter.
    switch (Nav.screen)
    {
    case dywx::Screen::Main: Nav.Start(); Rebuild(); break;
    case dywx::Screen::Character: Nav.ChooseMap(); Rebuild(); break;
    case dywx::Screen::Map:
        if (Nav.Enter())
        {
            HUD = FDywxHUDState(); // Explicit, static UI fixture. No implicit gameplay clock.
            Rebuild(); SyncInput();
            if (Owner.IsValid()) Owner->RequestRun(Nav.character,Nav.map,static_cast<int32>(Nav.mode));
        }
        break;
    case dywx::Screen::Battle: break;
    }
    return FReply::Handled();
}
FReply SDywxRoot::Back()
{
    if (Nav.modal != dywx::Modal::None) return CloseModal();
    const bool WasBattle = Nav.screen == dywx::Screen::Battle;
    Nav.Back(); Rebuild(); SyncInput();
    if (WasBattle && Owner.IsValid()) Owner->SetUIPaused(true);
    return FReply::Handled();
}
FReply SDywxRoot::OpenModal(dywx::Modal Modal)
{
    if (Nav.modal == dywx::Modal::None) { Nav.modal=Modal;Rebuild();SyncInput(); }
    return FReply::Handled();
}
FReply SDywxRoot::CloseModal()
{
    const bool WasPause=Nav.modal==dywx::Modal::Pause;
    Nav.modal=dywx::Modal::None; Rebuild(); SyncInput();
    if (WasPause && Owner.IsValid()) Owner->SetUIPaused(false);
    return FReply::Handled();
}
FReply SDywxRoot::Pause()
{
    if (Nav.Pause()) { Rebuild();SyncInput();if(Owner.IsValid()) Owner->SetUIPaused(true); }
    return FReply::Handled();
}
FReply SDywxRoot::Dash()
{
    if (Nav.screen==dywx::Screen::Battle && Nav.modal==dywx::Modal::None && Nav.liveBattle && HUD.DashCooldownRemaining<=0.0f && Owner.IsValid()) Owner->RequestDash();
    return FReply::Handled();
}
FReply SDywxRoot::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.IsRepeat()) return SCompoundWidget::OnKeyDown(Geometry,Event);
    if (Event.GetKey()==EKeys::Escape) return Back();
    if (Event.GetKey()==EKeys::Enter) return Primary();
    if (Event.GetKey()==EKeys::SpaceBar && Nav.screen==dywx::Screen::Battle) return Dash();
    return SCompoundWidget::OnKeyDown(Geometry,Event);
}
void SDywxRoot::SyncInput()
{
    if (Owner.IsValid()) Owner->ApplyUIInput(Nav.screen==dywx::Screen::Battle&&Nav.liveBattle,Nav.modal!=dywx::Modal::None);
}
void SDywxRoot::SetHUDState(const FDywxHUDState& State)
{
    HUD=State;
    auto FiniteNonNegative=[](float Value,float Maximum){return FMath::IsFinite(Value)?FMath::Clamp(Value,0.0f,Maximum):0.0f;};
    HUD.MaxHealth=FiniteNonNegative(HUD.MaxHealth,1.e9f);
    HUD.Health=FiniteNonNegative(HUD.Health,HUD.MaxHealth);
    HUD.ExperienceToNext=FiniteNonNegative(HUD.ExperienceToNext,1.e9f);
    HUD.Experience=FiniteNonNegative(HUD.Experience,HUD.ExperienceToNext);
    HUD.ElapsedSeconds=FiniteNonNegative(HUD.ElapsedSeconds,8640000.0f);
    HUD.Wave=FMath::Clamp(HUD.Wave,1,9999);
    HUD.DashCooldownTotal=FiniteNonNegative(HUD.DashCooldownTotal,86400.0f);
    HUD.DashCooldownRemaining=FiniteNonNegative(HUD.DashCooldownRemaining,HUD.DashCooldownTotal);
    HUD.SkillLevels.SetNum(3);
    for(int32& Level:HUD.SkillLevels) Level=FMath::Clamp(Level,0,999);
}
void SDywxRoot::BeginLiveHUD(const FDywxHUDState& State)
{
    Nav.screen=dywx::Screen::Battle;Nav.modal=dywx::Modal::None;Nav.liveBattle=true;
    SetHUDState(State);Rebuild();SyncInput();
}
void SDywxRoot::ReturnToMenu()
{
    Nav.ReturnToMenu();Rebuild();SyncInput();
}
void SDywxRoot::SetEndlessUnlocked(bool bUnlocked)
{
    Nav.endlessUnlocked=bUnlocked;
    if (!bUnlocked && Nav.mode==dywx::Mode::Endless) Nav.mode=dywx::Mode::Quick;
    if (Nav.screen==dywx::Screen::Map) Rebuild();
}
