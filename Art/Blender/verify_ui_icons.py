"""Check rendered UI PNGs. Requires Pillow; does not modify any image."""
from pathlib import Path
from PIL import Image
import hashlib
import json

root = Path(__file__).resolve().parents[2]
results = []
for name in ['slash', 'orbit', 'arrow', 'dash', 'stone', 'mirror']:
    path = root / 'Content' / 'UI' / 'Images' / ('icon-' + name + '.png')
    with Image.open(path) as image:
        assert image.mode == 'RGBA', (name, image.mode)
        assert image.size == (512, 512), (name, image.size)
        alpha = image.getchannel('A')
        minimum, maximum = alpha.getextrema()
        assert minimum == 0 and maximum == 255, (name, minimum, maximum)
        bbox = alpha.getbbox()
        assert bbox and min(bbox[0], bbox[1], 512-bbox[2], 512-bbox[3]) >= 12, (name, bbox)
        count = sum(alpha.histogram()[1:])
        assert 5000 < count < 220000, (name, count)
        results.append({'file': path.relative_to(root).as_posix(), 'mode': image.mode,
                        'size': image.size, 'alpha_bbox': bbox, 'nonzero_alpha_pixels': count,
                        'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
print(json.dumps({'status': 'passed', 'icons': results}, ensure_ascii=False, indent=2))
