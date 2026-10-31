#!/usr/bin/env python3
"""Five-screen source regressions. NOT Unreal compile, Slate input or render proof."""
from pathlib import Path
import json, hashlib
from PIL import Image
R = Path(__file__).resolve().parents[1]
u = (R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text()
h = (R/'Source/DywxUI/Private/SDywxRoot.h').read_text()
c = (R/'Source/DywxUI/Private/DywxUIPlayerController.cpp').read_text()
def section(start, end):
    return u.split(start, 1)[1].split(end, 1)[0]
main=section('void SDywxRoot::MainScreen()', 'void SDywxRoot::CharacterScreen()')
hero=section('void SDywxRoot::CharacterScreen()', 'void SDywxRoot::MapScreen()')
realm=section('void SDywxRoot::MapScreen()', 'TSharedRef<SWidget> SDywxRoot::LiveText')
select=section('void SDywxRoot::HandleUpgradeSelection', 'FReply SDywxRoot::ConfirmUpgrade')
confirm=section('FReply SDywxRoot::ConfirmUpgrade', 'void SDywxRoot::UpgradePanel')
upgrade=section('void SDywxRoot::UpgradePanel', 'void SDywxRoot::ResultPanel')
keys=section('FReply SDywxRoot::OnKeyDown', 'void SDywxRoot::SyncInput')
primary=section('FReply SDywxRoot::Primary', 'FReply SDywxRoot::Back')
# Locate the balanced non-upgrade branch: covered widgets must not exist in the
# focus tree, rather than merely being painted over by the illustration.
battle=section('void SDywxRoot::BattleScreen()', 'FReply SDywxRoot::RunCommand')
start=battle.index('{',battle.index('if(!bUpgradeOpen)'))
depth=1
end=start+1
while depth:
    depth+=(battle[end]=='{')-(battle[end]=='}')
    end+=1
uncovered=battle[start:end]
checks={
 'viewport resize is presentation only': all(x in section('void SDywxRoot::Tick(', 'FSlateFontInfo SDywxRoot::Font') for x in ['GetViewportSize','ReferenceScale','FontChanged || LayoutChanged','Rebuild()']) and not any(x in section('void SDywxRoot::Tick(', 'FSlateFontInfo SDywxRoot::Font') for x in ['Session()', 'SelectedUpgrade=', 'SetUIPaused(', 'ChooseUpgrade(']),
 'footer is restricted to main and cannot overlap selection CTA': 'if (Nav.screen == dywx::Screen::Main)\n        At(1450,1020,420,44,' in u,
 'font uses tested shared policy': 'dywx::FontPoints(Size,ResolutionScale,UIScale)' in u and 'const bool bSmallViewport=ResolutionScale<1.0' in battle,
 'minimum small-profile pointer controls are 32px at720p': all(x in u for x in ['bSmallViewport?48:40,CombatButton(TEXT("暂停 Esc")', 'At(825,RowY+104,155,48,', 'At(40,990,160,48,', 'At(1660,955,205,52,']) and 48*2/3>=32,
 'combat primary numbers retain readable font size': 'At(28,0,600,44,' in battle and 'At(650,24,700,46,' in battle and 'Run.player.shield);},20,Paper)' in battle and 'static_cast<unsigned long long>(Run.statistics.kills));},20,Paper)' in battle,
 'confirmation refreshes external mutations before selection read': confirm.index('Owner->RefreshRunUI()')<confirm.index('return RunCommand'),
 'six native main actions, genuine continuation and currency': main.count('Button(TEXT(')==6 and all(x in main for x in ['HasSave=Nav.savedRuns[0]||Nav.savedRuns[1]||Nav.savedRuns[2]', 'false,HasSave', 'CampaignProfile().daoYun']),
 'clean scenes selected by actual hero/realm': all(x in u for x in ['concept-main-bg.png','concept-body-bg.png','concept-sword-bg.png','concept-beast-bg.png','concept-thunder-bg.png','concept-ember-bg.png','concept-mist-bg.png','Background=Heroes[FMath::Clamp(Nav.character,0,2)]','Background=Realms[FMath::Clamp(Nav.map,0,2)]']),
 'no fake battle screenshot': 'if(Nav.screen!=dywx::Screen::Battle)\n        At(0,0,1920,1080' in u and '04-battle-hud' not in u and 'battle-background.png' not in u,
 'square portrait atlas thirds and strong texture ownership': all(x in u for x in ['concept-portraits.png','Index / 3.0f','(Index + 1) / 3.0f','FVector2D(724, 724)','Textures.Emplace(Texture)']),
 'ornament crop and alpha frame contract': all(x in u for x in ['24.f/1983.f,104.f/793.f','1975.f/1983.f,720.f/793.f','0.2762431f','0.7182320f','Frame->DrawAs=ESlateBrushDrawType::Border','FMargin(.13f,.12f,.13f,.12f)']),
 'character sidebar keeps real starter build and save controls': all(x in hero for x in ['SceneSidebar()', 'Preview.Reset(1,HeroId(Nav.character))','GetSpellConfig(Spell.id,Spell.rank)','PassiveCatalog()', 'Nav.heroes[I]', 'Nav.savedRuns[Nav.character]', 'Owner->ResumeRun(Nav.character)']),
 'realm advanced options collapsed by default without losing risk gating': 'bool bAdvancedSettings = false' in h and all(x in realm for x in ['SceneSidebar()', 'if(bAdvancedSettings)', 'Nav.realms[I]', 'Nav.endlessUnlocked', 'Profile.unlockedRisk', 'HasFeature(Profile,Feature::Trials)', 'Nav.mode!=dywx::Mode::Quick', 'Nav.savedRuns[Nav.character]']),
 'selection does not consume, pause or resume': 'SelectedUpgrade=Index' in select and 'SelectedOffer=Now.progression.offers[Index]' in select and 'ChooseUpgrade(' not in select and 'SetUIPaused' not in select,
 'confirmation rechecks identity, generation and pending count': all(x in confirm for x in ['S->GetSnapshot().paused','S->IsAwaitingUpgrade()', 'Now.generation!=SelectedUpgradeGeneration', 'Now.progression.pendingUpgrades!=SelectedUpgradePending', 'I<0 || I>=int32(Now.progression.offers.size())','Current.kind!=SelectedOffer.kind','Current.spell!=SelectedOffer.spell','Current.passive!=SelectedOffer.passive']) and confirm.index('SelectedUpgrade=-1')<confirm.index('Owner->ChooseUpgrade(I)'),
 'keyboard number keys select only and Enter confirms': 'HandleUpgradeSelection(' in keys and 'Owner->ChooseUpgrade' not in keys and 'EKeys::Enter) return Primary()' in keys and '!Run.progression.offers.empty()) return ConfirmUpgrade()' in primary and all(f'RootWidget->HandleUpgradeSelection({i})' in c for i in range(3)),
 'pending upgrade blocks UI dash': 'Run.progression.offers.empty() && HUD.DashCooldownRemaining' in u,
 'upgrade has paused label and left stacked native choices': all(x in upgrade for x in ['战斗已暂停', 'At(95,RowY,900,160,Framed(SNew(SButton)', 'Epoch==UIEpoch', 'HandleUpgradeSelection(I)', 'ConfirmUpgrade()', 'Run.progression.rerollsRemaining>0', 'concept-upgrade-bg.png']) and '.OnHovered' not in upgrade,
 'offer mutations clear selection, not manual pause': all(x in upgrade for x in ['SelectedUpgrade=-1;', 'SealUpgrade(I)', 'Reroll()', 'SkipUpgrade()']) and 'SetUIPaused' not in upgrade and 'if(Changed) { SelectedUpgrade=-1;' in u,
 'upgrade excludes covered HUD and rest from focus tree': all(x in uncovered for x in ['RestPanel();','ResultPanel();','CombatButton(TEXT("遁术 Space")']) and battle.index('if(bUpgradeOpen) UpgradePanel();')>end and battle.index('auto Fusions=')>end,
 'upgrade shield consumes pointer presses': 'At(0,0,1920,1080,SNew(SBorder)' in upgrade and '.OnMouseButtonDown_Lambda([](const FGeometry&,const FPointerEvent&){return FReply::Handled();})' in upgrade,
 'fusion invalidates pending selection before command': 'SelectedUpgrade=-1; bFusionOpen=false; Commands->Fuse(Fusion);' in battle,
 'external build and charge changes invalidate same-ID offers': all(x in u for x in ['Before.hero!=After.hero','Before.spells.size()!=After.spells.size()','Before.passives!=After.passives','Before.spells[I].id!=After.spells[I].id','Before.spells[I].rank!=After.spells[I].rank','Run.progression.sealsRemaining!=State.progression.sealsRemaining','Run.progression.skippedUpgrades!=State.progression.skippedUpgrades','if(Changed) { SelectedUpgrade=-1;']),
 'upgrade scroll and seal have separate disjoint viewports': all(x in upgrade for x in ['At(265,RowY+108,530,44,SNew(SScrollBox)', 'Paragraph(Desc,500,18,Paper)', 'At(825,RowY+104,155,48,']) and 265+530<825,
 'image row padding fits image and style adds no hidden padding': all(x in u for x in ['RowNormalButton.SetNormalPadding(FMargin(0))','RowNormalButton.SetPressedPadding(FMargin(0))','RowPrimaryButton.SetNormalPadding(FMargin(0))','RowPrimaryButton.SetPressedPadding(FMargin(0))']) and '.ContentPadding(FMargin(10,6))' in hero and '.HeightOverride(94)' in hero and 94+2*6<=110 and '.ContentPadding(FMargin(10,2))' in realm and '.HeightOverride(100)' in realm and 100+2*2<=110,
 'explicit padding styles used by image, mode and combat controls': 'Nav.character==I?&RowPrimaryButton:&RowNormalButton' in hero and 'Nav.map==I?&RowPrimaryButton:&RowNormalButton' in realm and 'int(Nav.mode)==I?&RowPrimaryButton:&RowNormalButton' in realm and 'SelectedUpgrade==I?&RowPrimaryButton:&RowNormalButton' in upgrade and 'SNew(SButton).ButtonStyle(&RowNormalButton).ContentPadding(FMargin(5,2))' in u,
 'passive wrap leaves dedicated scrollbar allowance': 'At(95,875,440,95,SNew(SScrollBox)' in hero and 'Paragraph(CN(Passive.description),410,20,Paper)' in hero and 440-410>=30,
 'passive description is independently scrollable': 'At(95,875,440,95,SNew(SScrollBox)' in hero and 'Paragraph(CN(Passive.description),410,20,Paper)' in hero,
 'framed face rows and active stepper': 'FacePortrait(I)' in hero and 'Framed(SNew(SButton)' in hero and all(x in u for x in ['const int ActiveStep=Nav.screen==dywx::Screen::Character?0:1', 'const bool Active=I==ActiveStep', 'Framed(Steps)']),
 'HUD uses real circular portrait atlas by selected hero': 'ConceptCell(TEXT("concept-portrait-medallions.png"),Nav.character)' in uncovered,
 'realm rows use actual thumbnail atlas thirds': 'ConceptCell(TEXT("concept-realm-thumbs.png"),I)' in realm and 'Index/3.f+U,V' in u and 'Nav.map==I' in realm,
 'upgrade identity art follows current hero': all(x in upgrade for x in ['concept-upgrade-body-bg.png','concept-upgrade-beast-bg.png','Nav.character==0','Nav.character==2']),
 'title is cropped art and broad slab removed': all(x in upgrade for x in ['concept-upgrade-title.png','15.f/2060.f,75.f/763.f','2045.f/2060.f,700.f/763.f','At(95,90,600,185,']) and 'At(45,100,990,900,' not in upgrade,
 'closed fusion popup has no recipe focus targets': battle.index('if(!bUpgradeOpen || bFusionOpen)')<battle.index('auto Fusions=') and upgrade.index('if(bFusionOpen)return;')<upgrade.index('for(int32 I=0;') and 'if(bFusionOpen)return FReply::Handled();' in confirm and 'bFusionOpen || Run.paused' in select,
 'fusion popup Escape returns without unpausing': 'if(bFusionOpen){bFusionOpen=false;Rebuild();return FReply::Handled();}' in u,
 'bottom dock keeps XP, level and safe readable labels': all(x in uncovered for x in ['At(480,DockY,960,bSmallViewport?200:160,Framed(', 'At(485,bSmallViewport?970:1000,155,bSmallViewport?44:36,SNew(SBox).HAlign(HAlign_Center)[Label(Names[Nav.character],16,Gold)])', 'At(675+I*120,RankY,82,bSmallViewport?44:32,SNew(STextBlock).Font(Font(14))', 'At(660,bSmallViewport?1015:1008,bSmallViewport?420:480,10,SNew(SProgressBar)', 'At(bSmallViewport?1100:1155,bSmallViewport?1000:995,bSmallViewport?320:270,bSmallViewport?44:40,LiveText', 'At(360,78,bSmallViewport?190:170,bSmallViewport?44:38,LiveText']) and 1000+36<=1040 and 974+32<=1040,
 'seal keeps feature and charge gates': 'HasFeature(Owner->CampaignProfile(),Feature::SealSkip)&&Run.progression.sealsRemaining>0' in upgrade,
 'fusion, rest and rewards remain reachable': all(x in u for x in ['for(const auto& FusionRecipe:FusionCatalog())', 'At(95,325,900,590,Framed(Panel(Popup,Ink,32)))', 'RestPanel();', 'ResultPanel();', 'ChooseBossReward(I)', 'Owner->SaveRun()', 'Owner->ResumeRun(']),
}
for name,ok in checks.items():
    assert ok,name
    print('PASS:',name)
# Real atlas bytes, not inferred dimensions or a renamed rectangular screenshot.
manifest={Path(a['path']).name:a for a in json.loads((R/'Art/UIConcepts/assets.json').read_text())}
for name in ['concept-realm-thumbs.png','concept-upgrade-icons.png','concept-portrait-medallions.png']:
    entry=manifest[name]; path=R/entry['path']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==entry['sha256']
    with Image.open(path) as atlas:
        assert atlas.mode=='RGBA' and atlas.width==3*atlas.height
        width=atlas.height
        for i in range(3):
            alpha=atlas.getchannel('A').crop((i*width,0,(i+1)*width,width))
            margin=width//10
            for box in [(0,0,margin,margin),(width-margin,0,width,margin),(0,width-margin,margin,width),(width-margin,width-margin,width,width)]:
                assert alpha.crop(box).getextrema()[1]<=8, (name,i,'opaque corner')
            assert alpha.crop((width//3,width//3,2*width//3,2*width//3)).getextrema()[0]>=240
    print('PASS: manifest-backed transparent three-cell atlas',name)
print('NOT_RUN: Unreal compilation, native interaction, 720p/1080p and110% rendered layout')
