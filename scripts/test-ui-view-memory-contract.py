#!/usr/bin/env python3
"""Source regressions for native view restoration; not Slate input/layout evidence."""
from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text()
h=(R/'Source/DywxUI/Public/DywxUIViewMemory.h').read_text()
def body(signature):
    start=s.index('{',s.index(signature));depth=1;end=start+1
    while depth:
        depth+=(s[end]=='{')-(s[end]=='}');end+=1
    return s[start:end]
context=body('FString SDywxRoot::ViewContext')
rebuild=body('void SDywxRoot::Rebuild')
scroll=body('TSharedRef<SScrollBox> SDywxRoot::RememberedScroll')
restore=body('void SDywxRoot::RestoreScrollAfterLayout')
focus=body('TSharedRef<SWidget> SDywxRoot::FocusTarget')
open_modal=body('FReply SDywxRoot::OpenModal')
checks={
 'shop discovery follows accepted modal open or successful explicit refresh only': s.count('DiscoverCurrentShop()')==2 and open_modal.index('Nav.modal=Modal')<open_modal.index('DiscoverCurrentShop()')<open_modal.index('Rebuild()') and 'if(Refreshed)Sim.DiscoverCurrentShop()' in s and 'DiscoverCurrentShop' not in rebuild,
 'context includes screen/modal/run-instance/generation/phase/wave/fusion': all(x in context for x in ['Nav.screen','Nav.modal','Nav.liveBattle','PresentationRunId','Run.generation','Run.phase','Run.wave','bFusionOpen']) and '++PresentationRunId' in s,
 'double rebuild never captures pending restore as zero': 'if(!PendingScrollRestore.Contains(Pair.Key))' in rebuild and 'GetCachedGeometry().GetLocalSize().Y>0' in rebuild,
 'restore clamps against actual content after cached geometry exists': 'GetCachedGeometry().GetLocalSize().Y<=0)continue' in restore and restore.index('GetCachedGeometry()')<restore.index('GetScrollOffsetOfEnd()') and 'ViewMemory.SaveScroll' in restore,
 'stale scroll callbacks ignored and current user gesture wins': 'Epoch=UIEpoch' in scroll and scroll.index('if(Epoch!=UIEpoch)return')<scroll.index('ViewMemory.SaveScroll') and 'PendingScrollRestore.Remove(Key)' in scroll,
 'widget registries reset before new widget registration': 'ScrollTargets.Empty();FocusTargets.Empty();AmbiguousFocusKeys.Empty();PendingScrollRestore.Empty();' in rebuild and rebuild.index('FocusTargets.Empty()')<rebuild.index('SAssignNew(Canvas'),
 'only enabled current focus keys restored; root is fallback': all(x in rebuild for x in ['Widget->IsEnabled() && Widget->SupportsKeyboardFocus()', 'ViewMemory.Focus(Available)', 'SameContext?FocusTargets.Find(Key):nullptr', 'StaticCastSharedRef<SWidget>(SharedThis(this))', 'if(!Widget.IsValid())ViewMemory.SaveFocus("")']),
 'ambiguous labels never regain focus': 'FocusTargets.Remove(Key);AmbiguousFocusKeys.Add(Key)' in focus and '!AmbiguousFocusKeys.Contains(Key)' in focus,
 'scroll panels retain independent names': all('RememberedScroll(TEXT("'+x+'"))' in s for x in ['codex','legacy','economy','rest-choices','equipped','bag']),
 'native image and mode buttons register current semantic focus targets': all('Framed(FocusTarget(FString::Printf(TEXT("'+key+':%d"),I),SNew(SButton)' in s for key in ['hero','realm','mode']),
 'semantic item keys not list indices': all(x in s for x in ['TEXT("bag:%llu")','static_cast<unsigned long long>(Id)','TEXT("economy:good:%llu")','static_cast<unsigned long long>(G.token)','TEXT("economy:reforge:%llu:%llu")','static_cast<unsigned long long>(R.id)']),
 'memory itself cannot invoke gameplay or retain widget pointers': not any(x in h for x in ['Simulation','Session','Upgrade','UObject','TSharedPtr','TWeakPtr']) and all(x in h for x in ['std::map<std::string,double>','std::string context_,focus_']),
 'restore helper does not alter selection or command gameplay': not any(x in restore for x in ['SelectedUpgrade','SelectedBag','Session()','SetUIPaused','ChooseUpgrade']),
}
for label,passed in checks.items():
    assert passed,label
    print('PASS:',label)
print('NOT_RUN: native Slate scroll geometry, Tab focus and repeated rebuild interaction')
