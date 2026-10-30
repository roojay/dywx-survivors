#include "SDywxRoot.h"
#include "DywxUIPlayerController.h"
#include "MVP/Simulation.h"
#include "MVP/ContentCatalog.h"
#include "MVP/DywxMVPProfile.h"
#include "Widgets/Layout/SScrollBox.h"
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
#include "Widgets/SToolTip.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
const FLinearColor Gold(0.88f, 0.64f, 0.30f, 1.0f);
const FLinearColor Paper(0.94f, 0.90f, 0.80f, 1.0f);
const FLinearColor Muted(0.60f, 0.66f, 0.69f, 1.0f);
const FLinearColor Ink(0.015f, 0.026f, 0.045f, 0.94f);
const TCHAR* Names[] = {TEXT("断岳行者"), TEXT("巡星羽客")};
FString CN(std::string_view S) { return FString(UTF8_TO_TCHAR(std::string(S).c_str())); }
using namespace dywx::mvp;
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
            .ToolTip(SNew(SToolTip)[Paragraph(Tooltip,650,18,Paper)])
            .OnClicked_Lambda([this, Epoch=UIEpoch, Fn = MoveTemp(Action)]() mutable { return Epoch == UIEpoch ? Fn() : FReply::Handled(); })
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
    ++UIEpoch;
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
        At(1450,1020,420,35, Label(TEXT("道衍万象 · 初入洞天"),18,Muted));
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
    At(70,170,900,50,Label(TEXT("法体近战与飞剑远攻 · 术法自动施展"),24,Gold));
    for (int32 Index=0; Index<2; ++Index)
    {
        At(80+Index*550,260,480,550,Panel(SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Portrait(Index))],Ink,8));
        At(80+Index*550,825,480,90,Button(Names[Index],[this,Index]{if(Nav.SelectCharacter(Index)) Rebuild();return FReply::Handled();},Nav.character==Index));
    }
    At(1270,280,590,650,Panel(SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[Label(Names[Nav.character],48,Paper,true)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,35)[Paragraph(Nav.character==0?TEXT("法体修士\n起手：焚心掌\n专属心法：炼体真诀\n近身斗法，灼烧群敌。") : TEXT("飞剑修士\n起手：赤焰飞剑\n专属心法：御剑诀\n飞剑穿阵，远攻破敌。"),510,28,Gold)]
        +SVerticalBox::Slot().AutoHeight()[Button(TEXT("选择洞天"),[this]{return Primary();},true,true,TEXT(""),true)],Ink,30));
    At(670,975,900,42,Label(TEXT("修士称号与外观暂定 · 起手术法与心法已生效"),20,Muted));
}
void SDywxRoot::MapScreen()
{
    At(55,55,800,130,Label(TEXT("选择洞天"),65,Paper,true));
    At(1360,0,560,1080,Panel(SNew(SBox),Ink,0));
    At(1410,190,460,110,Label(TEXT("栖雷古坛"),56,Paper,true));
    At(1420,345,430,120,Paragraph(TEXT("火雷交织，妖潮将至。\n四波妖潮后迎战洞主。"),430,26,Paper,true));
    const TCHAR* Modes[] = {TEXT("快速"),TEXT("传统"),TEXT("无尽")};
    for (int32 Index=0; Index<3; ++Index)
    {
        const auto Mode = static_cast<dywx::Mode>(Index);
        At(1395+Index*160,530,145,95,Button(Modes[Index],[this,Mode]{Nav.SelectMode(Mode);Rebuild();return FReply::Handled();},Nav.mode==Mode,Index==0));
    }
    At(1410,655,460,70,Paragraph(TEXT("快速：四波妖潮＋洞主\n传统与无尽 · 后续开放"),460,21,Muted));
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
TSharedRef<SWidget> SDywxRoot::LiveText(TFunction<FString()> Value, int32 Size, FLinearColor Color)
{
    return SNew(STextBlock).Font(Font(Size)).ColorAndOpacity(Color).AutoWrapText(false)
        .Text_Lambda([Fn=MoveTemp(Value)] { return FText::FromString(Fn()); });
}
void SDywxRoot::BattleScreen()
{
    At(30,25,500,100,Panel(SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[LiveText([this]{return FString::Printf(TEXT("气血 %.0f / %.0f   护盾 %.0f"),HUD.Health,HUD.MaxHealth,Run.player.shield);},24,Paper)]
        +SVerticalBox::Slot().FillHeight(1).Padding(0,12,0,0)[SNew(SProgressBar).Style(&ProgressStyle).FillColorAndOpacity(FLinearColor(.85f,.12f,.035f)).Percent_Lambda([this]{return TOptional<float>(dywx::SafeFraction(HUD.Health,HUD.MaxHealth));})],Ink,12));
    At(580,25,750,90,Panel(LiveText([this]{return FString::Printf(TEXT("第 %d / %d 波    %02d:%02d    斩妖 %llu"),Run.wave,Run.waveCount,int(HUD.ElapsedSeconds)/60,int(HUD.ElapsedSeconds)%60,static_cast<unsigned long long>(Run.statistics.kills));},26,Paper),Ink,15));
    At(1640,35,230,65,Button(TEXT("暂停  Esc"),[this]{return Pause();}));
    At(35,140,700,45,LiveText([this]{const auto& R=Run.progression.build.resonance;return FString::Printf(TEXT("道象  火 %d  雷 %d  法体 %d  飞剑 %d  阵法 %d"),R.fire,R.thunder,R.body,R.sword,R.array);},22,Gold));
    At(35,185,800,40,Label(TEXT("同道象达到 2 / 4 点共鸣 · 破境继承道象"),18,Muted));
    At(35,230,900,40,LiveText([this]{return FString::Printf(TEXT("累计伤害 %.0f    承伤 %.0f    妖潮余时 %.0f 秒"),Run.statistics.damageDealt,Run.statistics.damageTaken,FMath::Max(0.f,Run.waveDuration-Run.waveElapsed));},20,Paper));
    if(Run.phase==RunPhase::Boss)
    {
        At(580,130,750,45,LiveText([this]{return FString::Printf(TEXT("洞主 · 第 %d 相   %.0f / %.0f"),Run.bossPhase,BossHP,BossMaxHP);},26,Gold));
        At(580,180,750,18,SNew(SProgressBar).Style(&ProgressStyle).FillColorAndOpacity(FLinearColor(.7f,.12f,.6f)).Percent_Lambda([this]{return TOptional<float>(dywx::SafeFraction(BossHP,BossMaxHP));}));
    }
    for(int32 I=0;I<6;++I)
        At(30+I*235,845,225,100,Panel(LiveText([this,I]{const auto& S=Run.progression.build.spells;if(I>=int(S.size()))return FString::Printf(TEXT("术法槽 %d · 空"),I+1);return CN(GetSpellConfig(S[I].id,S[I].rank).name)+FString::Printf(TEXT("\n%d重 · %.1f秒"),S[I].rank,Run.spellCooldowns[int(S[I].id)]);},18,Paper),Ink,10));
    for(int32 I=0;I<4;++I)
        At(30+I*300,950,290,52,Panel(LiveText([this,I]{const auto& P=Run.progression.build.passives;return I<int(P.size())?CN(PassiveCatalog()[int(P[I])].name):FString::Printf(TEXT("心法槽 %d · 空"),I+1);},20,Gold),Ink,8));
    At(1450,855,400,55,LiveText([this]{return HUD.DashCooldownRemaining>0?FString::Printf(TEXT("遁术冷却 %.1f 秒"),HUD.DashCooldownRemaining):FString(TEXT("遁术就绪 · 空格"));},24,Gold));
    At(1450,920,400,65,Button(TEXT("遁术  Space"),[this]{return Dash();}));
    At(30,1020,1350,14,SNew(SProgressBar).Style(&ProgressStyle).FillColorAndOpacity(FLinearColor(.1f,.5f,.9f)).Percent_Lambda([this]{return TOptional<float>(dywx::SafeFraction(HUD.Experience,HUD.ExperienceToNext));}));
    At(30,1045,1350,30,LiveText([this]{return FString::Printf(TEXT("修为 %.0f / %.0f   境界 %d   WASD / 方向键移动 · 空格遁术 · 1 / 2 / 3 顿悟"),HUD.Experience,HUD.ExperienceToNext,Run.progression.level);},18,Paper));
    // Keep the action right of the boss bar, above the upgrade cards.
    if (Run.phase == RunPhase::Wave || Run.phase == RunPhase::Boss || Run.phase == RunPhase::Rest)
    {
        const SpellId Fusion = Run.progression.build.hero == HeroId::Body
            ? SpellId::FusionPalm : SpellId::FusionSwords;
        const auto* Sim = Owner.IsValid() ? Owner->Simulation() : nullptr;
        const bool CanFuseNow = Sim && !Run.paused && Sim->GetProgression().CanFuse(Fusion)
            && (Run.phase == RunPhase::Rest || !Run.progression.offers.empty());
        const FString Recipe = Fusion == SpellId::FusionPalm
            ? TEXT("焚心掌五重＋惊雷拳五重") : TEXT("赤焰飞剑五重＋紫电飞剑五重");
        At(1360,130,510,80,Button(TEXT("破境 · ")+CN(GetSpellConfig(Fusion).name),
            [this,Fusion,Generation=Run.generation] {
                return RunCommand([this,Fusion,Generation] {
                    auto* S = Owner->Simulation();
                    if (!S) return;
                    const auto& Now = S->GetSnapshot();
                    if (Now.generation != Generation || Now.paused ||
                        (Now.phase != RunPhase::Wave && Now.phase != RunPhase::Boss && Now.phase != RunPhase::Rest)) return;
                    auto& P = S->GetProgression();
                    if (P.CanFuse(Fusion) && (Now.phase == RunPhase::Rest || !Now.progression.offers.empty()) && P.Fuse(Fusion))
                        S->RefreshProgression();
                });
            },true,CanFuseNow,Recipe+TEXT("。两门均圆满后，仅在调息或有待选顿悟时可破境；暂停时不可用。合成替换两门术法，继承双方道象。")));
    }
    if(Run.phase==RunPhase::Rest) RestPanel();
    if(Run.phase==RunPhase::Won || Run.phase==RunPhase::Dead) ResultPanel();
    if(!Run.progression.offers.empty() && (Run.phase==RunPhase::Wave || Run.phase==RunPhase::Boss || Run.phase==RunPhase::Rest)) UpgradePanel();
}
FReply SDywxRoot::RunCommand(TFunction<void()> Command)
{
    if(Nav.liveBattle && Nav.modal==dywx::Modal::None && Owner.IsValid()) { Command(); Owner->RefreshRunUI(); RefreshRunUI(); }
    return FReply::Handled();
}
void SDywxRoot::UpgradePanel()
{
    At(1080,225,810,55,Panel(Label(FString::Printf(TEXT("顿悟待选 %d · 战斗不停"),Run.progression.pendingUpgrades),24,Gold),Ink,6));
    for(int32 I=0;I<int(Run.progression.offers.size()) && I<3;++I)
    {
        const auto O=Run.progression.offers[I];
        FString Name,Desc;
        if(O.kind==UpgradeKind::Passive) { const auto& P=PassiveCatalog()[int(O.passive)];Name=CN(P.name);Desc=CN(P.name)+TEXT("：\n")+CN(P.description); }
        else { int Rank=1;for(const auto& S:Run.progression.build.spells)if(S.id==O.spell)Rank=S.rank+1;Name=CN(GetSpellConfig(O.spell,Rank).name);Desc=CN(DescribeSpell(O.spell,Rank)); }
        auto Card=SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight()[Button(FString::Printf(TEXT("%d  "),I+1)+Name,[this,I,O,Generation=Run.generation]{return RunCommand([this,I,O,Generation]{auto*S=Owner->Simulation();if(!S)return;const auto&Now=S->GetSnapshot();if(Now.generation!=Generation||I>=int(Now.progression.offers.size()))return;const auto&Current=Now.progression.offers[I];if(Current.kind==O.kind&&Current.spell==O.spell&&Current.passive==O.passive)Owner->ChooseUpgrade(I);});})]
            +SVerticalBox::Slot().FillHeight(1).Padding(8,8)[SNew(SScrollBox)+SScrollBox::Slot()[Paragraph(Desc,230,18,Paper)]];
        At(1080+I*270,290,260,220,Panel(Card,Ink,6));
    }
    At(1460,520,430,60,Button(FString::Printf(TEXT("重悟 · 剩余 %d 次"),Run.progression.rerollsRemaining),[this]{return RunCommand([this]{if(auto* S=Owner->Simulation())S->Reroll();});},false,Run.progression.rerollsRemaining>0));
}
void SDywxRoot::ResultPanel()
{
    for(int32 I=0;I<int(Run.progression.bossRewards.size()) && I<3;++I)
    {
        const auto R=Run.progression.bossRewards[I];FString Tip=CN(RarityName(R.rarity));for(const auto&A:R.affixes)Tip+=TEXT("\n")+CN(DescribeAffix(A));
        At(430+I*360,220,350,65,Button(CN(RelicCatalog()[R.catalogId].name),[this,I]{return RunCommand([this,I]{if(auto*S=Owner->Simulation()){if(S->GetSnapshot().phase==RunPhase::Won){S->GetProgression().ChooseBossReward(I);S->RefreshProgression();}}});},false,true,Tip));
    }
    At(510,300,900,500,Panel(SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[Label(Run.phase==RunPhase::Won?TEXT("洞天已平 · 修行有成"):TEXT("道途未尽 · 再赴洞天"),40,Gold,true)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,30)[Paragraph(FString::Printf(TEXT("历时 %02d:%02d   境界 %d\n斩妖 %llu   伤害 %.0f   承伤 %.0f\n本局法宝仅在局内生效。"),int(Run.statistics.elapsed)/60,int(Run.statistics.elapsed)%60,Run.progression.level,static_cast<unsigned long long>(Run.statistics.kills),Run.statistics.damageDealt,Run.statistics.damageTaken),800,26,Paper)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,10)[Button(TEXT("再次入洞天"),[this]{return RunCommand([this]{Owner->RetryRun();});},true)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,10)[Button(TEXT("返回主菜单"),[this]{if(Owner.IsValid())Owner->ShowMainMenu();return FReply::Handled();})],Ink,35));
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
        Add(Label(TEXT("当前修行进度不会保存。"),23,Muted),55);
        Add(Button(TEXT("取消"),[this]{return CloseModal();},true),72);
        Add(Button(TEXT("确认退出"),[this]{if (Owner.IsValid()) Owner->ConfirmQuit();return FReply::Handled();}),65);
        break;
    case dywx::Modal::Pause:
        Add(Label(TEXT("暂停菜单"),40,Paper,true),80);
        Add(Label(TEXT("妖潮已停，继续后恢复斗法。"),22,Muted),55);
        Add(Button(TEXT("继续"),[this]{return CloseModal();},true),72);
        Add(Button(TEXT("返回主菜单"),[this]{if(Owner.IsValid()) Owner->ShowMainMenu();else ReturnToMenu();return FReply::Handled();}),70);
        break;
    case dywx::Modal::Codex:
    {
        Add(Label(TEXT("道典"),42,Paper,true),85);
        Add(Paragraph(TEXT("神话为表，科技为里；探索为引，认知为赏。"),596,26,Gold,true),100);
        Add(Paragraph(TEXT("自动施展术法，移动与遁术避险。\nWASD / 方向键移动，空格遁术。\n1 / 2 / 3 选择顿悟；顿悟不暂停战斗。"),596,23,Paper),150);
        auto Entries=SNew(SScrollBox);
        // Read the saved discoveries even before the first world/run is created.
        // Observe only merges into this local view: opening the Codex never saves.
        FDywxMVPProfile Codex;
        Codex.Load();
        if (Owner.IsValid())
            if (const auto* Simulation = Owner->Simulation())
                Codex.Observe(Simulation->GetProgression().GetState());
        const auto& Known = Codex.GetKnown();
        auto Entry = [&Entries](const FString& Text, bool Seen) {
            Entries->AddSlot().Padding(0,8)[Paragraph(
                (Seen ? TEXT("已见 · ") : TEXT("未见 · ")) + Text,
                570,21,Seen ? Paper : Muted)];
        };
        Entries->AddSlot()[Paragraph(TEXT("灰色为未亲历，点亮只记录发现，不增加局外属性。妖魔进入战场即记入；心法取得、共鸣达到两门后点亮。"),570,21,Gold)];
        Entry(TEXT("破境配方：焚心掌五重＋惊雷拳五重 → 雷火焚天掌。"),Known.seenFusions[0]);
        Entry(TEXT("破境配方：赤焰飞剑五重＋紫电飞剑五重 → 雷火双剑。"),Known.seenFusions[1]);
        for (int32 I = 0; I < 12; ++I)
            Entry(CN(DescribeSpell(SpellId(I),1))+TEXT("\n圆满：\n")+CN(DescribeSpell(SpellId(I),5)),Known.seenSpells[I]);
        for (const auto& P : PassiveCatalog())
            Entry(CN(P.name)+TEXT("：")+CN(P.description),Known.seenPassives[static_cast<size_t>(P.id)]);
        for (const auto& R : RelicCatalog())
            Entry(CN(R.name)+TEXT(" · ")+CN(RarityName(R.rarity)),Known.seenRelics[R.id]);
        const TCHAR* Enemies[] = {
            TEXT("魑魅 · 群行：围近后接触攻击。"),
            TEXT("磷火 · 自爆：近身预警后爆炸；雷系命中会提前引爆，只伤妖魔。"),
            TEXT("赤狰 · 冲锋：直线预警后冲撞，侧向走位避开。"),
            TEXT("毕方 · 远程：发射灵火弹，落点留下火焰。"),
            TEXT("阵灵 · 护持：为附近妖魔施加雷盾；火系对雷盾伤害翻倍。"),
            TEXT("蜃 · 召唤：持续幻化魑魅。"),
            TEXT("流波夔君 · 洞主：两阶段斗法，留意冲锋、落雷与火墙预警。"),
            TEXT("灵兽 · 稀有访客：迅速远离修士，击败可得法宝。")};
        for (int32 I = 0; I < 8; ++I)
            Entry(Enemies[I],Known.seenEnemies[I]);
        const TCHAR* Mutations[] = {
            TEXT("狂暴 · 移速提高；冲锋与远程攻击间隔缩短。"),
            TEXT("灵甲 · 额外灵甲，未破时减伤。"),
            TEXT("分裂 · 死亡后分出两只较弱妖魔。"),
            TEXT("环绕雷 · 三颗雷球绕行，接触造成伤害。")};
        for (int32 I = 0; I < 4; ++I)
            Entry(Mutations[I],Known.seenMutations[I]);
        Entries->AddSlot()[Paragraph(TEXT("道象计数包含术法与心法倾向；破境继承双方计数。元素轴始终启用，形态轴依本局配置启用。"),570,21,Gold)];
        const TCHAR* Resonances[] = {
            TEXT("火 · 两门：灼烧伤害＋25%；四门：最多三层灼烧，灼烧中死亡引发爆炸。"),
            TEXT("雷 · 两门：命中有8%几率落雷；四门：提高到15%，附带麻痹。"),
            TEXT("法体 · 两门：护体＋10%、近身术法伤害＋15%；四门：近身命中有10%几率获得一秒无敌，冷却五秒。"),
            TEXT("飞剑 · 两门：数量或穿透＋1；四门：每八秒剑意爆发，三秒内伤害＋30%。"),
            TEXT("阵法 · 两门：阵域持续时间＋25%；四门：阵域内妖魔承伤＋20%。")};
        for (int32 I = 0; I < 5; ++I)
            Entry(Resonances[I],Known.seenResonances[I]);
        Entries->AddSlot()[Paragraph(TEXT("后续内容预告 · 坊市、异象等第二与第三层内容尚未开放，本原型不可参与。"),570,21,Muted)];
        Add(Entries,270);
        Add(Button(TEXT("返回"),[this]{return CloseModal();},true),70);
        break;
    }
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
            HUD = FDywxHUDState();
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
    if (Nav.liveBattle && Nav.modal==dywx::Modal::None && Owner.IsValid())
    {
        if(Event.GetKey()==EKeys::One || Event.GetKey()==EKeys::Two || Event.GetKey()==EKeys::Three)
        { Owner->ChooseUpgrade(Event.GetKey()==EKeys::One?0:(Event.GetKey()==EKeys::Two?1:2));return FReply::Handled(); }
    }
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
    HUD.SkillLevels.SetNum(6);
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

void SDywxRoot::RefreshRunUI() { if(Nav.liveBattle) Rebuild(); }
void SDywxRoot::SetSnapshot(const dywx::mvp::Snapshot& State)
{
    if(!Nav.liveBattle) return;
    bool Changed=Run.generation!=State.generation || Run.paused!=State.paused || Run.phase!=State.phase || Run.progression.pendingUpgrades!=State.progression.pendingUpgrades || Run.progression.rerollsRemaining!=State.progression.rerollsRemaining;
    if(Run.progression.bossRewards.size()!=State.progression.bossRewards.size()) Changed=true;
    if(Run.progression.offers.size()!=State.progression.offers.size()) Changed=true;
    else for(size_t I=0;I<State.progression.offers.size();++I) { const auto&A=Run.progression.offers[I];const auto&B=State.progression.offers[I];if(A.kind!=B.kind || A.spell!=B.spell || A.passive!=B.passive) Changed=true; }
    Run.generation=State.generation;Run.phase=State.phase;Run.paused=State.paused;Run.player=State.player;
    Run.wave=State.wave;Run.waveCount=State.waveCount;Run.waveElapsed=State.waveElapsed;Run.waveDuration=State.waveDuration;Run.bossPhase=State.bossPhase;
    Run.statistics=State.statistics;Run.progression=State.progression;Run.spellCooldowns=State.spellCooldowns;
    BossHP=0;BossMaxHP=1;for(const auto&E:State.enemies)if(E.role==EnemyRole::Boss){BossHP=E.hp;BossMaxHP=E.maxHp;break;}
    HUD.Health=State.player.hp;HUD.MaxHealth=State.progression.build.stats.maxHp;HUD.Experience=State.progression.xp;HUD.ExperienceToNext=State.progression.xpRequired;
    HUD.ElapsedSeconds=State.statistics.elapsed;HUD.Wave=State.wave;HUD.DashCooldownRemaining=State.player.dashCooldown;HUD.DashCooldownTotal=State.progression.build.stats.dashCooldown;
    if(Changed) { SelectedBag=0;bRefinePreview=false;Rebuild(); }
}

void SDywxRoot::RestPanel()
{
    auto* Sim=Owner.IsValid()?Owner->Simulation():nullptr;
    if(!Sim) return;
    auto& P=Sim->GetProgression();
    At(30,285,1010,550,Panel(SNew(SBox),Ink,0));
    At(45,290,980,55,Label(FString::Printf(TEXT("调息 · 战斗暂停   炼化待入账修为 %d"),P.PendingRefineXp()),26,Gold,true));
    RelicFilter Filter;Filter.rarity=RarityFilter;Filter.aspect=AspectFilter;Filter.affixKind=AffixFilter;
    const auto Visible=P.FilterBag(Filter);
    const TCHAR* Aspects[]={TEXT("全部道象"),TEXT("火"),TEXT("雷"),TEXT("法体"),TEXT("飞剑"),TEXT("阵法")};
    At(45,350,315,60,Button(RarityFilter<0?TEXT("品阶：全部"):TEXT("品阶：")+CN(RarityName(Rarity(RarityFilter))),[this]{RarityFilter=(RarityFilter+2)%5-1;bRefinePreview=false;Rebuild();return FReply::Handled();}));
    At(370,350,315,60,Button(Aspects[AspectFilter+1],[this]{AspectFilter=(AspectFilter+2)%6-1;bRefinePreview=false;Rebuild();return FReply::Handled();}));
    At(695,350,325,60,Button(AffixFilter<0?TEXT("灵纹：全部"):TEXT("灵纹：")+CN(AffixName({AffixKind(AffixFilter),0,0})),[this]{AffixFilter=(AffixFilter+2)%21-1;bRefinePreview=false;Rebuild();return FReply::Handled();}));
    auto Equipped=SNew(SScrollBox);
    for(int32 I=0;I<6;++I)
    {
        const auto& E=Run.progression.equipped;
        FString Name=I<int(E.size())?CN(RelicCatalog()[E[I].catalogId].name)+(E[I].locked?TEXT(" [锁]"):TEXT("")):TEXT("空槽");
        Equipped->AddSlot().Padding(0,3)[SNew(SBox).HeightOverride(60)[Button(FString::Printf(TEXT("%d  "),I+1)+Name,[this,I]{SelectedSlot=I;SelectedBag=0;bRefinePreview=false;Rebuild();return FReply::Handled();},false,true,TEXT("选择装备槽，再从储物袋选择法宝；下方换装或卸下。"))]];
    }
    auto Bag=SNew(SScrollBox);
    for(auto Id:Visible)
    {
        const Relic* Item=nullptr;for(const auto& R:Run.progression.bag)if(R.id==Id){Item=&R;break;}
        if(!Item)continue;
        FString Tip=CN(RarityName(Item->rarity));for(const auto&A:Item->affixes)Tip+=TEXT("\n")+CN(DescribeAffix(A));
        const auto Compare=P.PreviewEquip(Id,SelectedSlot);const auto Current=Run.progression.build.stats;
        Tip+=FString::Printf(TEXT("\n对比第 %d 槽：气血 %+.0f / 威力 %+.0f%% / 护体 %+.0f%% / 周天 %+.0f%%"),SelectedSlot+1,Compare.maxHp-Current.maxHp,(Compare.power-Current.power)*100,(Compare.armor-Current.armor)*100,(Compare.cooldownReduction-Current.cooldownReduction)*100);
        Bag->AddSlot().Padding(0,3)[SNew(SBox).HeightOverride(60)[Button(CN(RelicCatalog()[Item->catalogId].name)+(Item->locked?TEXT(" [锁]"):TEXT("")),[this,Id]{SelectedBag=Id;bRefinePreview=false;Rebuild();return FReply::Handled();},false,true,Tip)]];
    }
    if(Visible.empty())Bag->AddSlot()[Label(TEXT("储物袋 · 暂无法宝"),22,Muted)];
    At(45,420,475,275,Equipped);At(540,420,480,275,Bag);
    At(45,705,225,60,Button(TEXT("换装"),[this]{return RunCommand([this]{auto*S=Owner->Simulation();if(S&&S->GetSnapshot().phase==RunPhase::Rest){RunNotice=S->GetProgression().Equip(SelectedBag,SelectedSlot)?TEXT("换装完成"):TEXT("无法换装：请选择首个空槽，或检查互斥灵纹。");S->RefreshProgression();}});},false,SelectedBag!=0));
    At(280,705,225,60,Button(TEXT("卸下"),[this]{return RunCommand([this]{auto*S=Owner->Simulation();if(S&&S->GetSnapshot().phase==RunPhase::Rest){S->GetProgression().Unequip(SelectedSlot);S->RefreshProgression();}});},false,SelectedSlot<int(Run.progression.equipped.size())));
    At(515,705,245,60,Button(TEXT("锁定 / 解锁"),[this]{return RunCommand([this]{auto*S=Owner->Simulation();if(!S||S->GetSnapshot().phase!=RunPhase::Rest)return;auto&P=S->GetProgression();uint64 Id=SelectedBag;if(!Id&&SelectedSlot<int(Run.progression.equipped.size()))Id=Run.progression.equipped[SelectedSlot].id;for(const auto&R:Run.progression.bag)if(R.id==Id){P.SetLocked(Id,!R.locked);S->RefreshProgression();return;}for(const auto&R:Run.progression.equipped)if(R.id==Id){P.SetLocked(Id,!R.locked);S->RefreshProgression();return;}}); }));
    At(770,705,250,60,Button(TEXT("一键排序"),[this]{return RunCommand([this]{if(auto*S=Owner->Simulation()){S->GetProgression().SortBag();S->RefreshProgression();}});}));
    At(45,775,470,60,Button(TEXT("炼化筛选结果 · 先预览"),[this]{bRefinePreview=true;Rebuild();return FReply::Handled();},false,!Visible.empty()));
    At(540,775,480,60,Button(TEXT("撤销上次炼化"),[this]{return RunCommand([this]{if(auto*S=Owner->Simulation()){S->GetProgression().UndoRefine();S->RefreshProgression();}});},false,P.PendingRefineXp()>0));
    if(bRefinePreview)
    {
        const auto Preview=P.PreviewRefine(Visible);
        FString List;for(auto Id:Preview.ids)for(const auto&R:Run.progression.bag)if(R.id==Id)List+=CN(RelicCatalog()[R.catalogId].name)+TEXT("  ");
        At(1080,590,810,70,Paragraph(FString::Printf(TEXT("炼化 %d 件，获得 %d 修为（迎战时入账）"),int(Preview.ids.size()),Preview.xp),800,22,Gold));
        At(1080,655,810,75,SNew(SScrollBox)+SScrollBox::Slot()[Paragraph(List,790,19,Paper)]);
        At(1080,740,395,60,Button(TEXT("确认炼化"),[this,Ids=Preview.ids]{return RunCommand([this,Ids]{if(auto*S=Owner->Simulation()){S->GetProgression().Refine(Ids);S->RefreshProgression();}bRefinePreview=false;SelectedBag=0;});},false,!Preview.ids.empty()));
        At(1490,740,395,60,Button(TEXT("取消预览"),[this]{bRefinePreview=false;Rebuild();return FReply::Handled();}));
    }
    else
    {
        FString Details=FString::Printf(TEXT("已选装备槽 %d · 选择储物袋法宝以对比\n"),SelectedSlot+1)+RunNotice+TEXT("\n");
        if(SelectedSlot<int(Run.progression.equipped.size()))
        {
            const auto&E=Run.progression.equipped[SelectedSlot];Details+=TEXT("当前：")+CN(RelicCatalog()[E.catalogId].name)+TEXT(" · ")+CN(RarityName(E.rarity))+TEXT("\n");
            for(const auto&A:E.affixes)Details+=CN(DescribeAffix(A))+TEXT("  ");Details+=TEXT("\n");
        }
        const Stats Current=Run.progression.build.stats;
        Details+=FString::Printf(TEXT("身法 %.1f  摄灵 %.1f  生机 %.1f  气运 %.1f\n范围 %.0f%%  会心 %.0f%% / %.0f%%  遁息 %.1f 秒\n"),Current.moveSpeed,Current.pickupRadius,Current.regen,Current.luck,Current.area*100,Current.critChance*100,Current.critMultiplier*100,Current.dashCooldown);
        for(const auto&R:Run.progression.bag)if(R.id==SelectedBag)
        {
            Details+=CN(RelicCatalog()[R.catalogId].name)+TEXT(" · ")+CN(RarityName(R.rarity))+TEXT("\n");
            for(const auto&A:R.affixes)Details+=CN(DescribeAffix(A))+TEXT("  ");
            const Stats A=Run.progression.build.stats,B=P.PreviewEquip(R.id,SelectedSlot);
            Details+=FString::Printf(TEXT("\n换装后 气血 %.0f (%+.0f)  威力 %.0f%% (%+.0f%%)\n护体 %.0f%% (%+.0f%%)  周天 %.0f%% (%+.0f%%)"),B.maxHp,B.maxHp-A.maxHp,B.power*100,(B.power-A.power)*100,B.armor*100,(B.armor-A.armor)*100,B.cooldownReduction*100,(B.cooldownReduction-A.cooldownReduction)*100);
        }
        auto Inspection=SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[Paragraph(Details,790,19,Paper)];
        if(SelectedBag)
        {
            const Stats Before=Run.progression.build.stats,After=P.PreviewEquip(SelectedBag,SelectedSlot);
            auto Difference=[&](const TCHAR* Name,float Old,float New,bool LowerBetter=false)
            {
                const float Delta=New-Old;const bool Improved=LowerBetter?Delta<0:Delta>0;
                const FLinearColor Tint=FMath::IsNearlyZero(Delta)?Muted:(Improved?FLinearColor(1,.35f,.25f):FLinearColor(.3f,.85f,.5f));
                Inspection->AddSlot().AutoHeight()[Label(FString::Printf(TEXT("%s  %.1f → %.1f  (%+.1f)"),Name,Old,New,Delta),20,Tint)];
            };
            Difference(TEXT("气血"),Before.maxHp,After.maxHp);Difference(TEXT("护体 %"),Before.armor*100,After.armor*100);
            Difference(TEXT("生机"),Before.regen,After.regen);Difference(TEXT("威力 %"),Before.power*100,After.power*100);
            Difference(TEXT("周天 %"),Before.cooldownReduction*100,After.cooldownReduction*100);Difference(TEXT("范围 %"),Before.area*100,After.area*100);
            Difference(TEXT("会心 %"),Before.critChance*100,After.critChance*100);Difference(TEXT("身法"),Before.moveSpeed,After.moveSpeed);
            Difference(TEXT("摄灵"),Before.pickupRadius,After.pickupRadius);Difference(TEXT("遁息"),Before.dashCooldown,After.dashCooldown,true);
            Difference(TEXT("气运"),Before.luck,After.luck);
        }
        At(1080,590,810,150,SNew(SScrollBox)+SScrollBox::Slot()[Inspection]);
        At(1080,750,810,70,Button(Run.progression.pendingUpgrades>0?TEXT("先完成待选顿悟"):TEXT("迎战 · 炼化修为入账"),[this]{return RunCommand([this]{if(auto*S=Owner->Simulation())S->BeginNextWave();});},true,Run.progression.pendingUpgrades==0));
    }
}
