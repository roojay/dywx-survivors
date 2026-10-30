#!/usr/bin/env python3
"""Static source contracts + gait arithmetic model; does not execute/compile Unreal."""
from pathlib import Path
import math
ROOT=Path(__file__).resolve().parents[3]
s=(ROOT/'Source/DywxUI/Private/MVP/DywxMVPWorld.cpp').read_text()
h=(ROOT/'Source/DywxUI/Public/MVP/DywxMVPWorld.h').read_text()
assert 'TMap<uint64,float> SwarmWalkPhases' in h
assert 'HeroWalkPhase+HeroDistance/150.f' in s
assert '*ExistingPhase+Distance/(110.f*Scale)' in s
assert 'if(HeroMoving) HeroWalkPhase=' in s and 'Dt>0 && HeroDistance>.1f' in s
assert 'if(Dt>0 && Distance>.1f)' in s
assert 'float(E.id%8)/8.f' in s
assert 'if(!CurrentEnemies.Contains(It.Key())) It.RemoveCurrent()' in s
assert s.count('HeroWalkPhase=0; SwarmWalkPhases.Reset()')==2
assert 'FMath::Sin(HeroWalkPhase*2*PI)' in s
assert 'FMath::FloorToInt(VisualTime*10)' not in s
assert 'while(B.Allocated>Count+8)' in s
assert 'Entries->Num()<=64' in s
# Model the exact positive-distance normalized phase arithmetic used above.
def advance(phase,distance,stride,dt):
 return math.fmod(phase+distance/stride,1) if dt>0 and distance>.1 else phase
for stride in (150,110,110*1.55):
 assert abs(advance(.125,stride,stride,1)-.125)<1e-9
 assert advance(.3,0,stride,1)==.3
 assert advance(.3,50,stride,0)==.3
 a=advance(.2,90,stride,1)
 b=.2
 for _ in range(9):b=advance(b,10,stride,1/9)
 assert abs(a-b)<1e-9
print('PASS: distance-linked gait source contracts, stride arithmetic, idle/pause freeze model, run/death cleanup, bounded instance slack')
print('NOT_RUN: Unreal compilation, actual movement/render integration, GPU performance')
