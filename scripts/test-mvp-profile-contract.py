#!/usr/bin/env python3
"""Source contract only: real UE JSON/disk round-trip remains NOT_RUN."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
profile = (root / "Source/DywxUI/Private/MVP/DywxMVPProfile.cpp").read_text()
for field in ("Spells", "Relics", "Fusions", "Passives", "Enemies", "Mutations", "Resonances"):
    assert f'ReadFlags(*Json, TEXT("{field.lower()}"), Known.seen{field});' in profile
    assert f'WriteFlags(*Json, TEXT("{field.lower()}"), Known.seen{field});' in profile
    assert f'MergeFlags(Known.seen{field}, State.seen{field});' in profile
assert 'Version != 1' in profile
assert '!Json.TryGetArrayField(Field, Values)' in profile  # Optional fields, old v1 readable.
assert 'if (!bDirty)' in profile
assert 'State.build' not in profile and 'State.bag' not in profile
print('PASS: optional v1 discovery fields, OR merge and no build/inventory persistence source contract')
print('NOT_RUN: Unreal JSON serialization, restart/disk IO and Codex visual state')
