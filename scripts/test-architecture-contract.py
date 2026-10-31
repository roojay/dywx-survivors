#!/usr/bin/env python3
"""Offline module/source boundary gate; not UHT/UBT, DLL-link or runtime evidence."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]
def read(path):
    return (ROOT / path).read_text()
def check(ok, message):
    assert ok, message
    print('PASS:', message)

names = ('Types', 'Simulation', 'Progression', 'Campaign', 'Economy', 'Arena', 'SaveCodec', 'ContentCatalog')
for name in names:
    header = ROOT / f'Source/DywxCore/Public/MVP/{name}.h'
    check(header.is_file(), f'portable domain header: {name}')
    check(not (ROOT / f'Source/DywxUI/Public/MVP/{name}.h').exists(), f'no shadow UI domain header: {name}')
for path in (ROOT / 'Source/DywxCore').rglob('*'):
    if path.suffix not in ('.h', '.cpp') or path.name == 'DywxCoreModule.cpp':
        continue
    source = path.read_text()
    for include in re.findall(r'#include\s+"([^"]+)"', source):
        if path.name == 'Export.h' and include == 'HAL/Platform.h':
            continue
        check(include.startswith('MVP/') and (ROOT / 'Source/DywxCore/Public' / include).is_file(), f'core-only include: {path.name} -> {include}')
    check(not re.search(r'\b(?:UCLASS|USTRUCT|UPROPERTY|UFUNCTION|UObject|FString|TArray)\b', source), f'no UE types: {path.name}')
    if path.suffix == '.h' and path.name != 'Export.h':
        check('#include "MVP/Export.h"' in source, f'explicit export contract: {path.name}')
        for line in source.splitlines():
            if re.match(r'^(?:class|struct) ', line) and line != 'class SaveArchive;':
                check('DYWXCORE_API' in line, f'exported domain type: {line.split()[2]}')
            if re.match(r'^(?:bool|void|float|int|unsigned|std::|const std::|ArenaLayout|WaveSchedule|TrialEffects|SpellConfig|AffixKind)\b.*\(', line):
                raise AssertionError(f'unexported namespace API: {path.name}: {line}')

core = read('Source/DywxCore/DywxCore.Build.cs')
runtime = read('Source/DywxRuntime/DywxRuntime.Build.cs')
ui = read('Source/DywxUI/DywxUI.Build.cs')
check('PublicDependencyModuleNames.Add("Core")' in core and not any(f'"{x}"' in core for x in ('DywxRuntime', 'DywxUI', 'Engine', 'CoreUObject', 'Slate')), 'domain module dependency direction')
check('"DywxCore"' in runtime and not any(f'"{x}"' in runtime for x in ('DywxUI', 'Slate', 'SlateCore', 'ApplicationCore')), 'runtime cannot depend on presentation')
check(all(f'"{x}"' in ui for x in ('DywxCore', 'DywxRuntime')), 'presentation explicitly depends on core/runtime')
project = json.loads(next(ROOT.glob('*.uproject')).read_text())
check({m['Name'] for m in project['Modules']} == {'DywxCore', 'DywxRuntime', 'DywxUI'}, 'three registered runtime modules')
for target in (ROOT / 'Source').glob('*.Target.cs'):
    check(all(f'"{x}"' in target.read_text() for x in ('DywxCore', 'DywxRuntime', 'DywxUI')), f'target modules: {target.name}')
for path in (ROOT / 'Source/DywxRuntime').rglob('*'):
    if path.suffix in ('.h', '.cpp'):
        source = path.read_text()
        check(not any(x in source for x in ('SDywx', 'DywxUIPlayerController', 'DywxMVPWorld', 'FSlateApplication', 'GetFirstPlayerController', 'GetMaxFPS', 'SetMaxFPS')), f'no presentation dependency: {path.name}')
session = read('Source/DywxRuntime/Public/MVP/DywxRunSession.h')
impl = read('Source/DywxRuntime/Private/MVP/DywxRunSession.cpp')
check('class DYWXRUNTIME_API UDywxRunSession' in session and 'class DYWXRUNTIME_API FDywxMVPProfile' in read('Source/DywxRuntime/Public/MVP/DywxMVPProfile.h'), 'runtime exported adapter APIs')
check('const dywx::mvp::Simulation* GetSimulation() const' in session, 'runtime offers read-only simulation view')
check(all(x in impl for x in ('EWorldType::Game', 'EWorldType::PIE', 'NM_Standalone')), 'runtime world/network whitelist')
world = read('Source/DywxUI/Private/MVP/DywxMVPWorld.cpp')
check(not any(x in world for x in ('Simulation.Step', 'Profile.Save', 'GetFirstPlayerController', 'FSlateApplication', 'GetMaxFPS', 'SetMaxFPS')), 'world actor is presentation only')
check(sum(p.read_text().count('Simulation.Step(') for p in (ROOT/'Source').rglob('*.cpp')) == 1, 'single simulation integration owner')
print('NOT_RUN: UHT/UBT modular DLL exports, UE lifecycle ordering, packaged gameplay')
