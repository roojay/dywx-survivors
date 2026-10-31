#!/usr/bin/env python3
"""Offline bridge/source/font contracts; expressly NOT an Unreal compile/render test."""
from pathlib import Path
import json,re
from fontTools.ttLib import TTFont
R=Path(__file__).resolve().parents[1]
u=(R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text()
c=(R/'Source/DywxUI/Private/DywxUIPlayerController.cpp').read_text()
for label,condition in {
 'new run clears transient old-run display':all(x in u for x in ['Run=dywx::mvp::Snapshot{}; BossChangeUntil=0;', 'bBeastVisible=false; BeastOffset={}; SelectedBag=0; SelectedSlot=0;', 'bRefinePreview=false; RunNotice.Empty();']),
 'death result consumes authoritative damage facts':all(x in u for x in ['Run.lastDamage=State.lastDamage', 'Run.phase==RunPhase::Dead && Run.lastDamage.lethal', 'dywx::DamageLabel(Run.lastDamage)', 'Run.lastDamage.hpLost, Run.lastDamage.shieldLost', 'dywx::DamageAdvice(Run.lastDamage)']),
 'failed retry retains current result':'if (!Session()->IsRunActive()) RootWidget->ReturnToMenu();' in c and 'Paragraph(RunNotice,800,19,Gold)' in u,
 'full game transactions':all(x in u for x in ['Sim.BuyGood','Sim.RefreshShop','Sim.BeginReforge','Sim.ResolveReforge','Sim.IdentifyRelic','Sim.RevealTreasure','ChooseOpportunity','SealUpgrade','SkipUpgrade','Owner->SaveRun','Owner->ResumeRun','Owner->PurchaseLegacy','Owner->PurchasePermanent']),
 'save and leave only after success':'if(Owner.IsValid()&&Owner->SaveRun())' in u and '保存并离开' in u and 'Nav.savedRuns[Nav.character]' in u and 'Session()->HasSavedRun(CharacterIndex)' in c,
 'pause save guard rejects unresolved encounters and transactions':all(x in u for x in ['if(C.phase==CampaignPhase::OpportunityCombat)return TEXT("大妖机缘尚未完成', 'C.phase!=CampaignPhase::Intermission || !C.opportunityResolved', '!SaveSim->GetSnapshot().progression.bossRewards.empty()', 'SaveSim->GetEconomy().HasTreasureToReveal()', 'SaveSim->GetEconomy().PendingReforge()', 'false,CanSaveRun,SaveHint', 'Add(Paragraph(SaveHint,596,20,Muted),85)']),
 'pause save preserves pending upgrades':'pendingUpgrades' not in u.split('const FString SaveBlockReason=')[1].split('const bool CanSaveRun=')[0] and '待选顿悟可以保存' in u,
 'first-layer cave identification reachable':'dywx::CanOpenRestServices(' in u and 'false,Cave,TEXT("仅所选洞府可祭炼' in u and 'Layer2&&Cave' in u,
 'data-driven opportunities and anomaly rules':'Sim->DescribeOpportunity(I)' in u and 'DescribeAnomaly(' in u and 'AnomalyDescription(' not in u and 'GetArena().cave)].bossName' in u,
 'all hero fusion recipes remain selectable':'for(const auto& FusionRecipe:FusionCatalog())' in u and 'const SpellId Fusion=FusionRecipe.result' in u and 'Fusions->AddSlot()' in u and 'Generation=Run.generation' in u,
 'explicit ready fusion and tier-aware opportunity guidance':'CanFuseNow?TEXT("可破境 · ")' in u and 'HasFeature(Owner->CampaignProfile(),Feature::Chest)?' in u and '宝匣在传统首胜后开放' in u,
 'elite combat HUD is distinct from normal wave countdown':all(x in u for x in ['auto IsOpportunityCombat=[this]', 'Sim->IsCampaign() && Run.phase==RunPhase::Wave', 'Sim->GetCampaign().GetState().phase==CampaignPhase::OpportunityCombat', '机缘 · 大妖    第 %d 波', '击败后继续整理']) and u.count('[this,IsOpportunityCombat]')==2,
 'elite preparation has an explicit battle action':'const bool EliteReady=CampaignState.phase==CampaignPhase::OpportunityCombat' in u and '迎战大妖 · 炼化修为入账' in u and '迎战下一波 · 炼化修为入账' in u,
 'local ranking uses authoritative order':'I<P.endlessRanking.size()' in u and 'const auto& Record=P.endlessRanking[I]' in u and '本地，不联网' in u,
 'active anomaly HUD with numeric tooltip':'auto AnomalyText=' in u and 'A.remaining' in u and '.Visibility_Lambda' in u and 'S->GetAnomalies().State().active' in u,
 'authoritative complete codex rules':all(x in u for x in ['Simulation::DescribeEnemy','Simulation::DescribeMutation','Simulation::DescribeElementInteractions','Simulation::DescribeBossMechanics(CaveId(I))','seenBosses[I]','Odds.DescribeDropOdds(false)','Odds.DescribeDropOdds(true)']),
 'shared full stat colored hover comparison':'Comparison(Current,Compare,Tip)' in u and 'Comparison(Run.progression.build.stats,P.PreviewEquip' in u and 'TooltipContent.ToSharedRef()' in u and 'Before.critMultiplier' in u and 'Before.dashCooldown,After.dashCooldown,true' in u,
 'phase notice uses paused simulation clock':'BossChangeUntil=State.statistics.elapsed+4.f' in u and 'Run.statistics.elapsed<BossChangeUntil' in u,
 'no quick-only retry':'RequestRun(LastHero,LastMap,LastMode,LastRisk)' in c,
 'player lore preserved':'科技为里' not in u,
 'world bridge':all(x in c for x in ['SpawnActor<ADywxMVPWorld>','Session()->StartFullRun','Session()->SetInput(Input)','MVPWorld->StopRun()', 'InitializePresentation(RunSession, this)']),
 'six spell four passive slots':'I<6' in u and 'I<4' in u and 'Run.spellCooldowns' in u,
 'catalog not legacy three skills':'SkillNames' not in u and 'SkillArt' not in u and 'GetSpellConfig' in u,
 'upgrade not modal':'void SDywxRoot::UpgradePanel()' in u and 'Run.progression.offers' in u,
 'inventory controls':all(x in u for x in ['PreviewRefine','UndoRefine','PreviewEquip','SetLocked','SortBag','FilterBag','PendingRefineXp']),
 'stale widget guard':'Epoch == UIEpoch ? Fn()' in u,
 'snapshot copy excludes entity arrays':'Run.enemies=State.enemies' not in u,
 'movement and immediate choices':all(x in c for x in ['EKeys::W','EKeys::D','EKeys::Up','EKeys::Right','UpgradeOne','UpgradeThree']),
 'bounded HUD refresh':'HUDRefreshElapsed >= .05f' in c,
 'playable fusion action':all(x in u for x in ['Sim->CanFuse(Fusion)', 'Commands->Fuse(Fusion)', 'Now.generation != Generation || Now.paused', 'Run.phase == RunPhase::Rest || !Run.progression.offers.empty()', 'Now.phase != RunPhase::Wave && Now.phase != RunPhase::Boss && Now.phase != RunPhase::Rest', 'At(1360,120,510,100,Fusions)']),
 'codex saved view without world spawn':all(x in u for x in ['FDywxMVPProfile Codex;', 'Codex.Load();', 'Codex.Observe(', 'Codex.GetKnown()']) and 'Codex.Save()' not in u,
 'codex grey and lit complete list':all(x in u for x in ['Seen ? Paper : Muted', 'Known.seenSpells', 'Known.seenFusions[I]', 'FusionCatalog()', 'Known.seenPassives', 'Known.seenRelics', 'Known.seenEnemies', 'Known.seenMutations', 'Known.seenResonances']),
}.items():
 assert condition,label
 print('PASS:',label)
for path in ['Source/DywxUI/Private/SDywxRoot.cpp','Source/DywxUI/Private/DywxUIPlayerController.cpp']:
 s=(R/path).read_text();s=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"','',s,flags=re.S);stack=[]
 for char in s:
  if char in '([{':stack.append(char)
  if char in ')]}':assert stack and stack.pop()=={')':'(',']':'[','}':'{'}[char],path
 assert not stack,path
feedback=(R/'Source/DywxUI/Public/DywxCombatFeedback.h').read_text()
chars={x for x in u+c+feedback+(R/'Source/DywxCore/Private/MVP/ContentCatalog.cpp').read_text() if 0x3000<=ord(x)<=0x9fff}
for style in ['Regular','Bold']:
 font=TTFont(R/f'Content/UI/Fonts/SarasaUiSC-{style}.ttf');assert all(ord(x) in font.getBestCmap() for x in chars),style
print(f'PASS: balanced source delimiters and {len(chars)} CJK glyphs in both actual Sarasa fonts')
print('NOT_RUN: UHT/C++ Unreal build, Slate input/layout, packaged gameplay, GPU/FPS')
