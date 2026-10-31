"""Verify and reconstruct only; never runs a shell/Git or overwrites existing output."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent

def need(ok, message):
    if not ok: raise ValueError(message)

def check(path, size, sha):
    need(not path.is_symlink(), 'Symlink refused: ' + path.name)
    digest = hashlib.sha256()
    total = 0
    with path.open('rb') as f:
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk: break
            total += len(chunk)
            digest.update(chunk)
    need(total == size and digest.hexdigest() == sha, 'Checksum/size mismatch: ' + path.name)

m = json.loads((ROOT / 'manifest.json').read_text(encoding='utf-8'))
need(m['format'] == 1 and m['bundle_name'] == 'art-delta.bundle', 'Unknown format/name')
need(0 < len(m['parts']) < 10000, 'Invalid parts')
for i, part in enumerate(m['parts']):
    need(part['name'] == f'art-delta.bundle.part-{i:03d}', 'Unexpected part basename')
    check(ROOT / part['name'], part['bytes'], part['sha256'])
output = ROOT / 'art-delta.bundle'
if output.exists() or output.is_symlink():
    check(output, m['bytes'], m['sha256'])
    print('Existing bundle verified; no file overwritten.')
else:
    with output.open('xb') as dest:
        for part in m['parts']:
            source = ROOT / part['name']
            need(not source.is_symlink(), 'Symlink part refused')
            data = source.read_bytes()
            need(len(data) == part['bytes'] and hashlib.sha256(data).hexdigest() == part['sha256'], 'Part changed during reconstruction')
            dest.write(data)
    check(output, m['bytes'], m['sha256'])
    print('Bundle reconstructed and verified.')
print('Required base:', m['base_commit'])
print('Target source:', m['target_commit'])
