#!/usr/bin/env python3
"""Compact HUD source/asset contracts; not Unreal compilation or rendered evidence."""
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parents[1]
s=(R/'Source/DywxUI/Private/SDywxRoot.cpp').read_text()
battle=s.split('void SDywxRoot::BattleScreen()')[1].split('FReply SDywxRoot::RunCommand')[0]
checks={
    'slender framed health and experience': all(x in battle for x in ['At(24,20,400,32,','SNew(SCombatOrnament)','At(50,57,348,3,']),
    'six centered compact image slots': 'I<6' in battle and 'At(696+I*88,970,80,80,' in battle and '.Image_Lambda' in battle,
    'numeric details not discarded': all(x in battle for x in ['Details->AddSlot()', 'Run.player.shield', 'R.fire,R.thunder', 'Run.statistics.damageTaken', 'I<4', 'HUD.ExperienceToNext', 'Run.spellCooldowns', '.ToolTip(Tip)']),
    'anomaly tooltip retains complete rules': 'AnomalyText()+TEXT("\\n")' in battle and 'DescribeAnomaly(' in battle,
    'fusion only during actionable phases': 'if (Run.phase == RunPhase::Rest || !Run.progression.offers.empty())' in battle and 'Now.generation != Generation || Now.paused' in battle,
    'atlas cropped and strongly owned': all(x in s for x in ['float(Cell%3)/3.f','float(Cell/3)/2.f','AtlasBrush->GetResourceObject()', 'Textures.Emplace(Texture)', 'Brushes.Add(Key,MoveTemp(Brush))']),
    'font and resolution scaling preserved': 'SarasaUiSC-Regular.ttf' in s and 'WidthOverride(1920).HeightOverride(1080)' in s and 'EStretch::ScaleToFit' in s,
}
for label,ok in checks.items():
    assert ok,label
    print('PASS:',label)
for name in ['icon-slash.png','icon-arrow.png','icon-orbit.png','icon-stone.png','icon-dash.png','icon-mirror.png']:
    with Image.open(R/'Content/UI/Images'/name) as im: im.verify()
print('PASS: all six fallback glyph files decode')
a=R/'Content/UI/Images/hud-skills.png'
if a.exists():
    with Image.open(a) as im:
        assert im.width*2==im.height*3, f'Atlas must have square 3x2 cells, got {im.size}'
        assert im.width>=768 and im.height>=512
        assert im.mode=='RGBA' and im.getextrema()[3][0]<255, 'Transparent medallion gutters required'
        # Inspect each cell, not merely a single transparent pixel in the atlas.
        w,h=im.width//3,im.height//2
        for row in range(2):
            for col in range(3):
                alpha=im.getchannel('A').crop((col*w,row*h,(col+1)*w,(row+1)*h))
                hist=alpha.histogram()
                assert sum(hist[:16])/(w*h)>.25, f'Cell {col,row} has opaque gutters'
                assert sum(hist[192:])/(w*h)>.45, f'Cell {col,row} has missing medallion'
                corner=max(4,w//8)
                for box in [(0,0,corner,corner),(w-corner,0,w,corner),(0,h-corner,corner,h),(w-corner,h-corner,w,h)]:
                    assert alpha.crop(box).getextrema()[1]<=4, f'Cell {col,row} corner background leak'
        print('PASS: transparent 3x2 medallion atlas and all six clean cell gutters',im.size)
else:
    print('PENDING: generated atlas absent; runtime uses checked-in glyph fallback')
print('NOT_RUN: Unreal compile, 720p/1080p Slate render, hover and gameplay interaction')
