#!/usr/bin/env python3
"""Offline bridge/source/font contracts; expressly NOT an Unreal compile/render test."""
from pathlib import Path
import json,re
from fontTools.ttLib import TTFont
R=Path(__file__).resolve().parents[1]
u=(R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text()
c=(R/'Source/DywxUI/Private/DywxUIPlayerController.cpp').read_text()
for label,condition in {
 'world bridge':all(x in c for x in ['SpawnActor<ADywxMVPWorld>','MVPWorld->StartRun','MVPWorld->SetInput(Input)','MVPWorld->StopRun()']),
 'six spell four passive slots':'I<6' in u and 'I<4' in u and 'Run.spellCooldowns' in u,
 'catalog not legacy three skills':'SkillNames' not in u and 'SkillArt' not in u and 'GetSpellConfig' in u,
 'upgrade not modal':'void SDywxRoot::UpgradePanel()' in u and 'Run.progression.offers' in u,
 'inventory controls':all(x in u for x in ['PreviewRefine','UndoRefine','PreviewEquip','SetLocked','SortBag','FilterBag','PendingRefineXp']),
 'stale widget guard':'Epoch == UIEpoch ? Fn()' in u,
 'snapshot copy excludes entity arrays':'Run.enemies=State.enemies' not in u,
 'movement and immediate choices':all(x in c for x in ['EKeys::W','EKeys::D','EKeys::Up','EKeys::Right','UpgradeOne','UpgradeThree']),
 'bounded HUD refresh':'HUDRefreshElapsed >= .05f' in c,
 'playable fusion action':all(x in u for x in ['P.CanFuse(Fusion)', 'P.Fuse(Fusion)', 'Now.generation != Generation || Now.paused', 'Now.phase == RunPhase::Rest || !Now.progression.offers.empty()', 'S->RefreshProgression()', 'At(1360,130,510,80,']),
 'codex saved view without world spawn':all(x in u for x in ['FDywxMVPProfile Codex;', 'Codex.Load();', 'Codex.Observe(', 'Codex.GetKnown()']) and 'Codex.Save()' not in u,
 'codex grey and lit complete list':all(x in u for x in ['Seen ? Paper : Muted', 'Known.seenSpells', 'Known.seenFusions[0]', 'Known.seenFusions[1]', 'Known.seenPassives', 'Known.seenRelics', 'Known.seenEnemies', 'Known.seenMutations', 'Known.seenResonances']),
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
