"""Verify checkpoint parts and reconstruct source.bundle; never runs Git or a shell."""
import hashlib
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parent
manifest = json.loads((ROOT / 'manifest.json').read_text(encoding='utf-8'))
if not (manifest['format'] == 1 and manifest['bundle_name'] == 'source.bundle'):
    raise ValueError('Invalid checkpoint manifest or data')
parts = manifest['parts']
if not 0 < len(parts) < 10000:
    raise ValueError('Invalid checkpoint manifest or data')
for i, part in enumerate(parts):
    if not part['name'] == f'source.bundle.part-{i:03d}':
        raise ValueError('Unexpected part basename')
    path = ROOT / part['name']
    if not not path.is_symlink():
        raise ValueError('Symlink part refused')
    data = path.read_bytes()
    if not len(data) == part['bytes']:
        raise ValueError(f'Wrong size: {path.name}')
    if not hashlib.sha256(data).hexdigest() == part['sha256']:
        raise ValueError(f'Wrong SHA256: {path.name}')
output = ROOT / 'source.bundle'
if output.exists() or output.is_symlink():
    if not not output.is_symlink():
        raise ValueError('Symlink output refused')
    data = output.read_bytes()
    if not (len(data) == manifest['bytes'] and hashlib.sha256(data).hexdigest() == manifest['sha256']):
        raise ValueError('Existing different output refused; move it manually first')
    print('Existing source.bundle already matches; nothing overwritten.')
else:
    with output.open('xb') as dest:
        digest = hashlib.sha256()
        total = 0
        for part in parts:
            data = (ROOT / part['name']).read_bytes()
            if not (len(data) == part['bytes'] and hashlib.sha256(data).hexdigest() == part['sha256']):
                raise ValueError('Part changed during restore')
            dest.write(data)
            digest.update(data)
            total += len(data)
        if not (total == manifest['bytes'] and digest.hexdigest() == manifest['sha256']):
            raise ValueError('Bundle checksum mismatch')
    print('Restored and verified source.bundle:', total, 'bytes;', digest.hexdigest())
print('Target source commit:', manifest['target_commit'])
