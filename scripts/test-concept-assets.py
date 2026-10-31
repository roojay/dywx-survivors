#!/usr/bin/env python3
"""Dependency-free identity/PNG structure check; not visual or UE verification."""
import hashlib
import json
from pathlib import Path
import struct
import zlib
root=Path(__file__).resolve().parents[1]
assets=json.loads((root/'Art/UIConcepts/assets.json').read_text(encoding='utf-8'))
assert len(assets)==18
assert {a["path"] for a in assets} == {p.relative_to(root).as_posix() for p in (root/"Content/UI/Images").glob("concept-*.png")}
for a in assets:
    p=root/a['path']; b=p.read_bytes()
    assert len(b)==a['bytes'] and hashlib.sha256(b).hexdigest()==a['sha256'],p
    assert b[:8]==b'\x89PNG\r\n\x1a\n',p
    w,h,depth,color,compression,filtration,interlace=struct.unpack('>IIBBBBB',b[16:29])
    assert (w,h)==(a['width'],a['height']) and depth==8 and color in (2,6),p
    i=8; ended=False
    while i<len(b):
        n=struct.unpack('>I',b[i:i+4])[0];kind=b[i+4:i+8]
        payload=b[i+8:i+8+n];crc=struct.unpack('>I',b[i+8+n:i+12+n])[0]
        assert zlib.crc32(kind+payload)&0xffffffff==crc,p
        i+=n+12
        if kind==b'IEND':ended=True;break
    assert ended and i==len(b),p
    print('PASS:',a['path'],w,h)
print('NOT_RUN: Slate compositing, native layout, interactions, GPU or game appearance')
