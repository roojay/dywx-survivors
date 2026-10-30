#!/usr/bin/env python3
"""Offline bridge/source/font contracts; expressly NOT an Unreal compile/render test."""
from pathlib import Path
import json,re
from fontTools.ttLib import TTFont
R=Path(__file__).resolve().parents[1]
u=(R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text()
c=(R/'Source/DywxUI/Private/DywxUIPlayerController.cpp').read_text()
for label,condition in {
 'full game transactions':all(x in u for x in ['Sim.BuyGood','Sim.RefreshShop','Sim.BeginReforge','Sim.ResolveReforge','Sim.IdentifyRelic','Sim.RevealTreasure','ChooseOpportunity','SealUpgrade','SkipUpgrade','Owner->SaveRun','Owner->ResumeRun','Owner->PurchaseLegacy','Owner->PurchasePermanent']),
 'save and leave only after success':'if(Owner.IsValid()&&Owner->SaveRun())' in u and '保存并离开' in u and 'Nav.savedRuns[Nav.character]' in u and 'MVPWorld->HasSavedRun(CharacterIndex)' in c,
 'first-layer cave identification reachable':'dywx::CanOpenRestServices(' in u and 'false,Cave,TEXT("仅所选洞府可祭炼' in u and 'Layer2&&Cave' in u,
 'data-driven opportunities and anomaly rules':'Sim->DescribeOpportunity(I)' in u and 'DescribeAnomaly(' in u and 'AnomalyDescription(' not in u and 'GetArena().cave)].bossName' in u,
 'all hero fusion recipes remain selectable':'for(const auto& FusionRecipe:FusionCatalog())' in u and 'const SpellId Fusion=FusionRecipe.result' in u and 'Fusions->AddSlot()' in u and 'Generation=Run.generation' in u,
 'local ranking uses authoritative order':'I<P.endlessRanking.size()' in u and 'const auto& Record=P.endlessRanking[I]' in u and '本地，不联网' in u,
 'active anomaly HUD with numeric tooltip':'auto AnomalyText=' in u and 'A.remaining' in u and '.Visibility_Lambda' in u and 'S->GetAnomalies().State().active' in u,
 'authoritative complete codex rules':all(x in u for x in ['Simulation::DescribeEnemy','Simulation::DescribeMutation','Simulation::DescribeElementInteractions','Simulation::DescribeBossMechanics(CaveId(I))','seenBosses[I]','Odds.DescribeDropOdds(false)','Odds.DescribeDropOdds(true)']),
 'shared full stat colored hover comparison':'Comparison(Current,Compare,Tip)' in u and 'Comparison(Run.progression.build.stats,P.PreviewEquip' in u and 'TooltipContent.ToSharedRef()' in u and 'Before.critMultiplier' in u and 'Before.dashCooldown,After.dashCooldown,true' in u,
 'phase notice uses paused simulation clock':'BossChangeUntil=State.statistics.elapsed+4.f' in u and 'Run.statistics.elapsed<BossChangeUntil' in u,
 'no quick-only retry':'RequestRun(LastHero,LastMap,LastMode,LastRisk)' in c,
 'player lore preserved':'科技为里' not in u,
 'world bridge':all(x in c for x in ['SpawnActor<ADywxMVPWorld>','MVPWorld->StartFullRun','MVPWorld->SetInput(Input)','MVPWorld->StopRun()']),
 'six spell four passive slots':'I<6' in u and 'I<4' in u and 'Run.spellCooldowns' in u,
 'catalog not legacy three skills':'SkillNames' not in u and 'SkillArt' not in u and 'GetSpellConfig' in u,
 'upgrade not modal':'void SDywxRoot::UpgradePanel()' in u and 'Run.progression.offers' in u,
 'inventory controls':all(x in u for x in ['PreviewRefine','UndoRefine','PreviewEquip','SetLocked','SortBag','FilterBag','PendingRefineXp']),
 'stale widget guard':'Epoch == UIEpoch ? Fn()' in u,
 'snapshot copy excludes entity arrays':'Run.enemies=State.enemies' not in u,
 'movement and immediate choices':all(x in c for x in ['EKeys::W','EKeys::D','EKeys::Up','EKeys::Right','UpgradeOne','UpgradeThree']),
 'bounded HUD refresh':'HUDRefreshElapsed >= .05f' in c,
 'playable fusion action':all(x in u for x in ['P.CanFuse(Fusion)', 'P.Fuse(Fusion)', 'Now.generation != Generation || Now.paused', 'Now.phase == RunPhase::Rest || !Now.progression.offers.empty()', 'S->RefreshProgression()', 'At(1360,120,510,100,Fusions)']),
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
chars={x for x in u+c+(R/'Source/DywxUI/Private/MVP/ContentCatalog.cpp').read_text() if 0x3000<=ord(x)<=0x9fff}
for style in ['Regular','Bold']:
 font=TTFont(R/f'Content/UI/Fonts/SarasaUiSC-{style}.ttf');assert all(ord(x) in font.getBestCmap() for x in chars),style
print(f'PASS: balanced source delimiters and {len(chars)} CJK glyphs in both actual Sarasa fonts')
print('NOT_RUN: UHT/C++ Unreal build, Slate input/layout, packaged gameplay, GPU/FPS')
