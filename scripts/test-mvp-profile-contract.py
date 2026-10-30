#!/usr/bin/env python3
"""Source contract only: UE file replacement/fault injection remains NOT_RUN."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
profile = (root / 'Source/DywxUI/Private/MVP/DywxMVPProfile.cpp').read_text()
codec = (root / 'Source/DywxUI/Private/MVP/SaveCodec.cpp').read_text()
for field in ('Spells', 'Relics', 'Fusions', 'Passives', 'Enemies', 'Mutations', 'Resonances'):
    assert f'ReadFlags(*Json,TEXT("{field.lower()}"),Known.seen{field})' in profile
    assert f'merge(data_.known.seen{field},s.seen{field})' in codec
assert 'Version!=1' in profile
assert 'if(!Json.HasField(Field))return true;' in profile
assert 'bWritable=false;' in profile and 'return bWritable&&Store.Save()' in profile
assert 'profile-v2.dat' in profile and 'profile.json' in profile
assert 'Store.Load(Blob)' in profile and 'Store.ImportLegacy(Known)' in profile
assert 'FFileHelper::SaveArrayToFile(Data,*Temporary)' in profile
assert 'IFileManager::Get().Move(*Path,*Temporary' in profile
assert 'return !dirty_||Commit(data_)' in codec
assert 'if(!Commit(std::move(next)))return false;out=std::move(blob)' in codec
print('PASS: v1 optional flags migration, v2 bounded codec, dirty/write guards and transactional consume source contract')
print('NOT_RUN: Unreal compile, restart/file IO, atomic replacement and power-loss fault injection')
