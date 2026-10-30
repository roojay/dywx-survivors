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
const TCHAR* Names[] = {TEXT("断岳行者"), TEXT("巡星羽客"), TEXT("灵契山人")};
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
TSharedRef<SWidget> SDywxRoot::Comparison(const Stats& Before,const Stats& After,const FString& Description) const
{
    auto Rows=SNew(SVerticalBox);
    if(!Description.IsEmpty())Rows->AddSlot().AutoHeight()[Paragraph(Description,570,18,Paper)];
    Rows->AddSlot().AutoHeight().Padding(0,8)[Label(TEXT("有效属性       当前装备    候选换装    差值"),18,Gold)];
    auto Difference=[&](const TCHAR* Name,float Old,float New,bool LowerBetter=false) {
        const float Delta=New-Old;const bool Improved=LowerBetter?Delta<0:Delta>0;
        const FLinearColor Tint=FMath::IsNearlyZero(Delta)?Muted:(Improved?FLinearColor(1,.35f,.25f):FLinearColor(.3f,.85f,.5f));
        Rows->AddSlot().AutoHeight().Padding(0,2)[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1.25f)[Label(Name,18,Paper)]
            +SHorizontalBox::Slot().FillWidth(1)[Label(FString::Printf(TEXT("%.1f"),Old),18,Paper)]
            +SHorizontalBox::Slot().FillWidth(1)[Label(FString::Printf(TEXT("%.1f"),New),18,Tint)]
            +SHorizontalBox::Slot().FillWidth(1)[Label(FString::Printf(TEXT("%+.1f"),Delta),18,Tint)]];
    };
    Difference(TEXT("气血"),Before.maxHp,After.maxHp);Difference(TEXT("护体 %"),Before.armor*100,After.armor*100);
    Difference(TEXT("生机"),Before.regen,After.regen);Difference(TEXT("威力 %"),Before.power*100,After.power*100);
    Difference(TEXT("周天 %"),Before.cooldownReduction*100,After.cooldownReduction*100);Difference(TEXT("范围 %"),Before.area*100,After.area*100);
    Difference(TEXT("会心 %"),Before.critChance*100,After.critChance*100);Difference(TEXT("会心伤害 %"),Before.critMultiplier*100,After.critMultiplier*100);
    Difference(TEXT("身法"),Before.moveSpeed,After.moveSpeed);Difference(TEXT("摄灵"),Before.pickupRadius,After.pickupRadius);
    Difference(TEXT("遁息 秒"),Before.dashCooldown,After.dashCooldown,true);Difference(TEXT("气运"),Before.luck,After.luck);
    return SNew(SBox).WidthOverride(590).MaxDesiredHeight(570)[SNew(SScrollBox)+SScrollBox::Slot()[Rows]];
}
TSharedRef<SWidget> SDywxRoot::Panel(TSharedRef<SWidget> Content, FLinearColor Tint, float Padding) const
{
    return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(Tint).Padding(Padding)[Content];
}
TSharedRef<SWidget> SDywxRoot::Button(const FString& Text, TFunction<FReply()> Action, bool bPrimary, bool bEnabled, const FString& Tooltip, bool bFramed, TSharedPtr<SWidget> TooltipContent)
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
            .ToolTip(SNew(SToolTip)[TooltipContent.IsValid()?TooltipContent.ToSharedRef():Paragraph(Tooltip,650,18,Paper)])
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
    if (Owner.IsValid()) {
        const auto& Profile=Owner->CampaignProfile();
        Nav.endlessUnlocked=Profile.traditionalWin;
        for(int I=0;I<3;++I) { Nav.heroes[I]=IsHeroUnlocked(Profile,I); Nav.realms[I]=IsRealmUnlocked(Profile,I); Nav.savedRuns[I]=Owner->HasSavedRun(I); }
    }
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
    At(750,800,1100,85,Paragraph(RunNotice,1050,22,Gold));
    At(60,90,660,870,Panel(SNew(SBox),FLinearColor(0.01f,0.02f,0.035f,0.22f),0));
    At(110,155,610,155,Label(TEXT("道衍万象"),82,Paper,true));
    At(125,320,530,55,Label(TEXT("洞天有序 · 万象待解"),24,Gold,true));
    At(120,455,490,90,Button(TEXT("开始修行"),[this]{ return Primary(); },true,true,TEXT(""),true));
    At(145,580,440,70,Button(TEXT("道典"),[this]{ return OpenModal(dywx::Modal::Codex); }));
    At(145,685,440,70,Button(TEXT("传承"),[this]{ return OpenModal(dywx::Modal::Legacy); }));
    At(750,900,440,70,Button(TEXT("设置"),[this]{ return OpenModal(dywx::Modal::Settings); }));
    At(145,790,440,70,Button(TEXT("退出游戏"),[this]{ return OpenModal(dywx::Modal::Quit); }));
    At(80,1020,950,40,Label(TEXT("Enter 确认   ·   Tab 切换焦点   ·   Esc 返回"),20,Muted));
}
void SDywxRoot::CharacterScreen()
{
    At(55,45,800,100,Label(TEXT("选择修士"),50,Paper,true));
    const TCHAR* Descriptions[]={TEXT("法体近战 · 焚心掌与炼体真诀"),TEXT("飞剑远攻 · 赤焰飞剑与御剑诀"),TEXT("御兽随行 · 灵兽自主寻敌")};
    for(int I=0;I<3;++I) {
        // Existing atlas art is reused as a concept portrait, not a new character asset.
        At(80+I*600,190,540,470,Panel(SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SImage).Image(Portrait(I))],Ink,8));
        At(80+I*600,680,540,75,Button(FString(Names[I])+(Nav.heroes[I]?TEXT(""):TEXT(" · 未解锁")),[this,I]{Nav.SelectCharacter(I);Rebuild();return FReply::Handled();},Nav.character==I,Nav.heroes[I],TEXT("以道韵在传承中解锁修士。")));
        At(80+I*600,770,540,50,Label(Descriptions[I],22,Gold));
        At(80+I*600,830,540,60,Button(TEXT("续接此修士存档"),[this,I]{if(Owner.IsValid()&&!Owner->ResumeRun(I)){RunNotice=TEXT("读档失败：存档未消费，请检查存档有效性。");Rebuild();}return FReply::Handled();},false,Owner.IsValid()&&Owner->HasSavedRun(I),TEXT("成功恢复后消费存档；回到保存的待迎战波开头。")));
    }
    At(600,920,500,75,Button(Nav.savedRuns[Nav.character]?TEXT("存档待续 · 请先续接"):TEXT("选择洞天"),[this]{return Primary();},true,!Nav.savedRuns[Nav.character],TEXT("同一修士有进行中存档时不可新开；请先续接，或选择其他修士。")));
    At(1150,920,500,75,Button(TEXT("传承"),[this]{return OpenModal(dywx::Modal::Legacy);}));
    At(70,150,1500,40,Label(TEXT("立绘沿用概念图集 · 起手路线以术法说明为准"),20,Muted));
    At(350,1020,1400,40,Label(RunNotice,20,Gold));
}
void SDywxRoot::MapScreen()
{
    At(55,55,800,100,Label(TEXT("选择洞天与劫数"),50,Paper,true));
    for(int I=0;I<3;++I) {
        const auto& C=CaveCatalog()[I];
        At(60+I*620,190,580,80,Button(CN(C.name)+(Nav.realms[I]?TEXT(""):TEXT(" · 未解锁")),[this,I]{Nav.SelectMap(I);Rebuild();return FReply::Handled();},Nav.map==I,Nav.realms[I],TEXT("未解锁洞天可在传承中购买；图景沿用现有概念背景。")));
        At(65+I*620,290,560,150,Paragraph(TEXT("洞主：")+CN(C.bossName)+TEXT("\n妖魔：")+CN(C.enemies[0])+TEXT("、")+CN(C.enemies[3])+TEXT("\n机缘：")+CN(C.opportunities[0]),540,24,Paper));
    }
    const TCHAR* Modes[]={TEXT("快速 · 四波"),TEXT("传统 · 十波"),TEXT("无尽 · 每五波洞主")};
    for(int I=0;I<3;++I) At(60+I*620,475,580,85,Button(Modes[I],[this,I]{Nav.SelectMode(dywx::Mode(I));if(I==0)Nav.riskMask=0;Rebuild();return FReply::Handled();},int(Nav.mode)==I,I<2||Nav.endlessUnlocked,TEXT("传统首胜后解锁无尽；快速无波间机缘、无劫数。")));
    const auto& Profile=Owner->CampaignProfile();
    At(65,590,1600,60,Label(FString::Printf(TEXT("劫数 · 当前允许风险 %u · 已选 %u · 高品权重与道韵随风险提高"),Profile.unlockedRisk,GetTrialEffects(Nav.riskMask).risk),24,Gold));
    const TCHAR* Risks[]={TEXT("妖潮更多"),TEXT("妖魔更快"),TEXT("大妖更多"),TEXT("异象更凶险")};
    for(int I=0;I<4;++I) At(60+I*465,670,440,80,Button(Risks[I],[this,I]{unsigned Next=Nav.riskMask^(1u<<I);if(GetTrialEffects(Next).risk<=Owner->CampaignProfile().unlockedRisk)Nav.riskMask=Next;else RunNotice=TEXT("风险未解锁：通关当前风险后提升上限。");Rebuild();return FReply::Handled();},(Nav.riskMask&(1u<<I))!=0,Nav.mode!=dywx::Mode::Quick&&HasFeature(Profile,Feature::Trials),TEXT("第三层开放：传统首胜。通关当前风险解锁下一档。")));
    At(60,790,1750,60,Label(RunNotice,22,Gold));
    At(1050,900,360,80,Button(TEXT("传承"),[this]{return OpenModal(dywx::Modal::Legacy);}));
    At(1450,900,400,80,Button(TEXT("进入洞天"),[this]{return Primary();},true,!Nav.savedRuns[Nav.character],TEXT("有进行中存档时请先续接此修士，不覆盖存档。")));
}
TSharedRef<SWidget> SDywxRoot::LiveText(TFunction<FString()> Value, int32 Size, FLinearColor Color)
{
    return SNew(STextBlock).Font(Font(Size)).ColorAndOpacity(Color).AutoWrapText(false)
        .Text_Lambda([Fn=MoveTemp(Value)] { return FText::FromString(Fn()); });
}
void SDywxRoot::BattleScreen()
{
    const float Xs[]={20,1570,800,800},Ys[]={500,500,240,800};
    const TCHAR* Arrows[]={TEXT("←"),TEXT("→"),TEXT("↑"),TEXT("↓")};
    for(int I=0;I<4;++I)At(Xs[I],Ys[I],330,45,LiveText([this,I,Arrow=FString(Arrows[I])]{
        if(!bBeastVisible)return FString();
        int Direction=FMath::Abs(BeastOffset.x)>FMath::Abs(BeastOffset.y)?(BeastOffset.x<0?0:1):(BeastOffset.y>0?2:3);
        return I==Direction?Arrow+FString::Printf(TEXT(" 灵兽 %.0f格"),FMath::Sqrt(BeastOffset.x*BeastOffset.x+BeastOffset.y*BeastOffset.y)):FString();
    },24,Gold));
    At(30,25,500,100,Panel(SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[LiveText([this]{return FString::Printf(TEXT("气血 %.0f / %.0f   护盾 %.0f"),HUD.Health,HUD.MaxHealth,Run.player.shield);},24,Paper)]
        +SVerticalBox::Slot().FillHeight(1).Padding(0,12,0,0)[SNew(SProgressBar).Style(&ProgressStyle).FillColorAndOpacity(FLinearColor(.85f,.12f,.035f)).Percent_Lambda([this]{return TOptional<float>(dywx::SafeFraction(HUD.Health,HUD.MaxHealth));})],Ink,12));
    At(580,25,750,90,Panel(LiveText([this]{return FString::Printf(TEXT("第 %d / %d 波    %02d:%02d    斩妖 %llu"),Run.wave,Run.waveCount,int(HUD.ElapsedSeconds)/60,int(HUD.ElapsedSeconds)%60,static_cast<unsigned long long>(Run.statistics.kills));},26,Paper),Ink,15));
    At(1640,35,230,65,Button(TEXT("暂停  Esc"),[this]{return Pause();}));
    At(35,140,1100,45,LiveText([this]{const auto& R=Run.progression.build.resonance;return FString::Printf(TEXT("道象 火%d 雷%d 金%d 木%d 水%d 土%d · 体%d 剑%d 阵%d 兽%d 符%d"),R.fire,R.thunder,R.metal,R.wood,R.water,R.earth,R.body,R.sword,R.array,R.beast,R.talisman);},22,Gold));
    At(35,185,800,40,Label(TEXT("同道象达到 2 / 4 点共鸣 · 破境继承道象 · C 打开道典"),18,Muted));
    At(35,230,900,40,LiveText([this]{return FString::Printf(TEXT("累计伤害 %.0f    承伤 %.0f    妖潮余时 %.0f 秒"),Run.statistics.damageDealt,Run.statistics.damageTaken,FMath::Max(0.f,Run.waveDuration-Run.waveElapsed));},20,Paper));
    if(Run.phase==RunPhase::Boss)
    {
        At(580,700,750,45,SNew(SBox).ToolTip(SNew(SToolTip)[Paragraph(Owner.IsValid()&&Owner->Simulation()?CN(Owner->Simulation()->DescribeBossMechanics()):TEXT(""),570,20,Paper)])[LiveText([this]{auto* S=Owner.IsValid()?Owner->Simulation():nullptr;const FString Name=S?CN(CaveCatalog()[int(S->GetArena().cave)].bossName):TEXT("洞主");return (Run.statistics.elapsed<BossChangeUntil?FString(TEXT("洞主破相！ ")):FString())+FString::Printf(TEXT("%s · 第 %d 相   %.0f / %.0f"),*Name,Run.bossPhase,BossHP,BossMaxHP);},22,Gold)]);
        At(580,755,750,18,SNew(SProgressBar).Style(&ProgressStyle).FillColorAndOpacity(FLinearColor(.7f,.12f,.6f)).Percent_Lambda([this]{return TOptional<float>(dywx::SafeFraction(BossHP,BossMaxHP));}));
    }
    auto AnomalyText=[this]() -> FString {
        auto* S=Owner.IsValid()?Owner->Simulation():nullptr;if(!S)return FString();
        const auto& A=S->GetAnomalies().State();const auto& C=S->GetAnomalies().Config();if(!A.active)return FString();
        const TCHAR* Icons[]={TEXT("雷"),TEXT("潮"),TEXT("裂"),TEXT("雾"),TEXT("月"),TEXT("阵")};
        FString Key;
        switch(A.id) {
        case AnomalyId::Lightning: Key=FString::Printf(TEXT("预警%.1f秒 · 伤害%.0f · 半径%.1f格"),C.warning,C.lightningDamage,C.hazardRadius);if(Run.progression.build.resonance.thunder>0)Key+=FString::Printf(TEXT(" · 雷象可吸纳，威力×%.2f"),C.thunderReward);break;
        case AnomalyId::SpiritTide: Key=FString::Printf(TEXT("妖潮×%.2f · 灵石×%.2f"),C.tideSpawn,C.tideCurrency);break;
        case AnomalyId::Fissure: Key=FString::Printf(TEXT("预警%.1f秒 · 伤害%.0f · 冲锋权重×%.2f"),C.warning,C.fissureDamage,C.fissureChargers);break;
        case AnomalyId::Fog: Key=FString::Printf(TEXT("远眺%.0f格 · 远程权重×%.2f · 灵兽权重×%.2f"),C.fogVision,C.fogRanged,C.fogBeasts);break;
        case AnomalyId::Eclipse: Key=FString::Printf(TEXT("夜行妖魔威力×%.2f · 修为×%.2f"),C.eclipseEnemyPower,C.eclipseXp);break;
        case AnomalyId::Array: {const float X=Run.player.position.x-A.anchor.x,Y=Run.player.position.y-A.anchor.y;Key=FString::Printf(TEXT("阵台半径%.1f格 · 台内威力×%.2f%s"),C.arrayRadius,C.arrayPower,X*X+Y*Y<=C.arrayRadius*C.arrayRadius?TEXT(" · 当前有利：已在台内"):TEXT(" · 当前未在台内"));break;}
        default:break;
        }
        return FString::Printf(TEXT("【%s】%s · 剩余%.0f秒 · 悬停查看完整规律\n"),Icons[int(A.id)],*CN(AnomalyName(A.id)),A.remaining)+Key;
    };
    At(35,285,990,100,SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.015f,0.026f,0.045f,.45f)).Padding(8)
        .Visibility_Lambda([this]{auto*S=Owner.IsValid()?Owner->Simulation():nullptr;return S&&S->GetAnomalies().State().active&&(Run.phase==RunPhase::Wave||Run.phase==RunPhase::Boss)?EVisibility::Visible:EVisibility::Collapsed;})
        .ToolTip(SNew(SToolTip)[SNew(STextBlock).Font(Font(20)).ColorAndOpacity(Paper).WrapTextAt(570).Text_Lambda([this]{auto*S=Owner.IsValid()?Owner->Simulation():nullptr;return FText::FromString(S?CN(DescribeAnomaly(S->GetAnomalies().State().id,S->GetAnomalies().Config())):FString());})])
        [SNew(STextBlock).Font(Font(18)).ColorAndOpacity(Gold).WrapTextAt(970).Text_Lambda([AnomalyText]{return FText::FromString(AnomalyText());})]);
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
        auto Fusions=SNew(SScrollBox);
        for(const auto& FusionRecipe:FusionCatalog()) {
            if(FusionRecipe.hero!=Run.progression.build.hero)continue;
            const SpellId Fusion=FusionRecipe.result;
        const auto* Sim = Owner.IsValid() ? Owner->Simulation() : nullptr;
        const bool CanFuseNow = Sim && !Run.paused && Sim->GetProgression().CanFuse(Fusion)
            && (Run.phase == RunPhase::Rest || !Run.progression.offers.empty());
        const FString Recipe=CN(DescribeFusion(FusionRecipe));
        Fusions->AddSlot().Padding(0,2)[SNew(SBox).HeightOverride(80)[Button(TEXT("破境 · ")+CN(GetSpellConfig(Fusion).name),
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
            },true,CanFuseNow,Recipe+TEXT("。仅调息或有待选顿悟时可破境；暂停时不可用。破境继承道象，消耗配方指定的术法。"))]];
        }
        At(1360,120,510,100,Fusions);
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
        Card->AddSlot().AutoHeight()[Button(FString::Printf(TEXT("封存 · 余%d"),Run.progression.sealsRemaining),[this,I]{return RunCommand([this,I]{auto* S=Owner->Simulation();RunNotice=S&&S->SealUpgrade(I)?TEXT("本局已封存"):TEXT("不可封存：第二层开放，每局限三次。");});},false,Owner.IsValid()&&HasFeature(Owner->CampaignProfile(),Feature::SealSkip)&&Run.progression.sealsRemaining>0,TEXT("永久从本局候选池移除；每局三次。"))];
        At(1080+I*270,290,260,220,Panel(Card,Ink,6));
    }
    At(1080,520,365,60,Button(TEXT("跳过 · 修为与灵石"),[this]{return RunCommand([this]{auto*S=Owner->Simulation();RunNotice=S&&S->SkipUpgrade()?TEXT("已跳过本次顿悟"):TEXT("不可跳过：需要第二层与有效顿悟。");});},false,Owner.IsValid()&&HasFeature(Owner->CampaignProfile(),Feature::SealSkip)));
    At(1460,520,430,60,Button(FString::Printf(TEXT("重悟 · 剩余 %d 次"),Run.progression.rerollsRemaining),[this]{return RunCommand([this]{if(auto* S=Owner->Simulation())S->Reroll();});},false,Run.progression.rerollsRemaining>0));
}
void SDywxRoot::ResultPanel()
{
    for(int32 I=0;I<int(Run.progression.bossRewards.size()) && I<3;++I)
    {
        const auto R=Run.progression.bossRewards[I];FString Tip=CN(RarityName(R.rarity));for(const auto&A:R.affixes)Tip+=TEXT("\n")+CN(DescribeAffix(A));
        At(430+I*360,220,350,65,Button(CN(RelicCatalog()[R.catalogId].name),[this,I]{return RunCommand([this,I]{if(auto*S=Owner->Simulation()){S->ChooseBossReward(I);}});},false,true,Tip));
    }
    At(510,825,900,60,LiveText([this]{auto*S=Owner.IsValid()?Owner->Simulation():nullptr;return S?FString::Printf(TEXT("本局道韵 %llu · 账号道韵 %llu"),static_cast<unsigned long long>(S->GetCampaign().Reward()),static_cast<unsigned long long>(Owner->CampaignProfile().daoYun)):FString();},24,Gold));
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
        Add(Button(TEXT("保存并离开"),[this]{
            if(Owner.IsValid()&&Owner->SaveRun()) {
                RunNotice=TEXT("已保存并离开；请在修士页续接，成功恢复时消耗存档。");
                Owner->ShowMainMenu();
            } else {
                RunNotice=TEXT("保存失败：仅传统/无尽波间可存；先完成机缘、赠宝与交易，或检查磁盘。待选顿悟可保存。");
                Rebuild();
            }
            return FReply::Handled();
        },false,Run.phase==RunPhase::Rest&&Nav.mode!=dywx::Mode::Quick,TEXT("保存成功才离开本局；失败保持当前暂停。待选顿悟原样保留，不重复领取机缘奖励。")),65);
        Add(Paragraph(RunNotice,596,20,Gold),55);
        Add(Button(TEXT("继续"),[this]{return CloseModal();},true),72);
        Add(Button(TEXT("返回主菜单"),[this]{if(Owner.IsValid()) Owner->ShowMainMenu();else ReturnToMenu();return FReply::Handled();}),70);
        break;
    case dywx::Modal::Codex:
    {
        Add(Label(TEXT("道典"),42,Paper,true),85);
        Add(Paragraph(TEXT("探索为引，认知为赏。洞天万象，皆有其律。"),596,26,Gold,true),100);
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
        auto Entry = [this,&Entries](const FString& Text, bool Seen) {
            Entries->AddSlot().Padding(0,8)[Paragraph(
                (Seen ? TEXT("已见 · ") : TEXT("未见 · ")) + Text,
                570,21,Seen ? Paper : Muted)];
        };
        Entries->AddSlot()[Paragraph(TEXT("灰色为未亲历，点亮只记录发现，不增加局外属性。妖魔进入战场即记入；心法取得、共鸣达到两门后点亮。"),570,21,Gold)];
        for(size_t I=0;I<FusionCatalog().size();++I) Entry(CN(DescribeFusion(FusionCatalog()[I])),Known.seenFusions[I]);
        for (int I=0;I<int(kSpellCount);++I) {
            FString Full;for(int Rank=1;Rank<=5;++Rank)Full+=CN(DescribeSpell(SpellId(I),Rank))+TEXT("\n");
            bool Useful=false;
            for(const auto& Owned:Run.progression.build.spells){const auto A=GetSpellConfig(Owned.id),B=GetSpellConfig(SpellId(I));if(Owned.id==SpellId(I)||A.element==B.element||A.form==B.form)Useful=true;}
            Entry((Useful?TEXT("【当前道象有利】 "):TEXT(""))+Full,Known.seenSpells[I]);
        }
        for (const auto& P : PassiveCatalog()) Entry(CN(P.name)+TEXT("：")+CN(P.description),Known.seenPassives[static_cast<size_t>(P.id)]);
        for (const auto& R : RelicCatalog()) {
            bool Useful=false;for(const auto& O:Run.progression.build.spells){const auto Spell=GetSpellConfig(O.id);if(R.aspect==AspectAffix(ElementResonanceCode(Spell.element))||R.aspect==AspectAffix(FormResonanceCode(Spell.form)))Useful=true;}
            Entry((Useful?TEXT("【当前道象有利】 "):TEXT(""))+CN(DescribeRelic(R.id)),Known.seenRelics[R.id]);
        }
        const TCHAR* EnemyTitles[]={TEXT("魑魅 · 群行"),TEXT("磷火 · 自爆"),TEXT("赤狰 · 冲锋"),TEXT("毕方 · 远程"),TEXT("阵灵 · 护持"),TEXT("蜃 · 召唤"),TEXT("洞主"),TEXT("灵兽 · 逃逸")};
        for(int I=0;I<8;++I)if(EnemyRole(I)!=EnemyRole::Boss)Entry(FString(EnemyTitles[I])+TEXT("\n")+CN(Simulation::DescribeEnemy(EnemyRole(I))),Known.seenEnemies[I]);
        for(size_t I=0;I<CaveCatalog().size();++I)Entry(CN(Simulation::DescribeBossMechanics(CaveId(I))),Owner.IsValid()&&Owner->CampaignProfile().seenBosses[I]);
        auto* CurrentSim=Owner.IsValid()?Owner->Simulation():nullptr;
        Progression Reference;
        const auto& Odds=CurrentSim?CurrentSim->GetProgression():Reference;
        Entries->AddSlot()[Paragraph(TEXT("当前／最近修行掉落规律\n")+CN(Odds.DescribeDropOdds(false))+TEXT("\n大妖与灵兽保底\n")+CN(Odds.DescribeDropOdds(true)),570,21,Gold)];
        Entries->AddSlot()[Paragraph(CN(Simulation::DescribeElementInteractions()),570,21,Gold)];
        for(int I=0;I<4;++I)Entry(CN(Simulation::DescribeMutation(unsigned(I))),Known.seenMutations[I]);
        Entries->AddSlot()[Paragraph(TEXT("道象计数包含术法与心法倾向；破境继承双方计数。元素轴始终启用，形态轴依本局配置启用。"),570,21,Gold)];
        for(int I=0;I<int(kResonanceCount);++I) Entry(CN(DescribeResonance(I)),Known.seenResonances[I]);
        for(int I=0;I<int(AnomalyId::Count);++I) {
            auto* Sim=Owner.IsValid()?Owner->Simulation():nullptr;
            FString Favorable;
            if(Nav.liveBattle && Sim) {
                const auto& A=Sim->GetAnomalies().State();const auto& Config=Sim->GetAnomalies().Config();
                if(AnomalyId(I)==AnomalyId::Lightning && Run.progression.build.resonance.thunder>0)Favorable=TEXT("【当前雷象有利：可吸纳引雷】 ");
                const float Dx=Run.player.position.x-A.anchor.x,Dy=Run.player.position.y-A.anchor.y;
                if(AnomalyId(I)==AnomalyId::Array && A.active && A.id==AnomalyId::Array && Dx*Dx+Dy*Dy<=Config.arrayRadius*Config.arrayRadius)Favorable=TEXT("【当前站在阵台内：威力提高】 ");
            }
            Entry(Favorable+CN(DescribeAnomaly(AnomalyId(I),Sim?Sim->GetAnomalies().Config():AnomalyConfig{})),(Owner.IsValid()&&Owner->CampaignProfile().seenAnomalies[I])||(Sim&&Sim->GetAnomalies().State().encountered[I]));
        }
        Entries->AddSlot()[Paragraph(TEXT("第二层：首胜开放形态共鸣、灵石坊市、封存跳过、异象。第三层：传统首胜开放重铸、宝匣、劫数、无尽。整理工具始终可用。"),570,21,Gold)];
        Add(Entries,270);
        Add(Button(TEXT("返回"),[this]{return CloseModal();},true),70);
        break;
    }
    case dywx::Modal::Legacy:
    {
        const auto& P=Owner->CampaignProfile();
        Add(Label(FString::Printf(TEXT("传承 · 道韵 %llu · 第 %u 层"),static_cast<unsigned long long>(P.daoYun),FeatureTier(P)),30,Gold),65);
        Add(Paragraph(TEXT("首胜开启第二层；传统首胜开启第三层与无尽。永久增益总计上限10%，气运不入传承。"),596,21,Paper),90);
        auto List=SNew(SScrollBox);
        const TCHAR* Unlocks[]={TEXT("巡星羽客"),TEXT("灵契山人"),TEXT("第二洞天"),TEXT("第三洞天"),TEXT("扩展术法候选"),TEXT("扩展法宝候选"),TEXT("额外重悟"),TEXT("每处洞府免费刷新")};
        for(int I=0;I<int(LegacyUnlock::Count);++I) List->AddSlot().Padding(0,5)[Button(FString(Unlocks[I])+(I==int(LegacyUnlock::HeroSword)?TEXT(" · 初始开放"):(P.unlocks[I]?TEXT(" · 已解锁"):TEXT(" · 20道韵"))),[this,I]{RunNotice=Owner->PurchaseLegacy(LegacyUnlock(I))?TEXT("传承已解锁"):TEXT("未能购买：道韵不足、已解锁或资料保存失败。");Rebuild();return FReply::Handled();},false,I!=int(LegacyUnlock::HeroSword)&&!P.unlocks[I]&&P.daoYun>=20,TEXT("账号共享，影响后续新局。"))];
        const TCHAR* Stats[]={TEXT("气血"),TEXT("护体"),TEXT("生机"),TEXT("威力"),TEXT("周天"),TEXT("范围"),TEXT("会心"),TEXT("身法"),TEXT("摄灵"),TEXT("遁息")};
        for(int I=0;I<int(PermanentStat::Count);++I) List->AddSlot().Padding(0,5)[Button(FString::Printf(TEXT("%s · 当前 %.0f%% · 10道韵"),Stats[I],PermanentBonus(P,PermanentStat(I))*100),[this,I]{RunNotice=Owner->PurchasePermanent(PermanentStat(I))?TEXT("传承提升"):TEXT("购买失败：道韵不足、总增益达到上限或保存失败。");Rebuild();return FReply::Handled();},false,P.daoYun>=10)];
        for(const auto& H:P.heroes) List->AddSlot()[Label(FString::Printf(TEXT("修士 %d · 最佳 %d波 · 熟练度 %llu · 胜场 %llu"),H.hero+1,H.bestWave,static_cast<unsigned long long>(H.mastery),static_cast<unsigned long long>(H.wins)),20,Paper)];
        List->AddSlot().Padding(0,12)[Label(TEXT("无尽波数排行 · 本地，不联网"),24,Gold)];
        if(P.endlessRanking.empty())List->AddSlot()[Label(TEXT("尚无无尽结算记录"),20,Muted)];
        for(size_t I=0;I<P.endlessRanking.size();++I) {
            const auto& Record=P.endlessRanking[I];
            const FString Hero=Record.hero>=0&&Record.hero<3?FString(Names[Record.hero]):TEXT("修士");
            List->AddSlot().Padding(0,5)[Paragraph(FString::Printf(TEXT("%d · %s · 第 %d 波\n斩妖 %llu"),int(I)+1,*Hero,Record.wave,static_cast<unsigned long long>(Record.kills)),560,20,Paper)];
        }
        Add(List,400);Add(Paragraph(RunNotice,596,20,Gold),55);Add(Button(TEXT("返回"),[this]{return CloseModal();}),65);break;
    }
    case dywx::Modal::Economy:
    {
        auto* S=Owner.IsValid()?Owner->Simulation():nullptr;
        if(!S) break;
        const auto& E=S->GetEconomy();
        const auto& Campaign=S->GetCampaign().GetState();
        const bool Cave=Campaign.selected>=0&&Campaign.selected<int(Campaign.offerCount)&&Campaign.offers[Campaign.selected]==OpportunityKind::Cave;
        const bool Layer2=HasFeature(Owner->CampaignProfile(),Feature::Economy);
        const bool Layer3=HasFeature(Owner->CampaignProfile(),Feature::Reforge);
        Add(Label(FString::Printf(TEXT("洞府与宝匣 · 灵石 %d"),E.Stones()),30,Gold),65);
        auto List=SNew(SScrollBox);
        if(!Layer2)List->AddSlot()[Paragraph(TEXT("洞府祭炼已开放；灵石商店与刷新须首胜开启第二层。"),560,20,Muted)];
        auto Action=[this](TFunction<bool(dywx::mvp::Simulation&)> Fn) {
            auto* Sim=Owner.IsValid()?Owner->Simulation():nullptr;
            if(!Sim||Nav.modal!=dywx::Modal::Economy||Sim->GetSnapshot().generation!=Run.generation||Sim->GetSnapshot().phase!=RunPhase::Rest||Sim->GetSnapshot().paused) return FReply::Handled();
            std::vector<EntityId> Before;
            for(const auto& R:Sim->GetSnapshot().progression.bag)Before.push_back(R.id);
            for(const auto& R:Sim->GetSnapshot().progression.equipped)Before.push_back(R.id);
            RunNotice=Fn(*Sim)?TEXT("操作完成"):TEXT("不可用：检查层级、机缘、余额、锁定或待选交易。未扣除额外费用。");
            auto Reveal=[&](const Relic& R){bool Existed=false;for(auto Id:Before)if(Id==R.id)Existed=true;if(!Existed)RunNotice+=TEXT("\n获得：")+CN(RelicCatalog()[R.catalogId].name)+TEXT(" · ")+CN(RarityName(R.rarity))+(Sim->GetEconomy().IsUnidentified(R.id)?TEXT(" · 未祭炼"):TEXT(""));};
            for(const auto& R:Sim->GetSnapshot().progression.bag)Reveal(R);
            for(const auto& R:Sim->GetSnapshot().progression.equipped)Reveal(R);
            Owner->RefreshRunUI();Rebuild();return FReply::Handled();
        };
        for(const auto& G:E.Shop()) {
            FString Tip;for(const auto& A:G.relic.affixes)Tip+=CN(DescribeAffix(A))+TEXT("\n");
            List->AddSlot().Padding(0,5)[Button(CN(RelicCatalog()[G.relic.catalogId].name)+FString::Printf(TEXT(" · %d灵石%s"),G.price,G.sold?TEXT(" · 售罄"):TEXT("")),[Action,Token=G.token]()mutable{return Action([Token](Simulation& Sim){return Sim.BuyGood(Token);});},false,Layer2&&!G.sold&&E.Stones()>=G.price,Tip)];
        }
        List->AddSlot().Padding(0,5)[Button(TEXT("刷新洞府 · 免费传承或15灵石"),[Action]()mutable{return Action([](Simulation& Sim){return Sim.RefreshShop();});},false,Layer2&&Cave,TEXT("第二层开启；仅洞府可用；快速不可刷新；每处免费一次需传承。"))];
        const auto Prob=E.TreasureProbabilities(S->GetProgression());
        List->AddSlot()[Paragraph(FString::Printf(TEXT("第三层开放重铸与宝匣。\n宝匣概率：凡 %.2f%% / 灵 %.2f%% / 仙 %.2f%% / 道 %.2f%%\n已额外开启 %d / 5，仙品受20波保底，道品无保底。"),Prob[0]*100,Prob[1]*100,Prob[2]*100,Prob[3]*100,E.PaidTreasureCount()),560,20,Gold)];
        List->AddSlot()[Button(FString::Printf(TEXT("额外开匣 · %d灵石"),E.TreasurePrice()),[Action]()mutable{return Action([](Simulation& Sim){return Sim.OpenTreasure(true);});},false,Layer3&&E.Stones()>=E.TreasurePrice()&&E.PaidTreasureCount()<5,TEXT("第三层开放；余额须足够；每局额外开启最多五次。"))];
        if(E.HasTreasureToReveal()) List->AddSlot()[Button(TEXT("揭示匣中法宝"),[Action]()mutable{return Action([](Simulation& Sim){return Sim.RevealTreasure();});},true)];
        if(E.PendingReforge()) {
            const auto& R=*E.PendingReforge();
            List->AddSlot()[Paragraph(TEXT("已付费 · 旧纹：")+CN(DescribeAffix(R.oldAffix))+TEXT("\n新纹：")+CN(DescribeAffix(R.newAffix)),560,22,Gold)];
            for(int I=0;I<2;++I)List->AddSlot()[Button(I?TEXT("保留新纹"):TEXT("保留旧纹（不退款）"),[Action,I]()mutable{return Action([I](Simulation& Sim){return Sim.ResolveReforge(I!=0);});})];
        }
        auto Item=[&](const Relic& R) {
            if(E.IsUnidentified(R.id)) { List->AddSlot()[Button(TEXT("祭炼 · ")+CN(RelicCatalog()[R.catalogId].name),[Action,Id=R.id]()mutable{return Action([Id](Simulation& Sim){return Sim.IdentifyRelic(Id);});},false,Cave,TEXT("仅所选洞府可祭炼；未祭炼灵纹不生效。"))];return; }
            List->AddSlot().Padding(0,8)[Label(CN(RelicCatalog()[R.catalogId].name),21,Gold)];
            for(size_t I=0;I<R.affixes.size();++I)if(R.affixes[I].kind!=AffixKind::Mystery&&R.affixes[I].kind!=AffixKind::Unique)List->AddSlot()[Button(FString::Printf(TEXT("重铸 · %d灵石 · "),E.ReforgePrice(R.id))+CN(AffixName(R.affixes[I])),[Action,Id=R.id,I]()mutable{return Action([Id,I](Simulation& Sim){return Sim.BeginReforge(Id,I);});},false,Layer3&&Cave&&!R.locked&&!E.PendingReforge(),CN(DescribeAffix(R.affixes[I])))];
        };
        for(const auto& R:Run.progression.equipped)Item(R);for(const auto& R:Run.progression.bag)Item(R);
        Add(List,470);Add(Paragraph(RunNotice,596,19,Gold),70);Add(Button(TEXT("返回整理"),[this]{return CloseModal();}),65);break;
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
            if (Owner.IsValid()) Owner->RequestRun(Nav.character,Nav.map,static_cast<int32>(Nav.mode),Nav.riskMask);
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
    if (Nav.modal == dywx::Modal::None) {
        bModalPausedRun=Nav.liveBattle && Modal!=dywx::Modal::Economy && Owner.IsValid();
        if(bModalPausedRun)Owner->SetUIPaused(true);
        Nav.modal=Modal;Rebuild();SyncInput();
    }
    return FReply::Handled();
}
FReply SDywxRoot::CloseModal()
{
    const bool WasPause=Nav.modal==dywx::Modal::Pause || bModalPausedRun;
    bModalPausedRun=false;
    Nav.modal=dywx::Modal::None; Rebuild(); SyncInput();
    if (WasPause && Owner.IsValid()) Owner->SetUIPaused(false);
    return FReply::Handled();
}
void SDywxRoot::HandleFocusLoss()
{
    if(!Nav.FocusLoss()) return;
    // Discard only the modal view, never the pending core transaction. A player
    // explicitly continues, then can reopen the economy to resolve that choice.
    bModalPausedRun=false;
    if(Owner.IsValid())Owner->SetUIPaused(true);
    Rebuild();SyncInput();
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
    if (Event.GetKey()==EKeys::C && Nav.modal==dywx::Modal::None) return OpenModal(dywx::Modal::Codex);
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
    Nav.screen=dywx::Screen::Battle;Nav.modal=dywx::Modal::None;Nav.liveBattle=true;bModalPausedRun=false;
    if(Owner.IsValid())if(auto* S=Owner->Simulation())if(S->IsCampaign()) {
        const auto& C=S->GetCampaign().GetState();Nav.character=C.hero;Nav.mode=dywx::Mode(int(C.mode));Nav.map=int(S->GetArena().cave);Nav.riskMask=C.riskMask;
    }
    SetHUDState(State);Rebuild();SyncInput();
}
void SDywxRoot::ReturnToMenu()
{
    bModalPausedRun=false;Nav.ReturnToMenu();Rebuild();SyncInput();
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
    if(State.bossPhase>Run.bossPhase)BossChangeUntil=State.statistics.elapsed+4.f;
    Run.generation=State.generation;Run.phase=State.phase;Run.paused=State.paused;Run.player=State.player;
    Run.wave=State.wave;Run.waveCount=State.waveCount;Run.waveElapsed=State.waveElapsed;Run.waveDuration=State.waveDuration;Run.bossPhase=State.bossPhase;
    Run.statistics=State.statistics;Run.progression=State.progression;Run.spellCooldowns=State.spellCooldowns;
    bBeastVisible=false;for(const auto& E:State.enemies)if(E.role==EnemyRole::SpiritBeast){BeastOffset={E.position.x-State.player.position.x,E.position.y-State.player.position.y};bBeastVisible=true;break;}
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
    if(Run.progression.offers.empty()) {
        auto Choices=SNew(SScrollBox);
        const auto& C=Sim->GetCampaign().GetState();
        if(C.offerCount && C.selected<0) {
            for(unsigned I=0;I<C.offerCount;++I) {
                const auto Details=Sim->DescribeOpportunity(I);
                Choices->AddSlot().Padding(0,4)[Button(CN(Details.title),[this,I]{return RunCommand([this,I]{auto*S=Owner->Simulation();RunNotice=S&&S->ChooseOpportunity(I)?TEXT("机缘已选择"):TEXT("不可选择：机缘已解决或状态改变。");});},false,true,CN(Details.description))];
                Choices->AddSlot().Padding(8,0,8,12)[Paragraph(CN(Details.description),780,20,Paper)];
            }
        }
        if(C.selected>=0&&C.selected<int(C.offerCount)) {
            const auto Details=Sim->DescribeOpportunity(unsigned(C.selected));
            Choices->AddSlot()[Paragraph(CN(Details.title)+TEXT("\n")+CN(Details.description),780,20,Gold)];
        }
        const bool CaveSelected=C.selected>=0&&C.selected<int(C.offerCount)&&C.offers[C.selected]==OpportunityKind::Cave;
        for(size_t I=0;I<Run.progression.bossRewards.size();++I)Choices->AddSlot()[Button(TEXT("洞主赠宝 · ")+CN(RelicCatalog()[Run.progression.bossRewards[I].catalogId].name),[this,I]{return RunCommand([this,I]{if(auto*S=Owner->Simulation())S->ChooseBossReward(I);});})];
        Choices->AddSlot()[Button(FString::Printf(TEXT("灵石 %d · 洞府 / 重铸 / 宝匣"),Sim->GetEconomy().Stones()),[this]{return OpenModal(dywx::Modal::Economy);},false,dywx::CanOpenRestServices(HasFeature(Owner->CampaignProfile(),Feature::Economy),CaveSelected),TEXT("所选洞府第一层即可祭炼；商店首胜开启，重铸宝匣第三层开启。"))];
        Choices->AddSlot()[Button(bRefineStones?TEXT("炼化所得：灵石（点击切换）"):TEXT("炼化所得：修为（点击切换）"),[this]{bRefineStones=!bRefineStones;bRefinePreview=false;Rebuild();return FReply::Handled();},false,HasFeature(Owner->CampaignProfile(),Feature::Economy))];
        At(1080,290,810,280,Choices);
        if(C.offerCount && C.selected<0) {
            At(40,300,980,150,Panel(Paragraph(TEXT("波间机缘 · 三选一\n每三波至少遇见洞府与宝匣各一次；选择并解决机缘后整理法宝，再手动迎战。"),920,26,Gold),Ink,20));
            return;
        }
    }
    At(30,285,1010,550,Panel(SNew(SBox),Ink,0));
    At(45,290,980,55,Label(FString::Printf(TEXT("调息 · 战斗暂停   炼化待入账修为 %d"),P.PendingRefineXp()+Sim->GetEconomy().PendingXp()),26,Gold,true));
    RelicFilter Filter;Filter.rarity=RarityFilter;Filter.aspect=AspectFilter;Filter.affixKind=AffixFilter;
    const auto Visible=P.FilterBag(Filter);
    const TCHAR* Aspects[]={TEXT("全部道象"),TEXT("火"),TEXT("雷"),TEXT("法体"),TEXT("飞剑"),TEXT("阵法"),TEXT("金"),TEXT("木"),TEXT("水"),TEXT("土"),TEXT("御兽"),TEXT("符箓")};
    At(45,350,315,60,Button(RarityFilter<0?TEXT("品阶：全部"):TEXT("品阶：")+CN(RarityName(Rarity(RarityFilter))),[this]{RarityFilter=(RarityFilter+2)%5-1;bRefinePreview=false;Rebuild();return FReply::Handled();}));
    At(370,350,315,60,Button(Aspects[AspectFilter+1],[this]{AspectFilter=(AspectFilter+2)%(int(kResonanceCount)+1)-1;bRefinePreview=false;Rebuild();return FReply::Handled();}));
    At(695,350,325,60,Button(AffixFilter<0?TEXT("灵纹：全部"):TEXT("灵纹：")+CN(AffixName({AffixKind(AffixFilter),0,0})),[this]{AffixFilter=(AffixFilter+2)%(int(AffixKind::Count)+1)-1;bRefinePreview=false;Rebuild();return FReply::Handled();}));
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
        FString Tip=CN(RarityName(Item->rarity))+(Sim->GetEconomy().IsUnidentified(Item->id)?TEXT(" · 未祭炼（洞府显纹）"):TEXT(""));for(const auto&A:Item->affixes)Tip+=TEXT("\n")+CN(DescribeAffix(A));
        const auto Compare=P.PreviewEquip(Id,SelectedSlot);const auto Current=Run.progression.build.stats;
        Tip+=FString::Printf(TEXT("\n对比第 %d 槽：气血 %+.0f / 威力 %+.0f%% / 护体 %+.0f%% / 周天 %+.0f%%"),SelectedSlot+1,Compare.maxHp-Current.maxHp,(Compare.power-Current.power)*100,(Compare.armor-Current.armor)*100,(Compare.cooldownReduction-Current.cooldownReduction)*100);
        Tip+=FString::Printf(TEXT("\n生机 %.1f→%.1f · 范围 %.0f%%→%.0f%%\n会心 %.0f%%→%.0f%% · 会心伤害 %.0f%%→%.0f%%\n身法 %.1f→%.1f · 摄灵 %.1f→%.1f\n遁息 %.1f→%.1f秒 · 气运 %.1f→%.1f\n选择后查看红升绿降的完整逐项比较。"),Current.regen,Compare.regen,Current.area*100,Compare.area*100,Current.critChance*100,Compare.critChance*100,Current.critMultiplier*100,Compare.critMultiplier*100,Current.moveSpeed,Compare.moveSpeed,Current.pickupRadius,Compare.pickupRadius,Current.dashCooldown,Compare.dashCooldown,Current.luck,Compare.luck);
        Bag->AddSlot().Padding(0,3)[SNew(SBox).HeightOverride(60)[Button(CN(RelicCatalog()[Item->catalogId].name)+(Item->locked?TEXT(" [锁]"):TEXT("")),[this,Id]{SelectedBag=Id;bRefinePreview=false;Rebuild();return FReply::Handled();},false,true,Tip,false,Comparison(Current,Compare,Tip))]];
    }
    if(Visible.empty())Bag->AddSlot()[Label(TEXT("储物袋 · 暂无法宝"),22,Muted)];
    At(45,420,475,275,Equipped);At(540,420,480,275,Bag);
    At(45,705,225,60,Button(TEXT("换装"),[this]{return RunCommand([this]{auto*S=Owner->Simulation();if(S&&S->GetSnapshot().phase==RunPhase::Rest){RunNotice=S->GetProgression().Equip(SelectedBag,SelectedSlot)?TEXT("换装完成"):TEXT("无法换装：请选择首个空槽，或检查互斥灵纹。");S->RefreshProgression();}});},false,SelectedBag!=0));
    At(280,705,225,60,Button(TEXT("卸下"),[this]{return RunCommand([this]{auto*S=Owner->Simulation();if(S&&S->GetSnapshot().phase==RunPhase::Rest){S->GetProgression().Unequip(SelectedSlot);S->RefreshProgression();}});},false,SelectedSlot<int(Run.progression.equipped.size())));
    At(515,705,245,60,Button(TEXT("锁定 / 解锁"),[this]{return RunCommand([this]{auto*S=Owner->Simulation();if(!S||S->GetSnapshot().phase!=RunPhase::Rest)return;auto&P=S->GetProgression();uint64 Id=SelectedBag;if(!Id&&SelectedSlot<int(Run.progression.equipped.size()))Id=Run.progression.equipped[SelectedSlot].id;for(const auto&R:Run.progression.bag)if(R.id==Id){P.SetLocked(Id,!R.locked);S->RefreshProgression();return;}for(const auto&R:Run.progression.equipped)if(R.id==Id){P.SetLocked(Id,!R.locked);S->RefreshProgression();return;}}); }));
    At(770,705,250,60,Button(TEXT("一键排序"),[this]{return RunCommand([this]{if(auto*S=Owner->Simulation()){S->GetProgression().SortBag();S->RefreshProgression();}});}));
    At(45,775,470,60,Button(TEXT("炼化筛选结果 · 先预览"),[this]{bRefinePreview=true;Rebuild();return FReply::Handled();},false,!Visible.empty()));
    At(540,775,480,60,Button(TEXT("撤销上次炼化"),[this]{return RunCommand([this]{if(auto*S=Owner->Simulation()){RunNotice=S->UndoRefine()?TEXT("已撤销炼化，返还法宝并扣回所得。"):TEXT("无法撤销：没有可撤销记录，或所得灵石已花费、余额不足。");}});},false,true));
    if(bRefinePreview)
    {
        const auto Preview=Sim->GetEconomy().PreviewRefine(P,Visible,bRefineStones?RefineReward::Stones:RefineReward::Experience);
        FString List;for(auto Id:Preview.ids)for(const auto&R:Run.progression.bag)if(R.id==Id)List+=CN(RelicCatalog()[R.catalogId].name)+TEXT("  ");
        At(1080,590,810,70,Paragraph(FString::Printf(TEXT("炼化 %d 件，获得 %d %s"),int(Preview.ids.size()),Preview.amount,bRefineStones?TEXT("灵石"):TEXT("修为（迎战时入账）")),800,22,Gold));
        At(1080,655,810,75,SNew(SScrollBox)+SScrollBox::Slot()[Paragraph(List,790,19,Paper)]);
        At(1080,740,395,60,Button(TEXT("确认炼化"),[this,Ids=Preview.ids]{return RunCommand([this,Ids]{if(auto*S=Owner->Simulation()){RunNotice=S->Refine(Ids,bRefineStones?RefineReward::Stones:RefineReward::Experience)?TEXT("炼化完成，可在迎战前撤销。"):TEXT("炼化失败：当前法宝锁定、交易占用或状态已改变。");}bRefinePreview=false;SelectedBag=0;});},false,!Preview.ids.empty()));
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
            Inspection->AddSlot().AutoHeight()[Comparison(Run.progression.build.stats,P.PreviewEquip(SelectedBag,SelectedSlot))];
        }
        At(1080,590,810,150,SNew(SScrollBox)+SScrollBox::Slot()[Inspection]);
        At(1080,750,810,70,Button(Run.progression.pendingUpgrades>0?TEXT("先完成待选顿悟"):TEXT("迎战 · 炼化修为入账"),[this]{return RunCommand([this]{if(auto*S=Owner->Simulation())RunNotice=S->BeginNextWave()?TEXT(""):TEXT("尚不能迎战：先完成机缘、顿悟、洞主赠宝或待选重铸/宝匣。");});},true,Run.progression.pendingUpgrades==0));
    }
}
